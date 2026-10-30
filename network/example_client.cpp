/**
 * @file example_client.cpp
 * @brief Demonstration of Traffic Node TCP Client in SMART AEGIS.
 *
 * Implements an edge Traffic Node (e.g., Traffic Node A, B, or C) connecting
 * to Coordinator TCP Server, transmitting 1-second heartbeats, reporting
 * traffic states, acknowledging preemption commands, and gracefully reconnecting.
 */

#include "tcp_client.hpp"
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
    std::string nodeId = "TRAFFIC_NODE_A";
    std::string host = "127.0.0.1";
    uint16_t port = 9000;

    if (argc > 1) {
        nodeId = argv[1];
    }
    if (argc > 2) {
        host = argv[2];
    }
    if (argc > 3) {
        port = static_cast<uint16_t>(std::atoi(argv[3]));
    }

    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS - QNX Traffic Node TCP Client              \n";
    std::cout << " Target: Raspberry Pi 4 (QNX SDP 8.0 / POSIX)            \n";
    std::cout << " Node Identity: " << nodeId << "\n";
    std::cout << "==========================================================\n";

    aegis::network::TcpClient client(nodeId);

    // Set auto-reconnect with 1-second retry interval
    client.SetAutoReconnect(true, 1000);

    // Register message handler
    client.SetOnMessage([&client, &nodeId](const aegis::network::Message& msg) {
        switch (msg.GetType()) {
            case aegis::network::MessageType::COMMAND: {
                std::cout << "[" << nodeId << "] Preemption Command received: "
                          << msg.GetPayload() << "\n";
                // Acknowledge command back to Coordinator
                aegis::network::Message ack = aegis::network::Message::CreateAck(
                    nodeId, "COMMAND_EXECUTED:SIGNAL_SET_GREEN");
                client.Send(ack);
                break;
            }
            case aegis::network::MessageType::ACK: {
                std::cout << "[" << nodeId << "] Coordinator ACK: " << msg.GetPayload() << "\n";
                break;
            }
            case aegis::network::MessageType::ERROR: {
                std::cerr << "[" << nodeId << "] Coordinator ERROR alert: " << msg.GetPayload() << "\n";
                break;
            }
            default:
                break;
        }
    });

    client.SetOnConnectionChange([&nodeId](bool connected) {
        std::cout << "[" << nodeId << "] Link status changed: "
                  << (connected ? "CONNECTED (ONLINE)" : "DISCONNECTED (OFFLINE)") << "\n";
    });

    std::cout << "[" << nodeId << "] Connecting to Coordinator at " << host << ":" << port << "...\n";

    if (!client.Connect(host, port, 3000)) {
        std::cerr << "[" << nodeId << "] [WARN] Initial connection failed. Background auto-reconnect will retry...\n";
    }

    std::cout << "[" << nodeId << "] Press Ctrl+C to disconnect and exit.\n\n";

    uint32_t count = 0;
    while (!g_shutdownRequested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        count++;

        if (!client.IsConnected()) {
            continue;
        }

        // Every 3 seconds, send a STATUS message
        if (count % 3 == 0) {
            std::string statusJson = "{\"node\":\"" + nodeId + "\",\"cpuTemp\":44.5,\"queueLength\":12}";
            client.Send(aegis::network::MessageType::STATUS, statusJson);
        }

        // Every 5 seconds, send a TRAFFIC_STATE message
        if (count % 5 == 0) {
            std::string stateJson = "{\"phase\":\"RED\",\"timeRemaining\":15,\"direction\":\"NORTH_SOUTH\"}";
            client.Send(aegis::network::MessageType::TRAFFIC_STATE, stateJson);
        }
    }

    std::cout << "\n[" << nodeId << "] Shutdown signal received. Disconnecting...\n";
    client.Disconnect();
    std::cout << client.GenerateStatusReport();
    std::cout << "[" << nodeId << "] Clean shutdown complete.\n";

    return 0;
}

