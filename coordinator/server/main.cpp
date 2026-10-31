/**
 * @file main.cpp
 * @brief Standalone entrypoint for SMART AEGIS Coordinator TCP Server (QNX SDP 8.0 Raspberry Pi 4).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/server/CoordinatorServer.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

namespace {
std::atomic<bool> g_terminateSignal{false};

void SignalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        g_terminateSignal.store(true);
    }
}
} // namespace

int main(int argc, char* argv[]) {
    // Register signal handlers for graceful termination
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    aegis::config::ServerConfig config;

    // Command-line argument parsing
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config.LoadFromFile(argv[++i]);
        } else if (arg == "--ip" && i + 1 < argc) {
            config.bindAddress = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: coordinator_server [OPTIONS]\n"
                      << "  --config <path>    Load configuration file\n"
                      << "  --ip <address>     Bind IP address (default: 0.0.0.0)\n"
                      << "  --port <port>      Listen port (default: 8080)\n"
                      << "  --help, -h         Display this help message\n";
            return 0;
        }
    }

    std::cout << "==========================================================\n"
              << " SMART AEGIS - COORDINATOR TCP SERVER                     \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / aarch64le) \n"
              << "==========================================================\n";

    aegis::coordinator::CoordinatorServer server(config);

    // Register protocol event hooks
    server.SetConnectCallback([](const std::string& nodeId, const std::string& ip, uint16_t port) {
        std::cout << "[EVENT] Node " << nodeId << " connected from " << ip << ":" << port << "\n";
    });

    server.SetDisconnectCallback([](const std::string& nodeId, const std::string& reason) {
        std::cout << "[EVENT] Node " << nodeId << " disconnected: " << reason << "\n";
    });

    server.SetPacketCallback([&server](const std::string& nodeId, const aegis::protocol::Packet& packet) {
        // Echo or process corridor signals
        if (packet.GetType() == aegis::protocol::MessageType::STATUS) {
            // Acknowledge intersection status
            aegis::protocol::Packet ack(aegis::protocol::MessageType::ACK, "COORDINATOR", "STATUS_RECEIVED");
            server.SendToNode(nodeId, ack);
        }
    });

    if (!server.Start()) {
        std::cerr << "[FATAL] Failed to start Coordinator TCP Server!\n";
        return 1;
    }

    std::cout << "[INFO] Coordinator TCP Server is running. Press Ctrl+C to exit.\n";

    // Supervisor loop
    while (!g_terminateSignal.load()) {
        aegis::common::TimeUtils::SleepMs(250);
    }

    std::cout << "\n[INFO] Termination signal captured. Shutting down...\n";
    server.Stop();
    std::cout << "[INFO] SMART AEGIS Coordinator shut down cleanly.\n";

    return 0;
}

