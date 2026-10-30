/**
 * @file DeploymentManager.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Master Deployment Manager Implementation.
 */

#include "DeploymentManager.h"
#include <iostream>
#include <chrono>

namespace aegis {
namespace deployment {

DeploymentManager::DeploymentManager() = default;

DeploymentManager::~DeploymentManager() {
    Stop();
}

bool DeploymentManager::Initialize(const std::string& configFilePath) {
    NodeConfiguration config;
    std::string path = configFilePath;
    if (path.empty()) {
        path = ConfigurationLoader::FindDefaultConfigFile();
    }

    if (!path.empty()) {
        std::string err;
        if (!ConfigurationLoader::LoadFromFile(path, config, &err)) {
            std::cerr << "[DEPLOYMENT] Warning: Failed to load config from " << path << ": " << err << std::endl;
            config = NodeConfiguration::DefaultCoordinator();
        }
    } else {
        config = NodeConfiguration::DefaultCoordinator();
    }

    return Initialize(config);
}

bool DeploymentManager::Initialize(const NodeConfiguration& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;

    ValidationResult val = DeploymentValidator::ValidateNode(m_config);
    if (!val.valid) {
        std::cerr << "[DEPLOYMENT] Configuration validation failed:" << std::endl;
        for (const auto& e : val.errors) {
            std::cerr << "  - " << e << std::endl;
        }
        return false;
    }

    m_runtimeConfig.SetConfiguration(m_config);

    // Initialize Log Directory & Streams
    StartupManager::InitializeEnvironment(m_config.logDirectory);
    m_logger.Initialize(m_config.logDirectory, m_config.nodeId);
    m_logger.LogRuntime("INFO", "DEPLOYMENT", "Initialized configuration for " + m_config.nodeId);

    // Initialize GPIO Driver
    if (m_config.gpioEnabled) {
        m_gpioDriver.Initialize(m_config.gpioMapping, m_config.simulationEnabled);
        m_logger.LogGpio(m_config.gpioMapping.redPin, "INIT", "HIGH", "Safe Red State Asserted");
    }

    // Instantiate Underlying Runtimes
    if (m_config.IsCoordinator()) {
        m_coordinatorRuntime = std::make_shared<runtime::CoordinatorRuntime>();
        if (!m_coordinatorRuntime->Initialize()) {
            m_logger.LogCrash("COORDINATOR_INIT_FAILED", "DeploymentManager::Initialize", "Failed to init CoordinatorRuntime");
            return false;
        }

        // Configure Coordinator Heartbeat Supervisor
        m_heartbeatSupervisor.SetTimeoutCallback([this](const std::string& nodeId) {
            m_logger.LogDiagnostics("HEARTBEAT", 0, 0, 0, 0, "Heartbeat timeout for node " + nodeId + " -> OFFLINE");
            if (m_coordinatorRuntime) {
                m_coordinatorRuntime->GetClusterManager().UpdateHealth(nodeId, rpi_cluster::NodeHealth::OFFLINE);
                m_coordinatorRuntime->GetDiagnosticsEngine().RecordFailure(nodeId, "HEARTBEAT_TIMEOUT");
            }
        });

        m_heartbeatSupervisor.SetRecoveryCallback([this](const std::string& nodeId) {
            m_logger.LogDiagnostics("HEARTBEAT", 0, 0, 0, 100.0, "Node " + nodeId + " recovered -> ONLINE");
            if (m_coordinatorRuntime) {
                m_coordinatorRuntime->GetClusterManager().RecoverNode(nodeId);
            }
        });
    } else {
        m_trafficNodeRuntime = std::make_shared<runtime::TrafficNodeRuntime>(m_config.nodeId);
        if (!m_trafficNodeRuntime->Initialize(m_config.nodeId)) {
            m_logger.LogCrash("TRAFFIC_NODE_INIT_FAILED", "DeploymentManager::Initialize", "Failed to init TrafficNodeRuntime");
            return false;
        }
    }

    // Register Graceful Shutdown Cleanup Hook
    m_shutdownManager.RegisterCleanupHook([this]() {
        m_logger.LogRuntime("INFO", "SHUTDOWN", "Shutdown cleanup hook executed");
        if (m_config.gpioEnabled) {
            m_gpioDriver.Cleanup();
        }
    });

    return true;
}

bool DeploymentManager::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running.load()) return true;

    // Run deterministic boot sequence
    if (!m_bootManager.RunFullBootSequence(m_config)) {
        m_logger.LogCrash("BOOT_FAILED", "BootManager::RunFullBootSequence", m_bootManager.GetLastError());
        return false;
    }

    m_logger.LogRuntime("INFO", "BOOT", "Boot sequence reached READY in " +
                        std::to_string(m_bootManager.GetTotalBootTimeUs()) + " us");

