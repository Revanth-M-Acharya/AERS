/**
 * @file ConflictDetectionEngine.cpp
 * @brief Implementation of ConflictDetectionEngine arrival tracking, window contention, and event dispatch.
 */

#include "scheduler/ConflictDetectionEngine.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

bool ConflictEvent::HasAmbulance(const std::string& ambId) const {
    for (const auto& id : conflictingAmbulanceIds) {
        if (id == ambId) return true;
    }
    return false;
}

std::string ConflictEvent::ToString() const {
    std::ostringstream oss;
    oss << "ConflictEvent [" << eventId << "] at Intersection: " << intersectionId << "\n"
        << "  Conflicting Units (" << conflictingAmbulanceIds.size() << "): ";
    for (size_t i = 0; i < conflictingAmbulanceIds.size(); ++i) {
        oss << conflictingAmbulanceIds[i];
        if (i + 1 < conflictingAmbulanceIds.size()) oss << ", ";
    }
    oss << "\n  Time Window       : " << timeWindowSeconds << "s (Delta: " << timeDeltaSeconds << "s)\n"
        << "  ETA Span          : [" << earliestEtaSeconds << "s - " << latestEtaSeconds << "s]\n"
        << "  Severity          : " << logger::EventSeverityToString(severity) << "\n"
        << "  Description       : " << description;
    return oss.str();
}

std::string ConflictEvent::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"event_id\":\"" << eventId << "\","
        << "\"intersection_id\":\"" << intersectionId << "\","
        << "\"time_window_seconds\":" << timeWindowSeconds << ","
        << "\"earliest_eta_seconds\":" << earliestEtaSeconds << ","
        << "\"latest_eta_seconds\":" << latestEtaSeconds << ","
        << "\"time_delta_seconds\":" << timeDeltaSeconds << ","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << ","
        << "\"severity\":\"" << logger::EventSeverityToString(severity) << "\","
        << "\"description\":\"" << description << "\","
        << "\"conflicting_ambulances\":[";

    for (size_t i = 0; i < conflictingAmbulanceIds.size(); ++i) {
        oss << "\"" << conflictingAmbulanceIds[i] << "\"";
        if (i + 1 < conflictingAmbulanceIds.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

uint64_t ConflictDetectionEngine::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

ConflictDetectionEngine::ConflictDetectionEngine()
    : m_detector(DEFAULT_CONFLICT_WINDOW_SECONDS)
    , m_conflictWindowSeconds(DEFAULT_CONFLICT_WINDOW_SECONDS) {}

ConflictDetectionEngine::ConflictDetectionEngine(uint32_t conflictWindowSeconds)
    : m_detector(conflictWindowSeconds)
    , m_conflictWindowSeconds(conflictWindowSeconds) {}

uint32_t ConflictDetectionEngine::GetConflictWindowSeconds() const noexcept {
    return m_conflictWindowSeconds;
}

void ConflictDetectionEngine::SetConflictWindowSeconds(uint32_t seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_conflictWindowSeconds = seconds;
    m_detector.SetConflictWindowSeconds(seconds);
}

void ConflictDetectionEngine::RegisterArrival(const AmbulanceArrival& arrival) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_detector.RegisterArrival(arrival);
}

void ConflictDetectionEngine::RegisterArrival(const std::string& ambulanceId,
                                             const std::string& intersectionId,
                                             uint32_t etaSeconds,
                                             double distanceMeters,
                                             const ambulance::AmbulanceProfile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_detector.RegisterArrival(ambulanceId, intersectionId, etaSeconds, distanceMeters, profile);
}

void ConflictDetectionEngine::ClearArrivals() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_detector.ClearArrivals();
}

