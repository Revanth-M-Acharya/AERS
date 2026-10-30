/**
 * @file TrafficNetworkOrchestrator.h
 * @brief SMART AEGIS (AERS) Phase 13A - City-Wide Distributed Traffic Network Orchestration Engine
 *
 * Implements the municipal traffic execution layer for SMART AEGIS.
 * While MissionExecutionEngine decides WHAT should happen and IncidentCommandEngine
 * decides WHY it should happen, TrafficNetworkOrchestrator decides HOW the entire
 * city's intersections execute the mission.
 *
 * Maintains every intersection in the city, topology and adjacency graphs, corridor membership,
 * signal execution states, synchronization groups, green wave coordination, distributed execution,
 * leader election, heartbeat monitoring, offline detection, node recovery, and execution rollbacks.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_TRAFFIC_NETWORK_ORCHESTRATOR_H
#define AEGIS_TRAFFIC_NETWORK_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace traffic_network {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Operational communication and hardware status of an intersection node.
 */
enum class IntersectionStatus {
    ONLINE,       ///< Node connected, healthy, and communicating with central coordinator
    OFFLINE,      ///< Heartbeat timed out or communication link lost
    FAILED,       ///< Hardware lamp error, controller fault, or fatal actuation failure
    RECOVERING,   ///< Re-establishing connection or completing safe clearance cycle
    MAINTENANCE   ///< Manually flagged for field technician testing or maintenance
};

/**
 * @brief Detailed execution state of an intersection's traffic signal controller.
 */
enum class SignalExecutionState {
    IDLE,         ///< Normal civil cyclical traffic signal operation
    WAITING,      ///< Scheduled for upcoming emergency corridor preemption
    PREPARING,    ///< Transitioning to yellow clearance / all-red preemption safety buffer
    GREEN,        ///< Active green preemption window granted to emergency vehicle
    TRANSITION,   ///< Progressing to the next signal phase or downstream junction
    RESTORING,    ///< Post-clearance recovery cycle restoring civilian coordination
    FAILED        ///< Execution aborted or signal command rejected
};

/**
 * @brief Coordination and synchronization state of a distributed corridor or group.
 */
enum class SynchronizationState {
    READY,        ///< Group configured and ready for execution
    EXECUTING,    ///< Green wave actively propagating across member intersections
    WAITING,      ///< Waiting for upstream arrival or timing offset synchronization
    COMPLETED,    ///< Green wave window successfully finalized across all members
    FAILED        ///< Synchronization aborted due to node timeout or signal conflict
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(IntersectionStatus status);
std::string ToString(SignalExecutionState state);
std::string ToString(SynchronizationState state);

IntersectionStatus IntersectionStatusFromString(const std::string& str);
SignalExecutionState SignalExecutionStateFromString(const std::string& str);
SynchronizationState SynchronizationStateFromString(const std::string& str);

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Comprehensive data model representing an edge intersection node.
 */
struct IntersectionNode {
    std::string intersectionId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    IntersectionStatus status{IntersectionStatus::ONLINE};
    std::string controllerId;
    bool online{true};
    uint64_t lastHeartbeat{0};
    std::string currentSignalPhase{"RED"};
    std::string currentMission;
    std::string currentCorridor;
    double executionLatency{0.0}; ///< Milliseconds
    uint32_t queueLength{0};
    uint32_t priority{1};         ///< Higher value = higher leader election priority
    double healthScore{100.0};    ///< 0.0 to 100.0
    SignalExecutionState signalState{SignalExecutionState::IDLE};
    std::vector<std::string> neighbors; ///< Adjacency topology

    IntersectionNode() = default;

