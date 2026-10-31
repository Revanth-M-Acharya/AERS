/**
 * @file SafetyPolicyEngine.hpp
 * @brief Safety policy validation engine preventing conflicting greens, deadlocks, and invalid timing.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/safety/ISafetyPolicy.hpp"

namespace aegis::coordinator::intelligence {

class SafetyPolicyEngine : public ISafetyPolicy {
public:
    explicit SafetyPolicyEngine(uint32_t minHoldDurationMs = 2000, uint32_t maxHoldDurationMs = 60000);
    ~SafetyPolicyEngine() override = default;

    SafetyValidationResult ValidatePlan(const ExecutionPlan& plan,
                                        const RoadGraph& graph,
                                        const std::vector<ExecutionPlan>& activePlans,
                                        uint64_t currentMonoMs) override;

private:
    uint32_t minHoldDurationMs_{2000};
    uint32_t maxHoldDurationMs_{60000};
};

} // namespace aegis::coordinator::intelligence

