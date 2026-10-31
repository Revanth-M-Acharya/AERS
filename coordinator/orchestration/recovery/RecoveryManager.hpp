/**
 * @file RecoveryManager.hpp
 * @brief Dynamic corridor failover, bypass route recomputation, and safe rollback manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/monitor/NodeHealthMonitor.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "coordinator/orchestration/dispatcher/ExecutionDispatcher.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <memory>

namespace aegis::coordinator::orchestration {

using RollbackSenderFn = std::function<void(const std::string& nodeId, const std::string& rollbackCommand)>;

struct RecoveryRecord {
    std::string emergencyId;
    std::string failedNodeId;
    std::vector<std::string> originalRoute;
    std::vector<std::string> bypassRoute;
    bool recoveryCompleted{false};
    uint64_t recoveredAtMonoMs{0};
    std::string recoveryStatus;
};

class RecoveryManager {
public:
    RecoveryManager(std::shared_ptr<CoordinatorStateManager> stateManager,
                    std::shared_ptr<NodeHealthMonitor> healthMonitor,
                    std::shared_ptr<EmergencySessionManager> sessionManager,
                    std::shared_ptr<IEventBus> eventBus,
                    RollbackSenderFn rollbackSender = nullptr);
    ~RecoveryManager() = default;

    /**
     * @brief Trigger failover recovery upon detection of node timeout or physical failure.
     */
    bool HandleNodeFailure(const std::string& failedNodeId,
                           const std::string& reason = "Node failure",
                           uint64_t currentMonoMs = 0);

    /**
     * @brief Apply an alternative bypass corridor around a failed intersection.
     */
    bool ApplyBypassCorridor(const std::string& emergencyId,
                            const std::string& failedNodeId,
                            const std::vector<std::string>& newRoute,
                            const intelligence::ExecutionPlan& newPlan,
                            uint64_t currentMonoMs = 0);

    /**
     * @brief Send safe rollback command (e.g. RESTORE_NORMAL_MODE) to unaffected/abandoned nodes.
     */
    void SendSafeRollback(const std::vector<std::string>& nodeIds,
                         const std::string& rollbackCommand = "RESTORE_NORMAL_MODE");

    /**
     * @brief Handle node rejoin, verify self-check, and resynchronize with state manager.
     */
    bool HandleNodeRejoin(const std::string& nodeId, uint64_t currentMonoMs = 0);

    bool HasActiveRecovery(const std::string& emergencyId) const;
    std::optional<RecoveryRecord> GetRecoveryRecord(const std::string& emergencyId) const;

    void SetRollbackSender(RollbackSenderFn sender);
    void Reset();

private:
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<NodeHealthMonitor> healthMonitor_;
    std::shared_ptr<EmergencySessionManager> sessionManager_;
    std::shared_ptr<IEventBus> eventBus_;
    RollbackSenderFn rollbackSender_;

    mutable std::mutex recoveryMutex_;
    std::unordered_map<std::string, RecoveryRecord> recoveries_;
};

} // namespace aegis::coordinator::orchestration