    IntersectionNode(const std::string& id,
                     const std::string& nodeName = "",
                     double lat = 0.0,
                     double lon = 0.0,
                     const std::string& ctrlId = "")
        : intersectionId(id),
          name(nodeName.empty() ? id : nodeName),
          latitude(lat),
          longitude(lon),
          controllerId(ctrlId.empty() ? ("CTRL_" + id) : ctrlId) {}
};

/**
 * @brief Represents an end-to-end emergency traffic corridor across intersections.
 */
struct TrafficCorridor {
    std::string corridorId;
    std::string missionId;
    std::vector<std::string> intersections; ///< Ordered junction sequence
    uint32_t version{1};
    uint64_t creationTime{0};
    uint64_t activationTime{0};
    uint64_t completionTime{0};
    bool active{false};
    std::string leaderIntersection;
    SignalExecutionState state{SignalExecutionState::IDLE};

    TrafficCorridor() = default;

    TrafficCorridor(const std::string& id,
                    const std::string& missId = "",
                    const std::vector<std::string>& nodes = {})
        : corridorId(id),
          missionId(missId),
          intersections(nodes) {}
};

/**
 * @brief Manages a synchronization cluster of intersections with a designated leader.
 */
struct SynchronizationGroup {
    std::string groupId;
    std::vector<std::string> intersections;
    std::string leaderIntersection;
    SynchronizationState state{SynchronizationState::READY};
    uint64_t lastSyncTimestamp{0};

    SynchronizationGroup() = default;

    SynchronizationGroup(const std::string& id,
                         const std::vector<std::string>& nodes = {},
                         const std::string& leader = "")
        : groupId(id),
          intersections(nodes),
          leaderIntersection(leader.empty() ? (nodes.empty() ? "" : nodes.front()) : leader) {}
};

/**
 * @brief Aggregated network statistics across the municipal traffic grid.
 */
struct TrafficNetworkStatistics {
    uint32_t totalIntersections{0};
    uint32_t onlineIntersections{0};
    uint32_t offlineIntersections{0};
    uint32_t activeCorridors{0};
    uint32_t executingSignals{0};
    double averageExecutionLatency{0.0}; ///< Milliseconds
    double averageHeartbeat{0.0};        ///< Milliseconds interval / freshness
    double greenWaveSuccessRate{100.0};  ///< Percentage (0.0 to 100.0)
    uint32_t failedSynchronizations{0};
    double averageRecoveryTime{0.0};     ///< Milliseconds
};

// =============================================================================
// TrafficNetworkOrchestrator Class
// =============================================================================

/**
 * @brief City-Wide Distributed Traffic Network Orchestration Engine.
 */
class TrafficNetworkOrchestrator {
public:
    TrafficNetworkOrchestrator();
    ~TrafficNetworkOrchestrator() = default;

    // Non-copyable, non-movable to guarantee thread safety
    TrafficNetworkOrchestrator(const TrafficNetworkOrchestrator&) = delete;
    TrafficNetworkOrchestrator& operator=(const TrafficNetworkOrchestrator&) = delete;
    TrafficNetworkOrchestrator(TrafficNetworkOrchestrator&&) = delete;
    TrafficNetworkOrchestrator& operator=(TrafficNetworkOrchestrator&&) = delete;

    // -------------------------------------------------------------------------
    // Intersection Lifecycle & Topology Management
    // -------------------------------------------------------------------------
    bool RegisterIntersection(const IntersectionNode& node);
    bool RemoveIntersection(const std::string& intersectionId);
    bool UpdateHeartbeat(const std::string& intersectionId, uint64_t timestamp = 0);
    bool SetIntersectionOnline(const std::string& intersectionId);
    bool SetIntersectionOffline(const std::string& intersectionId);
    bool AssignMission(const std::string& intersectionId, const std::string& missionId);
    bool AddNeighbor(const std::string& intersectionId, const std::string& neighborId);
    std::vector<std::string> GetNeighbors(const std::string& intersectionId) const;

