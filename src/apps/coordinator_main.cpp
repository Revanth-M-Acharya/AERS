/**
 * @file coordinator_main.cpp
 * @brief Central AERS Coordinator daemon entry point (Raspberry Pi 1 / QNX RTOS).
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#include "coordinator/CoordinatorManager.h"
#include "config/NodeConfig.h"

static std::atomic<bool> g_shutdownRequested{false};

static void SignalHandler(int signum) {
    (void)signum;
    g_shutdownRequested.store(true);
}

int main(int argc, char* argv[]) {
    std::string configPath = "config/deployment/rpi1_coordinator.json";
    for (int i = 1; i < argc; ++i) {
        if ((std::string(argv[i]) == "--config" || std::string(argv[i]) == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    aegis::config::NodeConfig config;
    if (!aegis::config::NodeConfigLoader::LoadFromFile(configPath, config)) {
        std::cerr << "[ERROR] Failed to load coordinator configuration from: " << configPath << std::endl;
        return 1;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Central Coordinator (Raspberry Pi 1)" << std::endl;
    std::cout << " Node ID         : " << config.nodeId << std::endl;
    std::cout << " Role            : " << aegis::config::NodeRoleToString(config.role) << std::endl;
    std::cout << " Listening Host  : " << config.ipAddress << std::endl;
    std::cout << " Listening Port  : " << config.port << std::endl;
    std::cout << " Heartbeat Rate  : " << config.heartbeatIntervalMs << " ms" << std::endl;
    std::cout << "==========================================================" << std::endl;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    aegis::coordinator::CoordinatorManager coordinator;
    if (!coordinator.Start(config.port, config.ipAddress)) {
        std::cerr << "[FATAL] Failed to start Coordinator server on "
                  << config.ipAddress << ":" << config.port << std::endl;
        return 1;
    }

    std::cout << "[READY] Central Coordinator listening for Intersection Nodes..." << std::endl;

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n[SHUTDOWN] Terminating Central Coordinator gracefully..." << std::endl;
    coordinator.Stop();
    std::cout << "[SHUTDOWN] Central Coordinator stopped." << std::endl;

    return 0;
}
