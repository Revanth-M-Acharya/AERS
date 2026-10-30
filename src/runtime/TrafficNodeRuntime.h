/**
 * @file TrafficNodeRuntime.h
 * @brief SMART AEGIS (AERS) Phase 16C - Edge Traffic Node Distributed Runtime Engine.
 *
 * Integrates edge-side network communication, traffic signal state machines,
 * Hardware Abstraction Layer (HAL), QNX Device Driver Framework, and Raspberry Pi 4 BSP
 * into an autonomous, fault-tolerant edge controller runtime (Raspberry Pis 2, 3, 4 / QNX RTOS).
 */

#ifndef AEGIS_RUNTIME_TRAFFIC_NODE_RUNTIME_H
#define AEGIS_RUNTIME_TRAFFIC_NODE_RUNTIME_H

#include "traffic_signal/TrafficSignalNode.h"
#include "intersection/SignalController.h"
#include "hardware_abstraction/HardwareAbstractionLayer.h"
#include "qnx_driver_framework/QNXDeviceDriverFramework.h"
#include "bsp/RaspberryPi4BSP.h"
#include "logger/EventLogger.h"
#include "aegis/safety/safety_monitor.hpp"
#include "aegis/safety/watchdog.hpp"
#include "aegis/config/config_manager.hpp"
#include "config/NodeConfig.h"

// QNX Networking Layer
#include "tcp_client.hpp"
#include "message.hpp"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>

namespace aegis {
namespace runtime {

/**
 * @brief Edge Traffic Node Runtime executing signal control, GPIO actuation,
 *        heartbeat generation, and command dispatching on an edge Raspberry Pi controller.
 */
class TrafficNodeRuntime {
public:
    using CommandExecutedCallback = std::function<void(const std::string& command, const std::string& resultingState)>;

    explicit TrafficNodeRuntime(std::string nodeId = "INT_A");
    virtual ~TrafficNodeRuntime();

    // Non-copyable, non-movable
    TrafficNodeRuntime(const TrafficNodeRuntime&) = delete;
    TrafficNodeRuntime& operator=(const TrafficNodeRuntime&) = delete;
    TrafficNodeRuntime(TrafficNodeRuntime&&) = delete;
    TrafficNodeRuntime& operator=(TrafficNodeRuntime&&) = delete;

    // =========================================================================
    // Lifecycle Management (Task 3 & Task 7)
    // =========================================================================

    /**
     * @brief Initialize hardware drivers, GPIO pin reservations, signal FSM, and config.
     * @param nodeId Unique intersection identifier (e.g., "INT_A", "INT_B", "INT_C").
     * @param configFilePath Optional deployment configuration file.
     * @return true on success, false if hardware reservation or configuration fails.
     */
    bool Initialize(const std::string& nodeId, const std::string& configFilePath = "");

    /**
     * @brief Connect to central coordinator, initiate heartbeat, and launch edge threads.
     * @param host Coordinator IPv4 host address (e.g. "127.0.0.1" or "192.168.1.100").
     * @param port Coordinator TCP port (default 9000).
     * @return true if connection established; false otherwise.
     */
    bool Start(const std::string& host = "127.0.0.1", uint16_t port = 9000);

    /**
     * @brief Gracefully terminate edge controller, restore safe clearance, and release hardware.
     */
    void Stop();

    /**
     * @brief Query whether edge node is currently running.
     */
    bool IsRunning() const noexcept;

    /**
     * @brief Query whether TCP socket link to coordinator is currently active.
     */
    bool IsConnected() const noexcept;

    /**
     * @brief Get the unique intersection identifier.
     */
    const std::string& GetNodeId() const noexcept { return m_nodeId; }

    // =========================================================================
    // Command Dispatcher & Preemption Actuation (Task 3, 4, 5)
    // =========================================================================

    /**
     * @brief Execute an incoming preemption or operational command string.
     * @param commandStr Command string (e.g. "PREEMPT_GREEN INT_A 30", "RESTORE_CYCLE INT_A").
     * @return true if command was successfully parsed and applied; false otherwise.
     */
    bool ExecuteCommand(const std::string& commandStr);

    /**
     * @brief Trigger emergency green preemption on the primary approach.
     * @param durationSec Duration in seconds to lock green preemption (default 30s).
     */
    bool TriggerPreemption(uint32_t durationSec = 30);

    /**
     * @brief Restore normal cyclic operation through yellow and all-red clearance.
     */
    bool RestoreNormalCycle();

    /**
     * @brief Degrade node to fail-safe flashing yellow upon communication loss.
     */
    void DegradeToFailSafe();

