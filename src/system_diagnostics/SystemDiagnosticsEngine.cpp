/**
 * @file SystemDiagnosticsEngine.cpp
 * @brief Implementation of SMART AEGIS (AERS) Phase 14B System Diagnostics & Predictive Maintenance Engine
 *
 * Pure C++17, QNX SDP 8.0 & POSIX compliant.
 * Zero third-party dependencies.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "SystemDiagnosticsEngine.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis {
namespace diagnostics {

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(DiagnosticState state) {
    switch (state) {
        case DiagnosticState::HEALTHY:  return "HEALTHY";
        case DiagnosticState::WARNING:  return "WARNING";
        case DiagnosticState::CRITICAL: return "CRITICAL";
        case DiagnosticState::FAILED:   return "FAILED";
        default:                        return "UNKNOWN";
    }
}

std::string ToString(ComponentType type) {
    switch (type) {
        case ComponentType::COORDINATOR:       return "COORDINATOR";
        case ComponentType::RPI_CLUSTER:       return "RPI_CLUSTER";
        case ComponentType::TRAFFIC_NETWORK:   return "TRAFFIC_NETWORK";
        case ComponentType::CONSENSUS:         return "CONSENSUS";
        case ComponentType::SIGNAL_CONTROLLER: return "SIGNAL_CONTROLLER";
        case ComponentType::INTERSECTION:      return "INTERSECTION";
        case ComponentType::COMMUNICATION:     return "COMMUNICATION";
        case ComponentType::HOSPITAL:          return "HOSPITAL";
        case ComponentType::AMBULANCE:         return "AMBULANCE";
        case ComponentType::UNKNOWN:
        default:                               return "UNKNOWN";
    }
}

std::string ToString(MaintenancePriority priority) {
    switch (priority) {
        case MaintenancePriority::NONE:      return "NONE";
        case MaintenancePriority::LOW:       return "LOW";
        case MaintenancePriority::MEDIUM:    return "MEDIUM";
        case MaintenancePriority::HIGH:      return "HIGH";
        case MaintenancePriority::IMMEDIATE: return "IMMEDIATE";
        default:                             return "NONE";
    }
}

DiagnosticState DiagnosticStateFromString(const std::string& str) {
    if (str == "HEALTHY")  return DiagnosticState::HEALTHY;
    if (str == "WARNING")  return DiagnosticState::WARNING;
    if (str == "CRITICAL") return DiagnosticState::CRITICAL;
    if (str == "FAILED")   return DiagnosticState::FAILED;
    return DiagnosticState::HEALTHY;
}

ComponentType ComponentTypeFromString(const std::string& str) {
    if (str == "COORDINATOR")       return ComponentType::COORDINATOR;
    if (str == "RPI_CLUSTER")       return ComponentType::RPI_CLUSTER;
    if (str == "TRAFFIC_NETWORK")   return ComponentType::TRAFFIC_NETWORK;
    if (str == "CONSENSUS")         return ComponentType::CONSENSUS;
    if (str == "SIGNAL_CONTROLLER") return ComponentType::SIGNAL_CONTROLLER;
    if (str == "INTERSECTION")      return ComponentType::INTERSECTION;
    if (str == "COMMUNICATION")     return ComponentType::COMMUNICATION;
    if (str == "HOSPITAL")          return ComponentType::HOSPITAL;
    if (str == "AMBULANCE")         return ComponentType::AMBULANCE;
    return ComponentType::UNKNOWN;
}

MaintenancePriority MaintenancePriorityFromString(const std::string& str) {
    if (str == "IMMEDIATE") return MaintenancePriority::IMMEDIATE;
    if (str == "HIGH")      return MaintenancePriority::HIGH;
    if (str == "MEDIUM")    return MaintenancePriority::MEDIUM;
    if (str == "LOW")       return MaintenancePriority::LOW;
    return MaintenancePriority::NONE;
}

// =============================================================================
// Constructor & Reset
// =============================================================================

SystemDiagnosticsEngine::SystemDiagnosticsEngine() {
    Reset();
}

void SystemDiagnosticsEngine::Reset() {
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        components_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(diagnosticsMutex_);
        diagnosticHistory_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(maintenanceMutex_);
        maintenanceRecommendations_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_ = DiagnosticStatistics();
        totalCpuSum_ = 0.0;
        totalCpuSamples_ = 0;
        totalTempSum_ = 0.0;
        totalTempSamples_ = 0;
        totalLatencySum_ = 0.0;
        totalLatencySamples_ = 0;
        totalPacketLossSum_ = 0.0;
        totalPacketLossSamples_ = 0;
    }
}

uint64_t SystemDiagnosticsEngine::GetCurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// =============================================================================
// Multi-Metric Health Evaluation (Rules 2, 3, 4)
// =============================================================================

DiagnosticState SystemDiagnosticsEngine::EvaluateMultiMetricHealth(const ComponentHealth& comp) const {
    if (!comp.isOnline || comp.diagnosticState == DiagnosticState::FAILED) {
        return DiagnosticState::FAILED;
    }

    // Rule 3: Temperature over threshold -> CRITICAL (takes precedence)
    if (comp.temperature >= criticalTempThresholdC_) {
        return DiagnosticState::CRITICAL;
    }

    // Critical CPU or Packet Loss or RAM/Storage exhaustion
    if (comp.cpuUsage >= criticalCpuThresholdPct_ ||
        comp.packetLoss >= criticalPacketLossPct_ ||
        comp.ramUsage >= criticalRamThresholdPct_ ||
        comp.storageUsage >= criticalStorageThresholdPct_) {
        return DiagnosticState::CRITICAL;
    }

    // Rule 2 & Rule 4: CPU, Temperature, Packet Loss, RAM, Storage warnings
    if (comp.cpuUsage >= warningCpuThresholdPct_ ||
        comp.temperature >= warningTempThresholdC_ ||
        comp.packetLoss >= warningPacketLossPct_ ||
        comp.ramUsage >= warningRamThresholdPct_ ||
        comp.storageUsage >= warningStorageThresholdPct_ ||
        !comp.isSynchronized) {
        return DiagnosticState::WARNING;
    }

    return DiagnosticState::HEALTHY;
}

// =============================================================================
// Component Registration & Lifecycle
// =============================================================================

bool SystemDiagnosticsEngine::RegisterComponent(const ComponentHealth& component) {
    if (component.componentId.empty()) {
        return false;
    }

    uint64_t now = GetCurrentTimeMs();
    ComponentHealth copy = component;
    if (copy.lastHeartbeat == 0) {
        copy.lastHeartbeat = now;
    }

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        if (components_.find(copy.componentId) != components_.end()) {
            return false; // Duplicate registration rejected
        }
        components_[copy.componentId] = copy;
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.totalComponents++;
        if (copy.diagnosticState == DiagnosticState::HEALTHY) statistics_.healthyComponents++;
        else if (copy.diagnosticState == DiagnosticState::WARNING) statistics_.warningComponents++;
        else if (copy.diagnosticState == DiagnosticState::CRITICAL) statistics_.criticalComponents++;
        else if (copy.diagnosticState == DiagnosticState::FAILED) statistics_.failedComponents++;

        if (copy.diagnosticState == DiagnosticState::CRITICAL || copy.diagnosticState == DiagnosticState::FAILED || !copy.isOnline) {
            statistics_.systemReady = false;
        }
    }

    return true;
}

bool SystemDiagnosticsEngine::RegisterComponent(const std::string& componentId,
                                                ComponentType type,
                                                const std::string& name) {
    ComponentHealth comp(componentId, type, name);
    return RegisterComponent(comp);
}

bool SystemDiagnosticsEngine::RemoveComponent(const std::string& componentId) {
    ComponentHealth removed;
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            removed = it->second;
            components_.erase(it);
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        if (statistics_.totalComponents > 0) statistics_.totalComponents--;
        if (removed.diagnosticState == DiagnosticState::HEALTHY && statistics_.healthyComponents > 0) {
            statistics_.healthyComponents--;
        } else if (removed.diagnosticState == DiagnosticState::WARNING && statistics_.warningComponents > 0) {
            statistics_.warningComponents--;
        } else if (removed.diagnosticState == DiagnosticState::CRITICAL && statistics_.criticalComponents > 0) {
            statistics_.criticalComponents--;
        } else if (removed.diagnosticState == DiagnosticState::FAILED && statistics_.failedComponents > 0) {
            statistics_.failedComponents--;
        }
    }

    return found;
}

// =============================================================================
// Health & Telemetry Updates
// =============================================================================

bool SystemDiagnosticsEngine::UpdateComponentHealth(const std::string& componentId, DiagnosticState state) {
    bool found = false;
    DiagnosticState prevState = DiagnosticState::HEALTHY;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            prevState = it->second.diagnosticState;
            it->second.diagnosticState = state;
            if (state == DiagnosticState::FAILED) {
                it->second.isOnline = false;
            } else {
                it->second.isOnline = true;
            }
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        // Update state bucket counts
        if (prevState == DiagnosticState::HEALTHY && statistics_.healthyComponents > 0) statistics_.healthyComponents--;
        else if (prevState == DiagnosticState::WARNING && statistics_.warningComponents > 0) statistics_.warningComponents--;
        else if (prevState == DiagnosticState::CRITICAL && statistics_.criticalComponents > 0) statistics_.criticalComponents--;
        else if (prevState == DiagnosticState::FAILED && statistics_.failedComponents > 0) statistics_.failedComponents--;

        if (state == DiagnosticState::HEALTHY) statistics_.healthyComponents++;
        else if (state == DiagnosticState::WARNING) statistics_.warningComponents++;
        else if (state == DiagnosticState::CRITICAL) statistics_.criticalComponents++;
        else if (state == DiagnosticState::FAILED) statistics_.failedComponents++;

        // Rule 8: Critical or failed components immediately reduce system readiness
        if (state == DiagnosticState::CRITICAL || state == DiagnosticState::FAILED) {
            statistics_.systemReady = false;
            statistics_.systemReadinessScore = std::max(0.0, statistics_.systemReadinessScore - 25.0);
        }
    }

    return found;
}

bool SystemDiagnosticsEngine::UpdateHeartbeat(const std::string& componentId, double latencyMs) {
    bool found = false;
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            it->second.lastHeartbeat = now;
            it->second.networkLatency = latencyMs;
            it->second.heartbeatDelay = 0.0;
            it->second.isOnline = true;
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        totalLatencySum_ += latencyMs;
        totalLatencySamples_++;
        statistics_.averageLatency = totalLatencySum_ / static_cast<double>(totalLatencySamples_);
    }

    return found;
}

bool SystemDiagnosticsEngine::UpdateTelemetry(const std::string& componentId,
                                             double cpuUsage,
                                             double ramUsage,
                                             double storageUsage,
                                             double temperature,
                                             double packetLoss) {
    bool found = false;
    DiagnosticState newState = DiagnosticState::HEALTHY;
    DiagnosticState oldState = DiagnosticState::HEALTHY;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            oldState = it->second.diagnosticState;
            it->second.cpuUsage = cpuUsage;
            it->second.ramUsage = ramUsage;
            it->second.storageUsage = storageUsage;
            it->second.temperature = temperature;
            it->second.packetLoss = packetLoss;

            // Deterministic multi-metric health evaluation
            newState = EvaluateMultiMetricHealth(it->second);
            it->second.diagnosticState = newState;
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        totalCpuSum_ += cpuUsage;
        totalCpuSamples_++;
        statistics_.averageCpuUsage = totalCpuSum_ / static_cast<double>(totalCpuSamples_);

        totalTempSum_ += temperature;
        totalTempSamples_++;
        statistics_.averageTemperature = totalTempSum_ / static_cast<double>(totalTempSamples_);

        totalPacketLossSum_ += packetLoss;
        totalPacketLossSamples_++;
        statistics_.averagePacketLoss = totalPacketLossSum_ / static_cast<double>(totalPacketLossSamples_);

        if (oldState != newState) {
            if (oldState == DiagnosticState::HEALTHY && statistics_.healthyComponents > 0) statistics_.healthyComponents--;
            else if (oldState == DiagnosticState::WARNING && statistics_.warningComponents > 0) statistics_.warningComponents--;
            else if (oldState == DiagnosticState::CRITICAL && statistics_.criticalComponents > 0) statistics_.criticalComponents--;
            else if (oldState == DiagnosticState::FAILED && statistics_.failedComponents > 0) statistics_.failedComponents--;

            if (newState == DiagnosticState::HEALTHY) statistics_.healthyComponents++;
            else if (newState == DiagnosticState::WARNING) statistics_.warningComponents++;
            else if (newState == DiagnosticState::CRITICAL) statistics_.criticalComponents++;
            else if (newState == DiagnosticState::FAILED) statistics_.failedComponents++;
        }
    }

    return found;
}

bool SystemDiagnosticsEngine::UpdateSyncState(const std::string& componentId, bool isSynchronized) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = components_.find(componentId);
    if (it != components_.end()) {
        it->second.isSynchronized = isSynchronized;
        if (!isSynchronized) {
            // Rule 7: Synchronization failure elevates state to at least WARNING
            if (it->second.diagnosticState == DiagnosticState::HEALTHY) {
                it->second.diagnosticState = DiagnosticState::WARNING;
            }
        }
        return true;
    }
    return false;
}

bool SystemDiagnosticsEngine::UpdateMemoryGrowth(const std::string& componentId, double memoryGrowthRate) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = components_.find(componentId);
    if (it != components_.end()) {
        it->second.memoryGrowthRate = memoryGrowthRate;
        return true;
    }
    return false;
}

bool SystemDiagnosticsEngine::RecordFailure(const std::string& componentId, const std::string& reason) {
    bool found = false;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            it->second.failureCount++;
            it->second.consecutiveFailures++;
            it->second.lastError = reason;
            found = true;
        }
    }

    return found;
}

// =============================================================================
// Diagnostics & Scanning (Rules 1, 2, 3, 4, 6, 7, 9)
// =============================================================================

DiagnosticRecord SystemDiagnosticsEngine::RunDiagnostics(const std::string& componentId) {
    ComponentHealth comp;
    bool found = false;
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            comp = it->second;
            found = true;
        }
    }

    DiagnosticRecord record;
    record.recordId = "DIAG_" + componentId + "_" + std::to_string(now);
    record.componentId = componentId;
    record.timestamp = now;

    if (!found) {
        record.state = DiagnosticState::FAILED;
        record.detectedIssues.push_back("Component not found in registry");
        record.recommendation = "Register component with SystemDiagnosticsEngine";
        return record;
    }

    record.componentType = comp.componentType;
    record.cpuUsage = comp.cpuUsage;
    record.ramUsage = comp.ramUsage;
    record.storageUsage = comp.storageUsage;
    record.temperature = comp.temperature;
    record.networkLatency = comp.networkLatency;
    record.packetLoss = comp.packetLoss;

    // Rule 1: Heartbeat timeout -> FAILED
    uint64_t elapsedSinceHeartbeat = (now >= comp.lastHeartbeat) ? (now - comp.lastHeartbeat) : 0;
    comp.heartbeatDelay = static_cast<double>(elapsedSinceHeartbeat);

    if (elapsedSinceHeartbeat > heartbeatTimeoutMs_) {
        comp.diagnosticState = DiagnosticState::FAILED;
        comp.isOnline = false;
        record.detectedIssues.push_back("Heartbeat timeout exceeded (" + std::to_string(elapsedSinceHeartbeat) + " ms)");
        record.predictedRisks.push_back("Node unreachable or crashed");
    }

    // Rule 3: Temperature over threshold -> CRITICAL
    if (comp.temperature >= criticalTempThresholdC_) {
        comp.diagnosticState = DiagnosticState::CRITICAL;
        record.detectedIssues.push_back("Critical temperature: " + std::to_string(comp.temperature) + " C");
        record.predictedRisks.push_back("Thermal throttling / hardware shutdown imminent");
    } else if (comp.temperature >= warningTempThresholdC_) {
        if (comp.diagnosticState != DiagnosticState::CRITICAL && comp.diagnosticState != DiagnosticState::FAILED) {
            comp.diagnosticState = DiagnosticState::WARNING;
        }
        record.detectedIssues.push_back("Elevated temperature: " + std::to_string(comp.temperature) + " C");
    }

    // Rule 2: CPU over threshold -> WARNING (or CRITICAL if extreme)
    if (comp.cpuUsage >= criticalCpuThresholdPct_) {
        if (comp.diagnosticState != DiagnosticState::FAILED) {
            comp.diagnosticState = DiagnosticState::CRITICAL;
        }
        record.detectedIssues.push_back("Extreme CPU usage: " + std::to_string(comp.cpuUsage) + "%");
        record.predictedRisks.push_back("Process starvation / starvation-induced watchdog trip");
    } else if (comp.cpuUsage >= warningCpuThresholdPct_) {
        if (comp.diagnosticState == DiagnosticState::HEALTHY) {
            comp.diagnosticState = DiagnosticState::WARNING;
        }
        record.detectedIssues.push_back("High CPU usage: " + std::to_string(comp.cpuUsage) + "%");
    }

    // Rule 4: Packet loss over threshold -> WARNING (or CRITICAL)
    if (comp.packetLoss >= criticalPacketLossPct_) {
        if (comp.diagnosticState != DiagnosticState::FAILED) {
            comp.diagnosticState = DiagnosticState::CRITICAL;
        }
        record.detectedIssues.push_back("Severe packet loss: " + std::to_string(comp.packetLoss) + "%");
        record.predictedRisks.push_back("Network partition / consensus abort risk");
    } else if (comp.packetLoss >= warningPacketLossPct_) {
        if (comp.diagnosticState == DiagnosticState::HEALTHY) {
            comp.diagnosticState = DiagnosticState::WARNING;
        }
        record.detectedIssues.push_back("Elevated packet loss: " + std::to_string(comp.packetLoss) + "%");
    }

    // Rule 6: Memory growth beyond threshold predicts memory leak
    if (comp.memoryGrowthRate >= memoryGrowthThreshold_ ||
        (comp.ramUsage >= warningRamThresholdPct_ && comp.memoryGrowthRate > 0.0)) {
        if (comp.diagnosticState == DiagnosticState::HEALTHY) {
            comp.diagnosticState = DiagnosticState::WARNING;
        }
        record.detectedIssues.push_back("High memory growth rate: " + std::to_string(comp.memoryGrowthRate) + " MB/min");
        record.predictedRisks.push_back("Potential memory leak detected");
    }

    // Rule 7: Synchronization failure blocks deployment
    if (!comp.isSynchronized) {
        if (comp.diagnosticState == DiagnosticState::HEALTHY) {
            comp.diagnosticState = DiagnosticState::WARNING;
        }
        record.detectedIssues.push_back("Clock/Configuration desynchronization");
        record.predictedRisks.push_back("Corridor timing drift / Consensus rejection risk");
    }

    // Rule 5: Repeated failures
    if (comp.consecutiveFailures >= 2 || comp.failureCount >= 3) {
        record.detectedIssues.push_back("Repeated subsystem failures (" + std::to_string(comp.failureCount) + " total)");
        record.predictedRisks.push_back("Hardware degradation or chronic software fault");
    }

    record.state = comp.diagnosticState;

    // Formulate actionable recommendation
    if (record.state == DiagnosticState::FAILED) {
        record.recommendation = "Reboot/re-initialize node and perform clock resynchronization";
    } else if (record.state == DiagnosticState::CRITICAL) {
        record.recommendation = "Immediate operator intervention: reduce load, inspect cooling or failover";
    } else if (record.state == DiagnosticState::WARNING) {
        record.recommendation = "Schedule inspection and monitor telemetry trends";
    } else {
        record.recommendation = "System operating within nominal parameters";
    }

    // Update component in registry
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        components_[componentId] = comp;
    }

    // Save diagnostic record
    {
        std::lock_guard<std::mutex> lock(diagnosticsMutex_);
        diagnosticHistory_.push_back(record);
    }

    // Rule 9: Automatically generate maintenance recommendations if issues exist
    if (record.state == DiagnosticState::CRITICAL || record.state == DiagnosticState::FAILED ||
        comp.consecutiveFailures >= 2 || comp.memoryGrowthRate >= memoryGrowthThreshold_) {
        MaintenanceRecommendation rec;
        rec.recommendationId = "REC_" + componentId + "_" + std::to_string(now);
        rec.componentId = componentId;
        rec.componentType = comp.componentType;
        rec.timestamp = now;

        if (record.state == DiagnosticState::FAILED) {
            rec.priority = MaintenancePriority::IMMEDIATE;
            rec.reason = "Node unresponsive or failed heartbeat";
            rec.recommendedAction = "Restart component and execute recovery";
            rec.predictedFailureType = "NODE_FAILURE";
        } else if (comp.temperature >= criticalTempThresholdC_) {
            rec.priority = MaintenancePriority::IMMEDIATE;
            rec.reason = "Critical operating temperature";
            rec.recommendedAction = "Inspect thermal heatsink, fan, and ambient ventilation";
            rec.predictedFailureType = "THERMAL_SHUTDOWN";
        } else if (comp.consecutiveFailures >= 2) {
            rec.priority = MaintenancePriority::HIGH;
            rec.reason = "Repeated operational failures detected";
            rec.recommendedAction = "Run hardware diagnostics and audit logs";
            rec.predictedFailureType = "HARDWARE_DEGRADATION";
        } else if (comp.memoryGrowthRate >= memoryGrowthThreshold_) {
            rec.priority = MaintenancePriority::HIGH;
            rec.reason = "Abnormal memory growth rate detected";
            rec.recommendedAction = "Profile heap allocation and restart daemon if necessary";
            rec.predictedFailureType = "MEMORY_LEAK";
        } else {
            rec.priority = MaintenancePriority::MEDIUM;
            rec.reason = "Component operating in degraded or warning state";
            rec.recommendedAction = "Inspect network connection and process resource limits";
            rec.predictedFailureType = "DEGRADED_OPERATION";
        }

        std::lock_guard<std::mutex> lock(maintenanceMutex_);
        maintenanceRecommendations_.push_back(rec);
    }

    // Update diagnostic statistics
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.totalDiagnosticsRun++;
    }

    return record;
}

std::vector<DiagnosticRecord> SystemDiagnosticsEngine::RunFullSystemScan() {
    std::vector<std::string> ids;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        ids.reserve(components_.size());
        for (const auto& pair : components_) {
            ids.push_back(pair.first);
        }
    }

    std::vector<DiagnosticRecord> scanResults;
    scanResults.reserve(ids.size());

    for (const auto& id : ids) {
        scanResults.push_back(RunDiagnostics(id));
    }

    // Refresh readiness score
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        uint32_t healthy = 0;
        uint32_t warning = 0;
        uint32_t critical = 0;
        uint32_t failed = 0;

        for (const auto& r : scanResults) {
            if (r.state == DiagnosticState::HEALTHY) healthy++;
            else if (r.state == DiagnosticState::WARNING) warning++;
            else if (r.state == DiagnosticState::CRITICAL) critical++;
            else if (r.state == DiagnosticState::FAILED) failed++;
        }

        statistics_.healthyComponents = healthy;
        statistics_.warningComponents = warning;
        statistics_.criticalComponents = critical;
        statistics_.failedComponents = failed;

        if (ids.empty()) {
            statistics_.systemReadinessScore = 0.0;
            statistics_.systemReady = false;
        } else {
            double penalty = (static_cast<double>(critical) * 35.0) +
                             (static_cast<double>(failed) * 50.0) +
                             (static_cast<double>(warning) * 10.0);
            statistics_.systemReadinessScore = std::max(0.0, 100.0 - penalty);
            statistics_.systemReady = (critical == 0 && failed == 0);
        }
    }

    return scanResults;
}

// =============================================================================
// Predictive Maintenance Engine (Rules 5, 6, 9)
// =============================================================================

std::vector<MaintenanceRecommendation> SystemDiagnosticsEngine::PredictFailures() {
    std::vector<ComponentHealth> comps;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        comps.reserve(components_.size());
        for (const auto& pair : components_) {
            comps.push_back(pair.second);
        }
    }

    uint64_t now = GetCurrentTimeMs();
    std::vector<MaintenanceRecommendation> generated;

    for (const auto& comp : comps) {
        // 1. Overheating prediction
        if (comp.temperature >= criticalTempThresholdC_) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_TEMP_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = MaintenancePriority::IMMEDIATE;
            rec.reason = "Critical temperature (" + std::to_string(comp.temperature) + " C)";
            rec.recommendedAction = "Inspect cooling fan and heat sink";
            rec.predictedFailureType = "THERMAL_SHUTDOWN";
            rec.timestamp = now;
            generated.push_back(rec);
        } else if (comp.temperature >= warningTempThresholdC_) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_TEMP_WARN_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = MaintenancePriority::MEDIUM;
            rec.reason = "Approaching thermal limit (" + std::to_string(comp.temperature) + " C)";
            rec.recommendedAction = "Clean ventilation ducts and monitor ambient temperature";
            rec.predictedFailureType = "OVERHEATING";
            rec.timestamp = now;
            generated.push_back(rec);
        }

        // 2. Rule 6: Memory leak prediction
        if (comp.memoryGrowthRate >= memoryGrowthThreshold_ ||
            (comp.ramUsage >= warningRamThresholdPct_ && comp.memoryGrowthRate > 0.0)) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_MEM_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = (comp.ramUsage >= criticalRamThresholdPct_) ? MaintenancePriority::IMMEDIATE : MaintenancePriority::HIGH;
            rec.reason = "Memory growth rate " + std::to_string(comp.memoryGrowthRate) + " MB/min";
            rec.recommendedAction = "Audit memory leaks and restart daemon before OOM";
            rec.predictedFailureType = "MEMORY_LEAK";
            rec.timestamp = now;
            generated.push_back(rec);
        }

        // 3. Rule 5: Repeated failures predict maintenance requirement
        if (comp.consecutiveFailures >= 2 || comp.failureCount >= 3) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_FAIL_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = MaintenancePriority::HIGH;
            rec.reason = "Repeated operational faults (" + std::to_string(comp.failureCount) + " failures)";
            rec.recommendedAction = "Replace edge hardware module or re-flash firmware";
            rec.predictedFailureType = "HARDWARE_DEGRADATION";
            rec.timestamp = now;
            generated.push_back(rec);
        }

        // 4. Communication instability / packet loss
        if (comp.packetLoss >= warningPacketLossPct_) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_NET_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = (comp.packetLoss >= criticalPacketLossPct_) ? MaintenancePriority::HIGH : MaintenancePriority::MEDIUM;
            rec.reason = "Elevated network packet loss (" + std::to_string(comp.packetLoss) + "%)";
            rec.recommendedAction = "Inspect Ethernet link, switch port, and RF antenna";
            rec.predictedFailureType = "COMMUNICATION_INSTABILITY";
            rec.timestamp = now;
            generated.push_back(rec);
        }

        // 5. Rule 7: Synchronization failure blocks deployment
        if (!comp.isSynchronized) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_SYNC_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = MaintenancePriority::HIGH;
            rec.reason = "Node desynchronized from master clock";
            rec.recommendedAction = "Execute NTP/PTP resync and re-verify corridor timing";
            rec.predictedFailureType = "SYNC_FAILURE";
            rec.timestamp = now;
            generated.push_back(rec);
        }

        // 6. Storage exhaustion
        if (comp.storageUsage >= warningStorageThresholdPct_) {
            MaintenanceRecommendation rec;
            rec.recommendationId = "PRED_STOR_" + comp.componentId + "_" + std::to_string(now);
            rec.componentId = comp.componentId;
            rec.componentType = comp.componentType;
            rec.priority = (comp.storageUsage >= criticalStorageThresholdPct_) ? MaintenancePriority::IMMEDIATE : MaintenancePriority::MEDIUM;
            rec.reason = "Storage utilization at " + std::to_string(comp.storageUsage) + "%";
            rec.recommendedAction = "Rotate forensic event logs and purge expired traces";
            rec.predictedFailureType = "STORAGE_EXHAUSTION";
            rec.timestamp = now;
            generated.push_back(rec);
        }
    }

    {
        std::lock_guard<std::mutex> lock(maintenanceMutex_);
        for (const auto& r : generated) {
            maintenanceRecommendations_.push_back(r);
        }
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.totalPredictionsGenerated += static_cast<uint32_t>(generated.size());
        statistics_.totalMaintenanceRecommendations += static_cast<uint32_t>(generated.size());
    }

    return generated;
}

std::vector<MaintenanceRecommendation> SystemDiagnosticsEngine::GenerateMaintenancePlan() {
    PredictFailures();

    std::vector<MaintenanceRecommendation> plan;
    {
        std::lock_guard<std::mutex> lock(maintenanceMutex_);
        plan = maintenanceRecommendations_;
    }

    // Sort deterministically by priority: IMMEDIATE > HIGH > MEDIUM > LOW > NONE
    std::sort(plan.begin(), plan.end(), [](const MaintenanceRecommendation& a, const MaintenanceRecommendation& b) {
        if (static_cast<int>(a.priority) != static_cast<int>(b.priority)) {
            return static_cast<int>(a.priority) > static_cast<int>(b.priority);
        }
        return a.timestamp < b.timestamp;
    });

    return plan;
}

// =============================================================================
// System Readiness Verification (Rules 8 & 10)
// =============================================================================

bool SystemDiagnosticsEngine::VerifySystemReadiness() const {
    std::vector<ComponentHealth> comps;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        if (components_.empty()) {
            return false;
        }
        comps.reserve(components_.size());
        for (const auto& pair : components_) {
            comps.push_back(pair.second);
        }
    }

    // Rule 10: System readiness requires every critical subsystem healthy
    for (const auto& comp : comps) {
        // Any offline or failed component blocks readiness
        if (!comp.isOnline || comp.diagnosticState == DiagnosticState::FAILED) {
            return false;
        }

        // Rule 8: Critical components immediately reduce system readiness
        if (comp.diagnosticState == DiagnosticState::CRITICAL) {
            return false;
        }

        // Subsystems marked not synchronized block system readiness
        if (!comp.isSynchronized) {
            return false;
        }
    }

    return true;
}

// =============================================================================
// Queries & Statistics
// =============================================================================

bool SystemDiagnosticsEngine::GetComponent(const std::string& componentId, ComponentHealth& outComponent) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = components_.find(componentId);
    if (it != components_.end()) {
        outComponent = it->second;
        return true;
    }
    return false;
}

std::vector<ComponentHealth> SystemDiagnosticsEngine::GetAllComponents() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<ComponentHealth> result;
    result.reserve(components_.size());
    for (const auto& pair : components_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<ComponentHealth> SystemDiagnosticsEngine::GetCriticalComponents() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<ComponentHealth> result;
    for (const auto& pair : components_) {
        if (pair.second.diagnosticState == DiagnosticState::CRITICAL ||
            pair.second.diagnosticState == DiagnosticState::FAILED) {
            result.push_back(pair.second);
        }
    }
    return result;
}

DiagnosticStatistics SystemDiagnosticsEngine::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    return statistics_;
}

// =============================================================================
// Structured ASCII Operational Reports
// =============================================================================

std::string SystemDiagnosticsEngine::GenerateHealthReport() const {
    bool isReady = VerifySystemReadiness();

    std::lock_guard<std::mutex> lock(reportMutex_);

    DiagnosticStatistics stats;
    {
        std::lock_guard<std::mutex> sLock(statisticsMutex_);
        stats = statistics_;
    }

    std::string sysStatus = "HEALTHY";
    if (stats.failedComponents > 0) sysStatus = "FAILED";
    else if (stats.criticalComponents > 0) sysStatus = "CRITICAL";
    else if (stats.warningComponents > 0) sysStatus = "WARNING";

    std::ostringstream oss;
    oss << "[HEALTH]\n\n"
        << "System Status:\n" << sysStatus << "\n\n"
        << "Total Components:\n" << stats.totalComponents << "\n\n"
        << "Healthy:\n" << stats.healthyComponents << "\n\n"
        << "Warning:\n" << stats.warningComponents << "\n\n"
        << "Critical:\n" << stats.criticalComponents << "\n\n"
        << "Failed:\n" << stats.failedComponents << "\n\n"
        << "Readiness:\n" << (isReady ? "READY" : "NOT_READY") << "\n";

    return oss.str();
}

std::string SystemDiagnosticsEngine::GenerateDiagnosticsReport(const std::string& componentId) const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    ComponentHealth comp;
    bool found = false;

    if (!componentId.empty()) {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = components_.find(componentId);
        if (it != components_.end()) {
            comp = it->second;
            found = true;
        }
    } else {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        if (!components_.empty()) {
            comp = components_.begin()->second;
            found = true;
        }
    }

    std::ostringstream oss;
    oss << "[DIAGNOSTICS]\n\n";

    if (found) {
        oss << "Component:\n" << comp.componentId << "\n\n"
            << "Type:\n" << ToString(comp.componentType) << "\n\n"
            << "State:\n" << ToString(comp.diagnosticState) << "\n\n"
            << std::fixed << std::setprecision(1)
            << "CPU:\n" << comp.cpuUsage << "%\n\n"
            << "Temperature:\n" << comp.temperature << " C\n\n"
            << std::setprecision(2)
            << "Latency:\n" << comp.networkLatency << " ms\n\n"
            << "Packet Loss:\n" << comp.packetLoss << "%\n\n"
            << "Issues:\n" << ((comp.diagnosticState != DiagnosticState::HEALTHY) ? 1 : 0) << "\n";
    } else {
        oss << "Component:\nNONE\n\n"
            << "State:\nUNKNOWN\n\n"
            << "Issues:\n0\n";
    }

    return oss.str();
}

std::string SystemDiagnosticsEngine::GenerateFailureReport() const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    std::vector<ComponentHealth> failures;
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& pair : components_) {
            if (pair.second.diagnosticState == DiagnosticState::FAILED ||
                pair.second.diagnosticState == DiagnosticState::CRITICAL) {
                failures.push_back(pair.second);
            }
        }
    }

    std::ostringstream oss;
    oss << "[FAILURE]\n\n";

    if (!failures.empty()) {
        const auto& f = failures.front();
        oss << "Component:\n" << f.componentId << "\n\n"
            << "Type:\n" << ToString(f.componentType) << "\n\n"
            << "State:\n" << ToString(f.diagnosticState) << "\n\n"
            << "Reason:\n" << (f.lastError.empty() ? "Heartbeat Timeout" : f.lastError) << "\n\n"
            << "Failure Count:\n" << std::max<uint32_t>(1, f.failureCount) << "\n\n"
            << "Action:\n" << (f.diagnosticState == DiagnosticState::FAILED ? "Restart Node" : "Cooling / Load Reduction") << "\n";
    } else {
        oss << "Status:\nNO_FAILURES\n\n"
            << "Active Failures:\n0\n\n"
            << "Critical Components:\n0\n";
    }

    return oss.str();
}

std::string SystemDiagnosticsEngine::GenerateMaintenanceReport() const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    MaintenanceRecommendation topRec;
    bool found = false;

    {
        std::lock_guard<std::mutex> mLock(maintenanceMutex_);
        if (!maintenanceRecommendations_.empty()) {
            topRec = maintenanceRecommendations_.front();
            for (const auto& r : maintenanceRecommendations_) {
                if (static_cast<int>(r.priority) > static_cast<int>(topRec.priority)) {
                    topRec = r;
                }
            }
            found = true;
        }
    }

    std::ostringstream oss;
    oss << "[MAINTENANCE]\n\n";

    if (found) {
        oss << "Recommendation:\n" << topRec.recommendationId << "\n\n"
            << "Component:\n" << topRec.componentId << "\n\n"
            << "Priority:\n" << ToString(topRec.priority) << "\n\n"
            << "Failure Type:\n" << topRec.predictedFailureType << "\n\n"
            << "Action:\n" << topRec.recommendedAction << "\n";
    } else {
        oss << "Status:\nNO_MAINTENANCE_REQUIRED\n\n"
            << "Recommendations:\n0\n\n"
            << "Priority:\nNONE\n";
    }

    return oss.str();
}

// =============================================================================
// Configuration Setters
// =============================================================================

void SystemDiagnosticsEngine::SetHeartbeatTimeoutMs(uint64_t timeoutMs) {
    heartbeatTimeoutMs_ = timeoutMs;
}

void SystemDiagnosticsEngine::SetCpuThresholds(double warningPct, double criticalPct) {
    warningCpuThresholdPct_ = warningPct;
    criticalCpuThresholdPct_ = criticalPct;
}

void SystemDiagnosticsEngine::SetTemperatureThresholds(double warningTemp, double criticalTemp) {
    warningTempThresholdC_ = warningTemp;
    criticalTempThresholdC_ = criticalTemp;
}

void SystemDiagnosticsEngine::SetPacketLossThresholds(double warningPct, double criticalPct) {
    warningPacketLossPct_ = warningPct;
    criticalPacketLossPct_ = criticalPct;
}

void SystemDiagnosticsEngine::SetMemoryGrowthThreshold(double thresholdMBPerMin) {
    memoryGrowthThreshold_ = thresholdMBPerMin;
}

} // namespace diagnostics
} // namespace aegis
