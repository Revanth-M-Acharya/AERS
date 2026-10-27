/**
 * @file PriorityScheduler.h
 * @brief Multi-Ambulance Priority Scheduling and Conflict Resolution Engine.
 */

#ifndef AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H
#define AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/ConflictDetector.h"
#include "medical/PriorityEngine.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Output result of resolving a junction contention between two or more ambulances.
 */
struct ConflictResolution {
    std::string intersectionId{""};
    std::string winnerAmbulanceId{""};
    std::vector<std::string> delayedAmbulanceIds{};
    uint32_t delayDurationSeconds{15};                                 ///< Clearance window for winner
    std::unordered_map<std::string, uint32_t> assignedDelaysSeconds{}; ///< Delay assigned to each delayed unit
    std::string resolutionReason{""};
    uint64_t timestampEpochMs{0};

    bool HasDelayedAmbulance(const std::string& ambId) const {
        for (const auto& delayed : delayedAmbulanceIds) {
            if (delayed == ambId) return true;
        }
        return false;
    }
};

/**
 * @brief Real-time scheduling engine that arbitrates junction contention and guarantees exclusive green corridors.
 */
class PriorityScheduler {
public:
    static constexpr uint32_t DEFAULT_CLEARANCE_SECONDS = 15;

    PriorityScheduler();
    explicit PriorityScheduler(uint32_t conflictWindowSeconds,
                               uint32_t clearanceDurationSeconds = DEFAULT_CLEARANCE_SECONDS);
    ~PriorityScheduler() = default;

    // Configuration
    void SetConflictWindowSeconds(uint32_t seconds) noexcept;
    uint32_t GetConflictWindowSeconds() const noexcept;

    void SetClearanceDurationSeconds(uint32_t seconds) noexcept;
    uint32_t GetClearanceDurationSeconds() const noexcept;

    // Arbitration & Resolution
    ConflictResolution ResolveConflict(const IntersectionConflict& conflict);
    std::vector<ConflictResolution> ScheduleAmbulances(const std::vector<ambulance::AmbulanceProfile>& ambulances);

    // Green Corridor Grants & Safety Guarantees
    bool IsAmbulanceGrantedGreen(const std::string& ambulanceId, const std::string& intersectionId) const;
    std::string GetGrantedAmbulanceForIntersection(const std::string& intersectionId) const;
    void RevokeGreenGrant(const std::string& intersectionId);
    void ForceGreenGrant(const std::string& intersectionId, const std::string& ambulanceId);
    
    // Inspection & Queries
    std::vector<ConflictResolution> GetActiveResolutions() const;
    std::optional<ConflictResolution> GetResolutionForIntersection(const std::string& intersectionId) const;
    void Clear();

private:
    static int PriorityToRank(medical::PriorityLevel priority);
    static bool CompareAmbulanceUrgency(const AmbulanceArrival& a, const AmbulanceArrival& b);

    mutable std::mutex m_mutex;
    ConflictDetector m_detector;
    uint32_t m_clearanceDurationSeconds{DEFAULT_CLEARANCE_SECONDS};
    std::unordered_map<std::string, std::string> m_activeGreenGrants{};       ///< intersectionId -> winnerAmbulanceId
    std::unordered_map<std::string, ConflictResolution> m_lastResolutions{}; ///< intersectionId -> ConflictResolution
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_PRIORITY_SCHEDULER_H
