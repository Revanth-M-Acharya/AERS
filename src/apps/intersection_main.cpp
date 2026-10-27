/**
 * @file intersection_main.cpp
 * @brief Roadside Intersection Node daemon entry point (Raspberry Pis 2, 3, 4).
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

#include "intersection/IntersectionNode.h"
#include "config/NodeConfig.h"

static std::atomic<bool> g_shutdownRequested{false};

static void SignalHandler(int signum) {
    (void)signum;
    g_shutdownRequested.store(true);
}

int main(int argc, char* argv[]) {
    std::string configPath = "config/deployment/rpi2_intersection_a.json";
    for (int i = 1; i < argc; ++i) {
        if ((std::string(argv[i]) == "--config" || std::string(argv[i]) == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        }
    }

    aegis::config::NodeConfig config;
    if (!aegis::config::NodeConfigLoader::LoadFromFile(configPath, config)) {
        std::cerr << "[ERROR] Failed to load intersection node configuration from: " << configPath << std::endl;
        return 1;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Roadside Intersection Node" << std::endl;
    std::cout << " Node ID         : " << config.nodeId << std::endl;
    std::cout << " Role            : " << aegis::config::NodeRoleToString(config.role) << std::endl;
    std::cout << " Coordinator IP  : " << config.coordinatorIp << std::endl;
    std::cout << " Coordinator Port: " << config.coordinatorPort << std::endl;
    std::cout << " Heartbeat Rate  : " << config.heartbeatIntervalMs << " ms" << std::endl;
    std::cout << " Auto Reconnect  : " << (config.autoReconnect ? "Enabled" : "Disabled") << std::endl;
    std::cout << "==========================================================" << std::endl;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    aegis::intersection::IntersectionNode node(config.nodeId, config.heartbeatIntervalMs);

    node.SetCommandCallback([&node](const std::string& cmd) {
        std::cout << "[COMMAND] Executed: " << cmd
                  << " | Current Signal State: " << node.GetSignalController().StateString()
                  << " | Emergency: " << (node.GetSignalController().IsEmergency() ? "YES" : "NO")
                  << std::endl;
    });

    std::cout << "[CONNECTING] Connecting to Central Coordinator at "
              << config.coordinatorIp << ":" << config.coordinatorPort << "..." << std::endl;

    if (!node.Start(config.coordinatorIp, config.coordinatorPort, config.connectTimeoutMs, config.ackTimeoutMs)) {
        std::cerr << "[WARN] Initial handshake failed. Background reconnection active..." << std::endl;
    } else {
        std::cout << "[READY] Registered with Coordinator! Heartbeat beaconing active." << std::endl;
    }

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n[SHUTDOWN] Terminating Intersection Node " << config.nodeId << " gracefully..." << std::endl;
    node.Stop();
    std::cout << "[SHUTDOWN] Intersection Node " << config.nodeId << " stopped." << std::endl;

    return 0;
}
