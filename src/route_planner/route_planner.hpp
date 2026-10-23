#ifndef AEGIS_ROUTE_PLANNER_HPP
#define AEGIS_ROUTE_PLANNER_HPP

#include <vector>
#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace routing {

struct NavigationRoute {
    std::string routeId;
    std::vector<common::GeoCoordinate> waypoints;
    std::vector<std::string> intersectionIds;
    double estimatedTimeSec{0.0};
    double totalDistanceMeters{0.0};
};

class RoutePlanner {
public:
    RoutePlanner() = default;
    ~RoutePlanner() = default;

    NavigationRoute CalculateRoute(const common::GeoCoordinate& start,
                                  const common::GeoCoordinate& destination);
};

} // namespace routing
} // namespace aegis

#endif // AEGIS_ROUTE_PLANNER_HPP
