/**
 * @file DynamicCorridorReleaseManager.cpp
 * @brief Implementation of Phase 3 Dynamic Corridor Release Manager for SMART AEGIS.
 */

#include "aegis/corridor/DynamicCorridorReleaseManager.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>

namespace aegis {
namespace corridor {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "DYN_RELEASE", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "DYN_RELEASE", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "DYN_RELEASE", msg);
}

DynamicCorridorReleaseManager::DynamicCorridorReleaseManager(const DynamicReleaseConfig& config)
    : m_config(config) {}

uint64_t DynamicCorridorReleaseManager::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

bool DynamicCorridorReleaseManager::LockIntersection(const std::string& corridorId,
                                                    const std::string& ambulanceId,
                                                    const std::string& intersectionId,
                                                    double yellowSec,
                                                    double allRedSec,
                                                    uint64_t timestampUs) {
    if (corridorId.empty() || intersectionId.empty()) {
        return false;
    }

    SignalActionCallback localCallback = nullptr;
    std::string cmd = "";

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
        localCallback = m_signalCallback;

        auto it = m_intersections.find(intersectionId);
        if (it != m_intersections.end()) {
            if (it->second.state == DynamicReleaseState::LOCKED_EMERGENCY) {
                // If already locked by another vehicle, enqueue reservation
                if (it->second.holdingCorridorId != corridorId) {
                    it->second.queuedCorridorClaims.push_back(corridorId);
                    LogWarn("Intersection " + intersectionId +
                            " already locked by corridor " + it->second.holdingCorridorId +
                            "; enqueued claim from corridor " + corridorId);
                    return true;
                }
                return true; // Already locked for this corridor
            }
        }

        IntersectionReleaseRecord rec;
        rec.intersectionId = intersectionId;
        rec.holdingCorridorId = corridorId;
        rec.holdingAmbulanceId = ambulanceId;
        rec.state = DynamicReleaseState::LOCKED_EMERGENCY;
        rec.lockedTimestampUs = nowUs;
        rec.departureTimestampUs = 0;
        rec.yellowStartTimestampUs = 0;
        rec.allRedStartTimestampUs = 0;
        rec.releasedTimestampUs = 0;
        rec.customYellowSec = (yellowSec > 0.0) ? yellowSec : m_config.yellowDurationSec;
        rec.customAllRedSec = (allRedSec > 0.0) ? allRedSec : m_config.allRedClearanceSec;

        m_intersections[intersectionId] = std::move(rec);

        DynamicReleaseEvent ev;
        ev.intersectionId = intersectionId;
        ev.corridorId = corridorId;
        ev.ambulanceId = ambulanceId;
        ev.fromState = DynamicReleaseState::IDLE_NOMINAL;
        ev.toState = DynamicReleaseState::LOCKED_EMERGENCY;
        ev.timestampUs = nowUs;
        ev.transitionDurationMs = 0.0;
        ev.actionSummary = "LOCKED_FOR_EMERGENCY_APPROACH";
        m_events.push_back(ev);

        cmd = "PREEMPTION_GREEN";
        LogInfo("Intersection " + intersectionId + " LOCKED for ambulance " + ambulanceId);
    }

    if (localCallback) {
        localCallback(intersectionId, cmd);
    }
    return true;
}

bool DynamicCorridorReleaseManager::OnAmbulancePassed(const std::string& corridorId,
                                                      const std::string& intersectionId,
                                                      uint64_t departureTimestampUs) {
    SignalActionCallback localCallback = nullptr;
    std::string cmd = "";

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_intersections.find(intersectionId);
        if (it == m_intersections.end() || it->second.state != DynamicReleaseState::LOCKED_EMERGENCY) {
            return false;
        }

        if (it->second.holdingCorridorId != corridorId) {
            return false; // Mismatched corridor claim
        }

        uint64_t nowUs = (departureTimestampUs > 0) ? departureTimestampUs : GetCurrentTimeUs();
        localCallback = m_signalCallback;

        it->second.departureTimestampUs = nowUs;
        it->second.yellowStartTimestampUs = nowUs;
        it->second.state = DynamicReleaseState::CLEARING_YELLOW;

        DynamicReleaseEvent ev;
        ev.intersectionId = intersectionId;
        ev.corridorId = corridorId;
        ev.ambulanceId = it->second.holdingAmbulanceId;
        ev.fromState = DynamicReleaseState::LOCKED_EMERGENCY;
        ev.toState = DynamicReleaseState::CLEARING_YELLOW;
        ev.timestampUs = nowUs;
        ev.transitionDurationMs = (nowUs >= it->second.lockedTimestampUs)
                                      ? static_cast<double>(nowUs - it->second.lockedTimestampUs) / 1000.0
                                      : 0.0;
        ev.actionSummary = "AMBULANCE_PASSED_INITIATING_YELLOW_CLEARANCE";
        m_events.push_back(ev);

        cmd = "CLEARANCE_YELLOW";
        LogInfo("Ambulance passed " + intersectionId +
                "; starting dynamic yellow clearance (" +
                std::to_string(it->second.customYellowSec) + "s)");
    }

    if (localCallback) {
        localCallback(intersectionId, cmd);
    }
    return true;
}

