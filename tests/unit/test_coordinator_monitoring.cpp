/**
 * @file test_coordinator_monitoring.cpp
 * @brief Unit tests for Coordinator runtime logging and node observability (Phase 5A).
 */

#include <iostream>
#include <sstream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>

#include "coordinator/CoordinatorManager.h"
#include "communication/SocketClient.h"
#include "communication/Message.h"

using namespace aegis::coordinator;
using namespace aegis::communication;

void TestInitialNodeTableAndNetworkStatus() {
    std::cout << "[TEST] 1. Initial In-Memory Table and Network Status Format..." << std::endl;

    CoordinatorManager coordinator;
    auto nodes = coordinator.GetConnectedNodes();
    assert(nodes.size() == 5);

    // Verify deterministic initial order
    assert(nodes[0].nodeId == "INT_A");
    assert(nodes[0].role == "Intersection");
    assert(nodes[0].connectionStatus == "OFFLINE");

    assert(nodes[1].nodeId == "INT_B");
    assert(nodes[1].role == "Intersection");
    assert(nodes[1].connectionStatus == "OFFLINE");

    assert(nodes[2].nodeId == "INT_C");
    assert(nodes[2].role == "Intersection");
    assert(nodes[2].connectionStatus == "OFFLINE");

    assert(nodes[3].nodeId == "AMB_01");
    assert(nodes[3].role == "Ambulance");
    assert(nodes[3].connectionStatus == "OFFLINE");

    assert(nodes[4].nodeId == "HOSPITAL");
    assert(nodes[4].role == "Hospital");
    assert(nodes[4].connectionStatus == "OFFLINE");

    // Check offline status string when not running
    std::string offlineStatus = coordinator.GetNetworkStatusString();
    std::string expectedOffline =
        "==============================\n"
        "AERS NETWORK STATUS\n"
        "Coordinator : OFFLINE\n\n"
        "Connected Nodes\n"
        "------------------------------------\n"
        "INT_A    OFFLINE\n"
        "INT_B    OFFLINE\n"
        "INT_C    OFFLINE\n"
        "AMB_01   OFFLINE\n"
        "HOSPITAL OFFLINE\n"
        "------------------------------------\n";
    assert(offlineStatus == expectedOffline);

    std::cout << "  ✓ Initial table and formatting verified." << std::endl;
}

