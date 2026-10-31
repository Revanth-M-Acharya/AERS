/**
 * @file ContinuousETAEngine.cpp
 * @brief Implementation of ContinuousETAEngine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/eta/ContinuousETAEngine.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cmath>
#include <algorithm>

namespace aegis::coordinator::intelligence {

ContinuousETAEngine::ContinuousETAEngine(double emaAlpha)
    : emaAlpha_(std::clamp(emaAlpha, 0.05, 0.95)) {}

void ContinuousETAEngine::UpdateHistoricalEdgeTraversal(const std::string& segmentId, double observedTimeSec) {
    if (observedTimeSec <= 0.0) return;
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = historicalTraversalTimes_.find(segmentId);
    if (it == historicalTraversalTimes_.end()) {
        historicalTraversalTimes_[segmentId] = observedTimeSec;
    } else {
        it->second = (emaAlpha_ * observedTimeSec) + ((1.0 - emaAlpha_) * it->second);
    }
}

double ContinuousETAEngine::GetHistoricalTraversalTime(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = historicalTraversalTimes_.find(segmentId);
    if (it != historicalTraversalTimes_.end()) {
        return it->second;
    }
    return 0.0;
}

ETAReport ContinuousETAEngine::EstimateETA(const std::string& ambulanceId,
                                          const std::vector<std::string>& remainingSegmentIds,
                                          double currentSpeedKmh,
                                          const RoadGraph& graph) {
    ETAReport report{};
    if (remainingSegmentIds.empty()) {
        report.etaSeconds = 0.0;
        report.remainingDistanceMeters = 0.0;
        report.arrivalConfidencePercent = 100.0;
        report.predictedArrivalTimeUnixMs = common::TimeUtils::GetCurrentTimestampMs();
        report.currentAverageSpeedKmh = currentSpeedKmh;
        return report;
    }

    double totalDist = 0.0;
    double totalTime = 0.0;
    bool isFirst = true;

    for (const auto& segId : remainingSegmentIds) {
        const RoadSegment* seg = graph.GetSegment(segId);
        if (!seg) continue;

        totalDist += seg->distanceMeters;

        double segBaseTime = seg->GetEffectiveTravelTimeSec();
        double histTime = GetHistoricalTraversalTime(segId);

        double segTime = segBaseTime;
        if (histTime > 0.0) {
            segTime = (0.5 * histTime) + (0.5 * segBaseTime);
        }

        // For the immediate current segment, incorporate instantaneous speed if moving
        if (isFirst && currentSpeedKmh > 5.0) {
            double curSpeedMs = currentSpeedKmh * (1000.0 / 3600.0);
            double kinematicTime = seg->distanceMeters / curSpeedMs;
            segTime = (0.7 * kinematicTime) + (0.3 * segTime);
            isFirst = false;
        }

        totalTime += segTime;
    }

    report.remainingDistanceMeters = totalDist;
    report.etaSeconds = totalTime;

    if (totalTime > 0.0) {
        report.currentAverageSpeedKmh = (totalDist / totalTime) * 3.6;
    } else {
        report.currentAverageSpeedKmh = currentSpeedKmh;
    }

    // Confidence estimation
    double confidence = 95.0;
    // Degrade confidence slightly with extreme distance
    if (totalDist > 3000.0) {
        confidence -= (totalDist - 3000.0) / 1000.0 * 2.5;
    }
    // Degrade confidence if vehicle is stationary
    if (currentSpeedKmh < 2.0 && remainingSegmentIds.size() > 1) {
        confidence -= 10.0;
    }
    report.arrivalConfidencePercent = std::clamp(confidence, 15.0, 99.0);

    uint64_t nowMs = common::TimeUtils::GetCurrentTimestampMs();
    report.predictedArrivalTimeUnixMs = nowMs + static_cast<uint64_t>(totalTime * 1000.0);

    CoordinatorLogger::Instance().Info("ETA", "Ambulance " + ambulanceId + " ETA: " +
                                       std::to_string(report.etaSeconds) + "s, Dist: " +
                                       std::to_string(report.remainingDistanceMeters) + "m, Conf: " +
                                       std::to_string(report.arrivalConfidencePercent) + "%");
    return report;
}

} // namespace aegis::coordinator::intelligence

