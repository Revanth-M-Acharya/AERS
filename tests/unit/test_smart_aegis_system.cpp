/**
 * @file test_smart_aegis_system.cpp
 * @brief Comprehensive Verification and Test Suite for SMART AEGIS Production System.
 *
 * Verifies:
 * 1. Configuration parsing from config/intersections.json
 * 2. GPIO Hardware Abstraction and Node 3 shortage constraint
 * 3. JSON protocol serialization, deserialization, sequence numbers, and CRC16 checksum
 * 4. TCP Client-Server connection and INTERSECTION_READY greeting handshake
 * 5. Command execution and ACK handling (BLUE_ON, RED_OFF, RESET, etc.)
 * 6. Node 3 shortage constraint safe execution and ACK
 * 7. 3-Retry and timeout error handling
 * 8. Periodic 5-second PING and health status transitions
 * 9. Multi-node fault tolerance during node disconnect & automatic 2s reconnection
 * 10. Complete 5-stage Emergency Green Corridor live demonstration flow
 *
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include <iostream>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <thread>
#include <memory>
#include <vector>
#include <csignal>

#include "aegis/config/IntersectionsConfig.hpp"
#include "gpio/IGpioController.hpp"
#include "gpio/SimulationGpioController.hpp"
#include "gpio/QnxHardwareGpioController.hpp"
#include "gpio/GpioFactory.hpp"
#include "protocol/AegisProtocol.hpp"
#include "intersection_service/IntersectionService.hpp"
#include "coordinator_client/CoordinatorClientManager.hpp"

using namespace aegis;

// Test 1: Configuration Loading
void TestConfigurationLoading() {
    std::cout << "[TEST 1] Testing config/intersections.json configuration parser..." << std::endl;
    auto config = config::IntersectionsConfig::LoadFromFile("config/intersections.json");
    assert(config != nullptr && "Config pointer should not be null");
    assert(config->IsValid() && "Config should be valid");

    const auto& system = config->GetSystemSettings();
    assert(system.reconnectIntervalMs == 2000 && "Reconnect interval must be 2000 ms");
    assert(system.pingIntervalMs == 5000 && "Ping interval must be 5000 ms");
    assert(system.commandTimeoutMs == 1000 && "Command timeout must be 1000 ms");
    assert(system.maxRetries == 3 && "Max retries must be 3");
    assert(system.coordinator.ip == "10.204.179.4" && "Coordinator IP must be 10.204.179.4");
    assert(!system.coordinator.hasLeds && "Coordinator must have zero LEDs connected");

    const auto& nodes = config->GetIntersections();
    assert(nodes.size() == 3 && "Cluster must configure exactly 3 intersection nodes");

    // Node 1
    assert(nodes[0].id == 1);
    assert(nodes[0].ip == "10.204.179.245");
    assert(nodes[0].port == 5001);
    assert(nodes[0].gpio.redPin == 5 && nodes[0].gpio.bluePin == 6);
    assert(nodes[0].gpio.hasRed && nodes[0].gpio.hasBlue);

    // Node 2
    assert(nodes[1].id == 2);
    assert(nodes[1].ip == "10.204.179.130");
    assert(nodes[1].port == 5002);
    assert(nodes[1].gpio.redPin == 5 && nodes[1].gpio.bluePin == 6);
    assert(nodes[1].gpio.hasRed && nodes[1].gpio.hasBlue);

    // Node 3 (Shortage constraint: Blue only, no Red)
    assert(nodes[2].id == 3);
    assert(nodes[2].ip == "10.204.179.144");
    assert(nodes[2].port == 5003);
    assert(nodes[2].gpio.bluePin == 6);
    assert(!nodes[2].gpio.hasRed && "Node 3 must have NO Red LED due to hardware shortage");
    assert(nodes[2].gpio.hasBlue && "Node 3 must have Blue LED");

    std::cout << "[PASS] Configuration parser validated successfully." << std::endl;
}

// Test 2: GPIO Abstraction and Node 3 Shortage Constraint
void TestGpioAbstraction() {
    std::cout << "[TEST 2] Testing GPIO Abstraction & Hardware Shortage Handling..." << std::endl;

    // Node 1: Red + Blue
    gpio::SimulationGpioController node1Gpio(1);
    assert(node1Gpio.initialize());
    assert(node1Gpio.hasRedLed());
    assert(node1Gpio.hasBlueLed());
    assert(!node1Gpio.getBlue() && !node1Gpio.getRed());

    assert(node1Gpio.setBlue(true));
    assert(node1Gpio.getBlue());
    assert(node1Gpio.setRed(true));
    assert(node1Gpio.getRed());

    node1Gpio.reset();
    assert(!node1Gpio.getBlue() && !node1Gpio.getRed());
    node1Gpio.shutdown();

    // Node 3: Blue only (No Red)
    gpio::SimulationGpioController node3Gpio(3);
    assert(node3Gpio.initialize());
    assert(!node3Gpio.hasRedLed() && "Node 3 hasRedLed must return false");
    assert(node3Gpio.hasBlueLed());

    assert(node3Gpio.setBlue(true));
    assert(node3Gpio.getBlue());

    // setRed on Node 3 returns false because Red LED is not present (safe rejection / hardware shortage)
    assert(!node3Gpio.setRed(true) && "Node 3 setRed should return false as Red LED is omitted");
    assert(!node3Gpio.getRed() && "Node 3 getRed must always return false");
    assert(!node3Gpio.flashRed() && "Node 3 flashRed should return false");

    node3Gpio.shutdown();
    std::cout << "[PASS] GPIO Abstraction and Shortage Constraint validated successfully." << std::endl;
}

// Test 3: JSON Message Protocol and CRC16 Checksum
void TestProtocolAndChecksum() {
    std::cout << "[TEST 3] Testing AegisProtocol JSON serialization and CRC16..." << std::endl;

    protocol::AegisMessage msg;
    msg.timestamp = "2026-10-31T07:30:00";
    msg.intersection = 2;
    msg.command = std::string(protocol::Commands::BLUE_ON);
    msg.sequence = 101;
    msg.status = "OK";
    msg.details = "Test preemption command";

    std::string json = msg.ToJson();
    assert(!json.empty());
    assert(json.find("\"intersection\":2") != std::string::npos);
    assert(json.find("\"command\":\"BLUE_ON\"") != std::string::npos);
    assert(json.find("\"sequence\":101") != std::string::npos);
    assert(json.find("\"checksum\":") != std::string::npos);

    auto parsed = protocol::AegisMessage::FromJson(json);
    assert(parsed.has_value());
    assert(parsed->intersection == 2);
    assert(parsed->command == protocol::Commands::BLUE_ON);
    assert(parsed->sequence == 101);
    assert(parsed->status == "OK");
    assert(parsed->details == "Test preemption command");
    assert(parsed->checksum != 0);

    // Invalid JSON check
    auto invalid = protocol::AegisMessage::FromJson("{invalid_json");
    assert(!invalid.has_value());

    std::cout << "[PASS] AegisProtocol JSON & Checksum validated successfully." << std::endl;
}

// Test 4: TCP Client-Server Greeting Handshake
void TestGreetingHandshake() {
    std::cout << "[TEST 4] Testing TCP Client-Server INTERSECTION_READY greeting handshake..." << std::endl;

    auto gpio1 = std::make_shared<gpio::SimulationGpioController>(1);
    intersection::IntersectionService service1(1, 15001, gpio1);
    assert(service1.Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15001);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15092);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15093);
    coord.SetReconnectIntervalMs(300);
    coord.SetPingIntervalMs(1000);

    assert(coord.Start());

    // Wait for connection and greeting handshake
    int waited = 0;
    while (coord.GetNodeStatus(1) != coordinator::NodeHealthStatus::ONLINE && waited < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited++;
    }

    assert(coord.GetNodeStatus(1) == coordinator::NodeHealthStatus::ONLINE);
    assert(service1.IsClientConnected());

    coord.Stop();
    service1.Stop();
    std::cout << "[PASS] Greeting handshake validated successfully." << std::endl;
}

// Test 5: Command Execution and ACK Handling
void TestCommandExecution() {
    std::cout << "[TEST 5] Testing command dispatch (BLUE_ON, RED_OFF, RESET) and ACK..." << std::endl;

    auto gpio1 = std::make_shared<gpio::SimulationGpioController>(1);
    intersection::IntersectionService service1(1, 15002, gpio1);
    assert(service1.Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15002);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15092);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15093);
    coord.SetReconnectIntervalMs(300);

    assert(coord.Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send BLUE_ON
    bool ok = coord.SendCommandWithAck(1, std::string(protocol::Commands::BLUE_ON));
    assert(ok && "BLUE_ON command must succeed");
    assert(gpio1->getBlue() && "GPIO Blue LED must be ON");

    // Send RED_OFF
    ok = coord.SendCommandWithAck(1, std::string(protocol::Commands::RED_OFF));
    assert(ok && "RED_OFF command must succeed");
    assert(!gpio1->getRed() && "GPIO Red LED must be OFF");

    // Send RESET
    ok = coord.SendCommandWithAck(1, std::string(protocol::Commands::RESET));
    assert(ok && "RESET command must succeed");
    assert(!gpio1->getBlue() && !gpio1->getRed() && "All LEDs must be OFF after RESET");

    coord.Stop();
    service1.Stop();
    std::cout << "[PASS] Command execution and ACK validated successfully." << std::endl;
}

// Test 6: Node 3 Shortage Constraint Command Handling
void TestNode3ShortageExecution() {
    std::cout << "[TEST 6] Testing Node 3 Shortage Constraint (Blue only) execution..." << std::endl;

    auto gpio3 = std::make_shared<gpio::SimulationGpioController>(3);
    intersection::IntersectionService service3(3, 15003, gpio3);
    assert(service3.Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15091);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15092);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15003);
    coord.SetReconnectIntervalMs(300);

    assert(coord.Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Turn Blue ON
    bool ok = coord.SendCommandWithAck(3, std::string(protocol::Commands::BLUE_ON));
    assert(ok);
    assert(gpio3->getBlue());

    // Send RED_ON to Node 3 - should ACK safely without error
    ok = coord.SendCommandWithAck(3, std::string(protocol::Commands::RED_ON));
    assert(ok && "RED_ON on Node 3 must safely ACK without error");
    assert(!gpio3->getRed() && "Node 3 physical Red LED remains false");

    coord.Stop();
    service3.Stop();
    std::cout << "[PASS] Node 3 Shortage execution validated successfully." << std::endl;
}

// Test 7: Retry and Timeout Failure Handling
void TestRetryAndTimeoutHandling() {
    std::cout << "[TEST 7] Testing 3-retry attempt and timeout error marking..." << std::endl;

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    // Node 1 set to an offline address
    coord.SetNodeEndpoint(1, "127.0.0.1", 15099);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15092);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15093);
    coord.SetReconnectIntervalMs(5000); // Don't reconnect immediately

    assert(coord.Start());

    // Command to offline node fails
    bool ok = coord.SendCommandWithAck(1, std::string(protocol::Commands::BLUE_ON));
    assert(!ok && "Command to offline node must return false");

    coord.Stop();
    std::cout << "[PASS] Retry and timeout failure handled gracefully." << std::endl;
}

// Test 8: Health Ping and Automatic Reconnection
void TestPingAndReconnection() {
    std::cout << "[TEST 8] Testing automatic reconnection and periodic health PING..." << std::endl;

    auto gpio1 = std::make_shared<gpio::SimulationGpioController>(1);
    auto service1 = std::make_unique<intersection::IntersectionService>(1, 15004, gpio1);
    assert(service1->Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15004);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15092);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15093);
    coord.SetReconnectIntervalMs(400);
    coord.SetPingIntervalMs(600);

    assert(coord.Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    assert(coord.GetNodeStatus(1) == coordinator::NodeHealthStatus::ONLINE);

    // Stop service -> simulate network drop
    service1->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    assert(coord.GetNodeStatus(1) != coordinator::NodeHealthStatus::ONLINE);

    // Restart service -> Coordinator must automatically reconnect in background!
    service1 = std::make_unique<intersection::IntersectionService>(1, 15004, gpio1);
    assert(service1->Start());

    // Wait for auto-reconnect
    int waited = 0;
    while (coord.GetNodeStatus(1) != coordinator::NodeHealthStatus::ONLINE && waited < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited++;
    }

    assert(coord.GetNodeStatus(1) == coordinator::NodeHealthStatus::ONLINE && "Node must auto-reconnect to ONLINE");

    coord.Stop();
    service1->Stop();
    std::cout << "[PASS] Auto-reconnect and health supervision validated successfully." << std::endl;
}

// Test 9: Multi-Node Cluster Fault Tolerance
void TestMultiNodeFaultTolerance() {
    std::cout << "[TEST 9] Testing multi-node fault tolerance (Node 2 failure does not affect Node 1 or 3)..." << std::endl;

    auto gpio1 = std::make_shared<gpio::SimulationGpioController>(1);
    auto gpio2 = std::make_shared<gpio::SimulationGpioController>(2);
    auto gpio3 = std::make_shared<gpio::SimulationGpioController>(3);

    intersection::IntersectionService service1(1, 15011, gpio1);
    auto service2 = std::make_unique<intersection::IntersectionService>(2, 15012, gpio2);
    intersection::IntersectionService service3(3, 15013, gpio3);

    assert(service1.Start());
    assert(service2->Start());
    assert(service3.Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15011);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15012);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15013);
    coord.SetReconnectIntervalMs(400);

    assert(coord.Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    assert(coord.GetNodeStatus(1) == coordinator::NodeHealthStatus::ONLINE);
    assert(coord.GetNodeStatus(2) == coordinator::NodeHealthStatus::ONLINE);
    assert(coord.GetNodeStatus(3) == coordinator::NodeHealthStatus::ONLINE);

    // Simulate sudden failure of Node 2
    service2->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));

    // Nodes 1 and 3 must continue serving commands without error!
    assert(coord.SendCommandWithAck(1, std::string(protocol::Commands::BLUE_ON)));
    assert(coord.SendCommandWithAck(3, std::string(protocol::Commands::BLUE_ON)));
    assert(gpio1->getBlue());
    assert(gpio3->getBlue());

    // Recover Node 2
    service2 = std::make_unique<intersection::IntersectionService>(2, 15012, gpio2);
    assert(service2->Start());

    int waited = 0;
    while (coord.GetNodeStatus(2) != coordinator::NodeHealthStatus::ONLINE && waited < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited++;
    }

    assert(coord.GetNodeStatus(2) == coordinator::NodeHealthStatus::ONLINE);

    coord.Stop();
    service1.Stop();
    service2->Stop();
    service3.Stop();
    std::cout << "[PASS] Multi-node fault tolerance validated successfully." << std::endl;
}

// Test 10: Complete 5-Stage Live Emergency Green Corridor Flow
void TestFullCorridorLiveDemo() {
    std::cout << "[TEST 10] Testing Complete 5-Stage Emergency Green Corridor Live Demonstration Flow..." << std::endl;

    auto gpio1 = std::make_shared<gpio::SimulationGpioController>(1);
    auto gpio2 = std::make_shared<gpio::SimulationGpioController>(2);
    auto gpio3 = std::make_shared<gpio::SimulationGpioController>(3);

    intersection::IntersectionService service1(1, 15021, gpio1);
    intersection::IntersectionService service2(2, 15022, gpio2);
    intersection::IntersectionService service3(3, 15023, gpio3);

    assert(service1.Start());
    assert(service2.Start());
    assert(service3.Start());

    auto config = config::IntersectionsConfig::CreateDefaultCluster();
    coordinator::CoordinatorClientManager coord(config);
    coord.SetNodeEndpoint(1, "127.0.0.1", 15021);
    coord.SetNodeEndpoint(2, "127.0.0.1", 15022);
    coord.SetNodeEndpoint(3, "127.0.0.1", 15023);
    coord.SetReconnectIntervalMs(400);

    assert(coord.Start());
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    // STAGE 1: Baseline Normal State
    std::cout << "  -> Stage 1: Baseline Reset..." << std::endl;
    assert(coord.ResetAll());
    assert(!gpio1->getBlue() && !gpio1->getRed());
    assert(!gpio2->getBlue() && !gpio2->getRed());
    assert(!gpio3->getBlue() && !gpio3->getRed());

    // STAGE 2: Emergency Preemption - Ambulance Dispatched
    std::cout << "  -> Stage 2: Ambulance Dispatched -> Grant Green Corridor (Nodes 1, 2, 3)..." << std::endl;
    assert(coord.GrantGreenCorridor({1, 2, 3}));
    assert(gpio1->getBlue() && !gpio1->getRed());
    assert(gpio2->getBlue() && !gpio2->getRed());
    assert(gpio3->getBlue() && !gpio3->getRed());

    // STAGE 3: Ambulance Clears Intersection 1
    std::cout << "  -> Stage 3: Ambulance Cleared Intersection 1 -> Release Node 1..." << std::endl;
    assert(coord.ReleaseGreenCorridor({1}));
    assert(!gpio1->getBlue());
    assert(gpio2->getBlue() && "Node 2 corridor remains active");
    assert(gpio3->getBlue() && "Node 3 corridor remains active");

    // STAGE 4: Ambulance Clears Intersection 2
    std::cout << "  -> Stage 4: Ambulance Cleared Intersection 2 -> Release Node 2..." << std::endl;
    assert(coord.ReleaseGreenCorridor({2}));
    assert(!gpio2->getBlue());
    assert(gpio3->getBlue() && "Node 3 corridor remains active");

    // STAGE 5: Ambulance Arrives at Hospital
    std::cout << "  -> Stage 5: Ambulance Arrived at Hospital -> Release Node 3 & Final Reset..." << std::endl;
    assert(coord.ReleaseGreenCorridor({3}));
    assert(!gpio3->getBlue());
    assert(coord.ResetAll());

    assert(!gpio1->getBlue() && !gpio1->getRed());
    assert(!gpio2->getBlue() && !gpio2->getRed());
    assert(!gpio3->getBlue() && !gpio3->getRed());

    coord.Stop();
    service1.Stop();
    service2.Stop();
    service3.Stop();
    std::cout << "[PASS] Complete 5-Stage Live Emergency Green Corridor Flow validated successfully." << std::endl;
}

int main() {
#if defined(SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
#endif
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - Comprehensive System Verification Suite" << std::endl;
    std::cout << " Target: QNX SDP 8.0 & Cross-Platform Simulation" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestConfigurationLoading();
    TestGpioAbstraction();
    TestProtocolAndChecksum();
    TestGreetingHandshake();
    TestCommandExecution();
    TestNode3ShortageExecution();
    TestRetryAndTimeoutHandling();
    TestPingAndReconnection();
    TestMultiNodeFaultTolerance();
    TestFullCorridorLiveDemo();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 10 SMART AEGIS SYSTEM TESTS PASSED SUCCESSFULLY! (100%)" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
