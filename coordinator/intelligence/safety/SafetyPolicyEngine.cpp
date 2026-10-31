/**
 * @file SafetyPolicyEngine.cpp
 * @brief Implementation of SafetyPolicyEngine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/safety/SafetyPolicyEngine.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"

namespace aegis::coordinator::intelligence {

SafetyPolicyEngine::SafetyPolicyEngine(uint32_t minHoldDurationMs, uint32_t maxHoldDurationMs)
    : minHoldDurationMs_(minHoldDurationMs),
      maxHoldDurationMs_(maxHoldDurationMs) {}

SafetyValidationResult SafetyPolicyEngine::ValidatePlan(const ExecutionPlan& plan,
                                                        const RoadGraph& graph,
                                                        const std::vector<ExecutionPlan>& activePlans,
                                                        uint64_t currentMonoMs) {
    SafetyValidationResult result;
    result.isValid = true;

    // 1. Verify Checksum & Plan Integrity
    if (!plan.VerifyChecksum()) {
        result.isValid = false;
        result.violations.push_back("CHECKSUM_CORRUPTION: Plan CRC16 validation failed");
    }

    // 2. Empty Plan Check
    if (plan.orderedIntersections.empty() || plan.signalCommands.empty()) {
        result.isValid = false;
        result.violations.push_back("EMPTY_PLAN: Execution plan contains no intersection commands");
    }

    // 3. Rollback Safety
    if (plan.rollbackPlan.empty() ||
        (plan.rollbackPlan.find("NORMAL") == std::string::npos &&
         plan.rollbackPlan.find("ALL_RED") == std::string::npos)) {
        result.isValid = false;
        result.violations.push_back("UNSAFE_ROLLBACK: Rollback plan must restore NORMAL or ALL_RED safe state");
    }

    // 4. Validate Intersections in Topology
    for (const auto& interId : plan.orderedIntersections) {
        const Intersection* node = graph.GetIntersection(interId);
        if (!node) {
            result.isValid = false;
            result.violations.push_back("UNKNOWN_INTERSECTION: Intersection " + interId + " does not exist in RoadGraph");
        } else if (!node->isEnabled) {
            result.isValid = false;
            result.violations.push_back("DISABLED_INTERSECTION: Intersection " + interId + " is marked disabled");
        }
    }

    // 5. Validate Timing Bounds
    for (const auto& cmd : plan.signalCommands) {
        if (cmd.holdDurationMs < minHoldDurationMs_) {
            result.isValid = false;
            result.violations.push_back("INVALID_TIMING: Hold duration " + std::to_string(cmd.holdDurationMs) +
                                        "ms is below safety minimum (" + std::to_string(minHoldDurationMs_) + "ms) on " + cmd.intersectionId);
        }
        if (cmd.holdDurationMs > maxHoldDurationMs_) {
            result.isValid = false;
            result.violations.push_back("INVALID_TIMING: Hold duration " + std::to_string(cmd.holdDurationMs) +
                                        "ms exceeds safety maximum (" + std::to_string(maxHoldDurationMs_) + "ms) on " + cmd.intersectionId);
        }
        if (cmd.activationTimeMonoMs + cmd.holdDurationMs < currentMonoMs) {
            result.isValid = false;
            result.violations.push_back("EXPIRED_TIMING: Activation window expired in past on " + cmd.intersectionId);
        }
    }

    // 6. Conflict Detection with Concurrent Active Plans
    for (const auto& active : activePlans) {
        if (active.planId == plan.planId || active.ambulanceId == plan.ambulanceId) {
            continue;
        }

        for (const auto& newCmd : plan.signalCommands) {
            for (const auto& activeCmd : active.signalCommands) {
                if (newCmd.intersectionId == activeCmd.intersectionId) {
                    uint64_t newStart = newCmd.activationTimeMonoMs;
                    uint64_t newEnd = newStart + newCmd.holdDurationMs;
                    uint64_t actStart = activeCmd.activationTimeMonoMs;
                    uint64_t actEnd = actStart + activeCmd.holdDurationMs;

                    bool timeOverlap = (newStart < actEnd) && (newEnd > actStart);
                    if (timeOverlap) {
                        // Check priority inversion
                        if (plan.priorityLevel <= active.priorityLevel) {
                            result.isValid = false;
                            result.violations.push_back(
                                "CONFLICTING_GREEN: Intersection " + newCmd.intersectionId +
                                " overlap with active Plan " + active.planId +
                                " (Priority: " + std::to_string(active.priorityLevel) +
                                " >= Incoming Priority: " + std::to_string(plan.priorityLevel) + ")"
                            );
                        }
                    }
                }
            }
        }
    }

    if (!result.isValid) {
        std::string allReasons;
        for (const auto& v : result.violations) {
            if (!allReasons.empty()) allReasons += "; ";
            allReasons += v;
        }
        result.rejectionReason = allReasons;
        CoordinatorLogger::Instance().Warn("SAFETY", "Rejected Execution Plan " + plan.planId + ": " + allReasons);
    } else {
        CoordinatorLogger::Instance().Info("SAFETY", "Validated Execution Plan " + plan.planId + " successfully.");
    }

    return result;
}

} // namespace aegis::coordinator::intelligence

