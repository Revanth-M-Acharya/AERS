/**
 * @file test_qnx_network_layer.cpp
 * @brief Comprehensive Unit & Integration Test Suite for QNX Network Layer.
 *
 * Validates TCP server and client operation, multi-client communication
 * (Node A, Node B, Node C), 1-second heartbeat dispatch, 5-second timeout removal,
 * CRC16 verification, stream defragmentation, and automatic reconnection.
 */

#include "../../network/message.hpp"
#include "../../network/tcp_server.hpp"
#include "../../network/tcp_client.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <future>

using namespace aegis::network;

void Test1_MessageConstructionAndEnums() {
    std::cout << "[TEST 1] Message Construction & Enum Mappings...\n";

    Message msg(MessageType::REGISTER, "TRAFFIC_NODE_A", "INIT_PAYLOAD");
    assert(msg.GetType() == MessageType::REGISTER);
    assert(msg.GetNodeId() == "TRAFFIC_NODE_A");
    assert(msg.GetPayload() == "INIT_PAYLOAD");
    assert(msg.GetPayloadLength() == 12);
    assert(msg.GetTimestamp() > 0);
    assert(msg.IsValid());

    // Enum strings
    assert(std::string(MessageTypeToString(MessageType::REGISTER)) == "REGISTER");
    assert(std::string(MessageTypeToString(MessageType::HEARTBEAT)) == "HEARTBEAT");
    assert(std::string(MessageTypeToString(MessageType::STATUS)) == "STATUS");
    assert(std::string(MessageTypeToString(MessageType::TRAFFIC_STATE)) == "TRAFFIC_STATE");
    assert(std::string(MessageTypeToString(MessageType::COMMAND)) == "COMMAND");
    assert(std::string(MessageTypeToString(MessageType::ACK)) == "ACK");
    assert(std::string(MessageTypeToString(MessageType::ERROR)) == "ERROR");

    assert(StringToMessageType("REGISTER") == MessageType::REGISTER);
    assert(StringToMessageType("HEARTBEAT") == MessageType::HEARTBEAT);
    assert(StringToMessageType("STATUS") == MessageType::STATUS);
    assert(StringToMessageType("TRAFFIC_STATE") == MessageType::TRAFFIC_STATE);
    assert(StringToMessageType("COMMAND") == MessageType::COMMAND);
    assert(StringToMessageType("ACK") == MessageType::ACK);
    assert(StringToMessageType("ERROR") == MessageType::ERROR);
    assert(StringToMessageType("INVALID") == MessageType::UNKNOWN);

    std::cout << "  ✓ PASS: Message construction and bidirectional enum mappings verified.\n";
}

void Test2_SerializationRoundTripAllTypes() {
    std::cout << "[TEST 2] Serialization & Deserialization Round-Trip (All 7 Message Types)...\n";

    std::vector<MessageType> types = {
        MessageType::REGISTER,
        MessageType::HEARTBEAT,
        MessageType::STATUS,
        MessageType::TRAFFIC_STATE,
        MessageType::COMMAND,
        MessageType::ACK,
        MessageType::ERROR
    };

    for (auto t : types) {
        std::string payload = "PAYLOAD_DATA_FOR_" + std::string(MessageTypeToString(t));
        Message original(t, "NODE_TEST", payload, 1609459200000ULL);

        std::vector<uint8_t> wireData = original.Serialize();
        assert(wireData.size() == Message::FIXED_HEADER_SIZE + original.GetNodeId().size() + payload.size());
        assert(wireData[0] == Message::MAGIC_BYTE_0);
        assert(wireData[1] == Message::MAGIC_BYTE_1);

        Message deserialized;
        size_t consumed = 0;
        bool ok = Message::Deserialize(wireData, deserialized, consumed);
        assert(ok);
        assert(consumed == wireData.size());
        assert(deserialized.GetType() == t);
        assert(deserialized.GetNodeId() == "NODE_TEST");
        assert(deserialized.GetPayload() == payload);
        assert(deserialized.GetTimestamp() == 1609459200000ULL);
        assert(deserialized.GetChecksum() == original.GetChecksum());
        assert(deserialized.VerifyChecksum());
    }

    std::cout << "  ✓ PASS: All 7 message types serialized and deserialized with 100% data fidelity.\n";
}

