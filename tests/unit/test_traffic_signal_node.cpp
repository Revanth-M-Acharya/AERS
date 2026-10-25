/**
 * @file test_traffic_signal_node.cpp
 * @brief Unit tests for SMART AEGIS Phase 5A Traffic Signal Node and Subsystems.
 */

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>
#include <string>

#include "aegis/edge/gpio_interface.hpp"
#include "aegis/edge/signal_state_machine.hpp"
#include "aegis/edge/preemption_manager.hpp"
#include "aegis/edge/recovery_manager.hpp"
#include "aegis/edge/traffic_controller.hpp"
#include "aegis/edge/traffic_signal_node.hpp"
#include "aegis/logger/logger.hpp"

using namespace aegis::edge::traffic;

// Custom Mock GPIO to verify pure IGpioInterface abstraction
class MockGpioInterface : public IGpioInterface {
public:
    bool Initialize() override {
        initialized = true;
        calls.push_back("Initialize");
        return true;
    }
    void SetRed() override { calls.push_back("SetRed"); }
    void SetYellow() override { calls.push_back("SetYellow"); }
    void SetGreen() override { calls.push_back("SetGreen"); }
    void FlashYellow() override { calls.push_back("FlashYellow"); }
    void FlashRed() override { calls.push_back("FlashRed"); }
    void AllOff() override { calls.push_back("AllOff"); }
    void Shutdown() override {
        initialized = false;
        calls.push_back("Shutdown");
    }

    bool initialized{false};
    std::vector<std::string> calls;
};

void TestGpioAbstractionAndLogging() {
    std::cout << "[TEST] 1. GPIO Abstraction & Simulation Logging..." << std::endl;

    SimulationGpioInterface simGpio;
    assert(!simGpio.IsInitialized());

    assert(simGpio.Initialize());
    assert(simGpio.IsInitialized());
    assert(simGpio.GetLastAction() == "INITIALIZED");

    simGpio.SetRed();
    assert(simGpio.GetLastAction() == "SET_RED");

    simGpio.SetYellow();
    assert(simGpio.GetLastAction() == "SET_YELLOW");

    simGpio.SetGreen();
    assert(simGpio.GetLastAction() == "SET_GREEN");

    simGpio.FlashYellow();
    assert(simGpio.GetLastAction() == "FLASH_YELLOW");

    simGpio.FlashRed();
    assert(simGpio.GetLastAction() == "FLASH_RED");

    simGpio.AllOff();
    assert(simGpio.GetLastAction() == "ALL_OFF");

    simGpio.Shutdown();
    assert(!simGpio.IsInitialized());
    assert(simGpio.GetLastAction() == "SHUTDOWN");

    // Factory test
    auto created = CreateSimulationGpio();
    assert(created != nullptr);
    assert(created->Initialize());
    created->SetGreen();
    created->Shutdown();

    std::cout << "  -> PASS: GPIO Abstraction & Simulation verified." << std::endl;
}

