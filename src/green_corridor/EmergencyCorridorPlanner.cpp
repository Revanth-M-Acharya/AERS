/**
 * @file EmergencyCorridorPlanner.cpp
 * @brief Implementation of EmergencyCorridorPlanner for AERS (Phase 7A).
 */

#include "EmergencyCorridorPlanner.h"
#include "aegis/common/geo_utils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>
#include <unordered_set>

namespace aegis {
namespace green_corridor {

EmergencyCorridorPlanner::EmergencyCorridorPlanner()
    : m_mutex()
    , m_costMetric(CostMetric::TRAVEL_TIME)
    , m_autoLogging(true)
    , m_intersections()
    , m_adjacency()
    , m_segments()
    , m_ambulanceId("AMBULANCE")
    , m_ambulanceStartNode("")
    , m_ambulanceLat(0.0)
    , m_ambulanceLon(0.0)
    , m_ambulancePosSet(false)
    , m_hospitalId("HOSPITAL")
    , m_hospitalLat(0.0)
    , m_hospitalLon(0.0)
    , m_hospitalSet(false)
    , m_currentPlan()
    , m_lastRouteLog("") {
}

EmergencyCorridorPlanner::EmergencyCorridorPlanner(CostMetric metric)
    : EmergencyCorridorPlanner() {
    m_costMetric = metric;
}

EmergencyCorridorPlanner::EmergencyCorridorPlanner(EmergencyCorridorPlanner&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_costMetric = other.m_costMetric;
    m_autoLogging = other.m_autoLogging;
    m_intersections = std::move(other.m_intersections);
    m_adjacency = std::move(other.m_adjacency);
    m_segments = std::move(other.m_segments);
    m_ambulanceId = std::move(other.m_ambulanceId);
    m_ambulanceStartNode = std::move(other.m_ambulanceStartNode);
    m_ambulanceLat = other.m_ambulanceLat;
    m_ambulanceLon = other.m_ambulanceLon;
    m_ambulancePosSet = other.m_ambulancePosSet;
    m_hospitalId = std::move(other.m_hospitalId);
    m_hospitalLat = other.m_hospitalLat;
    m_hospitalLon = other.m_hospitalLon;
    m_hospitalSet = other.m_hospitalSet;
    m_currentPlan = std::move(other.m_currentPlan);
    m_lastRouteLog = std::move(other.m_lastRouteLog);
}

EmergencyCorridorPlanner& EmergencyCorridorPlanner::operator=(EmergencyCorridorPlanner&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_costMetric = other.m_costMetric;
        m_autoLogging = other.m_autoLogging;
        m_intersections = std::move(other.m_intersections);
        m_adjacency = std::move(other.m_adjacency);
        m_segments = std::move(other.m_segments);
        m_ambulanceId = std::move(other.m_ambulanceId);
        m_ambulanceStartNode = std::move(other.m_ambulanceStartNode);
        m_ambulanceLat = other.m_ambulanceLat;
        m_ambulanceLon = other.m_ambulanceLon;
        m_ambulancePosSet = other.m_ambulancePosSet;
        m_hospitalId = std::move(other.m_hospitalId);
        m_hospitalLat = other.m_hospitalLat;
        m_hospitalLon = other.m_hospitalLon;
        m_hospitalSet = other.m_hospitalSet;
        m_currentPlan = std::move(other.m_currentPlan);
        m_lastRouteLog = std::move(other.m_lastRouteLog);
    }
    return *this;
}

void EmergencyCorridorPlanner::AddIntersection(const IntersectionInfo& info) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections[info.intersectionId] = info;
    if (m_adjacency.find(info.intersectionId) == m_adjacency.end()) {
        m_adjacency[info.intersectionId] = {};
    }
}

void EmergencyCorridorPlanner::AddIntersection(const std::string& id, double lat, double lon) {
    AddIntersection(IntersectionInfo(id, lat, lon));
}

void EmergencyCorridorPlanner::SetIntersections(const std::vector<IntersectionInfo>& intersections) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections.clear();
    for (const auto& info : intersections) {
        m_intersections[info.intersectionId] = info;
        if (m_adjacency.find(info.intersectionId) == m_adjacency.end()) {
            m_adjacency[info.intersectionId] = {};
        }
    }
}

