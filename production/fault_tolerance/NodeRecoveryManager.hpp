/**
 * @file NodeRecoveryManager.hpp
 * @brief Automated recovery and reconnection manager for distributed cluster nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "production/health/HeartbeatSupervisor.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class RecoveryStrategy {
    RECONNECT_WITH_BACKOFF,
    FAILSAFE_YELLOW_FLASH,
    RESTORE_ACTIVE_CORRIDOR,
    RECONCILE_ROUTE_PLAN,
    FALLBACK_HISTORICAL_DENSITY
};

struct RecoveryAction {
    std::string nodeId;
    ClusterNodeType type;
    RecoveryStrategy strategy;
    uint32_t attemptCount{0};
    uint32_t maxAttempts{5};
    uint32_t nextRetryDelayMs{500};
    uint64_t nextRetryMonoMs{0};
    bool succeeded{false};
    std::string details;
};

using RecoveryCallback = std::function<void(const RecoveryAction& action)>;

class NodeRecoveryManager {
public:
    NodeRecoveryManager();
    ~NodeRecoveryManager() = default;

    /**
     * @brief Initiate a recovery workflow for a failed node.
     */
    void TriggerRecovery(const std::string& nodeId, ClusterNodeType type, const std::string& reason);

    /**
     * @brief Process active recovery retries against current monotonic time.
     * @return Number of recoveries successfully completed this cycle.
     */
    uint32_t ProcessPendingRecoveries();

    void MarkRecoverySuccess(const std::string& nodeId);
    void CancelRecovery(const std::string& nodeId);

    bool IsRecovering(const std::string& nodeId) const;
    RecoveryAction GetRecoveryStatus(const std::string& nodeId) const;
    size_t GetPendingRecoveryCount() const;
    uint32_t GetTotalSuccessfulRecoveries() const;

    void SetRecoveryCallback(RecoveryCallback callback);

    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RecoveryAction> pendingRecoveries_;
    uint32_t totalSuccessfulRecoveries_{0};
    RecoveryCallback callback_;
};

} // namespace aegis::production

