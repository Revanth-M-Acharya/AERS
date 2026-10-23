#include "aegis/core/route_planner.hpp"
#include "aegis/common/uuid_utils.hpp"
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>

namespace aegis {
namespace core {
namespace routing {

class RoutePlanner::GraphImpl {
public:
    std::unordered_map<std::string, GraphNode> nodes;
    std::unordered_map<std::string, std::vector<GraphEdge>> adjacency;
    size_t edgeCount{0};

    struct PathResult {
        std::vector<std::string> nodeSequence;
        double totalDistance{0.0};
        double totalTimeSec{0.0};
        bool found{false};
    };

    // A* algorithm using Haversine heuristic
    PathResult SearchAStar(const std::string& startId, const std::string& goalId) const {
        PathResult res;
        if (nodes.find(startId) == nodes.end() || nodes.find(goalId) == nodes.end()) {
            return res;
        }

        if (startId == goalId) {
            res.nodeSequence = {startId};
            res.found = true;
            return res;
        }

        const auto& goalCoord = nodes.at(goalId).coord;

        // Comparator for priority queue (min-heap on f-score)
        using Element = std::pair<double, std::string>;
        std::priority_queue<Element, std::vector<Element>, std::greater<Element>> openSet;

        std::unordered_map<std::string, double> gScore;
        std::unordered_map<std::string, double> fScore;
        std::unordered_map<std::string, std::string> cameFrom;
        std::unordered_map<std::string, double> edgeDistMap;

        for (const auto& kv : nodes) {
            gScore[kv.first] = std::numeric_limits<double>::infinity();
            fScore[kv.first] = std::numeric_limits<double>::infinity();
        }

        gScore[startId] = 0.0;
        double initialH = common::CalculateHaversineDistance(nodes.at(startId).coord, goalCoord) / (60.0 / 3.6);
        fScore[startId] = initialH;
        openSet.push({initialH, startId});

        while (!openSet.empty()) {
            auto current = openSet.top().second;
            openSet.pop();

            if (current == goalId) {
                // Reconstruct path
                std::string curr = goalId;
                while (curr != startId) {
                    res.nodeSequence.push_back(curr);
                    curr = cameFrom[curr];
                }
                res.nodeSequence.push_back(startId);
                std::reverse(res.nodeSequence.begin(), res.nodeSequence.end());
                res.found = true;
                res.totalDistance = gScore[goalId];
                // Average municipal transit speed 50 km/h = ~13.89 m/s
                res.totalTimeSec = res.totalDistance / 13.8889;
                return res;
            }

            auto it = adjacency.find(current);
            if (it == adjacency.end()) continue;

            for (const auto& edge : it->second) {
                double tentativeG = gScore[current] + edge.distanceMeters;
                if (tentativeG < gScore[edge.targetNodeId]) {
                    cameFrom[edge.targetNodeId] = current;
                    gScore[edge.targetNodeId] = tentativeG;
                    double h = common::CalculateHaversineDistance(nodes.at(edge.targetNodeId).coord, goalCoord) / (60.0 / 3.6);
                    fScore[edge.targetNodeId] = tentativeG + h;
                    openSet.push({fScore[edge.targetNodeId], edge.targetNodeId});
                }
            }
        }

        // Fallback to Dijkstra search if A* fails
        return SearchDijkstra(startId, goalId);
    }