void EmergencyCorridorPlanner::AddRoadSegment(const RoadSegment& segment, bool bidirectional) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_segments.push_back(segment);
    m_adjacency[segment.fromIntersection].push_back(segment);

    // Auto-create nodes in map if not explicitly added
    if (m_intersections.find(segment.fromIntersection) == m_intersections.end()) {
        m_intersections[segment.fromIntersection] = IntersectionInfo(segment.fromIntersection, 0.0, 0.0);
    }
    if (m_intersections.find(segment.toIntersection) == m_intersections.end()) {
        m_intersections[segment.toIntersection] = IntersectionInfo(segment.toIntersection, 0.0, 0.0);
        if (m_adjacency.find(segment.toIntersection) == m_adjacency.end()) {
            m_adjacency[segment.toIntersection] = {};
        }
    }

    if (bidirectional) {
        RoadSegment rev(segment.toIntersection, segment.fromIntersection,
                        segment.distanceMeters, segment.estimatedTravelTimeSeconds);
        m_segments.push_back(rev);
        m_adjacency[segment.toIntersection].push_back(rev);
    }
}

void EmergencyCorridorPlanner::AddRoadSegment(const std::string& from, const std::string& to,
                                              double distanceMeters, double estimatedTravelTimeSeconds,
                                              bool bidirectional) {
    AddRoadSegment(RoadSegment(from, to, distanceMeters, estimatedTravelTimeSeconds), bidirectional);
}

void EmergencyCorridorPlanner::SetRoadSegments(const std::vector<RoadSegment>& segments) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_segments.clear();
    for (auto& pair : m_adjacency) {
        pair.second.clear();
    }
    for (const auto& seg : segments) {
        m_segments.push_back(seg);
        m_adjacency[seg.fromIntersection].push_back(seg);

        if (m_intersections.find(seg.fromIntersection) == m_intersections.end()) {
            m_intersections[seg.fromIntersection] = IntersectionInfo(seg.fromIntersection, 0.0, 0.0);
        }
        if (m_intersections.find(seg.toIntersection) == m_intersections.end()) {
            m_intersections[seg.toIntersection] = IntersectionInfo(seg.toIntersection, 0.0, 0.0);
            if (m_adjacency.find(seg.toIntersection) == m_adjacency.end()) {
                m_adjacency[seg.toIntersection] = {};
            }
        }
    }
}

void EmergencyCorridorPlanner::ClearGraph() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections.clear();
    m_adjacency.clear();
    m_segments.clear();
    m_currentPlan = EmergencyRoute{};
    m_lastRouteLog.clear();
}

void EmergencyCorridorPlanner::SetAmbulancePosition(double lat, double lon, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulanceLat = lat;
    m_ambulanceLon = lon;
    m_ambulanceId = ambulanceId;
    m_ambulancePosSet = true;
    m_ambulanceStartNode.clear();
}

void EmergencyCorridorPlanner::SetAmbulancePosition(const std::string& nearestIntersectionId,
                                                    const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulanceStartNode = nearestIntersectionId;
    m_ambulanceId = ambulanceId;
    m_ambulancePosSet = true;
}

void EmergencyCorridorPlanner::SetDestinationHospital(const std::string& hospitalId, double lat, double lon) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hospitalId = hospitalId;
    m_hospitalLat = lat;
    m_hospitalLon = lon;
    m_hospitalSet = true;
}

void EmergencyCorridorPlanner::SetCostMetric(CostMetric metric) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_costMetric = metric;
}

bool EmergencyCorridorPlanner::HasIntersection(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.find(id) != m_intersections.end();
}

IntersectionInfo EmergencyCorridorPlanner::GetIntersection(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(id);
    if (it != m_intersections.end()) {
        return it->second;
    }
    return IntersectionInfo{};
}

std::vector<IntersectionInfo> EmergencyCorridorPlanner::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionInfo> result;
    result.reserve(m_intersections.size());
    for (const auto& pair : m_intersections) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<RoadSegment> EmergencyCorridorPlanner::GetAllRoadSegments() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_segments;
}

