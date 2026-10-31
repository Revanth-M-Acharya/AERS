/**
 * @file ContinuousETAEngine.hpp
 * @brief Continuous ETA estimator using instantaneous kinematics and historical moving averages.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/eta/IETAEngine.hpp"
#include <unordered_map>
#include <mutex>

namespace aegis::coordinator::intelligence {

class ContinuousETAEngine : public IETAEngine {
public:
    explicit ContinuousETAEngine(double emaAlpha = 0.25);
    ~ContinuousETAEngine() override = default;

    ETAReport EstimateETA(const std::string& ambulanceId,
                          const std::vector<std::string>& remainingSegmentIds,
                          double currentSpeedKmh,
                          const RoadGraph& graph) override;

    void UpdateHistoricalEdgeTraversal(const std::string& segmentId, double observedTimeSec) override;

    double GetHistoricalTraversalTime(const std::string& segmentId) const;

private:
    double emaAlpha_{0.25};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, double> historicalTraversalTimes_;
};

} // namespace aegis::coordinator::intelligence

