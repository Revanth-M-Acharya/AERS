/**
 * @file IEventBus.hpp
 * @brief Abstract interface for type-safe publish/subscribe event bus in SMART AEGIS Coordinator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>
#include <functional>

namespace aegis::coordinator::orchestration {

enum class EventType {
    EmergencyCreated,
    AmbulanceRegistered,
    NodeConnected,
    NodeDisconnected,
    HeartbeatReceived,
    HeartbeatTimeout,
    RouteChanged,
    ETAUpdated,
    ExecutionPlanGenerated,
    ExecutionPlanApproved,
    ExecutionStarted,
    ExecutionCompleted,
    EmergencyFinished,
    HospitalUpdated,
    TrafficIncident,
    NodeRecovered,

    // Phase 6 Control Center & Hospital Coordination Events
    EmergencyValidated,
    EmergencyAssigned,
    HospitalAssigned,
    HospitalChanged,
    MissionCompleted,
    MissionArchived,
    ResourceAllocated,
    ResourceReleased,

    // Phase 7 Intelligent Road Situation Awareness (IRSA) Events
    RoadConditionUpdated,
    RoadIncidentDetected,
    CorridorReplanningAdvised,
    CameraStatusChanged,
    TrafficDensityUpdated
};

inline std::string EventTypeToString(EventType type) {
    switch (type) {
        case EventType::EmergencyCreated:       return "EmergencyCreated";
        case EventType::AmbulanceRegistered:    return "AmbulanceRegistered";
        case EventType::NodeConnected:          return "NodeConnected";
        case EventType::NodeDisconnected:       return "NodeDisconnected";
        case EventType::HeartbeatReceived:      return "HeartbeatReceived";
        case EventType::HeartbeatTimeout:       return "HeartbeatTimeout";
        case EventType::RouteChanged:           return "RouteChanged";
        case EventType::ETAUpdated:             return "ETAUpdated";
        case EventType::ExecutionPlanGenerated: return "ExecutionPlanGenerated";
        case EventType::ExecutionPlanApproved:  return "ExecutionPlanApproved";
        case EventType::ExecutionStarted:       return "ExecutionStarted";
        case EventType::ExecutionCompleted:     return "ExecutionCompleted";
        case EventType::EmergencyFinished:      return "EmergencyFinished";
        case EventType::HospitalUpdated:        return "HospitalUpdated";
        case EventType::TrafficIncident:        return "TrafficIncident";
        case EventType::NodeRecovered:          return "NodeRecovered";

        // Phase 6
        case EventType::EmergencyValidated:     return "EmergencyValidated";
        case EventType::EmergencyAssigned:      return "EmergencyAssigned";
        case EventType::HospitalAssigned:       return "HospitalAssigned";
        case EventType::HospitalChanged:        return "HospitalChanged";
        case EventType::MissionCompleted:       return "MissionCompleted";
        case EventType::MissionArchived:        return "MissionArchived";
        case EventType::ResourceAllocated:      return "ResourceAllocated";
        case EventType::ResourceReleased:       return "ResourceReleased";

        // Phase 7 IRSA
        case EventType::RoadConditionUpdated:     return "RoadConditionUpdated";
        case EventType::RoadIncidentDetected:     return "RoadIncidentDetected";
        case EventType::CorridorReplanningAdvised: return "CorridorReplanningAdvised";
        case EventType::CameraStatusChanged:      return "CameraStatusChanged";
        case EventType::TrafficDensityUpdated:    return "TrafficDensityUpdated";
        default:                                return "Unknown";
    }
}

struct Event {
    EventType type{EventType::EmergencyCreated};
    std::string sourceId;
    std::string targetId;
    uint64_t timestampMonoMs{0};
    std::string payload;

    Event() = default;
    Event(EventType t, const std::string& src, const std::string& tgt = "", const std::string& data = "", uint64_t ts = 0)
        : type(t), sourceId(src), targetId(tgt), timestampMonoMs(ts), payload(data) {}
};

using SubscriptionId = uint64_t;
using EventHandler = std::function<void(const Event&)>;

class IEventBus {
public:
    virtual ~IEventBus() = default;

    virtual SubscriptionId Subscribe(EventType type, EventHandler handler) = 0;
    virtual SubscriptionId SubscribeAll(EventHandler handler) = 0;
    virtual void Unsubscribe(SubscriptionId id) = 0;
    virtual void Publish(const Event& event) = 0;
    virtual size_t GetSubscriberCount(EventType type) const = 0;
    virtual void Clear() = 0;
};

} // namespace aegis::coordinator::orchestration

