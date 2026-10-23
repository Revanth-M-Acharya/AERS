/**
 * @file route_planner.hpp
 * @brief Graph-based path planning engine utilizing A* with Dijkstra fallback over municipal street networks.
 */

#ifndef AEGIS_CORE_ROUTE_PLANNER_HPP
#define AEGIS_CORE_ROUTE_PLANNER_HPP

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "aegis/common/types.hpp"
#include "aegis/common/geo_utils.hpp"

namespace aegis {
namespace core {
namespace routing {

struct NavigationRoute {
    std::string routeId;
    std::vector<common::GeoCoordinate> waypoints;
    std::vector<std::string> intersectionIds;
    double estimatedTimeSec{0.0};
    double totalDistanceMeters{0.0};
};

struct GraphNode {
    std::string id;
    std::string name;
    common::GeoCoordinate coord;
    bool isIntersection{true};
};

struct GraphEdge {
    std::string targetNodeId;
    double distanceMeters{0.0};
    double speedLimitKmH{50.0};
    std::string streetName;
};

class RoutePlanner {
public:
    RoutePlanner();
    ~RoutePlanner();

    void AddNode(const GraphNode& node);
    void AddEdge(const std::string& fromId, const std::string& toId,
                 double distanceMeters, double speedLimitKmH = 50.0,
                 const std::string& streetName = "");

    void InitializeDefaultCityGraph();

    NavigationRoute CalculateRoute(const common::GeoCoordinate& origin,
                                   const common::GeoCoordinate& destination) const;

    std::string FindNearestNode(const common::GeoCoordinate& coord) const;
    double CalculateETA(double distanceMeters, double speedKmH) const noexcept;

    size_t GetNodeCount() const noexcept;
    size_t GetEdgeCount() const noexcept;

private:
    class GraphImpl;
    std::unique_ptr<GraphImpl> m_graph;
};

} // namespace routing
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_ROUTE_PLANNER_HPP
