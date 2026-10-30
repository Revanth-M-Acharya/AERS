/**
 * @file CoordinatorRuntime.h
 * @brief SMART AEGIS (AERS) Phase 16C - Central Coordinator Distributed Runtime Engine.
 *
 * Integrates every coordinator-side subsystem into one unified, thread-safe,
 * deterministic execution runtime for the Central Municipal Coordinator (Raspberry Pi 1 / QNX RTOS).
 */

#ifndef AEGIS_RUNTIME_COORDINATOR_RUNTIME_H
#define AEGIS_RUNTIME_COORDINATOR_RUNTIME_H

#include "RuntimeEventFlow.h"

// Core Coordinator Subsystems
#include "aegis/core/emergency_call_handler.hpp"
#include "medical/PriorityEngine.h"
#include "incident_command/IncidentCommandEngine.h"
#include "fleet_management/FleetManagementEngine.h"
#include "aegis/core/route_planner.hpp"
#include "replanning/DynamicRouteReplanner.h"
#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "scheduler/PriorityScheduler.h"
#include "multi_corridor/MultiCorridorCoordinator.h"
#include "coordinator/CoordinatorManager.h"
#include "mission_execution/MissionExecutionEngine.h"
#include "hospital_management/HospitalResourceEngine.h"
#include "hospital_selection/HospitalSelectionEngine.h"
#include "traffic_network_orchestrator/TrafficNetworkOrchestrator.h"
#include "distributed_consensus/DistributedConsensusEngine.h"
#include "rpi_cluster/RaspberryPiClusterManager.h"
#include "system_diagnostics/SystemDiagnosticsEngine.h"
#include "logger/EventLogger.h"
#include "logger/SimulationRecorder.h"
#include "aegis/safety/safety_monitor.hpp"
#include "aegis/safety/watchdog.hpp"
#include "aegis/config/config_manager.hpp"
#include "config/NodeConfig.h"
#include "aegis/coordinator/NodeRegistrationManager.hpp"

// QNX Networking Layer
#include "tcp_server.hpp"
#include "message.hpp"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>

namespace aegis {
namespace runtime {

/**
 * @brief Central Coordinator Runtime integrating all municipal decision, scheduling,
 *        fleet, hospital, network, and diagnostics engines into one distributed pipeline.
 */
class CoordinatorRuntime {
public:
    using AckNotificationCallback = std::function<void(const std::string& nodeId, const std::string& ackPayload)>;

    CoordinatorRuntime();
    virtual ~CoordinatorRuntime();

    // Non-copyable, non-movable
    CoordinatorRuntime(const CoordinatorRuntime&) = delete;
    CoordinatorRuntime& operator=(const CoordinatorRuntime&) = delete;
    CoordinatorRuntime(CoordinatorRuntime&&) = delete;
    CoordinatorRuntime& operator=(CoordinatorRuntime&&) = delete;

    // =========================================================================
    // Lifecycle Management (Task 2 & Task 7)
    // =========================================================================

    /**
     * @brief Initialize all internal coordinator subsystems and load configuration.
     * @param configFilePath Path to coordinator JSON configuration file (optional).
     * @return true on success, false if initialization fails.
     */
    bool Initialize(const std::string& configFilePath = "");

    /**
     * @brief Start coordinator runtime, open TCP server listener, and launch worker threads.
     * @param host Local network host binding (default "0.0.0.0" or "127.0.0.1").
     * @param port TCP port to bind (default 9000).
     * @return true if successfully started; false otherwise.
     */
    bool Start(const std::string& host = "127.0.0.1", uint16_t port = 9000);

    /**
     * @brief Gracefully terminate coordinator runtime, signal clearance, and join threads.
     */
    void Stop();

    /**
     * @brief Check if coordinator runtime is currently executing.
     */
    bool IsRunning() const noexcept;

    // =========================================================================
    // Master Emergency Pipeline Execution (Task 4)
    // =========================================================================

    /**
     * @brief Execute the full end-to-end emergency mission pipeline.
     * Ingests call -> Clinical Triage -> Fleet Dispatch -> Hospital Bed Reservation
     * -> A* Route Planning -> Green Wave Corridor -> Multi-Ambulance Arbitration
     * -> Distributed Consensus -> TCP Command Transmission -> Execution Tracking.
     *
     * @param input Detailed emergency call and patient vitals telemetry.
     * @return EmergencyPipelineResult Comprehensive diagnostic result of pipeline.
     */
    EmergencyPipelineResult ExecuteEmergencyPipeline(const EmergencyPipelineInput& input);

    // =========================================================================
    // Network & Node Operations (Task 5)
    // =========================================================================

    /**
     * @brief Broadcast preemption or operational command to all connected edge nodes.
     */
    bool BroadcastCommand(const std::string& commandPayload);

    /**
     * @brief Send directed preemption command to a specific edge intersection node.
     */
    bool SendCommandToNode(const std::string& nodeId, const std::string& commandPayload);

