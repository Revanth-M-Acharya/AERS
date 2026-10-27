/**
 * @file topics.hpp
 * @brief Unified, standardized MQTT topic hierarchy for SMART AEGIS.
 */

#ifndef AEGIS_NETWORK_TOPICS_HPP
#define AEGIS_NETWORK_TOPICS_HPP

#include <string>
#include <string_view>

namespace aegis {
namespace network {
namespace topics {

// ==============================================================================
// 1. Intersection / Traffic Control Domain
// ==============================================================================
// Command topic (Coordinator / Operator -> Traffic Signal Node)
// Supported commands: PREEMPT, CLEAR, RESTORE, HEARTBEAT, PING, STATUS_REQUEST
inline std::string IntersectionCommands(std::string_view intersectionId) {
    return "aegis/intersection/" + std::string(intersectionId) + "/commands";
}

// Status & Heartbeat topic (Traffic Signal Node -> Coordinator / Dashboard)
inline std::string IntersectionStatus(std::string_view intersectionId) {
    return "aegis/intersection/" + std::string(intersectionId) + "/status";
}

// Wildcard filters
inline constexpr std::string_view ALL_INTERSECTION_COMMANDS = "aegis/intersection/+/commands";
inline constexpr std::string_view ALL_INTERSECTION_STATUS = "aegis/intersection/+/status";

// Legacy alias filters for backwards compatibility
inline constexpr std::string_view LEGACY_TRAFFIC_PREEMPT = "aegis/traffic/+/preempt";
inline constexpr std::string_view LEGACY_TRAFFIC_STATUS = "aegis/traffic/+/status";

// ==============================================================================
// 2. Fleet / Ambulance Domain
// ==============================================================================
// 10 Hz Kinematics / GPS Telemetry
inline std::string FleetLocation(std::string_view unitId) {
    return "aegis/fleet/" + std::string(unitId) + "/location";
}

// 1 Hz Streaming Patient Vitals
inline std::string FleetVitals(std::string_view unitId) {
    return "aegis/fleet/" + std::string(unitId) + "/vitals";
}

// Asynchronous Patient Deterioration Alarms
inline std::string FleetAlarms(std::string_view unitId) {
    return "aegis/fleet/" + std::string(unitId) + "/alarms";
}

// 1 Hz Node Heartbeat & System Health
inline std::string FleetHeartbeat(std::string_view unitId) {
    return "aegis/fleet/" + std::string(unitId) + "/heartbeat";
}

// Operational State Transitions (AVAILABLE, DISPATCHED, ON_SCENE, etc.)
inline std::string FleetState(std::string_view unitId) {
    return "aegis/fleet/" + std::string(unitId) + "/state";
}

// Wildcard filters
inline constexpr std::string_view ALL_FLEET_LOCATION = "aegis/fleet/+/location";
inline constexpr std::string_view ALL_FLEET_VITALS = "aegis/fleet/+/vitals";
inline constexpr std::string_view ALL_FLEET_ALARMS = "aegis/fleet/+/alarms";
inline constexpr std::string_view ALL_FLEET_HEARTBEAT = "aegis/fleet/+/heartbeat";
inline constexpr std::string_view ALL_FLEET_STATE = "aegis/fleet/+/state";

// Legacy alias filter for backward compatibility
inline constexpr std::string_view LEGACY_AMBULANCE_GPS = "aegis/ambulance/+/gps";

// ==============================================================================
// 3. Hospital Domain
// ==============================================================================
// Clinical Pre-Arrival Advisories / Directives to In-Transit Ambulance
inline std::string HospitalCommands(std::string_view unitId) {
    return "aegis/hospital/" + std::string(unitId) + "/commands";
}

inline constexpr std::string_view ALL_HOSPITAL_COMMANDS = "aegis/hospital/+/commands";

// ==============================================================================
// 4. Dispatch & Incident Domain
// ==============================================================================
// 911 Emergency Distress Call Intake
inline constexpr std::string_view EMERGENCY_REQUEST = "aegis/emergency/request";

// Dispatch Notification to Units
inline std::string IncidentDispatched(std::string_view incidentId) {
    return "aegis/incident/" + std::string(incidentId) + "/dispatched";
}

inline constexpr std::string_view ALL_INCIDENT_DISPATCHED = "aegis/incident/+/dispatched";

// ==============================================================================
// 5. System Health & Diagnostics Domain
// ==============================================================================
inline std::string SystemHeartbeat(std::string_view nodeId) {
    return "aegis/system/heartbeat/" + std::string(nodeId);
}

inline constexpr std::string_view ALL_SYSTEM_HEARTBEATS = "aegis/system/heartbeat/+";
inline constexpr std::string_view SYSTEM_LOGS = "aegis/system/logs";

} // namespace topics
} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_TOPICS_HPP
