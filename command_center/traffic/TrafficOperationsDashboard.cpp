/**
 * @file TrafficOperationsDashboard.cpp
 * @brief Implementation of Traffic Operations Dashboard.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/traffic/TrafficOperationsDashboard.hpp"

namespace aegis::command_center {

TrafficOperationsDashboard::TrafficOperationsDashboard() {
    InitializeDefaultIntersections();
}

void TrafficOperationsDashboard::InitializeDefaultIntersections() {
    std::lock_guard<std::mutex> lock(mutex_);
    intersections_.clear();
    displayOrder_.clear();

    auto add = [this](const std::string& id, const std::string& name,
                      const std::string& signal, IntersectionControlMode mode,
                      const std::string& amb, uint32_t vehicles, uint32_t ir, uint32_t queue) {
        IntersectionTelemetryView v;
        v.intersectionId = id;
        v.intersectionName = name;
        v.currentSignal = signal;
        v.currentMode = mode;
        v.currentAmbulanceId = amb;
        v.vehicleCount = vehicles;
        v.irSensorCount = ir;
        v.currentQueue = queue;
        intersections_[id] = v;
        displayOrder_.push_back(id);
    };

    add("INT_1", "Intersection 1 (RPi Traffic Node 1)", "ROAD_A_GREEN", IntersectionControlMode::NORMAL, "None", 48, 52, 2);
    add("INT_2", "Intersection 2 (RPi Traffic Node 2)", "ROAD_A_GREEN", IntersectionControlMode::NORMAL, "None", 64, 71, 3);
    add("INT_3", "Intersection 3 (RPi Traffic Node 3)", "ROAD_A_GREEN", IntersectionControlMode::NORMAL, "None", 41, 44, 1);
}

bool TrafficOperationsDashboard::UpdateIntersection(const IntersectionTelemetryView& view) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (intersections_.find(view.intersectionId) == intersections_.end()) {
        displayOrder_.push_back(view.intersectionId);
    }
    intersections_[view.intersectionId] = view;
    return true;
}

bool TrafficOperationsDashboard::UpdateSignalState(const std::string& intersectionId,
                                                   const std::string& signal,
                                                   IntersectionControlMode mode,
                                                   const std::string& ambulanceId,
                                                   uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }
    it->second.currentSignal = signal;
    it->second.currentMode = mode;
    it->second.currentAmbulanceId = ambulanceId;
    it->second.lastTransitionTimestampMonoMs = timestampMs;
    return true;
}

bool TrafficOperationsDashboard::RecordVehicleDetection(const std::string& intersectionId, uint32_t queueEstimate) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }
    it->second.vehicleCount++;
    it->second.irSensorCount++;
    it->second.currentQueue = queueEstimate;
    return true;
}

std::optional<IntersectionTelemetryView> TrafficOperationsDashboard::GetIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<IntersectionTelemetryView> TrafficOperationsDashboard::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IntersectionTelemetryView> results;
    results.reserve(displayOrder_.size());
    for (const auto& id : displayOrder_) {
        auto it = intersections_.find(id);
        if (it != intersections_.end()) {
            results.push_back(it->second);
        }
    }
    return results;
}

void TrafficOperationsDashboard::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    intersections_.clear();
    displayOrder_.clear();
}

} // namespace aegis::command_center
