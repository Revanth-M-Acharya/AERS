/**
 * @file RaspberryPiClusterManager.h
 * @brief SMART AEGIS (AERS) Phase 14A - Raspberry Pi Cluster Deployment & Health Management Engine
 *
 * Implements the edge node deployment, health monitoring, synchronization, configuration management,
 * heartbeat supervision, firmware tracking, node discovery, and fault recovery of every Raspberry Pi 4
 * participating in the SMART AEGIS network.
 *
 * Sits immediately after MissionExecutionEngine (12A), IncidentCommandEngine (12B),
 * TrafficNetworkOrchestrator (13A), and DistributedConsensusEngine (13B), orchestrating
 * physical edge Raspberry Pi hardware controllers.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 Model B & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_RASPBERRY_PI_CLUSTER_MANAGER_H
#define AEGIS_RASPBERRY_PI_CLUSTER_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace cluster {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Operational health state of a Raspberry Pi node.
 */
enum class NodeHealth {
    HEALTHY,
    WARNING,
    CRITICAL,
    OFFLINE
};

/**
 * @brief Edge deployment lifecycle state.
 */
enum class DeploymentState {
    NOT_DEPLOYED,
    DEPLOYING,
    DEPLOYED,
    FAILED,
    RECOVERING
};

/**
 * @brief Configuration and clock synchronization state.
 */
enum class SyncState {
    NOT_SYNCED,
    SYNCING,
    SYNCED
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(NodeHealth health);
std::string ToString(DeploymentState state);
std::string ToString(SyncState state);

NodeHealth NodeHealthFromString(const std::string& str);
DeploymentState DeploymentStateFromString(const std::string& str);
SyncState SyncStateFromString(const std::string& str);

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Complete data model representing a physical Raspberry Pi 4 node.
 */
struct RaspberryPiNode {
    std::string nodeId;
    std::string hostname;
    std::string ipAddress;
    std::string macAddress;
    std::string firmwareVersion{"1.0.0"};
    std::string softwareVersion{"1.0.0"};
    DeploymentState deploymentState{DeploymentState::NOT_DEPLOYED};
    SyncState syncState{SyncState::NOT_SYNCED};
    NodeHealth nodeHealth{NodeHealth::HEALTHY};
    double cpuUsage{0.0};            ///< Percentage (0.0 to 100.0)
    double ramUsage{0.0};            ///< Percentage (0.0 to 100.0)
    double storageUsage{0.0};        ///< Percentage (0.0 to 100.0)
    double cpuTemperature{45.0};     ///< Celsius
    uint64_t uptime{0};              ///< Seconds
    double latency{0.0};             ///< Milliseconds
    uint64_t lastHeartbeat{0};       ///< Milliseconds timestamp
    bool online{true};
    std::string assignedIntersection;
    std::string assignedMission;
    std::string lastConfigPayload;
    uint32_t configVersion{0};

    RaspberryPiNode() = default;

    RaspberryPiNode(const std::string& id,
                    const std::string& host = "",
                    const std::string& ip = "",
                    const std::string& mac = "")
        : nodeId(id),
          hostname(host.empty() ? ("rpi-" + id) : host),
          ipAddress(ip.empty() ? "127.0.0.1" : ip),
          macAddress(mac.empty() ? "B8:27:EB:00:00:00" : mac) {}
};

/**
 * @brief Aggregated operational statistics across the Raspberry Pi cluster.
 */
struct ClusterStatistics {
    uint32_t totalNodes{0};
    uint32_t onlineNodes{0};
    uint32_t offlineNodes{0};
    uint32_t healthyNodes{0};
    uint32_t warningNodes{0};
    uint32_t criticalNodes{0};
    uint32_t deployedNodes{0};
    uint32_t synchronizedNodes{0};
    double averageLatency{0.0};      ///< Milliseconds
    double averageTemperature{0.0};  ///< Celsius
};

// =============================================================================
// RaspberryPiClusterManager Class
// =============================================================================

/**
 * @brief Master Cluster Deployment & Health Management Engine for SMART AEGIS.
 */
class RaspberryPiClusterManager {
public:
    // Operational thresholds
    static constexpr double CRITICAL_TEMP_THRESHOLD_C = 80.0;
    static constexpr double WARNING_CPU_THRESHOLD_PCT = 85.0;
    static constexpr double WARNING_RAM_THRESHOLD_PCT = 85.0;
    static constexpr double WARNING_STORAGE_THRESHOLD_PCT = 90.0;
    static constexpr uint64_t DEFAULT_HEARTBEAT_TIMEOUT_MS = 5000;

    RaspberryPiClusterManager();
    ~RaspberryPiClusterManager() = default;

