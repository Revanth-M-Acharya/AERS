/**
 * @file DistributedRuntimeOrchestrator.h
 * @brief SMART AEGIS (AERS) Phase 16C - Distributed Runtime Orchestrator.
 *
 * Master orchestration facade coordinating the Central Coordinator Runtime
 * and multiple distributed Edge Traffic Node Runtimes into one complete,
 * end-to-end cyber-physical execution pipeline.
 */

#ifndef AEGIS_RUNTIME_DISTRIBUTED_RUNTIME_ORCHESTRATOR_H
#define AEGIS_RUNTIME_DISTRIBUTED_RUNTIME_ORCHESTRATOR_H

#include "CoordinatorRuntime.h"
#include "TrafficNodeRuntime.h"
#include "RuntimeEventFlow.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>

namespace aegis {
namespace runtime {

/**
 * @brief Top-level Distributed Runtime Orchestration Facade.
 */
class DistributedRuntimeOrchestrator {
public:
    DistributedRuntimeOrchestrator();
    virtual ~DistributedRuntimeOrchestrator();

    // Non-copyable, non-movable
    DistributedRuntimeOrchestrator(const DistributedRuntimeOrchestrator&) = delete;
    DistributedRuntimeOrchestrator& operator=(const DistributedRuntimeOrchestrator&) = delete;
    DistributedRuntimeOrchestrator(DistributedRuntimeOrchestrator&&) = delete;
    DistributedRuntimeOrchestrator& operator=(DistributedRuntimeOrchestrator&&) = delete;

    // =========================================================================
    // Cluster Lifecycle Management (Task 1 & Task 7)
    // =========================================================================

    /**
     * @brief Initialize entire distributed cluster runtime.
     * @param coordinatorPort TCP listening port for Coordinator (default 9000).
     * @param nodeIds Vector of intersection identifiers to instantiate (default {"INT_A", "INT_B", "INT_C"}).
     * @return true if all nodes and coordinator initialized successfully.
     */
    bool InitializeCluster(uint16_t coordinatorPort = 9000,
                           const std::vector<std::string>& nodeIds = {"INT_A", "INT_B", "INT_C"});

    /**
     * @brief Start coordinator followed by edge intersection nodes in strict chronological order.
     * @return true if all nodes established connection and registered.
     */
    bool StartCluster();

    /**
     * @brief Gracefully terminate cluster in reverse chronological order.
     */
    void StopCluster();

    /**
     * @brief Check whether cluster is currently active and executing.
     */
    bool IsClusterRunning() const noexcept;

    // =========================================================================
    // End-to-End Emergency Execution (Task 4)
    // =========================================================================

    /**
     * @brief Execute full end-to-end emergency pipeline across coordinator and distributed edge nodes.
     * @param input Emergency call and patient vitals.
     * @return EmergencyPipelineResult Detailed telemetry and verification report.
     */
    EmergencyPipelineResult TriggerEmergency(const EmergencyPipelineInput& input);

    // =========================================================================
    // Fault Injection & Resilience Testing (Task 7 & 10)
    // =========================================================================

    /**
     * @brief Simulate network cable disconnect on an edge node.
     */
    bool SimulateNodeDisconnect(const std::string& nodeId);

    /**
     * @brief Reconnect previously disconnected edge node.
     */
    bool ReconnectNode(const std::string& nodeId);

    /**
     * @brief Check overall health across coordinator and all registered edge nodes.
     */
    bool IsClusterHealthy() const;

    // Subsystem Accessors
    CoordinatorRuntime& GetCoordinator() noexcept { return m_coordinator; }
    TrafficNodeRuntime* GetTrafficNode(const std::string& nodeId);
    std::vector<std::string> GetNodeIds() const;

    // Lower camelCase aliases for interoperability
    bool initialize(uint16_t port = 9000, const std::vector<std::string>& nodes = {"INT_A", "INT_B", "INT_C"}) {
        return InitializeCluster(port, nodes);
    }
    bool start() { return StartCluster(); }
    void stop() { StopCluster(); }
    bool isRunning() const noexcept { return IsClusterRunning(); }
    EmergencyPipelineResult triggerEmergency(const EmergencyPipelineInput& in) { return TriggerEmergency(in); }
    EmergencyPipelineResult executeEmergencyPipeline(const EmergencyPipelineInput& in) { return TriggerEmergency(in); }
    bool simulateNodeDisconnect(const std::string& id) { return SimulateNodeDisconnect(id); }
    bool reconnectNode(const std::string& id) { return ReconnectNode(id); }
    bool isClusterHealthy() const { return IsClusterHealthy(); }
    CoordinatorRuntime& getCoordinator() noexcept { return GetCoordinator(); }
    TrafficNodeRuntime* getTrafficNode(const std::string& id) { return GetTrafficNode(id); }
    std::vector<std::string> getNodeIds() const { return GetNodeIds(); }
    size_t getConnectedNodeCount() const { return m_coordinator.GetConnectedNodeCount(); }

private:
    std::atomic<bool> m_isInitialized{false};
    std::atomic<bool> m_isRunning{false};
    uint16_t m_coordinatorPort{9000};
    std::string m_coordinatorHost{"127.0.0.1"};
    std::vector<std::string> m_configuredNodeIds{};

    CoordinatorRuntime m_coordinator;
    std::unordered_map<std::string, std::unique_ptr<TrafficNodeRuntime>> m_trafficNodes;

    mutable std::mutex m_orchestratorMutex;
};

} // namespace runtime
} // namespace aegis

#endif // AEGIS_RUNTIME_DISTRIBUTED_RUNTIME_ORCHESTRATOR_H
