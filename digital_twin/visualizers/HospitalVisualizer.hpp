/**
 * @file HospitalVisualizer.hpp
 * @brief Visualizer for hospital ER capacity, ICU beds, triage readiness, and incoming patient alerts.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>

namespace aegis::digital_twin::visualizers {

class HospitalVisualizer {
public:
    static std::string RenderAsciiHospitalBoard(const std::vector<HospitalFacility>& hospitals);
    static std::string ToJson(const HospitalFacility& hospital);
};

} // namespace aegis::digital_twin::visualizers

