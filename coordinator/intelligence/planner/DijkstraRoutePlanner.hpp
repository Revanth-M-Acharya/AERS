/**
 * @file DijkstraRoutePlanner.hpp
 * @brief Dijkstra's shortest path routing implementation over RoadGraph.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/planner/IRoutePlanner.hpp"
#include <unordered_set>

namespace aegis::coordinator::intelligence {

class DijkstraRoutePlanner : public IRoutePlanner {
public:
    DijkstraRoutePlanner() = default;
    ~DijkstraRoutePlanner() override = default;

    RouteResult ComputeShortestPath(const std::string& startId,
                                    const std::string& destinationId,
                                    const RoadGraph& graph) override;

    RouteResult RecomputeRoute(const std::string& currentId,
                               const std::string& newDestinationId,
                               const RoadGraph& graph,
                               const std::vector<std::string>& avoidedSegments = {}) override;

private:
    RouteResult SolveDijkstra(const std::string& startId,
                              const std::string& destinationId,
                              const RoadGraph& graph,
                              const std::unordered_set<std::string>& avoidedSet);
};

} // namespace aegis::coordinator::intelligence

