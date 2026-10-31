/**
 * @file intersection_main.cpp
 * @brief Roadside Intersection Node TCP Server Daemon (Raspberry Pis 2, 3, 4 / Simulation).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

#include "intersection_service/IntersectionService.hpp"
#include "aegis/config/IntersectionsConfig.hpp"
#include "gpio/GpioFactory.hpp"

static std::atomic<bool> g_shutdownRequested{false};

static void SignalHandler(int signum) {
    (void)signum;
    g_shutdownRequested.store(true);
}

int main(int argc, char* argv[]) {
    int intersectionId = 1;
    uint16_t port = 0;
    bool forceSimulation = false;
    bool forceHardware = false;
    std::string configPath = "config/intersections.json";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--id" || arg == "-i") && i + 1 < argc) {
            intersectionId = std::stoi(argv[++i]);
        } else if ((arg == "--port" || arg == "-p") && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--simulation" || arg == "-s") {
            forceSimulation = true;
        } else if (arg == "--hardware" || arg == "-hw") {
            forceHardware = true;
        }
    }

    // Try loading cluster config to get default port if not specified on CLI
    auto clusterConfig = aegis::config::IntersectionsConfig::LoadFromFile(configPath);
    if (port == 0) {
        if (clusterConfig) {
            auto nodeCfg = clusterConfig->GetIntersectionById(intersectionId);
            if (nodeCfg) {
                port = nodeCfg->port;
            }
        }
        if (port == 0) {
            port = static_cast<uint16_t>(5000 + intersectionId);
        }
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Roadside Intersection Node (TCP Server Daemon)" << std::endl;
    std::cout << " Intersection ID : " << intersectionId << std::endl;
    std::cout << " Listening Port  : " << port << std::endl;
    std::cout << " Mode            : " << (forceSimulation ? "SIMULATION" : (forceHardware ? "QNX HARDWARE" : "AUTO-DETECT")) << std::endl;
    if (intersectionId == 3) {
        std::cout << " Hardware Profile: Node 3 (Blue LED only, Physical Pin 31 - Hardware Shortage)" << std::endl;
    } else {
        std::cout << " Hardware Profile: Node " << intersectionId << " (Red Pin 29, Blue Pin 31)" << std::endl;
    }
    std::cout << "==========================================================" << std::endl;

    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    bool simFlag = forceSimulation || (!forceHardware &&
#if defined(__QNX__) || defined(__QNXNTO__)
        false
#else
        true
#endif
    );

    auto gpio = aegis::gpio::GpioFactory::Create(intersectionId, simFlag);
    aegis::intersection::IntersectionService service(intersectionId, port, gpio);

    service.SetCommandCallback([intersectionId](const aegis::protocol::AegisMessage& msg) {
        std::cout << "[INT " << intersectionId << " COMMAND] Received: " << msg.command
                  << " (seq: " << msg.sequence << ")" << std::endl;
    });

    if (!service.Start()) {
        std::cerr << "[FATAL] Failed to start Intersection " << intersectionId << " on port " << port << std::endl;
        return 1;
    }

    std::cout << "[READY] Intersection Node " << intersectionId << " ready and awaiting Coordinator connection..." << std::endl;

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\n[SHUTDOWN] Stopping Intersection Node " << intersectionId << "..." << std::endl;
    service.Stop();
    std::cout << "[SHUTDOWN] Intersection Node terminated gracefully." << std::endl;
    return 0;
}
