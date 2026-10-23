#include <iostream>
#include "aegis/edge/ambulance_node.hpp"
#include "aegis/logger/logger.hpp"

int main() {
    aegis::logger::Logger::Instance().Log(
        aegis::logger::LogLevel::INFO, "MAIN", "Starting SMART AEGIS Ambulance OBU Node...");
    
    aegis::edge::ambulance::AmbulanceNode ambulance;
    if (ambulance.Initialize("AMB-01", "config/default.json") && ambulance.Start()) {
        std::cout << "[SUCCESS] SMART AEGIS Ambulance Node started successfully (Phase 1 Build Skeleton)." << std::endl;
        ambulance.Stop();
        return 0;
    }
    return 1;
}
