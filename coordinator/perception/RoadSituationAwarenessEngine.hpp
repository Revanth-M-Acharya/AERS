/**
 * @file RoadSituationAwarenessEngine.hpp
 * @brief Master facade for Intelligent Road Situation Awareness (IRSA) perception subsystem.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/CameraRegistry.hpp"
#include "coordinator/perception/CameraFeedManager.hpp"
#include "coordinator/perception/VideoStreamManager.hpp"
#include "coordinator/perception/RoadConditionEvent.hpp"
#include "coordinator/perception/RoadEventPublisher.hpp"
#include "coordinator/perception/RoadConfidenceEngine.hpp"

#include "coordinator/perception/analytics/TrafficDensityAnalyzer.hpp"
#include "coordinator/perception/analytics/QueueLengthEstimator.hpp"
#include "coordinator/perception/analytics/LaneOccupancyEstimator.hpp"
#include "coordinator/perception/analytics/RoadHealthAnalyzer.hpp"
#include "coordinator/perception/analytics/IncidentDetectionEngine.hpp"
#include "coordinator/perception/analytics/EmergencyVehicleDetectionEngine.hpp"
#include "coordinator/perception/analytics/RoadBlockageAnalyzer.hpp"
#include "coordinator/perception/analytics/CongestionAnalyzer.hpp"
#include "coordinator/perception/analytics/TrafficPredictionEngine.hpp"
#include "coordinator/perception/analytics/CameraHealthMonitor.hpp"
#include "coordinator/perception/analytics/RoadSuitabilityCalculator.hpp"
#include "coordinator/perception/analytics/HistoricalTrafficAnalytics.hpp"

#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aegis::coordinator::perception {

struct SegmentPerceptionState {
    std::string segmentId;
    RoadStatusColor statusColor{RoadStatusColor::GREEN};
    double densityScore{0.0};
    analytics::LevelOfService los{analytics::LevelOfService::LOS_A};
    double currentAverageSpeedKmh{50.0};
    double estimatedTransitDelaySec{0.0};
    double healthScore{1.0};
    analytics::BlockageType blockage{analytics::BlockageType::NONE};
    uint32_t recommendedEmergencyLane{1};
    double suitabilityScore{100.0};
    std::string suitabilityRating{"OPTIMAL"};
    bool isRecommendedForCorridor{true};
    std::vector<std::string> suitabilityPenaltyReasons;
    std::vector<RoadConditionEvent> activeEvents;
    uint64_t lastUpdatedMonoMs{0};
};

class RoadSituationAwarenessEngine {
public:
    explicit RoadSituationAwarenessEngine(std::shared_ptr<orchestration::IEventBus> eventBus = nullptr);
    ~RoadSituationAwarenessEngine() = default;

    /**
     * @brief Ingest a raw detection observation from an external AI vision camera.
     */
    RoadConditionEvent IngestRawDetection(const RoadConditionEvent& rawEvent);

    /**
     * @brief Update road traffic telemetry from physical or simulated camera analysis.
     */
    void UpdateSegmentTraffic(const std::string& segmentId,
                              uint32_t vehicleCount,
                              double avgSpeedKmh,
                              uint32_t laneCount = 2,
                              double segmentLengthM = 500.0);

    /**
     * @brief Update intersection approach queue.
     */
    void UpdateApproachQueue(const std::string& approachId,
                             uint32_t stoppedVehicles,
                             double segmentLengthM = 300.0);

    // Subsystem Component Accessors
    std::shared_ptr<CameraRegistry> GetCameraRegistry() const { return cameraRegistry_; }
    std::shared_ptr<CameraFeedManager> GetCameraFeedManager() const { return feedManager_; }
    std::shared_ptr<VideoStreamManager> GetVideoStreamManager() const { return videoStreamManager_; }
    std::shared_ptr<RoadConfidenceEngine> GetConfidenceEngine() const { return confidenceEngine_; }
    std::shared_ptr<RoadEventPublisher> GetEventPublisher() const { return eventPublisher_; }

    // Analytics Accessors
    std::shared_ptr<analytics::TrafficDensityAnalyzer> GetDensityAnalyzer() const { return densityAnalyzer_; }
    std::shared_ptr<analytics::QueueLengthEstimator> GetQueueEstimator() const { return queueEstimator_; }
    std::shared_ptr<analytics::LaneOccupancyEstimator> GetLaneEstimator() const { return laneEstimator_; }
    std::shared_ptr<analytics::RoadHealthAnalyzer> GetHealthAnalyzer() const { return healthAnalyzer_; }
    std::shared_ptr<analytics::IncidentDetectionEngine> GetIncidentEngine() const { return incidentEngine_; }
    std::shared_ptr<analytics::EmergencyVehicleDetectionEngine> GetEmergencyVehicleEngine() const { return emergencyVehicleEngine_; }
    std::shared_ptr<analytics::RoadBlockageAnalyzer> GetBlockageAnalyzer() const { return blockageAnalyzer_; }
    std::shared_ptr<analytics::CongestionAnalyzer> GetCongestionAnalyzer() const { return congestionAnalyzer_; }
    std::shared_ptr<analytics::TrafficPredictionEngine> GetPredictionEngine() const { return predictionEngine_; }
    std::shared_ptr<analytics::CameraHealthMonitor> GetCameraHealthMonitor() const { return cameraHealthMonitor_; }
    std::shared_ptr<analytics::RoadSuitabilityCalculator> GetSuitabilityCalculator() const { return suitabilityCalculator_; }
    std::shared_ptr<analytics::HistoricalTrafficAnalytics> GetHistoricalAnalytics() const { return historicalAnalytics_; }

    // Global Network Queries
    SegmentPerceptionState GetSegmentState(const std::string& segmentId) const;
    RoadStatusColor GetRoadStatusColor(const std::string& segmentId) const;
    std::vector<RoadConditionEvent> GetAllActiveEvents() const;
    std::vector<RoadConditionEvent> GetEventsForSegment(const std::string& segmentId) const;

    void Reset();

