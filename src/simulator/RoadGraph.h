/**
 * @file RoadGraph.h
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Road Graph & Dynamic Routing Network.
 */

#ifndef AEGIS_SIMULATOR_ROAD_GRAPH_H
#define AEGIS_SIMULATOR_ROAD_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief A topological junction or waypoint in the road network.
 */
struct RoadNode {
    std::string id;
    std::string label;
    double latitude{0.0};
    double longitude{0.0};
    bool isIntersection{false};
};

/**
 * @brief A directional roadway connecting two nodes.
 */
struct RoadEdge {
    std::string id;
    std::string fromNode;
    std::string toNode;
    double lengthMeters{100.0};
    double baseSpeedLimitKmh{50.0};
    uint8_t numLanes{2};

    // Dynamic environmental modifiers
    double congestionFactor{1.0};      // 1.0 (free flow) to 5.0 (gridlock)
    bool isClosed{false};
    bool isUnderConstruction{false};
    bool hasAccident{false};

    double GetEffectiveSpeedKmh() const {
        if (isClosed) return 0.0;
        double speed = baseSpeedLimitKmh / congestionFactor;
        if (isUnderConstruction) speed *= 0.6;
        if (hasAccident) speed *= 0.4;
        return (speed < 5.0) ? 5.0 : speed;
    }

    double GetTravelTimeSeconds() const {
        if (isClosed) return 1e9; // Impassable
        double speedMps = GetEffectiveSpeedKmh() * 0.277778;
        return (speedMps > 0.0) ? (lengthMeters / speedMps) : 1e9;
    }
};

/**
 * @brief Spatial segment along a roadway with lane geometry.
 */
struct RoadSegment {
    std::string segmentId;
    std::string edgeId;
    double startFraction{0.0}; // 0.0 to 1.0 along edge
    double endFraction{1.0};
    uint8_t laneIndex{0};
    uint32_t currentVehicleCount{0};
};

/**
 * @brief Thread-safe directed graph with dynamic edge-cost Dijkstra shortest path calculation.
 */
class RoadGraph {
public:
    RoadGraph();
    ~RoadGraph() = default;

    // Node & Edge Management
    void AddNode(const RoadNode& node);
    bool GetNode(const std::string& id, RoadNode& outNode) const;
    std::vector<RoadNode> GetAllNodes() const;

    void AddEdge(const RoadEdge& edge);
    bool GetEdge(const std::string& id, RoadEdge& outEdge) const;
    std::vector<RoadEdge> GetAllEdges() const;

    // Modifiers
    void SetRoadClosed(const std::string& edgeId, bool closed);
    void SetCongestion(const std::string& edgeId, double congestionFactor);
    void SetConstruction(const std::string& edgeId, bool underConstruction);
    void SetAccident(const std::string& edgeId, bool hasAccident);

    // Pathfinding
    std::vector<std::string> FindShortestPath(const std::string& startNodeId,
                                             const std::string& targetNodeId) const;
    double CalculatePathTravelTimeSeconds(const std::vector<std::string>& pathNodeIds) const;
    double CalculateDistanceMeters(double lat1, double lon1, double lat2, double lon2) const;

    void Clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, RoadNode> m_nodes;
    std::unordered_map<std::string, RoadEdge> m_edges;
    std::unordered_map<std::string, std::vector<std::string>> m_adjacency; // nodeId -> list of edgeIds
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_ROAD_GRAPH_H

