/**
 * @file HardwareEventLogger.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Dedicated Hardware Event Logger Implementation.
 */

#include "HardwareEventLogger.h"
#include "aegis/common/time_utils.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace aegis {
namespace hardware {

static std::string GetCurrentTimestampIso() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

static uint64_t GetCurrentEpochMs() {
    auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

HardwareEventLogger::HardwareEventLogger(size_t maxCapacity)
    : m_maxCapacity(maxCapacity)
{
}

void HardwareEventLogger::Log(const std::string& subsystem, const std::string& severity, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    HardwareLogEntry entry;
    entry.timestampMs = GetCurrentEpochMs();
    entry.timestampUs = common::GetMonotonicMicroseconds();
    entry.timestampIso = GetCurrentTimestampIso();
    entry.subsystem = subsystem;
    entry.severity = severity;
    entry.message = message;

    m_logs.push_back(entry);
    if (m_logs.size() > m_maxCapacity) {
        m_logs.pop_front();
    }
}

void HardwareEventLogger::Info(const std::string& subsystem, const std::string& message) {
    Log(subsystem, "INFO", message);
}

void HardwareEventLogger::Warn(const std::string& subsystem, const std::string& message) {
    Log(subsystem, "WARN", message);
}

void HardwareEventLogger::Error(const std::string& subsystem, const std::string& message) {
    Log(subsystem, "ERROR", message);
}

void HardwareEventLogger::Critical(const std::string& subsystem, const std::string& message) {
    Log(subsystem, "CRITICAL", message);
}

void HardwareEventLogger::LogBootEvent(const std::string& stepName, bool passed, uint64_t durationUs, const std::string& details) {
    std::string sev = passed ? "INFO" : "CRITICAL";
    std::string msg = "[BOOT] Step: " + stepName + " | Result: " + (passed ? "PASS" : "FAIL") +
                      " | Duration: " + std::to_string(durationUs) + " us | " + details;
    Log("BOOT", sev, msg);
}

void HardwareEventLogger::LogRelayPost(uint8_t channel, bool passed, uint64_t durationUs, const std::string& details) {
    std::string sev = passed ? "INFO" : "ERROR";
    std::string msg = "[RELAY_POST] Channel " + std::to_string(channel) + " | Status: " +
                      (passed ? "PASS" : "FAIL") + " | SwitchTime: " + std::to_string(durationUs) +
                      " us | " + details;
    Log("RELAY_POST", sev, msg);
}

void HardwareEventLogger::LogRegistrationEvent(const std::string& nodeId, const std::string& ip, bool success) {
    std::string sev = success ? "INFO" : "ERROR";
    std::string msg = "[REGISTRATION] Node: " + nodeId + " | IP: " + ip + " | " +
                      (success ? "Registered Successfully" : "Registration Rejected (Conflict)");
    Log("REGISTRATION", sev, msg);
}

void HardwareEventLogger::LogRecoveryEvent(const std::string& nodeId, const std::string& details) {
    std::string msg = "[RECOVERY] Node: " + nodeId + " | Reconnected & State Synchronized | " + details;
    Log("RECOVERY", "INFO", msg);
}

void HardwareEventLogger::LogWatchdogEvent(const std::string& eventType, const std::string& details) {
    std::string sev = (eventType == "SUPERVISION_PASS") ? "INFO" : "CRITICAL";
    std::string msg = "[WATCHDOG] Event: " + eventType + " | " + details;
    Log("WATCHDOG", sev, msg);
}

void HardwareEventLogger::LogShutdownEvent(const std::string& stepName, bool success) {
    std::string sev = success ? "INFO" : "ERROR";
    std::string msg = "[SHUTDOWN] Step: " + stepName + " | Status: " + (success ? "PASS" : "FAIL");
    Log("SHUTDOWN", sev, msg);
}

void HardwareEventLogger::LogSubsystemFailure(const std::string& subsystem, const std::string& reason) {
    std::string msg = "[FAILURE] Subsystem: " + subsystem + " | Reason: " + reason;
    Log(subsystem + "_FAIL", "CRITICAL", msg);
}

std::vector<HardwareLogEntry> HardwareEventLogger::GetRecentLogs(size_t count) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<HardwareLogEntry> result;
    if (m_logs.empty()) {
        return result;
    }
    size_t take = std::min(count, m_logs.size());
    auto it = m_logs.end() - static_cast<ptrdiff_t>(take);
    result.assign(it, m_logs.end());
    return result;
}

std::vector<HardwareLogEntry> HardwareEventLogger::GetAllLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::vector<HardwareLogEntry>(m_logs.begin(), m_logs.end());
}

void HardwareEventLogger::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logs.clear();
}

size_t HardwareEventLogger::GetLogCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logs.size();
}

std::string HardwareEventLogger::ExportToCsv() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "Timestamp,Subsystem,Severity,Message\n";
    for (const auto& log : m_logs) {
        oss << "\"" << log.timestampIso << "\","
            << "\"" << log.subsystem << "\","
            << "\"" << log.severity << "\","
            << "\"" << log.message << "\"\n";
    }
    return oss.str();
}

std::string HardwareEventLogger::ExportToJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "[\n";
    for (size_t i = 0; i < m_logs.size(); ++i) {
        const auto& log = m_logs[i];
        oss << "  {\n"
            << "    \"timestamp\": \"" << log.timestampIso << "\",\n"
            << "    \"timestampMs\": " << log.timestampMs << ",\n"
            << "    \"timestampUs\": " << log.timestampUs << ",\n"
            << "    \"subsystem\": \"" << log.subsystem << "\",\n"
            << "    \"severity\": \"" << log.severity << "\",\n"
            << "    \"message\": \"" << log.message << "\"\n"
            << "  }" << (i + 1 < m_logs.size() ? "," : "") << "\n";
    }
    oss << "]\n";
    return oss.str();
}

} // namespace hardware
} // namespace aegis

