#include "simulation/SimulationRunner.hpp"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string configPath = "config/intersections.json";
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    std::cout << "[INFO] Starting SMART AEGIS Desktop Simulator...\n";
    aegis::simulation::RunSimulationMode(configPath);
    return 0;
}
