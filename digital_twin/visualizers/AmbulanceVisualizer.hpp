/**
 * @file AmbulanceVisualizer.hpp
 * @brief Visualizer for ambulance fleet telemetry, location, status, and patient vitals.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>

namespace aegis::digital_twin::visualizers {

class AmbulanceVisualizer {
public:
    static std::string RenderAsciiCard(const AmbulanceEntity& amb);
    static std::string RenderFleetDashboard(const std::vector<AmbulanceEntity>& fleet);
    static std::string ToJson(const AmbulanceEntity& amb);
};

} // namespace aegis::digital_twin::visualizers