size_t DynamicCorridorReleaseManager::ProcessTransitions(uint64_t currentTimestampUs) {
    std::vector<std::pair<std::string, std::string>> pendingCommands;
    size_t transitionsCount = 0;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();

        for (auto& pair : m_intersections) {
            auto& rec = pair.second;

            // 1. CLEARING_YELLOW -> ALL_RED_CLEARANCE
            if (rec.state == DynamicReleaseState::CLEARING_YELLOW) {
                if (nowUs >= rec.yellowStartTimestampUs) {
                    double yellowElapsedSec = static_cast<double>(nowUs - rec.yellowStartTimestampUs) / 1000000.0;
                    if (yellowElapsedSec >= rec.customYellowSec) {
                        rec.state = DynamicReleaseState::ALL_RED_CLEARANCE;
                        rec.allRedStartTimestampUs = nowUs;

                        DynamicReleaseEvent ev;
                        ev.intersectionId = rec.intersectionId;
                        ev.corridorId = rec.holdingCorridorId;
                        ev.ambulanceId = rec.holdingAmbulanceId;
                        ev.fromState = DynamicReleaseState::CLEARING_YELLOW;
                        ev.toState = DynamicReleaseState::ALL_RED_CLEARANCE;
                        ev.timestampUs = nowUs;
                        ev.transitionDurationMs = yellowElapsedSec * 1000.0;
                        ev.actionSummary = "YELLOW_COMPLETED_INITIATING_ALL_RED_CLEARANCE";
                        m_events.push_back(ev);

                        pendingCommands.push_back({rec.intersectionId, "ALL_RED"});
                        transitionsCount++;

                        LogInfo("Intersection " + rec.intersectionId +
                                " yellow clear complete; starting all-red safety buffer (" +
                                std::to_string(rec.customAllRedSec) + "s)");
                    }
                }
            }
            // 2. ALL_RED_CLEARANCE -> RELEASED_NOMINAL
            else if (rec.state == DynamicReleaseState::ALL_RED_CLEARANCE) {
                if (nowUs >= rec.allRedStartTimestampUs) {
                    double allRedElapsedSec = static_cast<double>(nowUs - rec.allRedStartTimestampUs) / 1000000.0;
                    if (allRedElapsedSec >= rec.customAllRedSec) {
                        // Check if another corridor is queued for this intersection
                        if (!rec.queuedCorridorClaims.empty()) {
                            std::string nextCorridor = rec.queuedCorridorClaims.front();
                            rec.queuedCorridorClaims.erase(rec.queuedCorridorClaims.begin());
                            rec.holdingCorridorId = nextCorridor;
                            rec.state = DynamicReleaseState::LOCKED_EMERGENCY;
                            rec.lockedTimestampUs = nowUs;

                            DynamicReleaseEvent ev;
                            ev.intersectionId = rec.intersectionId;
                            ev.corridorId = nextCorridor;
                            ev.ambulanceId = rec.holdingAmbulanceId;
                            ev.fromState = DynamicReleaseState::ALL_RED_CLEARANCE;
                            ev.toState = DynamicReleaseState::LOCKED_EMERGENCY;
                            ev.timestampUs = nowUs;
                            ev.transitionDurationMs = allRedElapsedSec * 1000.0;
                            ev.actionSummary = "HANDOFF_TO_QUEUED_EMERGENCY_CORRIDOR";
                            m_events.push_back(ev);

                            pendingCommands.push_back({rec.intersectionId, "PREEMPTION_GREEN"});
                            transitionsCount++;
                        } else {
                            rec.state = DynamicReleaseState::RELEASED_NOMINAL;
                            rec.releasedTimestampUs = nowUs;

                            DynamicReleaseEvent ev;
                            ev.intersectionId = rec.intersectionId;
                            ev.corridorId = rec.holdingCorridorId;
                            ev.ambulanceId = rec.holdingAmbulanceId;
                            ev.fromState = DynamicReleaseState::ALL_RED_CLEARANCE;
                            ev.toState = DynamicReleaseState::RELEASED_NOMINAL;
                            ev.timestampUs = nowUs;
                            ev.transitionDurationMs = allRedElapsedSec * 1000.0;
                            ev.actionSummary = "RESTORED_NOMINAL_CYCLIC_OPERATION";
                            m_events.push_back(ev);

                            pendingCommands.push_back({rec.intersectionId, "RESTORE_CYCLIC"});
                            transitionsCount++;

                            LogInfo("Intersection " + rec.intersectionId +
                                    " FULLY RELEASED! Nominal cyclic traffic resumed.");
                        }
                    }
                }
            }
            // 3. Orphan hold safety purge
            else if (rec.state == DynamicReleaseState::LOCKED_EMERGENCY) {
                if (nowUs >= rec.lockedTimestampUs) {
                    double lockedSec = static_cast<double>(nowUs - rec.lockedTimestampUs) / 1000000.0;
                    if (lockedSec > m_config.maxEmergencyHoldSec) {
                        rec.state = DynamicReleaseState::CLEARING_YELLOW;
                        rec.yellowStartTimestampUs = nowUs;
                        LogWarn("Intersection " + rec.intersectionId +
                                " exceeded max emergency hold (" + std::to_string(lockedSec) +
                                "s); forcing dynamic yellow clearance purge!");
                        pendingCommands.push_back({rec.intersectionId, "CLEARANCE_YELLOW"});
                        transitionsCount++;
                    }
                }
            }
        }
    }

    if (m_signalCallback && !pendingCommands.empty()) {
        for (const auto& item : pendingCommands) {
            m_signalCallback(item.first, item.second);
        }
    }

    return transitionsCount;
}