void TestLiveNodeLifecycleAndObservability() {
    std::cout << "[TEST] 2. Live Node Connection, Registration, Heartbeat, and Network Status..." << std::endl;

    const uint16_t testPort = 19688;
    CoordinatorManager coordinator;

    assert(coordinator.Start(testPort, "127.0.0.1"));
    assert(coordinator.IsRunning());

    // 1. Connect INT_A
    SocketClient clientA;
    assert(clientA.Connect("127.0.0.1", testPort, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 2. Register INT_A
    Message regMsg(MessageType::REGISTER, "INT_A", "{\"nodeType\":\"INTERSECTION\"}");
    assert(clientA.Send(regMsg));

    Message ack;
    assert(clientA.Receive(ack, 2000));
    assert(ack.GetType() == MessageType::ACK);
    assert(ack.GetPayload() == "REGISTER_OK");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Verify INT_A is ONLINE in in-memory table
    ConnectedNodeInfo infoA;
    assert(coordinator.GetConnectedNodeInfo("INT_A", infoA));
    assert(infoA.nodeId == "INT_A");
    assert(infoA.role == "Intersection");
    assert(infoA.connectionStatus == "ONLINE");
    assert(infoA.ipAddress == "127.0.0.1");
    assert(infoA.port > 0);
    assert(infoA.lastHeartbeatTimestamp > 0);

    // Verify exact network status string required by Requirement 7:
    std::string expectedStatus =
        "==============================\n"
        "AERS NETWORK STATUS\n"
        "Coordinator : ONLINE\n\n"
        "Connected Nodes\n"
        "------------------------------------\n"
        "INT_A    ONLINE\n"
        "INT_B    OFFLINE\n"
        "INT_C    OFFLINE\n"
        "AMB_01   OFFLINE\n"
        "HOSPITAL OFFLINE\n"
        "------------------------------------\n";

    std::string actualStatus = coordinator.GetNetworkStatusString();
    assert(actualStatus == expectedStatus);

    // Verify free helper function PrintNetworkStatus compiles and executes
    PrintNetworkStatus(coordinator);

    // 3. Heartbeat from INT_A
    uint64_t hbTime = Message::CurrentTimestampMs() + 500;
    Message hbMsg(MessageType::HEARTBEAT, "INT_A", "PING", hbTime);
    assert(clientA.Send(hbMsg));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    assert(coordinator.GetConnectedNodeInfo("INT_A", infoA));
    assert(infoA.lastHeartbeatTimestamp >= hbTime);

    // 4. Disconnect INT_A
    clientA.Disconnect();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    assert(coordinator.GetConnectedNodeInfo("INT_A", infoA));
    assert(infoA.connectionStatus == "OFFLINE");

    // 5. Reconnect INT_A
    SocketClient clientAReconnect;
    assert(clientAReconnect.Connect("127.0.0.1", testPort, 3000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    Message reRegMsg(MessageType::REGISTER, "INT_A", "{\"nodeType\":\"INTERSECTION\"}");
    assert(clientAReconnect.Send(reRegMsg));

    Message reAck;
    assert(clientAReconnect.Receive(reAck, 2000));
    assert(reAck.GetType() == MessageType::ACK);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    assert(coordinator.GetConnectedNodeInfo("INT_A", infoA));
    assert(infoA.connectionStatus == "ONLINE");

    clientAReconnect.Disconnect();
    coordinator.Stop();

    std::cout << "  ✓ Live node connection, registration, heartbeat, disconnect, and reconnect verified." << std::endl;
}

void TestDynamicNodeRegistrationAndRoleResolution() {
    std::cout << "[TEST] 3. Dynamic Node Expansion and Role Parsing..." << std::endl;

    const uint16_t testPort = 19689;
    CoordinatorManager coordinator;
    assert(coordinator.Start(testPort, "127.0.0.1"));

    // Register Ambulance node
    SocketClient ambClient;
    assert(ambClient.Connect("127.0.0.1", testPort, 3000));
    Message ambReg(MessageType::REGISTER, "AMB_01", "{\"role\":\"Ambulance\"}");
    assert(ambClient.Send(ambReg));

    Message ack;
    assert(ambClient.Receive(ack, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    ConnectedNodeInfo ambInfo;
    assert(coordinator.GetConnectedNodeInfo("AMB_01", ambInfo));
    assert(ambInfo.connectionStatus == "ONLINE");
    assert(ambInfo.role == "Ambulance");

    // Register Dynamic New Node not in initial 5
    SocketClient dynamicClient;
    assert(dynamicClient.Connect("127.0.0.1", testPort, 3000));
    Message dynReg(MessageType::REGISTER, "INT_D", "{\"nodeType\":\"INTERSECTION\"}");
    assert(dynamicClient.Send(dynReg));
    assert(dynamicClient.Receive(ack, 2000));
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    ConnectedNodeInfo dynInfo;
    assert(coordinator.GetConnectedNodeInfo("INT_D", dynInfo));
    assert(dynInfo.connectionStatus == "ONLINE");
    assert(dynInfo.role == "Intersection");

    auto allNodes = coordinator.GetConnectedNodes();
    assert(allNodes.size() == 6);
    assert(allNodes.back().nodeId == "INT_D");

    ambClient.Disconnect();
    dynamicClient.Disconnect();
    coordinator.Stop();

    std::cout << "  ✓ Dynamic node expansion and role parsing verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 5A Coordinator Monitoring Unit Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestInitialNodeTableAndNetworkStatus();
    TestLiveNodeLifecycleAndObservability();
    TestDynamicNodeRegistrationAndRoleResolution();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Phase 5A Coordinator Monitoring 100% Verified!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
