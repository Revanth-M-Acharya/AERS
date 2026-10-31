/**
 * @file digital_twin_main.cpp
 * @brief SMART AEGIS City-Scale Digital Twin Operations Center Executable.
 * Target Platform: QNX Neutrino RTOS 8.0 / POSIX Simulation
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/DigitalTwinEngine.hpp"
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
    bool exportJson = false;
    bool exportMd = false;
    uint32_t stepCount = 10;
    double timeStepSec = 0.5;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--continuous" || arg == "-c") {
            continuous = true;
        } else if (arg == "--json") {
            exportJson = true;
        } else if (arg == "--markdown" || arg == "-md") {
            exportMd = true;
        } else if ((arg == "--steps" || arg == "-s") && i + 1 < argc) {
            stepCount = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if ((arg == "--dt" || arg == "-t") && i + 1 < argc) {
            timeStepSec = std::stod(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: digital_twin [OPTIONS]\n"
                      << "  --continuous, -c     Run continuous simulation loop\n"
                      << "  --steps, -s <N>      Run N simulation steps and report (default: 10)\n"
                      << "  --dt, -t <sec>       Simulation delta time in seconds (default: 0.5)\n"
                      << "  --json               Output JSON telemetry report\n"
                      << "  --markdown, -md      Output Markdown summary report\n"
                      << "  --help, -h           Display this help text\n";
            return 0;
        }
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: City-Scale Digital Twin Operations Center   " << std::endl;
    std::cout << " Target: QNX Neutrino RTOS 8.0 / Linux / macOS            " << std::endl;
    std::cout << "==========================================================" << std::endl;

    auto dtEngine = std::make_unique<aegis::digital_twin::DigitalTwinEngine>();
    dtEngine->Initialize();
    dtEngine->Start();

    if (continuous) {
        std::cout << "[INFO] Digital Twin running continuous simulation. Press Ctrl+C to stop.\n";
        while (g_running.load()) {
            dtEngine->Step(timeStepSec);
            dtEngine->Update();
            std::cout << "\033[2J\033[1;1H";
            std::cout << dtEngine->ExportAsciiDashboard();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    } else {
        std::cout << "[INFO] Stepping simulation for " << stepCount << " cycles (dt=" << timeStepSec << "s)...\n";
        for (uint32_t i = 0; i < stepCount; ++i) {
            dtEngine->Step(timeStepSec);
            dtEngine->Update();
        }

        if (exportJson) {
            std::cout << dtEngine->ExportJsonReport() << std::endl;
        } else if (exportMd) {
            std::cout << dtEngine->ExportMarkdownReport() << std::endl;
        } else {
            std::cout << dtEngine->ExportAsciiDashboard() << std::endl;
        }
    }

    dtEngine->Stop();
    std::cout << "[INFO] Digital Twin Operations Center stopped safely.\n";
    return 0;
}
