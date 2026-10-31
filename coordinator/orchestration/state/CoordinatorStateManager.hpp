/**
 * @file CoordinatorStateManager.hpp
 * @brief Thread-safe single source of truth for the distributed SMART AEGIS system.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/session/EmergencySession.hpp"
#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <shared_mutex>
#include <cstdint>

namespace aegis::coordinator::orchestration {

enum class NodeHealthState {
    ONLINE,
    OFFLINE,
    RECOVERING,
    FAILED
};

inline std::string NodeHealthStateToString(NodeHealthState state) {
    switch (state) {
        case NodeHealthState::ONLINE:     return "ONLINE";
        case NodeHealthState::OFFLINE:    return "OFFLINE";
        case NodeHealthState::RECOVERING: return "RECOVERING";
        case NodeHealthState::FAILED:     return "FAILED";
        default:                          return "UNKNOWN";
    }
}

enum class ConnectionStatus {
    CONNECTED,
    DISCONNECTED,
    CONNECTING
};

inline std::string ConnectionStatusToString(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::CONNECTED:    return "CONNECTED";
        case ConnectionStatus::DISCONNECTED: return "DISCONNECTED";
        case ConnectionStatus::CONNECTING:   return "CONNECTING";
        default:                             return "UNKNOWN";
    }
}

struct NodeHealthRecord {
    std::string nodeId;
    NodeHealthState healthState{NodeHealthState::OFFLINE};
    ConnectionStatus connectionStatus{ConnectionStatus::DISCONNECTED};
    uint32_t latencyRttMs{0};
    double packetLossPercent{0.0};
    double cpuUsagePercent{0.0};
    double memUsagePercent{0.0};
    double tempCelsius{0.0};
    uint64_t lastHeartbeatMonoMs{0};
    uint32_t consecutiveMissedHeartbeats{0};
};

struct TrafficNodeRecord {
    std::string nodeId;
    std::string ipAddress;
    uint16_t port{0};
    uint64_t registeredMonoMs{0};
    uint64_t lastSeenMonoMs{0};
    bool isOperational{true};
    std::string currentSignalState{"RED"};
};

struct AmbulanceRecord {
    std::string ambulanceId;
    std::string callsign;
    uint32_t priorityLevel{1};
    std::string currentIntersection;
    std::string targetHospitalId;
    double speedKmh{50.0};
    bool isActive{true};
};

struct HospitalRecord {
    std::string hospitalId;
    std::string name;
    std::string location;
    uint32_t totalCapacity{50};
    uint32_t availableBeds{10};
    uint32_t traumaLevel{1};
};

struct NodeSyncRecord {
    std::string nodeId;
    int64_t clockOffsetMs{0};
    uint32_t roundTripTimeMs{0};
    uint64_t lastSyncMonoMs{0};
    bool isSynchronized{false};
};

class CoordinatorStateManager {
public:
    CoordinatorStateManager();
    ~CoordinatorStateManager() = default;

    // Coordinator System State
    void SetSystemState(const std::string& state);
    std::string GetSystemState() const;

    // Traffic Node Management
    void RegisterTrafficNode(const TrafficNodeRecord& node);
    void UnregisterTrafficNode(const std::string& nodeId);
    bool HasTrafficNode(const std::string& nodeId) const;
    std::optional<TrafficNodeRecord> GetTrafficNode(const std::string& nodeId) const;
    std::vector<TrafficNodeRecord> GetAllTrafficNodes() const;
    size_t GetTrafficNodeCount() const;

    // Node Health & Connection Status
    void UpdateNodeHealth(const NodeHealthRecord& health);
    std::optional<NodeHealthRecord> GetNodeHealth(const std::string& nodeId) const;
    std::vector<NodeHealthRecord> GetAllNodeHealth() const;
    void SetNodeConnectionStatus(const std::string& nodeId, ConnectionStatus status);
    ConnectionStatus GetNodeConnectionStatus(const std::string& nodeId) const;

    // Ambulance Management
    void RegisterAmbulance(const AmbulanceRecord& ambulance);
    void UnregisterAmbulance(const std::string& ambulanceId);
    std::optional<AmbulanceRecord> GetAmbulance(const std::string& ambulanceId) const;
    std::vector<AmbulanceRecord> GetAllAmbulances() const;

    // Hospital Management
    void RegisterHospital(const HospitalRecord& hospital);
    std::optional<HospitalRecord> GetHospital(const std::string& hospitalId) const;
    void UpdateHospitalCapacity(const std::string& hospitalId, uint32_t availableBeds);
    std::vector<HospitalRecord> GetAllHospitals() const;

    // Emergency Session Management
    void SaveEmergencySession(const EmergencySession& session);
    std::optional<EmergencySession> GetEmergencySession(const std::string& emergencyId) const;
    std::vector<EmergencySession> GetAllEmergencySessions() const;
    std::vector<EmergencySession> GetActiveEmergencySessions() const;
    bool RemoveEmergencySession(const std::string& emergencyId);

    // Active Execution Plans
    void SetActiveExecutionPlan(const std::string& planId, const intelligence::ExecutionPlan& plan);
    std::optional<intelligence::ExecutionPlan> GetActiveExecutionPlan(const std::string& planId) const;
    std::vector<intelligence::ExecutionPlan> GetAllActiveExecutionPlans() const;
    void RemoveActiveExecutionPlan(const std::string& planId);

    // Node Synchronization Status
    void UpdateNodeSync(const NodeSyncRecord& sync);
    std::optional<NodeSyncRecord> GetNodeSync(const std::string& nodeId) const;
    std::vector<NodeSyncRecord> GetAllNodeSync() const;

    // System Reset
    void Reset();

private:
    mutable std::shared_mutex rwMutex_;

    std::string systemState_{"IDLE"};
    std::unordered_map<std::string, TrafficNodeRecord> trafficNodes_;
    std::unordered_map<std::string, NodeHealthRecord> nodeHealth_;
    std::unordered_map<std::string, AmbulanceRecord> ambulances_;
    std::unordered_map<std::string, HospitalRecord> hospitals_;
    std::unordered_map<std::string, EmergencySession> emergencySessions_;
    std::unordered_map<std::string, intelligence::ExecutionPlan> activePlans_;
    std::unordered_map<std::string, NodeSyncRecord> nodeSyncs_;
};

} // namespace aegis::coordinator::orchestration

