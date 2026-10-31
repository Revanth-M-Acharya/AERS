/**
 * @file RoadConditionEvent.hpp
 * @brief Definition of road condition events and perception data schemas for SMART AEGIS IRSA.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::perception {

/**
 * @brief Categorized road events detected by external AI vision and fused by IRSA.
 */
enum class RoadEventType {
    ACCIDENT,
    TRAFFIC_CONGESTION,
    STOPPED_VEHICLE,
    BROKEN_DOWN_TRUCK,
    FLOODING,
    ROAD_CONSTRUCTION,
    ROAD_CLOSED,
    POTHOLE,
    FALLEN_TREE,
    SMOKE,
    FOG,
    WRONG_WAY_VEHICLE,
    PEDESTRIAN_CROWD,
    ILLEGAL_PARKING,
    VIP_CONVOY,
    FIRE_TRUCK,
    POLICE_CONVOY,
    SECOND_AMBULANCE,
    UNKNOWN
};

std::string RoadEventTypeToString(RoadEventType type);
RoadEventType StringToRoadEventType(const std::string& str);

/**
 * @brief Severity classification of road condition events.
 */
enum class EventSeverity {
    INFO = 1,
    LOW = 2,
    MEDIUM = 3,
    HIGH = 4,
    CRITICAL = 5
};

std::string EventSeverityToString(EventSeverity severity);
EventSeverity StringToEventSeverity(const std::string& str);

/**
 * @brief Verification lifecycle status of an AI-generated event.
 */
enum class VerificationStatus {
    UNVERIFIED,
    CONFIRMED,
    MULTI_CAMERA_VERIFIED,
    FALSE_POSITIVE,
    RESOLVED
};

std::string VerificationStatusToString(VerificationStatus status);
VerificationStatus StringToVerificationStatus(const std::string& str);

/**
 * @brief Formal 6-stage lifecycle for all detected road incidents.
 */
enum class IncidentLifecycleState {
    DETECTED,
    CONFIRMED,
    ACTIVE,
    RESOLVING,
    CLEARED,
    ARCHIVED
};

std::string IncidentLifecycleStateToString(IncidentLifecycleState state);
IncidentLifecycleState StringToIncidentLifecycleState(const std::string& str);

/**
 * @brief Road segment visual status color for Digital Twin and operator displays.
 */
enum class RoadStatusColor {
    GREEN,    // Normal traffic flow
    YELLOW,   // Moderate congestion / Advisory warning
    ORANGE,   // Heavy congestion / Single-lane obstruction
    RED       // Impassable / Complete road blockage / Severe hazard
};

std::string RoadStatusColorToString(RoadStatusColor color);

/**
 * @brief Complete structured representation of a road condition perception event.
 */
struct RoadConditionEvent {
    std::string eventId;
    std::string incidentId; // Unique incident identifier (e.g. INC-001001)
    RoadEventType type{RoadEventType::UNKNOWN};
    std::string roadSegment;
    std::string cameraId;
    uint64_t timestampMonoMs{0};
    double confidence{0.0};              // 0.0 to 1.0
    EventSeverity severity{EventSeverity::LOW};
    std::vector<uint32_t> affectedLanes; // 1-based lane indices
    double estimatedDelaySeconds{0.0};
    VerificationStatus verificationStatus{VerificationStatus::UNVERIFIED};
    IncidentLifecycleState lifecycleState{IncidentLifecycleState::DETECTED};

    // Incident lifecycle tracking metadata
    uint64_t creationTimeMonoMs{0};
    uint64_t lastUpdatedMonoMs{0};
    uint64_t resolutionTimeMonoMs{0};
    uint64_t durationMonoMs{0};
    std::vector<EventSeverity> severityHistory;
    std::vector<double> confidenceHistory;

    // Metadata and spatial context
    std::string description;
    double latitude{0.0};
    double longitude{0.0};
    bool isBlocking{false};              // True if all lanes are closed/impassable
    double visibilityScore{1.0};         // 0.0 (zero visibility) to 1.0 (clear)

    // Diagnostic/Fusion telemetry
    uint32_t confirmingCameraCount{1};
    uint32_t consecutiveFrameDetections{1};

    RoadConditionEvent();
    RoadConditionEvent(const RoadConditionEvent&) = default;
    RoadConditionEvent& operator=(const RoadConditionEvent&) = default;
    RoadConditionEvent(RoadConditionEvent&&) = default;
    RoadConditionEvent& operator=(RoadConditionEvent&&) = default;

    bool IsEmergencyVehicle() const;
    bool IsWeatherHazard() const;
    bool IsRoadSurfaceHazard() const;

    // Lifecycle transitions
    bool AdvanceLifecycle(IncidentLifecycleState nextState, uint64_t nowMonoMs = 0);
    void RecordSeverityUpdate(EventSeverity newSeverity, uint64_t nowMonoMs = 0);
    void RecordConfidenceUpdate(double newConfidence, uint64_t nowMonoMs = 0);
    uint64_t GetDurationMs(uint64_t nowMonoMs = 0) const;

    std::string ToJson() const;
    static RoadConditionEvent FromJson(const std::string& json);
    std::string ToString() const;
};

} // namespace aegis::coordinator::perception