    // =========================================================================
    // Status & State Queries
    // =========================================================================

    /**
     * @brief Get human-readable signal state string (e.g. "GREEN", "RED", "YELLOW").
     */
    std::string GetSignalStateString() const;

    /**
     * @brief Query digital logic level of a physical GPIO pin.
     */
    aegis::bsp::PinState GetPinState(uint8_t pin) const;

    /**
     * @brief Register callback invoked when commands are executed.
     */
    void SetOnCommandExecuted(CommandExecutedCallback callback);

    // Subsystem accessors
    aegis::traffic_signal::TrafficSignalNode& GetSignalNode() noexcept { return m_signalNode; }
    aegis::bsp::RaspberryPi4BSP& GetBSP() noexcept { return m_bsp; }
    aegis::hal::HardwareAbstractionLayer& GetHAL() noexcept { return m_hal; }
    aegis::network::TcpClient& GetTcpClient() noexcept { return m_tcpClient; }

    // Lower camelCase aliases for interoperability
    bool initialize(const std::string& id = "", const std::string& cfg = "") {
        return Initialize(id.empty() ? m_nodeId : id, cfg);
    }
    bool start(const std::string& host = "127.0.0.1", uint16_t port = 9000) { return Start(host, port); }
    void stop() { Stop(); }
    bool isRunning() const noexcept { return IsRunning(); }
    bool isConnected() const noexcept { return IsConnected(); }
    const std::string& getNodeId() const noexcept { return GetNodeId(); }
    bool triggerPreemption(uint32_t dur = 30) { return TriggerPreemption(dur); }
    bool restoreNormalCycle() { return RestoreNormalCycle(); }
    void degradeToFailSafe() { DegradeToFailSafe(); }
    std::string getSignalStateString() const { return GetSignalStateString(); }
    aegis::bsp::PinState getPinState(uint8_t pin) const { return GetPinState(pin); }
    aegis::traffic_signal::TrafficSignalNode& getSignalNode() noexcept { return GetSignalNode(); }
    aegis::bsp::RaspberryPi4BSP& getBSP() noexcept { return GetBSP(); }
    aegis::hal::HardwareAbstractionLayer& getHAL() noexcept { return GetHAL(); }
    aegis::network::TcpClient& getTcpClient() noexcept { return GetTcpClient(); }

private:
    // Worker Thread Execution Functions (Task 6)
    void TrafficFsmWorkerLoop();
    void GpioMonitorWorkerLoop();
    void LoggerWorkerLoop();
    void SafetyWorkerLoop();

    // Internal Message Handler
    void HandleNetworkMessage(const aegis::network::Message& msg);

    // =========================================================================
    // Member State
    // =========================================================================
    std::string m_nodeId{"INT_A"};
    std::atomic<bool> m_isInitialized{false};
    std::atomic<bool> m_isRunning{false};
    std::string m_coordinatorHost{"127.0.0.1"};
    uint16_t m_coordinatorPort{9000};

    // Subsystem Instances
    aegis::network::TcpClient m_tcpClient;
    aegis::traffic_signal::TrafficSignalNode m_signalNode;
    aegis::intersection::SignalController m_signalController;
    aegis::hal::HardwareAbstractionLayer m_hal;
    aegis::qnx_driver::QNXDeviceDriverFramework m_qnxFramework;
    aegis::bsp::RaspberryPi4BSP m_bsp;
    aegis::logger::EventLogger m_eventLogger;
    aegis::safety::SafetyMonitor m_safetyMonitor;
    aegis::safety::Watchdog m_watchdog;
    aegis::config::ConfigManager m_configManager;

    // Standardized Pin Allocations (Raspberry Pi 4 BCM Pinout)
    uint8_t m_pinRedNS{17};
    uint8_t m_pinYellowNS{27};
    uint8_t m_pinGreenNS{22};
    uint8_t m_pinRedEW{23};
    uint8_t m_pinYellowEW{24};
    uint8_t m_pinGreenEW{25};
    uint8_t m_pinWatchdog{26};

    // Preemption Tracking
    std::atomic<bool> m_isPreempted{false};
    std::atomic<uint32_t> m_preemptionRemainingSec{0};

    // Worker Threads
    std::thread m_trafficFsmThread;
    std::thread m_gpioMonitorThread;
    std::thread m_loggerThread;
    std::thread m_safetyThread;

    mutable std::mutex m_stateMutex;
    CommandExecutedCallback m_commandCallback{nullptr};
};

} // namespace runtime
} // namespace aegis

#endif // AEGIS_RUNTIME_TRAFFIC_NODE_RUNTIME_H