size_t EmergencyCorridorPlanner::GetIntersectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.size();
}

size_t EmergencyCorridorPlanner::GetRoadSegmentCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_segments.size();
}

std::string EmergencyCorridorPlanner::FindNearestIntersection(double lat, double lon) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_intersections.empty()) {
        return "";
    }

    std::string bestId = "";
    double bestDistance = std::numeric_limits<double>::infinity();
    common::GeoCoordinate targetCoord{lat, lon};

    for (const auto& pair : m_intersections) {
        common::GeoCoordinate nodeCoord{pair.second.latitude, pair.second.longitude};
        double dist = common::CalculateHaversineDistance(targetCoord, nodeCoord);
        if (dist < bestDistance) {
            bestDistance = dist;
            bestId = pair.first;
        }
    }
    return bestId;
}

// =============================================================================
// Dijkstra Shortest-Path Implementation
// =============================================================================
EmergencyRoute EmergencyCorridorPlanner::RunDijkstra(
    const std::unordered_map<std::string, IntersectionInfo>& nodes,
    const std::unordered_map<std::string, std::vector<RoadSegment>>& adj,
    const std::string& startId,
    const std::string& goalId,
    const std::string& ambId,
    double initialEtaOffset,
    double initialDistOffset,
    CostMetric metric
) const {
    EmergencyRoute route;
    route.ambulanceId = ambId;
    route.hospitalId = goalId;
    route.isValid = false;

    if (nodes.find(startId) == nodes.end() || nodes.find(goalId) == nodes.end()) {
        return route;
    }

    // Direct match: start is already at destination
    if (startId == goalId) {
        route.intersections = {startId};
        route.totalDistance = initialDistOffset;
        route.estimatedArrivalTime = initialEtaOffset;
        route.timeline.push_back(IntersectionEta(startId, initialEtaOffset, initialDistOffset));
        route.isValid = true;
        return route;
    }

    // Min-heap structure with deterministic tie-breaking
    struct NodeCandidate {
        double cost;
        size_t hops;
        std::string id;

        bool operator>(const NodeCandidate& other) const noexcept {
            if (std::abs(cost - other.cost) > 1e-9) {
                return cost > other.cost;
            }
            if (hops != other.hops) {
                return hops > other.hops; // Prefer fewer hops on equal cost
            }
            return id > other.id; // Deterministic alphabetical tie-breaking
        }
    };

    std::priority_queue<NodeCandidate, std::vector<NodeCandidate>, std::greater<NodeCandidate>> openPq;
    std::unordered_map<std::string, double> distMap;
    std::unordered_map<std::string, size_t> hopsMap;
    std::unordered_map<std::string, std::string> prevNode;
    std::unordered_map<std::string, RoadSegment> edgeUsed;
    std::unordered_set<std::string> visited;

    for (const auto& pair : nodes) {
        distMap[pair.first] = std::numeric_limits<double>::infinity();
        hopsMap[pair.first] = std::numeric_limits<size_t>::max();
    }

    distMap[startId] = 0.0;
    hopsMap[startId] = 0;
    openPq.push(NodeCandidate{0.0, 0, startId});

    while (!openPq.empty()) {
        auto current = openPq.top();
        openPq.pop();

        if (visited.find(current.id) != visited.end()) {
            continue;
        }
        visited.insert(current.id);

        if (current.id == goalId) {
            break;
        }

        auto adjIt = adj.find(current.id);
        if (adjIt == adj.end()) {
            continue;
        }

        for (const auto& edge : adjIt->second) {
            const std::string& neighbor = edge.toIntersection;
            if (visited.find(neighbor) != visited.end()) {
                continue;
            }

            double edgeCost = (metric == CostMetric::TRAVEL_TIME)
                                  ? edge.estimatedTravelTimeSeconds
                                  : edge.distanceMeters;
            if (edgeCost < 0.0) {
                edgeCost = 0.0;
            }

            double newCost = current.cost + edgeCost;
            size_t newHops = current.hops + 1;
            double currentDist = distMap[neighbor];

            if (newCost < currentDist - 1e-9) {
                distMap[neighbor] = newCost;
                hopsMap[neighbor] = newHops;
                prevNode[neighbor] = current.id;
                edgeUsed[neighbor] = edge;
                openPq.push(NodeCandidate{newCost, newHops, neighbor});
            } else if (std::abs(newCost - currentDist) <= 1e-9) {
                // Deterministic tie-breaking: prefer fewer hops, then lexicographically smaller predecessor
                size_t currentHops = hopsMap[neighbor];
                bool takeNew = false;
                if (newHops < currentHops) {
                    takeNew = true;
                } else if (newHops == currentHops && current.id < prevNode[neighbor]) {
                    takeNew = true;
                }

                if (takeNew) {
                    distMap[neighbor] = newCost;
                    hopsMap[neighbor] = newHops;
                    prevNode[neighbor] = current.id;
                    edgeUsed[neighbor] = edge;
                    openPq.push(NodeCandidate{newCost, newHops, neighbor});
                }
            }
        }
    }

    if (distMap[goalId] >= std::numeric_limits<double>::infinity() || prevNode.find(goalId) == prevNode.end()) {
        // No path exists
        return route;
    }

    // Path Reconstruction
    std::vector<std::string> path;
    std::string curr = goalId;
    while (curr != startId) {
        path.push_back(curr);
        auto it = prevNode.find(curr);
        if (it == prevNode.end()) {
            return route; // Safety break
        }
        curr = it->second;
    }
    path.push_back(startId);
    std::reverse(path.begin(), path.end());

    // Build timeline and cumulative totals
    route.intersections = path;
    route.isValid = true;

    double cumulativeDist = initialDistOffset;
    double cumulativeTime = initialEtaOffset;

    route.timeline.reserve(path.size());
    route.timeline.push_back(IntersectionEta(path.front(), cumulativeTime, cumulativeDist));

    for (size_t i = 1; i < path.size(); ++i) {
        const auto& seg = edgeUsed.at(path[i]);
        cumulativeDist += seg.distanceMeters;
        cumulativeTime += seg.estimatedTravelTimeSeconds;
        route.timeline.push_back(IntersectionEta(path[i], cumulativeTime, cumulativeDist));
    }

    route.totalDistance = cumulativeDist;
    route.estimatedArrivalTime = cumulativeTime;
    return route;
}