    /**
     * @brief Retrieve list of currently registered and connected edge intersection nodes.
     */
    std::vector<std::string> GetConnectedNodeIds() const;

    /**
     * @brief Query count of registered nodes.
     */
    size_t GetConnectedNodeCount() const;

    /**
     * @brief Process an incoming network message (for testing and direct dispatch).
     */
    void ProcessIncomingMessage(const std::string& nodeId, const aegis::network::Message& msg) {
        HandleIncomingNetworkMessage(nodeId, msg);
    }

    /**
     * @brief Register a callback for node preemption acknowledgments.
     */
    void SetOnAckCallback(AckNotificationCallback callback);

    // =========================================================================
    // Subsystem Accessors (Dependency Injection & Introspection)
    // =========================================================================
    aegis::core::coordinator::EmergencyCallHandler& GetCallHandler() noexcept { return m_callHandler; }
    aegis::medical::PriorityEngine& GetPriorityEngine() noexcept { return m_priorityEngine; }
    aegis::incident_command::IncidentCommandEngine& GetIncidentCommandEngine() noexcept { return m_incidentCommand; }
    aegis::fleet::FleetManagementEngine& GetFleetEngine() noexcept { return m_fleetManager; }
    aegis::core::routing::RoutePlanner& GetRoutePlanner() noexcept { return m_routePlanner; }
    aegis::green_corridor::EmergencyCorridorPlanner& GetCorridorPlanner() noexcept { return m_corridorPlanner; }
    aegis::green_wave::GreenWaveCoordinator& GetGreenWaveCoordinator() noexcept { return m_greenWaveCoordinator; }
    aegis::scheduler::PriorityScheduler& GetPriorityScheduler() noexcept { return m_scheduler; }
    aegis::multi_corridor::MultiCorridorCoordinator& GetMultiCorridorCoordinator() noexcept { return m_multiCorridor; }
    aegis::mission::MissionExecutionEngine& GetMissionEngine() noexcept { return m_missionEngine; }
    aegis::hospital_management::HospitalResourceEngine& GetHospitalResourceEngine() noexcept { return m_hospitalResource; }
    aegis::hospital::HospitalSelectionEngine& GetHospitalSelectionEngine() noexcept { return m_hospitalSelection; }
    aegis::traffic_network::TrafficNetworkOrchestrator& GetTrafficNetworkOrchestrator() noexcept { return m_networkOrchestrator; }
    aegis::consensus::DistributedConsensusEngine& GetConsensusEngine() noexcept { return m_consensusEngine; }
    aegis::cluster::RaspberryPiClusterManager& GetClusterManager() noexcept { return m_clusterManager; }
    aegis::diagnostics::SystemDiagnosticsEngine& GetDiagnosticsEngine() noexcept { return m_diagnosticsEngine; }
    aegis::network::TcpServer& GetTcpServer() noexcept { return m_tcpServer; }
    aegis::logger::EventLogger& GetEventLogger() noexcept { return m_eventLogger; }
    aegis::logger::SimulationRecorder& GetSimulationRecorder() noexcept { return m_simulationRecorder; }
    std::shared_ptr<aegis::coordinator::NodeRegistrationManager> GetNodeRegistrationManager() noexcept { return m_nodeRegistrationManager; }

    // Lower camelCase aliases for interoperability
    bool initialize(const std::string& cfg = "") { return Initialize(cfg); }
    bool start(const std::string& host = "127.0.0.1", uint16_t port = 9000) { return Start(host, port); }
    void stop() { Stop(); }
    bool isRunning() const noexcept { return IsRunning(); }
    EmergencyPipelineResult executeEmergencyPipeline(const EmergencyPipelineInput& in) { return ExecuteEmergencyPipeline(in); }
    EmergencyPipelineResult processEmergencyCall(const EmergencyPipelineInput& in) { return ExecuteEmergencyPipeline(in); }
    size_t getConnectedNodeCount() const { return GetConnectedNodeCount(); }

