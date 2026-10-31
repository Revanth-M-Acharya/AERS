/**
 * @file RoadGraph.hpp
 * @brief Weighted directed graph representation of the road network topology.
 * Supports dynamic edge costs, temporary closures, and road blockages.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <limits>
#include <algorithm>

namespace aegis::coordinator::intelligence {

enum class LaneDirection {
    FORWARD,
    REVERSE,
    BIDIRECTIONAL
};

inline std::string LaneDirectionToString(LaneDirection dir) {
    switch (dir) {
        case LaneDirection::FORWARD: return "FORWARD";
        case LaneDirection::REVERSE: return "REVERSE";
        case LaneDirection::BIDIRECTIONAL: return "BIDIRECTIONAL";
        default: return "FORWARD";
    }
}

inline LaneDirection StringToLaneDirection(const std::string& str) {
    if (str == "REVERSE") return LaneDirection::REVERSE;
    if (str == "BIDIRECTIONAL") return LaneDirection::BIDIRECTIONAL;
    return LaneDirection::FORWARD;
}

struct Intersection {
    std::string id;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    bool isEnabled{true};
    std::vector<std::string> connectedSegmentIds;
};

struct RoadSegment {
    std::string segmentId;
    std::string fromIntersectionId;
    std::string toIntersectionId;
    double distanceMeters{100.0};
    double speedLimitKmh{50.0};
    double currentSpeedKmh{50.0};
    LaneDirection direction{LaneDirection::FORWARD};
    bool isBlocked{false};
    bool temporaryClosure{false};
    double aiCostMultiplier{1.0};
    uint32_t laneCount{1};

    /**
     * @brief Compute effective traversal time in seconds.
     * Returns huge value if segment is blocked or closed.
     */
    double GetEffectiveTravelTimeSec() const {
        if (isBlocked || temporaryClosure) {
            return 1e9; // Effectively infinity
        }
        double speedMs = std::max(1.0, currentSpeedKmh * (1000.0 / 3600.0));
        double baseTime = distanceMeters / speedMs;
        return baseTime * std::max(0.1, aiCostMultiplier);
    }
};

class RoadGraph {
public:
    RoadGraph();
    ~RoadGraph() = default;

    // Prevent accidental slicing
    RoadGraph(const RoadGraph& other);
    RoadGraph& operator=(const RoadGraph& other);

    /**
     * @brief Load topological definition from configuration file.
     * @param filePath Path to RoadNetwork.conf.
     * @return true on successful parsing, false otherwise.
     */
    bool LoadFromFile(const std::string& filePath);

    bool AddIntersection(const Intersection& intersection);
    bool AddRoadSegment(const RoadSegment& segment);

    bool SetSegmentBlocked(const std::string& segmentId, bool blocked);
    bool SetTemporaryClosure(const std::string& segmentId, bool closed);
    bool SetIntersectionEnabled(const std::string& intersectionId, bool enabled);
    bool UpdateSegmentSpeed(const std::string& segmentId, double currentSpeedKmh);
    bool SetAiCostMultiplier(const std::string& segmentId, double multiplier);

    std::vector<RoadSegment> GetOutgoingSegments(const std::string& intersectionId) const;
    const Intersection* GetIntersection(const std::string& id) const;
    const RoadSegment* GetSegment(const std::string& segmentId) const;

    std::vector<std::string> GetAllIntersectionIds() const;
    std::vector<std::string> GetAllSegmentIds() const;

    size_t GetIntersectionCount() const;
    size_t GetSegmentCount() const;

    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, Intersection> intersections_;
    std::unordered_map<std::string, RoadSegment> segments_;
    std::unordered_map<std::string, std::vector<std::string>> outgoingAdjList_;
};

} // namespace aegis::coordinator::intelligence

