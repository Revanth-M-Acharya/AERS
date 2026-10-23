#include <iostream>
#include "aegis/edge/traffic_controller.hpp"
#include "aegis/logger/logger.hpp"

int main() {
    aegis::logger::Logger::Instance().Log(
        aegis::logger::LogLevel::INFO, "MAIN", "Starting SMART AEGIS Traffic RSU Controller...");
    
    aegis::edge::traffic::TrafficController trafficController;
    if (trafficController.Initialize("INT-101", 3000) && trafficController.Start()) {
        std::cout << "[SUCCESS] SMART AEGIS Traffic Controller started successfully (Phase 1 Build Skeleton)." << std::endl;
        trafficController.Stop();
        return 0;
    }
    return 1;
}
