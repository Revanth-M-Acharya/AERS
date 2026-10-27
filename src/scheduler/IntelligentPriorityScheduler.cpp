/**
 * @file IntelligentPriorityScheduler.cpp
 * @brief Implementation of deterministic multi-factor IntelligentPriorityScheduler.
 */

#include "scheduler/IntelligentPriorityScheduler.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

bool IntelligentScheduleDecision::HasDelayedAmbulance(const std::string& ambId) const {
    for (const auto& id : delayedAmbulanceIds) {
        if (id == ambId) return true;
    }
    return false;
}

uint32_t IntelligentScheduleDecision::GetAssignedDelay(const std::string& ambId) const {
    auto it = staggeredDelaysSeconds.find(ambId);
    if (it != staggeredDelaysSeconds.end()) {
        return it->second;
    }
    return 0;
}

std::string IntelligentScheduleDecision::ToString() const {
    std::ostringstream oss;
    oss << "IntelligentScheduleDecision for [" << intersectionId << "]\n"
        << "  Winner Ambulance : " << winnerAmbulanceId << " (Score: " << winnerScore << ")\n"
        << "  Delayed Units    : " << delayedAmbulanceIds.size() << "\n";
    for (const auto& delId : delayedAmbulanceIds) {
        oss << "    -> " << delId << " : +" << GetAssignedDelay(delId) << "s delay\n";
    }
    oss << "  Decision Reason  : " << decisionReason;
    return oss.str();
}

std::string IntelligentScheduleDecision::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"intersection_id\":\"" << intersectionId << "\","
        << "\"winner_ambulance_id\":\"" << winnerAmbulanceId << "\","
        << "\"winner_score\":" << std::fixed << std::setprecision(2) << winnerScore << ","
        << "\"decision_reason\":\"" << decisionReason << "\","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << ","
        << "\"delayed_ambulances\":[";

    for (size_t i = 0; i < delayedAmbulanceIds.size(); ++i) {
        oss << "\"" << delayedAmbulanceIds[i] << "\"";
        if (i + 1 < delayedAmbulanceIds.size()) oss << ",";
    }
    oss << "],\"assigned_delays\":{";

    size_t count = 0;
    for (const auto& [id, delay] : staggeredDelaysSeconds) {
        oss << "\"" << id << "\":" << delay;
        if (++count < staggeredDelaysSeconds.size()) oss << ",";
    }
    oss << "}}";
    return oss.str();
}

uint64_t IntelligentPriorityScheduler::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

double IntelligentPriorityScheduler::CalculateEffectiveScore(const SchedulingCandidate& c) {
    double base = c.medicalScore;
    if (base <= 0.0) {
        switch (c.priorityLevel) {
            case medical::PriorityLevel::CRITICAL: base = 90.0; break;
            case medical::PriorityLevel::HIGH:     base = 70.0; break;
            case medical::PriorityLevel::MEDIUM:   base = 40.0; break;
            case medical::PriorityLevel::LOW:      base = 15.0; break;
            default:                               base = 10.0; break;
        }
    }
    // Waiting time urgency bonus: 0.5 points per second held
    double waitBonus = static_cast<double>(c.waitingTimeSeconds) * 0.5;
    return base + waitBonus;
}

bool IntelligentPriorityScheduler::CompareCandidatesDeterministic(
    const SchedulingCandidate& a,
    const SchedulingCandidate& b) {

    // 1. Manual Operator Override takes absolute precedence
    if (a.manualOverrideLevel != b.manualOverrideLevel) {
        return a.manualOverrideLevel > b.manualOverrideLevel;
    }

    // 2. Corridor Occupancy: Unit currently inside/occupying junction clears first
    if (a.isCorridorOccupied != b.isCorridorOccupied) {
        return a.isCorridorOccupied > b.isCorridorOccupied;
    }

    // 3. Active Mission Status: Active missions take priority over waiting/paused
    bool aActive = (a.missionState == MissionState::ACTIVE);
    bool bActive = (b.missionState == MissionState::ACTIVE);
    if (aActive != bActive) {
        return aActive > bActive;
    }

    // 4. Medical Priority Score combined with Waiting Time
    double scoreA = CalculateEffectiveScore(a);
    double scoreB = CalculateEffectiveScore(b);
    if (std::abs(scoreA - scoreB) > 0.001) {
        return scoreA > scoreB;
    }

    // 5. Estimated Arrival Time (ETA): Earlier arrival gets corridor
    if (a.etaSeconds != b.etaSeconds) {
        return a.etaSeconds < b.etaSeconds;
    }

    // 6. Kinematic Distance Remaining: Closer vehicle clears first
    if (std::abs(a.distanceRemainingMeters - b.distanceRemainingMeters) > 0.1) {
        return a.distanceRemainingMeters < b.distanceRemainingMeters;
    }

    // 7. Deterministic Tie-Breaker: Lexicographical vehicle identifier
    return a.ambulanceId < b.ambulanceId;
}

