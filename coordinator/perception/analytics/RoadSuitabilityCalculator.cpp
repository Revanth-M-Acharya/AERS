/**
 * @file RoadSuitabilityCalculator.cpp
 * @brief Implementation of RoadSuitabilityCalculator scoring algorithm.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/RoadSuitabilityCalculator.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception::analytics {

RoadSuitabilityResult RoadSuitabilityCalculator::CalculateSuitability(
    const std::string& segmentId,
    const SegmentDensityMetrics& density,
    const QueueMetrics& queue,
    const SegmentLanesOverview& lanes,
    const RoadHealthMetrics& health,
    const std::vector<RoadConditionEvent>& activeEvents,
    const PredictedCondition& prediction,
    const CongestionMetrics& congestion,
    const SegmentBlockageReport& blockage) {

    RoadSuitabilityResult res;
    res.segmentId = segmentId;
    res.computedMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    // Invariant: Complete physical blockage drops score immediately to 0
    if (blockage.blockageType == BlockageType::COMPLETE_BLOCKAGE || lanes.isCompletelyBlocked) {
        res.suitabilityScore = 0.0;
        res.rating = "IMPASSABLE";
        res.isPassable = false;
        res.isRecommendedForCorridor = false;
        res.penaltyReasons.push_back("Physical complete road blockage");

        std::lock_guard<std::mutex> lock(mutex_);
        cache_[segmentId] = res;
        return res;
    }

    double score = 100.0;

    // 1. Traffic Density (up to -20 pts)
    double densityPenalty = density.normalizedDensityScore * 20.0;
    if (densityPenalty > 3.0) {
        score -= densityPenalty;
        res.penaltyReasons.push_back("Density penalty: -" + std::to_string(static_cast<int>(densityPenalty)) + "pts");
    }

    // 2. Queue Length & Spillback (up to -15 pts)
    double queueFrac = std::clamp(queue.queueLengthMeters / 200.0, 0.0, 1.0);
    double queuePenalty = queueFrac * 10.0 + (queue.isSpillbackRisk ? 5.0 : 0.0);
    if (queuePenalty > 2.0) {
        score -= queuePenalty;
        res.penaltyReasons.push_back("Queue length penalty: -" + std::to_string(static_cast<int>(queuePenalty)) + "pts");
    }

    // 3. Lane Occupancy (up to -10 pts)
    if (!lanes.lanes.empty()) {
        double maxOcc = 0.0;
        for (const auto& l : lanes.lanes) {
            maxOcc = std::max(maxOcc, l.occupancyRatio);
        }
        double lanePenalty = maxOcc * 10.0;
        if (lanePenalty > 2.0) {
            score -= lanePenalty;
            res.penaltyReasons.push_back("Lane occupancy: -" + std::to_string(static_cast<int>(lanePenalty)) + "pts");
        }
    }

    // 4. Road Physical Health (up to -15 pts)
    double healthPenalty = (1.0 - health.healthScore) * 15.0;
    if (healthPenalty > 2.0) {
        score -= healthPenalty;
        res.penaltyReasons.push_back("Road health defect: -" + std::to_string(static_cast<int>(healthPenalty)) + "pts");
    }

    // 5. Incident Severity & Visibility (up to -25 pts)
    double incidentPenalty = 0.0;
    double minVisibility = 1.0;
    for (const auto& ev : activeEvents) {
        if (ev.lifecycleState == IncidentLifecycleState::CLEARED ||
            ev.lifecycleState == IncidentLifecycleState::ARCHIVED) continue;

        switch (ev.severity) {
            case EventSeverity::CRITICAL: incidentPenalty = std::max(incidentPenalty, 25.0); break;
            case EventSeverity::HIGH:     incidentPenalty = std::max(incidentPenalty, 18.0); break;
            case EventSeverity::MEDIUM:   incidentPenalty = std::max(incidentPenalty, 10.0); break;
            case EventSeverity::LOW:      incidentPenalty = std::max(incidentPenalty, 5.0);  break;
            case EventSeverity::INFO:     incidentPenalty = std::max(incidentPenalty, 2.0);  break;
        }
        minVisibility = std::min(minVisibility, ev.visibilityScore);
    }
    if (incidentPenalty > 0.0) {
        score -= incidentPenalty;
        res.penaltyReasons.push_back("Incident severity: -" + std::to_string(static_cast<int>(incidentPenalty)) + "pts");
    }

    // Visibility deduction (up to -10 pts)
    if (minVisibility < 0.90) {
        double visPenalty = (1.0 - minVisibility) * 10.0;
        score -= visPenalty;
        res.penaltyReasons.push_back("Low visibility: -" + std::to_string(static_cast<int>(visPenalty)) + "pts");
    }

    // 6. Predicted Congestion (up to -15 pts)
    if (prediction.predictedDensityScore > 0.50 || prediction.predictedSpeedKmh < 30.0) {
        double predPenalty = (prediction.predictedDensityScore * 10.0) +
                             ((50.0 - std::min(50.0, prediction.predictedSpeedKmh)) / 50.0) * 5.0;
        score -= predPenalty;
        res.penaltyReasons.push_back("Predicted congestion: -" + std::to_string(static_cast<int>(predPenalty)) + "pts");
    }

    // 7. Average Speed Deficit (up to -15 pts)
    double speedPenalty = congestion.speedDeficitRatio * 15.0;
    if (speedPenalty > 2.0) {
        score -= speedPenalty;
        res.penaltyReasons.push_back("Speed deficit: -" + std::to_string(static_cast<int>(speedPenalty)) + "pts");
    }

    // 8. Blockage Level (Partial Blockage: -20 pts)
    if (blockage.blockageType == BlockageType::PARTIAL_BLOCKAGE) {
        score -= 20.0;
        res.penaltyReasons.push_back("Partial lane blockage: -20pts");
    }

    score = std::clamp(score, 0.0, 100.0);
    res.suitabilityScore = score;
    res.isPassable = (score > 15.0);
    res.isRecommendedForCorridor = (score >= 55.0);

    if (score >= 85.0)      res.rating = "OPTIMAL";
    else if (score >= 70.0) res.rating = "GOOD";
    else if (score >= 45.0) res.rating = "DEGRADED";
    else if (score >= 20.0) res.rating = "POOR";
    else                    res.rating = "IMPASSABLE";

    std::lock_guard<std::mutex> lock(mutex_);
    cache_[segmentId] = res;
    return res;
}

RoadSuitabilityResult RoadSuitabilityCalculator::GetCachedSuitability(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_.find(segmentId);
    if (it != cache_.end()) return it->second;

    RoadSuitabilityResult def;
    def.segmentId = segmentId;
    return def;
}

void RoadSuitabilityCalculator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
}

} // namespace aegis::coordinator::perception::analytics

