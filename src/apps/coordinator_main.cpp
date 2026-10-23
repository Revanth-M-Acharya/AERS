#include <iostream>
#include "aegis/core/coordinator.hpp"
#include "aegis/logger/logger.hpp"

int main() {
    aegis::logger::Logger::Instance().Log(
        aegis::logger::LogLevel::INFO, "MAIN", "Starting SMART AEGIS Coordinator Daemon...");
    
    aegis::core::coordinator::CentralCoordinator coordinator;
    if (coordinator.Start()) {
        std::cout << "[SUCCESS] SMART AEGIS Coordinator started successfully (Phase 1 Build Skeleton)." << std::endl;
        coordinator.Stop();
        return 0;
    }
    return 1;
}