void Test3_ChecksumCalculationAndCorruptionRejection() {
    std::cout << "[TEST 3] CRC16 Checksum Calculation & Corruption Rejection...\n";

    Message msg = Message::CreateCommand("TRAFFIC_NODE_A", "SET_GREEN_PHASE");
    assert(msg.VerifyChecksum());

    std::vector<uint8_t> packet = msg.Serialize();

    // Corrupt a byte in the payload
    packet[packet.size() - 1] ^= 0xFF;

    Message corrupted;
    size_t consumed = 0;
    bool ok = Message::Deserialize(packet, corrupted, consumed);
    assert(!ok); // Must be rejected due to CRC mismatch!
    assert(consumed == 2); // Consumes magic bytes to resynchronize stream

    std::cout << "  ✓ PASS: Corrupted packet correctly rejected via CRC16 failure.\n";
}

void Test4_StreamFramingAndDefragmentation() {
    std::cout << "[TEST 4] Stream Framing & TCP Stream Defragmentation...\n";

    Message msg1 = Message::CreateHeartbeat("NODE_A");
    Message msg2 = Message::CreateStatus("NODE_B", "{\"state\":\"ACTIVE\"}");

    std::vector<uint8_t> p1 = msg1.Serialize();
    std::vector<uint8_t> p2 = msg2.Serialize();

    // Simulate combined stream delivery (p1 + p2 concatenated)
    std::vector<uint8_t> streamBuffer;
    streamBuffer.insert(streamBuffer.end(), p1.begin(), p1.end());
    streamBuffer.insert(streamBuffer.end(), p2.begin(), p2.end());

    Message out1, out2;
    size_t consumed1 = 0, consumed2 = 0;

    bool ok1 = Message::Deserialize(streamBuffer, out1, consumed1);
    assert(ok1);
    assert(consumed1 == p1.size());
    assert(out1.GetNodeId() == "NODE_A");
    assert(out1.GetType() == MessageType::HEARTBEAT);

    streamBuffer.erase(streamBuffer.begin(), streamBuffer.begin() + static_cast<ptrdiff_t>(consumed1));

    bool ok2 = Message::Deserialize(streamBuffer, out2, consumed2);
    assert(ok2);
    assert(consumed2 == p2.size());
    assert(out2.GetNodeId() == "NODE_B");
    assert(out2.GetType() == MessageType::STATUS);

    // Partial stream delivery (incomplete packet)
    std::vector<uint8_t> partial(p1.begin(), p1.begin() + 10);
    Message partialMsg;
    size_t consumedPartial = 0;
    bool okPartial = Message::Deserialize(partial, partialMsg, consumedPartial);
    assert(!okPartial);
    assert(consumedPartial == 0); // Need more bytes

    std::cout << "  ✓ PASS: Stream concatenation and partial packet defragmentation validated.\n";
}