    // Launch underlying runtime engines
    if (m_config.IsCoordinator()) {
        if (!m_coordinatorRuntime || !m_coordinatorRuntime->IsRunning()) {
            m_coordinatorRuntime = std::make_shared<runtime::CoordinatorRuntime>();
            m_coordinatorRuntime->Initialize("config/coordinator_profile.json");
        }
        if (!m_coordinatorRuntime->Start(m_config.ipAddress, m_config.port)) {
            m_logger.LogCrash("COORDINATOR_START_FAILED", "CoordinatorRuntime::Start", "Failed to bind/listen TCP");
            return false;
        }
        m_heartbeatSupervisor.StartSupervision(m_config.heartbeatIntervalMs * 3, 200);
        m_logger.LogNetwork("START", m_config.ipAddress + ":" + std::to_string(m_config.port), "TCP_SERVER", 0, "Listening");
    } else {
        // Resolve coordinator
        DiscoveredCoordinator disc;
        NetworkDiscovery::ResolveCoordinator(m_config, disc, 1500);

        if (!m_trafficNodeRuntime || !m_trafficNodeRuntime->IsRunning()) {
            m_trafficNodeRuntime = std::make_shared<runtime::TrafficNodeRuntime>(m_config.nodeId);
            m_trafficNodeRuntime->Initialize(m_config.nodeId);
        }

        if (!m_trafficNodeRuntime->Start(m_config.coordinatorAddress, m_config.coordinatorPort)) {
            m_logger.LogRuntime("WARN", "NETWORK", "Initial connect to Coordinator failed; auto-reconnect active");
        }
        m_logger.LogNetwork("CONNECT", m_config.coordinatorAddress + ":" + std::to_string(m_config.coordinatorPort),
                            "TCP_CLIENT", 0, "Connected");

        // Launch edge 1 Hz heartbeat streaming thread
        m_running.store(true);
        m_edgeHeartbeatThread = std::thread(&DeploymentManager::RunEdgeHeartbeatLoop, this);
        return true;
    }

    m_running.store(true);
    return true;
}

void DeploymentManager::Stop() {
    if (m_running.exchange(false)) {
        m_logger.LogRuntime("INFO", "SHUTDOWN", "Stopping deployment manager...");

        if (m_edgeHeartbeatThread.joinable()) {
            m_edgeHeartbeatThread.join();
        }

        if (m_config.IsCoordinator()) {
            m_heartbeatSupervisor.StopSupervision();
            if (m_coordinatorRuntime) {
                m_coordinatorRuntime->Stop();
            }
        } else {
            if (m_trafficNodeRuntime) {
                m_trafficNodeRuntime->Stop();
            }
        }

        m_shutdownManager.ExecuteShutdown(m_config.IsCoordinator());
        m_logger.LogRuntime("INFO", "SHUTDOWN", "Shutdown completed cleanly");
        m_logger.Close();
    }
}

bool DeploymentManager::CanAcceptEmergencies() const {
    if (!m_config.IsCoordinator()) return false;
    if (!m_running.load()) return false;
    if (m_config.expectedNodes.empty()) return true;

    for (const auto& expectedId : m_config.expectedNodes) {
        if (!m_heartbeatSupervisor.IsNodeOnline(expectedId)) {
            // Check CoordinatorRuntime directly as backup
            if (m_coordinatorRuntime) {
                rpi_cluster::RaspberryPiNode rpiNode;
                if (!m_coordinatorRuntime->GetClusterManager().GetNode(expectedId, rpiNode) ||
                    rpiNode.nodeHealth == rpi_cluster::NodeHealth::OFFLINE ||
                    !rpiNode.online) {
                    return false;
                }
            } else {
                return false;
            }
        }
    }
    return true;
}

void DeploymentManager::SimulateFailure(const std::string& nodeId) {
    if (m_config.IsCoordinator()) {
        m_heartbeatSupervisor.SetNodeOffline(nodeId);
        if (m_coordinatorRuntime) {
            m_coordinatorRuntime->GetClusterManager().UpdateHealth(nodeId, rpi_cluster::NodeHealth::OFFLINE);
            m_coordinatorRuntime->GetDiagnosticsEngine().RecordFailure(nodeId, "SIMULATED_FAILURE");
        }
    } else {
        if (m_trafficNodeRuntime) {
            m_trafficNodeRuntime->DegradeToFailSafe();
        }
    }
    m_logger.LogDiagnostics("SIMULATION", 0, 0, 0, 0, "Injected simulated failure on node " + nodeId);
}

void DeploymentManager::RecoverNode(const std::string& nodeId) {
    if (m_config.IsCoordinator()) {
        NodeTelemetry t = HeartbeatSupervisor::GatherLocalTelemetry(nodeId, "RED");
        m_heartbeatSupervisor.RecordHeartbeat(nodeId, t);
        if (m_coordinatorRuntime) {
            m_coordinatorRuntime->GetClusterManager().RecoverNode(nodeId);
        }
    }
    m_logger.LogDiagnostics("SIMULATION", 0, 0, 0, 100.0, "Recovered node " + nodeId);
}

void DeploymentManager::RunEdgeHeartbeatLoop() {
    while (m_running.load()) {
        uint32_t interval = (m_config.heartbeatIntervalMs > 0) ? m_config.heartbeatIntervalMs : 1000;
        uint32_t slept = 0;
        while (m_running.load() && slept < interval) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            slept += 25;
        }
        if (!m_running.load()) break;

        std::string sigState = "RED";
        if (m_trafficNodeRuntime) {
            sigState = m_trafficNodeRuntime->GetSignalStateString();
        }
        std::string gpioStatus = m_gpioDriver.GetStatusString();

        NodeTelemetry telemetry = HeartbeatSupervisor::GatherLocalTelemetry(m_config.nodeId, sigState, gpioStatus);

        m_logger.LogDiagnostics("TELEMETRY", telemetry.cpuUsagePercent, telemetry.memoryUsagePercent,
                                telemetry.temperatureCelsius, telemetry.nodeHealth,
                                "State: " + sigState + " | GPIO: " + gpioStatus);
    }
}

} // namespace deployment
} // namespace aegis