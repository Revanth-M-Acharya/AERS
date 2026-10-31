/**
 * @file PerceptionDigitalTwinBridge.hpp
 * @brief Digital Twin visualization and state snapshot bridge for IRSA perception subsystem.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadSituationAwarenessEngine.hpp"
#include "coordinator/perception/CoordinatorIntegration.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace aegis::coordinator::perception::digital_twin {

struct CameraMarker {
    std::string cameraId;
    double latitude{0.0};
    double longitude{0.0};
    std::string status;
    std::string intersectionId;
    std::vector<std::string> coveredSegments;
    double healthScore{1.0};
    double currentFps{30.0};
    uint64_t droppedFrames{0};
    std::string streamStatus{"HEALTHY"};
};

struct SimulatedFeedState {
    std::string cameraId;
    double fps{30.0};
    double bitrateKbps{2500.0};
    uint64_t totalFrames{0};
    uint64_t droppedFrames{0};
    bool motionDetected{false};
};

struct IncidentOverlay {
    std::string eventId;
    std::string incidentId;
    std::string type;
    std::string roadSegment;
    double latitude{0.0};
    double longitude{0.0};
    double confidence{0.0};
    std::string severity;
    std::string status;
    std::string lifecycleState;
    bool isBlocking{false};
    std::string description;

    // Aging indicators
    uint64_t creationTimeMonoMs{0};
    uint64_t lastUpdatedMonoMs{0};
    double durationSeconds{0.0};
    std::string agingStatus{"FRESH"}; // "FRESH", "AGING", "STALE"

    // Multi-sensor confidence visualization breakdown
    double aiConfidence{0.0};
    double cameraHealthFactor{1.0};
    double temporalDecayFactor{1.0};
    double predictionConfidence{1.0};
};

struct RoadColorState {
    std::string segmentId;
    std::string color; // "GREEN", "YELLOW", "ORANGE", "RED"
    double densityScore{0.0};
    double delaySeconds{0.0};
    uint32_t recommendedEmergencyLane{1};

    // Suitability overlay
    double suitabilityScore{100.0};
    std::string suitabilityRating{"OPTIMAL"};
    bool isRecommendedForCorridor{true};
    std::vector<std::string> penaltyReasons;
};

struct IncidentTimelineEntry {
    std::string incidentId;
    std::string eventId;
    std::string roadSegment;
    std::string type;
    std::string lifecycleState;
    std::string severity;
    double confidence{0.0};
    uint64_t timestampMonoMs{0};
};

struct DigitalTwinPerceptionSnapshot {
    uint64_t timestampMonoMs{0};
    std::vector<CameraMarker> cameraLocations;
    std::vector<std::string> activeCorridorCameras;
    std::vector<SimulatedFeedState> simulatedLiveFeeds;
    std::vector<IncidentOverlay> incidentOverlays;
    std::vector<RoadColorState> roadStatusColours;
    std::vector<IncidentTimelineEntry> incidentTimeline;
    bool corridorReplanningAdvised{false};
    std::string activeEmergencyId;
    std::vector<std::string> updatedAmbulanceRoute;
    double updatedEtaSeconds{0.0};
    std::unordered_map<std::string, std::string> signalStates; // intersectionId -> state

    std::string ToJson() const;
    std::string RenderAsciiDashboard() const;
};

class PerceptionDigitalTwinBridge {
public:
    PerceptionDigitalTwinBridge(std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine,
                                std::shared_ptr<CoordinatorIntegration> coordinatorIntegration);
    ~PerceptionDigitalTwinBridge() = default;

    /**
     * @brief Update tracking context of an active emergency corridor.
     */
    void SetActiveAmbulanceContext(const std::string& emergencyId,
                                  const std::vector<std::string>& route,
                                  double etaSeconds);

    /**
     * @brief Update simulated or real intersection signal state.
     */
    void UpdateSignalState(const std::string& intersectionId, const std::string& state);

    /**
     * @brief Record an event into the incident timeline.
     */
    void RecordTimelineEvent(const IncidentTimelineEntry& entry);

    /**
     * @brief Retrieve historical incident timeline entries.
     */
    std::vector<IncidentTimelineEntry> GetIncidentTimeline() const;

    /**
     * @brief Record a snapshot into the historical replay ring buffer.
     */
    void RecordSnapshotForReplay(const DigitalTwinPerceptionSnapshot& snapshot);

    /**
     * @brief Retrieve historical snapshots for replay.
     */
    std::vector<DigitalTwinPerceptionSnapshot> GetReplayHistory(size_t maxCount = 10) const;

    /**
     * @brief Generate comprehensive Digital Twin telemetry snapshot.
     */
    DigitalTwinPerceptionSnapshot GenerateSnapshot() const;

    void Reset();

private:
    std::shared_ptr<RoadSituationAwarenessEngine> irsaEngine_;
    std::shared_ptr<CoordinatorIntegration> coordinatorIntegration_;

    mutable std::mutex mutex_;
    std::string activeEmergencyId_;
    std::vector<std::string> activeRoute_;
    double activeEtaSeconds_{0.0};
    std::unordered_map<std::string, std::string> signalStates_;
    std::vector<IncidentTimelineEntry> incidentTimeline_;
    std::vector<DigitalTwinPerceptionSnapshot> replayHistory_;
};

} // namespace aegis::coordinator::perception::digital_twin
