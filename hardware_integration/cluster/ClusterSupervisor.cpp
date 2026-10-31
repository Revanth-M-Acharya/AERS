/**
 * @file ClusterSupervisor.cpp
 * @brief Implementation of ClusterSupervisor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/cluster/ClusterSupervisor.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::hardware_integration {

ClusterSupervisor::ClusterSupervisor() {
}

void ClusterSupervisor::InitializeDefault4PiCluster() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();

    // Node 1: Coordinator
    ClusterNodeTelemetry coord;
    coord.nodeId = "RPI_COORD";
    coord.role = NodeRole::COORDINATOR;
    coord.ipAddress = "192.168.1.100";
    coord.port = 9000;
    coord.connectivity = NodeConnectivityState::CONNECTED;
    coord.health = ClusterNodeHealth::HEALTHY;
    coord.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    coord.cpuPercent = 2.4;
    coord.memoryRssKb = 3280;
    coord.isCommissioned = true;
    nodes_["RPI_COORD"] = coord;

    // Node 2: Intersection A
    ClusterNodeTelemetry intA;
    intA.nodeId = "INT_A";
    intA.role = NodeRole::TRAFFIC_NODE;
    intA.ipAddress = "192.168.1.101";
    intA.port = 9001;
    intA.connectivity = NodeConnectivityState::CONNECTED;
    intA.health = ClusterNodeHealth::HEALTHY;
    intA.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    intA.cpuPercent = 1.1;
    intA.memoryRssKb = 1840;
    intA.currentSignalState = "ROAD_A_GREEN";
    intA.isCommissioned = true;
    nodes_["INT_A"] = intA;

    // Node 3: Intersection B
    ClusterNodeTelemetry intB;
    intB.nodeId = "INT_B";
    intB.role = NodeRole::TRAFFIC_NODE;
    intB.ipAddress = "192.168.1.102";
    intB.port = 9002;
    intB.connectivity = NodeConnectivityState::CONNECTED;
    intB.health = ClusterNodeHealth::HEALTHY;
    intB.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    intB.cpuPercent = 0.9;
    intB.memoryRssKb = 1820;
    intB.currentSignalState = "ROAD_B_GREEN";
    intB.isCommissioned = true;
    nodes_["INT_B"] = intB;

    // Node 4: Intersection C
    ClusterNodeTelemetry intC;
    intC.nodeId = "INT_C";
    intC.role = NodeRole::TRAFFIC_NODE;
    intC.ipAddress = "192.168.1.103";
    intC.port = 9003;
    intC.connectivity = NodeConnectivityState::CONNECTED;
    intC.health = ClusterNodeHealth::HEALTHY;
    intC.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    intC.cpuPercent = 1.0;
    intC.memoryRssKb = 1850;
    intC.currentSignalState = "ALL_RED";
    intC.isCommissioned = true;
    nodes_["INT_C"] = intC;
}

void ClusterSupervisor::RegisterNode(const std::string& nodeId, NodeRole role, const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(mutex_);
    ClusterNodeTelemetry node;
    node.nodeId = nodeId;
    node.role = role;
    node.ipAddress = ip;
    node.port = port;
    node.connectivity = NodeConnectivityState::CONNECTING;
    node.health = ClusterNodeHealth::WARNING;
    node.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    nodes_[nodeId] = node;
}

void ClusterSupervisor::RecordHeartbeat(const std::string& nodeId, double rttLatencyMs, uint64_t currentMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = (currentMonoMs > 0) ? currentMonoMs : common::TimeUtils::GetMonotonicTimeMs();

    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.lastHeartbeatMonoMs = now;
        it->second.heartbeatElapsedMs = 0;
        it->second.rttLatencyMs = rttLatencyMs;
        it->second.connectivity = NodeConnectivityState::CONNECTED;
        if (it->second.health == ClusterNodeHealth::OFFLINE || it->second.health == ClusterNodeHealth::CRITICAL) {
            it->second.health = ClusterNodeHealth::HEALTHY;
        }
    }
}

void ClusterSupervisor::UpdateTelemetry(const std::string& nodeId,
                                       double cpuPercent,
                                       uint64_t memoryRssKb,
                                       const std::string& signalState,
                                       uint64_t vehicleDetections,
                                       bool vehiclePresent,
                                       double clockSkewMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.cpuPercent = cpuPercent;
        it->second.memoryRssKb = memoryRssKb;
        it->second.currentSignalState = signalState;
        it->second.totalVehicleDetections = vehicleDetections;
        it->second.vehiclePresentInBeam = vehiclePresent;
        it->second.clockSkewMs = clockSkewMs;
    }
}

void ClusterSupervisor::SetNodeCommissioned(const std::string& nodeId, bool commissioned) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.isCommissioned = commissioned;
    }
}

void ClusterSupervisor::EvaluateClusterHealth(uint64_t currentMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = (currentMonoMs > 0) ? currentMonoMs : common::TimeUtils::GetMonotonicTimeMs();

    for (auto& [id, node] : nodes_) {
        node.heartbeatElapsedMs = (now >= node.lastHeartbeatMonoMs) ? (now - node.lastHeartbeatMonoMs) : 0;

        if (node.heartbeatElapsedMs >= offlineTimeoutMs_) {
            node.connectivity = NodeConnectivityState::DISCONNECTED;
            node.health = ClusterNodeHealth::OFFLINE;
        } else if (node.heartbeatElapsedMs >= heartbeatTimeoutMs_) {
            node.connectivity = NodeConnectivityState::DEGRADED;
            node.health = ClusterNodeHealth::CRITICAL;
        } else if (node.rttLatencyMs > 50.0 || node.cpuPercent > 85.0) {
            node.health = ClusterNodeHealth::WARNING;
        } else {
            node.connectivity = NodeConnectivityState::CONNECTED;
            node.health = ClusterNodeHealth::HEALTHY;
        }
    }
}

std::optional<ClusterNodeTelemetry> ClusterSupervisor::GetNodeTelemetry(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ClusterNodeTelemetry> ClusterSupervisor::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ClusterNodeTelemetry> res;
    res.reserve(nodes_.size());
    for (const auto& [id, n] : nodes_) {
        res.push_back(n);
    }
    return res;
}

static inline std::string HealthToString(ClusterNodeHealth h) {
    switch (h) {
        case ClusterNodeHealth::HEALTHY:  return "HEALTHY";
        case ClusterNodeHealth::WARNING:  return "WARNING";
        case ClusterNodeHealth::CRITICAL: return "CRITICAL";
        case ClusterNodeHealth::OFFLINE:  return "OFFLINE";
    }
    return "UNKNOWN";
}

static inline std::string ConnToString(NodeConnectivityState c) {
    switch (c) {
        case NodeConnectivityState::CONNECTED:    return "CONNECTED";
        case NodeConnectivityState::CONNECTING:   return "CONNECTING";
        case NodeConnectivityState::DEGRADED:     return "DEGRADED";
        case NodeConnectivityState::DISCONNECTED: return "DISCONNECTED";
    }
    return "UNKNOWN";
}

std::string ClusterSupervisor::RenderDashboardAscii(uint64_t currentMonoMs) const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = (currentMonoMs > 0) ? currentMonoMs : common::TimeUtils::GetMonotonicTimeMs();

    std::ostringstream ss;
    ss << "\n========================================================================================\n";
    ss << " SMART AEGIS: RASPBERRY PI 4 CLUSTER SUPERVISOR & HARDWARE TELEMETRY                    \n";
    ss << " Target: QNX Neutrino RTOS 8.0 (aarch64le) | Monitored Nodes: " << nodes_.size() << "  \n";
    ss << "========================================================================================\n";
    ss << "| Node ID   | Role         | IP Address      | Port | Conn       | Health   | CPU%  | Memory   | Signal Phase | IR Count |\n";
    ss << "|-----------|--------------|-----------------|------|------------|----------|-------|----------|--------------|----------|\n";

    for (const auto& [id, node] : nodes_) {
        uint64_t elapsed = (now >= node.lastHeartbeatMonoMs) ? (now - node.lastHeartbeatMonoMs) : 0;
        (void)elapsed;

        ss << "| " << std::left << std::setw(9) << node.nodeId
           << " | " << std::setw(12) << NodeRoleToString(node.role)
           << " | " << std::setw(15) << node.ipAddress
           << " | " << std::setw(4) << node.port
           << " | " << std::setw(10) << ConnToString(node.connectivity)
           << " | " << std::setw(8) << HealthToString(node.health)
           << " | " << std::right << std::setw(4) << std::fixed << std::setprecision(1) << node.cpuPercent << "%"
           << " | " << std::right << std::setw(6) << node.memoryRssKb << " KB"
           << " | " << std::left << std::setw(12) << node.currentSignalState
           << " | " << std::right << std::setw(8) << node.totalVehicleDetections << " |\n";
    }

    ss << "========================================================================================\n";
    return ss.str();
}

std::string ClusterSupervisor::ExportJson(uint64_t currentMonoMs) const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = (currentMonoMs > 0) ? currentMonoMs : common::TimeUtils::GetMonotonicTimeMs();

    std::ostringstream ss;
    ss << "{\n  \"timestampMonoMs\": " << now << ",\n  \"nodeCount\": " << nodes_.size() << ",\n  \"nodes\": [\n";

    size_t idx = 0;
    for (const auto& [id, node] : nodes_) {
        ss << "    {\n"
           << "      \"nodeId\": \"" << node.nodeId << "\",\n"
           << "      \"role\": \"" << NodeRoleToString(node.role) << "\",\n"
           << "      \"ipAddress\": \"" << node.ipAddress << "\",\n"
           << "      \"port\": " << node.port << ",\n"
           << "      \"connectivity\": \"" << ConnToString(node.connectivity) << "\",\n"
           << "      \"health\": \"" << HealthToString(node.health) << "\",\n"
           << "      \"cpuPercent\": " << std::fixed << std::setprecision(2) << node.cpuPercent << ",\n"
           << "      \"memoryRssKb\": " << node.memoryRssKb << ",\n"
           << "      \"currentSignalState\": \"" << node.currentSignalState << "\",\n"
           << "      \"vehicleCount\": " << node.totalVehicleDetections << ",\n"
           << "      \"isCommissioned\": " << (node.isCommissioned ? "true" : "false") << "\n"
           << "    }" << (idx + 1 < nodes_.size() ? "," : "") << "\n";
        idx++;
    }

    ss << "  ]\n}\n";
    return ss.str();
}

} // namespace aegis::hardware_integration
