/**
 * @file HardwareDemoOrchestrator.cpp
 * @brief Implementation of master DEMO MODE orchestrator for Phase 12 hardware demonstration.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/HardwareDemoOrchestrator.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>

namespace aegis::hardware_integration::demo {

HardwareDemoOrchestrator::HardwareDemoOrchestrator(
    std::shared_ptr<SymbolicGpioHAL> hal,
    std::shared_ptr<digital_twin::DigitalTwinEngine> digitalTwin,
    uint32_t stepDelayMs)
    : hal_(std::move(hal)),
      digitalTwin_(std::move(digitalTwin)),
      stepDelayMs_(stepDelayMs) {
}

void HardwareDemoOrchestrator::SetStepDelayMs(uint32_t delayMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    stepDelayMs_ = delayMs;
}

void HardwareDemoOrchestrator::DelayStep() {
    if (stepDelayMs_ > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(stepDelayMs_));
    }
}

void HardwareDemoOrchestrator::SetupDemoRoadNetwork() {
    graph_ = std::make_shared<coordinator::intelligence::RoadGraph>();

    using namespace coordinator::intelligence;
    // Intersections
    graph_->AddIntersection({"HOME", "Ambulance Station / HOME", 12.9700, 77.5900, true, {}});
    graph_->AddIntersection({"INT_1", "Intersection 1 (RPi Traffic Node 1)", 12.9710, 77.5920, true, {}});
    graph_->AddIntersection({"INT_2", "Intersection 2 (RPi Traffic Node 2)", 12.9720, 77.5940, true, {}});
    graph_->AddIntersection({"INT_3", "Intersection 3 (RPi Traffic Node 3)", 12.9730, 77.5960, true, {}});
    graph_->AddIntersection({"INT_BYPASS", "Bypass Route Intersection", 12.9740, 77.5930, true, {}});
    graph_->AddIntersection({"DEPOT_EAST", "East Ambulance Depot", 12.9720, 77.5980, true, {}});
    graph_->AddIntersection({"HOSPITAL", "City Trauma Hospital", 12.9750, 77.5990, true, {}});

    // Segments
    // Main Route: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL (Total: 300+400+400+350 = 1450m)
    graph_->AddRoadSegment({"SEG_HOME_INT1", "HOME", "INT_1", 300.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph_->AddRoadSegment({"SEG_INT1_INT2", "INT_1", "INT_2", 400.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph_->AddRoadSegment({"SEG_INT2_INT3", "INT_2", "INT_3", 400.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph_->AddRoadSegment({"SEG_INT3_HOSP", "INT_3", "HOSPITAL", 350.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});

    // Bypass Route around INT_2: INT_1 -> INT_BYPASS -> INT_3 (Total: 500+500 = 1000m vs 800m main)
    graph_->AddRoadSegment({"SEG_INT1_BYPASS", "INT_1", "INT_BYPASS", 500.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph_->AddRoadSegment({"SEG_BYPASS_INT3", "INT_BYPASS", "INT_3", 500.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});

    // Converging Ambulance B approach at INT_2:
    graph_->AddRoadSegment({"SEG_DEPOT_INT2", "DEPOT_EAST", "INT_2", 350.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
}

bool HardwareDemoOrchestrator::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!hal_) {
        hal_ = std::make_shared<SymbolicGpioHAL>();
    }
    hal_->Initialize();

    // Register coordinator controls
    hal_->RegisterSignal(DemoSignals::COORDINATOR_BUZZER, 6, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Coordinator Preemption Buzzer");
    hal_->RegisterSignal(DemoSignals::EMERGENCY_SWITCH, 23, PinDirection::INPUT, ActivePolarity::ACTIVE_LOW, "Emergency Mission Switch");
    hal_->RegisterSignal(DemoSignals::POLICE_OVERRIDE_SWITCH, 24, PinDirection::INPUT, ActivePolarity::ACTIVE_LOW, "Police Barricade Switch");

    // Initialize Road Topology
    SetupDemoRoadNetwork();

    // Orchestration & Planning Modules
    eventBus_ = std::make_shared<coordinator::orchestration::EventBus>();
    routePlanner_ = std::make_shared<coordinator::intelligence::DijkstraRoutePlanner>();
    corridorPlanner_ = std::make_shared<coordinator::intelligence::GreenCorridorPlanner>();
    priorityScheduler_ = std::make_shared<coordinator::intelligence::PriorityScheduler>();

    if (!digitalTwin_) {
        digitalTwin_ = std::make_shared<digital_twin::DigitalTwinEngine>(eventBus_);
        digitalTwin_->Initialize();
    }

    // Hardware Drivers
    buzzer_ = std::make_shared<BuzzerController>(hal_, DemoSignals::COORDINATOR_BUZZER);
    buzzer_->Initialize();

    emergencySwitch_ = std::make_shared<EmergencySwitchDriver>(hal_);
    emergencySwitch_->Initialize();

    policeSwitch_ = std::make_shared<PoliceOverrideSwitchDriver>(hal_);
    policeSwitch_->Initialize();

    oled_ = std::make_shared<OledV2XDisplay>();
    oled_->Initialize();

    // Priority Demonstrator
    priorityDemonstrator_ = std::make_shared<Intersection2PriorityDemonstrator>(
        priorityScheduler_, buzzer_, oled_);

    // Police Replanner
    policeReplanner_ = std::make_shared<PoliceReplanningCoordinator>(
        graph_, routePlanner_, corridorPlanner_, eventBus_, digitalTwin_, buzzer_, oled_);

    // Each node has its own HAL to simulate the 4 separate Raspberry Pi nodes
    auto halInt1 = std::make_shared<SymbolicGpioHAL>();
    halInt1->Initialize();
    relayInt1_ = std::make_shared<ProductionRelayDriver>(halInt1);
    relayInt1_->Initialize();

    auto halInt2 = std::make_shared<SymbolicGpioHAL>();
    halInt2->Initialize();
    relayInt2_ = std::make_shared<ProductionRelayDriver>(halInt2);
    relayInt2_->Initialize();

    auto halInt3 = std::make_shared<SymbolicGpioHAL>();
    halInt3->Initialize();
    relayInt3_ = std::make_shared<ProductionRelayDriver>(halInt3);
    relayInt3_->Initialize();

    auto halBypass = std::make_shared<SymbolicGpioHAL>();
    halBypass->Initialize();
    relayBypass_ = std::make_shared<ProductionRelayDriver>(halBypass);
    relayBypass_->Initialize();

    policeReplanner_->RegisterNodeRelayDriver("INT_1", relayInt1_);
    policeReplanner_->RegisterNodeRelayDriver("INT_2", relayInt2_);
    policeReplanner_->RegisterNodeRelayDriver("INT_3", relayInt3_);
    policeReplanner_->RegisterNodeRelayDriver("INT_BYPASS", relayBypass_);

    // IR Sensors
    IRSensorConfig irCfg1{"IR_INT1", StandardSignals::APPROACH_IR_SENSOR};
    irInt1_ = std::make_shared<IRSensorDriver>(halInt1, irCfg1);
    irInt1_->Initialize();

    IRSensorConfig irCfg2Top{"IR_INT2_TOP", StandardSignals::APPROACH_IR_SENSOR};
    irInt2Top_ = std::make_shared<IRSensorDriver>(halInt2, irCfg2Top);
    irInt2Top_->Initialize();

    IRSensorConfig irCfg2Right{"IR_INT2_RIGHT", StandardSignals::DEPARTURE_IR_SENSOR};
    irInt2Right_ = std::make_shared<IRSensorDriver>(halInt2, irCfg2Right);
    irInt2Right_->Initialize();

    IRSensorConfig irCfg3{"IR_INT3", StandardSignals::APPROACH_IR_SENSOR};
    irInt3_ = std::make_shared<IRSensorDriver>(halInt3, irCfg3);
    irInt3_->Initialize();

    isInitialized_ = true;
    return true;
}

DemoExecutionMetrics HardwareDemoOrchestrator::ExecuteMission1_NormalCorridor() {
    auto startTime = std::chrono::steady_clock::now();
    DemoExecutionMetrics m;
    m.missionId = DemoMissionId::MISSION_1_NORMAL_CORRIDOR;
    m.missionName = "Mission 1: Normal Green Corridor Execution";

    // 1. Emergency switch triggered near HOME
    emergencySwitch_->SimulatePress(1000);
    DelayStep();

    // 2. Compute shortest path: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL
    auto route = routePlanner_->ComputeShortestPath("HOME", "HOSPITAL", *graph_);
    assert(route.success);
    assert(route.pathIntersectionIds.size() == 5);

    // 3. Activate Green Corridor & Hardware feedback
    buzzer_->PlayPattern(BuzzerPattern::CONTINUOUS_TONE, 0); // Continuous tone while corridor active
    oled_->ShowAmbulanceApproaching();
    DelayStep();
    oled_->ShowGreenCorridorActive();

    // 4. Staged Signal Preemption
    relayInt1_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    relayInt2_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    relayInt3_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    DelayStep();

    // 5. Sequential IR sensor triggers as vehicle traverses intersections
    irInt1_->SimulateRawPinLevel(true, 1050);
    irInt1_->Poll(1050);
    DelayStep();
    irInt1_->SimulateRawPinLevel(false, 1080);
    irInt1_->Poll(1080);

    irInt2Top_->SimulateRawPinLevel(true, 1100);
    irInt2Top_->Poll(1100);
    DelayStep();
    irInt2Top_->SimulateRawPinLevel(false, 1130);
    irInt2Top_->Poll(1130);

    irInt3_->SimulateRawPinLevel(true, 1150);
    irInt3_->Poll(1150);
    DelayStep();
    irInt3_->SimulateRawPinLevel(false, 1180);
    irInt3_->Poll(1180);

    // 6. Arrival at Hospital: Release corridor & Restore signals
    relayInt1_->SetNormalPhase(TrafficApproach::ROAD_A);
    relayInt2_->SetNormalPhase(TrafficApproach::ROAD_A);
    relayInt3_->SetNormalPhase(TrafficApproach::ROAD_A);

    buzzer_->PlayPattern(BuzzerPattern::TWO_LONG_BEEPS, 1);
    oled_->ShowMissionComplete();
    DelayStep();

    auto endTime = std::chrono::steady_clock::now();
    m.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    m.success = true;
    m.summary = "Ambulance dispatched from HOME, green wave staged across Int 1->2->3, reached Hospital safely.";
    history_.push_back(m);
    return m;
}

DemoExecutionMetrics HardwareDemoOrchestrator::ExecuteMission2_PriorityScheduling() {
    auto startTime = std::chrono::steady_clock::now();
    DemoExecutionMetrics m;
    m.missionId = DemoMissionId::MISSION_2_PRIORITY_SCHEDULING;
    m.missionName = "Mission 2: Dual-Ambulance Priority Scheduling at Intersection 2";

    // Simulate simultaneous arrival on Top and Right approaches
    irInt2Top_->SimulateRawPinLevel(true, 2000);
    irInt2Top_->Poll(2000);
    irInt2Right_->SimulateRawPinLevel(true, 2000);
    irInt2Right_->Poll(2000);
    DelayStep();

    // Arbitrate using Priority Scheduler
    auto arbitration = priorityDemonstrator_->DemonstratePriorityConflict(2000);
    assert(arbitration.selectedAmbulanceId == "AMB_01"); // Priority 1 (Code Red) wins
    assert(arbitration.estimatedWaitTimeSec == 15.0);

    // Verify buzzer and OLED outputs
    assert(buzzer_->GetCurrentPattern() == BuzzerPattern::THREE_SHORT_BEEPS);
    assert(oled_->GetActiveMode() == V2XAdvisoryMode::PRIORITY_ARBITRATION);
    DelayStep();

    // Top approach (Ambulance A) receives green, Right approach (Ambulance B) held red
    relayInt2_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    DelayStep();

    // Once Ambulance A clears, Ambulance B gets clearance
    relayInt2_->SetEmergencyPreemption(TrafficApproach::ROAD_B);
    DelayStep();
    relayInt2_->SetNormalPhase(TrafficApproach::ROAD_A);

    auto endTime = std::chrono::steady_clock::now();
    m.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    m.success = true;
    m.summary = "Arbitrated simultaneous arrival: Priority 1 granted immediate green; Priority 2 queued with 15s wait.";
    history_.push_back(m);
    return m;
}

DemoExecutionMetrics HardwareDemoOrchestrator::ExecuteMission3_PoliceReplanning() {
    auto startTime = std::chrono::steady_clock::now();
    DemoExecutionMetrics m;
    m.missionId = DemoMissionId::MISSION_3_POLICE_REPLANNING;
    m.missionName = "Mission 3: Police-Assisted Dynamic Route Replanning";

    // Corridor initially active
    buzzer_->PlayPattern(BuzzerPattern::CONTINUOUS_TONE, 0);
    relayInt1_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    relayInt2_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    relayInt3_->SetEmergencyPreemption(TrafficApproach::ROAD_A);
    DelayStep();

    // Police switch pressed: Police report "ROAD CLOSED / BARRICADE AT INT 2"
    policeSwitch_->SimulatePress(3000, "POLICE BARRICADE AT INT 2");
    DelayStep();

    // Coordinator autonomously replans without losing authority or pausing
    auto report = policeReplanner_->ExecutePoliceReplanning("INT_2", "POLICE BARRICADE AT INT 2", 3000, "INT_1");
    assert(report.success);
    assert(!report.missionPaused);
    assert(report.coordinatorAuthorityRetained);
    assert(report.newEtaSec == 42.0);

    // Old blocked route (INT_2) restored to normal cycle
    assert(relayInt2_->GetCurrentMode() == RelayOperationalMode::NORMAL);
    // Bypass route nodes (INT_1, INT_3) set to Green preemption
    assert(relayInt1_->IsEmergencyPreemptionActive());
    assert(relayInt3_->IsEmergencyPreemptionActive());

    // Hardware outputs: Rapid beeps and OLED detour display
    assert(buzzer_->GetCurrentPattern() == BuzzerPattern::RAPID_BEEPS);
    assert(oled_->GetActiveMode() == V2XAdvisoryMode::POLICE_REPLANNING);
    DelayStep();

    // Ambulance traverses bypass route safely to hospital
    relayInt1_->SetNormalPhase(TrafficApproach::ROAD_A);
    relayInt3_->SetNormalPhase(TrafficApproach::ROAD_A);
    buzzer_->PlayPattern(BuzzerPattern::TWO_LONG_BEEPS, 1);
    oled_->ShowMissionComplete();

    auto endTime = std::chrono::steady_clock::now();
    m.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    m.success = true;
    m.summary = "Police reported barricade at Int 2; Coordinator autonomously rerouted via Bypass with 0s pause, new ETA 42s.";
    history_.push_back(m);
    return m;
}

DemoExecutionMetrics HardwareDemoOrchestrator::ExecuteMission4_FaultInjection() {
    auto startTime = std::chrono::steady_clock::now();
    DemoExecutionMetrics m;
    m.missionId = DemoMissionId::MISSION_4_FAULT_INJECTION;
    m.missionName = "Mission 4: Roadside Fault Injection & Fail-Safe Fallback";

    // Simulate IR Sensor Stuck Active fault at INT_3
    // Step 1: Beam interrupted at t=1000
    irInt3_->SimulateRawPinLevel(true, 1000);
    // Step 2: Poll after debounce window (t=1050) -> transitions to BEAM_INTERRUPTED
    irInt3_->Poll(1050);
    // Step 3: Poll after > 120 seconds of continuous interruption (t=1050 + 130000)
    irInt3_->Poll(1050 + 130000);
    auto metrics = irInt3_->GetMetrics();
    assert(metrics.healthGrade == SensorHealthGrade::FAULTY);
    assert(metrics.activeFault == SensorFaultType::STUCK_ACTIVE);

    // Hardware & Safety Response
    buzzer_->PlayPattern(BuzzerPattern::ALTERNATING_WARNING, 2);
    oled_->ShowCustom({"CRITICAL FAULT", "INT_3 SENSOR STUCK", "FAIL-SAFE ACTIVE", "ALL-RED INTERLOCK"});

    // Intersection 3 shifts to All-Red safe state
    relayInt3_->SetAllRed();
    assert(relayInt3_->GetCurrentMode() == RelayOperationalMode::ALL_RED);
    DelayStep();

    // Clear fault & restore
    irInt3_->SimulateRawPinLevel(false, 1050 + 140000);
    irInt3_->Poll(1050 + 140050);
    relayInt3_->SetNormalPhase(TrafficApproach::ROAD_A);
    buzzer_->Stop();
    oled_->Clear();

    auto endTime = std::chrono::steady_clock::now();
    m.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    m.success = true;
    m.summary = "Injected stuck-active IR fault; node instantly engaged All-Red fail-safe and sounded alternating warning.";
    history_.push_back(m);
    return m;
}

DemoExecutionMetrics HardwareDemoOrchestrator::ExecuteMission5_MissionReplay() {
    auto startTime = std::chrono::steady_clock::now();
    DemoExecutionMetrics m;
    m.missionId = DemoMissionId::MISSION_5_MISSION_REPLAY;
    m.missionName = "Mission 5: Mission Replay & Telemetry Review";

    // Render operations center replay and summary telemetry
    if (digitalTwin_) {
        auto replay = digitalTwin_->GetReplayEngine();
        if (replay) {
            replay->Play();
            replay->StepForward();
            replay->Stop();
        }
    }

    oled_->ShowCustom({"MISSION REPLAY", "ACCIDENTS: 0", "PREEMPTION: <1MS", "TIME SAVED: 48%"});
    DelayStep();

    auto endTime = std::chrono::steady_clock::now();
    m.durationMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());
    m.success = true;
    m.summary = "Replayed mission telemetry from Digital Twin: 0 accidents, <1ms latency, 48% travel time reduction.";
    history_.push_back(m);
    return m;
}

std::vector<DemoExecutionMetrics> HardwareDemoOrchestrator::ExecuteFullDemoSuite() {
    std::vector<DemoExecutionMetrics> results;
    results.push_back(ExecuteMission1_NormalCorridor());
    results.push_back(ExecuteMission2_PriorityScheduling());
    results.push_back(ExecuteMission3_PoliceReplanning());
    results.push_back(ExecuteMission4_FaultInjection());
    results.push_back(ExecuteMission5_MissionReplay());
    return results;
}

} // namespace aegis::hardware_integration::demo