bool DynamicCorridorReleaseManager::ForceReleaseImmediate(const std::string& intersectionId, uint64_t timestampUs) {
    SignalActionCallback localCallback = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_intersections.find(intersectionId);
        if (it == m_intersections.end()) {
            return false;
        }

        uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
        localCallback = m_signalCallback;

        it->second.state = DynamicReleaseState::RELEASED_NOMINAL;
        it->second.releasedTimestampUs = nowUs;
        it->second.queuedCorridorClaims.clear();

        DynamicReleaseEvent ev;
        ev.intersectionId = intersectionId;
        ev.corridorId = it->second.holdingCorridorId;
        ev.ambulanceId = it->second.holdingAmbulanceId;
        ev.fromState = DynamicReleaseState::LOCKED_EMERGENCY;
        ev.toState = DynamicReleaseState::RELEASED_NOMINAL;
        ev.timestampUs = nowUs;
        ev.transitionDurationMs = 0.0;
        ev.actionSummary = "FORCE_RELEASE_IMMEDIATE";
        m_events.push_back(ev);
    }

    if (localCallback) {
        localCallback(intersectionId, "RESTORE_CYCLIC");
    }
    return true;
}

void DynamicCorridorReleaseManager::SetSignalActionCallback(SignalActionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_signalCallback = std::move(callback);
}

bool DynamicCorridorReleaseManager::IsIntersectionLocked(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        return it->second.state == DynamicReleaseState::LOCKED_EMERGENCY ||
               it->second.state == DynamicReleaseState::CLEARING_YELLOW ||
               it->second.state == DynamicReleaseState::ALL_RED_CLEARANCE;
    }
    return false;
}

DynamicReleaseState DynamicCorridorReleaseManager::GetIntersectionState(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        return it->second.state;
    }
    return DynamicReleaseState::IDLE_NOMINAL;
}

size_t DynamicCorridorReleaseManager::GetActiveLockedCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_intersections) {
        if (pair.second.state != DynamicReleaseState::RELEASED_NOMINAL &&
            pair.second.state != DynamicReleaseState::IDLE_NOMINAL) {
            count++;
        }
    }
    return count;
}

bool DynamicCorridorReleaseManager::GetIntersectionRecord(const std::string& intersectionId,
                                                         IntersectionReleaseRecord& outRecord) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        outRecord = it->second;
        return true;
    }
    return false;
}

std::vector<DynamicReleaseEvent> DynamicCorridorReleaseManager::GetReleaseEvents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::string DynamicCorridorReleaseManager::GenerateReleaseReport(uint64_t currentTimestampUs) const {
    (void)currentTimestampUs;
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - DYNAMIC CORRIDOR RELEASE REPORT\n"
        << "=========================================================================\n"
        << " Managed Intersections : " << m_intersections.size() << "\n"
        << " Historical Transitions: " << m_events.size() << "\n"
        << "-------------------------------------------------------------------------\n"
        << " Intersection | Corridor    | Ambulance  | Current State      | Queued\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& pair : m_intersections) {
        const auto& r = pair.second;
        oss << " " << std::left << std::setw(13) << r.intersectionId.substr(0, 13) << " | "
            << std::setw(11) << r.holdingCorridorId.substr(0, 11) << " | "
            << std::setw(10) << r.holdingAmbulanceId.substr(0, 10) << " | "
            << std::setw(18) << DynamicReleaseStateToString(r.state) << " | "
            << r.queuedCorridorClaims.size() << "\n";
    }
    oss << "=========================================================================\n";
    return oss.str();
}

void DynamicCorridorReleaseManager::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections.clear();
    m_events.clear();
}

} // namespace corridor
} // namespace aegis
