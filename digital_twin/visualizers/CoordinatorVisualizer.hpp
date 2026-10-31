/**
 * @file CoordinatorVisualizer.hpp
 * @brief Visualizer for Raspberry Pi 4 Coordinator operational state, server links, and FSM runtime.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace aegis::digital_twin::visualizers {

struct CoordinatorOverview {
    std::string coordinatorId{"COORDINATOR_RPI4_01"};
    std::string platform{"Raspberry Pi 4 Model B (4 GB)"};
    std::string os{"QNX Neutrino RTOS 8.0 (aarch64le)"};
    std::string ipAddress{"127.0.0.1"};
    uint16_t listenPort{19123};
    std::string serverState{"LISTENING"};
    std::string fsmState{"ACTIVE_ORCHESTRATING"};
    uint32_t connectedTrafficNodesCount{3};
    uint32_t activeCorridorsCount{1};
    uint32_t pendingDispatchesCount{0};
    uint64_t uptimeSeconds{3600};
};

class CoordinatorVisualizer {
public:
    static std::string RenderAsciiCoordinatorStatus(const CoordinatorOverview& coord);
    static std::string ToJson(const CoordinatorOverview& coord);
};

} // namespace aegis::digital_twin::visualizers

