/**
 * @file ExecutionMonitor.cpp
 * @brief Implementation of runtime execution supervision, deadline enforcement, and ETA drift detection.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/monitor/ExecutionMonitor.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cmath>

namespace aegis::coordinator::orchestration {

ExecutionMonitor::ExecutionMonitor(std::shared_ptr<CoordinatorStateManager> stateManager,
                                   std::shared_ptr<IEventBus> eventBus,
                                   uint32_t deadlineToleranceMs,
                                   uint32_t etaDriftThresholdMs,
                                   uint32_t maxNodeDelayMs)
    : stateManager_(std::move(stateManager)),
      eventBus_(std::move(eventBus)),
      deadlineToleranceMs_(deadlineToleranceMs),
      etaDriftThresholdMs_(etaDriftThresholdMs),
      maxNodeDelayMs_(maxNodeDelayMs) {}

void ExecutionMonitor::StartSupervision(const intelligence::ExecutionPlan& plan,
                                        double initialEtaSeconds,
                                        uint64_t currentMonoMs) {
    if (plan.planId.empty()) return;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    MonitoredExecution mon;
    mon.planId = plan.planId;
    mon.ambulanceId = plan.ambulanceId;
    mon.initialPlannedEtaSeconds = initialEtaSeconds;
    mon.startTimeMonoMs = currentMonoMs;
    mon.replanningNeeded = false;
    mon.rollbackComplete = false;

    for (const auto& item : plan.signalCommands) {
        StageProgress sp;
        sp.intersectionId = item.intersectionId;
        sp.scheduledActivationMonoMs = item.activationTimeMonoMs;
        sp.holdDurationMs = item.holdDurationMs;
        sp.activated = false;
        sp.completed = false;
        mon.stages.push_back(sp);
    }

    std::lock_guard<std::mutex> lock(monitorMutex_);
    activeMonitors_[plan.planId] = mon;
}

void ExecutionMonitor::RecordSignalConfirmation(const std::string& planId,
                                                const std::string& intersectionId,
                                                const std::string& signalState,
                                                uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it == activeMonitors_.end()) return;

    for (auto& stage : it->second.stages) {
        if (stage.intersectionId == intersectionId) {
            if (signalState == "GREEN") {
                stage.activated = true;
                stage.actualActivationMonoMs = currentMonoMs;
            }
            break;
        }
    }
}

void ExecutionMonitor::RecordAmbulancePassage(const std::string& planId,
                                             const std::string& intersectionId,
                                             uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    bool allCompleted = true;
    std::string ambId;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        auto it = activeMonitors_.find(planId);
        if (it == activeMonitors_.end()) return;

        ambId = it->second.ambulanceId;
        for (auto& stage : it->second.stages) {
            if (stage.intersectionId == intersectionId) {
                stage.completed = true;
                stage.actualCompletionMonoMs = currentMonoMs;
            }
            if (!stage.completed) {
                allCompleted = false;
            }
        }
    }

    if (allCompleted && eventBus_) {
        eventBus_->Publish(Event(EventType::ExecutionCompleted, planId, ambId,
                                 "All corridor stages passed successfully", currentMonoMs));
    }
}

void ExecutionMonitor::RecordUpdatedEta(const std::string& planId,
                                        double updatedEtaSeconds,
                                        uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    bool triggerReplanning = false;
    std::string reason;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        auto it = activeMonitors_.find(planId);
        if (it == activeMonitors_.end()) return;

        double driftSeconds = std::abs(updatedEtaSeconds - it->second.initialPlannedEtaSeconds);
        uint32_t driftMs = static_cast<uint32_t>(driftSeconds * 1000.0);

        if (driftMs > etaDriftThresholdMs_) {
            it->second.replanningNeeded = true;
            reason = "ETA drift exceeded threshold: drift=" + std::to_string(driftSeconds) +
                     "s, threshold=" + std::to_string(etaDriftThresholdMs_ / 1000.0) + "s";
            it->second.replanningReason = reason;
            triggerReplanning = true;
        }
    }

    if (triggerReplanning && eventBus_) {
        eventBus_->Publish(Event(EventType::RouteChanged, planId, "",
                                 reason, currentMonoMs));
    }
}

void ExecutionMonitor::CheckExecutionDeadlines(uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::vector<std::pair<std::string, std::string>> violations;

    {
        std::lock_guard<std::mutex> lock(monitorMutex_);
        for (auto& [planId, mon] : activeMonitors_) {
            if (mon.replanningNeeded) continue;

            for (const auto& stage : mon.stages) {
                // If deadline passed and stage neither activated nor completed
                if (!stage.activated && !stage.completed) {
                    if (currentMonoMs > stage.scheduledActivationMonoMs &&
                        (currentMonoMs - stage.scheduledActivationMonoMs) > deadlineToleranceMs_) {
                        mon.replanningNeeded = true;
                        std::string reason = "Missed activation deadline at " + stage.intersectionId +
                                             ": scheduled=" + std::to_string(stage.scheduledActivationMonoMs) +
                                             ", current=" + std::to_string(currentMonoMs);
                        mon.replanningReason = reason;
                        violations.push_back({planId, reason});
                        break;
                    }
                }
            }
        }
    }

    for (const auto& [pId, r] : violations) {
        if (eventBus_) {
            eventBus_->Publish(Event(EventType::TrafficIncident, pId, "",
                                     "Deadline Violation: " + r, currentMonoMs));
        }
    }
}

void ExecutionMonitor::RecordRollbackCompletion(const std::string& planId, const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    rollbackedNodes_[planId].insert(intersectionId);

    auto it = activeMonitors_.find(planId);
    if (it != activeMonitors_.end()) {
        if (rollbackedNodes_[planId].size() >= it->second.stages.size()) {
            it->second.rollbackComplete = true;
        }
    }
}

bool ExecutionMonitor::IsReplanningRequired(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it != activeMonitors_.end()) {
        return it->second.replanningNeeded;
    }
    return false;
}

std::string ExecutionMonitor::GetReplanningReason(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it != activeMonitors_.end()) {
        return it->second.replanningReason;
    }
    return "";
}

bool ExecutionMonitor::IsPlanCompleted(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it == activeMonitors_.end()) return false;
    if (it->second.stages.empty()) return false;

    for (const auto& stage : it->second.stages) {
        if (!stage.completed) return false;
    }
    return true;
}

bool ExecutionMonitor::IsStageCompleted(const std::string& planId, const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it == activeMonitors_.end()) return false;

    for (const auto& stage : it->second.stages) {
        if (stage.intersectionId == intersectionId) {
            return stage.completed;
        }
    }
    return false;
}

bool ExecutionMonitor::IsRollbackCompleted(const std::string& planId) const {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    auto it = activeMonitors_.find(planId);
    if (it != activeMonitors_.end()) {
        return it->second.rollbackComplete;
    }
    return false;
}

void ExecutionMonitor::SetDeadlineToleranceMs(uint32_t ms) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    deadlineToleranceMs_ = ms;
}

void ExecutionMonitor::SetEtaDriftThresholdMs(uint32_t ms) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    etaDriftThresholdMs_ = ms;
}

void ExecutionMonitor::SetMaxNodeDelayMs(uint32_t ms) {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    maxNodeDelayMs_ = ms;
}

void ExecutionMonitor::Reset() {
    std::lock_guard<std::mutex> lock(monitorMutex_);
    activeMonitors_.clear();
    rollbackedNodes_.clear();
}

} // namespace aegis::coordinator::orchestration

