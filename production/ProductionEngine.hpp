/**
 * @file ProductionEngine.hpp
 * @brief Master facade integrating health monitoring, fault tolerance, communication reliability, security, and benchmarking.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "production/health/HealthSupervisor.hpp"
#include "production/health/WatchdogManager.hpp"
#include "production/health/RuntimeDiagnostics.hpp"
#include "production/health/HeartbeatSupervisor.hpp"
#include "production/fault_tolerance/NodeRecoveryManager.hpp"
#include "production/fault_tolerance/MissionContinuityManager.hpp"
#include "production/fault_tolerance/LifecycleManager.hpp"
#include "production/communication/ReliableChannel.hpp"
#include "production/communication/PacketValidator.hpp"
#include "production/security/NodeAuthenticator.hpp"
#include "production/security/MessageSecurityGuard.hpp"
#include "production/security/SecurityAuditLogger.hpp"
#include "production/config/ProductionConfigManager.hpp"
#include "production/config/VersionManager.hpp"
#include "production/reliability/DeadlockDetector.hpp"
#include "production/reliability/ResourceLeakMonitor.hpp"
#include "production/reliability/SoakTestRunner.hpp"
#include "production/fault_injection/FaultInjector.hpp"
#include "production/benchmark/ProductionBenchmarkReporter.hpp"

#include <memory>
#include <mutex>

namespace aegis::production {

class ProductionEngine {
public:
    explicit ProductionEngine(DeploymentProfile profile = DeploymentProfile::PRODUCTION);
    ~ProductionEngine();

    bool Initialize();
    bool Start();
    void Shutdown();

    /**
     * @brief Periodic background supervisory tick (checks watchdogs, heartbeats, recoveries, diagnostics).
     */
    void PeriodicTick();

    // Accessors
    HealthSupervisor& GetHealthSupervisor() { return health_; }
    WatchdogManager& GetWatchdogManager() { return watchdog_; }
    RuntimeDiagnostics& GetDiagnostics() { return diagnostics_; }
    HeartbeatSupervisor& GetHeartbeatSupervisor() { return heartbeat_; }
    NodeRecoveryManager& GetRecoveryManager() { return recovery_; }
    MissionContinuityManager& GetContinuityManager() { return continuity_; }
    LifecycleManager& GetLifecycleManager() { return lifecycle_; }
    ReliableChannel& GetReliableChannel() { return channel_; }
    PacketValidator& GetPacketValidator() { return validator_; }
    NodeAuthenticator& GetAuthenticator() { return authenticator_; }
    MessageSecurityGuard& GetSecurityGuard() { return securityGuard_; }
    SecurityAuditLogger& GetAuditLogger() { return auditLogger_; }
    ProductionConfigManager& GetConfigManager() { return config_; }
    DeadlockDetector& GetDeadlockDetector() { return deadlockDetector_; }
    ResourceLeakMonitor& GetLeakMonitor() { return leakMonitor_; }
    FaultInjector& GetFaultInjector() { return faultInjector_; }
    ProductionBenchmarkReporter& GetBenchmarkReporter() { return benchmarks_; }

    std::string GenerateSystemReportMarkdown() const;
    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    DeploymentProfile profile_{DeploymentProfile::PRODUCTION};

    HealthSupervisor health_;
    WatchdogManager watchdog_;
    RuntimeDiagnostics diagnostics_;
    HeartbeatSupervisor heartbeat_;
    NodeRecoveryManager recovery_;
    MissionContinuityManager continuity_;
    LifecycleManager lifecycle_;
    ReliableChannel channel_;
    PacketValidator validator_;
    NodeAuthenticator authenticator_;
    MessageSecurityGuard securityGuard_;
    SecurityAuditLogger auditLogger_;
    ProductionConfigManager config_;
    DeadlockDetector deadlockDetector_;
    ResourceLeakMonitor leakMonitor_;
    FaultInjector faultInjector_;
    ProductionBenchmarkReporter benchmarks_;
};

} // namespace aegis::production

