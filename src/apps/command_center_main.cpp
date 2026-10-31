/**
 * @file command_center_main.cpp
 * @brief SMART AEGIS Municipal Operations & Command Center Console.
 * Target Platform: QNX Neutrino RTOS 8.0 / POSIX Simulation
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/CommandCenterFacade.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};

static void SignalHandler(int signum) {
    (void)signum;
    g_running.store(false);
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    bool continuous = false;
    uint32_t intervalSec = 2;
    std::string replayMissionId = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--continuous" || arg == "-c") {
            continuous = true;
        } else if ((arg == "--interval" || arg == "-i") && i + 1 < argc) {
            intervalSec = static_cast<uint32_t>(std::stoul(argv[++i]));
            continuous = true;
        } else if ((arg == "--replay" || arg == "-r") && i + 1 < argc) {
            replayMissionId = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: command_center [OPTIONS]\n"
                      << "  --continuous, -c         Run continuous dashboard refresh\n"
                      << "  --interval, -i <sec>     Refresh interval in seconds (default: 2)\n"
                      << "  --replay, -r <id>        Replay historical mission\n"
                      << "  --help, -h               Display this help text\n";
            return 0;
        }
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Operations & Command Center Console         " << std::endl;
    std::cout << " Target: QNX SDP 8.0 / Linux / macOS                     " << std::endl;
    std::cout << "==========================================================" << std::endl;

    auto cmdCenter = std::make_unique<aegis::command_center::CommandCenterFacade>();
    cmdCenter->Initialize();
    cmdCenter->Start();

    if (!replayMissionId.empty()) {
        std::cout << "[INFO] Loading mission " << replayMissionId << " for replay..." << std::endl;
        cmdCenter->LoadMissionForReplay(replayMissionId);
    }

    if (!continuous) {
        // Single snapshot render
        cmdCenter->PrintDashboard(std::cout);
    } else {
        std::cout << "[INFO] Starting continuous monitor. Press Ctrl+C to exit.\n";
        while (g_running.load()) {
            // ANSI clear screen and home
            std::cout << "\033[2J\033[1;1H";
            cmdCenter->PrintDashboard(std::cout);
            std::this_thread::sleep_for(std::chrono::seconds(intervalSec));
        }
    }

    cmdCenter->Stop();
    std::cout << "\n[INFO] Operations Command Center terminated gracefully.\n";
    return 0;
}
