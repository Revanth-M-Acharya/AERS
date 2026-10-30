/**
 * @file HardwareEventLogger.h
 * @brief SMART AEGIS (AERS) Phase 20 - Dedicated Hardware Event Logger.
 */

#ifndef AEGIS_HARDWARE_EVENT_LOGGER_H
#define AEGIS_HARDWARE_EVENT_LOGGER_H

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace hardware {

struct HardwareLogEntry {
    uint64_t timestampMs{0};
    uint64_t timestampUs{0}; // Phase 2: Microsecond-precision timestamp
    std::string timestampIso;
    std::string subsystem; // "BOOT", "RELAY_POST", "REGISTRATION", "RECOVERY", "WATCHDOG", "SHUTDOWN", "RELAY", "GPIO", "NETWORK", "SENSOR"
    std::string severity;  // "INFO", "WARN", "ERROR", "CRITICAL"
    std::string message;
};

class HardwareEventLogger {
public:
    explicit HardwareEventLogger(size_t maxCapacity = 1000);
    ~HardwareEventLogger() = default;

    void Log(const std::string& subsystem, const std::string& severity, const std::string& message);
    void Info(const std::string& subsystem, const std::string& message);
    void Warn(const std::string& subsystem, const std::string& message);
    void Error(const std::string& subsystem, const std::string& message);
    void Critical(const std::string& subsystem, const std::string& message);

    // Phase 2 Structured Timestamped Embedded Event Loggers (Microsecond Precision)
    void LogBootEvent(const std::string& stepName, bool passed, uint64_t durationUs, const std::string& details);
    void LogRelayPost(uint8_t channel, bool passed, uint64_t durationUs, const std::string& details);
    void LogRegistrationEvent(const std::string& nodeId, const std::string& ip, bool success);
    void LogRecoveryEvent(const std::string& nodeId, const std::string& details);
    void LogWatchdogEvent(const std::string& eventType, const std::string& details);
    void LogShutdownEvent(const std::string& stepName, bool success);
    void LogSubsystemFailure(const std::string& subsystem, const std::string& reason);

    std::vector<HardwareLogEntry> GetRecentLogs(size_t count = 50) const;
    std::vector<HardwareLogEntry> GetAllLogs() const;
    void Clear();
    size_t GetLogCount() const;

    std::string ExportToCsv() const;
    std::string ExportToJson() const;

private:
    mutable std::mutex m_mutex;
    size_t m_maxCapacity;
    std::deque<HardwareLogEntry> m_logs;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_EVENT_LOGGER_H

