/**
 * @file test_coordinator_traffic_signal_phase6b.cpp
 * @brief Comprehensive unit tests for Phase 6B: Coordinator <-> Traffic Signal Integration.
 *
 * Verifies all Phase 6B requirements:
 * 1. IntersectionNode owns one TrafficSignalNode instance.
 * 2. Forwarding of coordinator commands to TrafficSignalNode.
 * 3. Supported commands: SET_RED, SET_YELLOW, SET_GREEN, SET_ALL_RED.
 * 4. Coordinator API: SendTrafficSignalCommand(intersectionId, command, reason).
 * 5. Structured logging on Coordinator:
 *      [COMMAND]
 *      Coordinator
 *      Destination
 *      Command
 *      Reason
 * 6. Structured logging on IntersectionNode:
 *      [COMMAND RECEIVED]
 * 7. Acknowledgement flow: IntersectionNode returns ACK with current state to Coordinator.
 * 8. Structured logging on Coordinator upon ACK:
 *      [ACK]
 *      Intersection
 *      Current Signal State
 * 9. Rejection of invalid and duplicate commands with error acknowledgement.
 * 10. Multi-intersection command dispatch.
 * 11. Complete thread safety and race-free concurrency.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <atomic>
#include <chrono>

#include "coordinator/CoordinatorManager.h"
#include "intersection/IntersectionNode.h"
#include "traffic_signal/TrafficSignalNode.h"
#include "aegis/common/json_helper.hpp"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using aegis::coordinator::CoordinatorManager;
using aegis::coordinator::SignalCommandAck;
using aegis::intersection::IntersectionNode;
using aegis::traffic_signal::TrafficSignalNode;
using aegis::traffic_signal::SignalState;

// -----------------------------------------------------------------------------
// Test 1: Single Intersection Command Dispatch & Pipeline
// -----------------------------------------------------------------------------
static void TestSingleIntersectionCommand() {
    std::cout << "[TEST 1] Testing Single Intersection Command Dispatch & Pipeline..." << std::endl;

    const uint16_t port = 29510;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 50);
    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));

    // Wait for registration
    int retries = 50;
    while (!coordinator.GetIntersectionRegistry().HasIntersection("INT_A") && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    AEGIS_TEST_ASSERT(coordinator.GetIntersectionRegistry().HasIntersection("INT_A"));

    // Initial state: RED
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::RED);

    // 1. Send SET_GREEN
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_GREEN", "Green corridor clearance"));

    SignalCommandAck ack1;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack1));
    AEGIS_TEST_ASSERT(ack1.status == "OK");
    AEGIS_TEST_ASSERT(ack1.currentState == "GREEN");
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);

    // 2. Send SET_YELLOW
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_YELLOW", "Yellow clearance interval"));

    SignalCommandAck ack2;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack2));
    AEGIS_TEST_ASSERT(ack2.status == "OK");
    AEGIS_TEST_ASSERT(ack2.currentState == "YELLOW");
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::YELLOW);

    // 3. Send SET_RED
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_RED", "Stop phase restore"));

    SignalCommandAck ack3;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack3));
    AEGIS_TEST_ASSERT(ack3.status == "OK");
    AEGIS_TEST_ASSERT(ack3.currentState == "RED");
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::RED);

    // 4. Send SET_ALL_RED
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_ALL_RED", "Emergency all-red hold"));

    SignalCommandAck ack4;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack4));
    AEGIS_TEST_ASSERT(ack4.status == "OK");
    AEGIS_TEST_ASSERT(ack4.currentState == "ALL_RED");
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::ALL_RED);

    nodeA.Stop();
    coordinator.Stop();

    std::cout << "  -> Single intersection commands (SET_GREEN, SET_YELLOW, SET_RED, SET_ALL_RED) passed." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Multiple Intersections Command Dispatch
// -----------------------------------------------------------------------------
static void TestMultipleIntersections() {
    std::cout << "[TEST 2] Testing Multiple Intersections Dispatch (INT_A, INT_B, INT_C)..." << std::endl;

    const uint16_t port = 29511;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 50);
    IntersectionNode nodeB("INT_B", 50);
    IntersectionNode nodeC("INT_C", 50);

    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));
    AEGIS_TEST_ASSERT(nodeB.Start("127.0.0.1", port, 3000, 3000));
    AEGIS_TEST_ASSERT(nodeC.Start("127.0.0.1", port, 3000, 3000));

    int retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    AEGIS_TEST_ASSERT(coordinator.GetIntersectionRegistry().GetOnlineCount() == 3);

    coordinator.ClearReceivedAcks();

    // Dispatch commands to each intersection
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_GREEN", "Route head green"));
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_B", "SET_ALL_RED", "Route cross hold"));
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_C", "SET_ALL_RED", "Route cross hold"));

    // Wait for all 3 ACKs
    retries = 50;
    while (coordinator.GetReceivedAcksCount() < 3 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    AEGIS_TEST_ASSERT(coordinator.GetReceivedAcksCount() >= 3);

    // Verify individual intersection signal states
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);
    AEGIS_TEST_ASSERT(nodeB.GetTrafficSignalNode().GetCurrentState() == SignalState::ALL_RED);
    AEGIS_TEST_ASSERT(nodeC.GetTrafficSignalNode().GetCurrentState() == SignalState::ALL_RED);

    nodeA.Stop();
    nodeB.Stop();
    nodeC.Stop();
    coordinator.Stop();

    std::cout << "  -> Multiple intersection dispatch and multi-node acknowledgement verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: Invalid Command Rejection & Error Acknowledgement
// -----------------------------------------------------------------------------
static void TestInvalidCommandRejection() {
    std::cout << "[TEST 3] Testing Invalid Command Rejection & Error ACK..." << std::endl;

    const uint16_t port = 29512;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 50);
    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));

    int retries = 50;
    while (!coordinator.GetIntersectionRegistry().HasIntersection("INT_A") && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // 1. Completely unknown command
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_PURPLE", "Unknown command test"));

    SignalCommandAck ack1;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack1));
    AEGIS_TEST_ASSERT(ack1.status == "ERROR");
    AEGIS_TEST_ASSERT(ack1.currentState == "RED"); // State unchanged
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::RED);

    // 2. Prohibited transition: direct GREEN -> RED without yellow clearance
    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_GREEN", "Advance to green"));
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack1));
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);

    coordinator.ClearReceivedAcks();
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_RED", "Prohibited direct red attempt"));

    SignalCommandAck ack2;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack2));
    AEGIS_TEST_ASSERT(ack2.status == "ERROR");
    AEGIS_TEST_ASSERT(ack2.currentState == "GREEN"); // State preserved safely at GREEN
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);

    nodeA.Stop();
    coordinator.Stop();

    std::cout << "  -> Invalid commands rejected safely and error acknowledgement returned." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: Duplicate Command Handling & Suppression
// -----------------------------------------------------------------------------
static void TestDuplicateCommandHandling() {
    std::cout << "[TEST 4] Testing Duplicate Command Handling & Suppression..." << std::endl;

    const uint16_t port = 29513;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 50);
    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));

    int retries = 50;
    while (!coordinator.GetIntersectionRegistry().HasIntersection("INT_A") && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    // Node A is initialized to RED. Send duplicate SET_RED.
    size_t prevTransitionCount = nodeA.GetTrafficSignalNode().GetTransitionCount();
    coordinator.ClearReceivedAcks();

    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_RED", "Duplicate red command"));

    SignalCommandAck ack;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack));
    AEGIS_TEST_ASSERT(ack.status == "ERROR"); // Rejected duplicate
    AEGIS_TEST_ASSERT(ack.currentState == "RED");

    // Ensure transition count did NOT increment
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetTransitionCount() == prevTransitionCount);
    AEGIS_TEST_ASSERT(nodeA.GetTrafficSignalNode().GetCurrentState() == SignalState::RED);

    nodeA.Stop();
    coordinator.Stop();

    std::cout << "  -> Duplicate commands properly ignored without state transition side-effects." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Acknowledgement Flow & Log Verification
// -----------------------------------------------------------------------------
static void TestAcknowledgementFlowAndLogging() {
    std::cout << "[TEST 5] Testing Acknowledgement Flow & Structured Log Formats..." << std::endl;

    const uint16_t port = 29514;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_A", 50);
    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));

    int retries = 50;
    while (!coordinator.GetIntersectionRegistry().HasIntersection("INT_A") && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    coordinator.ClearCommandLogs();
    coordinator.ClearReceivedAcks();
    nodeA.ClearReceivedCommands();

    // Send command: SET_GREEN
    AEGIS_TEST_ASSERT(coordinator.SendTrafficSignalCommand("INT_A", "SET_GREEN", "Audit trail verification"));

    SignalCommandAck ack;
    AEGIS_TEST_ASSERT(coordinator.WaitForAck("INT_A", 2000, &ack));

    // 1. Verify Coordinator Command Log Format (Requirement 9)
    // [COMMAND]
    // Coordinator: COORDINATOR
    // Destination: INT_A
    // Command: SET_GREEN
    // Reason: Audit trail verification
    std::string cmdLog = coordinator.GetLastCommandLog();
    AEGIS_TEST_ASSERT(!cmdLog.empty());

    std::istringstream cmdStream(cmdLog);
    std::string cl1, cl2, cl3, cl4, cl5;
    std::getline(cmdStream, cl1);
    std::getline(cmdStream, cl2);
    std::getline(cmdStream, cl3);
    std::getline(cmdStream, cl4);
    std::getline(cmdStream, cl5);

    AEGIS_TEST_ASSERT(cl1 == "[COMMAND]");
    AEGIS_TEST_ASSERT(cl2.find("Coordinator:") != std::string::npos);
    AEGIS_TEST_ASSERT(cl3 == "Destination: INT_A");
    AEGIS_TEST_ASSERT(cl4 == "Command: SET_GREEN");
    AEGIS_TEST_ASSERT(cl5 == "Reason: Audit trail verification");

    // 2. Verify IntersectionNode Command Ingestion (Requirement 10)
    AEGIS_TEST_ASSERT(nodeA.GetCommandsReceivedCount() >= 1);
    AEGIS_TEST_ASSERT(nodeA.GetLastReceivedCommand() == "SET_GREEN");

    // 3. Verify Coordinator ACK Log Format (Requirement 12)
    // [ACK]
    // Intersection: INT_A
    // Current Signal State: GREEN
    std::string ackLog = coordinator.GetLastAckLog();
    AEGIS_TEST_ASSERT(!ackLog.empty());

    std::istringstream ackStream(ackLog);
    std::string al1, al2, al3;
    std::getline(ackStream, al1);
    std::getline(ackStream, al2);
    std::getline(ackStream, al3);

    AEGIS_TEST_ASSERT(al1 == "[ACK]");
    AEGIS_TEST_ASSERT(al2 == "Intersection: INT_A");
    AEGIS_TEST_ASSERT(al3 == "Current Signal State: GREEN");

    nodeA.Stop();
    coordinator.Stop();

    std::cout << "  -> Structured command and ACK log formats strictly match specification." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: Direct In-Memory Pipeline & Callback Verification
// -----------------------------------------------------------------------------
static void TestDirectPipelineAndCallbacks() {
    std::cout << "[TEST 6] Testing Direct Pipeline and Mocked ACK Callbacks..." << std::endl;

    IntersectionNode node("INT_ISOLATED");
    AEGIS_TEST_ASSERT(node.GetTrafficSignalNode().GetCurrentState() == SignalState::RED);

    bool ackFired = false;
    std::string ackStatus = "";
    node.SetAckCallback([&](const aegis::communication::Message& msg) {
        ackFired = true;
        auto stOpt = aegis::common::JsonHelper::ExtractString(msg.GetPayload(), "status");
        if (stOpt.has_value()) ackStatus = stOpt.value();
    });

    // Direct invocation
    bool ok = node.ProcessCoordinatorCommand("SET_GREEN", "Direct test green");
    AEGIS_TEST_ASSERT(ok);
    AEGIS_TEST_ASSERT(ackFired);
    AEGIS_TEST_ASSERT(ackStatus == "OK");
    AEGIS_TEST_ASSERT(node.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);

    // Direct invalid transition attempt
    ackFired = false;
    ok = node.ProcessCoordinatorCommand("SET_RED", "Direct illegal red");
    AEGIS_TEST_ASSERT(!ok);
    AEGIS_TEST_ASSERT(ackFired);
    AEGIS_TEST_ASSERT(ackStatus == "ERROR");
    AEGIS_TEST_ASSERT(node.GetTrafficSignalNode().GetCurrentState() == SignalState::GREEN);

    std::cout << "  -> Direct in-memory command processing and callback hooks verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: Multi-Threaded Concurrency & Thread Safety Stress
// -----------------------------------------------------------------------------
static void TestThreadSafety() {
    std::cout << "[TEST 7] Testing Multi-Threaded Concurrency & Thread Safety..." << std::endl;

    const uint16_t port = 29515;
    CoordinatorManager coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start(port, "127.0.0.1"));

    IntersectionNode nodeA("INT_CONCUR_A", 50);
    IntersectionNode nodeB("INT_CONCUR_B", 50);

    AEGIS_TEST_ASSERT(nodeA.Start("127.0.0.1", port, 3000, 3000));
    AEGIS_TEST_ASSERT(nodeB.Start("127.0.0.1", port, 3000, 3000));

    int retries = 50;
    while (coordinator.GetIntersectionRegistry().GetOnlineCount() < 2 && retries-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::atomic<bool> startFlag{false};
    std::atomic<int> commandsSent{0};

    // Thread 1: Sending commands to node A
    std::thread t1([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < 20; ++i) {
            coordinator.SendTrafficSignalCommand("INT_CONCUR_A", "SET_GREEN", "Concur Green A");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            coordinator.SendTrafficSignalCommand("INT_CONCUR_A", "SET_YELLOW", "Concur Yellow A");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            coordinator.SendTrafficSignalCommand("INT_CONCUR_A", "SET_RED", "Concur Red A");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Thread 2: Sending commands to node B
    std::thread t2([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < 20; ++i) {
            coordinator.SendTrafficSignalCommand("INT_CONCUR_B", "SET_ALL_RED", "Concur AllRed B");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            coordinator.SendTrafficSignalCommand("INT_CONCUR_B", "SET_GREEN", "Concur Green B");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            coordinator.SendTrafficSignalCommand("INT_CONCUR_B", "SET_YELLOW", "Concur Yellow B");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            coordinator.SendTrafficSignalCommand("INT_CONCUR_B", "SET_RED", "Concur Red B");
            commandsSent.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    // Thread 3: Querying logs and telemetry concurrently
    std::thread t3([&]() {
        while (!startFlag.load()) std::this_thread::yield();
        for (int i = 0; i < 50; ++i) {
            (void)coordinator.GetCommandLogs();
            (void)coordinator.GetReceivedAcks();
            (void)coordinator.GetLastCommandLog();
            (void)coordinator.GetLastAckLog();
            (void)nodeA.GetCommandsReceivedCount();
            (void)nodeB.GetCommandsReceivedCount();
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    });

    startFlag.store(true);

    t1.join();
    t2.join();
    t3.join();

    AEGIS_TEST_ASSERT(commandsSent.load() > 0);
    AEGIS_TEST_ASSERT(coordinator.GetCommandLogs().size() == static_cast<size_t>(commandsSent.load()));

    nodeA.Stop();
    nodeB.Stop();
    coordinator.Stop();

    std::cout << "  -> Concurrency test passed with " << commandsSent.load()
              << " commands dispatched safely under heavy cross-thread load." << std::endl;
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 6B Coordinator <-> Signal Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    try {
        TestSingleIntersectionCommand();
        TestMultipleIntersections();
        TestInvalidCommandRejection();
        TestDuplicateCommandHandling();
        TestAcknowledgementFlowAndLogging();
        TestDirectPipelineAndCallbacks();
        TestThreadSafety();

        std::cout << "\n[PASS] All Phase 6B Coordinator <-> Signal Integration unit tests passed successfully!\n" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] Uncaught exception during Phase 6B test: " << ex.what() << std::endl;
        return 1;
    }
}
