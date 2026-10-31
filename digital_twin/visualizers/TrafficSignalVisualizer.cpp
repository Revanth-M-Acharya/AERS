/**
 * @file TrafficSignalVisualizer.cpp
 * @brief Implementation of TrafficSignalVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/TrafficSignalVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string TrafficSignalVisualizer::RenderAsciiIntersection(const IntersectionNode& node) {
    std::ostringstream ss;
    ss << "+--------------------------------------------------------+\n"
       << "| INTERSECTION: " << std::left << std::setw(14) << node.intersectionId
       << " [" << std::setw(24) << node.name << "] |\n"
       << "+--------------------------------------------------------+\n"
       << "| Signal State: [" << std::setw(8) << node.signalState << "] "
       << "Preemption: " << std::setw(15) << node.preemptionState << "|\n"
       << "| Hardware    : " << std::setw(18) << node.hardwareType
       << " ESP32 HW  : " << (node.esp32ControllerConnected ? "ONLINE " : "OFFLINE") << "    |\n"
       << "| Network IP  : " << std::setw(18) << (node.ipAddress + ":" + std::to_string(node.port))
       << " Connected : " << (node.isConnected ? "YES    " : "NO     ") << "    |\n"
       << "| Relays [CH1-CH4]: "
       << (node.relayStates[0] ? "ON " : "OFF ")
       << (node.relayStates[1] ? "ON " : "OFF ")
       << (node.relayStates[2] ? "ON " : "OFF ")
       << (node.relayStates[3] ? "ON " : "OFF ") << "                       |\n"
       << "+--------------------------------------------------------+\n";
    return ss.str();
}

std::string TrafficSignalVisualizer::RenderIntersectionsSummary(const std::vector<IntersectionNode>& nodes) {
    std::ostringstream ss;
    ss << "=== TRAFFIC SIGNAL NETWORK =====================================================\n";
    if (nodes.empty()) {
        ss << "  (No traffic nodes connected)\n";
    } else {
        for (const auto& n : nodes) {
            ss << " * " << n.intersectionId << " [" << n.name << "]"
               << " | Signal: [" << n.signalState << "]"
               << " | Preempt: " << n.preemptionState
               << " | Corridor: " << (n.activeCorridorId.empty() ? "NONE" : n.activeCorridorId)
               << " | ESP32: " << (n.esp32ControllerConnected ? "OK" : "DISCONNECTED")
               << " | Status: " << (n.isConnected ? "ONLINE" : "OFFLINE") << "\n";
        }
    }
    ss << "================================================================================\n";
    return ss.str();
}

std::string TrafficSignalVisualizer::ToJson(const IntersectionNode& node) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << node.intersectionId << "\",\"signal\":\"" << node.signalState
       << "\",\"preemption\":\"" << node.preemptionState << "\",\"esp32\":"
       << (node.esp32ControllerConnected ? "true" : "false") << "}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