    // Non-copyable, non-movable to guarantee thread safety
    RaspberryPiClusterManager(const RaspberryPiClusterManager&) = delete;
    RaspberryPiClusterManager& operator=(const RaspberryPiClusterManager&) = delete;
    RaspberryPiClusterManager(RaspberryPiClusterManager&&) = delete;
    RaspberryPiClusterManager& operator=(RaspberryPiClusterManager&&) = delete;

    // -------------------------------------------------------------------------
    // Node Lifecycle & Discovery
    // -------------------------------------------------------------------------
    bool RegisterNode(const RaspberryPiNode& node);
    bool RemoveNode(const std::string& nodeId);
    bool DiscoverNode(const std::string& nodeId,
                      const std::string& ipAddress,
                      const std::string& hostname = "");

    // -------------------------------------------------------------------------
    // Heartbeat & Telemetry Supervision
    // -------------------------------------------------------------------------
    bool UpdateHeartbeat(const std::string& nodeId, uint64_t timestamp = 0);
    bool UpdateHealth(const std::string& nodeId, NodeHealth health);
    bool UpdateTemperature(const std::string& nodeId, double temperatureCelsius);
    bool UpdateCPUUsage(const std::string& nodeId, double cpuUsagePercent);
    bool UpdateRAMUsage(const std::string& nodeId, double ramUsagePercent);
    bool UpdateStorageUsage(const std::string& nodeId, double storageUsagePercent);
    bool UpdateLatency(const std::string& nodeId, double latencyMs);
    std::vector<std::string> DetectOfflineNodes(uint64_t currentTime,
                                               uint64_t timeoutThresholdMs = DEFAULT_HEARTBEAT_TIMEOUT_MS);

    // -------------------------------------------------------------------------
    // Deployment & Synchronization Management
    // -------------------------------------------------------------------------
    bool UpdateDeploymentState(const std::string& nodeId, DeploymentState state);
    bool UpdateSynchronizationState(const std::string& nodeId, SyncState state);
    bool AssignIntersection(const std::string& nodeId, const std::string& intersectionId);
    bool AssignMission(const std::string& nodeId, const std::string& missionId);
    bool RecoverNode(const std::string& nodeId, uint64_t timestamp = 0);
    bool PushConfiguration(const std::string& nodeId, const std::string& configPayload);
    bool VerifyClusterReadiness() const;

    // -------------------------------------------------------------------------
    // Node Queries
    // -------------------------------------------------------------------------
    bool GetNode(const std::string& nodeId, RaspberryPiNode& outNode) const;
    std::vector<RaspberryPiNode> GetAllNodes() const;
    std::vector<RaspberryPiNode> GetHealthyNodes() const;
    std::vector<RaspberryPiNode> GetOfflineNodes() const;

    // -------------------------------------------------------------------------
    // Structured ASCII Reports
    // -------------------------------------------------------------------------
    std::string GenerateClusterHealthReport() const;
    std::string GenerateDeploymentReport() const;
    std::string GenerateNodeReport(const std::string& nodeId) const;

    // -------------------------------------------------------------------------
    // Statistics & Lifecycle
    // -------------------------------------------------------------------------
    ClusterStatistics GetStatistics() const;
    void Reset();

private:
    static uint64_t GetCurrentTimeMs();

    // Seven dedicated mutexes as strictly specified (no nested locking):
    mutable std::mutex registryMutex_;
    mutable std::mutex heartbeatMutex_;
    mutable std::mutex healthMutex_;
    mutable std::mutex deploymentMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex configurationMutex_;

    // Data protected by registryMutex_:
    std::unordered_map<std::string, RaspberryPiNode> nodes_;

    // Data protected by heartbeatMutex_:
    std::unordered_map<std::string, uint64_t> lastHeartbeats_;

    // Data protected by healthMutex_:
    std::unordered_map<std::string, double> temperatures_;
    std::unordered_map<std::string, double> cpuUsages_;
    std::unordered_map<std::string, double> ramUsages_;
    std::unordered_map<std::string, double> storageUsages_;
    std::unordered_map<std::string, double> latencies_;

    // Data protected by deploymentMutex_:
    std::unordered_map<std::string, DeploymentState> deploymentStates_;
    std::unordered_map<std::string, SyncState> syncStates_;

    // Data protected by statisticsMutex_:
    ClusterStatistics statistics_;
    double totalLatencySum_{0.0};
    uint32_t totalLatencySamples_{0};
    double totalTemperatureSum_{0.0};
    uint32_t totalTemperatureSamples_{0};

    // Data protected by configurationMutex_:
    std::unordered_map<std::string, std::string> configurations_;
    std::unordered_map<std::string, uint32_t> configVersions_;
};

} // namespace cluster

// Namespace alias for rpi_cluster
namespace rpi_cluster = cluster;

} // namespace aegis

#endif // AEGIS_RASPBERRY_PI_CLUSTER_MANAGER_H

