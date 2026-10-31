/**
 * @file ExecutionPlanGenerator.hpp
 * @brief Concrete implementation of immutable execution plan generator with CRC16 verification.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"

namespace aegis::coordinator::intelligence {

class ExecutionPlanGenerator : public IExecutionPlanGenerator {
public:
    ExecutionPlanGenerator() = default;
    ~ExecutionPlanGenerator() override = default;

    ExecutionPlan CreateExecutionPlan(const CorridorPlan& corridor, uint32_t priorityLevel) override;
};

} // namespace aegis::coordinator::intelligence