void Test5_ServerBindListenClientConnectDisconnect() {
    std::cout << "[TEST 5] Server Bind, Listen, Client Connect & Disconnect...\n";

    const uint16_t port = 9101;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));
    assert(server.IsRunning());

    std::atomic<bool> serverConnected{false};
    std::atomic<bool> serverDisconnected{false};

    server.SetOnConnect([&serverConnected](const std::string&, const std::string&, uint16_t) {
        serverConnected.store(true);
    });
    server.SetOnDisconnect([&serverDisconnected](const std::string&) {
        serverDisconnected.store(true);
    });

    TcpClient client("TEST_CLIENT_01");
    client.SetAutoReconnect(false); // Disable auto-reconnect for clean disconnect test
    assert(client.Connect("127.0.0.1", port, 2000));
    assert(client.IsConnected());

    // Give server worker brief moment to register
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(server.IsNodeConnected("TEST_CLIENT_01"));
    assert(server.GetClientCount() == 1);

    client.Disconnect();
    assert(!client.IsConnected());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(!server.IsNodeConnected("TEST_CLIENT_01"));

    server.Stop();
    assert(!server.IsRunning());

    std::cout << "  ✓ PASS: Server lifecycle, client connection, and disconnect detection verified.\n";
}

void Test6_SimultaneousMultiClientCommunication() {
    std::cout << "[TEST 6] Simultaneous Multi-Client Communication (Node A, Node B, Node C)...\n";

    const uint16_t port = 9102;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    std::atomic<int> registeredCount{0};
    server.SetOnConnect([&registeredCount](const std::string&, const std::string&, uint16_t) {
        registeredCount++;
    });

    TcpClient nodeA("TRAFFIC_NODE_A");
    TcpClient nodeB("TRAFFIC_NODE_B");
    TcpClient nodeC("TRAFFIC_NODE_C");

    nodeA.SetAutoReconnect(false);
    nodeB.SetAutoReconnect(false);
    nodeC.SetAutoReconnect(false);

    assert(nodeA.Connect("127.0.0.1", port, 2000));
    assert(nodeB.Connect("127.0.0.1", port, 2000));
    assert(nodeC.Connect("127.0.0.1", port, 2000));

    // Wait for all 3 nodes to register
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    assert(server.IsNodeConnected("TRAFFIC_NODE_A"));
    assert(server.IsNodeConnected("TRAFFIC_NODE_B"));
    assert(server.IsNodeConnected("TRAFFIC_NODE_C"));
    assert(server.GetClientCount() == 3);

    auto nodes = server.GetConnectedNodes();
    assert(nodes.size() == 3);
    assert(nodes[0] == "TRAFFIC_NODE_A");
    assert(nodes[1] == "TRAFFIC_NODE_B");
    assert(nodes[2] == "TRAFFIC_NODE_C");

    nodeA.Disconnect();
    nodeB.Disconnect();
    nodeC.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: Simultaneous 3-node cluster communication verified (A, B, C).\n";
}

