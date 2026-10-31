/**
 * @file CameraEventListener.hpp
 * @brief Extension interface for future AI CCTV and edge vision models.
 * Pure virtual interface — contains ZERO implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::traffic::extension {

class ICameraEventListener {
public:
    virtual ~ICameraEventListener() = default;

    /**
     * @brief Triggered when edge vision model classifies a road vehicle.
     */
    virtual void OnVehicleDetected(const std::string& vehicleClass, double confidence, uint64_t timestamp) = 0;

    /**
     * @brief Triggered when optical camera identifies an approaching emergency ambulance.
     */
    virtual void OnEmergencyVehicleApproaching(const std::string& laneId, double estimatedDistanceMeters) = 0;
};

} // namespace aegis::traffic::extension

