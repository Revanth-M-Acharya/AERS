/**
 * @file CentralizedDeploymentLogger.cpp
 * @brief Implementation of CentralizedDeploymentLogger.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/logging/CentralizedDeploymentLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::hardware_integration {

CentralizedDeploymentLogger::CentralizedDeploymentLogger() = default;

static inline std::string LevelToString(DeploymentLogLevel l) {
    switch (l) {
        case DeploymentLogLevel::DEBUG:    return "DEBUG";
        case DeploymentLogLevel::INFO:     return "INFO";
        case DeploymentLogLevel::WARN:     return "WARN";
        case DeploymentLogLevel::ERROR:    return "ERROR";
        case DeploymentLogLevel::CRITICAL: return "CRITICAL";
    }
    return "INFO";
}

void CentralizedDeploymentLogger::Log(const std::string& nodeId,
                                     const std::string& subsystem,
                                     DeploymentLogLevel level,
                                     const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    DeploymentLogEntry e;
    e.timestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    e.nodeId = nodeId;
    e.subsystem = subsystem;
    e.level = level;
    e.message = message;
    logs_.push_back(e);
}

void CentralizedDeploymentLogger::RecordCommissioning(const CommissioningReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    commissioningReports_.push_back(report);

    DeploymentLogEntry e;
    e.timestampMonoMs = report.startTimestampMonoMs;
    e.nodeId = report.nodeId;
    e.subsystem = "COMMISSIONING";
    e.level = report.overallSuccess ? DeploymentLogLevel::INFO : DeploymentLogLevel::ERROR;
    e.message = report.summary + " (" + std::to_string(report.passedSteps) + "/" +
                std::to_string(report.totalSteps) + " steps passed in " +
                std::to_string(report.totalDurationMs) + " ms)";
    logs_.push_back(e);
}

void CentralizedDeploymentLogger::RecordValidation(const DeploymentValidationReport& report) {
    std::lock_guard<std::mutex> lock(mutex_);
    validationReports_.push_back(report);

    DeploymentLogEntry e;
    e.timestampMonoMs = report.timestampMonoMs;
    e.nodeId = "CLUSTER_VALIDATOR";
    e.subsystem = "DEPLOYMENT_AUDIT";
    e.level = report.overallPassed ? DeploymentLogLevel::INFO : DeploymentLogLevel::CRITICAL;
    e.message = report.auditSummary + " (" + std::to_string(report.passedVectors) + "/" +
                std::to_string(report.totalVectors) + " vectors passed)";
    logs_.push_back(e);
}

std::string CentralizedDeploymentLogger::GenerateMarkdownReport(const std::string& clusterName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;

    ss << "# SMART AEGIS: Cluster Deployment & Commissioning Report\n\n";
    ss << "**Cluster Identifier:** `" << clusterName << "`  \n";
    ss << "**Target Hardware:** Raspberry Pi 4 Model B (4-Node Cluster)  \n";
    ss << "**Operating System:** QNX Neutrino RTOS 8.0 (`aarch64le`)  \n";
    ss << "**Generated Timestamp (Mono):** " << common::TimeUtils::GetMonotonicTimeMs() << " ms  \n\n";

    ss << "## 1. Commissioning Audit Summary\n\n";
    ss << "| Node ID | Role | Commissioning Status | Steps Passed | Total Duration |\n";
    ss << "|---|---|---|---|---|\n";
    for (const auto& c : commissioningReports_) {
        ss << "| `" << c.nodeId << "` | " << c.nodeRole << " | **"
           << (c.overallSuccess ? "PASS" : "FAIL") << "** | "
           << c.passedSteps << " / " << c.totalSteps << " | "
           << c.totalDurationMs << " ms |\n";
    }

    ss << "\n## 2. 7-Vector Deployment Validation Summary\n\n";
    for (const auto& v : validationReports_) {
        ss << "**Overall Status:** " << (v.overallPassed ? "**CERTIFIED**" : "**REJECTED**")
           << " (" << v.passedVectors << "/" << v.totalVectors << " vectors)\n\n";
        ss << "| Validation Vector | Status | Latency | Diagnostic Message |\n";
        ss << "|---|---|---|---|\n";
        for (const auto& r : v.results) {
            ss << "| " << r.vectorName << " | **" << (r.passed ? "PASS" : "FAIL") << "** | "
               << r.latencyUs << " µs | " << r.diagnosticMessage << " |\n";
        }
    }

    ss << "\n## 3. Deployment Event Log (Recent Entries)\n\n";
    ss << "```text\n";
    size_t count = 0;
    for (auto it = logs_.rbegin(); it != logs_.rend() && count < 30; ++it, ++count) {
        ss << "[" << it->timestampMonoMs << " ms] [" << LevelToString(it->level)
           << "] [" << it->nodeId << "::" << it->subsystem << "] "
           << it->message << "\n";
    }
    ss << "```\n";

    return ss.str();
}

std::vector<DeploymentLogEntry> CentralizedDeploymentLogger::GetLogEntries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return logs_;
}

void CentralizedDeploymentLogger::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    logs_.clear();
    commissioningReports_.clear();
    validationReports_.clear();
}

} // namespace aegis::hardware_integration
