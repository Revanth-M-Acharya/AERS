/**
 * @file HealthSupervisor.hpp
 * @brief System-wide health monitoring supervisor for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class HealthStatus {
    HEALTHY,
    DEGRADED,
    CRITICAL,
    EMERGENCY_FAULT
};

inline std::string HealthStatusToString(HealthStatus status) {
    switch (status) {
        case HealthStatus::HEALTHY:         return "HEALTHY";
        case HealthStatus::DEGRADED:        return "DEGRADED";
        case HealthStatus::CRITICAL:        return "CRITICAL";
        case HealthStatus::EMERGENCY_FAULT: return "EMERGENCY_FAULT";
        default:                            return "UNKNOWN";
    }
}

struct ComponentHealth {
    std::string componentName;
    HealthStatus status{HealthStatus::HEALTHY};
    std::string message{"Operational"};
    uint64_t lastCheckTimestampMs{0};
    uint32_t consecutiveFailures{0};
};

using HealthProbeFn = std::function<ComponentHealth()>;
using HealthAlertCallback = std::function<void(const std::string& component, HealthStatus oldStatus, HealthStatus newStatus, const std::string& reason)>;

class HealthSupervisor {
public:
    HealthSupervisor();
    ~HealthSupervisor() = default;

    void RegisterProbe(const std::string& componentName, HealthProbeFn probe);
    void UnregisterProbe(const std::string& componentName);

    void SetAlertCallback(HealthAlertCallback callback);

    /**
     * @brief Execute all registered probes and update overall health status.
     * @return Aggregate system health status.
     */
    HealthStatus RunHealthChecks();

    HealthStatus GetOverallStatus() const;
    ComponentHealth GetComponentHealth(const std::string& componentName) const;
    std::vector<ComponentHealth> GetAllComponentHealth() const;

    void SetCpuThresholds(double degradedPercent, double criticalPercent);
    void SetMemoryThresholds(uint64_t degradedKb, uint64_t criticalKb);

    std::string ToJson() const;
    std::string ToAsciiDashboard() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, HealthProbeFn> probes_;
    std::unordered_map<std::string, ComponentHealth> componentHealth_;
    HealthStatus overallStatus_{HealthStatus::HEALTHY};
    HealthAlertCallback alertCallback_;

    double cpuDegradedThreshold_{75.0};
    double cpuCriticalThreshold_{90.0};
    uint64_t memDegradedKb_{512 * 1024};
    uint64_t memCriticalKb_{1024 * 1024};
};

} // namespace aegis::production

