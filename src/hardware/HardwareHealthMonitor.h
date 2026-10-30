/**
 * @file HardwareHealthMonitor.h
 * @brief SMART AEGIS (AERS) Phase 20 - Hardware Telemetry & Physical Health Monitor.
 */

#ifndef AEGIS_HARDWARE_HEALTH_MONITOR_H
#define AEGIS_HARDWARE_HEALTH_MONITOR_H

#include "IntersectionHardware.h"
#include "WirelessNodeManager.h"
#include "HardwareEventLogger.h"
#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/hardware/BootManager.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class HardwareHealthGrade : uint8_t {
    HEALTHY = 0,
    DEGRADED,
    CRITICAL_FAULT
};

std::string HardwareHealthGradeToString(HardwareHealthGrade grade);

struct AggregateHardwareHealth {
    std::string nodeId{"INT_A"};
    HardwareHealthGrade grade{HardwareHealthGrade::HEALTHY};
    double socTemperatureC{44.5};
    double cpuTemperatureC{44.5}; // Alias
    double coreVoltageV{1.20};
    uint64_t ramUsedMB{320};
    uint64_t ramTotalMB{4096};
    double cpuLoadPct{12.5};
    double cpuUsagePct{12.5};     // Alias
    double ramUsagePct{7.8};

    // Subsystem Health Status (Phase 2 Specific Fields)
    std::string gpioStatus{"RESERVED_PINS_OK"};
    std::string relayStatus{"4_CHANNELS_PASS"};
    std::string sensorStatus{"OPTICAL_IR_HEALTHY"};
    std::string networkStatus{"EXCELLENT"};
    std::string watchdogStatus{"ACTIVE_NOMINAL"};
    std::string bootStatus{"BOOT_COMPLETED"};

    bool gpioHealthy{true};
    bool relaysHealthy{true};
    bool irSensorsHealthy{true};
    bool wirelessHealthy{true};
    bool watchdogHealthy{true};
    bool bootHealthy{true};

    uint64_t totalRelaySwitches{0};
    uint64_t totalIrDetections{0};
    int8_t wifiRssiDbm{-55};
    double wifiLatencyMs{1.2};

    std::string ToJson() const;
};

class HardwareHealthMonitor {
public:
    explicit HardwareHealthMonitor(std::string nodeId = "INT_A",
                                   std::shared_ptr<HardwareEventLogger> logger = nullptr);
    ~HardwareHealthMonitor() = default;

    void SetIntersectionHardware(std::shared_ptr<IntersectionHardware> hardware);
    void SetWirelessManager(std::shared_ptr<WirelessNodeManager> wireless);
    void SetHardwareWatchdog(std::shared_ptr<HardwareWatchdog> watchdog);
    void SetBootManager(std::shared_ptr<BootManager> bootManager);

    AggregateHardwareHealth EvaluateHealth();
    std::string GenerateHealthReport();
    std::string PublishHealthJson();

    HardwareHealthGrade GetCurrentHealthGrade() const;
    bool IsSystemHealthy() const;

private:
    mutable std::mutex m_mutex;
    std::string m_nodeId;
    std::shared_ptr<HardwareEventLogger> m_logger;

    std::shared_ptr<IntersectionHardware> m_hardware;
    std::shared_ptr<WirelessNodeManager> m_wireless;
    std::shared_ptr<HardwareWatchdog> m_watchdog;
    std::shared_ptr<BootManager> m_bootManager;

    AggregateHardwareHealth m_latestHealth;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_HEALTH_MONITOR_H

