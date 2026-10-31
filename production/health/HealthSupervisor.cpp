/**
 * @file HealthSupervisor.cpp
 * @brief Implementation of system-wide health monitoring supervisor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/health/HealthSupervisor.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace aegis::production {

HealthSupervisor::HealthSupervisor() = default;

void HealthSupervisor::RegisterProbe(const std::string& componentName, HealthProbeFn probe) {
    std::lock_guard<std::mutex> lock(mutex_);
    probes_[componentName] = std::move(probe);
    ComponentHealth initial;
    initial.componentName = componentName;
    initial.status = HealthStatus::HEALTHY;
    initial.message = "Registered";
    initial.lastCheckTimestampMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    componentHealth_[componentName] = initial;
}

void HealthSupervisor::UnregisterProbe(const std::string& componentName) {
    std::lock_guard<std::mutex> lock(mutex_);
    probes_.erase(componentName);
    componentHealth_.erase(componentName);
}

void HealthSupervisor::SetAlertCallback(HealthAlertCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    alertCallback_ = std::move(callback);
}

HealthStatus HealthSupervisor::RunHealthChecks() {
    std::unordered_map<std::string, HealthProbeFn> probesCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        probesCopy = probes_;
    }

    HealthStatus highestSeverity = HealthStatus::HEALTHY;
    std::vector<std::tuple<std::string, HealthStatus, HealthStatus, std::string>> alertEvents;

    for (const auto& [name, probe] : probesCopy) {
        ComponentHealth result;
        try {
            result = probe();
        } catch (const std::exception& e) {
            result.componentName = name;
            result.status = HealthStatus::EMERGENCY_FAULT;
            result.message = std::string("Probe exception: ") + e.what();
        } catch (...) {
            result.componentName = name;
            result.status = HealthStatus::EMERGENCY_FAULT;
            result.message = "Unknown probe failure";
        }

        result.lastCheckTimestampMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

        std::lock_guard<std::mutex> lock(mutex_);
        auto it = componentHealth_.find(name);
        HealthStatus oldStatus = (it != componentHealth_.end()) ? it->second.status : HealthStatus::HEALTHY;

        if (result.status != HealthStatus::HEALTHY) {
            result.consecutiveFailures = (it != componentHealth_.end()) ? it->second.consecutiveFailures + 1 : 1;
        } else {
            result.consecutiveFailures = 0;
        }

        if (oldStatus != result.status) {
            alertEvents.emplace_back(name, oldStatus, result.status, result.message);
        }

        componentHealth_[name] = result;

        if (result.status == HealthStatus::EMERGENCY_FAULT) {
            highestSeverity = HealthStatus::EMERGENCY_FAULT;
        } else if (result.status == HealthStatus::CRITICAL && highestSeverity != HealthStatus::EMERGENCY_FAULT) {
            highestSeverity = HealthStatus::CRITICAL;
        } else if (result.status == HealthStatus::DEGRADED && highestSeverity == HealthStatus::HEALTHY) {
            highestSeverity = HealthStatus::DEGRADED;
        }
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        overallStatus_ = highestSeverity;
    }

    for (const auto& [comp, oldS, newS, reason] : alertEvents) {
        if (alertCallback_) {
            alertCallback_(comp, oldS, newS, reason);
        }
    }

    return highestSeverity;
}

HealthStatus HealthSupervisor::GetOverallStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return overallStatus_;
}

ComponentHealth HealthSupervisor::GetComponentHealth(const std::string& componentName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = componentHealth_.find(componentName);
    if (it != componentHealth_.end()) {
        return it->second;
    }
    ComponentHealth notFound;
    notFound.componentName = componentName;
    notFound.status = HealthStatus::CRITICAL;
    notFound.message = "Component not found";
    return notFound;
}

std::vector<ComponentHealth> HealthSupervisor::GetAllComponentHealth() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ComponentHealth> list;
    list.reserve(componentHealth_.size());
    for (const auto& [k, v] : componentHealth_) {
        list.push_back(v);
    }
    return list;
}

void HealthSupervisor::SetCpuThresholds(double degradedPercent, double criticalPercent) {
    std::lock_guard<std::mutex> lock(mutex_);
    cpuDegradedThreshold_ = degradedPercent;
    cpuCriticalThreshold_ = criticalPercent;
}

void HealthSupervisor::SetMemoryThresholds(uint64_t degradedKb, uint64_t criticalKb) {
    std::lock_guard<std::mutex> lock(mutex_);
    memDegradedKb_ = degradedKb;
    memCriticalKb_ = criticalKb;
}

std::string HealthSupervisor::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"overall_status\": \"" << HealthStatusToString(overallStatus_) << "\",\n"
       << "  \"components\": [\n";

    bool first = true;
    for (const auto& [name, h] : componentHealth_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"name\": \"" << name << "\",\n"
           << "      \"status\": \"" << HealthStatusToString(h.status) << "\",\n"
           << "      \"message\": \"" << h.message << "\",\n"
           << "      \"failures\": " << h.consecutiveFailures << ",\n"
           << "      \"timestamp_ms\": " << h.lastCheckTimestampMs << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

std::string HealthSupervisor::ToAsciiDashboard() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "+----------------------------------------------------------------------+\n"
       << "| SMART AEGIS SYSTEM HEALTH SUPERVISOR                                 |\n"
       << "+----------------------------------------------------------------------+\n"
       << "| System Status: " << std::left << std::setw(15) << HealthStatusToString(overallStatus_)
       << " Total Probes: " << std::setw(30) << componentHealth_.size() << "|\n"
       << "+----------------------------------------------------------------------+\n"
       << "| COMPONENT                 | STATUS      | FAILURES | MESSAGE         |\n"
       << "+---------------------------+-------------+----------+-----------------+\n";

    for (const auto& [name, h] : componentHealth_) {
        std::string msg = h.message;
        if (msg.size() > 16) msg = msg.substr(0, 13) + "...";
        ss << "| " << std::left << std::setw(25) << name.substr(0, 25)
           << " | " << std::setw(11) << HealthStatusToString(h.status)
           << " | " << std::setw(8) << h.consecutiveFailures
           << " | " << std::setw(15) << msg << " |\n";
    }
    ss << "+----------------------------------------------------------------------+\n";
    return ss.str();
}

} // namespace aegis::production

