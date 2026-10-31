/**
 * @file LaneOccupancyEstimator.hpp
 * @brief Evaluates per-lane occupancy and identifies unobstructed lanes for emergency vehicles.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis::coordinator::perception::analytics {

struct LaneStatus {
    uint32_t laneIndex{1};          // 1-based index (e.g., 1 = Leftmost/Fast lane)
    double occupancyRatio{0.0};     // 0.0 (empty) to 1.0 (fully saturated)
    bool isBlocked{false};          // Stopped vehicle, debris, or accident on lane
    double averageSpeedKmh{50.0};
};

struct SegmentLanesOverview {
    std::string segmentId;
    std::vector<LaneStatus> lanes;
    uint32_t recommendedEmergencyLane{1}; // Best lane for ambulance
    bool isCompletelyBlocked{false};
    uint64_t lastUpdatedMonoMs{0};
};

class LaneOccupancyEstimator {
public:
    LaneOccupancyEstimator() = default;
    ~LaneOccupancyEstimator() = default;

    SegmentLanesOverview UpdateLaneOccupancy(const std::string& segmentId,
                                            const std::vector<LaneStatus>& lanes);

    SegmentLanesOverview GetOverview(const std::string& segmentId) const;
    uint32_t GetRecommendedLane(const std::string& segmentId) const;
    bool IsLanePassable(const std::string& segmentId, uint32_t laneIndex) const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SegmentLanesOverview> segmentLanesMap_;
};

} // namespace aegis::coordinator::perception::analytics

