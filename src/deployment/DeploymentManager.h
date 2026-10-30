/**
 * @file DeploymentManager.h
 * @brief SMART AEGIS (AERS) Phase 16D - Master Deployment Manager.
 *
 * Master node daemon orchestrator combining configuration loading, validation,
 * deterministic boot sequencing, network discovery, multi-stream logging,
 * GPIO driver control, runtime engine binding, and graceful shutdown.
 */

#ifndef AEGIS_DEPLOYMENT_DEPLOYMENT_MANAGER_H
#define AEGIS_DEPLOYMENT_DEPLOYMENT_MANAGER_H

#include "NodeConfiguration.h"
#include "ConfigurationLoader.h"
#include "DeploymentValidator.h"
#include "RuntimeConfiguration.h"
#include "NetworkDiscovery.h"
#include "BootManager.h"
#include "StartupManager.h"
#include "DeploymentGpioDriver.h"
#include "DeploymentLogger.h"
#include "HeartbeatSupervisor.h"
#include "GracefulShutdownManager.h"

#include "runtime/CoordinatorRuntime.h"
#include "runtime/TrafficNodeRuntime.h"

#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>

namespace aegis {
namespace deployment {

class DeploymentManager {
public:
    DeploymentManager();
    virtual ~DeploymentManager();

    // Non-copyable, non-movable
    DeploymentManager(const DeploymentManager&) = delete;
    DeploymentManager& operator=(const DeploymentManager&) = delete;

    // =========================================================================
    // Initialization & Lifecycle
    // =========================================================================

    /**
     * @brief Initialize deployment layer from a configuration file.
     * @param configFilePath Path to JSON configuration file (e.g. "config/node.json").
     * @return true on success; false on load or validation error.
     */
    bool Initialize(const std::string& configFilePath = "");

    /**
     * @brief Initialize deployment layer directly from a NodeConfiguration object.
     * @param config NodeConfiguration instance.
     * @return true on success; false on validation error.
     */
    bool Initialize(const NodeConfiguration& config);

    /**
     * @brief Start node services, establish network, launch background loops, and advance boot to READY.
     * @return true on success; false if startup fails.
     */
    bool Start();

    /**
     * @brief Stop node services and perform multi-phase graceful shutdown.
     */
    void Stop();

    /**
     * @brief Query whether node deployment manager is actively running.
     */
    bool IsRunning() const noexcept { return m_running.load(); }

    /**
     * @brief Query whether deterministic boot process has fully reached READY stage.
     */
    bool IsReady() const noexcept { return m_bootManager.IsReady(); }

    // =========================================================================
    // Subsystem Accessors
    // =========================================================================

    const NodeConfiguration& GetConfiguration() const noexcept { return m_config; }
    RuntimeConfiguration& GetRuntimeConfig() noexcept { return m_runtimeConfig; }
    BootManager& GetBootManager() noexcept { return m_bootManager; }
    HeartbeatSupervisor& GetHeartbeatSupervisor() noexcept { return m_heartbeatSupervisor; }
    GracefulShutdownManager& GetShutdownManager() noexcept { return m_shutdownManager; }
    DeploymentGpioDriver& GetGpioDriver() noexcept { return m_gpioDriver; }
    DeploymentLogger& GetLogger() noexcept { return m_logger; }

    std::shared_ptr<runtime::CoordinatorRuntime> GetCoordinatorRuntime() noexcept {
        return m_coordinatorRuntime;
    }

    std::shared_ptr<runtime::TrafficNodeRuntime> GetTrafficNodeRuntime() noexcept {
        return m_trafficNodeRuntime;
    }

    /**
     * @brief Coordinator gating: Check if cluster is fully online before accepting emergencies.
     */
    bool CanAcceptEmergencies() const;

    /**
     * @brief Manually inject a network failure simulation on an edge node.
     */
    void SimulateFailure(const std::string& nodeId);

    /**
     * @brief Recover a failed node.
     */
    void RecoverNode(const std::string& nodeId);

private:
    void RunEdgeHeartbeatLoop();

    mutable std::mutex m_mutex;
    std::atomic<bool> m_running{false};
    NodeConfiguration m_config{};
    RuntimeConfiguration m_runtimeConfig{};
    BootManager m_bootManager{};
    DeploymentGpioDriver m_gpioDriver{};
    DeploymentLogger m_logger{};
    HeartbeatSupervisor m_heartbeatSupervisor{};
    GracefulShutdownManager m_shutdownManager{};

    // Runtimes
    std::shared_ptr<runtime::CoordinatorRuntime> m_coordinatorRuntime{nullptr};
    std::shared_ptr<runtime::TrafficNodeRuntime> m_trafficNodeRuntime{nullptr};

    std::thread m_edgeHeartbeatThread;
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_DEPLOYMENT_MANAGER_H
