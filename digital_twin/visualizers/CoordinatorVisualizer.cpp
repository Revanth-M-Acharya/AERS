/**
 * @file CoordinatorVisualizer.cpp
 * @brief Implementation of CoordinatorVisualizer.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/visualizers/CoordinatorVisualizer.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::digital_twin::visualizers {

std::string CoordinatorVisualizer::RenderAsciiCoordinatorStatus(const CoordinatorOverview& coord) {
    std::ostringstream ss;
    ss << "+==============================================================================+\n"
       << "| SMART AEGIS MASTER COORDINATOR RUNTIME STATUS                                |\n"
       << "+==============================================================================+\n"
       << "| Node ID    : " << std::left << std::setw(28) << coord.coordinatorId
       << " Status : " << std::setw(23) << coord.serverState << "|\n"
       << "| Platform   : " << std::setw(28) << coord.platform
       << " FSM    : " << std::setw(23) << coord.fsmState << "|\n"
       << "| OS Kernel  : " << std::setw(28) << coord.os
       << " Uptime : " << std::setw(20) << (std::to_string(coord.uptimeSeconds) + "s") << "   |\n"
       << "| TCP Socket : " << std::setw(28) << (coord.ipAddress + ":" + std::to_string(coord.listenPort))
       << " Nodes  : " << std::setw(20) << (std::to_string(coord.connectedTrafficNodesCount) + " Connected") << "   |\n"
       << "| Corridors  : " << std::setw(28) << (std::to_string(coord.activeCorridorsCount) + " Active")
       << " Queue  : " << std::setw(20) << (std::to_string(coord.pendingDispatchesCount) + " Pending") << "   |\n"
       << "+==============================================================================+\n";
    return ss.str();
}

std::string CoordinatorVisualizer::ToJson(const CoordinatorOverview& coord) {
    std::ostringstream ss;
    ss << "{\"id\":\"" << coord.coordinatorId
       << "\",\"server\":\"" << coord.serverState
       << "\",\"fsm\":\"" << coord.fsmState
       << "\",\"nodes\":" << coord.connectedTrafficNodesCount
       << ",\"corridors\":" << coord.activeCorridorsCount
       << ",\"uptime\":" << coord.uptimeSeconds << "}";
    return ss.str();
}

} // namespace aegis::digital_twin::visualizers

