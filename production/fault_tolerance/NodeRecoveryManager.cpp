/**
 * @file NodeRecoveryManager.cpp
 * @brief Implementation of node recovery manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/fault_tolerance/NodeRecoveryManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <vector>

namespace aegis::production {

NodeRecoveryManager::NodeRecoveryManager() = default;

void NodeRecoveryManager::TriggerRecovery(const std::string& nodeId, ClusterNodeType type, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pendingRecoveries_.find(nodeId);
    if (it != pendingRecoveries_.end() && !it->second.succeeded) {
        return; // Already actively recovering
    }

    RecoveryAction action;
    action.nodeId = nodeId;
    action.type = type;
    action.attemptCount = 0;
    action.maxAttempts = 5;
    action.nextRetryDelayMs = 500;
    action.nextRetryMonoMs = common::TimeUtils::GetMonotonicTimeMs() + action.nextRetryDelayMs;
    action.succeeded = false;
    action.details = reason;

    switch (type) {
        case ClusterNodeType::TRAFFIC_NODE:
            action.strategy = RecoveryStrategy::FAILSAFE_YELLOW_FLASH;
            break;
        case ClusterNodeType::AMBULANCE:
            action.strategy = RecoveryStrategy::RECONCILE_ROUTE_PLAN;
            break;
        case ClusterNodeType::HOSPITAL:
            action.strategy = RecoveryStrategy::RECONNECT_WITH_BACKOFF;
            break;
        case ClusterNodeType::CAMERA:
            action.strategy = RecoveryStrategy::FALLBACK_HISTORICAL_DENSITY;
            break;
        default:
            action.strategy = RecoveryStrategy::RECONNECT_WITH_BACKOFF;
            break;
    }

    pendingRecoveries_[nodeId] = action;
}

uint32_t NodeRecoveryManager::ProcessPendingRecoveries() {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    uint32_t newlySucceeded = 0;
    std::vector<RecoveryAction> completedActions;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, action] : pendingRecoveries_) {
            if (action.succeeded) continue;

            if (now >= action.nextRetryMonoMs) {
                action.attemptCount++;
                // Simulated recovery attempt; in production will trigger node reconnect handshake
                if (action.attemptCount >= 2) {
                    action.succeeded = true;
                    totalSuccessfulRecoveries_++;
                    newlySucceeded++;
                    completedActions.push_back(action);
                } else {
                    // Exponential backoff: 500ms -> 1000ms -> 2000ms
                    action.nextRetryDelayMs *= 2;
                    action.nextRetryMonoMs = now + action.nextRetryDelayMs;
                }
            }
        }
    }

    for (const auto& act : completedActions) {
        if (callback_) {
            callback_(act);
        }
    }

    return newlySucceeded;
}

void NodeRecoveryManager::MarkRecoverySuccess(const std::string& nodeId) {
    RecoveryAction actionCopy;
    bool notify = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = pendingRecoveries_.find(nodeId);
        if (it != pendingRecoveries_.end()) {
            if (!it->second.succeeded) {
                it->second.succeeded = true;
                totalSuccessfulRecoveries_++;
                actionCopy = it->second;
                notify = true;
            }
        }
    }

    if (notify && callback_) {
        callback_(actionCopy);
    }
}

void NodeRecoveryManager::CancelRecovery(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    pendingRecoveries_.erase(nodeId);
}

bool NodeRecoveryManager::IsRecovering(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pendingRecoveries_.find(nodeId);
    if (it != pendingRecoveries_.end()) {
        return !it->second.succeeded;
    }
    return false;
}

RecoveryAction NodeRecoveryManager::GetRecoveryStatus(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = pendingRecoveries_.find(nodeId);
    if (it != pendingRecoveries_.end()) {
        return it->second;
    }
    RecoveryAction empty;
    empty.nodeId = nodeId;
    return empty;
}

size_t NodeRecoveryManager::GetPendingRecoveryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : pendingRecoveries_) {
        if (!v.succeeded) count++;
    }
    return count;
}

uint32_t NodeRecoveryManager::GetTotalSuccessfulRecoveries() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalSuccessfulRecoveries_;
}

void NodeRecoveryManager::SetRecoveryCallback(RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

std::string NodeRecoveryManager::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"total_successful_recoveries\": " << totalSuccessfulRecoveries_ << ",\n"
       << "  \"pending_recoveries\": [\n";

    bool first = true;
    for (const auto& [id, a] : pendingRecoveries_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"node_id\": \"" << id << "\",\n"
           << "      \"attempts\": " << a.attemptCount << ",\n"
           << "      \"succeeded\": " << (a.succeeded ? "true" : "false") << ",\n"
           << "      \"details\": \"" << a.details << "\"\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

} // namespace aegis::production

