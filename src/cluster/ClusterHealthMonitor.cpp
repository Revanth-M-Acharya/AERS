/**
 * @file ClusterHealthMonitor.cpp
 * @brief Implementation of Phase 3 Raspberry Pi Cluster Health Monitor for SMART AEGIS.
 */

#include "aegis/cluster/ClusterHealthMonitor.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>
#include <algorithm>

namespace aegis {
namespace cluster {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "CLUSTER_MON", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "CLUSTER_MON", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "CLUSTER_MON", msg);
}

std::string ClusterHealthSummary::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"timestampUs\":" << timestampUs << ","
        << "\"totalNodes\":" << totalNodes << ","
        << "\"healthyNodes\":" << healthyNodes << ","
        << "\"degradedNodes\":" << degradedNodes << ","
        << "\"offlineNodes\":" << offlineNodes << ","
        << "\"overallClusterScore\":" << std::fixed << std::setprecision(1) << overallClusterScore << ","
        << "\"state\":\"" << ClusterStateToString(state) << "\","
        << "\"coordinator\":{"
        << "\"id\":\"" << coordinator.coordinatorId << "\","
        << "\"mode\":\"" << CoordinatorModeToString(coordinator.mode) << "\","
        << "\"activeCorridors\":" << coordinator.activeCorridors << ","
        << "\"activeIntersections\":" << coordinator.activeIntersections << ","
        << "\"uptimeSec\":" << coordinator.uptimeSeconds
        << "},"
        << "\"nodes\":[";

    for (size_t i = 0; i < nodeReports.size(); ++i) {
        const auto& n = nodeReports[i];
        oss << "{"
            << "\"nodeId\":\"" << n.nodeId << "\","
            << "\"hostname\":\"" << n.hostname << "\","
            << "\"ip\":\"" << n.ipAddress << "\","
            << "\"isCoordinator\":" << (n.isCoordinator ? "true" : "false") << ","
            << "\"cpu\":" << std::fixed << std::setprecision(1) << n.cpuUsagePercent << ","
            << "\"ram\":" << std::fixed << std::setprecision(1) << n.memoryUsagePercent << ","
            << "\"temp\":" << std::fixed << std::setprecision(1) << n.cpuTemperatureC << ","
            << "\"heartbeatAgeMs\":" << n.heartbeatAgeMs << ","
            << "\"score\":" << std::fixed << std::setprecision(1) << n.nodeScore << ","
            << "\"isDegraded\":" << (n.isDegraded ? "true" : "false")
            << "}";
        if (i + 1 < nodeReports.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

ClusterHealthMonitor::ClusterHealthMonitor() {
    m_coordinator.coordinatorId = "COORD_01";
    m_coordinator.mode = CoordinatorMode::ACTIVE;
    m_coordinator.activeCorridors = 0;
    m_coordinator.activeIntersections = 0;
    m_coordinator.uptimeSeconds = 0;
    m_coordinator.lastPulseUs = aegis::common::GetMonotonicMicroseconds();
}

uint64_t ClusterHealthMonitor::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

bool ClusterHealthMonitor::RegisterNode(const std::string& nodeId,
                                        const std::string& hostname,
                                        const std::string& ipAddress,
                                        bool isCoordinator) {
    if (nodeId.empty() || ipAddress.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = GetCurrentTimeUs();

    NodeHealthTelemetry tel;
    tel.nodeId = nodeId;
    tel.hostname = hostname;
    tel.ipAddress = ipAddress;
    tel.isCoordinator = isCoordinator;
    tel.cpuUsagePercent = 12.0;
    tel.memoryUsagePercent = 25.0;
    tel.memoryUsageMB = 1024.0;
    tel.cpuTemperatureC = 42.0;
    tel.lastHeartbeatUs = nowUs;
    tel.heartbeatAgeMs = 0;
    tel.networkLevel = aegis::hardware::LinkQualityLevel::EXCELLENT;
    tel.networkRssi = -55.0;
    tel.packetLossPercent = 0.0;
    tel.gpioHealthy = true;
    tel.relayHealthy = true;
    tel.watchdogHealthy = true;
    tel.isDegraded = false;
    tel.degradationReason = "";
    tel.nodeScore = 100.0;

    m_nodes[nodeId] = std::move(tel);

    if (isCoordinator) {
        m_coordinator.coordinatorId = nodeId;
    }

    LogInfo("Registered node: " + nodeId + " (" + ipAddress +
            (isCoordinator ? ", COORDINATOR" : ", INTERSECTION") + ")");
    return true;
}

bool ClusterHealthMonitor::UpdateNodeTelemetry(const NodeHealthTelemetry& telemetry, uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(telemetry.nodeId);
    if (it == m_nodes.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second = telemetry;
    it->second.lastHeartbeatUs = nowUs;
    CalculateNodeScore(it->second, nowUs);
    return true;
}

bool ClusterHealthMonitor::RecordHeartbeat(const std::string& nodeId, uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.lastHeartbeatUs = nowUs;
    it->second.heartbeatAgeMs = 0;
    return true;
}

void ClusterHealthMonitor::UpdateCoordinatorStatus(const CoordinatorHealthRecord& record, uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinator = record;
    m_coordinator.lastPulseUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
}

double ClusterHealthMonitor::CalculateNodeScore(NodeHealthTelemetry& node, uint64_t nowUs) {
    double score = 100.0;
    std::string reasons = "";

    // 1. Heartbeat age
    if (nowUs >= node.lastHeartbeatUs) {
        node.heartbeatAgeMs = (nowUs - node.lastHeartbeatUs) / 1000;
    }
    if (node.heartbeatAgeMs > 5000) {
        score -= 50.0;
        reasons += "[HEARTBEAT_CRITICAL] ";
    } else if (node.heartbeatAgeMs > 2000) {
        score -= 20.0;
        reasons += "[HEARTBEAT_STALE] ";
    }

    // 2. CPU Usage
    if (node.cpuUsagePercent > 90.0) {
        score -= 25.0;
        reasons += "[HIGH_CPU] ";
    } else if (node.cpuUsagePercent > 75.0) {
        score -= 10.0;
    }

    // 3. RAM Usage
    if (node.memoryUsagePercent > 90.0) {
        score -= 20.0;
        reasons += "[HIGH_RAM] ";
    } else if (node.memoryUsagePercent > 80.0) {
        score -= 10.0;
    }

    // 4. CPU Temperature
    if (node.cpuTemperatureC > 80.0) {
        score -= 25.0;
        reasons += "[HIGH_TEMP] ";
    } else if (node.cpuTemperatureC > 70.0) {
        score -= 10.0;
    }

    // 5. Hardware Subsystems
    if (!node.gpioHealthy) {
        score -= 20.0;
        reasons += "[GPIO_FAULT] ";
    }
    if (!node.relayHealthy) {
        score -= 30.0;
        reasons += "[RELAY_FAULT] ";
    }
    if (!node.watchdogHealthy) {
        score -= 25.0;
        reasons += "[WATCHDOG_FAULT] ";
    }

    // 6. Network Quality
    if (node.networkLevel == aegis::hardware::LinkQualityLevel::CRITICAL) {
        score -= 40.0;
        reasons += "[NET_CRITICAL] ";
    } else if (node.networkLevel == aegis::hardware::LinkQualityLevel::POOR) {
        score -= 20.0;
        reasons += "[NET_POOR] ";
    } else if (node.networkLevel == aegis::hardware::LinkQualityLevel::FAIR) {
        score -= 10.0;
    }

    if (score < 0.0) score = 0.0;
    node.nodeScore = score;
    node.isDegraded = (score < 75.0);
    node.degradationReason = reasons;

    return score;
}

ClusterHealthSummary ClusterHealthMonitor::EvaluateCluster(uint64_t currentTimestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();

    ClusterHealthSummary summary;
    summary.timestampUs = nowUs;
    summary.totalNodes = m_nodes.size();
    summary.healthyNodes = 0;
    summary.degradedNodes = 0;
    summary.offlineNodes = 0;
    summary.coordinator = m_coordinator;

    double totalScoreWeighted = 0.0;
    double totalWeight = 0.0;

    for (auto& pair : m_nodes) {
        auto& node = pair.second;
        double s = CalculateNodeScore(node, nowUs);

        // Weight coordinator more heavily in cluster grade
        double weight = node.isCoordinator ? 2.0 : 1.0;
        totalScoreWeighted += (s * weight);
        totalWeight += weight;

        if (node.heartbeatAgeMs > 10000) {
            summary.offlineNodes++;
        } else if (node.isDegraded) {
            summary.degradedNodes++;
        } else {
            summary.healthyNodes++;
        }

        summary.nodeReports.push_back(node);
    }

    double finalScore = (totalWeight > 0.0) ? (totalScoreWeighted / totalWeight) : 100.0;
    summary.overallClusterScore = finalScore;

    if (finalScore >= 90.0 && summary.degradedNodes == 0) {
        summary.state = ClusterState::OPTIMAL;
    } else if (summary.degradedNodes > 0 || finalScore < 75.0) {
        summary.state = ClusterState::DEGRADED;
    } else {
        summary.state = ClusterState::OPERATIONAL;
    }

    m_cachedClusterScore = finalScore;
    m_cachedClusterState = summary.state;
    m_cachedDegradedNodeCount = summary.degradedNodes;
    return summary;
}

bool ClusterHealthMonitor::GetNodeTelemetry(const std::string& nodeId, NodeHealthTelemetry& outTelemetry) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it != m_nodes.end()) {
        outTelemetry = it->second;
        return true;
    }
    return false;
}

double ClusterHealthMonitor::GetClusterHealthScore() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedClusterScore;
}

bool ClusterHealthMonitor::IsClusterDegraded() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedClusterState == ClusterState::DEGRADED ||
           m_cachedClusterState == ClusterState::CRITICAL ||
           m_cachedDegradedNodeCount > 0;
}

