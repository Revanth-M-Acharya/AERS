/**
 * @file ClusterHealthMonitor.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Real-Time Raspberry Pi Cluster Health Monitor.
 *
 * Continuously supervises the operational telemetry of all Raspberry Pi 4 Model B SBCs
 * participating in the SMART AEGIS network (1 Coordinator node + 3 Intersection nodes):
 * - CPU usage (%), Memory usage (MB / %), CPU temperature (deg C)
 * - Heartbeat age and staleness tracking (ms)
 * - Wireless / Ethernet network link quality (5-tier QoS)
 * - GPIO pin reservation integrity & relay readback health
 * - Degraded node detection and fault threshold alerts
 * - Composite weighted cluster health score (0.0 - 100.0%)
 * - Continuous coordinator health and execution state supervision
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CLUSTER_HEALTH_MONITOR_HPP
#define AEGIS_CLUSTER_HEALTH_MONITOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iomanip>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"
#include "aegis/hardware/WirelessQualityMonitor.hpp"

namespace aegis {
namespace cluster {

/**
 * @brief High-level operational state of the SMART AEGIS cluster.
 */
enum class ClusterState {
    OPTIMAL,        ///< All nodes healthy; cluster score >= 90.0%
    OPERATIONAL,    ///< Minor warnings / nominal latency; score >= 75.0%
    DEGRADED,       ///< One or more nodes experiencing hardware/network degradation; score >= 50.0%
    CRITICAL        ///< Essential coordinator or multiple intersections offline; score < 50.0%
};

inline const char* ClusterStateToString(ClusterState state) noexcept {
    switch (state) {
        case ClusterState::OPTIMAL:     return "OPTIMAL";
        case ClusterState::OPERATIONAL: return "OPERATIONAL";
        case ClusterState::DEGRADED:    return "DEGRADED";
        case ClusterState::CRITICAL:    return "CRITICAL";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Operational state of the central cluster coordinator.
 */
enum class CoordinatorMode {
    ACTIVE,         ///< Nominal active master coordinator
    STANDBY,        ///< Hot standby ready to assume leadership
    DEGRADED,       ///< Coordinator experiencing internal subsystem warnings
    FAILSAFE        ///< Coordinator in fail-safe safe-mode
};

inline const char* CoordinatorModeToString(CoordinatorMode mode) noexcept {
    switch (mode) {
        case CoordinatorMode::ACTIVE:   return "ACTIVE";
        case CoordinatorMode::STANDBY:  return "STANDBY";
        case CoordinatorMode::DEGRADED: return "DEGRADED";
        case CoordinatorMode::FAILSAFE: return "FAILSAFE";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Telemetry snapshot for an individual Raspberry Pi node.
 */
struct NodeHealthTelemetry {
    std::string nodeId{""};
    std::string hostname{""};
    std::string ipAddress{""};
    bool isCoordinator{false};

    double cpuUsagePercent{0.0};
    double memoryUsagePercent{0.0};
    double memoryUsageMB{0.0};
    double cpuTemperatureC{42.0};

    uint64_t lastHeartbeatUs{0};
    uint64_t heartbeatAgeMs{0};

    aegis::hardware::LinkQualityLevel networkLevel{aegis::hardware::LinkQualityLevel::EXCELLENT};
    double networkRssi{-55.0};
    double packetLossPercent{0.0};

    bool gpioHealthy{true};
    bool relayHealthy{true};
    bool watchdogHealthy{true};

    bool isDegraded{false};
    std::string degradationReason{""};
    double nodeScore{100.0};                ///< Health score: 0.0 - 100.0
};

/**
 * @brief Continuous status report for the central coordinator.
 */
struct CoordinatorHealthRecord {
    std::string coordinatorId{"COORD_01"};
    CoordinatorMode mode{CoordinatorMode::ACTIVE};
    size_t activeCorridors{0};
    size_t activeIntersections{0};
    uint64_t uptimeSeconds{0};
    uint64_t lastPulseUs{0};
};

/**
 * @brief Composite cluster health report.
 */
struct ClusterHealthSummary {
    uint64_t timestampUs{0};
    size_t totalNodes{0};
    size_t healthyNodes{0};
    size_t degradedNodes{0};
    size_t offlineNodes{0};
    double overallClusterScore{100.0};
    ClusterState state{ClusterState::OPTIMAL};
    CoordinatorHealthRecord coordinator{};
    std::vector<NodeHealthTelemetry> nodeReports{};

    std::string ToJson() const;
};

/**
 * @class ClusterHealthMonitor
 * @brief Thread-safe aggregator and evaluator of cluster-wide Raspberry Pi health telemetry.
 */
class ClusterHealthMonitor {
public:
    ClusterHealthMonitor();
    ~ClusterHealthMonitor() = default;

    ClusterHealthMonitor(const ClusterHealthMonitor&) = delete;
    ClusterHealthMonitor& operator=(const ClusterHealthMonitor&) = delete;
    ClusterHealthMonitor(ClusterHealthMonitor&&) = delete;
    ClusterHealthMonitor& operator=(ClusterHealthMonitor&&) = delete;

    /**
     * @brief Registers a Raspberry Pi node in the cluster monitor.
     */
    bool RegisterNode(const std::string& nodeId,
                      const std::string& hostname,
                      const std::string& ipAddress,
                      bool isCoordinator = false);

    /**
     * @brief Ingests telemetry metrics from a specific node.
     */
    bool UpdateNodeTelemetry(const NodeHealthTelemetry& telemetry, uint64_t timestampUs = 0);

    /**
     * @brief Records a heartbeat pulse from a node.
     */
    bool RecordHeartbeat(const std::string& nodeId, uint64_t timestampUs = 0);

    /**
     * @brief Updates coordinator operational status.
     */
    void UpdateCoordinatorStatus(const CoordinatorHealthRecord& record, uint64_t timestampUs = 0);

    /**
     * @brief Evaluates health scores across all nodes and derives overall cluster grade.
     * @param currentTimestampUs QNX monotonic clock in microseconds.
     * @return Summary of cluster health.
     */
    ClusterHealthSummary EvaluateCluster(uint64_t currentTimestampUs = 0);

    /**
     * @brief Queries health telemetry of a specific node.
     */
    bool GetNodeTelemetry(const std::string& nodeId, NodeHealthTelemetry& outTelemetry) const;

    /**
     * @brief Returns current composite cluster health score (0.0 - 100.0%).
     */
    double GetClusterHealthScore() const;

    /**
     * @brief Checks if any node in the cluster is degraded or offline.
     */
    bool IsClusterDegraded() const;

    /**
     * @brief Retrieves latest coordinator status.
     */
    CoordinatorHealthRecord GetCoordinatorStatus() const;

    /**
     * @brief Generates an ASCII diagnostic dashboard of the cluster.
     */
    std::string GenerateAsciiDashboard(uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Clears all internal tracking state.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;
    double CalculateNodeScore(NodeHealthTelemetry& node, uint64_t nowUs);

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, NodeHealthTelemetry> m_nodes;
    CoordinatorHealthRecord m_coordinator{};
    double m_cachedClusterScore{100.0};
    ClusterState m_cachedClusterState{ClusterState::OPTIMAL};
    size_t m_cachedDegradedNodeCount{0};
};

} // namespace cluster
} // namespace aegis

#endif // AEGIS_CLUSTER_HEALTH_MONITOR_HPP