// =============================================================================
// Primary Route Planning APIs
// =============================================================================
EmergencyRoute EmergencyCorridorPlanner::ComputeEmergencyRoute() {
    std::unordered_map<std::string, IntersectionInfo> nodes;
    std::unordered_map<std::string, std::vector<RoadSegment>> adj;
    std::string startNode;
    std::string goalNode;
    std::string ambId;
    double initialEta = 0.0;
    double initialDist = 0.0;
    CostMetric metric;
    bool doLogging = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        nodes = m_intersections;
        adj = m_adjacency;
        ambId = m_ambulanceId;
        goalNode = m_hospitalId;
        metric = m_costMetric;
        doLogging = m_autoLogging;

        if (!m_ambulanceStartNode.empty()) {
            startNode = m_ambulanceStartNode;
        } else if (m_ambulancePosSet) {
            // Find nearest intersection to GPS coordinates
            double bestD = std::numeric_limits<double>::infinity();
            common::GeoCoordinate ambCoord{m_ambulanceLat, m_ambulanceLon};
            for (const auto& pair : m_intersections) {
                common::GeoCoordinate nodeCoord{pair.second.latitude, pair.second.longitude};
                double d = common::CalculateHaversineDistance(ambCoord, nodeCoord);
                if (d < bestD) {
                    bestD = d;
                    startNode = pair.first;
                }
            }
            if (!startNode.empty() && bestD < std::numeric_limits<double>::infinity()) {
                initialDist = bestD;
                // Average urban speed: 50 km/h = 13.888 m/s
                initialEta = common::CalculateEtaSeconds(initialDist, 50.0);
            }
        }
    }

    EmergencyRoute route = RunDijkstra(nodes, adj, startNode, goalNode, ambId, initialEta, initialDist, metric);

    std::string logStr = FormatRouteLog(route);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentPlan = route;
        m_lastRouteLog = logStr;
    }

    if (doLogging) {
        PrintRouteLog(route);
    }
    return route;
}

