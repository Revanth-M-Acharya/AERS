/**
 * @file IRoutePlanner.hpp
 * @brief Abstract interface for route planning algorithms.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include <string>
#include <vector>

namespace aegis::coordinator::intelligence {

struct RouteResult {
    bool success{false};
    std::vector<std::string> pathIntersectionIds;
    std::vector<std::string> pathSegmentIds;
    double totalDistanceMeters{0.0};
    double estimatedTravelTimeSec{0.0};
    std::string failureReason;
};

class IRoutePlanner {
public:
    virtual ~IRoutePlanner() = default;

    /**
     * @brief Compute the shortest/fastest route between two intersections.
     */
    virtual RouteResult ComputeShortestPath(const std::string& startId,
                                            const std::string& destinationId,
                                            const RoadGraph& graph) = 0;

    /**
     * @brief Dynamically recompute route considering roadblocks or destination shifts.
     */
    virtual RouteResult RecomputeRoute(const std::string& currentId,
                                       const std::string& newDestinationId,
                                       const RoadGraph& graph,
                                       const std::vector<std::string>& avoidedSegments = {}) = 0;
};

} // namespace aegis::coordinator::intelligence

