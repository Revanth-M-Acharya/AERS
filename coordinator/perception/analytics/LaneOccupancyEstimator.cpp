/**
 * @file LaneOccupancyEstimator.cpp
 * @brief Implementation of LaneOccupancyEstimator and lane clearance recommendation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/LaneOccupancyEstimator.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

SegmentLanesOverview LaneOccupancyEstimator::UpdateLaneOccupancy(
    const std::string& segmentId,
    const std::vector<LaneStatus>& lanes) {

    SegmentLanesOverview overview;
    overview.segmentId = segmentId;
    overview.lanes = lanes;
    overview.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    if (lanes.empty()) {
        overview.recommendedEmergencyLane = 1;
        overview.isCompletelyBlocked = false;
    } else {
        bool allBlocked = true;
        uint32_t bestLane = 1;
        double minOccupancy = 999.0;

        for (const auto& lane : lanes) {
            if (!lane.isBlocked) {
                allBlocked = false;
                if (lane.occupancyRatio < minOccupancy) {
                    minOccupancy = lane.occupancyRatio;
                    bestLane = lane.laneIndex;
                }
            }
        }

        overview.isCompletelyBlocked = allBlocked;
        overview.recommendedEmergencyLane = allBlocked ? 0 : bestLane;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    segmentLanesMap_[segmentId] = overview;
    return overview;
}

SegmentLanesOverview LaneOccupancyEstimator::GetOverview(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentLanesMap_.find(segmentId);
    if (it != segmentLanesMap_.end()) return it->second;
    SegmentLanesOverview def;
    def.segmentId = segmentId;
    return def;
}

uint32_t LaneOccupancyEstimator::GetRecommendedLane(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentLanesMap_.find(segmentId);
    if (it != segmentLanesMap_.end()) return it->second.recommendedEmergencyLane;
    return 1;
}

bool LaneOccupancyEstimator::IsLanePassable(const std::string& segmentId, uint32_t laneIndex) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentLanesMap_.find(segmentId);
    if (it == segmentLanesMap_.end()) return true;

    for (const auto& l : it->second.lanes) {
        if (l.laneIndex == laneIndex) {
            return (!l.isBlocked && l.occupancyRatio < 0.95);
        }
    }
    return true;
}

void LaneOccupancyEstimator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    segmentLanesMap_.clear();
}

} // namespace aegis::coordinator::perception::analytics
