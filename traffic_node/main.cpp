/**
 * @file main.cpp
 * @brief Standalone executable entrypoint for SMART AEGIS Traffic Signal Node.
 * Target: Raspberry Pi 4 Model B (QNX SDP 8.0 Starter Kit Image / aarch64le).
 * Follows strict layered architecture:
 * IGpioDriver -> RelayController -> LampController -> TrafficSignalFSM -> SignalController -> TrafficClient
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/config/TrafficNodeConfig.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "traffic_node/gpio/IGpioDriver.hpp"
#include "traffic_node/gpio/MockGpioDriver.hpp"
#include "traffic_node/gpio/Rpi4GpioDriver.hpp"
#include "traffic_node/relay_controller/RelayController.hpp"
#include "traffic_node/lamp_controller/LampController.hpp"
#include "traffic_node/signal_controller/TrafficSignalFSM.hpp"
#include "traffic_node/signal_controller/SignalController.hpp"
#include "traffic_node/watchdog/NodeWatchdog.hpp"
#include "traffic_node/client/TrafficClient.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>

namespace {
std::atomic<bool> g_terminateSignal{false};

void SignalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_terminateSignal.store(true);
    }
}
} // namespace

int main(int argc, char* argv[]) {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    aegis::traffic::TrafficNodeConfig config;

    // Command-line argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config.LoadFromFile(argv[++i]);
        } else if (arg == "--node" && i + 1 < argc) {
            config.nodeId = argv[++i];
        } else if (arg == "--ip" && i + 1 < argc) {
            config.coordinatorIp = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.coordinatorPort = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: traffic_signal_node [OPTIONS]\n"
                      << "  --config <path>    Load configuration file\n"
                      << "  --node <id>        Set Node ID (INT_A, INT_B, INT_C)\n"
                      << "  --ip <address>     Coordinator IP address (default: 127.0.0.1)\n"
                      << "  --port <port>      Coordinator port (default: 8080)\n"
                      << "  --help, -h         Display this help message\n";
            return 0;
        }
    }

    std::cout << "==========================================================\n"
              << " SMART AEGIS - TRAFFIC SIGNAL NODE CONTROLLER             \n"
              << " Node ID : " << config.nodeId << "\n"
              << " Target  : Raspberry Pi 4 Model B (QNX SDP 8.0 / aarch64le)\n"
              << "==========================================================\n";

    // 1. Initialize Logger
    aegis::traffic::TrafficLogger::Instance().Initialize(config.nodeId, config.logFilePath, config.logToConsole);
    aegis::traffic::TrafficLogger::Instance().Info("MAIN", "Bootstrapping Traffic Signal Node: " + config.nodeId);
    aegis::traffic::TrafficLogger::Instance().Info("MAIN", config.ToString());

    // 2. Instantiate Hardware-Level GPIO Driver (BCM2711 on QNX; Mock on POSIX/Simulation)
    std::shared_ptr<aegis::traffic::IGpioDriver> gpioDriver;
#if defined(__QNX__)
    gpioDriver = std::make_shared<aegis::traffic::Rpi4GpioDriver>();
#else
    gpioDriver = std::make_shared<aegis::traffic::MockGpioDriver>();
#endif
    if (!gpioDriver->Initialize()) {
        aegis::traffic::TrafficLogger::Instance().Error("MAIN", "CRITICAL: IGpioDriver initialization failed!");
        return 1;
    }

    // 3. Initialize 4-Channel Relay Controller (Depends ONLY on IGpioDriver)
    auto relayController = std::make_shared<aegis::traffic::RelayController>(gpioDriver);
    if (!relayController->Initialize(config.relayPinRed, config.relayPinYellow,
                                     config.relayPinGreen, config.relayPinAux)) {
        aegis::traffic::TrafficLogger::Instance().Error("MAIN", "CRITICAL: Relay Controller initialization failed!");
        gpioDriver->Shutdown();
        return 1;
    }

    // 4. Initialize Optical Lamp Controller (Translates aspects to relay activations; ZERO TCP logic)
    auto lampController = std::make_shared<aegis::traffic::LampController>(relayController);
    if (!lampController->Initialize(config.relaySwitchDelayMs)) {
        aegis::traffic::TrafficLogger::Instance().Error("MAIN", "CRITICAL: Lamp Controller initialization failed!");
        relayController->Shutdown();
        gpioDriver->Shutdown();
        return 1;
    }

    // 5. Initialize Traffic Signal Finite State Machine (Deterministic safety transitions; ZERO GPIO)
    auto fsm = std::make_shared<aegis::traffic::TrafficSignalFSM>(lampController);
    if (!fsm->Initialize(config)) {
        aegis::traffic::TrafficLogger::Instance().Error("MAIN", "CRITICAL: Traffic Signal FSM initialization failed!");
        lampController->Shutdown();
        relayController->Shutdown();
        gpioDriver->Shutdown();
        return 1;
    }

    // 6. Initialize Signal Controller (High-level coordination interface; ZERO GPIO code)
    auto signalController = std::make_shared<aegis::traffic::SignalController>(fsm);

    // Boot Safety Rule: Always boot into ALL_RED
    signalController->CommandAllRed("System Boot Initial Safety State");

    // 7. Initialize Multi-Threaded Node Watchdog
    auto watchdog = std::make_shared<aegis::traffic::NodeWatchdog>(
        config.watchdogTimeoutMs,
        [signalController](aegis::traffic::WatchdogThreadId threadId, uint64_t elapsedMs) {
            aegis::traffic::TrafficLogger::Instance().Error(
                "WATCHDOG",
                "[WATCHDOG_FAULT] Thread " + aegis::traffic::WatchdogThreadIdToString(threadId) +
                " stalled for " + std::to_string(elapsedMs) + "ms. Forcing failsafe ALL_RED!");
            signalController->CommandAllRed("Watchdog Timeout Failsafe");
        }
    );
    watchdog->Start();

    // 8. Initialize & Start TCP Client (Handles connection, HELLO registration, heartbeats)
    auto client = std::make_unique<aegis::traffic::TrafficClient>(config, signalController, nullptr, watchdog);
    if (!client->Start()) {
        aegis::traffic::TrafficLogger::Instance().Error("MAIN", "CRITICAL: Traffic TCP Client failed to start!");
        watchdog->Stop();
        signalController->Shutdown();
        lampController->Shutdown();
        relayController->Shutdown();
        gpioDriver->Shutdown();
        return 1;
    }

    std::cout << "[INFO] Traffic Signal Node " << config.nodeId
              << " is operating. Press Ctrl+C to terminate.\n";

    // 9. Main Supervisor Loop (Monitors execution, pets watchdog)
    while (!g_terminateSignal.load()) {
        watchdog->Pet(aegis::traffic::WatchdogThreadId::MAIN);
        aegis::common::TimeUtils::SleepMs(250);
    }

    std::cout << "\n[INFO] Termination signal captured. Executing graceful shutdown...\n";

    watchdog->Stop();
    client->Stop();
    signalController->Shutdown();
    lampController->Shutdown();
    relayController->Shutdown();
    gpioDriver->Shutdown();
    aegis::traffic::TrafficLogger::Instance().Shutdown();

    std::cout << "[INFO] SMART AEGIS Traffic Signal Node " << config.nodeId << " shut down safely.\n";
    return 0;
}
