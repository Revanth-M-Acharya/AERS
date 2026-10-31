/**
 * @file TrafficPredictionEngine.hpp
 * @brief Forward-looking traffic prediction model for ambulance arrival time windows.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/analytics/TrafficDensityAnalyzer.hpp"
#include "coordinator/perception/analytics/CongestionAnalyzer.hpp"
#include <string>
#include <unordered_map>
#include <mutex>

namespace aegis::coordinator::perception::analytics {

struct PredictedCondition {
    std::string segmentId;
    double horizonSeconds{180.0}; // e.g. 3 minutes into future
    double predictedSpeedKmh{45.0};
    double predictedDensityScore{0.3};
    LevelOfService predictedLos{LevelOfService::LOS_B};
    double predictedDelaySec{10.0};
    bool likelyImpasse{false};

    // Extended metrics (Phase 7 Refinement Pass)
    double predictedEtaSeconds{0.0};
    double predictionConfidence{0.95};     // 0.0 to 1.0 (decays as horizon expands)
    double predictionHorizonSeconds{180.0};
    double expectedErrorSeconds{2.0};       // Estimated variance +/- seconds
};

class TrafficPredictionEngine {
public:
    TrafficPredictionEngine() = default;
    ~TrafficPredictionEngine() = default;

    /**
     * @brief Projects traffic state and ambulance transit time at a future ETA horizon.
     */
    PredictedCondition PredictConditionAtEta(const std::string& segmentId,
                                            double etaSecondsAhead,
                                            const SegmentDensityMetrics& currentDensity,
                                            const CongestionMetrics& currentCongestion);

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, PredictedCondition> cache_;
};

} // namespace aegis::coordinator::perception::analytics