    // -------------------------------------------------------------------------
    // Corridor Lifecycle & Membership
    // -------------------------------------------------------------------------
    bool CreateCorridor(const TrafficCorridor& corridor);
    bool ActivateCorridor(const std::string& corridorId);
    bool DeactivateCorridor(const std::string& corridorId);
    bool AssignIntersectionToCorridor(const std::string& corridorId, const std::string& intersectionId);
    bool RemoveIntersectionFromCorridor(const std::string& corridorId, const std::string& intersectionId);

    // -------------------------------------------------------------------------
    // Synchronization Groups & Distributed Execution
    // -------------------------------------------------------------------------
    bool CreateSynchronizationGroup(const std::string& groupId,
                                   const std::vector<std::string>& intersections,
                                   const std::string& leader = "");
    bool AddIntersectionToGroup(const std::string& groupId, const std::string& intersectionId);
    bool RemoveIntersectionFromGroup(const std::string& groupId, const std::string& intersectionId);
    std::string ElectLeader(const std::string& groupId);

    // -------------------------------------------------------------------------
    // Green Wave Execution, Restoration & Rollback
    // -------------------------------------------------------------------------
    bool ExecuteGreenWave(const std::string& corridorId);
    bool ExecuteIntersection(const std::string& intersectionId, SignalExecutionState state, double latencyMs = 0.0);
    bool RestoreIntersection(const std::string& intersectionId);
    bool RestoreCorridor(const std::string& corridorId);
    bool RollbackExecution(const std::string& corridorId);
    bool AcknowledgeExecution(const std::string& intersectionId, const std::string& corridorId, bool success);

    // -------------------------------------------------------------------------
    // Monitoring & Automatic Fault Recovery
    // -------------------------------------------------------------------------
    uint32_t CheckHeartbeats(uint64_t timeoutMs = 5000);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------
    bool GetIntersection(const std::string& intersectionId, IntersectionNode& outNode) const;
    std::vector<IntersectionNode> GetAllIntersections() const;
    bool GetCorridor(const std::string& corridorId, TrafficCorridor& outCorridor) const;
    std::vector<TrafficCorridor> GetAllCorridors() const;
    bool GetSynchronizationGroup(const std::string& groupId, SynchronizationGroup& outGroup) const;
    std::vector<SynchronizationGroup> GetAllSynchronizationGroups() const;

    // -------------------------------------------------------------------------
    // Reports & Statistics
    // -------------------------------------------------------------------------
    std::string GenerateNetworkReport() const;
    std::string GenerateCorridorReport(const std::string& corridorId) const;
    std::string GenerateSynchronizationReport(const std::string& groupId) const;
    std::string GenerateTopologyReport() const;
    TrafficNetworkStatistics GenerateStatistics() const;
    TrafficNetworkStatistics GetStatistics() const;

    // -------------------------------------------------------------------------
    // Lifecycle Reset & Cleanup
    // -------------------------------------------------------------------------
    void Reset();
    void ClearHistory();

private:
    static uint64_t GetCurrentTimeMs();

    // Dedicated Mutex Domains per Specification:
    // registry, corridors, groups, reports, statistics, heartbeats, execution
    mutable std::mutex registryMutex_;
    mutable std::mutex corridorsMutex_;
    mutable std::mutex groupsMutex_;
    mutable std::mutex reportsMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex heartbeatsMutex_;
    mutable std::mutex executionMutex_;

    // Core Registries
    std::unordered_map<std::string, IntersectionNode> intersections_;
    std::unordered_map<std::string, TrafficCorridor> corridors_;
    std::unordered_map<std::string, SynchronizationGroup> groups_;

    // Metrics Tracking
    mutable TrafficNetworkStatistics statistics_;
    double totalExecutionLatencyMs_{0.0};
    uint64_t latencyMeasurementCount_{0};
    double totalRecoveryTimeMs_{0.0};
    uint64_t recoveryCount_{0};
    uint32_t totalGreenWavesExecuted_{0};
    uint32_t successfulGreenWaves_{0};
};

} // namespace traffic_network
} // namespace aegis

#endif // AEGIS_TRAFFIC_NETWORK_ORCHESTRATOR_H

