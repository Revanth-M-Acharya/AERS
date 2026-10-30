/**
 * @file CorridorDeadlineMonitor.cpp
 * @brief Implementation of Phase 3 Corridor Deadline Monitor for SMART AEGIS.
 */

#include "aegis/corridor/CorridorDeadlineMonitor.hpp"
#include "aegis/logger/logger.hpp"

#include <cmath>
#include <iostream>

namespace aegis {
namespace corridor {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "DEADLINE_MON", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "DEADLINE_MON", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "DEADLINE_MON", msg);
}

CorridorDeadlineMonitor::CorridorDeadlineMonitor(const CorridorDeadlineConfig& config)
    : m_config(config) {}

uint64_t CorridorDeadlineMonitor::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

bool CorridorDeadlineMonitor::RegisterCorridor(const std::string& corridorId,
                                              const std::string& ambulanceId,
                                              const std::vector<std::string>& route,
                                              double etaSec,
                                              double customDeadlineSec,
                                              uint64_t creationTimeUs) {
    if (corridorId.empty() || ambulanceId.empty() || route.empty() || etaSec <= 0.0) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_corridors.find(corridorId) != m_corridors.end()) {
        return false; // Duplicate corridor registration rejected
    }

    uint64_t nowUs = (creationTimeUs > 0) ? creationTimeUs : GetCurrentTimeUs();
    double deadlineSec = (customDeadlineSec > 0.0)
                             ? customDeadlineSec
                             : (etaSec * m_config.slackFactor);
    if (deadlineSec < etaSec) {
        deadlineSec = etaSec;
    }

    CorridorTrackingRecord record;
    record.corridorId = corridorId;
    record.ambulanceId = ambulanceId;
    record.startIntersectionId = route.front();
    record.destinationId = route.back();
    record.routeIntersections = route;
    record.creationTimestampUs = nowUs;
    record.estimatedArrivalDurationSec = etaSec;
    record.deadlineDurationSec = deadlineSec;
    record.elapsedTimeSec = 0.0;
    record.remainingTimeSec = deadlineSec;
    record.currentIntersection = route.front();
    record.status = CorridorDeadlineStatus::ACTIVE;
    record.isRecovered = false;
    record.recoveryAction = "";
    record.completionTimestampUs = 0;

    m_corridors[corridorId] = std::move(record);

    LogInfo("Registered corridor " + corridorId +
            " for ambulance " + ambulanceId + " (ETA: " +
            std::to_string(etaSec) + "s, Deadline: " +
            std::to_string(deadlineSec) + "s)");
    return true;
}

bool CorridorDeadlineMonitor::UpdateProgress(const std::string& corridorId,
                                            const std::string& currentIntersectionId,
                                            uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.currentIntersection = currentIntersectionId;

    if (it->second.status == CorridorDeadlineStatus::ACTIVE ||
        it->second.status == CorridorDeadlineStatus::APPROACHING_DEADLINE) {
        if (nowUs >= it->second.creationTimestampUs) {
            it->second.elapsedTimeSec = static_cast<double>(nowUs - it->second.creationTimestampUs) / 1000000.0;
            it->second.remainingTimeSec = it->second.deadlineDurationSec - it->second.elapsedTimeSec;
        }
    }
    return true;
}

bool CorridorDeadlineMonitor::CompleteCorridor(const std::string& corridorId, uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.status = CorridorDeadlineStatus::COMPLETED;
    it->second.completionTimestampUs = nowUs;
    if (nowUs >= it->second.creationTimestampUs) {
        it->second.elapsedTimeSec = static_cast<double>(nowUs - it->second.creationTimestampUs) / 1000000.0;
        it->second.remainingTimeSec = it->second.deadlineDurationSec - it->second.elapsedTimeSec;
    }

    LogInfo("Corridor " + corridorId + " COMPLETED safely in " +
            std::to_string(it->second.elapsedTimeSec) + "s (Deadline: " +
            std::to_string(it->second.deadlineDurationSec) + "s)");
    return true;
}

bool CorridorDeadlineMonitor::AbortCorridor(const std::string& corridorId,
                                           const std::string& reason,
                                           uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.status = CorridorDeadlineStatus::ABORTED;
    it->second.recoveryAction = reason;
    it->second.completionTimestampUs = nowUs;
    return true;
}

