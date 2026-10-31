/**
 * @file RoadHealthAnalyzer.cpp
 * @brief Implementation of RoadHealthAnalyzer surface hazard scoring.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/RoadHealthAnalyzer.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

RoadHealthMetrics RoadHealthAnalyzer::RecordHazard(
    const std::string& segmentId,
    RoadEventType hazardType,
    EventSeverity severity,
    double defaultSpeedLimitKmh) {

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = healthMap_.find(segmentId);
    RoadHealthMetrics metrics;
    if (it != healthMap_.end()) {
        metrics = it->second;
    } else {
        metrics.segmentId = segmentId;
        metrics.recommendedSpeedKmh = defaultSpeedLimitKmh;
    }

    double penalty = 0.0;
    switch (severity) {
        case EventSeverity::INFO:     penalty = 0.05; break;
        case EventSeverity::LOW:      penalty = 0.15; break;
        case EventSeverity::MEDIUM:   penalty = 0.30; break;
        case EventSeverity::HIGH:     penalty = 0.50; break;
        case EventSeverity::CRITICAL: penalty = 0.85; break;
    }

    if (hazardType == RoadEventType::POTHOLE) {
        metrics.hasPotholes = true;
    } else if (hazardType == RoadEventType::FLOODING) {
        metrics.hasFlooding = true;
        penalty = std::max(penalty, 0.60);
    } else if (hazardType == RoadEventType::FALLEN_TREE) {
        metrics.hasDebrisOrFallenTree = true;
        penalty = std::max(penalty, 0.75);
    }

    metrics.healthScore = std::clamp(metrics.healthScore - penalty, 0.0, 1.0);
    metrics.recommendedSpeedKmh = defaultSpeedLimitKmh * std::max(0.25, metrics.healthScore);
    metrics.isSafeForEmergencyTransit = (metrics.healthScore >= 0.30 && !metrics.hasFlooding);
    metrics.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    healthMap_[segmentId] = metrics;
    return metrics;
}

RoadHealthMetrics RoadHealthAnalyzer::ClearHazards(const std::string& segmentId, double defaultSpeedLimitKmh) {
    std::lock_guard<std::mutex> lock(mutex_);
    RoadHealthMetrics metrics;
    metrics.segmentId = segmentId;
    metrics.healthScore = 1.0;
    metrics.recommendedSpeedKmh = defaultSpeedLimitKmh;
    metrics.hasPotholes = false;
    metrics.hasFlooding = false;
    metrics.hasDebrisOrFallenTree = false;
    metrics.isSafeForEmergencyTransit = true;
    metrics.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    healthMap_[segmentId] = metrics;
    return metrics;
}

RoadHealthMetrics RoadHealthAnalyzer::GetHealth(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = healthMap_.find(segmentId);
    if (it != healthMap_.end()) return it->second;
    return RoadHealthMetrics{segmentId};
}

bool RoadHealthAnalyzer::IsSegmentSafe(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = healthMap_.find(segmentId);
    if (it != healthMap_.end()) return it->second.isSafeForEmergencyTransit;
    return true;
}

double RoadHealthAnalyzer::GetSpeedMultiplier(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = healthMap_.find(segmentId);
    if (it != healthMap_.end()) return it->second.healthScore;
    return 1.0;
}

void RoadHealthAnalyzer::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    healthMap_.clear();
}

} // namespace aegis::coordinator::perception::analytics

