/**
 * @file example_server.cpp
 * @brief Demonstration of Coordinator TCP Server in SMART AEGIS.
 *
 * Implements Central Coordinator listening for edge Traffic Nodes (A, B, C)
 * over TCP/IP, managing registrations, heartbeats, status updates, commands,
 * and automated 5-second timeout node isolation.
 */

#include "tcp_server.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

namespace {
std::atomic<bool> g_shutdownRequested{false};

void SignalHandler(int signum) {
    (void)signum;
    g_shutdownRequested.store(true);
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    uint16_t port = 9000;
    std::string host = "0.0.0.0";

    if (argc > 1) {
        port = static_cast<uint16_t>(std::atoi(argv[1]));
    }
    if (argc > 2) {
        host = argv[2];
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS - QNX Coordinator TCP Server               \n";
    std::cout << " Target: Raspberry Pi 4 (QNX SDP 8.0 / POSIX)            \n";
    std::cout << "==========================================================\n";

    aegis::network::TcpServer server;

    // Set 5-second heartbeat timeout per requirement
    server.SetHeartbeatTimeoutMs(5000);

    // Register message handler
    server.SetOnMessage([&server](const std::string& nodeId, const aegis::network::Message& msg) {
        switch (msg.GetType()) {
            case aegis::network::MessageType::REGISTER: {
                std::cout << "[COORDINATOR] Acknowledging registration for node: " << nodeId << "\n";
                aegis::network::Message ack = aegis::network::Message::CreateAck(
                    "COORDINATOR", "NODE_REGISTERED_SUCCESSFULLY");
                server.Send(nodeId, ack);
                break;
            }
            case aegis::network::MessageType::HEARTBEAT: {
                // Heartbeat timestamp updated automatically in TcpServer
                break;
            }
            case aegis::network::MessageType::STATUS: {
                std::cout << "[COORDINATOR] Node status from " << nodeId
                          << ": " << msg.GetPayload() << "\n";
                aegis::network::Message ack = aegis::network::Message::CreateAck("COORDINATOR", "STATUS_RECORDED");
                server.Send(nodeId, ack);
                break;
            }
            case aegis::network::MessageType::TRAFFIC_STATE: {
                std::cout << "[COORDINATOR] Traffic State update from " << nodeId
                          << ": " << msg.GetPayload() << "\n";
                break;
            }
            case aegis::network::MessageType::COMMAND: {
                std::cout << "[COORDINATOR] Received command from " << nodeId << ": "
                          << msg.GetPayload() << "\n";
                break;
            }
            case aegis::network::MessageType::ACK: {
                std::cout << "[COORDINATOR] Received ACK from " << nodeId << ": "
                          << msg.GetPayload() << "\n";
                break;
            }
            case aegis::network::MessageType::ERROR: {
                std::cerr << "[COORDINATOR] Error alert from " << nodeId << ": "
                          << msg.GetPayload() << "\n";
                break;
            }
            case aegis::network::MessageType::UNKNOWN:
            default:
                break;
        }
    });

    server.SetOnConnect([](const std::string& nodeId, const std::string& ip, uint16_t clientPort) {
        std::cout << "[COORDINATOR] [NODE_ONLINE] " << nodeId
                  << " established link from " << ip << ":" << clientPort << "\n";
    });

    server.SetOnDisconnect([](const std::string& nodeId) {
        std::cout << "[COORDINATOR] [NODE_OFFLINE] " << nodeId
                  << " severed link / removed from cluster\n";
    });

    if (!server.Start(port, host)) {
        std::cerr << "[COORDINATOR] [FATAL] Failed to start server on " << host << ":" << port << "\n";
        return 1;
    }

    std::cout << "[COORDINATOR] Listening for Traffic Nodes on " << host << ":" << port << "...\n";
    std::cout << "[COORDINATOR] Press Ctrl+C to terminate cleanly.\n\n";

    uint32_t iteration = 0;
    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        iteration++;

        // Every 10 seconds, broadcast a simulated emergency preemption green corridor command
        if (iteration % 10 == 0) {
            auto nodes = server.GetConnectedNodes();
            if (!nodes.empty()) {
                std::cout << "\n[COORDINATOR] Broadcasting PREEMPTION_GREEN_CORRIDOR command to all nodes...\n";
                aegis::network::Message cmd = aegis::network::Message::CreateCommand(
                    "COORDINATOR", "CORRIDOR_PREEMPTION:AMBULANCE_01:WINDOW=30S");
                size_t sentCount = server.Broadcast(cmd);
                std::cout << "[COORDINATOR] Command broadcasted to " << sentCount << " active nodes.\n\n";
            }
        }
    }

    std::cout << "\n[COORDINATOR] Shutdown signal received. Stopping server...\n";
    server.Stop();
    std::cout << server.GenerateStatusReport();
    std::cout << "[COORDINATOR] Server stopped gracefully.\n";

    return 0;
}

