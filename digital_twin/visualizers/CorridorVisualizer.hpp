/**
 * @file CorridorVisualizer.hpp
 * @brief Visualizer for dynamic green wave corridors and emergency routing progression.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>

namespace aegis::digital_twin::visualizers {

struct CorridorStateOverview {
    std::string corridorId;
    std::string emergencyId;
    std::vector<std::string> path; // intersection IDs
    std::string activeIntersection;
    double totalDistanceMeters{0.0};
    double estimatedTransitSeconds{0.0};
    bool isFullyClear{true};
};

class CorridorVisualizer {
public:
    static std::string RenderAsciiCorridor(const CorridorStateOverview& corridor, const CityModel& city);
    static std::string ToJson(const CorridorStateOverview& corridor);
};

} // namespace aegis::digital_twin::visualizers

