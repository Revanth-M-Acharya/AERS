/**
 * @file ICorridorPlanner.hpp
 * @brief Abstract interface for generating staged green corridor wave execution schedules.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/planner/IRoutePlanner.hpp"
#include "coordinator/intelligence/eta/IETAEngine.hpp"
#include "coordinator/intelligence/scheduler/IPriorityScheduler.hpp"
#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::intelligence {

struct CorridorScheduleItem {
    std::string intersectionId;
    std::string signalCommand{"SET_GREEN"};
    uint64_t activationTimeMonoMs{0};
    uint32_t holdDurationMs{15000};
    std::string releaseCondition{"AMBULANCE_PASSED_OR_TIMEOUT"};
};

struct CorridorPlan {
    std::string planId;
    std::string ambulanceId;
    std::vector<CorridorScheduleItem> schedule;
    std::string rollbackMode{"NORMAL_MODE"};
    uint64_t createdTimeMonoMs{0};
};

class ICorridorPlanner {
public:
    virtual ~ICorridorPlanner() = default;

    /**
     * @brief Generate time-staged green corridor plan along calculated route.
     */
    virtual CorridorPlan GenerateCorridorPlan(const AmbulanceRequest& ambulance,
                                             const RouteResult& route,
                                             const ETAReport& eta,
                                             const RoadGraph& graph,
                                             uint64_t currentMonoMs) = 0;
};

} // namespace aegis::coordinator::intelligence

