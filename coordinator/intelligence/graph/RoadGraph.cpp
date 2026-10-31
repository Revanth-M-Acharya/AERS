/**
 * @file RoadGraph.cpp
 * @brief Implementation of RoadGraph topological network model.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include <fstream>
#include <sstream>

namespace aegis::coordinator::intelligence {

namespace {
std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> Split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, delim)) {
        tokens.push_back(Trim(item));
    }
    return tokens;
}
} // namespace

RoadGraph::RoadGraph() = default;

RoadGraph::RoadGraph(const RoadGraph& other) {
    std::lock_guard<std::mutex> lock(other.mutex_);
    intersections_ = other.intersections_;
    segments_ = other.segments_;
    outgoingAdjList_ = other.outgoingAdjList_;
}

RoadGraph& RoadGraph::operator=(const RoadGraph& other) {
    if (this == &other) return *this;
    std::scoped_lock lock(mutex_, other.mutex_);
    intersections_ = other.intersections_;
    segments_ = other.segments_;
    outgoingAdjList_ = other.outgoingAdjList_;
    return *this;
}

void RoadGraph::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    intersections_.clear();
    segments_.clear();
    outgoingAdjList_.clear();
}

bool RoadGraph::AddIntersection(const Intersection& intersection) {
    std::lock_guard<std::mutex> lock(mutex_);
    intersections_[intersection.id] = intersection;
    if (outgoingAdjList_.find(intersection.id) == outgoingAdjList_.end()) {
        outgoingAdjList_[intersection.id] = {};
    }
    return true;
}

bool RoadGraph::AddRoadSegment(const RoadSegment& segment) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (intersections_.find(segment.fromIntersectionId) == intersections_.end() ||
        intersections_.find(segment.toIntersectionId) == intersections_.end()) {
        CoordinatorLogger::Instance().Warn("GRAPH", "Cannot add segment " + segment.segmentId +
                                           ": Endpoints not found (" + segment.fromIntersectionId + " -> " + segment.toIntersectionId + ")");
        return false;
    }

    segments_[segment.segmentId] = segment;
    outgoingAdjList_[segment.fromIntersectionId].push_back(segment.segmentId);

    // If bidirectional, register reverse segment edge
    if (segment.direction == LaneDirection::BIDIRECTIONAL) {
        std::string revSegmentId = segment.segmentId + "_REV";
        RoadSegment revSegment = segment;
        revSegment.segmentId = revSegmentId;
        revSegment.fromIntersectionId = segment.toIntersectionId;
        revSegment.toIntersectionId = segment.fromIntersectionId;
        segments_[revSegmentId] = revSegment;
        outgoingAdjList_[segment.toIntersectionId].push_back(revSegmentId);
    }

    intersections_[segment.fromIntersectionId].connectedSegmentIds.push_back(segment.segmentId);
    intersections_[segment.toIntersectionId].connectedSegmentIds.push_back(segment.segmentId);
    return true;
}

bool RoadGraph::SetSegmentBlocked(const std::string& segmentId, bool blocked) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(segmentId);
    if (it == segments_.end()) return false;
    it->second.isBlocked = blocked;

    // If reverse segment exists, update it too
    auto revIt = segments_.find(segmentId + "_REV");
    if (revIt != segments_.end()) {
        revIt->second.isBlocked = blocked;
    }
    CoordinatorLogger::Instance().Info("GRAPH", "Segment " + segmentId + (blocked ? " BLOCKED" : " UNBLOCKED"));
    return true;
}

bool RoadGraph::SetTemporaryClosure(const std::string& segmentId, bool closed) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(segmentId);
    if (it == segments_.end()) return false;
    it->second.temporaryClosure = closed;

    auto revIt = segments_.find(segmentId + "_REV");
    if (revIt != segments_.end()) {
        revIt->second.temporaryClosure = closed;
    }
    CoordinatorLogger::Instance().Info("GRAPH", "Segment " + segmentId + " temporary closure set to " + (closed ? "TRUE" : "FALSE"));
    return true;
}

bool RoadGraph::SetIntersectionEnabled(const std::string& intersectionId, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) return false;
    it->second.isEnabled = enabled;
    CoordinatorLogger::Instance().Info("GRAPH", "Intersection " + intersectionId + (enabled ? " ENABLED" : " DISABLED"));
    return true;
}

bool RoadGraph::UpdateSegmentSpeed(const std::string& segmentId, double currentSpeedKmh) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(segmentId);
    if (it == segments_.end()) return false;
    it->second.currentSpeedKmh = std::max(1.0, currentSpeedKmh);

    auto revIt = segments_.find(segmentId + "_REV");
    if (revIt != segments_.end()) {
        revIt->second.currentSpeedKmh = it->second.currentSpeedKmh;
    }
    return true;
}

bool RoadGraph::SetAiCostMultiplier(const std::string& segmentId, double multiplier) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(segmentId);
    if (it == segments_.end()) return false;
    it->second.aiCostMultiplier = std::max(0.01, multiplier);

    auto revIt = segments_.find(segmentId + "_REV");
    if (revIt != segments_.end()) {
        revIt->second.aiCostMultiplier = it->second.aiCostMultiplier;
    }
    return true;
}

std::vector<RoadSegment> RoadGraph::GetOutgoingSegments(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadSegment> result;
    auto nodeIt = intersections_.find(intersectionId);
    if (nodeIt == intersections_.end() || !nodeIt->second.isEnabled) {
        return result;
    }

    auto adjIt = outgoingAdjList_.find(intersectionId);
    if (adjIt == outgoingAdjList_.end()) {
        return result;
    }

    for (const auto& segId : adjIt->second) {
        auto sIt = segments_.find(segId);
        if (sIt != segments_.end()) {
            // Check if destination intersection is enabled
            auto destIt = intersections_.find(sIt->second.toIntersectionId);
            if (destIt != intersections_.end() && destIt->second.isEnabled) {
                result.push_back(sIt->second);
            }
        }
    }
    return result;
}

const Intersection* RoadGraph::GetIntersection(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(id);
    if (it != intersections_.end()) {
        return &(it->second);
    }
    return nullptr;
}

const RoadSegment* RoadGraph::GetSegment(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = segments_.find(segmentId);
    if (it != segments_.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<std::string> RoadGraph::GetAllIntersectionIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    ids.reserve(intersections_.size());
    for (const auto& pair : intersections_) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::string> RoadGraph::GetAllSegmentIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    ids.reserve(segments_.size());
    for (const auto& pair : segments_) {
        ids.push_back(pair.first);
    }
    return ids;
}

size_t RoadGraph::GetIntersectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return intersections_.size();
}

size_t RoadGraph::GetSegmentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return segments_.size();
}

bool RoadGraph::LoadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        CoordinatorLogger::Instance().Error("GRAPH", "Failed to open road network file: " + filePath);
        return false;
    }

    Clear();
    std::string line;

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string type = Trim(line.substr(0, eqPos));
        std::string data = Trim(line.substr(eqPos + 1));
        std::vector<std::string> fields = Split(data, ',');

        if (type == "INTERSECTION") {
            // ID, Name, Latitude, Longitude, Enabled
            if (fields.size() >= 5) {
                Intersection inter;
                inter.id = fields[0];
                inter.name = fields[1];
                inter.latitude = std::stod(fields[2]);
                inter.longitude = std::stod(fields[3]);
                inter.isEnabled = (fields[4] == "1" || fields[4] == "true");
                AddIntersection(inter);
            }
        } else if (type == "SEGMENT") {
            // ID, From, To, DistanceMeters, SpeedLimitKmh, Direction, LaneCount
            if (fields.size() >= 7) {
                RoadSegment seg;
                seg.segmentId = fields[0];
                seg.fromIntersectionId = fields[1];
                seg.toIntersectionId = fields[2];
                seg.distanceMeters = std::stod(fields[3]);
                seg.speedLimitKmh = std::stod(fields[4]);
                seg.currentSpeedKmh = seg.speedLimitKmh;
                seg.direction = StringToLaneDirection(fields[5]);
                seg.laneCount = static_cast<uint32_t>(std::stoul(fields[6]));
                AddRoadSegment(seg);
            }
        }
    }

    CoordinatorLogger::Instance().Info("GRAPH", "Loaded Road Graph from " + filePath + " (" +
                                       std::to_string(GetIntersectionCount()) + " intersections, " +
                                       std::to_string(GetSegmentCount()) + " segments).");
    return true;
}

} // namespace aegis::coordinator::intelligence
