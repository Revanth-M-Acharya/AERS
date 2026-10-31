/**
 * @file CameraVisualizer.hpp
 * @brief Visualizer for CCTV cameras, active feeds, health metrics, and vision detections.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "digital_twin/CityModel.hpp"
#include <string>
#include <vector>

namespace aegis::digital_twin::visualizers {

class CameraVisualizer {
public:
    static std::string RenderAsciiCameraGrid(const std::vector<CameraEntity>& cameras, const std::vector<RoadConditionEntity>& conditions);
    static std::string ToJson(const CameraEntity& camera);
};

} // namespace aegis::digital_twin::visualizers

