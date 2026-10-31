/**
 * @file ClusterMonitor.hpp
 * @brief Supervisory cluster monitor tracking hardware health, CPU/RAM utilization, thread counts,
 * network round-trip latencies, and clock synchronization across the 4 Raspberry Pi boards.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

struct ClusterNodeTelemetry {
    std::string nodeId;
    std::string nodeName;
    std::string role; // "COORDINATOR", "TRAFFIC_NODE"
    std::string ipAddress{"192.168.1.100"};
    bool isOnline{true};
    uint64_t lastHeartbeatMonoMs{0};
    double cpuUsagePercent{12.4};
    double memoryUsagePercent{24.8};
    uint32_t currentThreadCount{8};
    double networkLatencyMs{0.82};
    std::string currentMission{"None"};
    std::string lastEvent{"HeartbeatAck"};
    std::string syncStatus{"SYNCHRONIZED"};
    int64_t clockDriftMicroseconds{8};
};

class ClusterMonitor {
public:
    ClusterMonitor();
    ~ClusterMonitor() = default;

    // Prevent copying
    ClusterMonitor(const ClusterMonitor&) = delete;
    ClusterMonitor& operator=(const ClusterMonitor&) = delete;

    /**
     * @brief Initialize default 4-Pi cluster topology (Coordinator Pi, Node A, Node B, Node C).
     */
    void InitializeDefaultNodes();

    /**
     * @brief Update telemetry for a node upon receiving heartbeat/diagnostics packet.
     */
    bool UpdateHeartbeat(const std::string& nodeId,
                         double cpuPercent,
                         double memPercent,
                         uint32_t threadCount,
                         double latencyMs,
                         int64_t clockDriftUs,
                         uint64_t timestampMs);

    /**
     * @brief Set operational liveness state (Online / Offline).
     */
    bool SetNodeOnline(const std::string& nodeId, bool online);

    /**
     * @brief Set active mission association.
     */
    bool SetCurrentMission(const std::string& nodeId, const std::string& missionId);

    /**
     * @brief Record most recent event string.
     */
    bool SetLastEvent(const std::string& nodeId, const std::string& eventStr);

    /**
     * @brief Query single node telemetry.
     */
    std::optional<ClusterNodeTelemetry> GetNode(const std::string& nodeId) const;

    /**
     * @brief Retrieve all nodes in cluster.
     */
    std::vector<ClusterNodeTelemetry> GetAllNodes() const;

    /**
     * @brief Check whether all nodes are currently online.
     */
    bool AreAllNodesOnline() const;

    /**
     * @brief Clear all node records.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ClusterNodeTelemetry> nodes_;
    std::vector<std::string> displayOrder_;
};

} // namespace aegis::command_center
