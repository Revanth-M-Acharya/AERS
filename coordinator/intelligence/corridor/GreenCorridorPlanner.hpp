/**
 * @file GreenCorridorPlanner.hpp
 * @brief Time-staged green corridor wave planning implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/corridor/ICorridorPlanner.hpp"

namespace aegis::coordinator::intelligence {

class GreenCorridorPlanner : public ICorridorPlanner {
public:
    explicit GreenCorridorPlanner(uint32_t leadTimeMs = 5000, uint32_t holdDurationMs = 15000);
    ~GreenCorridorPlanner() override = default;

    CorridorPlan GenerateCorridorPlan(const AmbulanceRequest& ambulance,
                                     const RouteResult& route,
                                     const ETAReport& eta,
                                     const RoadGraph& graph,
                                     uint64_t currentMonoMs) override;

private:
    uint32_t leadTimeMs_{5000};
    uint32_t holdDurationMs_{15000};
};

} // namespace aegis::coordinator::intelligence

