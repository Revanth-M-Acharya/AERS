/**
 * @file LiveAmbulanceTracker.cpp
 * @brief Implementation of Live Ambulance Tracking subsystem.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/tracking/LiveAmbulanceTracker.hpp"

namespace aegis::command_center {

void LiveAmbulanceTracker::UpdateTracking(const AmbulanceTrackingState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    trackingMap_[state.ambulanceId] = state;
}

bool LiveAmbulanceTracker::UpdateIntersectionProgress(const std::string& ambulanceId,
                                                     const std::string& currentInt,
                                                     const std::string& nextInt,
                                                     double remainingDistMeters,
                                                     double speedKmh,
                                                     double etaSec) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = trackingMap_.find(ambulanceId);
    if (it == trackingMap_.end()) {
        return false;
    }
    it->second.currentIntersection = currentInt;
    it->second.nextIntersection = nextInt;
    it->second.remainingDistanceMeters = remainingDistMeters;
    it->second.currentSpeedKmh = speedKmh;
    it->second.estimatedArrivalSec = etaSec;
    return true;
}

bool LiveAmbulanceTracker::UpdateCorridorStatus(const std::string& ambulanceId, const std::string& corridorStatus) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = trackingMap_.find(ambulanceId);
    if (it == trackingMap_.end()) {
        return false;
    }
    it->second.greenCorridorStatus = corridorStatus;
    return true;
}

bool LiveAmbulanceTracker::RemoveTracking(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    return trackingMap_.erase(ambulanceId) > 0;
}

std::optional<AmbulanceTrackingState> LiveAmbulanceTracker::GetTracking(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = trackingMap_.find(ambulanceId);
    if (it == trackingMap_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<AmbulanceTrackingState> LiveAmbulanceTracker::GetAllActiveTracking() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AmbulanceTrackingState> results;
    results.reserve(trackingMap_.size());
    for (const auto& [_, state] : trackingMap_) {
        results.push_back(state);
    }
    return results;
}

void LiveAmbulanceTracker::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    trackingMap_.clear();
}

} // namespace aegis::command_center
