/**
 * @file AmbulanceVisualizer.cpp
 * @brief Implementation of AmbulanceVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/AmbulanceVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string AmbulanceVisualizer::RenderAsciiCard(const AmbulanceEntity& amb) {
    std::ostringstream ss;
    ss << "+--------------------------------------------------------+\n"
       << "| AMBULANCE: " << std::left << std::setw(12) << amb.ambulanceId
       << " [" << std::setw(4) << amb.vehicleType << "] "
       << "Callsign: " << std::setw(12) << amb.callSign << "|\n"
       << "+--------------------------------------------------------+\n"
       << "| Status   : " << std::setw(18) << amb.status
       << " Priority: " << std::setw(17) << amb.priorityLevel << "|\n"
       << "| Speed    : " << std::fixed << std::setprecision(1) << amb.speedKmh << " km/h"
       << "          ETA     : " << std::setprecision(1) << amb.etaSeconds << "s         |\n"
       << "| Segment  : " << std::setw(18) << amb.currentSegmentId
       << " Target  : " << std::setw(17) << amb.targetIntersectionId << "|\n"
       << "| Vitals   : HR " << std::setw(3) << amb.heartRate << " bpm | BP "
       << amb.systolicBp << "/" << amb.diastolicBp << " mmHg | SpO2 " << amb.spO2 << "%  |\n"
       << "+--------------------------------------------------------+\n";
    return ss.str();
}

std::string AmbulanceVisualizer::RenderFleetDashboard(const std::vector<AmbulanceEntity>& fleet) {
    std::ostringstream ss;
    ss << "=== AMBULANCE FLEET OPERATIONS =================================================\n";
    if (fleet.empty()) {
        ss << "  (No active ambulances registered)\n";
    } else {
        for (const auto& a : fleet) {
            ss << " * " << a.ambulanceId << " [" << a.vehicleType << "] " << a.callSign
               << " | Status: " << a.status
               << " | Spd: " << std::fixed << std::setprecision(1) << a.speedKmh << " km/h"
               << " | Seg: " << (a.currentSegmentId.empty() ? "N/A" : a.currentSegmentId)
               << " | Vitals: HR=" << a.heartRate << " BP=" << a.systolicBp << "/" << a.diastolicBp
               << " SpO2=" << a.spO2 << "%\n";
        }
    }
    ss << "================================================================================\n";
    return ss.str();
}

std::string AmbulanceVisualizer::ToJson(const AmbulanceEntity& amb) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << amb.ambulanceId << "\",\"type\":\"" << amb.vehicleType
       << "\",\"status\":\"" << amb.status << "\",\"speed\":" << std::fixed << std::setprecision(1) << amb.speedKmh
       << ",\"hr\":" << amb.heartRate << ",\"bp\":\"" << amb.systolicBp << "/" << amb.diastolicBp
       << "\",\"spo2\":" << amb.spO2 << "}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

