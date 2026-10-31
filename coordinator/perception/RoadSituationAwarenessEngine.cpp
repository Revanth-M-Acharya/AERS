/**
 * @file RoadSituationAwarenessEngine.cpp
 * @brief Implementation of RoadSituationAwarenessEngine master facade.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/RoadSituationAwarenessEngine.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::perception {

RoadSituationAwarenessEngine::RoadSituationAwarenessEngine(std::shared_ptr<orchestration::IEventBus> eventBus)
    : eventBus_(std::move(eventBus)) {

    cameraRegistry_ = std::make_shared<CameraRegistry>();
    feedManager_ = std::make_shared<CameraFeedManager>(cameraRegistry_);
    videoStreamManager_ = std::make_shared<VideoStreamManager>();
    confidenceEngine_ = std::make_shared<RoadConfidenceEngine>(cameraRegistry_);
    eventPublisher_ = std::make_shared<RoadEventPublisher>(eventBus_);

    densityAnalyzer_ = std::make_shared<analytics::TrafficDensityAnalyzer>();
    queueEstimator_ = std::make_shared<analytics::QueueLengthEstimator>();
    laneEstimator_ = std::make_shared<analytics::LaneOccupancyEstimator>();
    healthAnalyzer_ = std::make_shared<analytics::RoadHealthAnalyzer>();
    incidentEngine_ = std::make_shared<analytics::IncidentDetectionEngine>();
    emergencyVehicleEngine_ = std::make_shared<analytics::EmergencyVehicleDetectionEngine>();
    blockageAnalyzer_ = std::make_shared<analytics::RoadBlockageAnalyzer>();
    congestionAnalyzer_ = std::make_shared<analytics::CongestionAnalyzer>();
    predictionEngine_ = std::make_shared<analytics::TrafficPredictionEngine>();
    cameraHealthMonitor_ = std::make_shared<analytics::CameraHealthMonitor>(cameraRegistry_);
    suitabilityCalculator_ = std::make_shared<analytics::RoadSuitabilityCalculator>();
    historicalAnalytics_ = std::make_shared<analytics::HistoricalTrafficAnalytics>();
}

RoadConditionEvent RoadSituationAwarenessEngine::IngestRawDetection(const RoadConditionEvent& rawEvent) {
    // 1. Evaluate confidence & multi-sensor fusion
    RoadConditionEvent validated = confidenceEngine_->EvaluateConfidence(rawEvent);

    // 2. Route to specialized analytics
    if (validated.IsRoadSurfaceHazard()) {
        healthAnalyzer_->RecordHazard(validated.roadSegment, validated.type, validated.severity);
    }

    if (validated.IsEmergencyVehicle()) {
        analytics::DetectedEmergencyVehicle ev;
        ev.vehicleType = validated.type;
        ev.roadSegment = validated.roadSegment;
        emergencyVehicleEngine_->ProcessVehicleDetection(ev);
    }

    // 3. Store active event & historical tracking
    if (validated.verificationStatus != VerificationStatus::FALSE_POSITIVE &&
        validated.verificationStatus != VerificationStatus::RESOLVED) {

        double durSec = static_cast<double>(validated.GetDurationMs()) / 1000.0;
        historicalAnalytics_->RecordIncident(validated.roadSegment, validated.type, durSec, validated.lastUpdatedMonoMs);

        std::lock_guard<std::mutex> lock(stateMutex_);
        auto& list = activeEventsBySegment_[validated.roadSegment];

        // Replace or add
        auto it = std::find_if(list.begin(), list.end(), [&](const RoadConditionEvent& e) {
            return e.eventId == validated.eventId || (e.roadSegment == validated.roadSegment && e.type == validated.type);
        });

        if (it != list.end()) {
            *it = validated;
        } else {
            list.push_back(validated);
        }
    }

    // 4. Update blockage analysis
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto it = activeEventsBySegment_.find(validated.roadSegment);
        if (it != activeEventsBySegment_.end()) {
            blockageAnalyzer_->EvaluateBlockage(validated.roadSegment, 2, it->second);
        }
    }

    // 5. Recompute composite road state
    ReevaluateSegmentState(validated.roadSegment);

    // 6. Publish if passes confidence
    if (validated.confidence >= confidenceEngine_->GetConfidenceThreshold()) {
        eventPublisher_->PublishRoadEvent(validated);
    }

    return validated;
}

void RoadSituationAwarenessEngine::UpdateSegmentTraffic(
    const std::string& segmentId,
    uint32_t vehicleCount,
    double avgSpeedKmh,
    uint32_t laneCount,
    double segmentLengthM) {

    densityAnalyzer_->UpdateDensity(segmentId, vehicleCount, laneCount, segmentLengthM);
    congestionAnalyzer_->UpdateSpeed(segmentId, avgSpeedKmh, 50.0, segmentLengthM);
    historicalAnalytics_->RecordSpeedSample(segmentId, avgSpeedKmh, common::TimeUtils::GetMonotonicTimeMs());

    ReevaluateSegmentState(segmentId);
}

void RoadSituationAwarenessEngine::UpdateApproachQueue(
    const std::string& approachId,
    uint32_t stoppedVehicles,
    double segmentLengthM) {

    queueEstimator_->UpdateQueue(approachId, stoppedVehicles, segmentLengthM);
}

void RoadSituationAwarenessEngine::ReevaluateSegmentState(const std::string& segmentId) {
    auto densityMetrics = densityAnalyzer_->GetMetrics(segmentId);
    auto congestionMetrics = congestionAnalyzer_->GetMetrics(segmentId);
    auto healthMetrics = healthAnalyzer_->GetHealth(segmentId);
    auto blockageReport = blockageAnalyzer_->GetBlockageReport(segmentId);
    auto queueMetrics = queueEstimator_->GetQueue(segmentId);
    auto lanesOverview = laneEstimator_->GetOverview(segmentId);
    auto predictedCond = predictionEngine_->PredictConditionAtEta(segmentId, 180.0, densityMetrics, congestionMetrics);
    uint32_t recLane = laneEstimator_->GetRecommendedLane(segmentId);

    std::vector<RoadConditionEvent> currentActive;
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        auto evIt = activeEventsBySegment_.find(segmentId);
        if (evIt != activeEventsBySegment_.end()) {
            currentActive = evIt->second;
        }
    }

    auto suitability = suitabilityCalculator_->CalculateSuitability(
        segmentId,
        densityMetrics,
        queueMetrics,
        lanesOverview,
        healthMetrics,
        currentActive,
        predictedCond,
        congestionMetrics,
        blockageReport);

    historicalAnalytics_->RecordAvailabilitySample(segmentId, suitability.isPassable);

    RoadStatusColor color = RoadStatusColor::GREEN;

    if (blockageReport.blockageType == analytics::BlockageType::COMPLETE_BLOCKAGE ||
        densityMetrics.los == analytics::LevelOfService::LOS_F ||
        !healthMetrics.isSafeForEmergencyTransit ||
        !suitability.isPassable) {
        color = RoadStatusColor::RED;
    } else if (blockageReport.blockageType == analytics::BlockageType::PARTIAL_BLOCKAGE ||
               densityMetrics.los == analytics::LevelOfService::LOS_E ||
               congestionMetrics.trend == analytics::CongestionTrend::GRIDLOCK ||
               !suitability.isRecommendedForCorridor) {
        color = RoadStatusColor::ORANGE;
    } else if (densityMetrics.los == analytics::LevelOfService::LOS_C ||
               densityMetrics.los == analytics::LevelOfService::LOS_D ||
               healthMetrics.healthScore < 0.70 ||
               congestionMetrics.estimatedTransitDelaySec > 30.0 ||
               suitability.suitabilityScore < 70.0) {
        color = RoadStatusColor::YELLOW;
    } else {
        color = RoadStatusColor::GREEN;
    }

    std::lock_guard<std::mutex> lock(stateMutex_);
    SegmentPerceptionState state;
    state.segmentId = segmentId;
    state.statusColor = color;
    state.densityScore = densityMetrics.normalizedDensityScore;
    state.los = densityMetrics.los;
    state.currentAverageSpeedKmh = congestionMetrics.currentAverageSpeedKmh;
    state.estimatedTransitDelaySec = congestionMetrics.estimatedTransitDelaySec;
    state.healthScore = healthMetrics.healthScore;
    state.blockage = blockageReport.blockageType;
    state.recommendedEmergencyLane = recLane;
    state.suitabilityScore = suitability.suitabilityScore;
    state.suitabilityRating = suitability.rating;
    state.isRecommendedForCorridor = suitability.isRecommendedForCorridor;
    state.suitabilityPenaltyReasons = suitability.penaltyReasons;
    state.lastUpdatedMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    state.activeEvents = currentActive;

    segmentStates_[segmentId] = state;
}

SegmentPerceptionState RoadSituationAwarenessEngine::GetSegmentState(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    auto it = segmentStates_.find(segmentId);
    if (it != segmentStates_.end()) return it->second;

    SegmentPerceptionState def;
    def.segmentId = segmentId;
    return def;
}

RoadStatusColor RoadSituationAwarenessEngine::GetRoadStatusColor(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    auto it = segmentStates_.find(segmentId);
    if (it != segmentStates_.end()) return it->second.statusColor;
    return RoadStatusColor::GREEN;
}

std::vector<RoadConditionEvent> RoadSituationAwarenessEngine::GetAllActiveEvents() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    std::vector<RoadConditionEvent> all;
    for (const auto& [_, list] : activeEventsBySegment_) {
        for (const auto& ev : list) {
            all.push_back(ev);
        }
    }
    return all;
}

std::vector<RoadConditionEvent> RoadSituationAwarenessEngine::GetEventsForSegment(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    auto it = activeEventsBySegment_.find(segmentId);
    if (it != activeEventsBySegment_.end()) return it->second;
    return {};
}

void RoadSituationAwarenessEngine::Reset() {
    cameraRegistry_->Clear();
    feedManager_->Reset();
    videoStreamManager_->Reset();
    confidenceEngine_->Reset();
    eventPublisher_->Reset();

    densityAnalyzer_->Reset();
    queueEstimator_->Reset();
    laneEstimator_->Reset();
    healthAnalyzer_->Reset();
    incidentEngine_->Reset();
    emergencyVehicleEngine_->Reset();
    blockageAnalyzer_->Reset();
    congestionAnalyzer_->Reset();
    predictionEngine_->Reset();
    cameraHealthMonitor_->Reset();
    suitabilityCalculator_->Reset();
    historicalAnalytics_->Reset();

    std::lock_guard<std::mutex> lock(stateMutex_);
    segmentStates_.clear();
    activeEventsBySegment_.clear();
}

} // namespace aegis::coordinator::perception

