/**
 * @file RoadGraph.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Road Graph & Dynamic Routing Network Implementation.
 */

#include "RoadGraph.h"
#include <queue>
#include <cmath>
#include <limits>
#include <algorithm>

namespace aegis {
namespace simulator {

RoadGraph::RoadGraph() = default;

void RoadGraph::AddNode(const RoadNode& node) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodes[node.id] = node;
    if (m_adjacency.find(node.id) == m_adjacency.end()) {
        m_adjacency[node.id] = {};
    }
}

bool RoadGraph::GetNode(const std::string& id, RoadNode& outNode) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(id);
    if (it != m_nodes.end()) {
        outNode = it->second;
        return true;
    }
    return false;
}

std::vector<RoadNode> RoadGraph::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<RoadNode> list;
    list.reserve(m_nodes.size());
    for (const auto& [id, n] : m_nodes) {
        (void)id;
        list.push_back(n);
    }
    return list;
}

void RoadGraph::AddEdge(const RoadEdge& edge) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_edges[edge.id] = edge;
    m_adjacency[edge.fromNode].push_back(edge.id);
}

bool RoadGraph::GetEdge(const std::string& id, RoadEdge& outEdge) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edges.find(id);
    if (it != m_edges.end()) {
        outEdge = it->second;
        return true;
    }
    return false;
}

std::vector<RoadEdge> RoadGraph::GetAllEdges() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<RoadEdge> list;
    list.reserve(m_edges.size());
    for (const auto& [id, e] : m_edges) {
        (void)id;
        list.push_back(e);
    }
    return list;
}

void RoadGraph::SetRoadClosed(const std::string& edgeId, bool closed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edges.find(edgeId);
    if (it != m_edges.end()) {
        it->second.isClosed = closed;
    }
}

void RoadGraph::SetCongestion(const std::string& edgeId, double congestionFactor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edges.find(edgeId);
    if (it != m_edges.end()) {
        it->second.congestionFactor = (congestionFactor < 1.0) ? 1.0 : congestionFactor;
    }
}

void RoadGraph::SetConstruction(const std::string& edgeId, bool underConstruction) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edges.find(edgeId);
    if (it != m_edges.end()) {
        it->second.isUnderConstruction = underConstruction;
    }
}

void RoadGraph::SetAccident(const std::string& edgeId, bool hasAccident) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edges.find(edgeId);
    if (it != m_edges.end()) {
        it->second.hasAccident = hasAccident;
    }
}

void RoadGraph::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodes.clear();
    m_edges.clear();
    m_adjacency.clear();
}

double RoadGraph::CalculateDistanceMeters(double lat1, double lon1, double lat2, double lon2) const {
    // Equirectangular distance approximation (accurate for city scale)
    constexpr double R = 6371000.0; // Earth radius in meters
    constexpr double degToRad = 3.14159265358979323846 / 180.0;
    double phi1 = lat1 * degToRad;
    double phi2 = lat2 * degToRad;
    double dPhi = (lat2 - lat1) * degToRad;
    double dLambda = (lon2 - lon1) * degToRad;
    double meanPhi = (phi1 + phi2) * 0.5;

    double x = dLambda * std::cos(meanPhi);
    double y = dPhi;
    return std::sqrt(x * x + y * y) * R;
}

std::vector<std::string> RoadGraph::FindShortestPath(const std::string& startNodeId,
                                                    const std::string& targetNodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (startNodeId == targetNodeId) {
        return {startNodeId};
    }
    if (m_nodes.find(startNodeId) == m_nodes.end() || m_nodes.find(targetNodeId) == m_nodes.end()) {
        return {};
    }

    // Dijkstra structures
    std::unordered_map<std::string, double> dist;
    std::unordered_map<std::string, std::string> prevNode;
    for (const auto& [nodeId, n] : m_nodes) {
        (void)n;
        dist[nodeId] = std::numeric_limits<double>::infinity();
    }
    dist[startNodeId] = 0.0;

    using NodeCostPair = std::pair<double, std::string>;
    std::priority_queue<NodeCostPair, std::vector<NodeCostPair>, std::greater<NodeCostPair>> pq;
    pq.push({0.0, startNodeId});

    while (!pq.empty()) {
        auto [currentCost, u] = pq.top();
        pq.pop();

        if (u == targetNodeId) break;
        if (currentCost > dist[u]) continue;

        auto adjIt = m_adjacency.find(u);
        if (adjIt == m_adjacency.end()) continue;

        for (const auto& edgeId : adjIt->second) {
            auto edgeIt = m_edges.find(edgeId);
            if (edgeIt == m_edges.end()) continue;

            const auto& edge = edgeIt->second;
            if (edge.isClosed) continue;

            double weight = edge.GetTravelTimeSeconds();
            const std::string& v = edge.toNode;

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                prevNode[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (dist[targetNodeId] == std::numeric_limits<double>::infinity()) {
        return {}; // No reachable path
    }

    // Reconstruct path
    std::vector<std::string> path;
    for (std::string curr = targetNodeId; !curr.empty(); curr = prevNode[curr]) {
        path.push_back(curr);
        if (curr == startNodeId) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

double RoadGraph::CalculatePathTravelTimeSeconds(const std::vector<std::string>& pathNodeIds) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (pathNodeIds.size() < 2) return 0.0;

    double totalTime = 0.0;
    for (size_t i = 0; i + 1 < pathNodeIds.size(); ++i) {
        const std::string& u = pathNodeIds[i];
        const std::string& v = pathNodeIds[i + 1];

        bool foundEdge = false;
        auto adjIt = m_adjacency.find(u);
        if (adjIt != m_adjacency.end()) {
            for (const auto& edgeId : adjIt->second) {
                auto edgeIt = m_edges.find(edgeId);
                if (edgeIt != m_edges.end() && edgeIt->second.toNode == v) {
                    totalTime += edgeIt->second.GetTravelTimeSeconds();
                    foundEdge = true;
                    break;
                }
            }
        }
        if (!foundEdge) {
            totalTime += 30.0; // Fallback default
        }
    }
    return totalTime;
}

} // namespace simulator
} // namespace aegis

