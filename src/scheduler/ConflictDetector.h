/**
 * @file ConflictDetector.h
 * @brief Predicts junction arrival windows and detects multi-ambulance spatial-temporal conflicts.
 */

#ifndef AEGIS_SCHEDULER_CONFLICT_DETECTOR_H
#define AEGIS_SCHEDULER_CONFLICT_DETECTOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <algorithm>

#include "ambulance/AmbulanceProfile.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Telemetry snapshot representing an ambulance approaching a specific junction.
 */
struct AmbulanceArrival {
    std::string ambulanceId{""};
    std::string intersectionId{""};
    uint32_t etaSeconds{0};              ///< Arrival time relative to evaluation epoch (seconds)
    double distanceRemainingMeters{0.0}; ///< Kinematic distance to junction stopline (meters)
    ambulance::AmbulanceProfile profile{};

    AmbulanceArrival() = default;

    AmbulanceArrival(std::string ambId,
                     std::string intId,
                     uint32_t etaSec,
                     double distM = 0.0,
                     ambulance::AmbulanceProfile prof = ambulance::AmbulanceProfile{})
        : ambulanceId(std::move(ambId))
        , intersectionId(std::move(intId))
        , etaSeconds(etaSec)
        , distanceRemainingMeters(distM)
        , profile(std::move(prof)) {}
};

/**
 * @brief Encapsulates a spatial-temporal conflict event at a specific intersection.
 */
struct IntersectionConflict {
    std::string intersectionId{""};
    std::vector<AmbulanceArrival> conflictingArrivals{};
    uint32_t timeDeltaSeconds{0};
    uint32_t earliestEtaSeconds{0};
    uint32_t latestEtaSeconds{0};

    size_t GetConflictCount() const noexcept { return conflictingArrivals.size(); }

    bool HasAmbulance(const std::string& ambId) const {
        for (const auto& arr : conflictingArrivals) {
            if (arr.ambulanceId == ambId) return true;
        }
        return false;
    }
};

/**
 * @brief Evaluates multi-vehicle ETA timelines and detects junction contention within configurable windows.
 */
class ConflictDetector {
public:
    static constexpr uint32_t DEFAULT_CONFLICT_WINDOW_SECONDS = 10;

    ConflictDetector();
    explicit ConflictDetector(uint32_t conflictWindowSeconds);
    ~ConflictDetector() = default;

    // Window Configuration
    uint32_t GetConflictWindowSeconds() const noexcept;
    void SetConflictWindowSeconds(uint32_t seconds) noexcept;

    // Arrival Registration
    void RegisterArrival(const AmbulanceArrival& arrival);
    void RegisterArrival(const std::string& ambulanceId,
                         const std::string& intersectionId,
                         uint32_t etaSeconds,
                         double distanceMeters = 0.0,
                         const ambulance::AmbulanceProfile& profile = ambulance::AmbulanceProfile{});
    void ClearArrivals();

    // Conflict Detection
    std::vector<IntersectionConflict> DetectConflicts() const;
    std::vector<IntersectionConflict> DetectConflicts(const std::vector<ambulance::AmbulanceProfile>& ambulances,
                                                      uint32_t windowSeconds = DEFAULT_CONFLICT_WINDOW_SECONDS);

    // Queries
    std::vector<AmbulanceArrival> GetArrivalsForIntersection(const std::string& intersectionId) const;
    size_t GetTotalArrivalCount() const;

private:
    mutable std::mutex m_mutex;
    uint32_t m_conflictWindowSeconds{DEFAULT_CONFLICT_WINDOW_SECONDS};
    std::unordered_map<std::string, std::vector<AmbulanceArrival>> m_intersectionArrivals{};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_CONFLICT_DETECTOR_H