IntelligentPriorityScheduler::IntelligentPriorityScheduler()
    : m_clearanceDurationSeconds(DEFAULT_CLEARANCE_DURATION_SECONDS) {}

IntelligentPriorityScheduler::IntelligentPriorityScheduler(uint32_t clearanceDurationSeconds)
    : m_clearanceDurationSeconds(clearanceDurationSeconds) {}

uint32_t IntelligentPriorityScheduler::GetClearanceDurationSeconds() const noexcept {
    return m_clearanceDurationSeconds;
}

void IntelligentPriorityScheduler::SetClearanceDurationSeconds(uint32_t seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clearanceDurationSeconds = seconds;
}

IntelligentScheduleDecision IntelligentPriorityScheduler::ScheduleIntersection(
    const std::string& intersectionId,
    std::vector<SchedulingCandidate> candidates) {

    IntelligentScheduleDecision decision;
    decision.intersectionId = intersectionId;
    decision.timestampEpochMs = CurrentTimeMillis();

    if (candidates.empty()) {
        decision.decisionReason = "No competing candidates at intersection";
        return decision;
    }

    // Calculate effective score for all candidates for telemetry
    for (auto& cand : candidates) {
        cand.effectiveCompositeScore = CalculateEffectiveScore(cand);
        decision.candidateScores[cand.ambulanceId] = cand.effectiveCompositeScore;
    }

    // Deterministic sort
    std::sort(candidates.begin(), candidates.end(), CompareCandidatesDeterministic);

    const auto& winner = candidates.front();
    decision.winnerAmbulanceId = winner.ambulanceId;
    decision.winnerScore = winner.effectiveCompositeScore;

    // Reason formulation
    std::ostringstream reasonOss;
    if (winner.manualOverrideLevel > 0) {
        reasonOss << "Manual operator override (Level " << static_cast<int>(winner.manualOverrideLevel) << ")";
    } else if (winner.isCorridorOccupied) {
        reasonOss << "Active corridor occupancy right-of-way";
    } else if (candidates.size() > 1 && winner.effectiveCompositeScore > candidates[1].effectiveCompositeScore) {
        reasonOss << "Higher clinical priority score (" << std::fixed << std::setprecision(1)
                  << winner.effectiveCompositeScore << " vs " << candidates[1].effectiveCompositeScore << ")";
    } else if (candidates.size() > 1 && winner.etaSeconds < candidates[1].etaSeconds) {
        reasonOss << "Earlier estimated arrival (" << winner.etaSeconds << "s vs "
                  << candidates[1].etaSeconds << "s)";
    } else if (candidates.size() > 1 && winner.distanceRemainingMeters < candidates[1].distanceRemainingMeters) {
        reasonOss << "Closer junction proximity (" << std::fixed << std::setprecision(0)
                  << winner.distanceRemainingMeters << "m vs " << candidates[1].distanceRemainingMeters << "m)";
    } else {
        reasonOss << "Deterministic identifier arbitration (" << winner.ambulanceId << ")";
    }
    decision.decisionReason = reasonOss.str();

    // Assign staggered clearance delays to non-winners
    uint32_t currentDelay = m_clearanceDurationSeconds;
    for (size_t i = 1; i < candidates.size(); ++i) {
        const auto& delayed = candidates[i];
        decision.delayedAmbulanceIds.push_back(delayed.ambulanceId);
        decision.staggeredDelaysSeconds[delayed.ambulanceId] = currentDelay;
        currentDelay += m_clearanceDurationSeconds;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_activeGreenGrants[intersectionId] = winner.ambulanceId;
        m_lastDecisions[intersectionId] = decision;
    }

    // Log resolution to event audit logger
    logger::EventLogger::Instance().LogConflictResolved(
        intersectionId,
        winner.ambulanceId,
        decision.decisionReason,
        logger::EventSeverity::INFO
    );

    return decision;
}