void TestInvalidTransitions() {
    std::cout << "[TEST] 2. State Machine Invalid Transitions Rejection..." << std::endl;

    SignalStateMachine fsm(SignalState::RED);
    assert(fsm.GetCurrentState() == SignalState::RED);

    // From RED:
    // RED -> YELLOW must be REJECTED
    assert(!fsm.CanTransitionTo(SignalState::YELLOW));
    assert(!fsm.TransitionTo(SignalState::YELLOW));
    assert(fsm.GetCurrentState() == SignalState::RED);

    // Valid RED -> GREEN
    assert(fsm.CanTransitionTo(SignalState::GREEN));
    assert(fsm.TransitionTo(SignalState::GREEN));
    assert(fsm.GetCurrentState() == SignalState::GREEN);

    // From GREEN:
    // GREEN -> RED directly must be REJECTED (skipping yellow is a safety hazard)
    assert(!fsm.CanTransitionTo(SignalState::RED));
    assert(!fsm.TransitionTo(SignalState::RED));
    assert(fsm.GetCurrentState() == SignalState::GREEN);

    // Valid GREEN -> YELLOW
    assert(fsm.CanTransitionTo(SignalState::YELLOW));
    assert(fsm.TransitionTo(SignalState::YELLOW));
    assert(fsm.GetCurrentState() == SignalState::YELLOW);

    // From YELLOW:
    // YELLOW -> GREEN directly must be REJECTED (cannot revert to green without red cycle)
    assert(!fsm.CanTransitionTo(SignalState::GREEN));
    assert(!fsm.TransitionTo(SignalState::GREEN));
    assert(fsm.GetCurrentState() == SignalState::YELLOW);

    // Valid YELLOW -> RED
    assert(fsm.CanTransitionTo(SignalState::RED));
    assert(fsm.TransitionTo(SignalState::RED));
    assert(fsm.GetCurrentState() == SignalState::RED);

    // String conversion
    assert(SignalStateToString(SignalState::RED) == "RED");
    assert(SignalStateToString(SignalState::YELLOW) == "YELLOW");
    assert(SignalStateToString(SignalState::GREEN) == "GREEN");
    assert(SignalStateToString(SignalState::ALL_RED) == "ALL_RED");
    assert(StringToSignalState("GREEN") == SignalState::GREEN);
    assert(StringToSignalState("INVALID") == SignalState::UNKNOWN);

    std::cout << "  -> PASS: Invalid transitions strictly rejected." << std::endl;
}

void TestPreemptionAndRecoveryManagers() {
    std::cout << "[TEST] 3. Preemption and Recovery Managers..." << std::endl;

    PreemptionManager preemption;
    assert(!preemption.IsEmergencyActive());

    preemption.StartEmergency();
    assert(preemption.IsEmergencyActive());

    // Duplicate StartEmergency should be idempotent
    preemption.StartEmergency();
    assert(preemption.IsEmergencyActive());

    preemption.CancelEmergency();
    assert(!preemption.IsEmergencyActive());

    RecoveryManager recovery;
    assert(!recovery.IsRecoveryActive());

    recovery.StartRecovery();
    assert(recovery.IsRecoveryActive());

    recovery.RecoveryFinished();
    assert(!recovery.IsRecoveryActive());

    recovery.StartRecovery();
    assert(recovery.IsRecoveryActive());
    recovery.Reset();
    assert(!recovery.IsRecoveryActive());

    std::cout << "  -> PASS: Preemption and Recovery managers verified." << std::endl;
}

void TestStateTransitionsNormalCycle() {
    std::cout << "[TEST] 4. Normal Traffic Cycle State Transitions..." << std::endl;

    TrafficTimingConfig config;
    config.redDurationMs = 200;
    config.greenDurationMs = 300;
    config.yellowDurationMs = 100;

    auto mockGpio = std::make_unique<MockGpioInterface>();
    auto* mockPtr = mockGpio.get();

    TrafficController controller(std::move(mockGpio), config);
    assert(controller.Initialize());
    assert(controller.Start());

    // Initial state: RED
    assert(controller.GetCurrentState() == SignalState::RED);
    assert(mockPtr->calls.back() == "SetRed");

    // Advance 100ms: still in RED
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::RED);

    // Advance another 100ms (total 200ms >= redDurationMs): transition to GREEN
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::GREEN);
    assert(mockPtr->calls.back() == "SetGreen");

    // Advance 200ms in GREEN: still GREEN
    controller.Tick(200);
    assert(controller.GetCurrentState() == SignalState::GREEN);

    // Advance another 100ms (total 300ms >= greenDurationMs): transition to YELLOW
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::YELLOW);
    assert(mockPtr->calls.back() == "SetYellow");

    // Advance 100ms in YELLOW (total 100ms >= yellowDurationMs): transition to RED
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::RED);
    assert(mockPtr->calls.back() == "SetRed");

    // Next cycle: after 200ms in RED, should go back to GREEN
    controller.Tick(200);
    assert(controller.GetCurrentState() == SignalState::GREEN);
    assert(mockPtr->calls.back() == "SetGreen");

    controller.Stop();
    std::cout << "  -> PASS: Deterministic normal cycle verified." << std::endl;
}

