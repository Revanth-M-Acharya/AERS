/**
 * @file HeartbeatSupervisor.hpp
 * @brief Distributed cluster heartbeat supervisor with automated node failure detection.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class ClusterNodeType {
    TRAFFIC_NODE,
    AMBULANCE,
    HOSPITAL,
    CAMERA
};

inline std::string ClusterNodeTypeToString(ClusterNodeType type) {
    switch (type) {
        case ClusterNodeType::TRAFFIC_NODE: return "TRAFFIC_NODE";
        case ClusterNodeType::AMBULANCE:    return "AMBULANCE";
        case ClusterNodeType::HOSPITAL:     return "HOSPITAL";
        case ClusterNodeType::CAMERA:       return "CAMERA";
        default:                            return "UNKNOWN";
    }
}

enum class NodeLiveness {
    ONLINE,
    MISSED_HEARTBEATS,
    OFFLINE,
    RECOVERING
};

inline std::string NodeLivenessToString(NodeLiveness state) {
    switch (state) {
        case NodeLiveness::ONLINE:            return "ONLINE";
        case NodeLiveness::MISSED_HEARTBEATS: return "MISSED_HEARTBEATS";
        case NodeLiveness::OFFLINE:           return "OFFLINE";
        case NodeLiveness::RECOVERING:        return "RECOVERING";
        default:                              return "UNKNOWN";
    }
}

struct MonitoredNode {
    std::string nodeId;
    ClusterNodeType type{ClusterNodeType::TRAFFIC_NODE};
    std::string ipAddress{"127.0.0.1"};
    uint64_t lastHeartbeatMonoMs{0};
    uint32_t missedCount{0};
    uint32_t maxAllowedMisses{3};
    uint32_t heartbeatIntervalMs{1000};
    NodeLiveness state{NodeLiveness::ONLINE};
    double rttLatencyMs{0.5};
};

using NodeStateChangeCallback = std::function<void(const std::string& nodeId, ClusterNodeType type, NodeLiveness oldState, NodeLiveness newState)>;

class HeartbeatSupervisor {
public:
    HeartbeatSupervisor();
    ~HeartbeatSupervisor() = default;

    void RegisterNode(const std::string& nodeId, ClusterNodeType type, const std::string& ip = "127.0.0.1", uint32_t intervalMs = 1000, uint32_t maxMisses = 3);
    void UnregisterNode(const std::string& nodeId);

    /**
     * @brief Record inbound heartbeat from a cluster node.
     */
    void RecordHeartbeat(const std::string& nodeId, double rttMs = 0.5);

    /**
     * @brief Evaluate deadlines across all registered cluster nodes.
     * @return Number of nodes currently declared OFFLINE.
     */
    uint32_t CheckClusterLiveness();

    void SetStateChangeCallback(NodeStateChangeCallback callback);

    NodeLiveness GetNodeState(const std::string& nodeId) const;
    MonitoredNode GetNode(const std::string& nodeId) const;
    std::vector<MonitoredNode> GetAllNodes() const;

    size_t GetOnlineCount() const;
    size_t GetOfflineCount() const;

    void MarkNodeRecovering(const std::string& nodeId);

    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, MonitoredNode> nodes_;
    NodeStateChangeCallback callback_;
};

} // namespace aegis::production

