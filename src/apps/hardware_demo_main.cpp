/**
 * @file hardware_demo_main.cpp
 * @brief SMART AEGIS Live Hardware Demonstration Runner (Phase 12).
 * Sequences and executes the 5 hackathon demonstration missions across the 4 Raspberry Pis.
 * Target Platform: QNX Neutrino RTOS 8.0 / POSIX Simulation
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/HardwareDemoOrchestrator.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    int selectedMission = 0; // 0 = Full Suite
    uint32_t stepDelayMs = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--mission" || arg == "-m") && i + 1 < argc) {
            selectedMission = std::stoi(argv[++i]);
        } else if ((arg == "--delay" || arg == "-d") && i + 1 < argc) {
            stepDelayMs = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if (arg == "--live") {
            stepDelayMs = 1200; // Realistic delay for live audience presentation
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: hardware_demo [OPTIONS]\n"
                      << "  --mission, -m <1-5>  Execute specific demo mission (default: all)\n"
                      << "                       1: Normal Green Corridor\n"
                      << "                       2: Dual-Ambulance Priority Arbitration\n"
                      << "                       3: Police-Assisted Dynamic Replanning\n"
                      << "                       4: Roadside Fault Injection & Fail-Safe\n"
                      << "                       5: Telemetry & Mission Replay\n"
                      << "  --delay, -d <ms>     Delay between mission steps (default: 0)\n"
                      << "  --live               Enable live presentation mode (1200ms step delay)\n"
                      << "  --help, -h           Display this help text\n";
            return 0;
        }
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Hackathon Hardware Demonstration Runner      " << std::endl;
    std::cout << " Target: Raspberry Pi 4 (QNX SDP 8.0 aarch64le / Host)    " << std::endl;
    std::cout << " Step Delay: " << stepDelayMs << " ms                     " << std::endl;
    std::cout << "==========================================================" << std::endl;

    auto orchestrator = std::make_unique<aegis::hardware_integration::demo::HardwareDemoOrchestrator>();
    if (!orchestrator->Initialize()) {
        std::cerr << "[ERROR] HardwareDemoOrchestrator initialization failed!" << std::endl;
        return 1;
    }

    orchestrator->SetStepDelayMs(stepDelayMs);

    bool overallSuccess = true;

    if (selectedMission == 0) {
        std::cout << "[INFO] Executing complete 5-Mission Demonstration Suite...\n\n";
        auto results = orchestrator->ExecuteFullDemoSuite();
        std::cout << "\n================ DEMONSTRATION SUMMARY ================\n";
        for (const auto& r : results) {
            std::cout << " [" << (r.success ? "PASS" : "FAIL") << "] "
                      << r.missionName << " (" << r.durationMs << "ms): "
                      << r.summary << "\n";
            if (!r.success) overallSuccess = false;
        }
        std::cout << "========================================================\n";
    } else {
        aegis::hardware_integration::demo::DemoExecutionMetrics m;
        switch (selectedMission) {
            case 1:
                std::cout << "[INFO] Executing Mission 1: Normal Green Corridor...\n";
                m = orchestrator->ExecuteMission1_NormalCorridor();
                break;
            case 2:
                std::cout << "[INFO] Executing Mission 2: Priority Conflict Arbitration...\n";
                m = orchestrator->ExecuteMission2_PriorityScheduling();
                break;
            case 3:
                std::cout << "[INFO] Executing Mission 3: Police Dynamic Replanning...\n";
                m = orchestrator->ExecuteMission3_PoliceReplanning();
                break;
            case 4:
                std::cout << "[INFO] Executing Mission 4: Roadside Fault Injection...\n";
                m = orchestrator->ExecuteMission4_FaultInjection();
                break;
            case 5:
                std::cout << "[INFO] Executing Mission 5: Mission Replay...\n";
                m = orchestrator->ExecuteMission5_MissionReplay();
                break;
            default:
                std::cerr << "[ERROR] Invalid mission index: " << selectedMission << " (choose 1-5)\n";
                return 1;
        }
        std::cout << "\nResult: [" << (m.success ? "PASS" : "FAIL") << "] "
                  << m.missionName << " - " << m.summary << "\n";
        overallSuccess = m.success;
    }

    if (overallSuccess) {
        std::cout << "\n>>> ALL DEMONSTRATION MISSIONS COMPLETED WITH 100% SUCCESS <<<\n\n";
        return 0;
    } else {
        std::cerr << "\n>>> DEMONSTRATION COMPLETED WITH ERRORS <<<\n\n";
        return 1;
    }
}