void TestEmergencyActivationAndRecovery() {
    std::cout << "[TEST] 5. Emergency Preemption & Recovery Cycle..." << std::endl;

    TrafficTimingConfig config;
    config.redDurationMs = 200;
    config.greenDurationMs = 300;
    config.yellowDurationMs = 100;
    config.allRedClearanceMs = 150;
    config.recoveryDurationMs = 200;

    auto mockGpio = std::make_unique<MockGpioInterface>();
    auto* mockPtr = mockGpio.get();

    TrafficController controller(std::move(mockGpio), config);
    assert(controller.Initialize());
    assert(controller.Start());

    assert(controller.GetCurrentState() == SignalState::RED);

    // 1. Request Emergency Preemption while in RED
    assert(controller.RequestPreemption());
    assert(controller.IsPreemptionActive());
    assert(controller.GetCurrentState() == SignalState::ALL_RED);
    assert(mockPtr->calls.back() == "SetRed");

    // Advance 100ms in ALL_RED: still ALL_RED (clearance is 150ms)
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::ALL_RED);

    // Advance 50ms (total 150ms >= allRedClearanceMs): transitions to GREEN
    controller.Tick(50);
    assert(controller.GetCurrentState() == SignalState::GREEN);
    assert(mockPtr->calls.back() == "SetGreen");

    // Advance 1000ms: green should HOLD because emergency is still active!
    controller.Tick(1000);
    assert(controller.GetCurrentState() == SignalState::GREEN);
    assert(controller.IsPreemptionActive());

    // 2. Clear Preemption -> enters Recovery
    assert(controller.ClearPreemption());
    assert(!controller.IsPreemptionActive());
    assert(controller.IsRecoveryActive());

    // During recovery, signal stays GREEN
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::GREEN);
    assert(controller.IsRecoveryActive());

    // Advance another 100ms (total 200ms >= recoveryDurationMs):
    // Recovery finishes, signal transitions to YELLOW
    controller.Tick(100);
    assert(!controller.IsRecoveryActive());
    assert(controller.GetCurrentState() == SignalState::YELLOW);
    assert(mockPtr->calls.back() == "SetYellow");

    // Advance 100ms in YELLOW: transitions to RED
    controller.Tick(100);
    assert(controller.GetCurrentState() == SignalState::RED);
    assert(mockPtr->calls.back() == "SetRed");

    // Normal cycle resumes from RED
    controller.Tick(200);
    assert(controller.GetCurrentState() == SignalState::GREEN);

    controller.Stop();
    std::cout << "  -> PASS: Emergency preemption and recovery cycle verified." << std::endl;
}

void TestTrafficControllerLifecycle() {
    std::cout << "[TEST] 6. TrafficController Lifecycle & Edge Cases..." << std::endl;

    TrafficController controller;
    assert(controller.GetCurrentState() == SignalState::RED);

    assert(controller.Initialize());
    assert(controller.Start());

    // Test timing configuration update
    TrafficTimingConfig newConfig;
    newConfig.redDurationMs = 50;
    newConfig.greenDurationMs = 50;
    newConfig.yellowDurationMs = 50;
    controller.SetTimingConfig(newConfig);

    controller.Tick(50);
    assert(controller.GetCurrentState() == SignalState::GREEN);

    controller.Stop();
    assert(controller.GetCurrentState() == SignalState::GREEN || controller.GetCurrentState() == SignalState::RED);

    controller.Shutdown();
    assert(controller.GetCurrentState() == SignalState::UNKNOWN);

    std::cout << "  -> PASS: TrafficController lifecycle verified." << std::endl;
}