    aegis::medical::PriorityEngine& getPriorityEngine() noexcept { return GetPriorityEngine(); }
    aegis::incident_command::IncidentCommandEngine& getIncidentCommandEngine() noexcept { return GetIncidentCommandEngine(); }
    aegis::fleet::FleetManagementEngine& getFleetManagementEngine() noexcept { return GetFleetEngine(); }
    aegis::core::routing::RoutePlanner& getRoutePlanner() noexcept { return GetRoutePlanner(); }
    aegis::replanning::DynamicRouteReplanner& getDynamicRouteReplanner() noexcept { return m_routeReplanner; }
    aegis::green_corridor::EmergencyCorridorPlanner& getEmergencyCorridorPlanner() noexcept { return GetCorridorPlanner(); }
    aegis::green_wave::GreenWaveCoordinator& getGreenWaveCoordinator() noexcept { return GetGreenWaveCoordinator(); }
    aegis::scheduler::PriorityScheduler& getPriorityScheduler() noexcept { return GetPriorityScheduler(); }
    aegis::multi_corridor::MultiCorridorCoordinator& getMultiCorridorCoordinator() noexcept { return GetMultiCorridorCoordinator(); }
    aegis::hospital_management::HospitalResourceEngine& getHospitalResourceEngine() noexcept { return GetHospitalResourceEngine(); }
    aegis::hospital::HospitalSelectionEngine& getHospitalSelectionEngine() noexcept { return GetHospitalSelectionEngine(); }
    aegis::traffic_network::TrafficNetworkOrchestrator& getTrafficNetworkOrchestrator() noexcept { return GetTrafficNetworkOrchestrator(); }
    aegis::consensus::DistributedConsensusEngine& getDistributedConsensusEngine() noexcept { return GetConsensusEngine(); }
    aegis::cluster::RaspberryPiClusterManager& getRaspberryPiClusterManager() noexcept { return GetClusterManager(); }
    aegis::diagnostics::SystemDiagnosticsEngine& getSystemDiagnosticsEngine() noexcept { return GetDiagnosticsEngine(); }
    aegis::network::TcpServer& getTcpServer() noexcept { return GetTcpServer(); }
    aegis::logger::EventLogger& getEventLogger() noexcept { return GetEventLogger(); }
    aegis::logger::SimulationRecorder& getSimulationRecorder() noexcept { return GetSimulationRecorder(); }
    std::shared_ptr<aegis::coordinator::NodeRegistrationManager> getNodeRegistrationManager() noexcept { return GetNodeRegistrationManager(); }

private:
    // Worker Thread Execution Functions (Task 6)
    void SchedulerWorkerLoop();
    void DecisionEngineWorkerLoop();
    void LoggingWorkerLoop();
    void WatchdogWorkerLoop();
    void HospitalWorkerLoop();
    void SimulationWorkerLoop();

    // Internal Helpers
    void SeedDefaultMunicipalInfrastructure();
    void HandleIncomingNetworkMessage(const std::string& nodeId, const aegis::network::Message& msg);

    // =========================================================================
    // Member State
    // =========================================================================
    std::atomic<bool> m_isInitialized{false};
    std::atomic<bool> m_isRunning{false};
    std::string m_nodeId{"COORDINATOR"};
    std::string m_listenHost{"127.0.0.1"};
    uint16_t m_listenPort{9000};
    uint32_t m_heartbeatTimeoutMs{5000};

    // Subsystem Instances
    aegis::core::coordinator::EmergencyCallHandler m_callHandler;
    aegis::medical::PriorityEngine m_priorityEngine;
    aegis::incident_command::IncidentCommandEngine m_incidentCommand;
    aegis::fleet::FleetManagementEngine m_fleetManager;
    aegis::core::routing::RoutePlanner m_routePlanner;
    aegis::replanning::DynamicRouteReplanner m_routeReplanner;
    aegis::green_corridor::EmergencyCorridorPlanner m_corridorPlanner;
    aegis::green_wave::GreenWaveCoordinator m_greenWaveCoordinator;
    aegis::scheduler::PriorityScheduler m_scheduler;
    aegis::multi_corridor::MultiCorridorCoordinator m_multiCorridor;
    aegis::coordinator::CoordinatorManager m_coordinatorManager;
    aegis::mission::MissionExecutionEngine m_missionEngine;
    aegis::hospital_management::HospitalResourceEngine m_hospitalResource;
    aegis::hospital::HospitalSelectionEngine m_hospitalSelection;
    aegis::traffic_network::TrafficNetworkOrchestrator m_networkOrchestrator;
    aegis::consensus::DistributedConsensusEngine m_consensusEngine;
    aegis::cluster::RaspberryPiClusterManager m_clusterManager;
    aegis::diagnostics::SystemDiagnosticsEngine m_diagnosticsEngine;
    aegis::logger::EventLogger m_eventLogger;
    aegis::logger::SimulationRecorder m_simulationRecorder;
    aegis::safety::SafetyMonitor m_safetyMonitor;
    aegis::safety::Watchdog m_watchdog;
    aegis::config::ConfigManager m_configManager;
    std::shared_ptr<aegis::coordinator::NodeRegistrationManager> m_nodeRegistrationManager;

    // Networking
    aegis::network::TcpServer m_tcpServer;

    // Worker Threads
    std::thread m_schedulerThread;
    std::thread m_decisionThread;
    std::thread m_loggingThread;
    std::thread m_watchdogThread;
    std::thread m_hospitalThread;
    std::thread m_simulationThread;

    // Synchronization & Callbacks
    mutable std::mutex m_stateMutex;
    mutable std::mutex m_acksMutex;
    AckNotificationCallback m_ackCallback{nullptr};
    std::unordered_map<std::string, std::string> m_recentAcks{};
};

} // namespace runtime
} // namespace aegis

#endif // AEGIS_RUNTIME_COORDINATOR_RUNTIME_H
