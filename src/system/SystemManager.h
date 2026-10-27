/**
 * @file SystemManager.h
 * @brief System Integration Layer for SMART AEGIS.
 *
 * Coordinates system-wide lifecycle across the Central Coordinator (Raspberry Pi 1)
 * and three Intersection Nodes:
 * - Raspberry Pi 2: Intersection Node A (INT_A)
 * - Raspberry Pi 3: Intersection Node B (INT_B)
 * - Raspberry Pi 4: Intersection Node C (INT_C)
 *
 * Manages ordered startup sequencing, registration verification, keepalive monitoring,
 * dynamic Green Corridor route activation & progression, and fault-tolerant reconnection.
 */

#ifndef AEGIS_SYSTEM_SYSTEM_MANAGER_H
#define AEGIS_SYSTEM_SYSTEM_MANAGER_H

#include "coordinator/CoordinatorManager.h"
#include "intersection/IntersectionNode.h"
#include "intersection/SignalController.h"
#include "communication/Message.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace aegis {
namespace system {

/**
 * @brief Detailed health snapshot for a single intersection node.
 */
struct NodeHealthStatus {
    std::string nodeId{""};
    bool connected{false};
    bool registered{false};
    bool heartbeatActive{false};
    bool coordinatorOnline{false};
    uint64_t lastHeartbeatTimestampMs{0};
    intersection::SignalState signalState{intersection::SignalState::NORMAL};
};

/**
 * @brief Comprehensive system health report across coordinator and all nodes.
 */
struct SystemHealthReport {
    bool coordinatorRunning{false};
    size_t totalConfiguredNodes{0};
    size_t registeredNodesCount{0};
    size_t onlineNodesCount{0};
    bool isFullyHealthy{false};
    std::vector<NodeHealthStatus> nodeStatuses{};
};

/**
 * @brief System Integration Manager governing Coordinator and 3 Intersection Nodes.
 */
class SystemManager {
public:
    /**
     * @brief Configuration parameters for system integration.
     */
    struct Config {
        std::string coordinatorHost{"127.0.0.1"};
        uint16_t coordinatorPort{19600};
        std::vector<std::string> nodeIds{"INT_A", "INT_B", "INT_C"};
        uint32_t heartbeatIntervalMs{200}; // 200ms default for fast, responsive beacons
        uint32_t connectTimeoutMs{3000};
        uint32_t ackTimeoutMs{3000};
        uint32_t startupTimeoutMs{5000};
    };

    SystemManager();
    explicit SystemManager(Config config);

    /**
     * @brief Destructor guarantees clean, orderly shutdown of all nodes and coordinator.
     */
    virtual ~SystemManager();

    // Non-copyable, non-movable
    SystemManager(const SystemManager&) = delete;
    SystemManager& operator=(const SystemManager&) = delete;
    SystemManager(SystemManager&&) = delete;
    SystemManager& operator=(SystemManager&&) = delete;

    // =========================================================================
    // 1. System Lifecycle & Startup Sequencing
    // =========================================================================

    /**
     * @brief Execute full automated startup sequence:
     *        1. Start Central Coordinator server.
     *        2. Start and connect all Intersection Nodes (Nodes A, B, C).
     *        3. Verify registration ACK handshake on all nodes.
     *        4. Wait until all nodes are confirmed registered and online.
     * @param timeoutMs Maximum duration to await complete system convergence.
     * @return true if all nodes started, connected, registered, and began heartbeats.
     */
    bool StartSystem(uint32_t timeoutMs = 5000);

    /**
     * @brief Cleanly stop all intersection nodes first, then stop Central Coordinator.
     */
    void StopSystem();

    /**
     * @brief Check if the system is currently active and running.
     */
    bool IsRunning() const noexcept;

    /**
     * @brief Start only the Central Coordinator subsystem.
     * @return true if coordinator server successfully bound and worker started.
     */
    bool StartCoordinator();

    /**
     * @brief Stop only the Central Coordinator subsystem.
     */
    void StopCoordinator();

    /**
     * @brief Start all configured intersection nodes and initiate connection to coordinator.
     * @return true if all nodes launched their connection attempts.
     */
    bool StartIntersectionNodes();

    /**
     * @brief Stop all configured intersection nodes.
     */
    void StopIntersectionNodes();

    /**
     * @brief Block until all configured intersection nodes are verified online in coordinator.
     * @param timeoutMs Timeout in milliseconds.
     * @return true if all nodes registered before timeout; false otherwise.
     */
    bool WaitForAllNodesRegistered(uint32_t timeoutMs = 5000);

    // =========================================================================
    // 2. Health Monitoring & Status Inspection
    // =========================================================================

    /**
     * @brief Inspect and generate a complete health report of coordinator and all nodes.
     */
    SystemHealthReport CheckSystemHealth() const;

    /**
     * @brief Evaluate if the entire distributed system is in a fully healthy, connected state.
     */
    bool IsSystemHealthy() const;

    // =========================================================================
    // 3. Green Corridor Orchestration
    // =========================================================================

