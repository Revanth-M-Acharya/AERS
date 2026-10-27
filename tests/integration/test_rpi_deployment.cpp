/**
 * @file test_rpi_deployment.cpp
 * @brief Integration tests for Raspberry Pi Deployment (Phase 5A).
 *
 * Verifies:
 * 1. Configuration loading from JSON files for all 4 Raspberry Pis (RPi 1-4).
 * 2. Automated startup sequencing: Coordinator starts first, then Intersection Nodes connect.
 * 3. Registration handshake, ACK confirmation, and continuous keepalive heartbeats.
 * 4. Signal preemption command delivery over deployed node configurations.
 * 5. Configuration serialization, deserialization, and error handling.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>

#include "config/NodeConfig.h"
#include "coordinator/CoordinatorManager.h"
#include "intersection/IntersectionNode.h"

using namespace aegis::config;
using namespace aegis::coordinator;
using namespace aegis::intersection;

void TestConfigurationLoading() {
    std::cout << "[TEST] 1. Loading Configuration Files for Raspberry Pis 1, 2, 3, 4..." << std::endl;

    // 1. Raspberry Pi 1: Central Coordinator (QNX RTOS)
    NodeConfig rpi1;
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi1_coordinator.json", rpi1));
    assert(rpi1.IsValid());
    assert(rpi1.nodeId == "COORDINATOR");
    assert(rpi1.role == NodeRole::COORDINATOR);
    assert(rpi1.port == 9000);
    assert(rpi1.coordinatorPort == 9000);
    assert(rpi1.heartbeatIntervalMs == 1000);
    assert(rpi1.autoReconnect == true);

    // 2. Raspberry Pi 2: Intersection Node A (INT_A)
    NodeConfig rpi2;
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi2_intersection_a.json", rpi2));
    assert(rpi2.IsValid());
    assert(rpi2.nodeId == "INT_A");
    assert(rpi2.role == NodeRole::INTERSECTION);
    assert(rpi2.port == 9001);
    assert(rpi2.coordinatorPort == 9000);
    assert(rpi2.heartbeatIntervalMs == 1000);

    // 3. Raspberry Pi 3: Intersection Node B (INT_B)
    NodeConfig rpi3;
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi3_intersection_b.json", rpi3));
    assert(rpi3.IsValid());
    assert(rpi3.nodeId == "INT_B");
    assert(rpi3.role == NodeRole::INTERSECTION);
    assert(rpi3.port == 9002);
    assert(rpi3.coordinatorPort == 9000);

    // 4. Raspberry Pi 4: Intersection Node C (INT_C)
    NodeConfig rpi4;
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi4_intersection_c.json", rpi4));
    assert(rpi4.IsValid());
    assert(rpi4.nodeId == "INT_C");
    assert(rpi4.role == NodeRole::INTERSECTION);
    assert(rpi4.port == 9003);
    assert(rpi4.coordinatorPort == 9000);

    // 5. Negative Test: Non-existent file
    NodeConfig invalidCfg;
    assert(!NodeConfigLoader::LoadFromFile("config/deployment/non_existent.json", invalidCfg));

    // 6. Roundtrip serialization test
    std::string serialized = NodeConfigLoader::Serialize(rpi2);
    NodeConfig deserialized;
    assert(NodeConfigLoader::LoadFromString(serialized, deserialized));
    assert(deserialized.nodeId == "INT_A");
    assert(deserialized.role == NodeRole::INTERSECTION);
    assert(deserialized.port == 9001);
    assert(deserialized.coordinatorPort == 9000);

    std::cout << "  ✓ All 4 Raspberry Pi configuration files loaded and validated successfully." << std::endl;
}

void TestDeploymentStartupSequencing() {
    std::cout << "[TEST] 2. Automated Multi-Pi Startup Sequencing & Registration..." << std::endl;

    const uint16_t testPort = 19620;

    // Load configs and set test port for loopback execution
    NodeConfig coordCfg, nodeACfg, nodeBCfg, nodeCCfg;
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi1_coordinator.json", coordCfg));
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi2_intersection_a.json", nodeACfg));
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi3_intersection_b.json", nodeBCfg));
    assert(NodeConfigLoader::LoadFromFile("config/deployment/rpi4_intersection_c.json", nodeCCfg));

    coordCfg.port = testPort;
    coordCfg.ipAddress = "127.0.0.1";

    nodeACfg.coordinatorPort = testPort;
    nodeACfg.coordinatorIp = "127.0.0.1";
    nodeACfg.heartbeatIntervalMs = 100;

    nodeBCfg.coordinatorPort = testPort;
    nodeBCfg.coordinatorIp = "127.0.0.1";
    nodeBCfg.heartbeatIntervalMs = 100;

    nodeCCfg.coordinatorPort = testPort;
    nodeCCfg.coordinatorIp = "127.0.0.1";
    nodeCCfg.heartbeatIntervalMs = 100;

    // 1. Coordinator (RPi 1) MUST start first
    CoordinatorManager coordinator;
    assert(!coordinator.IsRunning());
    assert(coordinator.Start(coordCfg.port, coordCfg.ipAddress));
    assert(coordinator.IsRunning());
    assert(coordinator.GetIntersectionRegistry().GetOnlineCount() == 0);

    // 2. Launch Raspberry Pi 2 (INT_A)
    IntersectionNode nodeA(nodeACfg.nodeId, nodeACfg.heartbeatIntervalMs);
    assert(nodeA.Start(nodeACfg.coordinatorIp, nodeACfg.coordinatorPort, nodeACfg.connectTimeoutMs, nodeACfg.ackTimeoutMs));
    assert(nodeA.IsConnected());
    assert(nodeA.IsRegistered());
    assert(nodeA.GetHeartbeatManager().IsRunning());

    // 3. Launch Raspberry Pi 3 (INT_B)
    IntersectionNode nodeB(nodeBCfg.nodeId, nodeBCfg.heartbeatIntervalMs);
    assert(nodeB.Start(nodeBCfg.coordinatorIp, nodeBCfg.coordinatorPort, nodeBCfg.connectTimeoutMs, nodeBCfg.ackTimeoutMs));
    assert(nodeB.IsConnected());
    assert(nodeB.IsRegistered());
    assert(nodeB.GetHeartbeatManager().IsRunning());

    // 4. Launch Raspberry Pi 4 (INT_C)
    IntersectionNode nodeC(nodeCCfg.nodeId, nodeCCfg.heartbeatIntervalMs);
    assert(nodeC.Start(nodeCCfg.coordinatorIp, nodeCCfg.coordinatorPort, nodeCCfg.connectTimeoutMs, nodeCCfg.ackTimeoutMs));
    assert(nodeC.IsConnected());
    assert(nodeC.IsRegistered());
    assert(nodeC.GetHeartbeatManager().IsRunning());

    // 5. Verify Coordinator has recorded all 3 nodes online
    int retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().GetOnlineCount() == 3);

    IntersectionEntry entryA, entryB, entryC;
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_A", entryA) && entryA.isOnline);
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_B", entryB) && entryB.isOnline);
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_C", entryC) && entryC.isOnline);

    // 6. Verify keepalive heartbeats are arriving
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_A", entryA) && entryA.lastHeartbeatTimestamp > 0);
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_B", entryB) && entryB.lastHeartbeatTimestamp > 0);
    assert(coordinator.GetIntersectionRegistry().GetIntersection("INT_C", entryC) && entryC.lastHeartbeatTimestamp > 0);

    // Clean teardown
    nodeA.Stop();
    nodeB.Stop();
    nodeC.Stop();
    coordinator.Stop();

    std::cout << "  ✓ Startup sequencing, automatic connection, ACK, and heartbeats verified." << std::endl;
}

void TestCorridorCommandDeliveryOverDeploymentConfig() {
    std::cout << "[TEST] 3. Corridor Signal Control Over Deployed Nodes..." << std::endl;

    const uint16_t testPort = 19621;

    CoordinatorManager coordinator;
    assert(coordinator.Start(testPort, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 100);
    IntersectionNode nodeB("INT_B", 100);
    IntersectionNode nodeC("INT_C", 100);

    assert(nodeA.Start("127.0.0.1", testPort));
    assert(nodeB.Start("127.0.0.1", testPort));
    assert(nodeC.Start("127.0.0.1", testPort));

    int retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    assert(coordinator.GetIntersectionRegistry().GetOnlineCount() == 3);

    // Setup and trigger Green Corridor
    assert(coordinator.CreateCorridor("AMB_DEPLOY_01", {"INT_A", "INT_B", "INT_C"}));
    assert(coordinator.ActivateCorridor());

    auto cmds = coordinator.PopGeneratedCommands();
    assert(cmds.size() == 3);

    for (const auto& cmd : cmds) {
        std::string action, target;
        assert(TrafficCommandManager::ParseCommand(cmd, action, target));
        assert(coordinator.SendMessageToIntersection(target, cmd));
    }

    retries = 50;
    while ((nodeA.GetSignalController().CurrentState() != aegis::intersection::SignalState::GREEN ||
            nodeB.GetSignalController().CurrentState() != aegis::intersection::SignalState::RED   ||
            nodeC.GetSignalController().CurrentState() != aegis::intersection::SignalState::RED) && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    assert(nodeA.GetSignalController().CurrentState() == aegis::intersection::SignalState::GREEN);
    assert(nodeB.GetSignalController().CurrentState() == aegis::intersection::SignalState::RED);
    assert(nodeC.GetSignalController().CurrentState() == aegis::intersection::SignalState::RED);

    nodeA.Stop();
    nodeB.Stop();
    nodeC.Stop();
    coordinator.Stop();

    std::cout << "  ✓ Corridor preemption commands delivered to deployed nodes successfully." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Raspberry Pi Deployment Test Suite (Phase 5A)" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestConfigurationLoading();
    TestDeploymentStartupSequencing();
    TestCorridorCommandDeliveryOverDeploymentConfig();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL PASS] Raspberry Pi Deployment Verified 100%!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
