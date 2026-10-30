/**
 * @file HardwareController.h
 * @brief SMART AEGIS (AERS) Phase 20 - Master Physical Hardware Controller.
 */

#ifndef AEGIS_HARDWARE_CONTROLLER_H
#define AEGIS_HARDWARE_CONTROLLER_H

#include "HardwareConfiguration.h"
#include "HardwareEventLogger.h"
#include "IntersectionHardware.h"
#include "WirelessNodeManager.h"
#include "HardwareHealthMonitor.h"
#include "HardwareDiagnostics.h"
#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/hardware/BootManager.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace aegis {
namespace hardware {

class HardwareController {
public:
    explicit HardwareController(std::string nodeId = "INT_A",
                                const std::string& configFilePath = "config/hardware.json");
    ~HardwareController();

    // Prevent copy/move
    HardwareController(const HardwareController&) = delete;
    HardwareController& operator=(const HardwareController&) = delete;

    // Initialization & Lifecycle
    bool Initialize(const std::string& configFilePath = "config/hardware.json");
    void Start();
    void Stop();
    bool IsRunning() const;
    void Update(double dtSec);

    // Node Identity
    std::string GetNodeId() const { return m_nodeId; }

    // Live Traffic Control
    void SetNormalPhase(DualPhase phase);
    DualPhase GetCurrentPhase() const;

    // Emergency Green Corridor Preemption
    void TriggerEmergencyPreemption(const std::string& corridorId, const std::string& direction = "ROAD_A");
    void ReleasePreemption();
    bool IsPreempted() const;

    // Fail-Safe Operation
    void TriggerFailSafe(const std::string& reason);
    void RecoverFromFailSafe();
    bool IsInFailSafe() const;

    // Manual Overrides
    void ForceGreen(const std::string& direction = "ROAD_A");
    void ForceRed();
    void RestoreNormalCycle();

    // IR Sensors
    void SimulateVehicleArrival();
    void SimulateVehicleDeparture();
    bool IsVehicleApproaching() const;
    bool IsVehicleDeparting() const;

    // Wireless & Fault Injection
    void SimulateWifiDrop(bool dropped);
    bool IsWifiConnected() const;

    // Diagnostics & Health
    bool RunDiagnostics();
    std::string GetDiagnosticReport();
    std::string GetHealthReport();
    AggregateHardwareHealth GetHealth() const;

    // Subsystem Accessors
    std::shared_ptr<IntersectionHardware> GetIntersectionHardware() const { return m_hardware; }
    std::shared_ptr<WirelessNodeManager> GetWirelessManager() const { return m_wireless; }
    std::shared_ptr<HardwareHealthMonitor> GetHealthMonitor() const { return m_healthMonitor; }
    std::shared_ptr<HardwareDiagnostics> GetDiagnostics() const { return m_diagnostics; }
    std::shared_ptr<HardwareWatchdog> GetWatchdog() const { return m_watchdog; }
    std::shared_ptr<BootManager> GetBootManager() const { return m_bootManager; }
    std::shared_ptr<HardwareEventLogger> GetLogger() const { return m_logger; }
    const HardwareConfiguration& GetConfig() const { return m_config; }

private:
    mutable std::recursive_mutex m_mutex;
    std::string m_nodeId;
    std::string m_configFilePath;

    std::atomic<bool> m_running{false};

    HardwareConfiguration m_config;
    std::shared_ptr<HardwareEventLogger> m_logger;
    std::shared_ptr<IntersectionHardware> m_hardware;
    std::shared_ptr<WirelessNodeManager> m_wireless;
    std::shared_ptr<HardwareHealthMonitor> m_healthMonitor;
    std::shared_ptr<HardwareDiagnostics> m_diagnostics;
    std::shared_ptr<HardwareWatchdog> m_watchdog;
    std::shared_ptr<BootManager> m_bootManager;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_CONTROLLER_H