private:
    void ReevaluateSegmentState(const std::string& segmentId);

    std::shared_ptr<orchestration::IEventBus> eventBus_;

    // Core perception components
    std::shared_ptr<CameraRegistry> cameraRegistry_;
    std::shared_ptr<CameraFeedManager> feedManager_;
    std::shared_ptr<VideoStreamManager> videoStreamManager_;
    std::shared_ptr<RoadConfidenceEngine> confidenceEngine_;
    std::shared_ptr<RoadEventPublisher> eventPublisher_;

    // Analytics components
    std::shared_ptr<analytics::TrafficDensityAnalyzer> densityAnalyzer_;
    std::shared_ptr<analytics::QueueLengthEstimator> queueEstimator_;
    std::shared_ptr<analytics::LaneOccupancyEstimator> laneEstimator_;
    std::shared_ptr<analytics::RoadHealthAnalyzer> healthAnalyzer_;
    std::shared_ptr<analytics::IncidentDetectionEngine> incidentEngine_;
    std::shared_ptr<analytics::EmergencyVehicleDetectionEngine> emergencyVehicleEngine_;
    std::shared_ptr<analytics::RoadBlockageAnalyzer> blockageAnalyzer_;
    std::shared_ptr<analytics::CongestionAnalyzer> congestionAnalyzer_;
    std::shared_ptr<analytics::TrafficPredictionEngine> predictionEngine_;
    std::shared_ptr<analytics::CameraHealthMonitor> cameraHealthMonitor_;
    std::shared_ptr<analytics::RoadSuitabilityCalculator> suitabilityCalculator_;
    std::shared_ptr<analytics::HistoricalTrafficAnalytics> historicalAnalytics_;

    mutable std::mutex stateMutex_;
    std::unordered_map<std::string, SegmentPerceptionState> segmentStates_;
    std::unordered_map<std::string, std::vector<RoadConditionEvent>> activeEventsBySegment_;
};

} // namespace aegis::coordinator::perception

