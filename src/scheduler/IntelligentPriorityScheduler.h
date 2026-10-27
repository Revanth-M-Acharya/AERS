/**
 * @file IntelligentPriorityScheduler.h
 * @brief Deterministic Multi-Factor Intelligent Priority Scheduler for SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_INTELLIGENT_PRIORITY_SCHEDULER_H
#define AEGIS_SCHEDULER_INTELLIGENT_PRIORITY_SCHEDULER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>
#include <memory>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/MissionManager.h"
#include "scheduler/ConflictDetectionEngine.h"
#include "logger/EventLogger.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Evaluation snapshot of an ambulance competing for green corridor right-of-way.
 */
struct SchedulingCandidate {
    std::string ambulanceId{""};
    double medicalScore{0.0};
    medical::PriorityLevel priorityLevel{medical::PriorityLevel::LOW};
    uint32_t etaSeconds{0};
    uint32_t waitingTimeSeconds{0};
    uint8_t manualOverrideLevel{0};
    bool isCorridorOccupied{false};
    MissionState missionState{MissionState::ACTIVE};
    double distanceRemainingMeters{0.0};
    double effectiveCompositeScore{0.0};

    SchedulingCandidate() = default;

    SchedulingCandidate(std::string ambId,
                        double medScore,
                        medical::PriorityLevel pLevel,
                        uint32_t etaSec,
                        uint32_t waitSec = 0,
                        uint8_t overrideLvl = 0,
                        bool occupied = false,
                        MissionState mState = MissionState::ACTIVE,
                        double distMeters = 0.0)
        : ambulanceId(std::move(ambId))
        , medicalScore(medScore)
        , priorityLevel(pLevel)
        , etaSeconds(etaSec)
        , waitingTimeSeconds(waitSec)
        , manualOverrideLevel(overrideLvl)
        , isCorridorOccupied(occupied)
        , missionState(mState)
        , distanceRemainingMeters(distMeters)
        , effectiveCompositeScore(0.0) {}
};

/**
 * @brief Deterministic arbitration decision produced by IntelligentPriorityScheduler.
 */
struct IntelligentScheduleDecision {
    std::string intersectionId{""};
    std::string winnerAmbulanceId{""};
    std::vector<std::string> delayedAmbulanceIds{};
    std::unordered_map<std::string, uint32_t> staggeredDelaysSeconds{};
    std::string decisionReason{""};
    double winnerScore{0.0};
    std::unordered_map<std::string, double> candidateScores{};
    uint64_t timestampEpochMs{0};

    bool HasDelayedAmbulance(const std::string& ambId) const;
    uint32_t GetAssignedDelay(const std::string& ambId) const;
    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Real-time multi-factor scheduler resolving junction contention using medical priority score,
 * estimated arrival time, waiting time, manual override, corridor occupancy, and active mission status.
 */
class IntelligentPriorityScheduler {
public:
    static constexpr uint32_t DEFAULT_CLEARANCE_DURATION_SECONDS = 15;

    IntelligentPriorityScheduler();
    explicit IntelligentPriorityScheduler(uint32_t clearanceDurationSeconds);
    ~IntelligentPriorityScheduler() = default;

    // Configuration
    uint32_t GetClearanceDurationSeconds() const noexcept;
    void SetClearanceDurationSeconds(uint32_t seconds) noexcept;

    // Strict Deterministic Comparator
    static bool CompareCandidatesDeterministic(const SchedulingCandidate& a, const SchedulingCandidate& b);

    // Scheduling APIs
    IntelligentScheduleDecision ScheduleIntersection(const std::string& intersectionId,
                                                    std::vector<SchedulingCandidate> candidates);

    IntelligentScheduleDecision ScheduleConflict(
        const ConflictEvent& conflict,
        const std::vector<ambulance::AmbulanceProfile>& profiles,
        const std::unordered_map<std::string, uint32_t>& waitingTimes = {},
        const std::string& occupiedAmbulanceId = "",
        const std::unordered_map<std::string, MissionState>& missionStates = {});

    // Green Corridor Grants & State
    bool IsGreenGranted(const std::string& ambulanceId, const std::string& intersectionId) const;
    std::string GetGrantedAmbulance(const std::string& intersectionId) const;
    void GrantGreen(const std::string& intersectionId, const std::string& ambulanceId);
    void RevokeGreen(const std::string& intersectionId);
    void ClearGrants();

    // Queries
    std::optional<IntelligentScheduleDecision> GetLastDecision(const std::string& intersectionId) const;
    std::vector<IntelligentScheduleDecision> GetAllLastDecisions() const;
    void Clear();

private:
    static uint64_t CurrentTimeMillis();
    static double CalculateEffectiveScore(const SchedulingCandidate& c);

    mutable std::mutex m_mutex;
    uint32_t m_clearanceDurationSeconds{DEFAULT_CLEARANCE_DURATION_SECONDS};
    std::unordered_map<std::string, std::string> m_activeGreenGrants{};               ///< intersectionId -> winnerAmbulanceId
    std::unordered_map<std::string, IntelligentScheduleDecision> m_lastDecisions{};  ///< intersectionId -> decision
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_INTELLIGENT_PRIORITY_SCHEDULER_H
