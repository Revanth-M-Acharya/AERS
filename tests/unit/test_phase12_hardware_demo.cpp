/**
 * @file test_phase12_hardware_demo.cpp
 * @brief Complete unit and integration test suite for Phase 12 Hardware Demonstration.
 * Verifies buzzer patterns, emergency/police switches, OLED display, priority arbitration,
 * police-assisted autonomous replanning without pause, and the 5-mission DEMO MODE suite.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/demo/DemoHardwareSignals.hpp"
#include "hardware_integration/demo/BuzzerController.hpp"
#include "hardware_integration/demo/EmergencySwitchDriver.hpp"
#include "hardware_integration/demo/PoliceOverrideSwitchDriver.hpp"
#include "hardware_integration/demo/OledV2XDisplay.hpp"
#include "hardware_integration/demo/Intersection2PriorityDemonstrator.hpp"
#include "hardware_integration/demo/PoliceReplanningCoordinator.hpp"
#include "hardware_integration/demo/HardwareDemoOrchestrator.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>

using namespace aegis;
using namespace aegis::coordinator;
using namespace aegis::digital_twin;
using namespace aegis::hardware_integration;
using namespace aegis::hardware_integration::demo;

static void LogPass(const std::string& testName) {
    std::cout << "  [PASS] " << testName << std::endl;
}

// ==============================================================================
// TEST 1: Buzzer Controller Patterns & Deterministic Rhythms
// ==============================================================================
static void TestBuzzerPatterns() {
    std::cout << "[TEST 1] Buzzer Controller - 5 Demonstration Patterns..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    BuzzerController buzzer(hal, DemoSignals::COORDINATOR_BUZZER);
    assert(buzzer.Initialize());
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::OFF);
    assert(!buzzer.IsPlaying());

    // Pattern 1: Continuous Tone (Corridor active)
    buzzer.PlayPattern(BuzzerPattern::CONTINUOUS_TONE, 0);
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::CONTINUOUS_TONE);
    assert(buzzer.IsPlaying());
    buzzer.StepSimulation(50);
    assert(hal->GetSignal(DemoSignals::COORDINATOR_BUZZER) == true);
    buzzer.Stop();
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::OFF);
    assert(!buzzer.IsPlaying());
    assert(hal->GetSignal(DemoSignals::COORDINATOR_BUZZER) == false);

    // Pattern 2: Three Short Beeps (Priority conflict)
    buzzer.PlayPattern(BuzzerPattern::THREE_SHORT_BEEPS, 1);
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::THREE_SHORT_BEEPS);
    assert(buzzer.IsPlaying());
    buzzer.StepSimulation(120); // Beep 1
    buzzer.StepSimulation(100); // Pause
    buzzer.StepSimulation(120); // Beep 2
    buzzer.StepSimulation(100); // Pause
    buzzer.StepSimulation(120); // Beep 3
    buzzer.StepSimulation(200); // Sequence complete
    assert(buzzer.GetToggleCount() > 0);

    // Pattern 3: Rapid Beeps (Police event / replanning)
    buzzer.PlayPattern(BuzzerPattern::RAPID_BEEPS, 1);
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::RAPID_BEEPS);
    buzzer.StepSimulation(600);

    // Pattern 4: Two Long Beeps (Mission complete)
    buzzer.PlayPattern(BuzzerPattern::TWO_LONG_BEEPS, 1);
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::TWO_LONG_BEEPS);
    buzzer.StepSimulation(1200);

    // Pattern 5: Alternating Warning (Critical fault)
    buzzer.PlayPattern(BuzzerPattern::ALTERNATING_WARNING, 1);
    assert(buzzer.GetCurrentPattern() == BuzzerPattern::ALTERNATING_WARNING);
    buzzer.StepSimulation(600);

    buzzer.Stop();
    assert(!buzzer.IsPlaying());
    LogPass("All 5 buzzer preemption patterns sequenced and stepped accurately");
}

// ==============================================================================
// TEST 2: Emergency Switch Driver (Debouncing & Mission Trigger)
// ==============================================================================
static void TestEmergencySwitchDriver() {
    std::cout << "[TEST 2] Emergency Switch Driver - Debouncing & Intake Trigger..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    EmergencySwitchConfig cfg;
    cfg.debounceMs = 50;
    EmergencySwitchDriver sw(hal, cfg);
    assert(sw.Initialize());

    bool callbackTriggered = false;
    uint64_t triggerTs = 0;
    sw.RegisterCallback([&](uint64_t ts) {
        callbackTriggered = true;
        triggerTs = ts;
    });

    // 1. Initial state released
    assert(!sw.IsPressed());
    assert(sw.GetTotalTriggers() == 0);

    // 2. Contact bounce (glitch under 50ms)
    hal->SetSignal(cfg.symbolicSignal, true);
    assert(!sw.Poll(100)); // Change detected, timer started
    assert(!sw.Poll(120)); // Only 20ms elapsed -> filtered
    assert(!sw.IsPressed());

    // 3. Steady press exceeding 50ms
    assert(sw.Poll(160)); // 60ms elapsed >= 50ms -> valid press
    assert(sw.IsPressed());
    assert(sw.GetTotalTriggers() == 1);
    assert(callbackTriggered);
    assert(triggerTs == 160);

    // 4. Release
    hal->SetSignal(cfg.symbolicSignal, false);
    assert(!sw.Poll(200));
    assert(!sw.Poll(260));
    assert(!sw.IsPressed());

    // 5. SimulatePress programmatic helper
    sw.SimulatePress(300);
    assert(sw.IsPressed());
    assert(sw.GetTotalTriggers() == 2);
    assert(sw.GetLastTriggerTimestampMs() == 300);

    LogPass("Emergency switch debouncing and trigger dispatch verified");
}

// ==============================================================================
// TEST 3: Police Override Switch Driver (Advisory Event)
// ==============================================================================
static void TestPoliceOverrideSwitchDriver() {
    std::cout << "[TEST 3] Police Override Switch Driver - Verified Road Information..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    PoliceOverrideSwitchConfig cfg;
    cfg.defaultTargetIntersection = "INT_2";
    cfg.defaultReason = "TEMPORARY_POLICE_BARRICADE";
    cfg.debounceMs = 50;

    PoliceOverrideSwitchDriver sw(hal, cfg);
    assert(sw.Initialize());

    std::string capturedIntersection;
    std::string capturedReason;
    uint64_t capturedTs = 0;

    sw.RegisterCallback([&](const std::string& intId, const std::string& r, uint64_t ts) {
        capturedIntersection = intId;
        capturedReason = r;
        capturedTs = ts;
    });

    assert(!sw.IsPressed());
    assert(sw.GetTotalReports() == 0);

    // Trigger programmatic press
    sw.SimulatePress(500, "POLICE_ROAD_CLOSED_INT2");
    assert(sw.IsPressed());
    assert(sw.GetTotalReports() == 1);
    assert(capturedIntersection == "INT_2");
    assert(capturedReason == "POLICE_ROAD_CLOSED_INT2");
    assert(capturedTs == 500);

    LogPass("Police override switch generates verified road event without manual override");
}

// ==============================================================================
// TEST 4: OLED Display Simulator (V2X Warnings)
// ==============================================================================
static void TestOledV2XDisplay() {
    std::cout << "[TEST 4] OLED V2X Display Simulator..." << std::endl;

    OledV2XDisplay oled;
    assert(oled.Initialize());
    assert(oled.GetActiveMode() == V2XAdvisoryMode::IDLE);

    // 1. Ambulance Approaching warning
    oled.ShowAmbulanceApproaching();
    assert(oled.GetActiveMode() == V2XAdvisoryMode::AMBULANCE_APPROACHING);
    auto lines = oled.GetCurrentLines();
    assert(lines.size() >= 4);
    assert(lines[0] == "AMBULANCE");
    assert(lines[1] == "APPROACHING");
    assert(lines[3] == "MOVE LEFT");

    std::string ascii = oled.RenderAscii();
    assert(ascii.find("AMBULANCE") != std::string::npos);
    assert(ascii.find("MOVE LEFT") != std::string::npos);

    // 2. Green Corridor Active advisory
    oled.ShowGreenCorridorActive();
    assert(oled.GetActiveMode() == V2XAdvisoryMode::GREEN_CORRIDOR_ACTIVE);
    assert(oled.GetCurrentLines()[0] == "GREEN");
    assert(oled.GetCurrentLines()[1] == "CORRIDOR");

    // 3. Mission Complete
    oled.ShowMissionComplete();
    assert(oled.GetActiveMode() == V2XAdvisoryMode::MISSION_COMPLETE);
    assert(oled.GetCurrentLines()[0] == "MISSION");
    assert(oled.GetCurrentLines()[1] == "COMPLETE");

    // 4. Police Replanning
    oled.ShowPoliceReplanning("NEW ETA: 42s");
    assert(oled.GetActiveMode() == V2XAdvisoryMode::POLICE_REPLANNING);
    assert(oled.GetCurrentLines()[0] == "POLICE BARRICADE");

    // 5. Clear
    oled.Clear();
    assert(oled.GetActiveMode() == V2XAdvisoryMode::IDLE);
    assert(oled.GetUpdateCount() >= 5);

    LogPass("OLED V2X warning sequences and ASCII rendering verified");
}

// ==============================================================================
// TEST 5: Intersection 2 Dual-Ambulance Priority Scheduling
// ==============================================================================
static void TestIntersection2PriorityScheduling() {
    std::cout << "[TEST 5] Intersection 2 Priority Scheduling Demonstration..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    auto scheduler = std::make_shared<coordinator::intelligence::PriorityScheduler>();
    auto buzzer = std::make_shared<BuzzerController>(hal);
    buzzer->Initialize();
    auto oled = std::make_shared<OledV2XDisplay>();
    oled->Initialize();

    Intersection2PriorityDemonstrator demonstrator(scheduler, buzzer, oled);

    // Execute arbitration
    auto details = demonstrator.DemonstratePriorityConflict(1000);

    // Mission A (Critical / Code Red) vs Mission B (High / Code Yellow)
    assert(details.intersectionId == "INT_2");
    assert(details.selectedAmbulanceId == "AMB_01");
    assert(details.selectedMissionId == "MISSION_A");
    assert(details.estimatedWaitTimeSec == 15.0);
    assert(details.selectionReason.find("Higher medical triage priority") != std::string::npos);

    // Check hardware feedback
    assert(buzzer->GetCurrentPattern() == BuzzerPattern::THREE_SHORT_BEEPS);
    assert(oled->GetActiveMode() == V2XAdvisoryMode::PRIORITY_ARBITRATION);

    // Verify dashboard panel format
    std::string panel = demonstrator.RenderDashboardPanel();
    assert(panel.find("INTERSECTION 2 PRIORITY SCHEDULING") != std::string::npos);
    assert(panel.find("Priority 1") != std::string::npos);
    assert(panel.find("Priority 2") != std::string::npos);
    assert(panel.find("Selected Mission") != std::string::npos);
    assert(panel.find("15.0 sec") != std::string::npos);

    LogPass("Intersection 2 priority conflict arbitrated correctly with 15s wait time for held ambulance");
}

// ==============================================================================
// TEST 6: Police-Assisted Dynamic Route Replanning (Zero Pause Invariant)
// ==============================================================================
static void TestPoliceDynamicReplanning() {
    std::cout << "[TEST 6] Police-Assisted Dynamic Route Replanning (Zero Pause Invariant)..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    // Setup road graph
    auto graph = std::make_shared<coordinator::intelligence::RoadGraph>();
    using namespace coordinator::intelligence;
    graph->AddIntersection({"HOME", "Ambulance Station", 12.9700, 77.5900, true, {}});
    graph->AddIntersection({"INT_1", "Intersection 1", 12.9710, 77.5920, true, {}});
    graph->AddIntersection({"INT_2", "Intersection 2", 12.9720, 77.5940, true, {}});
    graph->AddIntersection({"INT_3", "Intersection 3", 12.9730, 77.5960, true, {}});
    graph->AddIntersection({"INT_BYPASS", "Bypass Detour", 12.9740, 77.5930, true, {}});
    graph->AddIntersection({"HOSPITAL", "City Hospital", 12.9750, 77.5990, true, {}});

    graph->AddRoadSegment({"SEG_HOME_INT1", "HOME", "INT_1", 300.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph->AddRoadSegment({"SEG_INT1_INT2", "INT_1", "INT_2", 400.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph->AddRoadSegment({"SEG_INT2_INT3", "INT_2", "INT_3", 400.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph->AddRoadSegment({"SEG_INT3_HOSP", "INT_3", "HOSPITAL", 350.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph->AddRoadSegment({"SEG_INT1_BYPASS", "INT_1", "INT_BYPASS", 500.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});
    graph->AddRoadSegment({"SEG_BYPASS_INT3", "INT_BYPASS", "INT_3", 500.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2});

    auto routePlanner = std::make_shared<DijkstraRoutePlanner>();
    auto corridorPlanner = std::make_shared<GreenCorridorPlanner>();
    auto eventBus = std::make_shared<coordinator::orchestration::EventBus>();
    auto digitalTwin = std::make_shared<digital_twin::DigitalTwinEngine>(eventBus);
    digitalTwin->Initialize();

    auto buzzer = std::make_shared<BuzzerController>(hal);
    buzzer->Initialize();
    auto oled = std::make_shared<OledV2XDisplay>();
    oled->Initialize();

    auto relayInt2 = std::make_shared<ProductionRelayDriver>(hal);
    relayInt2->Initialize();
    relayInt2->SetEmergencyPreemption(TrafficApproach::ROAD_A);

    PoliceReplanningCoordinator replanner(
        graph, routePlanner, corridorPlanner, eventBus, digitalTwin, buzzer, oled);
    replanner.RegisterNodeRelayDriver("INT_2", relayInt2);

    // Initial shortest path: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL
    auto initialRoute = routePlanner->ComputeShortestPath("HOME", "HOSPITAL", *graph);
    assert(initialRoute.success);
    assert(initialRoute.pathIntersectionIds[2] == "INT_2");

    // Execute Police Replanning
    auto report = replanner.ExecutePoliceReplanning(
        "INT_2",
        "TEMPORARY POLICE BARRICADE / ROAD CLOSED",
        2000,
        "INT_1");

    // CRITICAL ARCHITECTURAL ASSERTIONS
    assert(report.success);
    assert(report.missionPaused == false);               // ZERO PAUSE
    assert(report.coordinatorAuthorityRetained == true); // COORDINATOR IS SOLE AUTHORITY
    assert(report.newEtaSec == 42.0);                    // NEW ETA
    assert(report.replanDurationMicroseconds < 5000);    // ULTRA-LOW LATENCY (<5ms)

    // Bypass route must avoid INT_2 and traverse INT_BYPASS
    bool foundBypass = false;
    for (const auto& node : report.newRouteNodes) {
        assert(node != "INT_2"); // Blocked node must NOT be in path
        if (node == "INT_BYPASS") foundBypass = true;
    }
    assert(foundBypass);

    // Blocked intersection signals restored to normal
    assert(relayInt2->GetCurrentMode() == RelayOperationalMode::NORMAL);
    assert(!relayInt2->IsEmergencyPreemptionActive());

    // Hardware Feedback
    assert(buzzer->GetCurrentPattern() == BuzzerPattern::RAPID_BEEPS);
    assert(oled->GetActiveMode() == V2XAdvisoryMode::POLICE_REPLANNING);

    // Dashboard Output
    std::string dash = replanner.RenderDashboardDisplay();
    assert(dash.find("POLICE EVENT") != std::string::npos);
    assert(dash.find("Road Closed") != std::string::npos);
    assert(dash.find("Intersection 2") != std::string::npos);
    assert(dash.find("42 sec") != std::string::npos);
    assert(dash.find("New Corridor Generated") != std::string::npos);

    LogPass("Police dynamic replanning executed with zero pause, new ETA 42s, and 100% Coordinator authority");
}

// ==============================================================================
// TEST 7: Hardware Demo Orchestrator (Full 5-Mission Suite)
// ==============================================================================
static void TestHardwareDemoOrchestrator_FullSuite() {
    std::cout << "[TEST 7] Hardware Demo Orchestrator - Full 5-Mission DEMO MODE..." << std::endl;

    auto hal = std::make_shared<SymbolicGpioHAL>();
    hal->Initialize();

    HardwareDemoOrchestrator orchestrator(hal, nullptr, 0); // 0ms delay for unit testing
    assert(orchestrator.Initialize());

    auto results = orchestrator.ExecuteFullDemoSuite();
    assert(results.size() == 5);

    // Mission 1: Normal Corridor
    assert(results[0].missionId == DemoMissionId::MISSION_1_NORMAL_CORRIDOR);
    assert(results[0].success);

    // Mission 2: Priority Scheduling
    assert(results[1].missionId == DemoMissionId::MISSION_2_PRIORITY_SCHEDULING);
    assert(results[1].success);

    // Mission 3: Police Replanning
    assert(results[2].missionId == DemoMissionId::MISSION_3_POLICE_REPLANNING);
    assert(results[2].success);

    // Mission 4: Fault Injection
    assert(results[3].missionId == DemoMissionId::MISSION_4_FAULT_INJECTION);
    assert(results[3].success);

    // Mission 5: Mission Replay
    assert(results[4].missionId == DemoMissionId::MISSION_5_MISSION_REPLAY);
    assert(results[4].success);

    LogPass("Full 5-Mission DEMO MODE executed seamlessly with 100% pass rate");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 12 Hardware Demonstration Test Suite  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    try {
        TestBuzzerPatterns();
        TestEmergencySwitchDriver();
        TestPoliceOverrideSwitchDriver();
        TestOledV2XDisplay();
        TestIntersection2PriorityScheduling();
        TestPoliceDynamicReplanning();
        TestHardwareDemoOrchestrator_FullSuite();

        std::cout << "==========================================================" << std::endl;
        std::cout << " ALL PHASE 12 HARDWARE DEMO TESTS PASSED SUCCESSFULLY!    " << std::endl;
        std::cout << "==========================================================" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FATAL EXCEPTION in Phase 12 tests: " << e.what() << std::endl;
        return 1;
    }
}