size_t CorridorDeadlineMonitor::CheckDeadlines(uint64_t currentTimestampUs) {
    std::vector<CorridorTimeoutEvent> newlyTimedOut;
    TimeoutRecoveryHandler localHandler = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();
        localHandler = m_recoveryHandler;

        for (auto& pair : m_corridors) {
            auto& record = pair.second;
            if (record.status == CorridorDeadlineStatus::COMPLETED ||
                record.status == CorridorDeadlineStatus::ABORTED ||
                record.status == CorridorDeadlineStatus::RECOVERED) {
                continue;
            }

            if (nowUs >= record.creationTimestampUs) {
                record.elapsedTimeSec = static_cast<double>(nowUs - record.creationTimestampUs) / 1000000.0;
                record.remainingTimeSec = record.deadlineDurationSec - record.elapsedTimeSec;
            }

            // Check if deadline exceeded
            if (record.elapsedTimeSec > record.deadlineDurationSec) {
                if (record.status != CorridorDeadlineStatus::TIMED_OUT) {
                    record.status = CorridorDeadlineStatus::TIMED_OUT;
                    record.isRecovered = m_config.autoTriggerRecovery;
                    record.recoveryAction = m_config.autoTriggerRecovery ? "TRIGGER_CORRIDOR_RECOVERY" : "NONE";

                    CorridorTimeoutEvent ev;
                    ev.corridorId = record.corridorId;
                    ev.ambulanceId = record.ambulanceId;
                    ev.timestampUs = nowUs;
                    ev.elapsedSec = record.elapsedTimeSec;
                    ev.deadlineSec = record.deadlineDurationSec;
                    ev.overrunSec = record.elapsedTimeSec - record.deadlineDurationSec;
                    ev.lastKnownIntersection = record.currentIntersection;
                    ev.actionTaken = record.recoveryAction;

                    m_timeoutEvents.push_back(ev);
                    newlyTimedOut.push_back(ev);

                    LogError("Corridor " + record.corridorId +
                             " TIMED OUT! Elapsed: " + std::to_string(record.elapsedTimeSec) +
                             "s > Deadline: " + std::to_string(record.deadlineDurationSec) +
                             "s. Overrun: " + std::to_string(ev.overrunSec) + "s");
                }
            } else if (record.remainingTimeSec <= m_config.warningThresholdSec ||
                       (record.remainingTimeSec / record.deadlineDurationSec) <= m_config.warningThresholdRatio) {
                if (record.status == CorridorDeadlineStatus::ACTIVE) {
                    record.status = CorridorDeadlineStatus::APPROACHING_DEADLINE;
                    LogWarn("Corridor " + record.corridorId +
                            " APPROACHING DEADLINE (Remaining: " +
                            std::to_string(record.remainingTimeSec) + "s)");
                }
            }
        }
    }

    // Invoke recovery handler outside lock to guarantee deadlock freedom
    if (localHandler && !newlyTimedOut.empty()) {
        for (const auto& ev : newlyTimedOut) {
            localHandler(ev);
        }
    }

    return newlyTimedOut.size();
}

void CorridorDeadlineMonitor::SetTimeoutRecoveryHandler(TimeoutRecoveryHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recoveryHandler = std::move(handler);
}

bool CorridorDeadlineMonitor::IsCorridorTimedOut(const std::string& corridorId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it != m_corridors.end()) {
        return it->second.status == CorridorDeadlineStatus::TIMED_OUT;
    }
    return false;
}

double CorridorDeadlineMonitor::GetRemainingDeadlineSec(const std::string& corridorId, uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end()) {
        return 0.0;
    }

    if (it->second.status == CorridorDeadlineStatus::COMPLETED ||
        it->second.status == CorridorDeadlineStatus::ABORTED) {
        return it->second.remainingTimeSec;
    }

    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();
    if (nowUs >= it->second.creationTimestampUs) {
        double elapsed = static_cast<double>(nowUs - it->second.creationTimestampUs) / 1000000.0;
        return it->second.deadlineDurationSec - elapsed;
    }
    return it->second.deadlineDurationSec;
}

bool CorridorDeadlineMonitor::GetCorridorInfo(const std::string& corridorId, CorridorTrackingRecord& outRecord) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it != m_corridors.end()) {
        outRecord = it->second;
        return true;
    }
    return false;
}

std::vector<CorridorTrackingRecord> CorridorDeadlineMonitor::GetAllCorridors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<CorridorTrackingRecord> records;
    records.reserve(m_corridors.size());
    for (const auto& pair : m_corridors) {
        records.push_back(pair.second);
    }
    return records;
}

std::vector<CorridorTimeoutEvent> CorridorDeadlineMonitor::GetTimeoutEvents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_timeoutEvents;
}

size_t CorridorDeadlineMonitor::GetActiveCorridorCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_corridors) {
        if (pair.second.status == CorridorDeadlineStatus::ACTIVE ||
            pair.second.status == CorridorDeadlineStatus::APPROACHING_DEADLINE) {
            ++count;
        }
    }
    return count;
}

std::string CorridorDeadlineMonitor::GenerateStatusReport(uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();

    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - CORRIDOR DEADLINE MONITOR REPORT\n"
        << "=========================================================================\n"
        << " Monitored Corridors : " << m_corridors.size() << "\n"
        << " Total Timeout Events: " << m_timeoutEvents.size() << "\n"
        << "-------------------------------------------------------------------------\n"
        << " Corridor ID | Ambulance  | Status               | Elapsed | Deadline | Remain\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& pair : m_corridors) {
        const auto& c = pair.second;
        double elapsed = c.elapsedTimeSec;
        double remain = c.remainingTimeSec;
        if ((c.status == CorridorDeadlineStatus::ACTIVE || c.status == CorridorDeadlineStatus::APPROACHING_DEADLINE) &&
            nowUs >= c.creationTimestampUs) {
            elapsed = static_cast<double>(nowUs - c.creationTimestampUs) / 1000000.0;
            remain = c.deadlineDurationSec - elapsed;
        }

        oss << " " << std::left << std::setw(11) << c.corridorId.substr(0, 11) << " | "
            << std::setw(10) << c.ambulanceId.substr(0, 10) << " | "
            << std::setw(20) << CorridorDeadlineStatusToString(c.status) << " | "
            << std::right << std::fixed << std::setprecision(1) << std::setw(6) << elapsed << "s | "
            << std::setw(7) << c.deadlineDurationSec << "s | "
            << std::setw(6) << remain << "s\n";
    }
    oss << "=========================================================================\n";
    return oss.str();
}

void CorridorDeadlineMonitor::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_corridors.clear();
    m_timeoutEvents.clear();
}

} // namespace corridor
} // namespace aegis
