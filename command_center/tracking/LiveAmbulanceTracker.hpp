/**
 * @file LiveAmbulanceTracker.hpp
 * @brief Real-time tracking of active emergency ambulances across intersections and green corridors.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

struct AmbulanceTrackingState {
    std::string ambulanceId;
    std::string missionId;
    std::vector<std::string> currentRoute;
    std::string currentIntersection{"HOME"};
    std::string nextIntersection{"INT_1"};
    double remainingDistanceMeters{1450.0};
    double currentSpeedKmh{52.0};
    double estimatedArrivalSec{42.0};
    std::string missionPriority{"CRITICAL (Code Red)"};
    std::string greenCorridorStatus{"ACTIVE"}; // ACTIVE, CLEARING, RESTORED
    uint64_t lastUpdateTimestampMonoMs{0};
};

class LiveAmbulanceTracker {
public:
    LiveAmbulanceTracker() = default;
    ~LiveAmbulanceTracker() = default;

    // Prevent copying
    LiveAmbulanceTracker(const LiveAmbulanceTracker&) = delete;
    LiveAmbulanceTracker& operator=(const LiveAmbulanceTracker&) = delete;

    /**
     * @brief Update or insert live tracking state for an ambulance.
     */
    void UpdateTracking(const AmbulanceTrackingState& state);

    /**
     * @brief Update progress along current route intersections.
     */
    bool UpdateIntersectionProgress(const std::string& ambulanceId,
                                    const std::string& currentInt,
                                    const std::string& nextInt,
                                    double remainingDistMeters,
                                    double speedKmh,
                                    double etaSec);

    /**
     * @brief Update green corridor preemption status.
     */
    bool UpdateCorridorStatus(const std::string& ambulanceId, const std::string& corridorStatus);

    /**
     * @brief Remove tracking when mission finishes.
     */
    bool RemoveTracking(const std::string& ambulanceId);

    /**
     * @brief Get live tracking state for a single vehicle.
     */
    std::optional<AmbulanceTrackingState> GetTracking(const std::string& ambulanceId) const;

    /**
     * @brief Retrieve snapshot of all currently tracked active ambulances.
     */
    std::vector<AmbulanceTrackingState> GetAllActiveTracking() const;

    /**
     * @brief Clear all tracking entries.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, AmbulanceTrackingState> trackingMap_;
};

} // namespace aegis::command_center
