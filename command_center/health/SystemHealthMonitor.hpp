/**
 * @file SystemHealthMonitor.hpp
 * @brief System-wide health supervisor aggregating hardware and software health states.
 * Evaluates relays, optical IR sensors, OLED, buzzer, switches, GPIO drivers, TCP connections, and nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

enum class HealthStatusGrade {
    HEALTHY,
    WARNING,
    FAILED
};

inline std::string HealthStatusGradeToString(HealthStatusGrade grade) {
    switch (grade) {
        case HealthStatusGrade::HEALTHY: return "HEALTHY";
        case HealthStatusGrade::WARNING: return "WARNING";
        case HealthStatusGrade::FAILED:  return "FAILED";
        default:                         return "UNKNOWN";
    }
}

struct SubsystemHealthReport {
    std::string subsystemName;
    HealthStatusGrade status{HealthStatusGrade::HEALTHY};
    std::string details{"Operational / Nominal"};
    uint64_t lastEvaluatedMonoMs{0};
};

class SystemHealthMonitor {
public:
    SystemHealthMonitor();
    ~SystemHealthMonitor() = default;

    // Prevent copying
    SystemHealthMonitor(const SystemHealthMonitor&) = delete;
    SystemHealthMonitor& operator=(const SystemHealthMonitor&) = delete;

    /**
     * @brief Initialize standard monitored components list.
     */
    void InitializeDefaultSubsystems();

    /**
     * @brief Ingest a health update for a specific hardware or software component.
     */
    bool ReportHealth(const std::string& subsystemName,
                      HealthStatusGrade status,
                      const std::string& details = "Operational",
                      uint64_t timestampMs = 0);

    /**
     * @brief Fetch health report for single subsystem.
     */
    std::optional<SubsystemHealthReport> GetSubsystemHealth(const std::string& subsystemName) const;

    /**
     * @brief Retrieve snapshot of all monitored subsystems.
     */
    std::vector<SubsystemHealthReport> GetAllReports() const;

    /**
     * @brief Compute overall composite system grade (worst-of evaluation).
     */
    HealthStatusGrade GetOverallHealth() const;

    /**
     * @brief Clear all reports.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, SubsystemHealthReport> reports_;
    std::vector<std::string> displayOrder_;
};

} // namespace aegis::command_center
