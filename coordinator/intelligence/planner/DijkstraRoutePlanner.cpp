/**
 * @file DijkstraRoutePlanner.cpp
 * @brief Implementation of DijkstraRoutePlanner.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/planner/DijkstraRoutePlanner.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include <queue>
#include <unordered_map>
#include <algorithm>

namespace aegis::coordinator::intelligence {

RouteResult DijkstraRoutePlanner::ComputeShortestPath(const std::string& startId,
                                                      const std::string& destinationId,
                                                      const RoadGraph& graph) {
    std::unordered_set<std::string> emptyAvoid;
    return SolveDijkstra(startId, destinationId, graph, emptyAvoid);
}

RouteResult DijkstraRoutePlanner::RecomputeRoute(const std::string& currentId,
                                                 const std::string& newDestinationId,
                                                 const RoadGraph& graph,
                                                 const std::vector<std::string>& avoidedSegments) {
    std::unordered_set<std::string> avoidedSet(avoidedSegments.begin(), avoidedSegments.end());
    CoordinatorLogger::Instance().Info("PLANNER", "Recomputing route from " + currentId + " to " +
                                       newDestinationId + " (avoiding " + std::to_string(avoidedSegments.size()) + " segments)");
    return SolveDijkstra(currentId, newDestinationId, graph, avoidedSet);
}

RouteResult DijkstraRoutePlanner::SolveDijkstra(const std::string& startId,
                                                const std::string& destinationId,
                                                const RoadGraph& graph,
                                                const std::unordered_set<std::string>& avoidedSet) {
    RouteResult result;

    if (startId == destinationId) {
        result.success = true;
        result.pathIntersectionIds = {startId};
        result.totalDistanceMeters = 0.0;
        result.estimatedTravelTimeSec = 0.0;
        return result;
    }

    const Intersection* startNode = graph.GetIntersection(startId);
    const Intersection* destNode = graph.GetIntersection(destinationId);

    if (!startNode || !startNode->isEnabled) {
        result.success = false;
        result.failureReason = "Start intersection " + startId + " invalid or disabled";
        CoordinatorLogger::Instance().Warn("PLANNER", result.failureReason);
        return result;
    }
    if (!destNode || !destNode->isEnabled) {
        result.success = false;
        result.failureReason = "Destination intersection " + destinationId + " invalid or disabled";
        CoordinatorLogger::Instance().Warn("PLANNER", result.failureReason);
        return result;
    }

    // Min-heap entry: (cost, intersectionId)
    using CostNode = std::pair<double, std::string>;
    std::priority_queue<CostNode, std::vector<CostNode>, std::greater<CostNode>> pq;

    std::unordered_map<std::string, double> minCost;
    std::unordered_map<std::string, std::string> parentIntersection;
    std::unordered_map<std::string, std::string> parentSegment;

    minCost[startId] = 0.0;
    pq.push({0.0, startId});

    bool found = false;

    while (!pq.empty()) {
        auto [cost, u] = pq.top();
        pq.pop();

        if (cost > minCost[u]) continue;

        if (u == destinationId) {
            found = true;
            break;
        }

        std::vector<RoadSegment> outgoing = graph.GetOutgoingSegments(u);
        for (const auto& seg : outgoing) {
            if (avoidedSet.find(seg.segmentId) != avoidedSet.end()) {
                continue; // Explicitly avoided
            }

            double edgeTime = seg.GetEffectiveTravelTimeSec();
            if (edgeTime >= 1e8) {
                continue; // Blocked or closed
            }

            std::string v = seg.toIntersectionId;
            double nextCost = cost + edgeTime;

            if (minCost.find(v) == minCost.end() || nextCost < minCost[v]) {
                minCost[v] = nextCost;
                parentIntersection[v] = u;
                parentSegment[v] = seg.segmentId;
                pq.push({nextCost, v});
            }
        }
    }

    if (!found) {
        result.success = false;
        result.failureReason = "No feasible path found between " + startId + " and " + destinationId;
        CoordinatorLogger::Instance().Warn("PLANNER", result.failureReason);
        return result;
    }

    // Backtrack path
    std::string curr = destinationId;
    std::vector<std::string> pathNodes;
    std::vector<std::string> pathSegs;

    while (curr != startId) {
        pathNodes.push_back(curr);
        pathSegs.push_back(parentSegment[curr]);
        curr = parentIntersection[curr];
    }
    pathNodes.push_back(startId);

    std::reverse(pathNodes.begin(), pathNodes.end());
    std::reverse(pathSegs.begin(), pathSegs.end());

    double totalDist = 0.0;
    double totalTime = 0.0;

    for (const auto& segId : pathSegs) {
        const RoadSegment* s = graph.GetSegment(segId);
        if (s) {
            totalDist += s->distanceMeters;
            totalTime += s->GetEffectiveTravelTimeSec();
        }
    }

    result.success = true;
    result.pathIntersectionIds = std::move(pathNodes);
    result.pathSegmentIds = std::move(pathSegs);
    result.totalDistanceMeters = totalDist;
    result.estimatedTravelTimeSec = totalTime;

    CoordinatorLogger::Instance().Info("PLANNER", "Shortest path computed: " + startId + " -> " +
                                       destinationId + " (" + std::to_string(result.pathIntersectionIds.size()) +
                                       " hops, " + std::to_string(totalDist) + "m, " +
                                       std::to_string(totalTime) + "s)");
    return result;
}

} // namespace aegis::coordinator::intelligence

