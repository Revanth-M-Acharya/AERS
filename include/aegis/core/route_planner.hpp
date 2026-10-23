#ifndef AEGIS_CORE_ROUTE_PLANNER_HPP
#define AEGIS_CORE_ROUTE_PLANNER_HPP

#include <vector>
#include <string>
#include <memory>
#include "aegis/common/types.hpp"

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

/**
 * @brief Graph-based path planning engine utilizing A* over municipal street topologies.
 * TODO: Implement A* heuristic path search with intersection passage ordering.
 */
class RoutePlanner {
public:
    RoutePlanner();
    ~RoutePlanner();

    bool LoadGraphTopology(const std::string& topologyFilePath);
    NavigationRoute CalculateRoute(const common::GeoCoordinate& origin,
                                  const common::GeoCoordinate& destination) const;

private:
    class GraphImpl;
    std::unique_ptr<GraphImpl> m_graph;
};

} // namespace routing
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_ROUTE_PLANNER_HPP
