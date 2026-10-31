/**
 * @file TrafficSignalVisualizer.hpp
 * @brief Visualizer for traffic signal states, preemption status, relays, and ESP32 nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>

namespace aegis::digital_twin::visualizers {

class TrafficSignalVisualizer {
public:
    static std::string RenderAsciiIntersection(const IntersectionNode& node);
    static std::string RenderIntersectionsSummary(const std::vector<IntersectionNode>& nodes);
    static std::string ToJson(const IntersectionNode& node);
};

} // namespace aegis::digital_twin::visualizers

