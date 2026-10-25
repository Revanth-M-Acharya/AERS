#include <iostream>
#include "aegis/edge/ambulance_node.hpp"
#include "aegis/logger/logger.hpp"

int main() {
    aegis::logger::Logger::Instance().Log(
        aegis::logger::LogLevel::INFO, "MAIN", "Starting SMART AEGIS Ambulance OBU Node...");
    
    aegis::edge::ambulance::AmbulanceNode ambulance;
    if (ambulance.Initialize("AMB-01", "config/default.json") && ambulance.Start()) {
        std::cout << "[SUCCESS] SMART AEGIS Ambulance Node started successfully." << std::endl;
        
        // Execute initial telemetry cycle
        ambulance.PublishKinematicsOnce();
        ambulance.PublishVitalsOnce();
        ambulance.PublishHeartbeatOnce();
        ambulance.PublishStateOnce();

        std::cout << "[OBU] Asset ID: " << ambulance.GetUnitId()
                  << " | State: " << ambulance.GetStateString()
                  << " | Telemetry: Operational" << std::endl;

        ambulance.Stop();
        return 0;
    }
    std::cerr << "[ERROR] Failed to initialize SMART AEGIS Ambulance Node." << std::endl;
    return 1;
}
