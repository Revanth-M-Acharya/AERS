#include "simulation/SimulationRunner.hpp"
#include "coordinator_client/CoordinatorClientManager.hpp"
#include "intersection_service/IntersectionService.hpp"
#include "aegis/config/IntersectionsConfig.hpp"
#include "gpio/SimulationGpioController.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

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
    const std::string& timestamp) {

    std::cout << "\033[2J\033[H";
    std::cout << "==================================================\n";
    std::cout << "             SMART AEGIS SIMULATION               \n";
    std::cout << "==================================================\n";
    std::cout << "Coordinator:         " << coordinatorStatus << "\n\n";

    std::cout << "Intersection 1:      " << int1Status << "\n";
    std::cout << "  Blue:              " << (int1Blue ? "\033[1;34mON [GREEN CORRIDOR]\033[0m" : "OFF") << "\n";
    std::cout << "  Red:               " << (int1Red  ? "\033[1;31mON [HOLD]\033[0m"           : "OFF") << "\n\n";

    std::cout << "Intersection 2:      " << int2Status << "\n";
    std::cout << "  Blue:              " << (int2Blue ? "\033[1;34mON [GREEN CORRIDOR]\033[0m" : "OFF") << "\n";
    std::cout << "  Red:               " << (int2Red  ? "\033[1;31mON [HOLD]\033[0m"           : "OFF") << "\n\n";

    std::cout << "Intersection 3:      " << int3Status << "\n";
    std::cout << "  Blue:              " << (int3Blue ? "\033[1;34mON [GREEN CORRIDOR]\033[0m" : "OFF") << "\n";
    std::cout << "  Red:               N/A (Hardware Shortage)\n\n";

    std::cout << "Emergency Status:    " << emergencyStatus << "\n";
    std::cout << "Hospital Status:     " << hospitalStatus << "\n";
    std::cout << "Police Status:       " << policeStatus << "\n";
    std::cout << "ETA:                 " << eta << "\n";
    std::cout << "Network Status:      " << networkStatus << "\n";
    std::cout << "Current Route:       " << currentRoute << "\n";
    std::cout << "Current Command:     " << currentCommand << "\n";
    std::cout << "Timestamp:           " << timestamp << "\n";
    std::cout << "==================================================\n" << std::flush;
}

void RunSimulationMode(const std::string& configPath) {
    std::cout << "[INFO] Initializing SMART AEGIS in-process simulation...\n";

    auto gpio1 = std::make_shared<aegis::gpio::SimulationGpioController>(1);
    auto gpio2 = std::make_shared<aegis::gpio::SimulationGpioController>(2);
    auto gpio3 = std::make_shared<aegis::gpio::SimulationGpioController>(3);

    aegis::intersection::IntersectionService node1(1, 15001, gpio1);
    aegis::intersection::IntersectionService node2(2, 15002, gpio2);
    aegis::intersection::IntersectionService node3(3, 15003, gpio3);

    if (!node1.Start() || !node2.Start() || !node3.Start()) {
        std::cerr << "[ERROR] Failed to start virtual intersection servers.\n";
        return;
    }

    auto config = aegis::config::IntersectionsConfig::LoadFromFile(configPath);
    if (!config || !config->IsValid()) {
        config = aegis::config::IntersectionsConfig::CreateDefaultCluster();
    }

    aegis::coordinator::CoordinatorClientManager coordinator(config);
    coordinator.SetNodeEndpoint(1, "127.0.0.1", 15001);
    coordinator.SetNodeEndpoint(2, "127.0.0.1", 15002);
    coordinator.SetNodeEndpoint(3, "127.0.0.1", 15003);
    coordinator.SetReconnectIntervalMs(300);
    coordinator.SetPingIntervalMs(4000);

    coordinator.Start();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Simulation Demonstration Sequence
    coordinator.ResetAll();
    RenderDashboard("ONLINE", "ONLINE", false, false, "ONLINE", false, false, "ONLINE", false,
                    "NORMAL / STANDBY", "GREEN (Trauma Center Ready)", "PATROL ACTIVE",
                    "--", "ALL NODES CONNECTED (RTT < 1ms)",
                    "HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL",
                    "RESET (ALL NODES)", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    RenderDashboard("ONLINE", "ONLINE", false, false, "ONLINE", false, false, "ONLINE", false,
                    "PRIORITY 1 - CARDIAC INTAKE", "BED RESERVED (Bay 4 Alerted)", "CORRIDOR CLEARED",
                    "34.2s", "HEALTHY (RTT < 1ms)",
                    "HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL",
                    "DISPATCH AMBULANCE AMB_01", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    coordinator.SendCommandWithAck(1, std::string(aegis::protocol::Commands::RED_OFF));
    coordinator.SendCommandWithAck(1, std::string(aegis::protocol::Commands::BLUE_ON));
    RenderDashboard("ONLINE", "ONLINE", true, false, "ONLINE", false, false, "ONLINE", false,
                    "AMBULANCE APPROACHING INTERSECTION 1", "PREPPING TRAUMA TEAM", "CROSS-TRAFFIC HALTED",
                    "26.0s", "HEALTHY (RTT < 1ms)",
                    "HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL",
                    "BLUE_ON (INT_1)", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    coordinator.SendCommandWithAck(1, std::string(aegis::protocol::Commands::BLUE_OFF));
    coordinator.SendCommandWithAck(2, std::string(aegis::protocol::Commands::RED_OFF));
    coordinator.SendCommandWithAck(2, std::string(aegis::protocol::Commands::BLUE_ON));
    RenderDashboard("ONLINE", "ONLINE", false, false, "ONLINE", true, false, "ONLINE", false,
                    "AMBULANCE APPROACHING INTERSECTION 2", "SURGICAL SUITE READY", "CROSS-TRAFFIC HALTED",
                    "15.5s", "HEALTHY (RTT < 1ms)",
                    "HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL",
                    "BLUE_ON (INT_2)", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    coordinator.SendCommandWithAck(2, std::string(aegis::protocol::Commands::BLUE_OFF));
    coordinator.SendCommandWithAck(3, std::string(aegis::protocol::Commands::BLUE_ON));
    RenderDashboard("ONLINE", "ONLINE", false, false, "ONLINE", false, false, "ONLINE", true,
                    "AMBULANCE APPROACHING INTERSECTION 3", "RECEIVING BAY OPEN", "CROSS-TRAFFIC HALTED",
                    "5.0s", "HEALTHY (RTT < 1ms)",
                    "HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL",
                    "BLUE_ON (INT_3)", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    coordinator.SendCommandWithAck(3, std::string(aegis::protocol::Commands::BLUE_OFF));
    coordinator.ResetAll();
    RenderDashboard("ONLINE", "ONLINE", false, false, "ONLINE", false, false, "ONLINE", false,
                    "MISSION COMPLETED - PATIENT HANDOVER", "PATIENT ADMITTED", "NORMAL PATROL RESUMED",
                    "0.0s (ARRIVED)", "HEALTHY",
                    "MISSION CONCLUDED",
                    "RESET (ALL NODES)", aegis::protocol::AegisMessage::CurrentIsoTimestamp());
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "\n[INFO] Simulation sequence completed successfully.\n";

    coordinator.Stop();
    node1.Stop();
    node2.Stop();
    node3.Stop();
}

} // namespace aegis::simulation
