/**
 * @file ConflictDetector.cpp
 * @brief Implementation of ConflictDetector spatial-temporal junction monitoring.
 */

#include "scheduler/ConflictDetector.h"
#include <algorithm>

namespace aegis {
namespace scheduler {

ConflictDetector::ConflictDetector()
    : m_conflictWindowSeconds(DEFAULT_CONFLICT_WINDOW_SECONDS) {}

ConflictDetector::ConflictDetector(uint32_t conflictWindowSeconds)
    : m_conflictWindowSeconds(conflictWindowSeconds) {}

uint32_t ConflictDetector::GetConflictWindowSeconds() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_conflictWindowSeconds;
}

void ConflictDetector::SetConflictWindowSeconds(uint32_t seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_conflictWindowSeconds = seconds;
}

void ConflictDetector::RegisterArrival(const AmbulanceArrival& arrival) {
    if (arrival.ambulanceId.empty() || arrival.intersectionId.empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& list = m_intersectionArrivals[arrival.intersectionId];

    // Replace if this ambulance already has an arrival registered for this intersection
    auto it = std::find_if(list.begin(), list.end(), [&](const AmbulanceArrival& existing) {
        return existing.ambulanceId == arrival.ambulanceId;
    });

    if (it != list.end()) {
        *it = arrival;
    } else {
        list.push_back(arrival);
    }
}

void ConflictDetector::RegisterArrival(const std::string& ambulanceId,
                                      const std::string& intersectionId,
                                      uint32_t etaSeconds,
                                      double distanceMeters,
                                      const ambulance::AmbulanceProfile& profile) {
    RegisterArrival(AmbulanceArrival(ambulanceId, intersectionId, etaSeconds, distanceMeters, profile));
}

void ConflictDetector::ClearArrivals() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersectionArrivals.clear();
}

std::vector<IntersectionConflict> ConflictDetector::DetectConflicts() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionConflict> conflicts;
    const uint32_t windowSec = m_conflictWindowSeconds;

    for (const auto& [intId, arrivalsOriginal] : m_intersectionArrivals) {
        if (arrivalsOriginal.size() < 2) {
            continue;
        }

        std::vector<AmbulanceArrival> sortedArrivals = arrivalsOriginal;
        std::sort(sortedArrivals.begin(), sortedArrivals.end(),
                  [](const AmbulanceArrival& a, const AmbulanceArrival& b) {
                      if (a.etaSeconds != b.etaSeconds) {
                          return a.etaSeconds < b.etaSeconds;
                      }
                      return a.ambulanceId < b.ambulanceId;
                  });

        std::vector<AmbulanceArrival> currentCluster;
        currentCluster.push_back(sortedArrivals[0]);

        for (size_t i = 1; i < sortedArrivals.size(); ++i) {
            if (sortedArrivals[i].etaSeconds - currentCluster.back().etaSeconds <= windowSec) {
                currentCluster.push_back(sortedArrivals[i]);
            } else {
                if (currentCluster.size() >= 2) {
                    IntersectionConflict conflict;
                    conflict.intersectionId = intId;
                    conflict.conflictingArrivals = currentCluster;
                    conflict.earliestEtaSeconds = currentCluster.front().etaSeconds;
                    conflict.latestEtaSeconds = currentCluster.back().etaSeconds;
                    conflict.timeDeltaSeconds = conflict.latestEtaSeconds - conflict.earliestEtaSeconds;
                    conflicts.push_back(conflict);
                }
                currentCluster.clear();
                currentCluster.push_back(sortedArrivals[i]);
            }
        }

        if (currentCluster.size() >= 2) {
            IntersectionConflict conflict;
            conflict.intersectionId = intId;
            conflict.conflictingArrivals = currentCluster;
            conflict.earliestEtaSeconds = currentCluster.front().etaSeconds;
            conflict.latestEtaSeconds = currentCluster.back().etaSeconds;
            conflict.timeDeltaSeconds = conflict.latestEtaSeconds - conflict.earliestEtaSeconds;
            conflicts.push_back(conflict);
        }
    }

    return conflicts;
}

std::vector<IntersectionConflict> ConflictDetector::DetectConflicts(
    const std::vector<ambulance::AmbulanceProfile>& ambulances,
    uint32_t windowSeconds) {
    
    ClearArrivals();
    SetConflictWindowSeconds(windowSeconds);

    for (const auto& amb : ambulances) {
        if (amb.GetCurrentIntersection().empty()) {
            continue;
        }
        RegisterArrival(amb.GetAmbulanceId(),
                        amb.GetCurrentIntersection(),
                        amb.GetETASeconds(),
                        amb.GetDistanceRemainingMeters(),
                        amb);
    }

    return DetectConflicts();
}

std::vector<AmbulanceArrival> ConflictDetector::GetArrivalsForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersectionArrivals.find(intersectionId);
    if (it != m_intersectionArrivals.end()) {
        return it->second;
    }
    return {};
}

size_t ConflictDetector::GetTotalArrivalCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& [id, list] : m_intersectionArrivals) {
        count += list.size();
    }
    return count;
}

} // namespace scheduler
} // namespace aegis
