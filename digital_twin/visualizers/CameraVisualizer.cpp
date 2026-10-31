/**
 * @file CameraVisualizer.cpp
 * @brief Implementation of CameraVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/CameraVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string CameraVisualizer::RenderAsciiCameraGrid(
    const std::vector<CameraEntity>& cameras,
    const std::vector<RoadConditionEntity>& conditions) {

    std::ostringstream ss;
    ss << "=== CCTV PERCEPTION & AI VISION FEEDS ==========================================\n";
    if (cameras.empty()) {
        ss << "  (No camera feeds registered)\n";
    } else {
        for (const auto& c : cameras) {
            ss << " [CAM] " << std::left << std::setw(12) << c.cameraId
               << " @" << std::setw(8) << c.intersectionId
               << " | Status: " << std::setw(10) << c.status
               << " | Health: " << std::fixed << std::setprecision(2) << c.healthScore
               << " | FPS: " << std::setprecision(1) << c.currentFps
               << " | Drops: " << c.droppedFrames
               << " | Stream: " << (c.isCorridorFeedActive ? "LIVE [CORRIDOR]" : "STANDBY") << "\n";
        }
    }

    if (!conditions.empty()) {
        ss << "--------------------------------------------------------------------------------\n"
           << " ACTIVE AI ROAD DETECTIONS:\n";
        for (const auto& d : conditions) {
            ss << "  * " << d.eventId << " (Inc: " << d.incidentId << ") [" << d.type << "] on "
               << d.roadSegment << " | Conf: " << static_cast<int>(d.confidence * 100) << "%"
               << " | Sev: " << d.severity
               << " | State: " << d.lifecycleState
               << (d.isBlocking ? " [BLOCKING]" : "") << "\n";
        }
    }
    ss << "================================================================================\n";
    return ss.str();
}

std::string CameraVisualizer::ToJson(const CameraEntity& camera) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << camera.cameraId << "\",\"int\":\"" << camera.intersectionId
       << "\",\"status\":\"" << camera.status << "\",\"health\":" << std::fixed << std::setprecision(2) << camera.healthScore
       << ",\"fps\":" << std::setprecision(1) << camera.currentFps
       << ",\"stream\":" << (camera.isCorridorFeedActive ? "true" : "false") << "}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

