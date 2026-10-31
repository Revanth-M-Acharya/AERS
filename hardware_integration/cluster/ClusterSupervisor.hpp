/**
 * @file ClusterSupervisor.hpp
 * @brief Multi-Node Raspberry Pi Cluster Supervisor and Health Monitor for SMART AEGIS.
 * Monitors Coordinator and all Roadside Intersection nodes across Wi-Fi / Ethernet topology.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/config/NodeConfigManager.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>

namespace aegis::hardware_integration {

enum class NodeConnectivityState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DEGRADED
};

enum class ClusterNodeHealth {
    HEALTHY,    // Heartbeats on time, nominal CPU/RAM, all sensors reporting
    WARNING,    // Jitter on heartbeats, elevated CPU, or non-critical sensor degraded
    CRITICAL,   // Missed heartbeats or hardware relay interlock tripped
    OFFLINE     // Complete communication silence > timeout
};

struct ClusterNodeTelemetry {
    std::string nodeId;
    NodeRole role{NodeRole::TRAFFIC_NODE};
    std::string ipAddress;
    uint16_t port{0};

    NodeConnectivityState connectivity{NodeConnectivityState::DISCONNECTED};
    ClusterNodeHealth health{ClusterNodeHealth::OFFLINE};

    uint64_t lastHeartbeatMonoMs{0};
    uint64_t heartbeatElapsedMs{0};
    double rttLatencyMs{0.0};

    double cpuPercent{0.0};
    uint64_t memoryRssKb{0};
    uint32_t activeCorridorCount{0};

    std::string currentSignalState{"ALL_RED"};
    uint64_t totalVehicleDetections{0};
    bool vehiclePresentInBeam{false};
    double clockSkewMs{0.0};
    bool isCommissioned{false};
};

class ClusterSupervisor {
public:
    ClusterSupervisor();
    ~ClusterSupervisor() = default;

    /**
     * @brief Register a node in the cluster supervisor.
     */
    void RegisterNode(const std::string& nodeId, NodeRole role, const std::string& ip, uint16_t port);

    /**
     * @brief Update heartbeat from a node with latency and timestamp.
     */
    void RecordHeartbeat(const std::string& nodeId, double rttLatencyMs = 1.0, uint64_t currentMonoMs = 0);

    /**
     * @brief Update telemetry metrics from a node.
     */
    void UpdateTelemetry(const std::string& nodeId,
                         double cpuPercent,
                         uint64_t memoryRssKb,
                         const std::string& signalState,
                         uint64_t vehicleDetections,
                         bool vehiclePresent,
                         double clockSkewMs = 0.0);

    /**
     * @brief Set commissioning status for a node.
     */
    void SetNodeCommissioned(const std::string& nodeId, bool commissioned);

    /**
     * @brief Evaluate health of all registered nodes against timeouts.
     */
    void EvaluateClusterHealth(uint64_t currentMonoMs = 0);

    /**
     * @brief Get telemetry snapshot of a specific node.
     */
    std::optional<ClusterNodeTelemetry> GetNodeTelemetry(const std::string& nodeId) const;

    /**
     * @brief Get telemetry snapshot of all nodes.
     */
    std::vector<ClusterNodeTelemetry> GetAllNodes() const;

    /**
     * @brief Render full ASCII cluster monitoring dashboard.
     */
    std::string RenderDashboardAscii(uint64_t currentMonoMs = 0) const;

    /**
     * @brief Export cluster telemetry in structured JSON format.
     */
    std::string ExportJson(uint64_t currentMonoMs = 0) const;

    /**
     * @brief Initialize default 4-Pi cluster topology (1 Coordinator + 3 Traffic Nodes).
     */
    void InitializeDefault4PiCluster();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ClusterNodeTelemetry> nodes_;
    uint64_t heartbeatTimeoutMs_{3000};
    uint64_t offlineTimeoutMs_{6000};
};

} // namespace aegis::hardware_integration
