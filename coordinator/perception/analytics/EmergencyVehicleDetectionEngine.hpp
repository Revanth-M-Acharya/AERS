/**
 * @file EmergencyVehicleDetectionEngine.hpp
 * @brief Detects conflicting emergency vehicles, convoys, and first responders on the road network.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/perception/RoadConditionEvent.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace aegis::coordinator::perception::analytics {

struct DetectedEmergencyVehicle {
    std::string vehicleDetectionId;
    RoadEventType vehicleType{RoadEventType::SECOND_AMBULANCE};
    std::string roadSegment;
    std::string approachingIntersection;
    double speedKmh{60.0};
    double distanceToIntersectionMeters{150.0};
    uint64_t detectedMonoMs{0};
};

class EmergencyVehicleDetectionEngine {
public:
    EmergencyVehicleDetectionEngine() = default;
    ~EmergencyVehicleDetectionEngine() = default;

    std::optional<RoadConditionEvent> ProcessVehicleDetection(const DetectedEmergencyVehicle& vehicle);

    std::vector<DetectedEmergencyVehicle> GetActiveVehicles(const std::string& intersectionId) const;
    bool HasConflictingEmergency(const std::string& intersectionId, const std::string& excludeSegment = "") const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DetectedEmergencyVehicle> activeVehicles_;
    uint64_t counter_{5000};
};

} // namespace aegis::coordinator::perception::analytics

