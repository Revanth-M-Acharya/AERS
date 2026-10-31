/**
 * @file ClusterMonitor.cpp
 * @brief Implementation of Raspberry Pi Cluster Monitor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/cluster/ClusterMonitor.hpp"

namespace aegis::command_center {

ClusterMonitor::ClusterMonitor() {
    InitializeDefaultNodes();
}

void ClusterMonitor::InitializeDefaultNodes() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    displayOrder_.clear();

    auto add = [this](const std::string& id, const std::string& name, const std::string& role,
                      const std::string& ip, double cpu, double mem, uint32_t threads,
                      double latency, int64_t drift) {
        ClusterNodeTelemetry n;
        n.nodeId = id;
        n.nodeName = name;
        n.role = role;
        n.ipAddress = ip;
        n.isOnline = true;
        n.cpuUsagePercent = cpu;
        n.memoryUsagePercent = mem;
        n.currentThreadCount = threads;
        n.networkLatencyMs = latency;
        n.clockDriftMicroseconds = drift;
        n.syncStatus = (std::abs(drift) < 50) ? "SYNCHRONIZED" : "DRIFT_DETECTED";
        nodes_[id] = n;
        displayOrder_.push_back(id);
    };

    add("RPI_COORD", "Coordinator Pi", "COORDINATOR",  "192.168.1.100", 14.8, 28.4, 12, 0.25, 0);
    add("INT_1",     "Node A (Int 1)",  "TRAFFIC_NODE", "192.168.1.101", 11.2, 22.1, 8,  0.84, 12);
    add("INT_2",     "Node B (Int 2)",  "TRAFFIC_NODE", "192.168.1.102", 12.6, 23.5, 9,  0.79, -8);
    add("INT_3",     "Node C (Int 3)",  "TRAFFIC_NODE", "192.168.1.103", 10.9, 21.8, 8,  0.91, 15);
}

bool ClusterMonitor::UpdateHeartbeat(const std::string& nodeId,
                                    double cpuPercent,
                                    double memPercent,
                                    uint32_t threadCount,
                                    double latencyMs,
                                    int64_t clockDriftUs,
                                    uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.isOnline = true;
    it->second.cpuUsagePercent = cpuPercent;
    it->second.memoryUsagePercent = memPercent;
    it->second.currentThreadCount = threadCount;
    it->second.networkLatencyMs = latencyMs;
    it->second.clockDriftMicroseconds = clockDriftUs;
    it->second.lastHeartbeatMonoMs = timestampMs;
    it->second.syncStatus = (std::abs(clockDriftUs) < 50) ? "SYNCHRONIZED" : "DRIFT_DETECTED";
    it->second.lastEvent = "HeartbeatAck";
    return true;
}

bool ClusterMonitor::SetNodeOnline(const std::string& nodeId, bool online) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.isOnline = online;
    if (!online) {
        it->second.syncStatus = "DISCONNECTED";
        it->second.lastEvent = "NodeDisconnected";
    }
    return true;
}

bool ClusterMonitor::SetCurrentMission(const std::string& nodeId, const std::string& missionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.currentMission = missionId;
    return true;
}

bool ClusterMonitor::SetLastEvent(const std::string& nodeId, const std::string& eventStr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.lastEvent = eventStr;
    return true;
}

std::optional<ClusterNodeTelemetry> ClusterMonitor::GetNode(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ClusterNodeTelemetry> ClusterMonitor::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ClusterNodeTelemetry> results;
    results.reserve(displayOrder_.size());
    for (const auto& id : displayOrder_) {
        auto it = nodes_.find(id);
        if (it != nodes_.end()) {
            results.push_back(it->second);
        }
    }
    return results;
}

bool ClusterMonitor::AreAllNodesOnline() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, node] : nodes_) {
        if (!node.isOnline) return false;
    }
    return true;
}

void ClusterMonitor::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    displayOrder_.clear();
}

} // namespace aegis::command_center
