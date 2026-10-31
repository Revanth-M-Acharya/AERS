/**
 * @file HeartbeatSupervisor.cpp
 * @brief Implementation of cluster heartbeat supervisor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/health/HeartbeatSupervisor.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <vector>

namespace aegis::production {

HeartbeatSupervisor::HeartbeatSupervisor() = default;

void HeartbeatSupervisor::RegisterNode(const std::string& nodeId, ClusterNodeType type, const std::string& ip, uint32_t intervalMs, uint32_t maxMisses) {
    std::lock_guard<std::mutex> lock(mutex_);
    MonitoredNode node;
    node.nodeId = nodeId;
    node.type = type;
    node.ipAddress = ip;
    node.heartbeatIntervalMs = (intervalMs == 0) ? 1000 : intervalMs;
    node.maxAllowedMisses = (maxMisses == 0) ? 3 : maxMisses;
    node.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    node.missedCount = 0;
    node.state = NodeLiveness::ONLINE;
    node.rttLatencyMs = 0.5;
    nodes_[nodeId] = node;
}

void HeartbeatSupervisor::UnregisterNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.erase(nodeId);
}

void HeartbeatSupervisor::RecordHeartbeat(const std::string& nodeId, double rttMs) {
    NodeLiveness oldState = NodeLiveness::ONLINE;
    NodeLiveness newState = NodeLiveness::ONLINE;
    ClusterNodeType type = ClusterNodeType::TRAFFIC_NODE;
    bool stateChanged = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodes_.find(nodeId);
        if (it != nodes_.end()) {
            oldState = it->second.state;
            type = it->second.type;
            it->second.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
            it->second.missedCount = 0;
            // Exponential moving average for ping RTT
            it->second.rttLatencyMs = 0.8 * it->second.rttLatencyMs + 0.2 * rttMs;
            if (it->second.state != NodeLiveness::ONLINE) {
                it->second.state = NodeLiveness::ONLINE;
                newState = NodeLiveness::ONLINE;
                stateChanged = true;
            }
        }
    }

    if (stateChanged && callback_) {
        callback_(nodeId, type, oldState, newState);
    }
}

uint32_t HeartbeatSupervisor::CheckClusterLiveness() {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    uint32_t offlineCount = 0;
    std::vector<std::tuple<std::string, ClusterNodeType, NodeLiveness, NodeLiveness>> stateTransitions;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, node] : nodes_) {
            uint64_t elapsed = (now >= node.lastHeartbeatMonoMs) ? (now - node.lastHeartbeatMonoMs) : 0;
            uint32_t missedIntervals = (node.heartbeatIntervalMs > 0) ? static_cast<uint32_t>(elapsed / node.heartbeatIntervalMs) : 0;

            NodeLiveness previous = node.state;
            if (missedIntervals >= node.maxAllowedMisses) {
                if (node.state != NodeLiveness::OFFLINE) {
                    node.state = NodeLiveness::OFFLINE;
                    stateTransitions.emplace_back(id, node.type, previous, node.state);
                }
                offlineCount++;
            } else if (missedIntervals > 0) {
                if (node.state == NodeLiveness::ONLINE) {
                    node.state = NodeLiveness::MISSED_HEARTBEATS;
                    stateTransitions.emplace_back(id, node.type, previous, node.state);
                }
            }
            node.missedCount = missedIntervals;
        }
    }

    for (const auto& [id, type, oldS, newS] : stateTransitions) {
        if (callback_) {
            callback_(id, type, oldS, newS);
        }
    }

    return offlineCount;
}

void HeartbeatSupervisor::SetStateChangeCallback(NodeStateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

NodeLiveness HeartbeatSupervisor::GetNodeState(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second.state;
    }
    return NodeLiveness::OFFLINE;
}

MonitoredNode HeartbeatSupervisor::GetNode(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    MonitoredNode missing;
    missing.nodeId = nodeId;
    missing.state = NodeLiveness::OFFLINE;
    return missing;
}

std::vector<MonitoredNode> HeartbeatSupervisor::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<MonitoredNode> list;
    list.reserve(nodes_.size());
    for (const auto& [k, v] : nodes_) {
        list.push_back(v);
    }
    return list;
}

size_t HeartbeatSupervisor::GetOnlineCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : nodes_) {
        if (v.state == NodeLiveness::ONLINE) count++;
    }
    return count;
}

size_t HeartbeatSupervisor::GetOfflineCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : nodes_) {
        if (v.state == NodeLiveness::OFFLINE) count++;
    }
    return count;
}

void HeartbeatSupervisor::MarkNodeRecovering(const std::string& nodeId) {
    NodeLiveness oldState = NodeLiveness::OFFLINE;
    ClusterNodeType type = ClusterNodeType::TRAFFIC_NODE;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = nodes_.find(nodeId);
        if (it != nodes_.end()) {
            oldState = it->second.state;
            type = it->second.type;
            it->second.state = NodeLiveness::RECOVERING;
            found = true;
        }
    }

    if (found && callback_) {
        callback_(nodeId, type, oldState, NodeLiveness::RECOVERING);
    }
}

std::string HeartbeatSupervisor::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"total_nodes\": " << nodes_.size() << ",\n"
       << "  \"nodes\": [\n";

    bool first = true;
    for (const auto& [id, n] : nodes_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"node_id\": \"" << id << "\",\n"
           << "      \"type\": \"" << ClusterNodeTypeToString(n.type) << "\",\n"
           << "      \"state\": \"" << NodeLivenessToString(n.state) << "\",\n"
           << "      \"missed_heartbeats\": " << n.missedCount << ",\n"
           << "      \"rtt_latency_ms\": " << n.rttLatencyMs << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

} // namespace aegis::production
