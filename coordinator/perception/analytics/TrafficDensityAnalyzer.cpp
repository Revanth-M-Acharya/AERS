/**
 * @file TrafficDensityAnalyzer.cpp
 * @brief Implementation of TrafficDensityAnalyzer vehicular density and LOS calculation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/TrafficDensityAnalyzer.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

std::string LevelOfServiceToString(LevelOfService los) {
    switch (los) {
        case LevelOfService::LOS_A: return "LOS_A";
        case LevelOfService::LOS_B: return "LOS_B";
        case LevelOfService::LOS_C: return "LOS_C";
        case LevelOfService::LOS_D: return "LOS_D";
        case LevelOfService::LOS_E: return "LOS_E";
        case LevelOfService::LOS_F: return "LOS_F";
        default:                    return "UNKNOWN";
    }
}

SegmentDensityMetrics TrafficDensityAnalyzer::UpdateDensity(
    const std::string& segmentId,
    uint32_t detectedVehicles,
    uint32_t laneCount,
    double segmentLengthMeters) {

    if (laneCount == 0) laneCount = 1;
    if (segmentLengthMeters <= 0.0) segmentLengthMeters = 500.0;

    double lengthKm = segmentLengthMeters / 1000.0;
    double densityPerLane = (static_cast<double>(detectedVehicles) / lengthKm) / static_cast<double>(laneCount);

    LevelOfService los = LevelOfService::LOS_A;
    if (densityPerLane < 7.0) {
        los = LevelOfService::LOS_A;
    } else if (densityPerLane < 11.0) {
        los = LevelOfService::LOS_B;
    } else if (densityPerLane < 16.0) {
        los = LevelOfService::LOS_C;
    } else if (densityPerLane < 22.0) {
        los = LevelOfService::LOS_D;
    } else if (densityPerLane < 28.0) {
        los = LevelOfService::LOS_E;
    } else {
        los = LevelOfService::LOS_F;
    }

    // Jam density threshold is approximately 40 vehicles/km/lane
    double normalized = std::clamp(densityPerLane / 40.0, 0.0, 1.0);

    std::lock_guard<std::mutex> lock(mutex_);
    SegmentDensityMetrics metrics;
    metrics.segmentId = segmentId;
    metrics.currentVehicleCount = detectedVehicles;
    metrics.laneCount = laneCount;
    metrics.segmentLengthMeters = segmentLengthMeters;
    metrics.vehiclesPerKmPerLane = densityPerLane;
    metrics.normalizedDensityScore = normalized;
    metrics.los = los;
    metrics.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    segmentMetrics_[segmentId] = metrics;
    return metrics;
}

SegmentDensityMetrics TrafficDensityAnalyzer::GetMetrics(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentMetrics_.find(segmentId);
    if (it != segmentMetrics_.end()) return it->second;
    return SegmentDensityMetrics{segmentId};
}

LevelOfService TrafficDensityAnalyzer::GetLevelOfService(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentMetrics_.find(segmentId);
    if (it != segmentMetrics_.end()) return it->second.los;
    return LevelOfService::LOS_A;
}

double TrafficDensityAnalyzer::GetNormalizedDensity(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segmentMetrics_.find(segmentId);
    if (it != segmentMetrics_.end()) return it->second.normalizedDensityScore;
    return 0.0;
}

void TrafficDensityAnalyzer::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    segmentMetrics_.clear();
}

} // namespace aegis::coordinator::perception::analytics