    /**
     * @brief Configure a green corridor route across an ordered list of intersections.
     * @param corridorId Identifier for the corridor (e.g. "CORR_AMB_01").
     * @param route Ordered sequence of intersection IDs (e.g. {"INT_A", "INT_B", "INT_C"}).
     * @return true if route configured successfully.
     */
    bool CreateCorridorRoute(const std::string& corridorId, const std::vector<std::string>& route);

    /**
     * @brief Activate green corridor preemption and dispatch commands to nodes.
     *        Active node becomes GREEN; other route nodes become RED.
     * @return true if corridor activated and commands dispatched.
     */
    bool ActivateCorridor();

    /**
     * @brief Progress corridor to next intersection down the path and dispatch commands.
     * @return true if corridor advanced and commands dispatched; false if end of route.
     */
    bool AdvanceCorridor();

    /**
     * @brief Conclude active corridor and restore all nodes to NORMAL cycling phase.
     * @return true if corridor completed and reset commands dispatched.
     */
    bool CompleteCorridor();

    /**
     * @brief Abort active corridor and restore all nodes to NORMAL cycling phase.
     * @return true if corridor cancelled and reset commands dispatched.
     */
    bool CancelCorridor();

    /**
     * @brief Pop all buffered commands from coordinator and transmit them to target nodes.
     * @return Number of commands successfully dispatched.
     */
    size_t DispatchGeneratedCommands();

    /**
     * @brief Verify that expected node is GREEN and all expected red nodes are RED.
     * @param expectedGreenNodeId Node ID that should be GREEN.
     * @param expectedRedNodeIds List of node IDs that should be RED.
     * @param timeoutMs Duration to await state convergence.
     * @return true if states match before timeout.
     */
    bool VerifyCorridorPhasing(const std::string& expectedGreenNodeId,
                               const std::vector<std::string>& expectedRedNodeIds,
                               uint32_t timeoutMs = 2000);

    /**
     * @brief Verify that all nodes have returned to NORMAL cycling state.
     * @param timeoutMs Duration to await state convergence.
     * @return true if all nodes are NORMAL before timeout.
     */
    bool VerifyAllNodesNormal(uint32_t timeoutMs = 2000);

    // =========================================================================
    // 4. Fault Injection & Recovery Verification
    // =========================================================================

    /**
     * @brief Simulate network drop on a specific intersection node.
     * @param nodeId Node ID to disconnect.
     * @return true if node found and disconnected.
     */
    bool DisconnectNode(const std::string& nodeId);

    /**
     * @brief Force explicit reconnection and re-registration on a disconnected node.
     * @param nodeId Node ID to reconnect.
     * @param timeoutMs Connection and registration timeout.
     * @return true if reconnected and re-registered.
     */
    bool ReconnectNode(const std::string& nodeId, uint32_t timeoutMs = 3000);

    /**
     * @brief Block until a node is marked online in the Coordinator's registry.
     * @param nodeId Node ID to monitor.
     * @param timeoutMs Timeout in milliseconds.
     * @return true if online before timeout.
     */
    bool WaitForNodeOnline(const std::string& nodeId, uint32_t timeoutMs = 3000);

    /**
     * @brief Block until a node is marked offline in the Coordinator's registry.
     * @param nodeId Node ID to monitor.
     * @param timeoutMs Timeout in milliseconds.
     * @return true if offline before timeout.
     */
    bool WaitForNodeOffline(const std::string& nodeId, uint32_t timeoutMs = 3000);

    // =========================================================================
    // 5. Hardware Architecture Direct Accessors
    // =========================================================================

    /**
     * @brief Access Central Coordinator (Raspberry Pi 1).
     */
    coordinator::CoordinatorManager& GetCoordinator() noexcept;
    const coordinator::CoordinatorManager& GetCoordinator() const noexcept;

    /**
     * @brief Access Intersection Node A (Raspberry Pi 2).
     */
    intersection::IntersectionNode* GetNodeA();

    /**
     * @brief Access Intersection Node B (Raspberry Pi 3).
     */
    intersection::IntersectionNode* GetNodeB();

    /**
     * @brief Access Intersection Node C (Raspberry Pi 4).
     */
    intersection::IntersectionNode* GetNodeC();

    /**
     * @brief Access an intersection node by its ID.
     */
    intersection::IntersectionNode* GetIntersectionNode(const std::string& nodeId);
    const intersection::IntersectionNode* GetIntersectionNode(const std::string& nodeId) const;

    /**
     * @brief Access all managed intersection nodes.
     */
    std::vector<std::shared_ptr<intersection::IntersectionNode>> GetAllNodes() const;

    /**
     * @brief Access system configuration.
     */
    const Config& GetConfig() const noexcept { return m_config; }

private:
    void SetupCoordinatorEventHook();

    Config m_config;
    std::atomic<bool> m_running{false};

    std::unique_ptr<coordinator::CoordinatorManager> m_coordinator;
    std::vector<std::shared_ptr<intersection::IntersectionNode>> m_nodes;

    mutable std::mutex m_stateMutex{};
    mutable std::mutex m_syncMutex{};
    mutable std::condition_variable m_syncCv{};
    uint64_t m_lastEventTimestampMs{0};
};

} // namespace system
} // namespace aegis

#endif // AEGIS_SYSTEM_SYSTEM_MANAGER_H