void TestTrafficSignalNodeLifecycle() {
    std::cout << "[TEST] 7. TrafficSignalNode Node Execution & Wrapping..." << std::endl;

    TrafficTimingConfig config;
    config.redDurationMs = 20;
    config.greenDurationMs = 20;
    config.yellowDurationMs = 20;

    TrafficSignalNode node(config);
    assert(!node.IsRunning());

    assert(node.Start());
    assert(node.IsRunning());
    assert(node.GetCurrentState() == SignalState::RED);

    // Step 20ms: RED -> GREEN
    node.Tick(20);
    assert(node.GetCurrentState() == SignalState::GREEN);

    // Run for bounded iterations
    node.Run(3, 20); // 20ms -> YELLOW, 20ms -> RED, 20ms -> GREEN
    assert(node.GetCurrentState() == SignalState::GREEN);

    node.Stop();
    assert(!node.IsRunning());

    std::cout << "  -> PASS: TrafficSignalNode lifecycle verified." << std::endl;
}

void TestMqttConnectionAndLifecycle() {
    std::cout << "[TEST] 8. MQTT Connection, Lifecycle & Topics..." << std::endl;

    TrafficSignalNode node("INT-01");
    assert(!node.IsConnected());
    assert(node.GetCommandTopic() == "aegis/intersection/INT-01/commands");
    assert(node.GetStatusTopic() == "aegis/intersection/INT-01/status");

    // Connect
    assert(node.Connect("127.0.0.1", 1883, false));
    assert(node.IsConnected());

    // Disconnect
    node.Disconnect();
    assert(!node.IsConnected());

    // Reconnect
    assert(node.Reconnect());
    assert(node.IsConnected());

    node.Disconnect();
    std::cout << "  -> PASS: MQTT Connection lifecycle verified." << std::endl;
}

