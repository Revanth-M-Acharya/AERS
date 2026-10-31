#pragma once

#include <string>

namespace aegis::simulation {

void RenderDashboard(
    const std::string& coordinatorStatus,
    const std::string& int1Status, bool int1Blue, bool int1Red,
    const std::string& int2Status, bool int2Blue, bool int2Red,
    const std::string& int3Status, bool int3Blue,
    const std::string& emergencyStatus,
    const std::string& hospitalStatus,
    const std::string& policeStatus,
    const std::string& eta,
    const std::string& networkStatus,
    const std::string& currentRoute,
    const std::string& currentCommand,
    const std::string& timestamp);

void RunSimulationMode(const std::string& configPath = "config/intersections.json");

} // namespace aegis::simulation
