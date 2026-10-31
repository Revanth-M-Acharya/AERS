/**
 * @file CorridorVisualizer.cpp
 * @brief Implementation of CorridorVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/CorridorVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string CorridorVisualizer::RenderAsciiCorridor(const CorridorStateOverview& corridor, const CityModel& city) {
    std::ostringstream ss;
    ss << "=== DYNAMIC GREEN CORRIDOR: " << corridor.corridorId
       << " [EMERGENCY: " << corridor.emergencyId << "] ===\n"
       << " Transit ETA: " << std::fixed << std::setprecision(1) << corridor.estimatedTransitSeconds << "s"
       << " | Distance: " << corridor.totalDistanceMeters << "m"
       << " | Clearance: " << (corridor.isFullyClear ? "OPTIMAL [100% CLEAR]" : "OBSTRUCTION DETECTED") << "\n"
       << " Pathway: ";

    for (size_t i = 0; i < corridor.path.size(); ++i) {
        const auto& node = city.GetIntersection(corridor.path[i]);
        ss << "[" << node.intersectionId << ": " << node.signalState << "]";
        if (i + 1 < corridor.path.size()) {
            std::string segId = "SEG_" + corridor.path[i] + "_" + corridor.path[i+1];
            auto seg = city.GetRoadSegment(segId);
            ss << " ===(Suitability: " << static_cast<int>(seg.suitabilityScore) << ")===> ";
        }
    }
    ss << "\n================================================================================\n";
    return ss.str();
}

std::string CorridorVisualizer::ToJson(const CorridorStateOverview& corridor) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << corridor.corridorId << "\",\"emg\":\"" << corridor.emergencyId
       << "\",\"eta\":" << std::fixed << std::setprecision(1) << corridor.estimatedTransitSeconds
       << ",\"clear\":" << (corridor.isFullyClear ? "true" : "false")
       << ",\"path\":[";
    for (size_t i = 0; i < corridor.path.size(); ++i) {
        ss << "\"" << corridor.path[i] << "\"" << (i + 1 < corridor.path.size() ? "," : "");
    }
    ss << "]}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

