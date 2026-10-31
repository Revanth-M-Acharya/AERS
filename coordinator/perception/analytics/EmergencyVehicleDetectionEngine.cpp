/**
 * @file EmergencyVehicleDetectionEngine.cpp
 * @brief Implementation of EmergencyVehicleDetectionEngine first responder perception.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/perception/analytics/EmergencyVehicleDetectionEngine.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::perception::analytics {

std::optional<RoadConditionEvent> EmergencyVehicleDetectionEngine::ProcessVehicleDetection(
    const DetectedEmergencyVehicle& vehicle) {

    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    DetectedEmergencyVehicle veh = vehicle;
    if (veh.detectedMonoMs == 0) veh.detectedMonoMs = now;

    RoadConditionEvent evt;
    evt.type = veh.vehicleType;
    evt.roadSegment = veh.roadSegment;
    evt.timestampMonoMs = now;
    evt.confidence = 0.95;
    evt.severity = EventSeverity::HIGH;
    evt.verificationStatus = VerificationStatus::CONFIRMED;
    evt.description = "Detected emergency vehicle: " + RoadEventTypeToString(veh.vehicleType) +
                      " approaching " + veh.approachingIntersection;
    evt.estimatedDelaySeconds = 30.0;

    std::lock_guard<std::mutex> lock(mutex_);
    if (veh.vehicleDetectionId.empty()) {
        veh.vehicleDetectionId = "EV-" + std::to_string(++counter_);
    }
    evt.eventId = veh.vehicleDetectionId;
    activeVehicles_[veh.vehicleDetectionId] = veh;

    return evt;
}

std::vector<DetectedEmergencyVehicle> EmergencyVehicleDetectionEngine::GetActiveVehicles(
    const std::string& intersectionId) const {

    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DetectedEmergencyVehicle> results;
    for (const auto& [_, v] : activeVehicles_) {
        if (v.approachingIntersection == intersectionId) {
            results.push_back(v);
        }
    }
    return results;
}

bool EmergencyVehicleDetectionEngine::HasConflictingEmergency(
    const std::string& intersectionId,
    const std::string& excludeSegment) const {

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [_, v] : activeVehicles_) {
        if (v.approachingIntersection == intersectionId) {
            if (excludeSegment.empty() || v.roadSegment != excludeSegment) {
                return true;
            }
        }
    }
    return false;
}

void EmergencyVehicleDetectionEngine::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    activeVehicles_.clear();
}

} // namespace aegis::coordinator::perception::analytics

