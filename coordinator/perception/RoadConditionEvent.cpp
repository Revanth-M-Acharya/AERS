/**
 * @file RoadConditionEvent.cpp
 * @brief Implementation of road condition event conversions, serialization, and incident lifecycle.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/RoadConditionEvent.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::coordinator::perception {

std::string RoadEventTypeToString(RoadEventType type) {
    switch (type) {
        case RoadEventType::ACCIDENT:            return "ACCIDENT";
        case RoadEventType::TRAFFIC_CONGESTION:  return "TRAFFIC_CONGESTION";
        case RoadEventType::STOPPED_VEHICLE:     return "STOPPED_VEHICLE";
        case RoadEventType::BROKEN_DOWN_TRUCK:   return "BROKEN_DOWN_TRUCK";
        case RoadEventType::FLOODING:            return "FLOODING";
        case RoadEventType::ROAD_CONSTRUCTION:   return "ROAD_CONSTRUCTION";
        case RoadEventType::ROAD_CLOSED:         return "ROAD_CLOSED";
        case RoadEventType::POTHOLE:             return "POTHOLE";
        case RoadEventType::FALLEN_TREE:         return "FALLEN_TREE";
        case RoadEventType::SMOKE:               return "SMOKE";
        case RoadEventType::FOG:                 return "FOG";
        case RoadEventType::WRONG_WAY_VEHICLE:   return "WRONG_WAY_VEHICLE";
        case RoadEventType::PEDESTRIAN_CROWD:    return "PEDESTRIAN_CROWD";
        case RoadEventType::ILLEGAL_PARKING:     return "ILLEGAL_PARKING";
        case RoadEventType::VIP_CONVOY:          return "VIP_CONVOY";
        case RoadEventType::FIRE_TRUCK:          return "FIRE_TRUCK";
        case RoadEventType::POLICE_CONVOY:       return "POLICE_CONVOY";
        case RoadEventType::SECOND_AMBULANCE:    return "SECOND_AMBULANCE";
        default:                                 return "UNKNOWN";
    }
}

RoadEventType StringToRoadEventType(const std::string& str) {
    if (str == "ACCIDENT")            return RoadEventType::ACCIDENT;
    if (str == "TRAFFIC_CONGESTION")  return RoadEventType::TRAFFIC_CONGESTION;
    if (str == "STOPPED_VEHICLE")     return RoadEventType::STOPPED_VEHICLE;
    if (str == "BROKEN_DOWN_TRUCK")   return RoadEventType::BROKEN_DOWN_TRUCK;
    if (str == "FLOODING")            return RoadEventType::FLOODING;
    if (str == "ROAD_CONSTRUCTION")   return RoadEventType::ROAD_CONSTRUCTION;
    if (str == "ROAD_CLOSED")         return RoadEventType::ROAD_CLOSED;
    if (str == "POTHOLE")             return RoadEventType::POTHOLE;
    if (str == "FALLEN_TREE")         return RoadEventType::FALLEN_TREE;
    if (str == "SMOKE")               return RoadEventType::SMOKE;
    if (str == "FOG")                 return RoadEventType::FOG;
    if (str == "WRONG_WAY_VEHICLE")   return RoadEventType::WRONG_WAY_VEHICLE;
    if (str == "PEDESTRIAN_CROWD")    return RoadEventType::PEDESTRIAN_CROWD;
    if (str == "ILLEGAL_PARKING")     return RoadEventType::ILLEGAL_PARKING;
    if (str == "VIP_CONVOY")          return RoadEventType::VIP_CONVOY;
    if (str == "FIRE_TRUCK")          return RoadEventType::FIRE_TRUCK;
    if (str == "POLICE_CONVOY")       return RoadEventType::POLICE_CONVOY;
    if (str == "SECOND_AMBULANCE")    return RoadEventType::SECOND_AMBULANCE;
    return RoadEventType::UNKNOWN;
}

std::string EventSeverityToString(EventSeverity severity) {
    switch (severity) {
        case EventSeverity::INFO:     return "INFO";
        case EventSeverity::LOW:      return "LOW";
        case EventSeverity::MEDIUM:   return "MEDIUM";
        case EventSeverity::HIGH:     return "HIGH";
        case EventSeverity::CRITICAL: return "CRITICAL";
        default:                      return "UNKNOWN";
    }
}

EventSeverity StringToEventSeverity(const std::string& str) {
    if (str == "INFO")     return EventSeverity::INFO;
    if (str == "LOW")      return EventSeverity::LOW;
    if (str == "MEDIUM")   return EventSeverity::MEDIUM;
    if (str == "HIGH")     return EventSeverity::HIGH;
    if (str == "CRITICAL") return EventSeverity::CRITICAL;
    return EventSeverity::LOW;
}

std::string VerificationStatusToString(VerificationStatus status) {
    switch (status) {
        case VerificationStatus::UNVERIFIED:            return "UNVERIFIED";
        case VerificationStatus::CONFIRMED:             return "CONFIRMED";
        case VerificationStatus::MULTI_CAMERA_VERIFIED: return "MULTI_CAMERA_VERIFIED";
        case VerificationStatus::FALSE_POSITIVE:        return "FALSE_POSITIVE";
        case VerificationStatus::RESOLVED:              return "RESOLVED";
        default:                                        return "UNKNOWN";
    }
}

VerificationStatus StringToVerificationStatus(const std::string& str) {
    if (str == "UNVERIFIED")            return VerificationStatus::UNVERIFIED;
    if (str == "CONFIRMED")             return VerificationStatus::CONFIRMED;
    if (str == "MULTI_CAMERA_VERIFIED") return VerificationStatus::MULTI_CAMERA_VERIFIED;
    if (str == "FALSE_POSITIVE")        return VerificationStatus::FALSE_POSITIVE;
    if (str == "RESOLVED")              return VerificationStatus::RESOLVED;
    return VerificationStatus::UNVERIFIED;
}

std::string IncidentLifecycleStateToString(IncidentLifecycleState state) {
    switch (state) {
        case IncidentLifecycleState::DETECTED:  return "DETECTED";
        case IncidentLifecycleState::CONFIRMED: return "CONFIRMED";
        case IncidentLifecycleState::ACTIVE:    return "ACTIVE";
        case IncidentLifecycleState::RESOLVING: return "RESOLVING";
        case IncidentLifecycleState::CLEARED:   return "CLEARED";
        case IncidentLifecycleState::ARCHIVED:  return "ARCHIVED";
        default:                                return "UNKNOWN";
    }
}

IncidentLifecycleState StringToIncidentLifecycleState(const std::string& str) {
    if (str == "DETECTED")  return IncidentLifecycleState::DETECTED;
    if (str == "CONFIRMED") return IncidentLifecycleState::CONFIRMED;
    if (str == "ACTIVE")    return IncidentLifecycleState::ACTIVE;
    if (str == "RESOLVING") return IncidentLifecycleState::RESOLVING;
    if (str == "CLEARED")   return IncidentLifecycleState::CLEARED;
    if (str == "ARCHIVED")  return IncidentLifecycleState::ARCHIVED;
    return IncidentLifecycleState::DETECTED;
}

std::string RoadStatusColorToString(RoadStatusColor color) {
    switch (color) {
        case RoadStatusColor::GREEN:  return "GREEN";
        case RoadStatusColor::YELLOW: return "YELLOW";
        case RoadStatusColor::ORANGE: return "ORANGE";
        case RoadStatusColor::RED:    return "RED";
        default:                      return "UNKNOWN";
    }
}

RoadConditionEvent::RoadConditionEvent() {
    creationTimeMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    lastUpdatedMonoMs = creationTimeMonoMs;
    timestampMonoMs = creationTimeMonoMs;
}

bool RoadConditionEvent::IsEmergencyVehicle() const {
    return (type == RoadEventType::VIP_CONVOY ||
            type == RoadEventType::FIRE_TRUCK ||
            type == RoadEventType::POLICE_CONVOY ||
            type == RoadEventType::SECOND_AMBULANCE);
}

bool RoadConditionEvent::IsWeatherHazard() const {
    return (type == RoadEventType::FLOODING ||
            type == RoadEventType::SMOKE ||
            type == RoadEventType::FOG);
}

bool RoadConditionEvent::IsRoadSurfaceHazard() const {
    return (type == RoadEventType::POTHOLE ||
            type == RoadEventType::FALLEN_TREE ||
            type == RoadEventType::ROAD_CONSTRUCTION);
}

bool RoadConditionEvent::AdvanceLifecycle(IncidentLifecycleState nextState, uint64_t nowMonoMs) {
    if (lifecycleState == IncidentLifecycleState::ARCHIVED) {
        return false; // Terminal state cannot transition
    }

    // Strict forward progression validation
    auto stateOrder = [](IncidentLifecycleState s) -> int {
        switch (s) {
            case IncidentLifecycleState::DETECTED: return 1;
            case IncidentLifecycleState::CONFIRMED: return 2;
            case IncidentLifecycleState::ACTIVE: return 3;
            case IncidentLifecycleState::RESOLVING: return 4;
            case IncidentLifecycleState::CLEARED: return 5;
            case IncidentLifecycleState::ARCHIVED: return 6;
        }
        return 0;
    };

    if (stateOrder(nextState) < stateOrder(lifecycleState)) {
        return false; // Cannot transition backward
    }

    if (nowMonoMs == 0) nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    if (creationTimeMonoMs == 0) creationTimeMonoMs = nowMonoMs;

    lastUpdatedMonoMs = nowMonoMs;
    lifecycleState = nextState;

    if (nextState == IncidentLifecycleState::CLEARED || nextState == IncidentLifecycleState::ARCHIVED) {
        if (resolutionTimeMonoMs == 0) resolutionTimeMonoMs = nowMonoMs;
        durationMonoMs = (resolutionTimeMonoMs >= creationTimeMonoMs) ? (resolutionTimeMonoMs - creationTimeMonoMs) : 0;
        verificationStatus = VerificationStatus::RESOLVED;
    }

    return true;
}

void RoadConditionEvent::RecordSeverityUpdate(EventSeverity newSeverity, uint64_t nowMonoMs) {
    if (nowMonoMs == 0) nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    severity = newSeverity;
    lastUpdatedMonoMs = nowMonoMs;
    severityHistory.push_back(newSeverity);
}

void RoadConditionEvent::RecordConfidenceUpdate(double newConfidence, uint64_t nowMonoMs) {
    if (nowMonoMs == 0) nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    confidence = std::clamp(newConfidence, 0.0, 1.0);
    lastUpdatedMonoMs = nowMonoMs;
    confidenceHistory.push_back(confidence);
}

uint64_t RoadConditionEvent::GetDurationMs(uint64_t nowMonoMs) const {
    if (resolutionTimeMonoMs > 0 && resolutionTimeMonoMs >= creationTimeMonoMs) {
        return resolutionTimeMonoMs - creationTimeMonoMs;
    }
    if (nowMonoMs == 0) nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    if (nowMonoMs >= creationTimeMonoMs && creationTimeMonoMs > 0) {
        return nowMonoMs - creationTimeMonoMs;
    }
    return 0;
}

std::string RoadConditionEvent::ToJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"eventId\":\"" << eventId << "\","
       << "\"incidentId\":\"" << (incidentId.empty() ? eventId : incidentId) << "\","
       << "\"type\":\"" << RoadEventTypeToString(type) << "\","
       << "\"roadSegment\":\"" << roadSegment << "\","
       << "\"cameraId\":\"" << cameraId << "\","
       << "\"timestampMonoMs\":" << timestampMonoMs << ","
       << "\"confidence\":" << std::fixed << std::setprecision(2) << confidence << ","
       << "\"severity\":\"" << EventSeverityToString(severity) << "\","
       << "\"lifecycleState\":\"" << IncidentLifecycleStateToString(lifecycleState) << "\","
       << "\"creationTimeMonoMs\":" << creationTimeMonoMs << ","
       << "\"lastUpdatedMonoMs\":" << lastUpdatedMonoMs << ","
       << "\"resolutionTimeMonoMs\":" << resolutionTimeMonoMs << ","
       << "\"durationMonoMs\":" << GetDurationMs() << ","
       << "\"affectedLanes\":[";
    for (size_t i = 0; i < affectedLanes.size(); ++i) {
        ss << affectedLanes[i] << (i + 1 < affectedLanes.size() ? "," : "");
    }
    ss << "],"
       << "\"estimatedDelaySeconds\":" << estimatedDelaySeconds << ","
       << "\"verificationStatus\":\"" << VerificationStatusToString(verificationStatus) << "\","
       << "\"description\":\"" << description << "\","
       << "\"latitude\":" << std::fixed << std::setprecision(6) << latitude << ","
       << "\"longitude\":" << std::fixed << std::setprecision(6) << longitude << ","
       << "\"isBlocking\":" << (isBlocking ? "true" : "false") << ","
       << "\"visibilityScore\":" << std::fixed << std::setprecision(2) << visibilityScore << ","
       << "\"confirmingCameraCount\":" << confirmingCameraCount << ","
       << "\"consecutiveFrameDetections\":" << consecutiveFrameDetections
       << "}";
    return ss.str();
}

static std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":\"";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.length();
    size_t end = json.find("\"", pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

static double ExtractJsonDouble(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return 0.0;
    pos += needle.length();
    char* endPtr = nullptr;
    return std::strtod(json.c_str() + pos, &endPtr);
}

static uint64_t ExtractJsonUint64(const std::string& json, const std::string& key) {
    std::string needle = "\"" + key + "\":";
    size_t pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.length();
    char* endPtr = nullptr;
    return std::strtoull(json.c_str() + pos, &endPtr, 10);
}

RoadConditionEvent RoadConditionEvent::FromJson(const std::string& json) {
    RoadConditionEvent evt;
    evt.eventId = ExtractJsonString(json, "eventId");
    evt.incidentId = ExtractJsonString(json, "incidentId");
    if (evt.incidentId.empty()) evt.incidentId = evt.eventId;

    evt.type = StringToRoadEventType(ExtractJsonString(json, "type"));
    evt.roadSegment = ExtractJsonString(json, "roadSegment");
    evt.cameraId = ExtractJsonString(json, "cameraId");
    evt.timestampMonoMs = ExtractJsonUint64(json, "timestampMonoMs");
    evt.confidence = ExtractJsonDouble(json, "confidence");
    evt.severity = StringToEventSeverity(ExtractJsonString(json, "severity"));
    evt.lifecycleState = StringToIncidentLifecycleState(ExtractJsonString(json, "lifecycleState"));
    evt.creationTimeMonoMs = ExtractJsonUint64(json, "creationTimeMonoMs");
    evt.lastUpdatedMonoMs = ExtractJsonUint64(json, "lastUpdatedMonoMs");
    evt.resolutionTimeMonoMs = ExtractJsonUint64(json, "resolutionTimeMonoMs");
    evt.durationMonoMs = ExtractJsonUint64(json, "durationMonoMs");

    evt.estimatedDelaySeconds = ExtractJsonDouble(json, "estimatedDelaySeconds");
    evt.verificationStatus = StringToVerificationStatus(ExtractJsonString(json, "verificationStatus"));
    evt.description = ExtractJsonString(json, "description");
    evt.latitude = ExtractJsonDouble(json, "latitude");
    evt.longitude = ExtractJsonDouble(json, "longitude");
    evt.isBlocking = (json.find("\"isBlocking\":true") != std::string::npos);
    evt.visibilityScore = ExtractJsonDouble(json, "visibilityScore");
    if (evt.visibilityScore == 0.0 && json.find("\"visibilityScore\":0") == std::string::npos) {
        evt.visibilityScore = 1.0;
    }
    return evt;
}

std::string RoadConditionEvent::ToString() const {
    std::ostringstream ss;
    ss << "[" << (incidentId.empty() ? eventId : incidentId) << "] "
       << RoadEventTypeToString(type)
       << " on " << roadSegment
       << " | State: " << IncidentLifecycleStateToString(lifecycleState)
       << " | Conf: " << static_cast<int>(confidence * 100) << "%"
       << " | Sev: " << EventSeverityToString(severity)
       << " | Delay: " << estimatedDelaySeconds << "s"
       << " | Duration: " << (GetDurationMs() / 1000) << "s"
       << (isBlocking ? " | BLOCKING" : "");
    return ss.str();
}

} // namespace aegis::coordinator::perception
