/**
 * @file test_communication_module.cpp
 * @brief Comprehensive unit tests for the reusable AERS communication module.
 *
 * Verifies Message packet serialization, framing, CRC16 verification, stream
 * parsing, JSON conversion, SocketServer multi-client non-blocking networking,
 * Broadcast, SendToClient, SocketClient transmission, synchronous Receive,
 * and automatic reconnection.
 */

#include <iostream>
#include <cassert>
#include <cstring>
#include <chrono>
#include <thread>
#include <atomic>
#include "communication/Message.h"
#include "communication/SocketServer.h"
#include "communication/SocketClient.h"

using namespace aegis::communication;

void TestMessagePacketStructure() {
    std::cout << "[TEST] 1. Message Structure & Enum Conversions..." << std::endl;

    assert(std::string(MessageTypeToString(MessageType::REGISTER)) == "REGISTER");
    assert(std::string(MessageTypeToString(MessageType::HEARTBEAT)) == "HEARTBEAT");
    assert(std::string(MessageTypeToString(MessageType::SIGNAL_STATUS)) == "SIGNAL_STATUS");
    assert(std::string(MessageTypeToString(MessageType::COMMAND)) == "COMMAND");
    assert(std::string(MessageTypeToString(MessageType::ACK)) == "ACK");
    assert(std::string(MessageTypeToString(MessageType::ERROR)) == "ERROR");

    assert(StringToMessageType("REGISTER") == MessageType::REGISTER);
    assert(StringToMessageType("HEARTBEAT") == MessageType::HEARTBEAT);
    assert(StringToMessageType("SIGNAL_STATUS") == MessageType::SIGNAL_STATUS);
    assert(StringToMessageType("COMMAND") == MessageType::COMMAND);
    assert(StringToMessageType("ACK") == MessageType::ACK);
    assert(StringToMessageType("ERROR") == MessageType::ERROR);

    Message msg(MessageType::COMMAND, "COORD_01", "{\"action\":\"PREEMPT\",\"intersectionId\":\"INT_101\"}", 1700000000123ULL);
    assert(msg.GetType() == MessageType::COMMAND);
    assert(msg.GetSenderId() == "COORD_01");
    assert(msg.GetPayload() == "{\"action\":\"PREEMPT\",\"intersectionId\":\"INT_101\"}");
    assert(msg.GetTimestamp() == 1700000000123ULL);

    msg.SetType(MessageType::ACK);
    msg.SetSenderId("INT_101");
    msg.SetPayload("{\"status\":\"PREEMPTED\"}");
    msg.SetTimestamp(1700000000456ULL);

    assert(msg.GetType() == MessageType::ACK);
    assert(msg.GetSenderId() == "INT_101");
    assert(msg.GetPayload() == "{\"status\":\"PREEMPTED\"}");
    assert(msg.GetTimestamp() == 1700000000456ULL);

    std::cout << "  ✓ Message structure and enum conversions verified." << std::endl;
}

void TestSerializationAndFraming() {
    std::cout << "[TEST] 2. Message Binary Framing & Checksum..." << std::endl;

    MessageType allTypes[] = {
        MessageType::REGISTER,
        MessageType::HEARTBEAT,
        MessageType::SIGNAL_STATUS,
        MessageType::COMMAND,
        MessageType::ACK,
        MessageType::ERROR
    };

    for (auto type : allTypes) {
        Message original(type, "NODE_TEST_42", "Arbitrary Payload Data 12345!@#$%", 1698765432100ULL);
        std::vector<uint8_t> wireData = original.Serialize();

        assert(wireData.size() == Message::HEADER_SIZE + original.GetSenderId().size() + original.GetPayload().size());
        assert(wireData[0] == Message::MAGIC_BYTE_0);
        assert(wireData[1] == Message::MAGIC_BYTE_1);

        Message deserialized;
        bool ok = Message::Deserialize(wireData, deserialized);
        assert(ok);
        assert(deserialized.GetType() == original.GetType());
        assert(deserialized.GetSenderId() == original.GetSenderId());
        assert(deserialized.GetPayload() == original.GetPayload());
        assert(deserialized.GetTimestamp() == original.GetTimestamp());

        // Checksum corruption test: Flip one bit in payload
        std::vector<uint8_t> corruptedData = wireData;
        corruptedData.back() ^= 0x01;
        Message rejectedMsg;
        bool rejected = Message::Deserialize(corruptedData, rejectedMsg);
        assert(!rejected); // Must be rejected due to CRC mismatch!
    }

    std::cout << "  ✓ Binary framing and CRC16 verification passed for all message types." << std::endl;
}

