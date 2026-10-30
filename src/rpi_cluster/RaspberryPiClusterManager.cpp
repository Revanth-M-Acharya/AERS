/**
 * @file RaspberryPiClusterManager.cpp
 * @brief SMART AEGIS (AERS) Phase 14A - Raspberry Pi Cluster Deployment & Health Management Engine
 *
 * Implements the edge node deployment, health monitoring, synchronization, configuration management,
 * heartbeat supervision, firmware tracking, node discovery, and fault recovery of every Raspberry Pi 4
 * participating in the SMART AEGIS network.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 Model B & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "RaspberryPiClusterManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace cluster {

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(NodeHealth health) {
    switch (health) {
        case NodeHealth::HEALTHY:  return "HEALTHY";
        case NodeHealth::WARNING:  return "WARNING";
        case NodeHealth::CRITICAL: return "CRITICAL";
        case NodeHealth::OFFLINE:  return "OFFLINE";
        default:                   return "HEALTHY";
    }
}

std::string ToString(DeploymentState state) {
    switch (state) {
        case DeploymentState::NOT_DEPLOYED: return "NOT_DEPLOYED";
        case DeploymentState::DEPLOYING:    return "DEPLOYING";
        case DeploymentState::DEPLOYED:     return "DEPLOYED";
        case DeploymentState::FAILED:       return "FAILED";
        case DeploymentState::RECOVERING:   return "RECOVERING";
        default:                            return "NOT_DEPLOYED";
    }
}

std::string ToString(SyncState state) {
    switch (state) {
        case SyncState::NOT_SYNCED: return "NOT_SYNCED";
        case SyncState::SYNCING:    return "SYNCING";
        case SyncState::SYNCED:     return "SYNCED";
        default:                    return "NOT_SYNCED";
    }
}

NodeHealth NodeHealthFromString(const std::string& str) {
    if (str == "HEALTHY")  return NodeHealth::HEALTHY;
    if (str == "WARNING")  return NodeHealth::WARNING;
    if (str == "CRITICAL") return NodeHealth::CRITICAL;
    if (str == "OFFLINE")  return NodeHealth::OFFLINE;
    return NodeHealth::HEALTHY;
}

DeploymentState DeploymentStateFromString(const std::string& str) {
    if (str == "NOT_DEPLOYED") return DeploymentState::NOT_DEPLOYED;
    if (str == "DEPLOYING")    return DeploymentState::DEPLOYING;
    if (str == "DEPLOYED")     return DeploymentState::DEPLOYED;
    if (str == "FAILED")       return DeploymentState::FAILED;
    if (str == "RECOVERING")   return DeploymentState::RECOVERING;
    return DeploymentState::NOT_DEPLOYED;
}

SyncState SyncStateFromString(const std::string& str) {
    if (str == "NOT_SYNCED") return SyncState::NOT_SYNCED;
    if (str == "SYNCING")    return SyncState::SYNCING;
    if (str == "SYNCED")     return SyncState::SYNCED;
    return SyncState::NOT_SYNCED;
}

// =============================================================================
// Constructor & Time Helper
// =============================================================================

RaspberryPiClusterManager::RaspberryPiClusterManager() = default;

uint64_t RaspberryPiClusterManager::GetCurrentTimeMs() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// =============================================================================
// Node Lifecycle & Discovery
// =============================================================================

bool RaspberryPiClusterManager::RegisterNode(const RaspberryPiNode& node) {
    if (node.nodeId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    if (nodes_.find(node.nodeId) != nodes_.end()) {
        return false; // Prevent duplicate registration
    }

    nodes_[node.nodeId] = node;
    return true;
}

bool RaspberryPiClusterManager::RemoveNode(const std::string& nodeId) {
    if (nodeId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    nodes_.erase(it);
    return true;
}

bool RaspberryPiClusterManager::DiscoverNode(const std::string& nodeId,
                                            const std::string& ipAddress,
                                            const std::string& hostname) {
    if (nodeId.empty() || ipAddress.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        // Node already known; update discovery attributes
        it->second.ipAddress = ipAddress;
        if (!hostname.empty()) {
            it->second.hostname = hostname;
        }
        it->second.online = true;
        return true;
    }

    RaspberryPiNode newNode(nodeId, hostname, ipAddress);
    newNode.deploymentState = DeploymentState::NOT_DEPLOYED;
    newNode.syncState = SyncState::NOT_SYNCED;
    newNode.nodeHealth = NodeHealth::HEALTHY;
    newNode.online = true;

    nodes_[nodeId] = newNode;
    return true;
}

// =============================================================================
// Heartbeat & Telemetry Supervision
// =============================================================================

bool RaspberryPiClusterManager::UpdateHeartbeat(const std::string& nodeId, uint64_t timestamp) {
    if (nodeId.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    double latencyMs = 1.0;
    {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        auto it = lastHeartbeats_.find(nodeId);
        if (it != lastHeartbeats_.end() && timestamp >= it->second && it->second > 0) {
            latencyMs = static_cast<double>(timestamp - it->second);
        }
        lastHeartbeats_[nodeId] = timestamp;
    }

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return false;
        }
        it->second.lastHeartbeat = timestamp;
        it->second.online = true;
        it->second.latency = latencyMs;
        if (it->second.nodeHealth == NodeHealth::OFFLINE) {
            it->second.nodeHealth = NodeHealth::HEALTHY;
        }
    }

    return true;
}

bool RaspberryPiClusterManager::UpdateHealth(const std::string& nodeId, NodeHealth health) {
    if (nodeId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.nodeHealth = health;
    if (health == NodeHealth::OFFLINE) {
        it->second.online = false;
    }
    return true;
}

bool RaspberryPiClusterManager::UpdateTemperature(const std::string& nodeId, double temperatureCelsius) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        temperatures_[nodeId] = temperatureCelsius;
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        totalTemperatureSum_ += temperatureCelsius;
        totalTemperatureSamples_++;
        statistics_.averageTemperature = totalTemperatureSum_ / totalTemperatureSamples_;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.cpuTemperature = temperatureCelsius;

    // Rule 2: Temperature > threshold becomes CRITICAL
    if (it->second.online) {
        if (temperatureCelsius > CRITICAL_TEMP_THRESHOLD_C) {
            it->second.nodeHealth = NodeHealth::CRITICAL;
        } else if (it->second.cpuUsage > WARNING_CPU_THRESHOLD_PCT ||
                   it->second.ramUsage > WARNING_RAM_THRESHOLD_PCT ||
                   it->second.storageUsage > WARNING_STORAGE_THRESHOLD_PCT) {
            it->second.nodeHealth = NodeHealth::WARNING;
        } else {
            it->second.nodeHealth = NodeHealth::HEALTHY;
        }
    }

    return true;
}

bool RaspberryPiClusterManager::UpdateCPUUsage(const std::string& nodeId, double cpuUsagePercent) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        cpuUsages_[nodeId] = cpuUsagePercent;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.cpuUsage = cpuUsagePercent;

    // Rule 3: CPU > threshold becomes WARNING (unless temperature is CRITICAL)
    if (it->second.online) {
        if (it->second.cpuTemperature > CRITICAL_TEMP_THRESHOLD_C) {
            it->second.nodeHealth = NodeHealth::CRITICAL;
        } else if (cpuUsagePercent > WARNING_CPU_THRESHOLD_PCT ||
                   it->second.ramUsage > WARNING_RAM_THRESHOLD_PCT ||
                   it->second.storageUsage > WARNING_STORAGE_THRESHOLD_PCT) {
            it->second.nodeHealth = NodeHealth::WARNING;
        } else {
            it->second.nodeHealth = NodeHealth::HEALTHY;
        }
    }

    return true;
}

bool RaspberryPiClusterManager::UpdateRAMUsage(const std::string& nodeId, double ramUsagePercent) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        ramUsages_[nodeId] = ramUsagePercent;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.ramUsage = ramUsagePercent;

    // Rule 4: RAM > threshold becomes WARNING (unless temperature is CRITICAL)
    if (it->second.online) {
        if (it->second.cpuTemperature > CRITICAL_TEMP_THRESHOLD_C) {
            it->second.nodeHealth = NodeHealth::CRITICAL;
        } else if (it->second.cpuUsage > WARNING_CPU_THRESHOLD_PCT ||
                   ramUsagePercent > WARNING_RAM_THRESHOLD_PCT ||
                   it->second.storageUsage > WARNING_STORAGE_THRESHOLD_PCT) {
            it->second.nodeHealth = NodeHealth::WARNING;
        } else {
            it->second.nodeHealth = NodeHealth::HEALTHY;
        }
    }

    return true;
}

bool RaspberryPiClusterManager::UpdateStorageUsage(const std::string& nodeId, double storageUsagePercent) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        storageUsages_[nodeId] = storageUsagePercent;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.storageUsage = storageUsagePercent;

    // Rule 5: Storage > threshold becomes WARNING (unless temperature is CRITICAL)
    if (it->second.online) {
        if (it->second.cpuTemperature > CRITICAL_TEMP_THRESHOLD_C) {
            it->second.nodeHealth = NodeHealth::CRITICAL;
        } else if (it->second.cpuUsage > WARNING_CPU_THRESHOLD_PCT ||
                   it->second.ramUsage > WARNING_RAM_THRESHOLD_PCT ||
                   storageUsagePercent > WARNING_STORAGE_THRESHOLD_PCT) {
            it->second.nodeHealth = NodeHealth::WARNING;
        } else {
            it->second.nodeHealth = NodeHealth::HEALTHY;
        }
    }

    return true;
}

bool RaspberryPiClusterManager::UpdateLatency(const std::string& nodeId, double latencyMs) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        latencies_[nodeId] = latencyMs;
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        totalLatencySum_ += latencyMs;
        totalLatencySamples_++;
        statistics_.averageLatency = totalLatencySum_ / totalLatencySamples_;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.latency = latencyMs;
    return true;
}

std::vector<std::string> RaspberryPiClusterManager::DetectOfflineNodes(uint64_t currentTime, uint64_t timeoutThresholdMs) {
    std::vector<std::string> offlineNodes;

    std::lock_guard<std::mutex> lock(registryMutex_);
    for (auto& pair : nodes_) {
        if (!pair.second.online) continue;
        // Rule 1: Nodes missing heartbeat for timeout become OFFLINE
        if (pair.second.lastHeartbeat > 0 &&
            currentTime > pair.second.lastHeartbeat &&
            (currentTime - pair.second.lastHeartbeat) > timeoutThresholdMs) {
            pair.second.online = false;
            pair.second.nodeHealth = NodeHealth::OFFLINE;
            offlineNodes.push_back(pair.first);
        }
    }

    return offlineNodes;
}

// =============================================================================
// Deployment & Synchronization Management
// =============================================================================

bool RaspberryPiClusterManager::UpdateDeploymentState(const std::string& nodeId, DeploymentState state) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(deploymentMutex_);
        deploymentStates_[nodeId] = state;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.deploymentState = state;
    return true;
}

bool RaspberryPiClusterManager::UpdateSynchronizationState(const std::string& nodeId, SyncState state) {
    if (nodeId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(deploymentMutex_);
        syncStates_[nodeId] = state;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.syncState = state;
    return true;
}

bool RaspberryPiClusterManager::AssignIntersection(const std::string& nodeId, const std::string& intersectionId) {
    if (nodeId.empty() || intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    // Rule 8: Offline nodes are excluded automatically
    if (!it->second.online) {
        return false;
    }

    // Rule 6: Only DEPLOYED nodes may participate
    if (it->second.deploymentState != DeploymentState::DEPLOYED) {
        return false;
    }

    // Rule 7: Only SYNCED nodes may participate
    if (it->second.syncState != SyncState::SYNCED) {
        return false;
    }

    it->second.assignedIntersection = intersectionId;
    return true;
}

bool RaspberryPiClusterManager::AssignMission(const std::string& nodeId, const std::string& missionId) {
    if (nodeId.empty() || missionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    // Rule 8: Offline nodes are excluded automatically
    if (!it->second.online) {
        return false;
    }

    // Rule 6: Only DEPLOYED nodes may participate
    if (it->second.deploymentState != DeploymentState::DEPLOYED) {
        return false;
    }

    // Rule 7: Only SYNCED nodes may participate
    if (it->second.syncState != SyncState::SYNCED) {
        return false;
    }

    it->second.assignedMission = missionId;
    return true;
}

bool RaspberryPiClusterManager::RecoverNode(const std::string& nodeId, uint64_t timestamp) {
    if (nodeId.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    bool found = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it != nodes_.end()) {
            it->second.online = true;
            it->second.nodeHealth = NodeHealth::HEALTHY;
            it->second.lastHeartbeat = timestamp;
            it->second.deploymentState = DeploymentState::RECOVERING;
            // Rule 9: Recovered nodes require synchronization before rejoining
            it->second.syncState = SyncState::NOT_SYNCED;
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        lastHeartbeats_[nodeId] = timestamp;
    }

    return found;
}

bool RaspberryPiClusterManager::PushConfiguration(const std::string& nodeId, const std::string& configPayload) {
    if (nodeId.empty() || configPayload.empty()) {
        return false;
    }

    uint32_t version = 1;
    {
        std::lock_guard<std::mutex> lock(configurationMutex_);
        configurations_[nodeId] = configPayload;
        configVersions_[nodeId]++;
        version = configVersions_[nodeId];
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }

    it->second.lastConfigPayload = configPayload;
    it->second.configVersion = version;
    return true;
}

bool RaspberryPiClusterManager::VerifyClusterReadiness() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    if (nodes_.empty()) {
        return false;
    }

    // Rule 10: Cluster readiness requires every active node healthy, deployed, and synced
    for (const auto& pair : nodes_) {
        const auto& node = pair.second;
        if (!node.online) {
            return false;
        }
        if (node.nodeHealth == NodeHealth::CRITICAL || node.nodeHealth == NodeHealth::OFFLINE) {
            return false;
        }
        if (node.deploymentState != DeploymentState::DEPLOYED) {
            return false;
        }
        if (node.syncState != SyncState::SYNCED) {
            return false;
        }
    }

    return true;
}

// =============================================================================
// Node Queries
// =============================================================================

bool RaspberryPiClusterManager::GetNode(const std::string& nodeId, RaspberryPiNode& outNode) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    outNode = it->second;
    return true;
}

std::vector<RaspberryPiNode> RaspberryPiClusterManager::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<RaspberryPiNode> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<RaspberryPiNode> RaspberryPiClusterManager::GetHealthyNodes() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<RaspberryPiNode> result;
    for (const auto& pair : nodes_) {
        if (pair.second.online && pair.second.nodeHealth == NodeHealth::HEALTHY) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<RaspberryPiNode> RaspberryPiClusterManager::GetOfflineNodes() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<RaspberryPiNode> result;
    for (const auto& pair : nodes_) {
        if (!pair.second.online || pair.second.nodeHealth == NodeHealth::OFFLINE) {
            result.push_back(pair.second);
        }
    }
    return result;
}

// =============================================================================
// Structured Reports
// =============================================================================

std::string RaspberryPiClusterManager::GenerateClusterHealthReport() const {
    ClusterStatistics stats = GetStatistics();

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[CLUSTER]\n\n";
    oss << "Cluster:\nSMART_AEGIS_RPI_CLUSTER\n\n";
    oss << "Total Nodes:\n" << stats.totalNodes << "\n\n";
    oss << "Online:\n" << stats.onlineNodes << "\n\n";
    oss << "Offline:\n" << stats.offlineNodes << "\n\n";
    oss << "Healthy:\n" << stats.healthyNodes << "\n\n";
    oss << "Warning:\n" << stats.warningNodes << "\n\n";
    oss << "Critical:\n" << stats.criticalNodes << "\n\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Average Temperature:\n" << stats.averageTemperature << " C\n\n";
    oss << "Average Latency:\n" << stats.averageLatency << " ms\n";
    return oss.str();
}

std::string RaspberryPiClusterManager::GenerateDeploymentReport() const {
    ClusterStatistics stats = GetStatistics();
    bool ready = VerifyClusterReadiness();

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[DEPLOYMENT]\n\n";
    oss << "Cluster Deployment:\nSMART_AEGIS_CLUSTER\n\n";
    oss << "Total Nodes:\n" << stats.totalNodes << "\n\n";
    oss << "Deployed:\n" << stats.deployedNodes << "\n\n";
    oss << "Synchronized:\n" << stats.synchronizedNodes << "\n\n";
    oss << "Readiness:\n" << (ready ? "READY" : "NOT_READY") << "\n";
    return oss.str();
}

std::string RaspberryPiClusterManager::GenerateNodeReport(const std::string& nodeId) const {
    RaspberryPiNode node;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return "";
        }
        node = it->second;
    }

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[NODE]\n\n";
    oss << "Node:\n" << node.nodeId << "\n\n";
    oss << "Hostname:\n" << node.hostname << "\n\n";
    oss << "IP Address:\n" << node.ipAddress << "\n\n";
    oss << "Health:\n" << ToString(node.nodeHealth) << "\n\n";
    oss << "Deployment:\n" << ToString(node.deploymentState) << "\n\n";
    oss << "Sync:\n" << ToString(node.syncState) << "\n\n";
    oss << std::fixed << std::setprecision(1);
    oss << "CPU:\n" << node.cpuUsage << "%\n\n";
    oss << "Temperature:\n" << node.cpuTemperature << " C\n\n";
    oss << std::setprecision(2);
    oss << "Latency:\n" << node.latency << " ms\n";
    return oss.str();
}

// =============================================================================
// Statistics & Lifecycle
// =============================================================================

ClusterStatistics RaspberryPiClusterManager::GetStatistics() const {
    ClusterStatistics stats;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        stats.totalNodes = static_cast<uint32_t>(nodes_.size());
        for (const auto& pair : nodes_) {
            const auto& n = pair.second;
            if (n.online) {
                stats.onlineNodes++;
            } else {
                stats.offlineNodes++;
            }

            switch (n.nodeHealth) {
                case NodeHealth::HEALTHY:  stats.healthyNodes++; break;
                case NodeHealth::WARNING:  stats.warningNodes++; break;
                case NodeHealth::CRITICAL: stats.criticalNodes++; break;
                case NodeHealth::OFFLINE:  break;
            }

            if (n.deploymentState == DeploymentState::DEPLOYED) {
                stats.deployedNodes++;
            }
            if (n.syncState == SyncState::SYNCED) {
                stats.synchronizedNodes++;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats.averageLatency = statistics_.averageLatency;
        stats.averageTemperature = statistics_.averageTemperature;
    }

    return stats;
}

void RaspberryPiClusterManager::Reset() {
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        nodes_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        lastHeartbeats_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(healthMutex_);
        temperatures_.clear();
        cpuUsages_.clear();
        ramUsages_.clear();
        storageUsages_.clear();
        latencies_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(deploymentMutex_);
        deploymentStates_.clear();
        syncStates_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_ = ClusterStatistics{};
        totalLatencySum_ = 0.0;
        totalLatencySamples_ = 0;
        totalTemperatureSum_ = 0.0;
        totalTemperatureSamples_ = 0;
    }
    {
        std::lock_guard<std::mutex> lock(configurationMutex_);
        configurations_.clear();
        configVersions_.clear();
    }
}

} // namespace cluster
} // namespace aegis
