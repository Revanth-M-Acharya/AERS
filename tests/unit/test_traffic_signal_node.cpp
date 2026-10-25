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

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - Phase 5A Traffic Signal Node Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestGpioAbstractionAndLogging();
    TestInvalidTransitions();
    TestPreemptionAndRecoveryManagers();
    TestStateTransitionsNormalCycle();
    TestEmergencyActivationAndRecovery();
    TestTrafficControllerLifecycle();
    TestTrafficSignalNodeLifecycle();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [ALL TESTS PASSED] Phase 5A Foundation Fully Verified!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
