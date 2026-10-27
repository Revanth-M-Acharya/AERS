/**
 * @file PriorityScheduler.cpp
 * @brief Implementation of PriorityScheduler multi-vehicle contention resolution and corridor grants.
 */

#include "scheduler/PriorityScheduler.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

PriorityScheduler::PriorityScheduler()
    : m_detector(ConflictDetector::DEFAULT_CONFLICT_WINDOW_SECONDS)
    , m_clearanceDurationSeconds(DEFAULT_CLEARANCE_SECONDS) {}

PriorityScheduler::PriorityScheduler(uint32_t conflictWindowSeconds,
                                     uint32_t clearanceDurationSeconds)
    : m_detector(conflictWindowSeconds)
    , m_clearanceDurationSeconds(clearanceDurationSeconds) {}

void PriorityScheduler::SetConflictWindowSeconds(uint32_t seconds) noexcept {
    m_detector.SetConflictWindowSeconds(seconds);
}

uint32_t PriorityScheduler::GetConflictWindowSeconds() const noexcept {
    return m_detector.GetConflictWindowSeconds();
}

void PriorityScheduler::SetClearanceDurationSeconds(uint32_t seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clearanceDurationSeconds = seconds;
}

uint32_t PriorityScheduler::GetClearanceDurationSeconds() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_clearanceDurationSeconds;
}

int PriorityScheduler::PriorityToRank(medical::PriorityLevel priority) {
    switch (priority) {
        case medical::PriorityLevel::CRITICAL: return 4;
        case medical::PriorityLevel::HIGH:     return 3;
        case medical::PriorityLevel::MEDIUM:   return 2;
        case medical::PriorityLevel::LOW:      return 1;
        default:                               return 0;
    }
}

bool PriorityScheduler::CompareAmbulanceUrgency(const AmbulanceArrival& a, const AmbulanceArrival& b) {
    // 1. Manual Dispatcher Override Level (higher wins)
    if (a.profile.GetManualOverrideLevel() != b.profile.GetManualOverrideLevel()) {
        return a.profile.GetManualOverrideLevel() > b.profile.GetManualOverrideLevel();
    }

    // 2. Emergency Category / Priority Level (CRITICAL > HIGH > MEDIUM > LOW)
    int rankA = PriorityToRank(a.profile.GetPriority());
    int rankB = PriorityToRank(b.profile.GetPriority());
    if (rankA != rankB) {
        return rankA > rankB;
    }

    // 3. Clinical Emergency Score (higher wins)
    double scoreA = 0.0;
    if (a.profile.HasPatient()) {
        scoreA = medical::PriorityEngine::CalculateEmergencyScore(*a.profile.GetPatient());
    }
    double scoreB = 0.0;
    if (b.profile.HasPatient()) {
        scoreB = medical::PriorityEngine::CalculateEmergencyScore(*b.profile.GetPatient());
    }
    if (std::abs(scoreA - scoreB) > 0.05) {
        return scoreA > scoreB;
    }

    // 4. Estimated Time of Arrival (sooner arrival wins)
    if (a.etaSeconds != b.etaSeconds) {
        return a.etaSeconds < b.etaSeconds;
    }

    // 5. Distance Remaining (closer to junction wins)
    if (std::abs(a.distanceRemainingMeters - b.distanceRemainingMeters) > 0.05) {
        return a.distanceRemainingMeters < b.distanceRemainingMeters;
    }

    // 6. Deterministic tie-breaker: Lexicographical ID comparison
    return a.ambulanceId < b.ambulanceId;
}