IntelligentScheduleDecision IntelligentPriorityScheduler::ScheduleConflict(
    const ConflictEvent& conflict,
    const std::vector<ambulance::AmbulanceProfile>& profiles,
    const std::unordered_map<std::string, uint32_t>& waitingTimes,
    const std::string& occupiedAmbulanceId,
    const std::unordered_map<std::string, MissionState>& missionStates) {

    std::vector<SchedulingCandidate> candidates;

    for (const auto& arr : conflict.conflictingArrivals) {
        SchedulingCandidate cand;
        cand.ambulanceId = arr.ambulanceId;
        cand.etaSeconds = arr.etaSeconds;
        cand.distanceRemainingMeters = arr.distanceRemainingMeters;

        // Lookup profile details
        bool foundProfile = false;
        for (const auto& prof : profiles) {
            if (prof.GetAmbulanceId() == arr.ambulanceId) {
                cand.manualOverrideLevel = prof.GetManualOverrideLevel();
                cand.priorityLevel = prof.GetPriority();
                cand.medicalScore = prof.GetPriorityScore();
                if (cand.medicalScore <= 0.0 && prof.HasPatient()) {
                    cand.medicalScore = medical::PriorityEngine::CalculateEmergencyScore(*prof.GetPatient());
                }
                foundProfile = true;
                break;
            }
        }

        if (!foundProfile) {
            cand.manualOverrideLevel = arr.profile.GetManualOverrideLevel();
            cand.priorityLevel = arr.profile.GetPriority();
            cand.medicalScore = arr.profile.GetPriorityScore();
            if (cand.medicalScore <= 0.0 && arr.profile.HasPatient()) {
                cand.medicalScore = medical::PriorityEngine::CalculateEmergencyScore(*arr.profile.GetPatient());
            }
        }

        // Waiting time
        auto waitIt = waitingTimes.find(arr.ambulanceId);
        if (waitIt != waitingTimes.end()) {
            cand.waitingTimeSeconds = waitIt->second;
        }

        // Corridor occupancy
        cand.isCorridorOccupied = (!occupiedAmbulanceId.empty() && arr.ambulanceId == occupiedAmbulanceId);

        // Mission state
        auto mIt = missionStates.find(arr.ambulanceId);
        if (mIt != missionStates.end()) {
            cand.missionState = mIt->second;
        } else {
            cand.missionState = MissionState::ACTIVE;
        }

        candidates.push_back(cand);
    }

    return ScheduleIntersection(conflict.intersectionId, candidates);
}

bool IntelligentPriorityScheduler::IsGreenGranted(
    const std::string& ambulanceId,
    const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeGreenGrants.find(intersectionId);
    if (it != m_activeGreenGrants.end()) {
        return it->second == ambulanceId;
    }
    return false;
}

std::string IntelligentPriorityScheduler::GetGrantedAmbulance(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeGreenGrants.find(intersectionId);
    if (it != m_activeGreenGrants.end()) {
        return it->second;
    }
    return "";
}

void IntelligentPriorityScheduler::GrantGreen(const std::string& intersectionId, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants[intersectionId] = ambulanceId;
}

void IntelligentPriorityScheduler::RevokeGreen(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants.erase(intersectionId);
}

void IntelligentPriorityScheduler::ClearGrants() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants.clear();
}

std::optional<IntelligentScheduleDecision> IntelligentPriorityScheduler::GetLastDecision(
    const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_lastDecisions.find(intersectionId);
    if (it != m_lastDecisions.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<IntelligentScheduleDecision> IntelligentPriorityScheduler::GetAllLastDecisions() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntelligentScheduleDecision> result;
    for (const auto& [id, dec] : m_lastDecisions) {
        result.push_back(dec);
    }
    return result;
}

void IntelligentPriorityScheduler::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeGreenGrants.clear();
    m_lastDecisions.clear();
}

} // namespace scheduler
} // namespace aegis
