/**
 * @file driver_dashboard_main.cpp
 * @brief SMART AEGIS In-Vehicle Ambulance Driver Dashboard & Tactical Telematics Console.
 * Target Platform: QNX Neutrino RTOS 8.0 (Raspberry Pi 4) / POSIX Simulation
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "aegis/edge/ambulance_node.hpp"
#include "common/utils/TimeUtils.hpp"
#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

static std::atomic<bool> g_running{true};

static void SignalHandler(int signum) {
    (void)signum;
    g_running.store(false);
}

void RenderDriverConsole(uint32_t speedKmh, uint32_t distMeters, uint32_t etaSec,
                         const std::string& road, const std::string& nextTurn,
                         uint32_t hr, uint32_t spo2, const std::string& bp,
                         const std::string& priority, const std::string& alert) {
    std::cout << "================================================================================" << std::endl;
    std::cout << " SMART AEGIS : IN-VEHICLE AMBULANCE DRIVER CONSOLE [TACTICAL HUD]" << std::endl;
    std::cout << " Vehicle: AMB-101 | Driver: Off. R. Kumar | QNX RTOS 8.0 Sync: 0.8ms | Fuel: 86%" << std::endl;
    std::cout << "================================================================================" << std::endl;

    // Tactical Maneuver Banner
    std::cout << "+------------------------------------------------------------------------------+" << std::endl;
    std::cout << "| NEXT TURN: " << std::left << std::setw(66) << nextTurn << "|" << std::endl;
    std::cout << "| In: " << std::setw(6) << distMeters << " m  | Current Road: " << std::setw(30) << road
              << " | Target ETA: " << std::setw(4) << etaSec << " s |" << std::endl;
    std::cout << "+------------------------------------------------------------------------------+" << std::endl;

    // Instrument Cluster
    std::cout << "\n[ VEHICLE TELEMETRY ]" << std::endl;
    std::cout << "  Speed   : " << std::setw(3) << speedKmh << " km/h  [";
    int bars = static_cast<int>(speedKmh / 3);
    for (int i = 0; i < 30; ++i) {
        if (i < bars) std::cout << "=";
        else std::cout << " ";
    }
    std::cout << "] (Speed Limit: 60 km/h)" << std::endl;
    std::cout << "  GPS     : 12.9716 N, 77.5946 E (14 Satellites, Fix: 3D DGPS)" << std::endl;
    std::cout << "  Network : 5G SA Mesh | QNX Coordinator Link: ONLINE | Latency: 0.8 ms" << std::endl;

    // Patient Vitals HUD & Waveform
    std::cout << "\n[ PATIENT VITALS MONITOR - TRIAGE LEVEL: " << priority << " ]" << std::endl;
    std::cout << "  Heart Rate  : " << std::setw(3) << hr << " BPM    | SpO2 : " << spo2 << " %" << std::endl;
    std::cout << "  Blood Press : " << std::setw(7) << bp << " mmHg | Resp : 22 /min | Temp: 37.2 C" << std::endl;
    std::cout << "  ECG Waveform: /\\_-_/\\__---/\\_-_/\\__---/\\_-_/\\__---/\\_-_/\\__ [LEAD II NOMINAL]" << std::endl;

    // Corridor and Alerts
    std::cout << "\n[ GREEN CORRIDOR & TRAFFIC SIGNALS ]" << std::endl;
    std::cout << "  Signal Status : INT_1 [GREEN *Preempted*] ===> INT_2 [GREEN] ===> INT_3 [GREEN]" << std::endl;
    std::cout << "  Corridor Mode : DYNAMIC GREEN WAVE (All-Red Conflict Lockdown Engaged)" << std::endl;

    if (!alert.empty()) {
        std::cout << "\n>>> FLASH ALERT: " << alert << " <<<" << std::endl;
    }
    std::cout << "================================================================================" << std::endl;
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    bool continuous = false;
    uint32_t speed = 58;
    std::string customAlert = "";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--continuous" || arg == "-c") {
            continuous = true;
        } else if ((arg == "--speed" || arg == "-s") && i + 1 < argc) {
            speed = static_cast<uint32_t>(std::stoul(argv[++i]));
        } else if ((arg == "--alert" || arg == "-a") && i + 1 < argc) {
            customAlert = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: driver_dashboard [OPTIONS]\n"
                      << "  --continuous, -c     Run continuous tactical refresh loop\n"
                      << "  --speed, -s <kmh>    Simulate vehicle speed (default: 58)\n"
                      << "  --alert, -a <text>   Inject tactical flash alert\n"
                      << "  --help, -h           Display this help text\n";
            return 0;
        }
    }

    if (!continuous) {
        RenderDriverConsole(speed, 220, 24, "MG Road Northern Arterial",
                            "Turn Left onto Queens Road Bypass",
                            118, 93, "155/95", "P1_CRITICAL", customAlert);
        return 0;
    }

    std::cout << "[INFO] Starting continuous driver console. Press Ctrl+C to exit.\n";
    uint32_t dist = 450;
    uint32_t eta = 42;

    while (g_running.load()) {
        std::cout << "\033[2J\033[1;1H";
        RenderDriverConsole(speed, dist, eta, "MG Road Northern Arterial",
                            "Turn Left onto Queens Road Bypass in 200m",
                            115 + (dist % 5), 92 + (dist % 3), "150/92", "P1_CRITICAL", customAlert);
        if (dist > 20) dist -= 10;
        if (eta > 2) eta -= 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "\n[INFO] Driver console stopped cleanly.\n";
    return 0;
}
