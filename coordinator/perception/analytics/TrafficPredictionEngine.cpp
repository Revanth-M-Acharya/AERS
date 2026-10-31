/**
 * @file TrafficPredictionEngine.cpp
 * @brief Implementation of TrafficPredictionEngine kinematic projection and ETA confidence.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/TrafficPredictionEngine.hpp"
#include <algorithm>
#include <cmath>

namespace aegis::coordinator::perception::analytics {

PredictedCondition TrafficPredictionEngine::PredictConditionAtEta(
    const std::string& segmentId,
    double etaSecondsAhead,
    const SegmentDensityMetrics& currentDensity,
    const CongestionMetrics& currentCongestion) {

    double predictedSpeed = currentCongestion.currentAverageSpeedKmh;
    double predictedDensity = currentDensity.normalizedDensityScore;

    if (etaSecondsAhead < 0.0) etaSecondsAhead = 0.0;

    // Kinematic extrapolation based on trend
    if (currentCongestion.trend == CongestionTrend::BUILDING) {
        double decayFactor = (etaSecondsAhead / 300.0) * 0.25; // Speed drops up to 25% over 5m
        predictedSpeed = std::max(5.0, predictedSpeed * (1.0 - decayFactor));
        predictedDensity = std::clamp(predictedDensity + (etaSecondsAhead / 300.0) * 0.20, 0.0, 1.0);
    } else if (currentCongestion.trend == CongestionTrend::DISSIPATING) {
        double recoveryFactor = (etaSecondsAhead / 300.0) * 0.20;
        predictedSpeed = std::min(currentCongestion.freeFlowSpeedKmh, predictedSpeed * (1.0 + recoveryFactor));
        predictedDensity = std::clamp(predictedDensity - (etaSecondsAhead / 300.0) * 0.15, 0.0, 1.0);
    } else if (currentCongestion.trend == CongestionTrend::GRIDLOCK) {
        predictedSpeed = std::min(predictedSpeed, 5.0);
        predictedDensity = 1.0;
    }

    LevelOfService los = currentDensity.los;
    if (predictedDensity > 0.85) los = LevelOfService::LOS_F;
    else if (predictedDensity > 0.65) los = LevelOfService::LOS_E;
    else if (predictedDensity > 0.45) los = LevelOfService::LOS_D;

    double segLenKm = currentDensity.segmentLengthMeters / 1000.0;
    double freeFlowSec = segLenKm / (currentCongestion.freeFlowSpeedKmh / 3600.0);
    double predTransitSec = (predictedSpeed > 1.0) ? (segLenKm / (predictedSpeed / 3600.0)) : (freeFlowSec + 300.0);
    double predDelay = std::max(0.0, predTransitSec - freeFlowSec);

    // Prediction confidence decays with temporal distance
    double predConf = 0.98 * std::exp(-0.0015 * etaSecondsAhead);
    predConf = std::clamp(predConf, 0.50, 0.98);

    // Expected variance / error in seconds grows with horizon
    double expectedError = 1.5 + (0.04 * etaSecondsAhead);

    PredictedCondition pred;
    pred.segmentId = segmentId;
    pred.horizonSeconds = etaSecondsAhead;
    pred.predictionHorizonSeconds = etaSecondsAhead;
    pred.predictedSpeedKmh = predictedSpeed;
    pred.predictedDensityScore = predictedDensity;
    pred.predictedLos = los;
    pred.predictedDelaySec = predDelay;
    pred.likelyImpasse = (predictedDensity > 0.90 || predictedSpeed < 3.0);
    pred.predictedEtaSeconds = predTransitSec;
    pred.predictionConfidence = predConf;
    pred.expectedErrorSeconds = expectedError;

    std::lock_guard<std::mutex> lock(mutex_);
    cache_[segmentId] = pred;
    return pred;
}

void TrafficPredictionEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

} // namespace aegis::coordinator::perception::analytics