EmergencyRoute EmergencyCorridorPlanner::ComputeEmergencyRoute(const std::string& startIntersectionId,
                                                              const std::string& destinationHospitalId) {
    return ComputeEmergencyRoute("AMBULANCE", startIntersectionId, destinationHospitalId, 0.0, 0.0);
}

EmergencyRoute EmergencyCorridorPlanner::ComputeEmergencyRoute(const std::string& ambulanceId,
                                                              const std::string& startIntersectionId,
                                                              const std::string& destinationHospitalId,
                                                              double initialEtaOffset,
                                                              double initialDistOffset) {
    std::unordered_map<std::string, IntersectionInfo> nodes;
    std::unordered_map<std::string, std::vector<RoadSegment>> adj;
    CostMetric metric;
    bool doLogging = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        nodes = m_intersections;
        adj = m_adjacency;
        metric = m_costMetric;
        doLogging = m_autoLogging;
    }

    EmergencyRoute route = RunDijkstra(nodes, adj, startIntersectionId, destinationHospitalId,
                                       ambulanceId, initialEtaOffset, initialDistOffset, metric);

    std::string logStr = FormatRouteLog(route);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentPlan = route;
        m_lastRouteLog = logStr;
    }

    if (doLogging) {
        PrintRouteLog(route);
    }
    return route;
}

EmergencyRoute EmergencyCorridorPlanner::ComputeEmergencyRoute(const std::string& ambulanceId,
                                                              double ambLat, double ambLon,
                                                              const std::string& destinationHospitalId) {
    std::string nearest = FindNearestIntersection(ambLat, ambLon);
    double initialDist = 0.0;
    double initialEta = 0.0;
    if (!nearest.empty()) {
        auto info = GetIntersection(nearest);
        common::GeoCoordinate ambCoord{ambLat, ambLon};
        common::GeoCoordinate nodeCoord{info.latitude, info.longitude};
        initialDist = common::CalculateHaversineDistance(ambCoord, nodeCoord);
        initialEta = common::CalculateEtaSeconds(initialDist, 50.0);
    }
    return ComputeEmergencyRoute(ambulanceId, nearest, destinationHospitalId, initialEta, initialDist);
}

EmergencyRoute EmergencyCorridorPlanner::ComputeEmergencyRoute(const std::string& ambulanceId,
                                                              double ambLat, double ambLon,
                                                              const std::string& destinationHospitalId,
                                                              double hospLat, double hospLon) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_intersections.find(destinationHospitalId) == m_intersections.end()) {
            m_intersections[destinationHospitalId] = IntersectionInfo(destinationHospitalId, hospLat, hospLon);
        }
    }
    return ComputeEmergencyRoute(ambulanceId, ambLat, ambLon, destinationHospitalId);
}

std::vector<std::string> EmergencyCorridorPlanner::ComputeGreenCorridor() {
    EmergencyRoute route = ComputeEmergencyRoute();
    return route.intersections;
}

std::vector<std::string> EmergencyCorridorPlanner::ComputeGreenCorridor(const std::string& startIntersectionId,
                                                                      const std::string& destinationHospitalId) {
    EmergencyRoute route = ComputeEmergencyRoute(startIntersectionId, destinationHospitalId);
    return route.intersections;
}

std::vector<std::string> EmergencyCorridorPlanner::ComputeGreenCorridor(const std::string& ambulanceId,
                                                                      const std::string& startIntersectionId,
                                                                      const std::string& destinationHospitalId) {
    EmergencyRoute route = ComputeEmergencyRoute(ambulanceId, startIntersectionId, destinationHospitalId);
    return route.intersections;
}

std::vector<IntersectionEta> EmergencyCorridorPlanner::EstimateArrivalTimes() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentPlan.timeline;
}

