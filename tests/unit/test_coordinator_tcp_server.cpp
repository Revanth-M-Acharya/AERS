/**
 * @file test_coordinator_tcp_server.cpp
 * @brief Unit & Integration Tests for SMART AEGIS Phase 2 Coordinator TCP Server.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/server/CoordinatorServer.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/network/TcpSocket.hpp"
#include "common/protocol/Packet.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>

using namespace aegis;

// Test Helper: Mock TCP Client representing a Traffic Node
class MockTrafficNode {
public:
    MockTrafficNode(const std::string& nodeId, const std::string& host, uint16_t port)
        : nodeId_(nodeId), host_(host), port_(port) {}

    ~MockTrafficNode() {
        Disconnect();
    }

    bool Connect() {
        if (!socket_.Create()) return false;
        return socket_.Connect(host_, port_, 2000);
    }

    bool SendPacket(const protocol::Packet& pkt) {
        return socket_.SendAll(pkt.Serialize());
    }

    bool ReceivePacket(protocol::Packet& outPkt, uint32_t timeoutMs = 2000) {
        uint8_t buf[2048];
        uint64_t start = common::TimeUtils::GetMonotonicTimeMs();
        std::vector<uint8_t> accumulator;

        while (common::TimeUtils::GetMonotonicTimeMs() - start < timeoutMs) {
            ssize_t n = socket_.Receive(buf, sizeof(buf));
            if (n > 0) {
                accumulator.insert(accumulator.end(), buf, buf + n);
                size_t consumed = 0;
                if (protocol::Packet::Deserialize(accumulator.data(), accumulator.size(), outPkt, consumed)) {
                    return true;
                }
            } else if (n == 0) {
                return false;
            }
            common::TimeUtils::SleepMs(10);
        }
        return false;
    }

    void Disconnect() {
        socket_.Close();
    }

    const std::string& GetNodeId() const { return nodeId_; }

private:
    std::string nodeId_;
    std::string host_;
    uint16_t port_;
    network::TcpSocket socket_;
};

int main() {
    std::cout << "==========================================================\n"
              << " SMART AEGIS: Coordinator TCP Server Test Suite           \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n"
              << "==========================================================\n\n";

    // ------------------------------------------------------------------------
    // TEST 1: ServerConfig Defaults & Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 1] ServerConfig Defaults & Validation...\n";
    {
        config::ServerConfig cfg;
        assert(cfg.Validate());
        assert(cfg.port == 8080);
        assert(cfg.maxClients == 32);
        assert(!cfg.ToString().empty());

        // Invalidate port
        cfg.port = 0;
        assert(!cfg.Validate());
        std::cout << "  ✓ PASS: ServerConfig validation and formatting verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 2: Packet Serialization, Deserialization & CRC Integrity
    // ------------------------------------------------------------------------
    std::cout << "[TEST 2] Packet Serialization, Deserialization & CRC...\n";
    {
        protocol::Packet original(protocol::MessageType::HELLO, "INT_A", "PAYLOAD_INIT");
        std::vector<uint8_t> wireData = original.Serialize();
        assert(wireData.size() == protocol::PACKET_HEADER_SIZE + original.GetPayloadSize());

        protocol::Packet decoded;
        size_t consumed = 0;
        bool ok = protocol::Packet::Deserialize(wireData.data(), wireData.size(), decoded, consumed);
        assert(ok);
        assert(consumed == wireData.size());
        assert(decoded.GetType() == protocol::MessageType::HELLO);
        assert(decoded.GetNodeId() == "INT_A");
        assert(decoded.GetPayload() == "PAYLOAD_INIT");

        // Test Corrupted CRC rejection
        wireData[48] ^= 0xFF; // Invert CRC byte
        protocol::Packet corruptedPkt;
        consumed = 0;
        bool corruptOk = protocol::Packet::Deserialize(wireData.data(), wireData.size(), corruptedPkt, consumed);
        assert(!corruptOk);
        assert(consumed == 2); // Discarded magic offset
        std::cout << "  ✓ PASS: Packet wire framing, CRC verification, and corruption rejection verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 3: Coordinator Server Startup & Listener Binding
    // ------------------------------------------------------------------------
    std::cout << "[TEST 3] Server Startup, Binding & Non-Blocking State...\n";
    const uint16_t TEST_PORT = 19123;
    config::ServerConfig serverConfig;
    serverConfig.bindAddress = "127.0.0.1";
    serverConfig.port = TEST_PORT;
    serverConfig.logToConsole = false; // Keep test logs clean
    serverConfig.heartbeatTimeoutMs = 1500; // Short timeout for test

    coordinator::CoordinatorServer server(serverConfig);
    assert(server.Start());
    assert(server.IsRunning());
    std::cout << "  ✓ PASS: Coordinator TCP Server bound to 127.0.0.1:" << TEST_PORT << " and listening.\n";

    // ------------------------------------------------------------------------
    // TEST 4: Single Client Connection & Automatic HELLO Registration
    // ------------------------------------------------------------------------
    std::cout << "[TEST 4] Client Connection & Automatic HELLO Registration...\n";
    {
        MockTrafficNode nodeA("INT_A", "127.0.0.1", TEST_PORT);
        assert(nodeA.Connect());

        // Send HELLO packet from INT_A
        protocol::Packet helloPkt(protocol::MessageType::HELLO, "INT_A", "INTERSECTION_A_ONLINE");
        assert(nodeA.SendPacket(helloPkt));

        // Expect ACK back from coordinator acknowledging registration
        protocol::Packet ackPkt;
        assert(nodeA.ReceivePacket(ackPkt, 2000));
        assert(ackPkt.GetType() == protocol::MessageType::ACK);
        assert(ackPkt.GetNodeId() == "COORDINATOR");
        assert(ackPkt.GetPayload().find("REGISTERED: INT_A") != std::string::npos);

        // Verify Coordinator Registry State
        common::TimeUtils::SleepMs(100);
        assert(server.IsNodeRegistered("INT_A"));
        assert(server.GetRegisteredNodeCount() == 1);
        std::cout << "  ✓ PASS: Automatic registration of node INT_A via HELLO packet verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 5: Multi-Client Concurrent Registration (Nodes A, B, C)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 5] Multi-Client Concurrent Registration (Nodes A, B, C)...\n";
    {
        MockTrafficNode nodeA("INT_A", "127.0.0.1", TEST_PORT);
        MockTrafficNode nodeB("INT_B", "127.0.0.1", TEST_PORT);
        MockTrafficNode nodeC("INT_C", "127.0.0.1", TEST_PORT);

        assert(nodeA.Connect());
        assert(nodeB.Connect());
        assert(nodeC.Connect());

        assert(nodeA.SendPacket(protocol::Packet(protocol::MessageType::HELLO, "INT_A")));
        assert(nodeB.SendPacket(protocol::Packet(protocol::MessageType::HELLO, "INT_B")));
        assert(nodeC.SendPacket(protocol::Packet(protocol::MessageType::HELLO, "INT_C")));

        protocol::Packet ackA, ackB, ackC;
        assert(nodeA.ReceivePacket(ackA, 2000));
        assert(nodeB.ReceivePacket(ackB, 2000));
        assert(nodeC.ReceivePacket(ackC, 2000));

        common::TimeUtils::SleepMs(100);
        assert(server.GetRegisteredNodeCount() == 3);
        assert(server.IsNodeRegistered("INT_A"));
        assert(server.IsNodeRegistered("INT_B"));
        assert(server.IsNodeRegistered("INT_C"));
        std::cout << "  ✓ PASS: Multi-client support verified (3 concurrent traffic nodes registered).\n";

        // --------------------------------------------------------------------
        // TEST 6: Targeted Command Transmission (Coordinator -> Specific Node)
        // --------------------------------------------------------------------
        std::cout << "[TEST 6] Targeted Command Transmission (SendToNode)...\n";
        protocol::Packet greenCmd(protocol::MessageType::GREEN, "COORDINATOR", "CORRIDOR_ACTIVE");
        assert(server.SendToNode("INT_B", greenCmd));

        protocol::Packet rxB;
        assert(nodeB.ReceivePacket(rxB, 2000));
        assert(rxB.GetType() == protocol::MessageType::GREEN);
        assert(rxB.GetPayload() == "CORRIDOR_ACTIVE");
        std::cout << "  ✓ PASS: Targeted GREEN command delivered exclusively to INT_B.\n";

        // --------------------------------------------------------------------
        // TEST 7: Global Broadcast (Coordinator -> All Nodes)
        // --------------------------------------------------------------------
        std::cout << "[TEST 7] Global Broadcast Transmission (Broadcast)...\n";
        protocol::Packet broadcastPkt(protocol::MessageType::NORMAL_MODE, "COORDINATOR", "RESUME_SCHEDULE");
        size_t sentCount = server.Broadcast(broadcastPkt);
        assert(sentCount == 3);

        protocol::Packet rxA, rxC;
        assert(nodeA.ReceivePacket(rxA, 2000));
        assert(nodeC.ReceivePacket(rxC, 2000));
        assert(rxA.GetType() == protocol::MessageType::NORMAL_MODE);
        assert(rxC.GetType() == protocol::MessageType::NORMAL_MODE);
        std::cout << "  ✓ PASS: Global broadcast delivered to all 3 registered intersections.\n";

        // --------------------------------------------------------------------
        // TEST 8: Heartbeat Exchange & Automatic Heartbeat ACK
        // --------------------------------------------------------------------
        std::cout << "[TEST 8] Heartbeat Exchange & Automated Coordinator ACK...\n";
        protocol::Packet heartbeat(protocol::MessageType::HEARTBEAT, "INT_A", "TICK_1");
        assert(nodeA.SendPacket(heartbeat));

        protocol::Packet hbAck;
        assert(nodeA.ReceivePacket(hbAck, 2000));
        assert(hbAck.GetType() == protocol::MessageType::ACK);
        assert(hbAck.GetPayload() == "HEARTBEAT_ACK");
        std::cout << "  ✓ PASS: Node HEARTBEAT recognized and answered with HEARTBEAT_ACK.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 9: Heartbeat Timeout Detection & Stale Node Cleanup
    // ------------------------------------------------------------------------
    std::cout << "[TEST 9] Heartbeat Timeout Detection & Inactive Node Pruning...\n";
    {
        MockTrafficNode silentNode("SILENT_NODE", "127.0.0.1", TEST_PORT);
        assert(silentNode.Connect());
        assert(silentNode.SendPacket(protocol::Packet(protocol::MessageType::HELLO, "SILENT_NODE")));

        protocol::Packet ack;
        assert(silentNode.ReceivePacket(ack, 2000));
        common::TimeUtils::SleepMs(100);
        assert(server.IsNodeRegistered("SILENT_NODE"));

        // Wait beyond configured heartbeat timeout (1500 ms + margin)
        std::cout << "  Waiting for heartbeat timeout (1.8s)...\n";
        common::TimeUtils::SleepMs(1800);

        // Verify silent node was pruned
        assert(!server.IsNodeRegistered("SILENT_NODE"));
        std::cout << "  ✓ PASS: Silent node pruned automatically after heartbeat timeout exceeded.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 10: Graceful Server Shutdown
    // ------------------------------------------------------------------------
    std::cout << "[TEST 10] Graceful Server Shutdown...\n";
    {
        server.Stop();
        assert(!server.IsRunning());
        assert(server.GetRegisteredNodeCount() == 0);
        std::cout << "  ✓ PASS: Server stopped gracefully, worker threads joined, sockets released.\n";
    }

    std::cout << "\n==========================================================\n"
              << " Phase 2 Coordinator TCP Server Validation:               \n"
              << " Total Tests Passed: 10 / 10 (100%)                       \n"
              << " Total Failures:     0                                    \n"
              << "==========================================================\n"
              << ">>> [SUCCESS] Coordinator TCP Server Verified 100%!       \n\n";

    return 0;
}

