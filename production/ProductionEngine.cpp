/**
 * @file ProductionEngine.cpp
 * @brief Implementation of master production engine facade.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/ProductionEngine.hpp"
#include <sstream>

namespace aegis::production {

ProductionEngine::ProductionEngine(DeploymentProfile profile)
    : profile_(profile), config_(profile) {}

ProductionEngine::~ProductionEngine() {
    Shutdown();
}

bool ProductionEngine::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Initialize configuration & validation
    auto valResult = config_.ValidateSchema();
    if (!valResult.isValid) {
        return false;
    }

    // 2. Register core watchdog tasks
    watchdog_.RegisterTask("CoordinatorMasterThread", 2000);
    watchdog_.RegisterTask("ExecutionDispatcherThread", 1000);
    watchdog_.RegisterTask("EventBusWorker", 1000);

    // 3. Register standard health probes
    health_.RegisterProbe("CoordinatorCore", [this]() {
        ComponentHealth h;
        h.componentName = "CoordinatorCore";
        h.status = (lifecycle_.IsRunning() || lifecycle_.GetState() == SystemLifecycleState::INITIALIZING)
            ? HealthStatus::HEALTHY : HealthStatus::DEGRADED;
        h.message = "Coordinator FSM running";
        return h;
    });

    health_.RegisterProbe("MemoryResource", [this]() {
        auto snap = diagnostics_.GetLatestSnapshot();
        ComponentHealth h;
        h.componentName = "MemoryResource";
        if (snap.residentMemoryKb > 1024 * 1024) {
            h.status = HealthStatus::CRITICAL;
            h.message = "Memory usage exceeds 1 GB";
        } else if (snap.residentMemoryKb > 512 * 1024) {
            h.status = HealthStatus::DEGRADED;
            h.message = "Memory usage exceeds 512 MB";
        } else {
            h.status = HealthStatus::HEALTHY;
            h.message = "Memory within normal limits";
        }
        return h;
    });

    // 4. Register lifecycle shutdown hooks
    lifecycle_.RegisterShutdownHook("FlushSecurityAuditLogs", []() {
        // In real QNX system, flushes audit logs to persistent flash
    });

    lifecycle_.RegisterShutdownHook("ResetTrafficNodesFailsafe", []() {
        // In real QNX system, signals all intersections to switch to flashing yellow
    });

    return true;
}

bool ProductionEngine::Start() {
    return lifecycle_.Start();
}

void ProductionEngine::Shutdown() {
    lifecycle_.GracefulShutdown();
}

void ProductionEngine::PeriodicTick() {
    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Pet master watchdog task
    watchdog_.Pet("CoordinatorMasterThread");

    // 2. Check watchdog deadlines
    watchdog_.CheckDeadlines();

    // 3. Check heartbeat liveness across nodes
    heartbeat_.CheckClusterLiveness();

    // 4. Process pending node recoveries
    recovery_.ProcessPendingRecoveries();

    // 5. Sample runtime diagnostics
    auto snap = diagnostics_.Sample();
    leakMonitor_.AddSample(snap.residentMemoryKb, snap.activeSockets, snap.activeThreads, snap.openFileDescriptors);
    benchmarks_.UpdateResourcePeaks(snap.cpuUsagePercent, snap.residentMemoryKb);

    // 6. Deadlock & contention audit
    deadlockDetector_.CheckForDeadlocks();

    // 7. Update health state
    health_.RunHealthChecks();
}

std::string ProductionEngine::GenerateSystemReportMarkdown() const {
    std::ostringstream ss;
    ss << "# SMART AEGIS Comprehensive Production & Operational Readiness Report\n\n"
       << "**Release:** " << VersionManager::GetFullVersionString() << "\n"
       << "**Profile:** " << DeploymentProfileToString(profile_) << "\n\n"
       << "## 1. System Health Status\n"
       << health_.ToAsciiDashboard() << "\n\n"
       << "## 2. Production Benchmarking & Real-Time SLAs\n"
       << benchmarks_.ExportMarkdown() << "\n\n"
       << "## 3. Communication Reliability & Security\n"
       << "- **Active Supervised Nodes:** " << heartbeat_.GetOnlineCount() << " Online / " << heartbeat_.GetOfflineCount() << " Offline\n"
       << "- **Authorized Nodes Whitelist:** " << authenticator_.GetAuthorizedCount() << " registered\n"
       << "- **Security Violations Detected:** " << auditLogger_.GetViolationCount() << "\n"
       << "- **Replay Attacks Prevented:** " << securityGuard_.GetReplayAttacksDetected() << "\n"
       << "- **Deadlock / Starvation Warnings:** " << deadlockDetector_.GetTotalDeadlockWarnings() << "\n"
       << "- **Active Fault Injections:** " << faultInjector_.GetActiveFaultCount() << "\n";

    return ss.str();
}

std::string ProductionEngine::ToJson() const {
    std::ostringstream ss;
    ss << "{\n"
       << "  \"version\": " << VersionManager::ToJson() << ",\n"
       << "  \"health\": " << health_.ToJson() << ",\n"
       << "  \"watchdog\": " << watchdog_.ToJson() << ",\n"
       << "  \"heartbeat\": " << heartbeat_.ToJson() << ",\n"
       << "  \"channel\": " << channel_.ToJson() << ",\n"
       << "  \"benchmarks\": " << benchmarks_.ExportJson() << "\n"
       << "}";
    return ss.str();
}

} // namespace aegis::production