std::vector<IntersectionEta> EmergencyCorridorPlanner::EstimateArrivalTimes(const std::vector<std::string>& corridor) const {
    std::unordered_map<std::string, std::vector<RoadSegment>> adj;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        adj = m_adjacency;
    }

    std::vector<IntersectionEta> timeline;
    if (corridor.empty()) {
        return timeline;
    }

    timeline.reserve(corridor.size());
    double cumDist = 0.0;
    double cumEta = 0.0;
    timeline.push_back(IntersectionEta(corridor.front(), cumEta, cumDist));

    for (size_t i = 1; i < corridor.size(); ++i) {
        const std::string& u = corridor[i - 1];
        const std::string& v = corridor[i];
        bool found = false;

        auto it = adj.find(u);
        if (it != adj.end()) {
            for (const auto& seg : it->second) {
                if (seg.toIntersection == v) {
                    cumDist += seg.distanceMeters;
                    cumEta += seg.estimatedTravelTimeSeconds;
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            // Unconnected hop; add 0 delta
        }
        timeline.push_back(IntersectionEta(v, cumEta, cumDist));
    }
    return timeline;
}

std::vector<IntersectionEta> EmergencyCorridorPlanner::EstimateArrivalTimes(const EmergencyRoute& route) const {
    return route.timeline;
}

EmergencyRoute EmergencyCorridorPlanner::GetCurrentPlan() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentPlan;
}

void EmergencyCorridorPlanner::ClearPlan() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentPlan = EmergencyRoute{};
    m_lastRouteLog.clear();
}

// =============================================================================
// Formatting & Structured Logging
// =============================================================================
std::string EmergencyCorridorPlanner::FormatGreenCorridor(const std::vector<std::string>& corridor) const {
    if (corridor.empty()) {
        return "(empty)";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < corridor.size(); ++i) {
        oss << corridor[i];
        if (i + 1 < corridor.size()) {
            oss << "\n↓\n";
        }
    }
    return oss.str();
}

std::string EmergencyCorridorPlanner::FormatGreenCorridor() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return FormatGreenCorridor(m_currentPlan.intersections);
}

std::string EmergencyCorridorPlanner::FormatTimeline(const std::vector<IntersectionEta>& timeline) const {
    if (timeline.empty()) {
        return "(empty timeline)";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < timeline.size(); ++i) {
        oss << timeline[i].intersectionId << "\nETA "
            << static_cast<long long>(std::round(timeline[i].etaSeconds)) << " s";
        if (i + 1 < timeline.size()) {
            oss << "\n\n";
        }
    }
    return oss.str();
}

std::string EmergencyCorridorPlanner::FormatTimeline() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return FormatTimeline(m_currentPlan.timeline);
}

std::string EmergencyCorridorPlanner::FormatRouteLog(const EmergencyRoute& route) {
    std::ostringstream oss;
    oss << "[ROUTE GENERATED]\n";
    oss << "Ambulance: " << (route.ambulanceId.empty() ? "AMBULANCE" : route.ambulanceId) << "\n";
    oss << "Hospital: " << (route.hospitalId.empty() ? "HOSPITAL" : route.hospitalId) << "\n";
    oss << "Distance: " << std::fixed << std::setprecision(2) << route.totalDistance << " m\n";
    oss << "Travel Time: " << std::fixed << std::setprecision(2) << route.estimatedArrivalTime << " s\n";
    oss << "Ordered Corridor:\n";

    if (route.intersections.empty()) {
        oss << "(None - No Route Available)";
    } else {
        for (size_t i = 0; i < route.intersections.size(); ++i) {
            oss << route.intersections[i];
            if (i + 1 < route.intersections.size()) {
                oss << "\n↓\n";
            }
        }
    }
    return oss.str();
}

std::string EmergencyCorridorPlanner::GetLastRouteLog() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastRouteLog;
}

void EmergencyCorridorPlanner::PrintRouteLog(const EmergencyRoute& route) const {
    std::cout << FormatRouteLog(route) << std::endl;
}

void EmergencyCorridorPlanner::EnableAutoLogging(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_autoLogging = enable;
}

} // namespace green_corridor
} // namespace aegis
