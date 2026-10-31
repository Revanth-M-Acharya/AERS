/**
 * @file RoadSuitabilityCalculator.hpp
 * @brief Multi-factor composite Road Suitability Score (0-100) advisory evaluator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include "coordinator/perception/analytics/TrafficDensityAnalyzer.hpp"
#include "coordinator/perception/analytics/QueueLengthEstimator.hpp"
#include "coordinator/perception/analytics/LaneOccupancyEstimator.hpp"
#include "coordinator/perception/analytics/RoadHealthAnalyzer.hpp"
#include "coordinator/perception/analytics/RoadBlockageAnalyzer.hpp"
#include "coordinator/perception/analytics/CongestionAnalyzer.hpp"
#include "coordinator/perception/analytics/TrafficPredictionEngine.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace aegis::coordinator::perception::analytics {

struct RoadSuitabilityResult {
    std::string segmentId;
    double suitabilityScore{100.0}; // 0.0 to 100.0
    std::string rating{"OPTIMAL"};  // "OPTIMAL", "GOOD", "DEGRADED", "POOR", "IMPASSABLE"
    bool isPassable{true};
    bool isRecommendedForCorridor{true};
    std::vector<std::string> penaltyReasons;
    uint64_t computedMonoMs{0};
};

class RoadSuitabilityCalculator {
public:
    RoadSuitabilityCalculator() = default;
    ~RoadSuitabilityCalculator() = default;

    /**
     * @brief Compute composite road suitability score (0-100) based on 9 perception factors.
     */
    RoadSuitabilityResult CalculateSuitability(
        const std::string& segmentId,
        const SegmentDensityMetrics& density,
        const QueueMetrics& queue,
        const SegmentLanesOverview& lanes,
        const RoadHealthMetrics& health,
        const std::vector<RoadConditionEvent>& activeEvents,
        const PredictedCondition& prediction,
        const CongestionMetrics& congestion,
        const SegmentBlockageReport& blockage);

    RoadSuitabilityResult GetCachedSuitability(const std::string& segmentId) const;
    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RoadSuitabilityResult> cache_;
};

} // namespace aegis::coordinator::perception::analytics