CoordinatorHealthRecord ClusterHealthMonitor::GetCoordinatorStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinator;
}

std::string ClusterHealthMonitor::GenerateAsciiDashboard(uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();

    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - RASPBERRY PI CLUSTER HEALTH DASHBOARD\n"
        << "=========================================================================\n"
        << " Overall Cluster Score : " << std::fixed << std::setprecision(1) << m_cachedClusterScore
        << "% (" << ClusterStateToString(m_cachedClusterState) << ")\n"
        << " Coordinator Node      : " << m_coordinator.coordinatorId << " ["
        << CoordinatorModeToString(m_coordinator.mode) << "] | Corridors: "
        << m_coordinator.activeCorridors << " | Intersections: " << m_coordinator.activeIntersections << "\n"
        << "-------------------------------------------------------------------------\n"
        << " Node ID   | IP Address    | CPU%  | RAM%  | Temp  | Heartbeat | Score | State\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& pair : m_nodes) {
        const auto& n = pair.second;
        uint64_t hbAgeMs = n.heartbeatAgeMs;
        if (nowUs >= n.lastHeartbeatUs) {
            hbAgeMs = (nowUs - n.lastHeartbeatUs) / 1000;
        }

        oss << " " << std::left << std::setw(9) << n.nodeId.substr(0, 9) << " | "
            << std::setw(13) << n.ipAddress.substr(0, 13) << " | "
            << std::right << std::fixed << std::setprecision(1) << std::setw(4) << n.cpuUsagePercent << "% | "
            << std::setw(4) << n.memoryUsagePercent << "% | "
            << std::setw(4) << n.cpuTemperatureC << "C | "
            << std::setw(7) << hbAgeMs << "ms | "
            << std::setw(5) << n.nodeScore << " | "
            << (n.isDegraded ? "DEGRADED" : "HEALTHY") << "\n";
    }
    oss << "=========================================================================\n";
    return oss.str();
}

void ClusterHealthMonitor::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodes.clear();
    m_cachedClusterScore = 100.0;
    m_cachedClusterState = ClusterState::OPTIMAL;
}

} // namespace cluster
} // namespace aegis
