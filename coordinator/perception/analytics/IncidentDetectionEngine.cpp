/**
 * @file IncidentDetectionEngine.cpp
 * @brief Implementation of IncidentDetectionEngine pattern analysis and event creation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/IncidentDetectionEngine.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::perception::analytics {

std::optional<RoadConditionEvent> IncidentDetectionEngine::AnalyzeVisionCluster(
    const VisionDetectionCluster& cluster) {

    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    RoadConditionEvent evt;
    evt.roadSegment = cluster.roadSegment;
    evt.cameraId = cluster.cameraId;
    evt.timestampMonoMs = now;
    evt.confidence = cluster.confidence;
    evt.verificationStatus = VerificationStatus::UNVERIFIED;

    if (cluster.wrongWayDetected) {
        evt.type = RoadEventType::WRONG_WAY_VEHICLE;
        evt.severity = EventSeverity::CRITICAL;
        evt.description = "Vehicle detected driving against traffic flow";
        evt.estimatedDelaySeconds = 90.0;
        evt.isBlocking = true;
    } else if (cluster.stoppedVehicleCount >= 2 && cluster.stationaryDurationSec > 15.0) {
        evt.type = RoadEventType::ACCIDENT;
        evt.severity = EventSeverity::HIGH;
        evt.description = "Multi-vehicle collision or major stoppage detected";
        evt.estimatedDelaySeconds = 300.0;
        evt.isBlocking = (cluster.stoppedVehicleCount >= 3);
        evt.affectedLanes = {1, 2};
    } else if (cluster.stoppedVehicleCount == 1 && cluster.stationaryDurationSec > 30.0) {
        evt.type = RoadEventType::STOPPED_VEHICLE;
        evt.severity = EventSeverity::MEDIUM;
        evt.description = "Disabled or stationary vehicle obstructing lane";
        evt.estimatedDelaySeconds = 60.0;
        evt.affectedLanes = {1};
    } else if (cluster.pedestrianOnRoadway) {
        evt.type = RoadEventType::PEDESTRIAN_CROWD;
        evt.severity = EventSeverity::HIGH;
        evt.description = "Pedestrians on active roadway";
        evt.estimatedDelaySeconds = 120.0;
        evt.isBlocking = false;
    } else if (cluster.smokeOrFogDetected) {
        evt.type = RoadEventType::SMOKE;
        evt.severity = EventSeverity::MEDIUM;
        evt.description = "Dense smoke or visual obscuration on road";
        evt.estimatedDelaySeconds = 45.0;
    } else {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    evt.eventId = "INC-" + std::to_string(++incidentCounter_);
    activeIncidents_[evt.eventId] = evt;
    return evt;
}

void IncidentDetectionEngine::RecordIncident(const RoadConditionEvent& incident) {
    std::lock_guard<std::mutex> lock(mutex_);
    activeIncidents_[incident.eventId] = incident;
}

std::vector<RoadConditionEvent> IncidentDetectionEngine::GetActiveIncidents(const std::string& roadSegment) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadConditionEvent> results;
    for (const auto& [_, inc] : activeIncidents_) {
        if (inc.roadSegment == roadSegment && inc.verificationStatus != VerificationStatus::RESOLVED) {
            results.push_back(inc);
        }
    }
    return results;
}

std::vector<RoadConditionEvent> IncidentDetectionEngine::GetAllIncidents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadConditionEvent> results;
    results.reserve(activeIncidents_.size());
    for (const auto& [_, inc] : activeIncidents_) {
        results.push_back(inc);
    }
    return results;
}

bool IncidentDetectionEngine::ResolveIncident(const std::string& eventId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = activeIncidents_.find(eventId);
    if (it == activeIncidents_.end()) return false;
    it->second.verificationStatus = VerificationStatus::RESOLVED;
    return true;
}

void IncidentDetectionEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    activeIncidents_.clear();
}

} // namespace aegis::coordinator::perception::analytics