void TestStreamAccumulatorParseNext() {
    std::cout << "[TEST] 3. Stream Accumulator & TCP Defragmentation..." << std::endl;

    Message msg1(MessageType::REGISTER, "AMB_01", "Payload One", 1000);
    Message msg2(MessageType::HEARTBEAT, "AMB_01", "Payload Two", 2000);

    std::vector<uint8_t> bytes1 = msg1.Serialize();
    std::vector<uint8_t> bytes2 = msg2.Serialize();

    std::vector<uint8_t> streamBuffer;

    // Add initial leading noise/garbage before valid packet
    streamBuffer.push_back(0xFF);
    streamBuffer.push_back(0x00);
    streamBuffer.push_back(0x7A);

    // Feed partial first packet (fragment)
    size_t splitPoint = bytes1.size() / 2;
    streamBuffer.insert(streamBuffer.end(), bytes1.begin(), bytes1.begin() + static_cast<ptrdiff_t>(splitPoint));

    Message parsed;
    assert(!Message::ParseNext(streamBuffer, parsed)); // Fragment should return false

    // Feed remainder of packet 1 and entire packet 2 together
    streamBuffer.insert(streamBuffer.end(), bytes1.begin() + static_cast<ptrdiff_t>(splitPoint), bytes1.end());
    streamBuffer.insert(streamBuffer.end(), bytes2.begin(), bytes2.end());

    // Packet 1 should now parse
    assert(Message::ParseNext(streamBuffer, parsed));
    assert(parsed.GetType() == MessageType::REGISTER);
    assert(parsed.GetSenderId() == "AMB_01");
    assert(parsed.GetPayload() == "Payload One");

    // Packet 2 should immediately parse from the remaining buffer
    assert(Message::ParseNext(streamBuffer, parsed));
    assert(parsed.GetType() == MessageType::HEARTBEAT);
    assert(parsed.GetSenderId() == "AMB_01");
    assert(parsed.GetPayload() == "Payload Two");

    // Buffer should now be empty
    assert(streamBuffer.empty());
    assert(!Message::ParseNext(streamBuffer, parsed));

    std::cout << "  ✓ Stream parsing handles fragmentation, batching, and leading noise cleanly." << std::endl;
}

void TestJsonSerialization() {
    std::cout << "[TEST] 4. JSON Serialization..." << std::endl;

    Message original(MessageType::SIGNAL_STATUS, "RSU_INT_07", "{\"state\":\"GREEN\",\"elapsed\":15}", 1712345678900ULL);
    std::string jsonStr = original.ToJson();
    assert(!jsonStr.empty());

    Message parsed;
    bool ok = Message::FromJson(jsonStr, parsed);
    assert(ok);
    assert(parsed.GetType() == MessageType::SIGNAL_STATUS);
    assert(parsed.GetSenderId() == "RSU_INT_07");
    assert(parsed.GetPayload() == "{\"state\":\"GREEN\",\"elapsed\":15}");
    assert(parsed.GetTimestamp() == 1712345678900ULL);

    std::cout << "  ✓ JSON serialization and deserialization verified." << std::endl;
}