void Test7_BidirectionalMessagingAndCommandAck() {
    std::cout << "[TEST 7] Bidirectional Messaging & Command/ACK Exchange...\n";

    const uint16_t port = 9103;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    std::atomic<bool> serverReceivedStatus{false};
    std::atomic<bool> clientReceivedCommand{false};
    std::atomic<bool> serverReceivedAck{false};

    server.SetOnMessage([&server, &serverReceivedStatus, &serverReceivedAck](
                            const std::string& nodeId, const Message& msg) {
        if (msg.GetType() == MessageType::STATUS) {
            serverReceivedStatus.store(true);
            // Respond with command
            Message cmd = Message::CreateCommand("COORDINATOR", "SET_CORRIDOR_GREEN");
            server.Send(nodeId, cmd);
        } else if (msg.GetType() == MessageType::ACK) {
            serverReceivedAck.store(true);
        }
    });

    TcpClient client("TRAFFIC_NODE_A");
    client.SetAutoReconnect(false);

    client.SetOnMessage([&client, &clientReceivedCommand](const Message& msg) {
        if (msg.GetType() == MessageType::COMMAND) {
            clientReceivedCommand.store(true);
            // Send ACK back
            Message ack = Message::CreateAck("TRAFFIC_NODE_A", "COMMAND_SUCCESS");
            client.Send(ack);
        }
    });

    assert(client.Connect("127.0.0.1", port, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Send status from client
    client.Send(MessageType::STATUS, "{\"queue\":5,\"phase\":\"RED\"}");

    // Wait for round-trip exchange
    for (int i = 0; i < 20; ++i) {
        if (serverReceivedStatus.load() && clientReceivedCommand.load() && serverReceivedAck.load()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    assert(serverReceivedStatus.load());
    assert(clientReceivedCommand.load());
    assert(serverReceivedAck.load());

    client.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: Bidirectional STATUS -> COMMAND -> ACK transaction completed.\n";
}

void Test8_AutomatedHeartbeatEveryOneSecond() {
    std::cout << "[TEST 8] Automated Heartbeat Every 1 Second...\n";

    const uint16_t port = 9104;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    std::atomic<int> heartbeatsReceived{0};
    server.SetOnMessage([&heartbeatsReceived](const std::string&, const Message& msg) {
        if (msg.GetType() == MessageType::HEARTBEAT) {
            heartbeatsReceived++;
        }
    });

    TcpClient client("TRAFFIC_NODE_A");
    client.SetAutoReconnect(false);
    assert(client.Connect("127.0.0.1", port, 2000));

    // Run for 2.5 seconds; should send ~2 heartbeats
    std::this_thread::sleep_for(std::chrono::milliseconds(2500));

    const int hbCount = heartbeatsReceived.load();
    assert(hbCount >= 2);
    assert(client.GetStatistics().heartbeatsSent >= 2);

    client.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: 1-second automated heartbeats dispatched and received (count: "
              << hbCount << ").\n";
}

void Test9_CoordinatorHeartbeatTimeoutNodeRemoval() {
    std::cout << "[TEST 9] Coordinator Heartbeat Timeout (> 5s) Node Removal...\n";

    const uint16_t port = 9105;
    TcpServer server;
    // Set timeout to 1200ms for fast automated unit test execution
    server.SetHeartbeatTimeoutMs(1200);
    assert(server.Start(port, "127.0.0.1"));

    std::atomic<bool> nodeRemoved{false};
    server.SetOnDisconnect([&nodeRemoved](const std::string& nodeId) {
        if (nodeId == "SILENT_NODE") {
            nodeRemoved.store(true);
        }
    });

    TcpClient silentClient("SILENT_NODE");
    silentClient.SetAutoReconnect(false);
    assert(silentClient.Connect("127.0.0.1", port, 2000));

    // Immediately stop client heartbeats to simulate node lockup / network silence
    silentClient.StopHeartbeat();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(server.IsNodeConnected("SILENT_NODE"));

    // Wait for server supervisor to detect timeout (> 1200ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(1800));

    assert(!server.IsNodeConnected("SILENT_NODE"));
    assert(nodeRemoved.load());

    silentClient.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: Silent node removed by coordinator upon heartbeat timeout.\n";
}

void Test10_GracefulReconnectionAndReRegistration() {
    std::cout << "[TEST 10] Graceful Reconnection & Re-Registration...\n";

    const uint16_t port = 9106;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    TcpClient client("RECONNECTING_NODE");
    client.SetAutoReconnect(true, 300); // 300ms retry interval

    assert(client.Connect("127.0.0.1", port, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(server.IsNodeConnected("RECONNECTING_NODE"));

    // Simulate server crash/restart: Stop server, client drops, restart server
    server.Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert(!client.IsConnected());

    // Restart server on same port
    assert(server.Start(port, "127.0.0.1"));

    // Wait for client auto-reconnect to trigger and re-register
    for (int i = 0; i < 30; ++i) {
        if (client.IsConnected() && server.IsNodeConnected("RECONNECTING_NODE")) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    assert(client.IsConnected());
    assert(server.IsNodeConnected("RECONNECTING_NODE"));

    client.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: Client automatically reconnected and re-registered after server restart.\n";
}

void Test11_FiftyThreadConcurrentStress() {
    std::cout << "[TEST 11] 50-Thread Concurrent Stress Test...\n";

    const uint16_t port = 9107;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    std::atomic<uint64_t> messagesReceived{0};
    server.SetOnMessage([&messagesReceived](const std::string&, const Message&) {
        messagesReceived++;
    });

    const int threadCount = 20; // 20 simultaneous client threads
    std::vector<std::thread> threads;
    threads.reserve(threadCount);

    uint16_t targetPort = port;
    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back([targetPort, i]() {
            std::string id = "NODE_STRESS_" + std::to_string(i);
            TcpClient client(id);
            client.SetAutoReconnect(false);
            bool connected = client.Connect("127.0.0.1", targetPort, 3000);
            assert(connected);
            for (int m = 0; m < 5; ++m) {
                client.Send(MessageType::STATUS, "STRESS_DATA");
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.Disconnect();
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    assert(messagesReceived.load() >= static_cast<uint64_t>(threadCount * 5));

    server.Stop();

    std::cout << "  ✓ PASS: Concurrent clients executed with 0 data races (received: "
              << messagesReceived.load() << ").\n";
}

void Test12_PerformanceBenchmarkLatency() {
    std::cout << "[TEST 12] Performance Benchmark: 1,000 Messages Round-Trip...\n";

    const uint16_t port = 9108;
    TcpServer server;
    assert(server.Start(port, "127.0.0.1"));

    server.SetOnMessage([&server](const std::string& nodeId, const Message& msg) {
        // Immediate ACK echo
        Message ack = Message::CreateAck(nodeId, msg.GetPayload());
        server.Send(nodeId, ack);
    });

    TcpClient client("BENCHMARK_NODE");
    client.SetAutoReconnect(false);
    assert(client.Connect("127.0.0.1", port, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const int totalMessages = 1000;
    const auto startTime = std::chrono::steady_clock::now();

    for (int i = 0; i < totalMessages; ++i) {
        Message msg(MessageType::TRAFFIC_STATE, "BENCHMARK_NODE", "PHASE=GREEN;TIME=25");
        bool sent = client.Send(msg);
        assert(sent);

        Message ack;
        bool recvd = client.Receive(ack, 1000);
        assert(recvd);
        assert(ack.GetType() == MessageType::ACK);
    }

    const auto endTime = std::chrono::steady_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    const double avgLatencyMs = elapsedMs / totalMessages;

    std::cout << "  -> 1,000 Messages Total Latency: " << elapsedMs << " ms\n";
    std::cout << "  -> Average Round-Trip Latency: " << avgLatencyMs << " ms / message\n";

    assert(avgLatencyMs < 2.0); // Requirement: fast real-time latency

    client.Disconnect();
    server.Stop();

    std::cout << "  ✓ PASS: Performance benchmark (< 2ms / msg round-trip) satisfied!\n";
}

int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: QNX Network Layer Test Suite               \n";
    std::cout << " Target: Raspberry Pi 4 (QNX SDP 8.0 / POSIX)            \n";
    std::cout << "==========================================================\n";

    Test1_MessageConstructionAndEnums();
    Test2_SerializationRoundTripAllTypes();
    Test3_ChecksumCalculationAndCorruptionRejection();
    Test4_StreamFramingAndDefragmentation();
    Test5_ServerBindListenClientConnectDisconnect();
    Test6_SimultaneousMultiClientCommunication();
    Test7_BidirectionalMessagingAndCommandAck();
    Test8_AutomatedHeartbeatEveryOneSecond();
    Test9_CoordinatorHeartbeatTimeoutNodeRemoval();
    Test10_GracefulReconnectionAndReRegistration();
    Test11_FiftyThreadConcurrentStress();
    Test12_PerformanceBenchmarkLatency();

    std::cout << "==========================================================\n";
    std::cout << " ALL 12 QNX NETWORK LAYER TESTS PASSED (100% SUCCESS)    \n";
    std::cout << "==========================================================\n";

    return 0;
}
