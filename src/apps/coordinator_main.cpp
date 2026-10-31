#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>

#include "coordinator_client/CoordinatorClientManager.hpp"
#include "aegis/config/IntersectionsConfig.hpp"
#include "simulation/SimulationRunner.hpp"

static std::atomic<bool> g_shutdownRequested{false};

static void SignalHandler(int signum) {
    (void)signum;
    g_shutdownRequested.store(true);
}

int main(int argc, char* argv[]) {
#if defined(SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
#endif
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    std::string configPath = "config/intersections.json";
    bool runSimulation = false;
    bool runDemo = false;
    bool forceHardware = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--simulate" || arg == "--simulation" || arg == "-s") {
            runSimulation = true;
        } else if (arg == "--hardware" || arg == "-hw") {
            forceHardware = true;
        } else if (arg == "--demo" || arg == "-d") {
            runDemo = true;
        }
    }

    if (runSimulation) {
        aegis::simulation::RunSimulationMode(configPath);
        return 0;
    }

    auto config = aegis::config::IntersectionsConfig::LoadFromFile(configPath);
    if (!config || !config->IsValid()) {
        std::cout << "[WARN] " << configPath << " not found or invalid; falling back to default cluster config.\n";
        config = aegis::config::IntersectionsConfig::CreateDefaultCluster();
    }

    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Central Coordinator (Raspberry Pi 1 / Client)\n";
    std::cout << " Role            : Master Controller (Pure Software - No LEDs)\n";
    std::cout << " Host IP         : " << config->GetSystemSettings().coordinator.ip << "\n";
    std::cout << " Target Nodes    : " << config->GetIntersections().size() << " Intersections\n";
    std::cout << " Reconnect Retry : " << config->GetSystemSettings().reconnectIntervalMs << " ms\n";
    std::cout << " Health Ping     : " << config->GetSystemSettings().pingIntervalMs << " ms\n";
    std::cout << " Mode            : " << (forceHardware ? "QNX PHYSICAL HARDWARE" : "STANDARD NETWORK") << "\n";
    std::cout << "==========================================================\n";

    aegis::coordinator::CoordinatorClientManager coordinator(config);

    if (!coordinator.Start()) {
        std::cerr << "[ERROR] Failed to initialize Coordinator Client Manager.\n";
        return 1;
    }

    std::cout << "[INFO] Central Coordinator connecting to intersection cluster...\n";

    if (runDemo) {
        std::cout << "\n>>> EXECUTING HARDWARE CORRIDOR DEMONSTRATION <<<\n\n";

        std::cout << "[DEMO] Stage 1: Resetting all intersections to baseline.\n";
        coordinator.ResetAll();
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "[DEMO] Stage 2: Emergency Alert -> Preempting Green Corridor (Nodes 1, 2, 3).\n";
        coordinator.GrantGreenCorridor({1, 2, 3});
        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::cout << "[DEMO] Stage 3: Ambulance traversed Intersection 1 -> Releasing Node 1.\n";
        coordinator.ReleaseGreenCorridor({1});
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "[DEMO] Stage 4: Ambulance traversed Intersection 2 -> Releasing Node 2.\n";
        coordinator.ReleaseGreenCorridor({2});
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "[DEMO] Stage 5: Ambulance arrived at hospital -> Releasing Node 3.\n";
        coordinator.ReleaseGreenCorridor({3});
        coordinator.ResetAll();
        std::cout << "\n>>> HARDWARE DEMONSTRATION COMPLETE <<<\n\n";

        coordinator.Stop();
        return 0;
    }

    std::cout << "[INFO] Coordinator active. Press Ctrl+C to terminate.\n";
    auto lastStatusPrint = std::chrono::steady_clock::now();

    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastStatusPrint).count() >= 5) {
            lastStatusPrint = now;
            auto statuses = coordinator.GetAllNodeStatuses();
            std::cout << "\n[HEALTH MONITOR - " << aegis::protocol::AegisMessage::CurrentIsoTimestamp() << "]\n";
            for (const auto& [nodeId, status] : statuses) {
                std::cout << "  Intersection " << nodeId << ": ["
                          << aegis::coordinator::NodeHealthStatusToString(status) << "]\n";
            }
        }
    }

    std::cout << "\n[INFO] Terminating Central Coordinator gracefully...\n";
    coordinator.Stop();
    std::cout << "[INFO] Central Coordinator stopped.\n";

    return 0;
}