void TestSocketServerAndClientNetwork() {
    std::cout << "[TEST] 5. SocketServer & SocketClient TCP Networking..." << std::endl;

    const uint16_t testPort = 19488;
    SocketServer server;
    bool started = server.Start(testPort, "127.0.0.1");
    assert(started);
    assert(server.IsRunning());
    assert(server.GetPort() == testPort);

    std::atomic<bool> serverReceivedRegister{false};
    std::atomic<int> registeredClientId{-1};
    Message receivedFromClient;

    server.SetMessageHandler([&](int clientId, const Message& msg) {
        if (msg.GetType() == MessageType::REGISTER) {
            serverReceivedRegister.store(true);
            registeredClientId.store(clientId);
            receivedFromClient = msg;

            // Send ACK back
            Message ackMsg(MessageType::ACK, "COORD_SERVER", "REGISTER_OK");
            server.SendToClient(clientId, ackMsg);
        }
    });

    SocketClient client1;
    bool connected = client1.Connect("127.0.0.1", testPort, 3000);
    assert(connected);
    assert(client1.IsConnected());

    // Give server worker loop a brief moment to accept
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(server.GetClientCount() == 1);

    // Client sends REGISTER
    Message regMsg(MessageType::REGISTER, "AMB_NODE_01", "VEHICLE_CLASS=AMBULANCE");
    bool sent = client1.Send(regMsg);
    assert(sent);

    // Wait for server to receive
    int retries = 50;
    while (!serverReceivedRegister.load() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(serverReceivedRegister.load());
    assert(receivedFromClient.GetSenderId() == "AMB_NODE_01");
    assert(receivedFromClient.GetPayload() == "VEHICLE_CLASS=AMBULANCE");

    // Client synchronously receives ACK
    Message ackFromLeader;
    bool ackReceived = client1.Receive(ackFromLeader, 2000);
    assert(ackReceived);
    assert(ackFromLeader.GetType() == MessageType::ACK);
    assert(ackFromLeader.GetSenderId() == "COORD_SERVER");
    assert(ackFromLeader.GetPayload() == "REGISTER_OK");

    // Connect second client to test Broadcast
    SocketClient client2;
    assert(client2.Connect("127.0.0.1", testPort, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(server.GetClientCount() == 2);

    // Server broadcasts COMMAND to all clients
    Message broadcastCmd(MessageType::COMMAND, "COORD_SERVER", "GLOBAL_SYNC_TICK");
    bool bcastOk = server.Broadcast(broadcastCmd);
    assert(bcastOk);

    Message bcastRx1;
    assert(client1.Receive(bcastRx1, 2000));
    assert(bcastRx1.GetType() == MessageType::COMMAND);
    assert(bcastRx1.GetPayload() == "GLOBAL_SYNC_TICK");

    Message bcastRx2;
    assert(client2.Receive(bcastRx2, 2000));
    assert(bcastRx2.GetType() == MessageType::COMMAND);
    assert(bcastRx2.GetPayload() == "GLOBAL_SYNC_TICK");

    client2.Disconnect();
    assert(!client2.IsConnected());
    client1.Disconnect();
    assert(!client1.IsConnected());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(server.GetClientCount() == 0);

    server.Stop();
    assert(!server.IsRunning());

    std::cout << "  ✓ TCP server multi-client accept, SendToClient, and Broadcast verified." << std::endl;
}

void TestSocketClientAutoReconnect() {
    std::cout << "[TEST] 6. SocketClient Automatic Reconnection..." << std::endl;

    const uint16_t testPort = 19489;
    auto server = std::make_unique<SocketServer>();
    assert(server->Start(testPort, "127.0.0.1"));

    SocketClient client;
    client.SetAutoReconnect(true, 100, 10); // fast reconnect every 100ms
    assert(client.Connect("127.0.0.1", testPort, 2000));
    assert(client.IsConnected());

    // Stop server to simulate network / server drop
    server->Stop();
    server.reset();

    // Give client thread brief moment to notice drop
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    assert(!client.IsConnected());

    // Restart server on same port
    server = std::make_unique<SocketServer>();
    assert(server->Start(testPort, "127.0.0.1"));

    // Wait for client to automatically reconnect
    int retries = 50;
    while (!client.IsConnected() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(client.IsConnected());

    // Verify transmission resumes normally after reconnect
    std::atomic<bool> heartbeatReceived{false};
    server->SetMessageHandler([&](int, const Message& msg) {
        if (msg.GetType() == MessageType::HEARTBEAT) {
            heartbeatReceived.store(true);
        }
    });

    Message hbMsg(MessageType::HEARTBEAT, "RECONNECTED_CLIENT", "ALIVE");
    assert(client.Send(hbMsg));

    retries = 30;
    while (!heartbeatReceived.load() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(heartbeatReceived.load());

    client.Disconnect();
    server->Stop();

    std::cout << "  ✓ Automatic reconnection and post-recovery messaging verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Reusable Communication Module Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestMessagePacketStructure();
    TestSerializationAndFraming();
    TestStreamAccumulatorParseNext();
    TestJsonSerialization();
    TestSocketServerAndClientNetwork();
    TestSocketClientAutoReconnect();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Reusable Communication Module Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
