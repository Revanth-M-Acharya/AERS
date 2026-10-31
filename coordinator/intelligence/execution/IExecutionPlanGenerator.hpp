/**
 * @file IExecutionPlanGenerator.hpp
 * @brief Abstract interface for generating immutable, tamper-verified execution plans.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/corridor/ICorridorPlanner.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::intelligence {

struct ExecutionPlan {
    std::string planId;
    std::string ambulanceId;
    uint32_t priorityLevel{1};
    uint64_t timestampMs{0};
    std::vector<std::string> orderedIntersections;
    std::vector<CorridorScheduleItem> signalCommands;
    std::string rollbackPlan{"RESTORE_NORMAL_MODE"};
    uint16_t checksum{0};

    std::string SerializeToString() const;
    uint16_t ComputeChecksum() const;
    bool VerifyChecksum() const;
};

class IExecutionPlanGenerator {
public:
    virtual ~IExecutionPlanGenerator() = default;

    /**
     * @brief Construct an immutable ExecutionPlan with integrity checksum.
     */
    virtual ExecutionPlan CreateExecutionPlan(const CorridorPlan& corridor, uint32_t priorityLevel) = 0;
};

} // namespace aegis::coordinator::intelligence

