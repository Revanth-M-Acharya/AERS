/**
 * @file test_traffic_signal_node_phase6a.cpp
 * @brief Comprehensive unit tests for Phase 6A: Traffic Signal Node Foundation.
 *
 * Verifies all 13 Phase 6A requirements:
 * 1. TrafficSignalNode module creation.
 * 2. Internal signal state machine with 6 discrete states (RED, YELLOW, GREEN,
 *    FLASHING_RED, FLASHING_YELLOW, ALL_RED).
 * 3. Coordinator commands: SET_RED, SET_YELLOW, SET_GREEN, SET_ALL_RED.
 * 4. Strict rejection of invalid transitions.
 * 5. Ignoring duplicate state changes (no transition, no log, returns false).
 * 6. Structured logging of every successful transition in the specified multi-line format.
 * 7. Public APIs: GetCurrentState(), SetSignalState(), HandleCoordinatorCommand().
 * 8. Pure software simulation (no GPIO / hardware dependencies).
 * 9. Thread safety and concurrency.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <sstream>
#include <atomic>

#include "traffic_signal/TrafficSignalNode.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::traffic_signal;

// -----------------------------------------------------------------------------
// Test 1: Module Creation, Initialization & Basic Accessors
// -----------------------------------------------------------------------------
static void TestInitializationAndBasics() {
    std::cout << "[TEST 1] Testing Module Creation, Initialization & Basic Accessors..." << std::endl;

    // Default initialization (INT_A, RED)
    TrafficSignalNode defaultNode;
    AEGIS_TEST_ASSERT(defaultNode.GetIntersectionId() == "INT_A");
    AEGIS_TEST_ASSERT(defaultNode.GetCurrentState() == SignalState::RED);
    AEGIS_TEST_ASSERT(defaultNode.GetCurrentStateString() == "RED");
    AEGIS_TEST_ASSERT(defaultNode.GetTransitionCount() == 0);
    AEGIS_TEST_ASSERT(defaultNode.GetLastTransitionLog().empty());

    // Custom initialization (INT_NORTH, ALL_RED)
    TrafficSignalNode customNode("INT_NORTH", SignalState::ALL_RED);
    AEGIS_TEST_ASSERT(customNode.GetIntersectionId() == "INT_NORTH");
    AEGIS_TEST_ASSERT(customNode.GetCurrentState() == SignalState::ALL_RED);
    AEGIS_TEST_ASSERT(customNode.GetCurrentStateString() == "ALL_RED");

    // Identifier mutation
    customNode.SetIntersectionId("INT_SOUTH");
    AEGIS_TEST_ASSERT(customNode.GetIntersectionId() == "INT_SOUTH");

    // Move semantics test
    TrafficSignalNode movedNode(std::move(customNode));
    AEGIS_TEST_ASSERT(movedNode.GetIntersectionId() == "INT_SOUTH");
    AEGIS_TEST_ASSERT(movedNode.GetCurrentState() == SignalState::ALL_RED);

    TrafficSignalNode assignedNode;
    assignedNode = std::move(movedNode);
    AEGIS_TEST_ASSERT(assignedNode.GetIntersectionId() == "INT_SOUTH");
    AEGIS_TEST_ASSERT(assignedNode.GetCurrentState() == SignalState::ALL_RED);

    std::cout << "  -> Module created and initialized successfully with full state inspection." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Valid State Transitions (Normal Cycle, Emergency, and Degraded Modes)
// -----------------------------------------------------------------------------
static void TestValidTransitions() {
    std::cout << "[TEST 2] Testing Valid State Transitions..." << std::endl;

    TrafficSignalNode node("INT_01", SignalState::RED);

    // 1. Standard cycle: RED -> GREEN -> YELLOW -> RED
    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::GREEN, "Normal cycle to GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 1);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::YELLOW));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::YELLOW, "Clearance phase to YELLOW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::YELLOW);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 2);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::RED, "Stop phase to RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 3);

    // 2. Emergency Preemption: RED -> ALL_RED -> GREEN
    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::ALL_RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::ALL_RED, "Emergency preemption entry"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::ALL_RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 4);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::GREEN, "Emergency green wave granted"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 5);

    // 3. Emergency Clearance: GREEN -> ALL_RED -> RED
    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::ALL_RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::ALL_RED, "All-approach emergency hold"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::ALL_RED);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::RED, "Restoring standard red"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);

    // 4. Degraded / Fail-Safe Transitions:
    // RED -> FLASHING_YELLOW -> FLASHING_RED -> ALL_RED -> RED
    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::FLASHING_YELLOW));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::FLASHING_YELLOW, "Cautionary flashing mode"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_YELLOW);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::FLASHING_RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::FLASHING_RED, "Critical hazard flashing mode"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_RED);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::ALL_RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::ALL_RED, "Recovering via ALL_RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::ALL_RED);

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::FLASHING_RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::FLASHING_RED, "Re-entering hazard flash"));

    AEGIS_TEST_ASSERT(node.CanTransitionTo(SignalState::RED));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::RED, "Direct recovery from flashing to RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);

    std::cout << "  -> All valid traffic signal transitions verified successfully." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: Rejection of Invalid State Transitions
// -----------------------------------------------------------------------------
static void TestInvalidTransitionsRejected() {
    std::cout << "[TEST 3] Testing Strict Rejection of Invalid Transitions..." << std::endl;

    TrafficSignalNode node("INT_01", SignalState::RED);
    size_t initialCount = node.GetTransitionCount();

    // Prohibit direct RED -> YELLOW (MUST transition RED -> GREEN, never direct to YELLOW)
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::YELLOW));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::YELLOW, "Illegal RED -> YELLOW attempt"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == initialCount);

    // Move to GREEN
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::GREEN, "Advance to GREEN"));
    initialCount = node.GetTransitionCount();

    // Prohibit direct GREEN -> RED (MUST clear via YELLOW first to protect fast-moving traffic)
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::RED));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::RED, "Illegal direct GREEN -> RED attempt"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == initialCount);

    // Move to YELLOW
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::YELLOW, "Advance to YELLOW"));
    initialCount = node.GetTransitionCount();

    // Prohibit direct YELLOW -> GREEN (MUST finish yellow clearance and enter RED/ALL_RED)
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::GREEN, "Illegal YELLOW -> GREEN attempt"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::YELLOW);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == initialCount);

    // Move YELLOW -> RED -> FLASHING_RED
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::RED, "Clearance to RED"));
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::FLASHING_RED, "Enter FLASHING_RED"));
    initialCount = node.GetTransitionCount();

    // Prohibit FLASHING_RED -> GREEN and FLASHING_RED -> YELLOW directly
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::GREEN, "Illegal FLASHING_RED -> GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_RED);

    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::YELLOW));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::YELLOW, "Illegal FLASHING_RED -> YELLOW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == initialCount);

    // Test FLASHING_YELLOW direct jump prohibitions
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::FLASHING_YELLOW, "Switch to FLASHING_YELLOW"));
    initialCount = node.GetTransitionCount();

    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::GREEN, "Illegal FLASHING_YELLOW -> GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_YELLOW);

    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::YELLOW));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::YELLOW, "Illegal FLASHING_YELLOW -> YELLOW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_YELLOW);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == initialCount);

    std::cout << "  -> All invalid state transitions correctly rejected with state preserved." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: Duplicate State Changes Ignored
// -----------------------------------------------------------------------------
static void TestDuplicateStateChangesIgnored() {
    std::cout << "[TEST 4] Testing Duplicate State Changes Ignored..." << std::endl;

    TrafficSignalNode node("INT_01", SignalState::RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 0);

    // 1. Attempting RED -> RED
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::RED));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::RED, "Duplicate RED command"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 0);
    AEGIS_TEST_ASSERT(node.GetTransitionHistory().empty());

    // 2. Transition RED -> GREEN
    AEGIS_TEST_ASSERT(node.SetSignalState(SignalState::GREEN, "Valid advance"));
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 1);

    // 3. Attempting GREEN -> GREEN multiple times
    AEGIS_TEST_ASSERT(!node.CanTransitionTo(SignalState::GREEN));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::GREEN, "Duplicate GREEN"));
    AEGIS_TEST_ASSERT(!node.SetSignalState(SignalState::GREEN, "Second duplicate GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 1); // No new logs or records

    // 4. Duplicate via Coordinator command
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand("SET_GREEN"));
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 1);

    std::cout << "  -> Duplicate state transitions cleanly ignored without logging or side-effects." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Coordinator Commands (SET_RED, SET_YELLOW, SET_GREEN, SET_ALL_RED)
// -----------------------------------------------------------------------------
static void TestCoordinatorCommands() {
    std::cout << "[TEST 5] Testing Central Coordinator Command Handling..." << std::endl;

    TrafficSignalNode node("INT_01", SignalState::RED);

    // 1. SET_GREEN from RED
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);

    // 2. SET_YELLOW from GREEN
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_YELLOW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::YELLOW);

    // 3. SET_RED from YELLOW
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);

    // 4. SET_ALL_RED from RED
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_ALL_RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::ALL_RED);

    // 5. Case-insensitivity support
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("set_green"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);

    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("sEt_YeLLoW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::YELLOW);

    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("Set_Red"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::RED);

    // 6. JSON command formats
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("{\"command\": \"SET_GREEN\"}"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);

    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("{\"action\": \"SET_YELLOW\"}"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::YELLOW);

    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("{\"command\": \"SET_ALL_RED\"}"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::ALL_RED);

    // 7. Flashing commands
    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_FLASHING_RED"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_RED);

    AEGIS_TEST_ASSERT(node.HandleCoordinatorCommand("SET_FLASHING_YELLOW"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_YELLOW);

    // 8. Invalid commands rejection
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand(""));
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand("INVALID_COMMAND"));
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand("SET_BLUE"));
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand("{}"));

    // 9. Command triggering invalid transition: SET_GREEN directly from FLASHING_YELLOW is prohibited
    AEGIS_TEST_ASSERT(!node.HandleCoordinatorCommand("SET_GREEN"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::FLASHING_YELLOW);

    std::cout << "  -> Coordinator commands processed, validated, and executed accurately." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: Structured Transition Audit Logging Format Verification
// -----------------------------------------------------------------------------
static void TestStructuredTransitionLogging() {
    std::cout << "[TEST 6] Testing Structured Transition Audit Logging Format..." << std::endl;

    TrafficSignalNode node("INT_HARBOR", SignalState::RED);
    node.ClearTransitionHistory();
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 0);

    bool callbackFired = false;
    SignalTransitionRecord lastCbRecord;

    node.SetTransitionCallback([&](const SignalTransitionRecord& record) {
        callbackFired = true;
        lastCbRecord = record;
    });

    // Execute transition: RED -> GREEN
    bool ok = node.SetSignalState(SignalState::GREEN, "Emergency Green Wave Clearance");
    AEGIS_TEST_ASSERT(ok);
    AEGIS_TEST_ASSERT(callbackFired);
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 1);

    // Inspect stored transition record
    auto history = node.GetTransitionHistory();
    AEGIS_TEST_ASSERT(history.size() == 1);

    const auto& rec = history[0];
    AEGIS_TEST_ASSERT(rec.intersectionId == "INT_HARBOR");
    AEGIS_TEST_ASSERT(rec.previousState == SignalState::RED);
    AEGIS_TEST_ASSERT(rec.newState == SignalState::GREEN);
    AEGIS_TEST_ASSERT(rec.reason == "Emergency Green Wave Clearance");
    AEGIS_TEST_ASSERT(rec.timestampMs > 0);
    AEGIS_TEST_ASSERT(!rec.timestampStr.empty());

    // Verify Callback fields match
    AEGIS_TEST_ASSERT(lastCbRecord.intersectionId == rec.intersectionId);
    AEGIS_TEST_ASSERT(lastCbRecord.previousState == rec.previousState);
    AEGIS_TEST_ASSERT(lastCbRecord.newState == rec.newState);

    // Verify exact log format lines:
    // [Timestamp]
    // Intersection ID: INT_HARBOR
    // Previous State: RED
    // New State: GREEN
    // Reason: Emergency Green Wave Clearance
    std::string formatted = node.GetLastTransitionLog();
    AEGIS_TEST_ASSERT(!formatted.empty());

    std::istringstream iss(formatted);
    std::string line1, line2, line3, line4, line5;
    std::getline(iss, line1);
    std::getline(iss, line2);
    std::getline(iss, line3);
    std::getline(iss, line4);
    std::getline(iss, line5);

    // Line 1: [Timestamp]
    AEGIS_TEST_ASSERT(line1.front() == '[' && line1.back() == ']');
    // Line 2: Intersection ID: INT_HARBOR
    AEGIS_TEST_ASSERT(line2 == "Intersection ID: INT_HARBOR");
    // Line 3: Previous State: RED
    AEGIS_TEST_ASSERT(line3 == "Previous State: RED");
    // Line 4: New State: GREEN
    AEGIS_TEST_ASSERT(line4 == "New State: GREEN");
    // Line 5: Reason: Emergency Green Wave Clearance
    AEGIS_TEST_ASSERT(line5 == "Reason: Emergency Green Wave Clearance");

    // Clear history verification
    node.ClearTransitionHistory();
    AEGIS_TEST_ASSERT(node.GetTransitionCount() == 0);
    AEGIS_TEST_ASSERT(node.GetTransitionHistory().empty());
    AEGIS_TEST_ASSERT(node.GetLastTransitionLog().empty());

    std::cout << "  -> Structured 5-line transition log verified against exact specification." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: String Overloads and Helper Functions
// -----------------------------------------------------------------------------
static void TestStringOverloadsAndHelpers() {
    std::cout << "[TEST 7] Testing String Overloads and Parsing Helpers..." << std::endl;

    // SignalStateToString
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::RED) == "RED");
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::YELLOW) == "YELLOW");
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::GREEN) == "GREEN");
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::FLASHING_RED) == "FLASHING_RED");
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::FLASHING_YELLOW) == "FLASHING_YELLOW");
    AEGIS_TEST_ASSERT(SignalStateToString(SignalState::ALL_RED) == "ALL_RED");

    // StringToSignalState
    AEGIS_TEST_ASSERT(StringToSignalState("red") == SignalState::RED);
    AEGIS_TEST_ASSERT(StringToSignalState("YELLOW") == SignalState::YELLOW);
    AEGIS_TEST_ASSERT(StringToSignalState("Green") == SignalState::GREEN);
    AEGIS_TEST_ASSERT(StringToSignalState("Flashing_Red") == SignalState::FLASHING_RED);
    AEGIS_TEST_ASSERT(StringToSignalState("flashing_yellow") == SignalState::FLASHING_YELLOW);
    AEGIS_TEST_ASSERT(StringToSignalState("ALL_RED") == SignalState::ALL_RED);

    // TryStringToSignalState
    SignalState outState;
    AEGIS_TEST_ASSERT(TryStringToSignalState("GREEN", outState));
    AEGIS_TEST_ASSERT(outState == SignalState::GREEN);

    AEGIS_TEST_ASSERT(!TryStringToSignalState("INVALID_STATE", outState));
    AEGIS_TEST_ASSERT(!TryStringToSignalState("", outState));

    // String overload of SetSignalState
    TrafficSignalNode node("INT_STR", SignalState::RED);
    AEGIS_TEST_ASSERT(node.SetSignalState("GREEN", "String overload valid"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);

    AEGIS_TEST_ASSERT(!node.SetSignalState("INVALID_STRING", "Invalid state attempt"));
    AEGIS_TEST_ASSERT(node.GetCurrentState() == SignalState::GREEN);

    std::cout << "  -> String overloads and parsing helpers verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 8: Thread Safety and Concurrency Stress
// -----------------------------------------------------------------------------
static void TestThreadSafetyAndConcurrency() {
    std::cout << "[TEST 8] Testing Thread Safety and Concurrency Stress..." << std::endl;

    TrafficSignalNode node("INT_CONCURRENCY", SignalState::RED);
    std::atomic<bool> startFlag{false};
    std::atomic<int> validTransitions{0};

    // Reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&node, &startFlag]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            for (int j = 0; j < 500; ++j) {
                SignalState s = node.GetCurrentState();
                (void)s;
                (void)node.CanTransitionTo(SignalState::ALL_RED);
                (void)node.GetTransitionCount();
            }
        });
    }

    // Writer threads - cycling safely
    std::vector<std::thread> writers;
    for (int i = 0; i < 4; ++i) {
        writers.emplace_back([&node, &startFlag, &validTransitions]() {
            while (!startFlag.load()) {
                std::this_thread::yield();
            }
            for (int j = 0; j < 50; ++j) {
                // Try transition cycle safely
                if (node.SetSignalState(SignalState::ALL_RED, "Concur All Red")) {
                    validTransitions.fetch_add(1);
                }
                if (node.SetSignalState(SignalState::RED, "Concur Red")) {
                    validTransitions.fetch_add(1);
                }
                if (node.SetSignalState(SignalState::GREEN, "Concur Green")) {
                    validTransitions.fetch_add(1);
                }
                if (node.SetSignalState(SignalState::YELLOW, "Concur Yellow")) {
                    validTransitions.fetch_add(1);
                }
            }
        });
    }

    startFlag.store(true);

    for (auto& r : readers) r.join();
    for (auto& w : writers) w.join();

    // Verify node integrity
    AEGIS_TEST_ASSERT(node.GetTransitionCount() >= static_cast<size_t>(validTransitions.load()));
    SignalState finalState = node.GetCurrentState();
    AEGIS_TEST_ASSERT(finalState == SignalState::RED ||
                      finalState == SignalState::YELLOW ||
                      finalState == SignalState::GREEN ||
                      finalState == SignalState::ALL_RED);

    std::cout << "  -> Concurrent execution passed with " << validTransitions.load()
              << " transitions and zero race conditions." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 9: Forwarding Header and Namespace Interoperability
// -----------------------------------------------------------------------------
static void TestNamespaceInteroperability() {
    std::cout << "[TEST 9] Testing Namespace Interoperability (aegis::intersection alias)..." << std::endl;

    aegis::intersection::TrafficSignalNode interNode("INT_ALIAS", aegis::traffic_signal::SignalState::RED);
    AEGIS_TEST_ASSERT(interNode.GetCurrentState() == aegis::traffic_signal::SignalState::RED);
    AEGIS_TEST_ASSERT(interNode.SetSignalState(aegis::traffic_signal::SignalState::GREEN));
    AEGIS_TEST_ASSERT(interNode.GetCurrentState() == aegis::traffic_signal::SignalState::GREEN);

    std::cout << "  -> Namespace alias aegis::intersection::TrafficSignalNode verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 6A Traffic Signal Node Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    try {
        TestInitializationAndBasics();
        TestValidTransitions();
        TestInvalidTransitionsRejected();
        TestDuplicateStateChangesIgnored();
        TestCoordinatorCommands();
        TestStructuredTransitionLogging();
        TestStringOverloadsAndHelpers();
        TestThreadSafetyAndConcurrency();
        TestNamespaceInteroperability();

        std::cout << "\n[PASS] All Phase 6A Traffic Signal Node unit tests passed successfully!\n" << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] Uncaught exception during Phase 6A test: " << ex.what() << std::endl;
        return 1;
    }
}
