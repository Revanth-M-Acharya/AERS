/**
 * @file CongestionAnalyzer.cpp
 * @brief Implementation of CongestionAnalyzer speed deficit and trend tracking.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/CongestionAnalyzer.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

std::string CongestionTrendToString(CongestionTrend trend) {
    switch (trend) {
        case CongestionTrend::FREE_FLOW:   return "FREE_FLOW";
        case CongestionTrend::BUILDING:    return "BUILDING";
        case CongestionTrend::GRIDLOCK:    return "GRIDLOCK";
        case CongestionTrend::DISSIPATING: return "DISSIPATING";
        default:                           return "UNKNOWN";
    }
}

CongestionMetrics CongestionAnalyzer::UpdateSpeed(
    const std::string& segmentId,
    double currentSpeedKmh,
    double freeFlowSpeedKmh,
    double segmentLengthMeters) {

    if (freeFlowSpeedKmh <= 0.0) freeFlowSpeedKmh = 50.0;
    if (segmentLengthMeters <= 0.0) segmentLengthMeters = 500.0;
    currentSpeedKmh = std::max(0.0, currentSpeedKmh);

    double freeFlowTransitSec = (segmentLengthMeters / 1000.0) / (freeFlowSpeedKmh / 3600.0);
    double actualTransitSec = (currentSpeedKmh > 1.0)
        ? ((segmentLengthMeters / 1000.0) / (currentSpeedKmh / 3600.0))
        : (freeFlowTransitSec + 300.0); // severe delay if stopped

    double delaySec = std::max(0.0, actualTransitSec - freeFlowTransitSec);
    double deficit = std::clamp((freeFlowSpeedKmh - currentSpeedKmh) / freeFlowSpeedKmh, 0.0, 1.0);

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metricsMap_.find(segmentId);
    CongestionTrend trend = CongestionTrend::FREE_FLOW;

    if (it != metricsMap_.end()) {
        double prevSpeed = it->second.currentAverageSpeedKmh;
        if (deficit < 0.15) {
            trend = CongestionTrend::FREE_FLOW;
        } else if (deficit > 0.70) {
            trend = CongestionTrend::GRIDLOCK;
        } else if (currentSpeedKmh < prevSpeed - 2.0) {
            trend = CongestionTrend::BUILDING;
        } else if (currentSpeedKmh > prevSpeed + 2.0) {
            trend = CongestionTrend::DISSIPATING;
        } else {
            trend = it->second.trend;
        }
    } else {
        if (deficit < 0.15) trend = CongestionTrend::FREE_FLOW;
        else if (deficit > 0.70) trend = CongestionTrend::GRIDLOCK;
        else trend = CongestionTrend::BUILDING;
    }

    CongestionMetrics metrics;
    metrics.segmentId = segmentId;
    metrics.currentAverageSpeedKmh = currentSpeedKmh;
    metrics.freeFlowSpeedKmh = freeFlowSpeedKmh;
    metrics.speedDeficitRatio = deficit;
    metrics.estimatedTransitDelaySec = delaySec;
    metrics.trend = trend;
    metrics.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    metricsMap_[segmentId] = metrics;
    return metrics;
}

CongestionMetrics CongestionAnalyzer::GetMetrics(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metricsMap_.find(segmentId);
    if (it != metricsMap_.end()) return it->second;
    return CongestionMetrics{segmentId};
}

double CongestionAnalyzer::GetEstimatedDelay(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metricsMap_.find(segmentId);
    if (it != metricsMap_.end()) return it->second.estimatedTransitDelaySec;
    return 0.0;
}

CongestionTrend CongestionAnalyzer::GetTrend(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = metricsMap_.find(segmentId);
    if (it != metricsMap_.end()) return it->second.trend;
    return CongestionTrend::FREE_FLOW;
}

void CongestionAnalyzer::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    metricsMap_.clear();
}

} // namespace aegis::coordinator::perception::analytics

