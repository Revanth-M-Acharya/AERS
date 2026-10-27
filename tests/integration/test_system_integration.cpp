/**
 * @file test_system_integration.cpp
 * @brief Comprehensive integration tests for the AERS System Integration Layer.
 *
 * Verifies:
 * 1. Coordinator startup sequencing & configuration.
 * 2. Three-node automatic connection, registration ACK, and heartbeat beaconing.
 * 3. Complete Green Corridor execution (Route setup -> Activation -> Command delivery
 *    -> SignalController updates -> Step-by-step progression -> Completion to NORMAL).
 * 4. Fault tolerance & automatic recovery (Node disconnect -> Coordinator offline detection
 *    -> Reconnect -> Automatic re-registration -> Heartbeat resumption).
 * 5. Full corridor preemption with mid-route failover recovery.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

#include "system/SystemManager.h"

using namespace aegis::system;
using namespace aegis::intersection;
using namespace aegis::communication;

void TestCoordinatorStartupAndConfiguration() {
    std::cout << "[TEST] 1. Coordinator Startup Sequencing & Lifecycle..." << std::endl;

    SystemManager::Config config;
    config.coordinatorHost = "127.0.0.1";
    config.coordinatorPort = 19610;
    config.nodeIds = {"INT_A", "INT_B", "INT_C"};

    SystemManager systemMgr(config);
    assert(!systemMgr.IsRunning());

    // Start only coordinator
    assert(systemMgr.StartCoordinator());
    assert(systemMgr.GetCoordinator().IsRunning());

    // Initially registry has no connected nodes
    assert(systemMgr.GetCoordinator().GetIntersectionRegistry().GetOnlineCount() == 0);

    // Health report should report coordinator running but nodes not registered
    SystemHealthReport report = systemMgr.CheckSystemHealth();
    assert(report.coordinatorRunning == true);
    assert(report.totalConfiguredNodes == 3);
    assert(report.onlineNodesCount == 0);
    assert(!report.isFullyHealthy);

    systemMgr.StopCoordinator();
    assert(!systemMgr.GetCoordinator().IsRunning());

    std::cout << "  ✓ Coordinator startup, inspection, and clean shutdown verified." << std::endl;
}

void TestThreeNodeRegistrationAndHeartbeats() {
    std::cout << "[TEST] 2. Three-Node Automatic Connection, Registration ACK, and Heartbeats..." << std::endl;

    SystemManager::Config config;
    config.coordinatorHost = "127.0.0.1";
    config.coordinatorPort = 19611;
    config.nodeIds = {"INT_A", "INT_B", "INT_C"};
    config.heartbeatIntervalMs = 100; // fast 100ms for deterministic testing
    config.startupTimeoutMs = 5000;

    SystemManager systemMgr(config);

    // Full system automated startup:
    // Coordinator starts -> Nodes connect -> REGISTER sent -> ACK received -> Heartbeats start
    assert(systemMgr.StartSystem(config.startupTimeoutMs));
    assert(systemMgr.IsRunning());

    // Verify all 3 nodes are registered and online
    assert(systemMgr.WaitForAllNodesRegistered(3000));

    // Verify individual nodes (Raspberry Pi 2, 3, 4)
    auto* nodeA = systemMgr.GetNodeA();
    auto* nodeB = systemMgr.GetNodeB();
    auto* nodeC = systemMgr.GetNodeC();

    assert(nodeA != nullptr && nodeA->GetIntersectionId() == "INT_A");
    assert(nodeB != nullptr && nodeB->GetIntersectionId() == "INT_B");
    assert(nodeC != nullptr && nodeC->GetIntersectionId() == "INT_C");

    assert(nodeA->IsConnected() && nodeA->IsRegistered());
    assert(nodeB->IsConnected() && nodeB->IsRegistered());
    assert(nodeC->IsConnected() && nodeC->IsRegistered());

    assert(nodeA->GetHeartbeatManager().IsRunning());
    assert(nodeB->GetHeartbeatManager().IsRunning());
    assert(nodeC->GetHeartbeatManager().IsRunning());

    // Check system health report
    SystemHealthReport report = systemMgr.CheckSystemHealth();
    assert(report.coordinatorRunning == true);
    assert(report.totalConfiguredNodes == 3);
    assert(report.registeredNodesCount == 3);
    assert(report.onlineNodesCount == 3);
    assert(report.isFullyHealthy == true);
    assert(systemMgr.IsSystemHealthy());

    // Wait for at least 3 heartbeats to be processed by coordinator
    std::this_thread::sleep_for(std::chrono::milliseconds(350));

    // Verify Coordinator holds recent heartbeat timestamps for all 3 nodes
    aegis::coordinator::IntersectionEntry entryA, entryB, entryC;
    auto& registry = systemMgr.GetCoordinator().GetIntersectionRegistry();
    assert(registry.GetIntersection("INT_A", entryA) && entryA.isOnline && entryA.lastHeartbeatTimestamp > 0);
    assert(registry.GetIntersection("INT_B", entryB) && entryB.isOnline && entryB.lastHeartbeatTimestamp > 0);
    assert(registry.GetIntersection("INT_C", entryC) && entryC.isOnline && entryC.lastHeartbeatTimestamp > 0);

    systemMgr.StopSystem();
    assert(!systemMgr.IsRunning());

    std::cout << "  ✓ Three-node connection, registration ACK, and continuous heartbeats verified." << std::endl;
}

void TestCompleteGreenCorridorExecution() {
    std::cout << "[TEST] 3. Complete Green Corridor Execution & Signal Phasing..." << std::endl;

    SystemManager::Config config;
    config.coordinatorHost = "127.0.0.1";
    config.coordinatorPort = 19612;
    config.nodeIds = {"INT_A", "INT_B", "INT_C"};
    config.heartbeatIntervalMs = 100;

    SystemManager systemMgr(config);
    assert(systemMgr.StartSystem(5000));

    auto* nodeA = systemMgr.GetNodeA();
    auto* nodeB = systemMgr.GetNodeB();
    auto* nodeC = systemMgr.GetNodeC();

    // Verify initial idle state: all nodes default to NORMAL
    assert(nodeA->GetSignalController().IsNormal());
    assert(nodeB->GetSignalController().IsNormal());
    assert(nodeC->GetSignalController().IsNormal());

    // 1. Create Route: INT_A -> INT_B -> INT_C
    assert(systemMgr.CreateCorridorRoute("CORR_AMB_01", {"INT_A", "INT_B", "INT_C"}));

    // 2. Activate Corridor:
    // Coordinator sets active = INT_A (GREEN), INT_B and INT_C (RED)
    // Commands dispatched over TCP -> SignalControllers update
    assert(systemMgr.ActivateCorridor());
    assert(systemMgr.VerifyCorridorPhasing("INT_A", {"INT_B", "INT_C"}, 2000));
    assert(nodeA->GetSignalController().CurrentState() == SignalState::GREEN);
    assert(nodeB->GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeC->GetSignalController().CurrentState() == SignalState::RED);

    // 3. Step 1: Advance Corridor (Ambulance reaches INT_B)
    // INT_B becomes GREEN; INT_A and INT_C become RED
    assert(systemMgr.AdvanceCorridor());
    assert(systemMgr.VerifyCorridorPhasing("INT_B", {"INT_A", "INT_C"}, 2000));
    assert(nodeA->GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeB->GetSignalController().CurrentState() == SignalState::GREEN);
    assert(nodeC->GetSignalController().CurrentState() == SignalState::RED);

    // 4. Step 2: Advance Corridor (Ambulance reaches INT_C)
    // INT_C becomes GREEN; INT_A and INT_B become RED
    assert(systemMgr.AdvanceCorridor());
    assert(systemMgr.VerifyCorridorPhasing("INT_C", {"INT_A", "INT_B"}, 2000));
    assert(nodeA->GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeB->GetSignalController().CurrentState() == SignalState::RED);
    assert(nodeC->GetSignalController().CurrentState() == SignalState::GREEN);

    // 5. Complete Corridor (Mission complete, reset all signals to NORMAL)
    assert(systemMgr.CompleteCorridor());
    assert(systemMgr.VerifyAllNodesNormal(2000));
    assert(nodeA->GetSignalController().CurrentState() == SignalState::NORMAL);
    assert(nodeB->GetSignalController().CurrentState() == SignalState::NORMAL);
    assert(nodeC->GetSignalController().CurrentState() == SignalState::NORMAL);

    systemMgr.StopSystem();

    std::cout << "  ✓ Route creation, activation, step progression, and normal completion verified." << std::endl;
}

void TestIntersectionNodeDisconnectAndReconnectRecovery() {
    std::cout << "[TEST] 4. Fault Tolerance: Node Disconnect & Automatic Reconnect Recovery..." << std::endl;

    SystemManager::Config config;
    config.coordinatorHost = "127.0.0.1";
    config.coordinatorPort = 19613;
    config.nodeIds = {"INT_A", "INT_B", "INT_C"};
    config.heartbeatIntervalMs = 100;

    SystemManager systemMgr(config);
    assert(systemMgr.StartSystem(5000));
    assert(systemMgr.IsSystemHealthy());

    // 1. Simulate network drop on Intersection Node B (Raspberry Pi 3)
    assert(systemMgr.DisconnectNode("INT_B"));

    // 2. Verify Coordinator detects drop and marks Node B offline
    assert(systemMgr.WaitForNodeOffline("INT_B", 3000));

    auto* nodeB = systemMgr.GetNodeB();
    assert(!nodeB->IsConnected());
    assert(!nodeB->IsRegistered());
    assert(!nodeB->GetHeartbeatManager().IsRunning());

    // Verify Nodes A and C remain online and fully healthy
    auto* nodeA = systemMgr.GetNodeA();
    auto* nodeC = systemMgr.GetNodeC();
    assert(nodeA->IsConnected() && nodeA->IsRegistered());
    assert(nodeC->IsConnected() && nodeC->IsRegistered());

    // System health report should reflect degraded status
    SystemHealthReport reportDegraded = systemMgr.CheckSystemHealth();
    assert(reportDegraded.coordinatorRunning == true);
    assert(reportDegraded.totalConfiguredNodes == 3);
    assert(reportDegraded.onlineNodesCount == 2);
    assert(!reportDegraded.isFullyHealthy);

    // 3. Reconnect Node B
    // Automatic re-registration handshake should execute, ACK should be received,
    // and heartbeat should automatically resume
    assert(systemMgr.ReconnectNode("INT_B", 3000));
    assert(systemMgr.WaitForNodeOnline("INT_B", 3000));

    assert(nodeB->IsConnected());
    assert(nodeB->IsRegistered());
    assert(nodeB->GetHeartbeatManager().IsRunning());

    // Wait for fresh heartbeats
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // Verify full system health is restored
    assert(systemMgr.IsSystemHealthy());
    SystemHealthReport reportRestored = systemMgr.CheckSystemHealth();
    assert(reportRestored.isFullyHealthy == true);
    assert(reportRestored.onlineNodesCount == 3);
    assert(reportRestored.registeredNodesCount == 3);

    systemMgr.StopSystem();

    std::cout << "  ✓ Disconnection detection, offline status, re-registration, and heartbeat resumption verified." << std::endl;
}

void TestFullCorridorPreemptionWithFailoverRecovery() {
    std::cout << "[TEST] 5. Full Corridor Preemption with Mid-Route Failover Recovery..." << std::endl;

    SystemManager::Config config;
    config.coordinatorHost = "127.0.0.1";
    config.coordinatorPort = 19614;
    config.nodeIds = {"INT_A", "INT_B", "INT_C"};
    config.heartbeatIntervalMs = 100;

    SystemManager systemMgr(config);
    assert(systemMgr.StartSystem(5000));

    // Set up and activate corridor
    assert(systemMgr.CreateCorridorRoute("CORR_FAILOVER", {"INT_A", "INT_B", "INT_C"}));
    assert(systemMgr.ActivateCorridor());
    assert(systemMgr.VerifyCorridorPhasing("INT_A", {"INT_B", "INT_C"}, 2000));

    // Mid-operation failover: Node A drops while active
    assert(systemMgr.DisconnectNode("INT_A"));
    assert(systemMgr.WaitForNodeOffline("INT_A", 3000));

    // Reconnect Node A
    assert(systemMgr.ReconnectNode("INT_A", 3000));
    assert(systemMgr.WaitForNodeOnline("INT_A", 3000));

    // Advance corridor to Node B
    assert(systemMgr.AdvanceCorridor());
    assert(systemMgr.VerifyCorridorPhasing("INT_B", {"INT_A", "INT_C"}, 2000));

    // Complete corridor
    assert(systemMgr.CompleteCorridor());
    assert(systemMgr.VerifyAllNodesNormal(2000));

    systemMgr.StopSystem();

    std::cout << "  ✓ Mid-route failover recovery and seamless corridor progression verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: System Integration Layer Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestCoordinatorStartupAndConfiguration();
    TestThreeNodeRegistrationAndHeartbeats();
    TestCompleteGreenCorridorExecution();
    TestIntersectionNodeDisconnectAndReconnectRecovery();
    TestFullCorridorPreemptionWithFailoverRecovery();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] System Integration Layer Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