ConflictResolution PriorityScheduler::ResolveConflict(const IntersectionConflict& conflict) {
    std::lock_guard<std::mutex> lock(m_mutex);

    ConflictResolution resolution;
    resolution.intersectionId = conflict.intersectionId;
    resolution.delayDurationSeconds = m_clearanceDurationSeconds;

    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    resolution.timestampEpochMs = nowMs;

    if (conflict.conflictingArrivals.empty()) {
        return resolution;
    }

    std::vector<AmbulanceArrival> sortedArrivals = conflict.conflictingArrivals;
    std::sort(sortedArrivals.begin(), sortedArrivals.end(), CompareAmbulanceUrgency);

    // Winner is the highest-ranked unit
    const auto& winner = sortedArrivals.front();
    resolution.winnerAmbulanceId = winner.ambulanceId;

    // Delayed units are all subsequent arrivals
    for (size_t i = 1; i < sortedArrivals.size(); ++i) {
        const auto& delayed = sortedArrivals[i];
        resolution.delayedAmbulanceIds.push_back(delayed.ambulanceId);

        uint32_t cumulativeDelay = static_cast<uint32_t>(i * m_clearanceDurationSeconds);
        resolution.assignedDelaysSeconds[delayed.ambulanceId] = cumulativeDelay;
    }

    // Generate clinical & operational explanation
    std::ostringstream oss;
    oss << "Arbitrated " << winner.ambulanceId << " granted GREEN over "
        << resolution.delayedAmbulanceIds.size() << " conflicting unit(s) at "
        << conflict.intersectionId << " [Priority: " << winner.profile.GetPriorityString();

    if (winner.profile.HasPatient()) {
        oss << ", Patient: " << winner.profile.GetPatient()->GetEmergencyTypeString();
    }
    if (winner.profile.GetManualOverrideLevel() > 0) {
        oss << ", ManualOverride: " << static_cast<int>(winner.profile.GetManualOverrideLevel());
    }
    oss << ", ETA: " << winner.etaSeconds << "s, Clearance: "
        << m_clearanceDurationSeconds << "s]";
    resolution.resolutionReason = oss.str();

    // Enforce strict mutual exclusion: ONLY the winner receives GREEN for this intersection
    m_activeGreenGrants[conflict.intersectionId] = winner.ambulanceId;
    m_lastResolutions[conflict.intersectionId] = resolution;

    return resolution;
}

std::vector<ConflictResolution> PriorityScheduler::ScheduleAmbulances(
    const std::vector<ambulance::AmbulanceProfile>& ambulances) {

    // 1. Detect spatial-temporal conflicts
    auto conflicts = m_detector.DetectConflicts(ambulances, m_detector.GetConflictWindowSeconds());

    std::vector<ConflictResolution> resolutions;
    resolutions.reserve(conflicts.size());

    std::unordered_map<std::string, bool> conflictedIntersections;

    // 2. Resolve every detected conflict
    for (const auto& conflict : conflicts) {
        conflictedIntersections[conflict.intersectionId] = true;
        resolutions.push_back(ResolveConflict(conflict));
    }

    // 3. For any non-conflicted intersection, award exclusive green to the imminent arrival
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unordered_map<std::string, const ambulance::AmbulanceProfile*> closestAmbulanceForInt;
    for (const auto& amb : ambulances) {
        const std::string& intId = amb.GetCurrentIntersection();
        if (intId.empty() || conflictedIntersections.find(intId) != conflictedIntersections.end()) {
            continue;
        }

        auto it = closestAmbulanceForInt.find(intId);
        if (it == closestAmbulanceForInt.end()) {
            closestAmbulanceForInt[intId] = &amb;
        } else if (amb.GetETASeconds() < it->second->GetETASeconds()) {
            closestAmbulanceForInt[intId] = &amb;
        }
    }

    for (const auto& [intId, ambPtr] : closestAmbulanceForInt) {
        m_activeGreenGrants[intId] = ambPtr->GetAmbulanceId();
    }

    return resolutions;
}

bool PriorityScheduler::IsAmbulanceGrantedGreen(const std::string& ambulanceId,
                                               const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeGreenGrants.find(intersectionId);
    if (it != m_activeGreenGrants.end() && it->second == ambulanceId) {
        return true;
    }
    return false;
}

std::string PriorityScheduler::GetGrantedAmbulanceForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeGreenGrants.find(intersectionId);
    if (it != m_activeGreenGrants.end()) {
        return it->second;
    }
    return "";
}

void PriorityScheduler::RevokeGreenGrant(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants.erase(intersectionId);
}

void PriorityScheduler::ForceGreenGrant(const std::string& intersectionId, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants[intersectionId] = ambulanceId;
}

std::vector<ConflictResolution> PriorityScheduler::GetActiveResolutions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ConflictResolution> results;
    results.reserve(m_lastResolutions.size());
    for (const auto& [intId, res] : m_lastResolutions) {
        results.push_back(res);
    }
    return results;
}

std::optional<ConflictResolution> PriorityScheduler::GetResolutionForIntersection(
    const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_lastResolutions.find(intersectionId);
    if (it != m_lastResolutions.end()) {
        return it->second;
    }
    return std::nullopt;
}

void PriorityScheduler::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_detector.ClearArrivals();
    m_activeGreenGrants.clear();
    m_lastResolutions.clear();
}

} // namespace scheduler
} // namespace aegis