ConflictEvent ConflictDetectionEngine::BuildConflictEvent(const IntersectionConflict& conflict) {
    ++m_eventSequence;
    ConflictEvent evt;
    std::ostringstream idOss;
    idOss << "CONF-" << conflict.intersectionId << "-" << m_eventSequence;
    evt.eventId = idOss.str();
    evt.intersectionId = conflict.intersectionId;
    evt.conflictingArrivals = conflict.conflictingArrivals;
    evt.timeWindowSeconds = m_conflictWindowSeconds;
    evt.earliestEtaSeconds = conflict.earliestEtaSeconds;
    evt.latestEtaSeconds = conflict.latestEtaSeconds;
    evt.timeDeltaSeconds = conflict.timeDeltaSeconds;
    evt.timestampEpochMs = CurrentTimeMillis();

    for (const auto& arr : conflict.conflictingArrivals) {
        evt.conflictingAmbulanceIds.push_back(arr.ambulanceId);
    }

    if (conflict.GetConflictCount() >= 3) {
        evt.severity = logger::EventSeverity::CRITICAL;
    } else {
        evt.severity = logger::EventSeverity::WARNING;
    }

    std::ostringstream descOss;
    descOss << "Simultaneous arrival conflict detected at " << conflict.intersectionId
            << " between " << conflict.GetConflictCount() << " emergency ambulances (ETA window: "
            << conflict.earliestEtaSeconds << "s - " << conflict.latestEtaSeconds << "s)";
    evt.description = descOss.str();

    return evt;
}

std::vector<ConflictEvent> ConflictDetectionEngine::DetectConflicts() {
    std::vector<ConflictEvent> generatedEvents;
    std::vector<ConflictListener> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::vector<IntersectionConflict> rawConflicts = m_detector.DetectConflicts();
        m_activeConflicts.clear();

        for (const auto& raw : rawConflicts) {
            ConflictEvent evt = BuildConflictEvent(raw);
            m_activeConflicts.push_back(evt);
            m_conflictHistory.push_back(evt);
            generatedEvents.push_back(evt);
        }
        listenersCopy = m_listeners;
    }

    // Dispatch listeners and logging outside lock
    for (const auto& evt : generatedEvents) {
        if (m_enableLogging) {
            std::string primaryAmb = evt.conflictingAmbulanceIds.empty() ? "" : evt.conflictingAmbulanceIds.front();
            logger::EventLogger::Instance().LogConflictDetected(
                evt.intersectionId,
                evt.description,
                primaryAmb,
                evt.severity
            );
        }
        for (const auto& listener : listenersCopy) {
            if (listener) {
                listener(evt);
            }
        }
    }

    return generatedEvents;
}

std::vector<ConflictEvent> ConflictDetectionEngine::DetectConflicts(
    const std::vector<ambulance::AmbulanceProfile>& ambulances,
    uint32_t windowSeconds) {
    
    std::vector<ConflictEvent> generatedEvents;
    std::vector<ConflictListener> listenersCopy;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_conflictWindowSeconds = windowSeconds;
        std::vector<IntersectionConflict> rawConflicts = m_detector.DetectConflicts(ambulances, windowSeconds);
        m_activeConflicts.clear();

        for (const auto& raw : rawConflicts) {
            ConflictEvent evt = BuildConflictEvent(raw);
            m_activeConflicts.push_back(evt);
            m_conflictHistory.push_back(evt);
            generatedEvents.push_back(evt);
        }
        listenersCopy = m_listeners;
    }

    for (const auto& evt : generatedEvents) {
        if (m_enableLogging) {
            std::string primaryAmb = evt.conflictingAmbulanceIds.empty() ? "" : evt.conflictingAmbulanceIds.front();
            logger::EventLogger::Instance().LogConflictDetected(
                evt.intersectionId,
                evt.description,
                primaryAmb,
                evt.severity
            );
        }
        for (const auto& listener : listenersCopy) {
            if (listener) {
                listener(evt);
            }
        }
    }

    return generatedEvents;
}

std::vector<ConflictEvent> ConflictDetectionEngine::GetActiveConflicts() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeConflicts;
}

std::optional<ConflictEvent> ConflictDetectionEngine::GetConflictForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& evt : m_activeConflicts) {
        if (evt.intersectionId == intersectionId) {
            return evt;
        }
    }
    return std::nullopt;
}

bool ConflictDetectionEngine::HasConflictAt(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& evt : m_activeConflicts) {
        if (evt.intersectionId == intersectionId) {
            return true;
        }
    }
    return false;
}

std::vector<ConflictEvent> ConflictDetectionEngine::GetConflictHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_conflictHistory;
}

size_t ConflictDetectionEngine::GetTotalArrivalCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_detector.GetTotalArrivalCount();
}

void ConflictDetectionEngine::AddConflictListener(ConflictListener listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.push_back(std::move(listener));
}

void ConflictDetectionEngine::ClearListeners() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
}

void ConflictDetectionEngine::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_detector.ClearArrivals();
    m_activeConflicts.clear();
    m_conflictHistory.clear();
}

} // namespace scheduler
} // namespace aegis
