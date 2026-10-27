/**
 * @file test_intersection_node.cpp
 * @brief Unit tests for the Raspberry Pi Intersection Node edge software.
 *
 * Verifies:
 * - SignalController software transitions (RED, YELLOW, GREEN, NORMAL, EMERGENCY)
 * - HeartbeatManager periodic generation and callback dispatch
 * - IntersectionNode registration handshake and ACK handling
 * - Automatic heartbeat initiation upon ACK
 * - Coordinator command execution (SET_GREEN, SET_RED, SET_NORMAL, EMERGENCY_ON/OFF)
 * - Multi-node deployment (Intersection Nodes A, B, C)
 * - Automatic failure recovery and reconnection
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

#include "intersection/SignalController.h"
#include "intersection/HeartbeatManager.h"
#include "intersection/IntersectionNode.h"
#include "coordinator/CoordinatorManager.h"

using namespace aegis::intersection;
using namespace aegis::communication;
using aegis::coordinator::CoordinatorManager;
using aegis::coordinator::IntersectionEntry;
using aegis::coordinator::TrafficCommandManager;

void TestSignalController() {
    std::cout << "[TEST] 1. SignalController State Mutations & Emergency Mode..." << std::endl;

    SignalController signal;
    assert(signal.CurrentState() == SignalState::RED);
    assert(signal.IsNormal());
    assert(!signal.IsEmergency());

    // Phase transitions
    signal.SetGreen();
    assert(signal.CurrentState() == SignalState::GREEN);
    assert(signal.StateString() == "GREEN");

    signal.SetYellow();
    assert(signal.CurrentState() == SignalState::YELLOW);
    assert(signal.StateString() == "YELLOW");

    signal.SetRed();
    assert(signal.CurrentState() == SignalState::RED);
    assert(signal.StateString() == "RED");

    // Emergency mode
    signal.EnableEmergency();
    assert(signal.IsEmergency());
    assert(signal.CurrentState() == SignalState::EMERGENCY);

    signal.DisableEmergency();
    assert(!signal.IsEmergency());
    assert(signal.IsNormal());
    assert(signal.CurrentState() == SignalState::NORMAL);

    signal.SetNormal();
    assert(signal.IsNormal());
    assert(!signal.IsEmergency());

    std::cout << "  ✓ SignalController transitions and emergency modes verified." << std::endl;
}

void TestHeartbeatManager() {
    std::cout << "[TEST] 2. HeartbeatManager Periodic Generation..." << std::endl;

    HeartbeatManager hbMgr("INT_RPI_02", 50); // fast 50ms interval for test
    assert(hbMgr.GetIntersectionId() == "INT_RPI_02");
    assert(hbMgr.GetIntervalMs() == 50);
    assert(!hbMgr.IsRunning());

    // Instantaneous generation
    Message instantMsg = hbMgr.GenerateHeartbeat();
    assert(instantMsg.GetType() == MessageType::HEARTBEAT);
    assert(instantMsg.GetSenderId() == "INT_RPI_02");
    assert(instantMsg.GetPayload() == "HEARTBEAT");

    // Periodic worker test
    std::atomic<int> receivedCount{0};
    assert(hbMgr.Start([&](const Message& msg) {
        assert(msg.GetType() == MessageType::HEARTBEAT);
        assert(msg.GetSenderId() == "INT_RPI_02");
        receivedCount.fetch_add(1);
    }));

    assert(hbMgr.IsRunning());

    // Wait for at least 3 heartbeats
    int retries = 50;
    while (receivedCount.load() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(receivedCount.load() >= 3);

    hbMgr.Stop();
    assert(!hbMgr.IsRunning());

    std::cout << "  ✓ HeartbeatManager periodic generation and clean shutdown verified." << std::endl;
}

void TestIntersectionNodeRegistrationAndCommands() {
    std::cout << "[TEST] 3. IntersectionNode Registration, ACK, and Coordinator Commands..." << std::endl;

    const uint16_t testPort = 19580;
    CoordinatorManager coordinator;
    assert(coordinator.Start(testPort, "127.0.0.1"));

    // Create Intersection Node A (Raspberry Pi 2)
    IntersectionNode nodeA("INT_A", 60);
    assert(!nodeA.IsConnected());
    assert(!nodeA.IsRegistered());

    // Start Node A: connects to coordinator, registers, receives ACK, starts heartbeat
    assert(nodeA.Start("127.0.0.1", testPort, 3000, 3000));
    assert(nodeA.IsConnected());
    assert(nodeA.IsRegistered());
    assert(nodeA.GetHeartbeatManager().IsRunning());

    // Verify Coordinator registered Node A
    int retries = 50;
    IntersectionEntry entryA;
    while ((!coordinator.GetIntersectionRegistry().GetIntersection("INT_A", entryA) || !entryA.isOnline) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().HasIntersection("INT_A"));
    assert(entryA.isOnline == true);

    // Coordinator sends SET_GREEN to Node A
    Message greenCmd = coordinator.GetTrafficCommandManager().CreateGreenCommand("INT_A");
    assert(coordinator.SendMessageToIntersection("INT_A", greenCmd));

    retries = 50;
    while (nodeA.GetSignalController().CurrentState() != SignalState::GREEN && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(nodeA.GetSignalController().CurrentState() == SignalState::GREEN);

    // Coordinator sends SET_RED to Node A
    Message redCmd = coordinator.GetTrafficCommandManager().CreateRedCommand("INT_A");
    assert(coordinator.SendMessageToIntersection("INT_A", redCmd));

    retries = 50;
    while (nodeA.GetSignalController().CurrentState() != SignalState::RED && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(nodeA.GetSignalController().CurrentState() == SignalState::RED);

    // Coordinator sends EMERGENCY_MODE_ON
    Message emergOnCmd = coordinator.GetTrafficCommandManager().CreateEmergencyOn("INT_A", "CORR_01");
    assert(coordinator.SendMessageToIntersection("INT_A", emergOnCmd));

    retries = 50;
    while (!nodeA.GetSignalController().IsEmergency() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(nodeA.GetSignalController().IsEmergency());

    // Coordinator sends EMERGENCY_MODE_OFF
    Message emergOffCmd = coordinator.GetTrafficCommandManager().CreateEmergencyOff("INT_A", "CORR_01");
    assert(coordinator.SendMessageToIntersection("INT_A", emergOffCmd));

    retries = 50;
    while (nodeA.GetSignalController().IsEmergency() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(!nodeA.GetSignalController().IsEmergency());

    // Coordinator sends SET_NORMAL
    Message normalCmd = coordinator.GetTrafficCommandManager().CreateNormalCommand("INT_A");
    assert(coordinator.SendMessageToIntersection("INT_A", normalCmd));

    retries = 50;
    while (nodeA.GetSignalController().CurrentState() != SignalState::NORMAL && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(nodeA.GetSignalController().CurrentState() == SignalState::NORMAL);

    nodeA.Stop();
    coordinator.Stop();

    std::cout << "  ✓ Registration handshake, ACK, heartbeat start, and commands verified." << std::endl;
}

void TestMultiIntersectionNodesCorridorDeployment() {
    std::cout << "[TEST] 4. Multi-Node Deployment: Nodes A, B, C (RPi 2, 3, 4)..." << std::endl;

    const uint16_t testPort = 19581;
    CoordinatorManager coordinator;
    assert(coordinator.Start(testPort, "127.0.0.1"));

    // Instantiate Raspberry Pi 2, 3, 4 edge nodes
    IntersectionNode nodeA("INT_A", 50);
    IntersectionNode nodeB("INT_B", 50);
    IntersectionNode nodeC("INT_C", 50);

    assert(nodeA.Start("127.0.0.1", testPort, 3000, 3000));
    assert(nodeB.Start("127.0.0.1", testPort, 3000, 3000));
    assert(nodeC.Start("127.0.0.1", testPort, 3000, 3000));

    // Wait for all 3 nodes to register in Coordinator
    int retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().GetOnlineCount() == 3);

    // Setup and activate Green Corridor across INT_A -> INT_B -> INT_C
    assert(coordinator.CreateCorridor("AMB_MISSION_01", {"INT_A", "INT_B", "INT_C"}));
    assert(coordinator.ActivateCorridor());

    // Coordinator transmits generated commands to active corridor nodes
    auto commands = coordinator.PopGeneratedCommands();
    assert(commands.size() == 3);

    for (const auto& cmd : commands) {
        std::string action, target;
        assert(TrafficCommandManager::ParseCommand(cmd, action, target));
        assert(coordinator.SendMessageToIntersection(target, cmd));
    }

    // Verify: Node A is GREEN, Node B and Node C are RED
    retries = 50;
    while ((nodeA.GetSignalController().CurrentState() != SignalState::GREEN ||
            nodeB.GetSignalController().CurrentState() != SignalState::RED   ||
            nodeC.GetSignalController().CurrentState() != SignalState::RED) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    assert(nodeA.GetSignalController().CurrentState() == SignalState::GREEN);
    assert(nodeB.GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeC.GetSignalController().CurrentState() == SignalState::RED);

    // Advance Corridor: ambulance reaches INT_B
    assert(coordinator.AdvanceCorridor());
    auto advanceCmds = coordinator.PopGeneratedCommands();
    for (const auto& cmd : advanceCmds) {
        std::string action, target;
        assert(TrafficCommandManager::ParseCommand(cmd, action, target));
        assert(coordinator.SendMessageToIntersection(target, cmd));
    }

    // Verify: Node B is GREEN, Node A and Node C are RED
    retries = 50;
    while ((nodeB.GetSignalController().CurrentState() != SignalState::GREEN ||
            nodeA.GetSignalController().CurrentState() != SignalState::RED   ||
            nodeC.GetSignalController().CurrentState() != SignalState::RED) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    assert(nodeB.GetSignalController().CurrentState() == SignalState::GREEN);
    assert(nodeA.GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeC.GetSignalController().CurrentState() == SignalState::RED);

    // Complete Corridor: all nodes return to NORMAL
    assert(coordinator.CompleteCorridor());
    auto completeCmds = coordinator.PopGeneratedCommands();
    for (const auto& cmd : completeCmds) {
        std::string action, target;
        assert(TrafficCommandManager::ParseCommand(cmd, action, target));
        assert(coordinator.SendMessageToIntersection(target, cmd));
    }

    retries = 50;
    while ((nodeA.GetSignalController().CurrentState() != SignalState::NORMAL ||
            nodeB.GetSignalController().CurrentState() != SignalState::NORMAL ||
            nodeC.GetSignalController().CurrentState() != SignalState::NORMAL) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    assert(nodeA.GetSignalController().CurrentState() == SignalState::NORMAL);
    assert(nodeB.GetSignalController().CurrentState() == SignalState::NORMAL);
    assert(nodeC.GetSignalController().CurrentState() == SignalState::NORMAL);

    nodeA.Stop();
    nodeB.Stop();
    nodeC.Stop();
    coordinator.Stop();

    std::cout << "  ✓ Multi-node corridor deployment across Nodes A, B, and C verified." << std::endl;
}

void TestAutomaticReconnectAndRecovery() {
    std::cout << "[TEST] 5. Automatic Reconnect & Failure Recovery..." << std::endl;

    const uint16_t testPort = 19582;
    auto coordinator = std::make_unique<CoordinatorManager>();
    assert(coordinator->Start(testPort, "127.0.0.1"));

    IntersectionNode node("INT_FAILOVER_TEST", 50);
    assert(node.Start("127.0.0.1", testPort, 3000, 3000));
    assert(node.IsConnected());
    assert(node.IsRegistered());

    // Stop coordinator to trigger node disconnect
    coordinator->Stop();
    coordinator.reset();

    // Node detects drop, unregisters and stops heartbeat
    int retries = 50;
    while (node.IsConnected() && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(!node.IsConnected());
    assert(!node.IsRegistered());
    assert(!node.GetHeartbeatManager().IsRunning());

    // Restart coordinator on the exact same port
    coordinator = std::make_unique<CoordinatorManager>();
    assert(coordinator->Start(testPort, "127.0.0.1"));

    // Wait for node's automatic reconnection and re-registration
    retries = 100;
    while ((!node.IsConnected() || !node.IsRegistered()) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    assert(node.IsConnected());
    assert(node.IsRegistered());
    assert(node.GetHeartbeatManager().IsRunning());

    // Verify Coordinator receives new heartbeats
    retries = 50;
    IntersectionEntry entry;
    while ((!coordinator->GetIntersectionRegistry().GetIntersection("INT_FAILOVER_TEST", entry) || !entry.isOnline) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator->GetIntersectionRegistry().HasIntersection("INT_FAILOVER_TEST"));
    assert(entry.isOnline == true);

    node.Stop();
    coordinator->Stop();

    std::cout << "  ✓ Automatic reconnect, re-registration, and heartbeat resumption verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Intersection Node Edge Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestSignalController();
    TestHeartbeatManager();
    TestIntersectionNodeRegistrationAndCommands();
    TestMultiIntersectionNodesCorridorDeployment();
    TestAutomaticReconnectAndRecovery();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Intersection Node Edge Implementation Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
