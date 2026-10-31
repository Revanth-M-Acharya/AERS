/**
 * @file GreenCorridorPlanner.cpp
 * @brief Implementation of GreenCorridorPlanner.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/corridor/GreenCorridorPlanner.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include <sstream>
#include <atomic>

namespace aegis::coordinator::intelligence {

namespace {
std::atomic<uint64_t> g_planSeq{1000};
}

GreenCorridorPlanner::GreenCorridorPlanner(uint32_t leadTimeMs, uint32_t holdDurationMs)
    : leadTimeMs_(leadTimeMs),
      holdDurationMs_(holdDurationMs) {}

CorridorPlan GreenCorridorPlanner::GenerateCorridorPlan(const AmbulanceRequest& ambulance,
                                                       const RouteResult& route,
                                                       const ETAReport& eta,
                                                       const RoadGraph& graph,
                                                       uint64_t currentMonoMs) {
    (void)eta;
    CorridorPlan plan;
    std::ostringstream oss;
    oss << "PLAN_" << ambulance.ambulanceId << "_" << ++g_planSeq;
    plan.planId = oss.str();
    plan.ambulanceId = ambulance.ambulanceId;
    plan.createdTimeMonoMs = currentMonoMs;
    plan.rollbackMode = "RESTORE_NORMAL_MODE";

    if (!route.success || route.pathIntersectionIds.empty()) {
        CoordinatorLogger::Instance().Warn("CORRIDOR", "Cannot generate corridor: Route invalid or empty for " + ambulance.ambulanceId);
        return plan;
    }

    double cumulativeTimeSec = 0.0;

    for (size_t i = 0; i < route.pathIntersectionIds.size(); ++i) {
        const std::string& interId = route.pathIntersectionIds[i];

        if (i > 0 && (i - 1) < route.pathSegmentIds.size()) {
            const RoadSegment* seg = graph.GetSegment(route.pathSegmentIds[i - 1]);
            if (seg) {
                cumulativeTimeSec += seg->GetEffectiveTravelTimeSec();
            }
        }

        uint64_t expectedArrivalMs = currentMonoMs + static_cast<uint64_t>(cumulativeTimeSec * 1000.0);
        uint64_t activationTimeMs = (expectedArrivalMs > leadTimeMs_) ? (expectedArrivalMs - leadTimeMs_) : currentMonoMs;

        CorridorScheduleItem item;
        item.intersectionId = interId;
        item.signalCommand = "SET_GREEN";
        item.activationTimeMonoMs = activationTimeMs;
        item.holdDurationMs = holdDurationMs_;
        item.releaseCondition = "AMBULANCE_PASSED_OR_TIMEOUT";

        plan.schedule.push_back(item);
    }

    CoordinatorLogger::Instance().Info("CORRIDOR", "Generated Corridor Plan " + plan.planId +
                                       " for " + ambulance.ambulanceId + " across " +
                                       std::to_string(plan.schedule.size()) + " intersections.");
    return plan;
}

} // namespace aegis::coordinator::intelligence

