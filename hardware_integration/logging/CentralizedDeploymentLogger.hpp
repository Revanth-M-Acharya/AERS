/**
 * @file CentralizedDeploymentLogger.hpp
 * @brief Centralized Deployment & Commissioning Logger for SMART AEGIS Cluster.
 * Aggregates hardware events, self-test logs, and multi-node telemetry into audit archives.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/commissioning/CommissioningManager.hpp"
#include "hardware_integration/validation/DeploymentValidator.hpp"
#include <string>
#include <vector>
#include <mutex>

namespace aegis::hardware_integration {

enum class DeploymentLogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR,
    CRITICAL
};

struct DeploymentLogEntry {
    uint64_t timestampMonoMs{0};
    std::string nodeId;
    std::string subsystem;
    DeploymentLogLevel level{DeploymentLogLevel::INFO};
    std::string message;
};

class CentralizedDeploymentLogger {
public:
    CentralizedDeploymentLogger();
    ~CentralizedDeploymentLogger() = default;

    /**
     * @brief Log an entry from a cluster node.
     */
    void Log(const std::string& nodeId,
             const std::string& subsystem,
             DeploymentLogLevel level,
             const std::string& message);

    /**
     * @brief Record commissioning report into centralized log.
     */
    void RecordCommissioning(const CommissioningReport& report);

    /**
     * @brief Record deployment validation audit report.
     */
    void RecordValidation(const DeploymentValidationReport& report);

    /**
     * @brief Generate comprehensive Markdown deployment report.
     */
    std::string GenerateMarkdownReport(const std::string& clusterName = "Downtown_Metropolitan_Cluster") const;

    /**
     * @brief Export all aggregated log entries.
     */
    std::vector<DeploymentLogEntry> GetLogEntries() const;

    /**
     * @brief Clear in-memory log buffer.
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::vector<DeploymentLogEntry> logs_;
    std::vector<CommissioningReport> commissioningReports_;
    std::vector<DeploymentValidationReport> validationReports_;
};

} // namespace aegis::hardware_integration