void TestMqttSubscriptionAndPublication() {
    std::cout << "[TEST] 9. MQTT Subscription, Publication & Status Schema..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    std::vector<std::string> receivedStatus;
    netManager->Subscribe("aegis/intersection/INT-02/status", [&](const std::string&, const std::string& payload) {
        receivedStatus.push_back(payload);
    });

    TrafficSignalNode node("INT-02", netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    // Force publish status
    assert(node.PublishStatus(true));
    assert(!receivedStatus.empty());

    // Verify deserialization using Serializer
    auto parseRes = aegis::network::Serializer::DeserializeIntersectionStatus(receivedStatus.back());
    assert(parseRes.IsOk());
    auto status = parseRes.Value();
    assert(status.intersectionId == "INT-02");
    assert(status.currentState == "RED");
    assert(!status.emergencyActive);
    assert(!status.recoveryActive);
    assert(status.health == "HEALTHY");

    node.Stop();
    std::cout << "  -> PASS: MQTT Subscription and Status publication verified." << std::endl;
}

void TestPreemptAndClearCommands() {
    std::cout << "[TEST] 10. PREEMPT & CLEAR Inbound Commands..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    std::vector<std::string> statusUpdates;
    netManager->Subscribe("aegis/intersection/INT-03/status", [&](const std::string&, const std::string& payload) {
        statusUpdates.push_back(payload);
    });

    TrafficTimingConfig config;
    config.allRedClearanceMs = 100;
    config.recoveryDurationMs = 100;

    TrafficSignalNode node("INT-03", std::make_unique<TrafficController>(CreateSimulationGpio(), config), netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    assert(node.GetCurrentState() == SignalState::RED);

    // 1. Send PREEMPT command (raw string)
    assert(node.ProcessCommand("PREEMPT"));
    assert(node.GetController().IsPreemptionActive());
    assert(node.GetCurrentState() == SignalState::ALL_RED);

    // Verify published status reflects emergency_active == true
    assert(!statusUpdates.empty());
    auto status1 = aegis::network::Serializer::DeserializeIntersectionStatus(statusUpdates.back()).Value();
    assert(status1.emergencyActive);
    assert(status1.currentState == "ALL_RED");

    // Advance clearance to reach GREEN
    node.Tick(100);
    assert(node.GetCurrentState() == SignalState::GREEN);

    // 2. Send CLEAR command (JSON format: {"command": "CLEAR"})
    assert(node.ProcessCommand("{\"command\": \"CLEAR\"}"));
    assert(!node.GetController().IsPreemptionActive());
    assert(node.GetController().IsRecoveryActive());

    auto status2 = aegis::network::Serializer::DeserializeIntersectionStatus(statusUpdates.back()).Value();
    assert(!status2.emergencyActive);
    assert(status2.recoveryActive);

    // Advance recovery to finish
    node.Tick(100);
    assert(!node.GetController().IsRecoveryActive());
    assert(node.GetCurrentState() == SignalState::YELLOW);

    node.Stop();
    std::cout << "  -> PASS: PREEMPT & CLEAR commands verified." << std::endl;
}

void TestRestorePingStatusCommands() {
    std::cout << "[TEST] 11. RESTORE, PING, & STATUS_REQUEST Commands..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    std::vector<std::string> statusList;
    netManager->Subscribe("aegis/intersection/INT-04/status", [&](const std::string&, const std::string& payload) {
        statusList.push_back(payload);
    });

    TrafficSignalNode node("INT-04", netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    // Force preemption then RESTORE
    node.ProcessCommand("PREEMPT");
    assert(node.GetCurrentState() == SignalState::ALL_RED);

    // RESTORE should reset directly to RED and normal cycle
    assert(node.ProcessCommand("RESTORE"));
    assert(node.GetCurrentState() == SignalState::RED);
    assert(!node.GetController().IsPreemptionActive());
    assert(!node.GetController().IsRecoveryActive());

    // PING should publish immediately
    size_t countBeforePing = statusList.size();
    assert(node.ProcessCommand("PING"));
    assert(statusList.size() > countBeforePing);

    // STATUS_REQUEST should publish complete state immediately
    size_t countBeforeReq = statusList.size();
    assert(node.ProcessCommand("STATUS_REQUEST"));
    assert(statusList.size() > countBeforeReq);

    auto finalStatus = aegis::network::Serializer::DeserializeIntersectionStatus(statusList.back()).Value();
    assert(finalStatus.intersectionId == "INT-04");
    assert(finalStatus.currentState == "RED");

    node.Stop();
    std::cout << "  -> PASS: RESTORE, PING, & STATUS_REQUEST commands verified." << std::endl;
}

void TestHeartbeatAndTimeoutFailSafe() {
    std::cout << "[TEST] 12. Periodic Heartbeat & 5s Timeout Fail-Safe (FLASHING_YELLOW)..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    TrafficSignalNode node("INT-05", netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    uint64_t initialHb = node.GetHeartbeatCounter();

    // Advance 1000ms: node should send heartbeat
    node.Tick(1000);
    assert(node.GetHeartbeatCounter() > initialHb);

    // Keep advancing without coordinator heartbeat up to 4000ms: still healthy
    node.Tick(3000);
    assert(node.GetCurrentState() != SignalState::FLASHING_YELLOW);
    assert(node.GetHealthStatus() == "HEALTHY");

    // Advance past 5000ms (total 5000ms elapsed without coordinator heartbeat)
    node.Tick(1000);
    // Timeout should trigger FLASHING_YELLOW
    assert(node.GetCurrentState() == SignalState::FLASHING_YELLOW);
    assert(node.GetHealthStatus() == "DEGRADED");

    // While in fail-safe, further ticks remain in FLASHING_YELLOW
    node.Tick(2000);
    assert(node.GetCurrentState() == SignalState::FLASHING_YELLOW);

    // Coordinator sends HEARTBEAT command -> communication returns!
    assert(node.ProcessCommand("HEARTBEAT"));
    assert(node.GetCurrentState() == SignalState::RED);
    assert(node.GetHealthStatus() == "HEALTHY");

    node.Stop();
    std::cout << "  -> PASS: Heartbeat and timeout fail-safe verified." << std::endl;
}

void TestCoordinatorReconnectAndStateSync() {
    std::cout << "[TEST] 13. Coordinator Reconnect & State Synchronization..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    std::vector<std::string> syncMessages;
    netManager->Subscribe("aegis/intersection/INT-06/status", [&](const std::string&, const std::string& payload) {
        syncMessages.push_back(payload);
    });

    TrafficSignalNode node("INT-06", netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    // Put node in emergency preemption
    node.ProcessCommand("PREEMPT");
    size_t countBeforeSync = syncMessages.size();

    // Simulate coordinator reconnected and requesting status
    node.ProcessCommand("STATUS_REQUEST");
    assert(syncMessages.size() > countBeforeSync);

    auto syncStatus = aegis::network::Serializer::DeserializeIntersectionStatus(syncMessages.back()).Value();
    assert(syncStatus.intersectionId == "INT-06");
    assert(syncStatus.emergencyActive);
    assert(syncStatus.currentState == "ALL_RED");

    node.Stop();
    std::cout << "  -> PASS: Coordinator Reconnect & State Synchronization verified." << std::endl;
}

void TestNetworkReconnectAndContinuity() {
    std::cout << "[TEST] 14. Network Reconnect & Local Traffic Continuity..." << std::endl;

    TrafficTimingConfig config;
    config.redDurationMs = 100;
    config.greenDurationMs = 100;
    config.yellowDurationMs = 100;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    TrafficSignalNode node("INT-07", std::make_unique<TrafficController>(CreateSimulationGpio(), config), netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    // Initial state: RED
    assert(node.GetCurrentState() == SignalState::RED);

    // Network disconnects!
    node.Disconnect();
    assert(!node.IsConnected());

    // Local controller must MAINTAIN state and continue cycling
    node.Tick(100);
    assert(node.GetCurrentState() == SignalState::GREEN);

    node.Tick(100);
    assert(node.GetCurrentState() == SignalState::YELLOW);

    // Network reconnects
    assert(node.Reconnect());
    assert(node.IsConnected());

    // Controller preserved state (still in YELLOW or advanced cleanly)
    assert(node.GetCurrentState() == SignalState::YELLOW);

    node.Stop();
    std::cout << "  -> PASS: Network Reconnect & Local Continuity verified." << std::endl;
}

void TestDuplicatePublishSuppression() {
    std::cout << "[TEST] 15. Duplicate Publication Suppression..." << std::endl;

    auto netManager = std::make_shared<aegis::network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "COORD-TEST");

    std::vector<std::string> published;
    netManager->Subscribe("aegis/intersection/INT-08/status", [&](const std::string&, const std::string& payload) {
        published.push_back(payload);
    });

    TrafficSignalNode node("INT-08", netManager);
    assert(node.Start());
    assert(node.Connect("127.0.0.1", 1883, false));

    size_t count1 = published.size();
    // First publish without force should be suppressed if identical to connect-publish
    bool publishedAgain = node.PublishStatus(false);
    assert(!publishedAgain);
    assert(published.size() == count1); // No duplicate published!

    // Force publish must bypass suppression
    bool forcedPublish = node.PublishStatus(true);
    assert(forcedPublish);
    assert(published.size() == count1 + 1);

    node.Stop();
    std::cout << "  -> PASS: Duplicate publication suppression verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - Phase 5 Traffic Signal Node Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // Phase 5A Foundation Tests
    TestGpioAbstractionAndLogging();
    TestInvalidTransitions();
    TestPreemptionAndRecoveryManagers();
    TestStateTransitionsNormalCycle();
    TestEmergencyActivationAndRecovery();
    TestTrafficControllerLifecycle();
    TestTrafficSignalNodeLifecycle();

    // Phase 5B MQTT Integration Tests
    TestMqttConnectionAndLifecycle();
    TestMqttSubscriptionAndPublication();
    TestPreemptAndClearCommands();
    TestRestorePingStatusCommands();
    TestHeartbeatAndTimeoutFailSafe();
    TestCoordinatorReconnectAndStateSync();
    TestNetworkReconnectAndContinuity();
    TestDuplicatePublishSuppression();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL 15 TESTS PASSED] Phase 5A & 5B Fully Verified!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}