    PathResult SearchDijkstra(const std::string& startId, const std::string& goalId) const {
        PathResult res;
        using Element = std::pair<double, std::string>;
        std::priority_queue<Element, std::vector<Element>, std::greater<Element>> pq;

        std::unordered_map<std::string, double> dist;
        std::unordered_map<std::string, std::string> prev;

        for (const auto& kv : nodes) {
            dist[kv.first] = std::numeric_limits<double>::infinity();
        }

        dist[startId] = 0.0;
        pq.push({0.0, startId});

        while (!pq.empty()) {
            auto current = pq.top().second;
            double d = pq.top().first;
            pq.pop();

            if (d > dist[current]) continue;
            if (current == goalId) break;

            auto it = adjacency.find(current);
            if (it == adjacency.end()) continue;

            for (const auto& edge : it->second) {
                double alt = dist[current] + edge.distanceMeters;
                if (alt < dist[edge.targetNodeId]) {
                    dist[edge.targetNodeId] = alt;
                    prev[edge.targetNodeId] = current;
                    pq.push({alt, edge.targetNodeId});
                }
            }
        }

        if (dist[goalId] < std::numeric_limits<double>::infinity()) {
            std::string curr = goalId;
            while (curr != startId) {
                res.nodeSequence.push_back(curr);
                curr = prev[curr];
            }
            res.nodeSequence.push_back(startId);
            std::reverse(res.nodeSequence.begin(), res.nodeSequence.end());
            res.found = true;
            res.totalDistance = dist[goalId];
            res.totalTimeSec = res.totalDistance / 13.8889;
        }

        return res;
    }
};

RoutePlanner::RoutePlanner() : m_graph(std::make_unique<GraphImpl>()) {
    InitializeDefaultCityGraph();
}

RoutePlanner::~RoutePlanner() = default;

void RoutePlanner::AddNode(const GraphNode& node) {
    m_graph->nodes[node.id] = node;
}

void RoutePlanner::AddEdge(const std::string& fromId, const std::string& toId,
                           double distanceMeters, double speedLimitKmH,
                           const std::string& streetName) {
    m_graph->adjacency[fromId].push_back({toId, distanceMeters, speedLimitKmH, streetName});
    // Add bidirectional edge for urban street network
    m_graph->adjacency[toId].push_back({fromId, distanceMeters, speedLimitKmH, streetName});
    m_graph->edgeCount += 2;
}

void RoutePlanner::InitializeDefaultCityGraph() {
    m_graph->nodes.clear();
    m_graph->adjacency.clear();
    m_graph->edgeCount = 0;

    // 20 Municipal Intersections and Zones across metropolitan grid
    AddNode({"INT-MG-01", "MG Road / Brigade", {12.9750, 77.6050, 0.0}, true});
    AddNode({"INT-MG-02", "MG Road / Residency", {12.9750, 77.6100, 0.0}, true});
    AddNode({"INT-MG-03", "MG Road / Trinity", {12.9750, 77.6180, 0.0}, true});
    AddNode({"INT-ORR-01", "Outer Ring / Indiranagar", {12.9780, 77.6350, 0.0}, true});
    AddNode({"INT-ORR-02", "Outer Ring / Domlur", {12.9620, 77.6380, 0.0}, true});
    AddNode({"INT-ORR-03", "Outer Ring / Koramangala", {12.9350, 77.6250, 0.0}, true});
    AddNode({"INT-KOR-01", "Sony World Junction", {12.9350, 77.6320, 0.0}, true});
    AddNode({"INT-KOR-02", "Koramangala Water Tank", {12.9300, 77.6200, 0.0}, true});
    AddNode({"INT-HOSP-01", "Central Trauma Hospital Bay", {12.9800, 77.6000, 0.0}, true});
    AddNode({"INT-HOSP-02", "City Memorial Emergency", {12.9400, 77.6100, 0.0}, true});
    AddNode({"INT-STA-01", "Central Ambulance Depot", {12.9700, 77.5900, 0.0}, true});
    AddNode({"INT-STA-02", "East Sector EMS Depot", {12.9650, 77.6450, 0.0}, true});
    AddNode({"INT-CBD-01", "Cubbon Park Crossing", {12.9760, 77.5920, 0.0}, true});
    AddNode({"INT-CBD-02", "Vidhana Soudha Junction", {12.9800, 77.5910, 0.0}, true});
    AddNode({"INT-CBD-03", "Raj Bhavan Road", {12.9850, 77.5930, 0.0}, true});
    AddNode({"INT-RES-01", "Richmond Circle", {12.9660, 77.5980, 0.0}, true});
    AddNode({"INT-RES-02", "Langford Town Junction", {12.9580, 77.6030, 0.0}, true});
    AddNode({"INT-AIR-01", "Old Airport Road / HAL", {12.9570, 77.6550, 0.0}, true});
    AddNode({"INT-AIR-02", "Old Airport Road / Manipal", {12.9590, 77.6480, 0.0}, true});
    AddNode({"INT-ULSOOR-01", "Ulsoor Lake Junction", {12.9820, 77.6200, 0.0}, true});

    // Connecting Street Corridors
    AddEdge("INT-STA-01", "INT-CBD-01", 700.0, 50.0, "Kasturba Road");
    AddEdge("INT-CBD-01", "INT-CBD-02", 500.0, 50.0, "Ambedkar Veedhi");
    AddEdge("INT-CBD-02", "INT-CBD-03", 600.0, 50.0, "Raj Bhavan Road");
    AddEdge("INT-CBD-01", "INT-MG-01", 1200.0, 60.0, "MG Road West");
    AddEdge("INT-MG-01", "INT-HOSP-01", 800.0, 45.0, "Hospital Access Rd");
    AddEdge("INT-MG-01", "INT-MG-02", 550.0, 50.0, "MG Road Corridor");
    AddEdge("INT-MG-02", "INT-MG-03", 900.0, 50.0, "Trinity Way");
    AddEdge("INT-MG-03", "INT-ULSOOR-01", 800.0, 40.0, "Ulsoor Road");
    AddEdge("INT-MG-03", "INT-ORR-01", 1800.0, 60.0, "Old Madras Link");
    AddEdge("INT-ORR-01", "INT-ORR-02", 1700.0, 60.0, "Intermediate Ring Road");
    AddEdge("INT-ORR-02", "INT-AIR-02", 1100.0, 50.0, "Domlur Link");
    AddEdge("INT-AIR-02", "INT-AIR-01", 800.0, 50.0, "Airport Arterial");
    AddEdge("INT-AIR-02", "INT-STA-02", 950.0, 45.0, "East EMS Access");
    AddEdge("INT-ORR-02", "INT-ORR-03", 2900.0, 60.0, "Koramangala Ring Corridor");
    AddEdge("INT-ORR-03", "INT-KOR-01", 750.0, 40.0, "100ft Road");
    AddEdge("INT-ORR-03", "INT-KOR-02", 600.0, 40.0, "Water Tank Road");
    AddEdge("INT-KOR-02", "INT-HOSP-02", 1200.0, 50.0, "Hosur Link");
    AddEdge("INT-MG-01", "INT-RES-01", 1100.0, 45.0, "Residency Road");
    AddEdge("INT-RES-01", "INT-RES-02", 900.0, 45.0, "Richmond Town");
    AddEdge("INT-RES-02", "INT-HOSP-02", 2100.0, 50.0, "Adugodi Link");
}

std::string RoutePlanner::FindNearestNode(const common::GeoCoordinate& coord) const {
    double bestDist = std::numeric_limits<double>::max();
    std::string bestId;

    for (const auto& kv : m_graph->nodes) {
        double d = common::CalculateHaversineDistance(coord, kv.second.coord);
        if (d < bestDist) {
            bestDist = d;
            bestId = kv.first;
        }
    }
    return bestId;
}

double RoutePlanner::CalculateETA(double distanceMeters, double speedKmH) const noexcept {
    if (speedKmH <= 1.0) speedKmH = 50.0;
    double speedMs = speedKmH / 3.6;
    return distanceMeters / speedMs;
}

NavigationRoute RoutePlanner::CalculateRoute(const common::GeoCoordinate& origin,
                                             const common::GeoCoordinate& destination) const {
    NavigationRoute route;
    route.routeId = "ROUTE-" + common::GenerateUuidV4().substr(0, 8);

    std::string startNode = FindNearestNode(origin);
    std::string endNode = FindNearestNode(destination);

    if (startNode.empty() || endNode.empty()) {
        route.waypoints = {origin, destination};
        route.totalDistanceMeters = common::CalculateHaversineDistance(origin, destination);
        route.estimatedTimeSec = CalculateETA(route.totalDistanceMeters, 50.0);
        return route;
    }

    auto pathRes = m_graph->SearchAStar(startNode, endNode);

    // Build waypoints sequence: Origin -> Snap Start -> Intersections -> Snap End -> Destination
    route.waypoints.push_back(origin);
    for (const auto& nid : pathRes.nodeSequence) {
        route.waypoints.push_back(m_graph->nodes.at(nid).coord);
        if (m_graph->nodes.at(nid).isIntersection) {
            route.intersectionIds.push_back(nid);
        }
    }
    route.waypoints.push_back(destination);

    // Calculate total distance including origin & destination connectors
    double startConnector = common::CalculateHaversineDistance(origin, m_graph->nodes.at(startNode).coord);
    double endConnector = common::CalculateHaversineDistance(m_graph->nodes.at(endNode).coord, destination);
    route.totalDistanceMeters = startConnector + pathRes.totalDistance + endConnector;
    route.estimatedTimeSec = CalculateETA(route.totalDistanceMeters, 50.0);

    return route;
}

size_t RoutePlanner::GetNodeCount() const noexcept {
    return m_graph->nodes.size();
}

size_t RoutePlanner::GetEdgeCount() const noexcept {
    return m_graph->edgeCount;
}

} // namespace routing
} // namespace core
} // namespace aegis
