/**
 * @file RecoveryManager.cpp
 * @brief Implementation of dynamic failover, bypass corridor reallocation, and safe rollback.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/recovery/RecoveryManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::orchestration {

RecoveryManager::RecoveryManager(std::shared_ptr<CoordinatorStateManager> stateManager,
                                 std::shared_ptr<NodeHealthMonitor> healthMonitor,
                                 std::shared_ptr<EmergencySessionManager> sessionManager,
                                 std::shared_ptr<IEventBus> eventBus,
                                 RollbackSenderFn rollbackSender)
    : stateManager_(std::move(stateManager)),
      healthMonitor_(std::move(healthMonitor)),
      sessionManager_(std::move(sessionManager)),
      eventBus_(std::move(eventBus)),
      rollbackSender_(std::move(rollbackSender)) {}

void RecoveryManager::SetRollbackSender(RollbackSenderFn sender) {
    std::lock_guard<std::mutex> lock(recoveryMutex_);
    rollbackSender_ = std::move(sender);
}

bool RecoveryManager::HandleNodeFailure(const std::string& failedNodeId,
                                        const std::string& reason,
                                        uint64_t currentMonoMs) {
    if (failedNodeId.empty()) return false;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    // 1. Mark node unhealthy in health monitor
    if (healthMonitor_) {
        healthMonitor_->MarkNodeFailed(failedNodeId, reason);
    }

    // 2. Identify all active emergency sessions traversing the failed node
    std::vector<EmergencySession> affectedSessions;
    if (sessionManager_) {
        auto activeSessions = sessionManager_->GetActiveSessions();
        for (const auto& sess : activeSessions) {
            bool traversesFailed = false;
            for (const auto& hop : sess.route) {
                if (hop == failedNodeId) {
                    traversesFailed = true;
                    break;
                }
            }
            if (traversesFailed) {
                affectedSessions.push_back(sess);
            }
        }
    }

    // 3. Mark recovery record and send rollback to nodes if needed
    for (const auto& sess : affectedSessions) {
        RecoveryRecord rec;
        rec.emergencyId = sess.emergencyId;
        rec.failedNodeId = failedNodeId;
        rec.originalRoute = sess.route;
        rec.recoveryCompleted = false;
        rec.recoveryStatus = "Triggered failover due to " + reason;

        {
            std::lock_guard<std::mutex> lock(recoveryMutex_);
            recoveries_[sess.emergencyId] = rec;
        }

        // Send safe rollback to unaffected intersections in that corridor
        std::vector<std::string> nodesToRollback;
        for (const auto& hop : sess.route) {
            if (hop != failedNodeId) {
                nodesToRollback.push_back(hop);
            }
        }
        SendSafeRollback(nodesToRollback, "RESTORE_NORMAL_MODE");

        // Inform EventBus
        if (eventBus_) {
            eventBus_->Publish(Event(EventType::TrafficIncident, sess.emergencyId, failedNodeId,
                                     "Corridor impacted by failed node " + failedNodeId + "; recovery triggered",
                                     currentMonoMs));
        }
    }

    return !affectedSessions.empty();
}

bool RecoveryManager::ApplyBypassCorridor(const std::string& emergencyId,
                                          const std::string& failedNodeId,
                                          const std::vector<std::string>& newRoute,
                                          const intelligence::ExecutionPlan& newPlan,
                                          uint64_t currentMonoMs) {
    if (emergencyId.empty() || newRoute.empty()) return false;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    // Ensure new route doesn't traverse the failed node
    for (const auto& hop : newRoute) {
        if (hop == failedNodeId) {
            return false;
        }
    }

    if (sessionManager_) {
        sessionManager_->UpdateRoute(emergencyId, newRoute, 0.0, currentMonoMs);
        sessionManager_->UpdateExecutionPlan(emergencyId, newPlan, currentMonoMs);
        sessionManager_->UpdateCorridorState(emergencyId, CorridorState::ACTIVE, currentMonoMs);
    }

    {
        std::lock_guard<std::mutex> lock(recoveryMutex_);
        auto it = recoveries_.find(emergencyId);
        if (it != recoveries_.end()) {
            it->second.bypassRoute = newRoute;
            it->second.recoveryCompleted = true;
            it->second.recoveredAtMonoMs = currentMonoMs;
            it->second.recoveryStatus = "Bypass corridor applied successfully";
        } else {
            RecoveryRecord rec;
            rec.emergencyId = emergencyId;
            rec.failedNodeId = failedNodeId;
            rec.bypassRoute = newRoute;
            rec.recoveryCompleted = true;
            rec.recoveredAtMonoMs = currentMonoMs;
            rec.recoveryStatus = "Bypass corridor applied successfully";
            recoveries_[emergencyId] = rec;
        }
    }

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::RouteChanged, emergencyId, newPlan.planId,
                                 "Bypass corridor established: hops=" + std::to_string(newRoute.size()),
                                 currentMonoMs));
    }

    return true;
}

void RecoveryManager::SendSafeRollback(const std::vector<std::string>& nodeIds,
                                      const std::string& rollbackCommand) {
    RollbackSenderFn senderCopy;
    {
        std::lock_guard<std::mutex> lock(recoveryMutex_);
        senderCopy = rollbackSender_;
    }

    if (senderCopy) {
        for (const auto& nodeId : nodeIds) {
            senderCopy(nodeId, rollbackCommand);
        }
    }
}

bool RecoveryManager::HandleNodeRejoin(const std::string& nodeId, uint64_t currentMonoMs) {
    if (nodeId.empty()) return false;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    if (healthMonitor_) {
        // Record recovery heartbeat
        healthMonitor_->RecordHeartbeat(nodeId, 5.0, 10.0, 42.0, 2, currentMonoMs);
    }

    if (stateManager_) {
        stateManager_->SetNodeConnectionStatus(nodeId, ConnectionStatus::CONNECTED);
        auto optNode = stateManager_->GetTrafficNode(nodeId);
        if (optNode.has_value()) {
            TrafficNodeRecord node = optNode.value();
            node.isOperational = true;
            node.lastSeenMonoMs = currentMonoMs;
            stateManager_->RegisterTrafficNode(node);
        }
    }

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::NodeRecovered, nodeId, "",
                                 "Node successfully rejoined and resynchronized", currentMonoMs));
    }

    return true;
}

bool RecoveryManager::HasActiveRecovery(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(recoveryMutex_);
    auto it = recoveries_.find(emergencyId);
    if (it != recoveries_.end()) {
        return !it->second.recoveryCompleted;
    }
    return false;
}

std::optional<RecoveryRecord> RecoveryManager::GetRecoveryRecord(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(recoveryMutex_);
    auto it = recoveries_.find(emergencyId);
    if (it != recoveries_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void RecoveryManager::Reset() {
    std::lock_guard<std::mutex> lock(recoveryMutex_);
    recoveries_.clear();
}

} // namespace aegis::coordinator::orchestration

