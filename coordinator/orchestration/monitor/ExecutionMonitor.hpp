/**
 * @file ExecutionMonitor.hpp
 * @brief Dynamic runtime supervisor for active execution plans, deadlines, and ETA drift.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::orchestration {

struct StageProgress {
    std::string intersectionId;
    uint64_t scheduledActivationMonoMs{0};
    uint32_t holdDurationMs{0};
    bool activated{false};
    bool completed{false};
    uint64_t actualActivationMonoMs{0};
    uint64_t actualCompletionMonoMs{0};
};

struct MonitoredExecution {
    std::string planId;
    std::string ambulanceId;
    double initialPlannedEtaSeconds{0.0};
    uint64_t startTimeMonoMs{0};
    std::vector<StageProgress> stages;
    bool replanningNeeded{false};
    std::string replanningReason;
    bool rollbackComplete{false};
};

class ExecutionMonitor {
public:
    ExecutionMonitor(std::shared_ptr<CoordinatorStateManager> stateManager = nullptr,
                     std::shared_ptr<IEventBus> eventBus = nullptr,
                     uint32_t deadlineToleranceMs = 3000,
                     uint32_t etaDriftThresholdMs = 5000,
                     uint32_t maxNodeDelayMs = 3000);
    ~ExecutionMonitor() = default;

    /**
     * @brief Register a newly dispatched execution plan for active supervision.
     */
    void StartSupervision(const intelligence::ExecutionPlan& plan,
                          double initialEtaSeconds,
                          uint64_t currentMonoMs = 0);

    /**
     * @brief Record node status update or green signal confirmation.
     */
    void RecordSignalConfirmation(const std::string& planId,
                                  const std::string& intersectionId,
                                  const std::string& signalState,
                                  uint64_t currentMonoMs = 0);

    /**
     * @brief Record vehicle progression across an intersection.
     */
    void RecordAmbulancePassage(const std::string& planId,
                               const std::string& intersectionId,
                               uint64_t currentMonoMs = 0);

    /**
     * @brief Record updated ETA and check against initial ETA for excessive drift.
     */
    void RecordUpdatedEta(const std::string& planId,
                          double updatedEtaSeconds,
                          uint64_t currentMonoMs = 0);

    /**
     * @brief Periodic deadline and late-execution watchdog check.
     */
    void CheckExecutionDeadlines(uint64_t currentMonoMs = 0);

    /**
     * @brief Confirm that rollback to normal mode completed.
     */
    void RecordRollbackCompletion(const std::string& planId, const std::string& intersectionId);

    bool IsReplanningRequired(const std::string& planId) const;
    std::string GetReplanningReason(const std::string& planId) const;
    bool IsPlanCompleted(const std::string& planId) const;
    bool IsStageCompleted(const std::string& planId, const std::string& intersectionId) const;
    bool IsRollbackCompleted(const std::string& planId) const;

    void SetDeadlineToleranceMs(uint32_t ms);
    void SetEtaDriftThresholdMs(uint32_t ms);
    void SetMaxNodeDelayMs(uint32_t ms);

    void Reset();

private:
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<IEventBus> eventBus_;
    uint32_t deadlineToleranceMs_{3000};
    uint32_t etaDriftThresholdMs_{5000};
    uint32_t maxNodeDelayMs_{3000};

    mutable std::mutex monitorMutex_;
    std::unordered_map<std::string, MonitoredExecution> activeMonitors_;
    std::unordered_map<std::string, std::unordered_set<std::string>> rollbackedNodes_;
};

} // namespace aegis::coordinator::orchestration

