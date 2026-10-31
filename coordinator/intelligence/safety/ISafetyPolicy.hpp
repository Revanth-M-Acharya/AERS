/**
 * @file ISafetyPolicy.hpp
 * @brief Abstract interface for safety policy validation over execution plans.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include <string>
#include <vector>

namespace aegis::coordinator::intelligence {

struct SafetyValidationResult {
    bool isValid{true};
    std::string rejectionReason;
    std::vector<std::string> violations;
};

class ISafetyPolicy {
public:
    virtual ~ISafetyPolicy() = default;

    /**
     * @brief Formally validate an execution plan against safety invariants before dispatch.
     */
    virtual SafetyValidationResult ValidatePlan(const ExecutionPlan& plan,
                                                const RoadGraph& graph,
                                                const std::vector<ExecutionPlan>& activePlans,
                                                uint64_t currentMonoMs) = 0;
};

} // namespace aegis::coordinator::intelligence

