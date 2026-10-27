/**
 * @file test_phase8_intelligent_scheduler.cpp
 * @brief Comprehensive Unit and Integration Test Suite for Phase 8:
 * Multi-Ambulance Conflict Resolution and Intelligent Priority Scheduling.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>

#include "ambulance/AmbulanceProfile.h"
#include "ambulance/AmbulanceRegistry.h"
#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"
#include "scheduler/MissionManager.h"
#include "scheduler/ConflictDetectionEngine.h"
#include "scheduler/IntelligentPriorityScheduler.h"
#include "scheduler/ConflictResolutionManager.h"
#include "scheduler/ManualOverrideManager.h"
#include "scheduler/AmbulanceAwarenessService.h"
#include "scheduler/MultiAmbulanceSimulation.h"
#include "logger/EventLogger.h"

using namespace aegis;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[-] ASSERTION FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            return false; \
        } \
    } while(0)

// =============================================================================
// Test 1: AmbulanceRegistry Phase 8 Enhancements
// =============================================================================
bool TestAmbulanceRegistryPhase8() {
    std::cout << "[TEST] 1. AmbulanceRegistry Phase 8 Attributes and Telemetry Mutators...\n";

    ambulance::AmbulanceRegistry registry;
    ambulance::AmbulanceProfile amb("AMB-101", ambulance::AmbulanceStatus::AVAILABLE, "Central Hospital", 5);

    // Initial state
    TEST_ASSERT(amb.GetAmbulanceId() == "AMB-101", "Ambulance ID mismatch");
    TEST_ASSERT(amb.GetOperationalState() == ambulance::AmbulanceStatus::AVAILABLE, "Initial state mismatch");
    TEST_ASSERT(amb.GetCurrentMission().empty(), "Mission should initially be empty");
    TEST_ASSERT(amb.GetCurrentSpeed() == 0.0, "Speed should initially be 0");
    TEST_ASSERT(amb.GetPriorityScore() == 0.0, "Priority score should initially be 0");
    TEST_ASSERT(amb.GetCorridorId().empty(), "Corridor ID should initially be empty");

    // Assign Patient & calculate medical score
    medical::PatientProfile patient;
    patient.SetPatientId("PAT-001");
    patient.SetPatientName("Alice Smith");
    patient.SetAge(68);
    patient.SetGender(medical::Gender::FEMALE);
    patient.SetDisease(medical::EmergencyType::HEART_ATTACK);
    patient.SetHeartRate(125);
    patient.SetBloodPressure(165, 95);
    patient.SetSpO2(88);
    patient.SetEstimatedTimeSinceIncident(45);
    amb.SetPatient(patient);

    TEST_ASSERT(amb.HasPatient(), "Patient should be assigned");
    TEST_ASSERT(amb.GetPriority() == medical::PriorityLevel::HIGH, "Heart attack priority should be HIGH");
    TEST_ASSERT(amb.GetPriorityScore() > 50.0, "Priority score should be computed from patient");

    // Test explicit mutators on AmbulanceProfile
    amb.SetCurrentMission("MIS-ALPHA");
    amb.SetCurrentSpeed(68.5);
    amb.SetCorridorId("CORR-GREEN-1");
    amb.SetCurrentIntersection("INT_A");
    amb.SetOperationalState(ambulance::AmbulanceStatus::TRANSPORTING);

    TEST_ASSERT(amb.GetCurrentMission() == "MIS-ALPHA", "Mission ID mismatch");
    TEST_ASSERT(std::abs(amb.GetCurrentSpeed() - 68.5) < 0.01, "Speed mismatch");
    TEST_ASSERT(amb.GetCorridorId() == "CORR-GREEN-1", "Corridor ID mismatch");
    TEST_ASSERT(amb.GetCurrentIntersection() == "INT_A", "Current intersection mismatch");
    TEST_ASSERT(amb.GetOperationalState() == ambulance::AmbulanceStatus::TRANSPORTING, "Operational state mismatch");

    // Test JSON Serialization includes new fields
    std::string json = amb.ToJson();
    TEST_ASSERT(json.find("\"current_mission\":\"MIS-ALPHA\"") != std::string::npos, "JSON missing current_mission");
    TEST_ASSERT(json.find("\"current_speed_kmh\":68.5") != std::string::npos, "JSON missing speed");
    TEST_ASSERT(json.find("\"corridor_id\":\"CORR-GREEN-1\"") != std::string::npos, "JSON missing corridor_id");

    // Register into AmbulanceRegistry and test registry-level mutators
    TEST_ASSERT(registry.RegisterAmbulance(amb), "Registration failed");
    TEST_ASSERT(registry.GetAmbulanceCount() == 1, "Count should be 1");

    TEST_ASSERT(registry.UpdateSpeed("AMB-101", 72.0), "UpdateSpeed failed");
    TEST_ASSERT(registry.UpdateMission("AMB-101", "MIS-BETA"), "UpdateMission failed");
    TEST_ASSERT(registry.UpdatePriorityScore("AMB-101", 85.5), "UpdatePriorityScore failed");
    TEST_ASSERT(registry.UpdateCorridor("AMB-101", "CORR-GREEN-2"), "UpdateCorridor failed");
    TEST_ASSERT(registry.UpdateOperationalState("AMB-101", ambulance::AmbulanceStatus::AT_HOSPITAL), "UpdateOperationalState failed");

    auto retrieved = registry.SearchAmbulance("AMB-101");
    TEST_ASSERT(retrieved.has_value(), "Ambulance not found");
    TEST_ASSERT(std::abs(retrieved->GetCurrentSpeed() - 72.0) < 0.01, "Updated speed mismatch");
    TEST_ASSERT(retrieved->GetCurrentMission() == "MIS-BETA", "Updated mission mismatch");
    TEST_ASSERT(std::abs(retrieved->GetPriorityScore() - 85.5) < 0.01, "Updated score mismatch");
    TEST_ASSERT(retrieved->GetCorridorId() == "CORR-GREEN-2", "Updated corridor mismatch");
    TEST_ASSERT(retrieved->GetOperationalState() == ambulance::AmbulanceStatus::AT_HOSPITAL, "Updated state mismatch");

    std::cout << "  PASS: AmbulanceRegistry storing and updating complete vehicle telemetry and state.\n";
    return true;
}

// =============================================================================
// Test 2: MissionManager Lifecycle Management
// =============================================================================
bool TestMissionManagerLifecycle() {
    std::cout << "[TEST] 2. MissionManager Lifecycle States (ACTIVE, WAITING, COMPLETED, CANCELLED, PAUSED)...\n";

    scheduler::MissionManager manager;

    // Create mission
    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C"};
    bool created = manager.CreateMission(
        "MIS-100", "AMB-201", "PAT-501", route, "St. Jude Hospital", 75.0, "CORR-10", "Cardiac event transfer"
    );
    TEST_ASSERT(created, "Failed to create mission");
    TEST_ASSERT(manager.GetMissionCount() == 1, "Count should be 1");

    auto mOpt = manager.GetMission("MIS-100");
    TEST_ASSERT(mOpt.has_value(), "Mission should be found");
    TEST_ASSERT(mOpt->state == scheduler::MissionState::ACTIVE, "Initial mission state should be ACTIVE");
    TEST_ASSERT(mOpt->startTimeEpochMs > 0, "Start time should be initialized");

    // Transition to WAITING
    TEST_ASSERT(manager.SetMissionWaiting("MIS-100", "Waiting for junction clearance"), "Set waiting failed");
    mOpt = manager.GetMission("MIS-100");
    TEST_ASSERT(mOpt->state == scheduler::MissionState::WAITING, "State should be WAITING");
    TEST_ASSERT(mOpt->reason == "Waiting for junction clearance", "Reason mismatch");

    // Transition to PAUSED
    TEST_ASSERT(manager.PauseMission("MIS-100", "Held by traffic command"), "Pause failed");
    mOpt = manager.GetMission("MIS-100");
    TEST_ASSERT(mOpt->state == scheduler::MissionState::PAUSED, "State should be PAUSED");

    // Transition back to ACTIVE (Resume)
    TEST_ASSERT(manager.ResumeMission("MIS-100"), "Resume failed");
    mOpt = manager.GetMission("MIS-100");
    TEST_ASSERT(mOpt->state == scheduler::MissionState::ACTIVE, "State should be ACTIVE");

    // Complete mission
    TEST_ASSERT(manager.CompleteMission("MIS-100"), "Complete failed");
    mOpt = manager.GetMission("MIS-100");
    TEST_ASSERT(mOpt->state == scheduler::MissionState::COMPLETED, "State should be COMPLETED");
    TEST_ASSERT(mOpt->endTimeEpochMs > 0, "End time should be recorded");

    // Create second mission and Cancel
    manager.CreateMission("MIS-200", "AMB-202", "PAT-502", route, "Metro General", 40.0);
    TEST_ASSERT(manager.CancelMission("MIS-200", "Call cancelled by dispatcher"), "Cancel failed");
    auto m2Opt = manager.GetMission("MIS-200");
    TEST_ASSERT(m2Opt->state == scheduler::MissionState::CANCELLED, "State should be CANCELLED");

    // Query filters
    TEST_ASSERT(manager.GetActiveMissions().empty(), "No active missions should remain");
    TEST_ASSERT(manager.GetMissionsByState(scheduler::MissionState::COMPLETED).size() == 1, "Should have 1 COMPLETED");
    TEST_ASSERT(manager.GetMissionsByState(scheduler::MissionState::CANCELLED).size() == 1, "Should have 1 CANCELLED");

    std::cout << "  PASS: MissionManager lifecycle states and queries verified successfully.\n";
    return true;
}

// =============================================================================
// Test 3: ConflictDetectionEngine Window Detection & Event Generation
// =============================================================================
bool TestConflictDetectionEngine() {
    std::cout << "[TEST] 3. ConflictDetectionEngine Time-Window Detection and Conflict Events...\n";

    scheduler::ConflictDetectionEngine engine(10); // 10s window
    TEST_ASSERT(engine.GetConflictWindowSeconds() == 10, "Window should be 10s");

    // Register two arrivals at INT_A within window (ETA 12s and 18s -> delta 6s <= 10s)
    engine.RegisterArrival("AMB-1", "INT_A", 12, 180.0);
    engine.RegisterArrival("AMB-2", "INT_A", 18, 250.0);

    // Register one arrival at INT_B (no conflict)
    engine.RegisterArrival("AMB-3", "INT_B", 30, 500.0);

    bool listenerTriggered = false;
    std::string detectedIntId = "";
    engine.AddConflictListener([&](const scheduler::ConflictEvent& evt) {
        listenerTriggered = true;
        detectedIntId = evt.intersectionId;
    });

    auto conflicts = engine.DetectConflicts();
    TEST_ASSERT(conflicts.size() == 1, "Exactly one conflict should be detected");
    TEST_ASSERT(conflicts.front().intersectionId == "INT_A", "Conflict intersection should be INT_A");
    TEST_ASSERT(conflicts.front().GetConflictCount() == 2, "Conflict should involve 2 ambulances");
    TEST_ASSERT(conflicts.front().timeDeltaSeconds == 6, "Time delta should be 6s");
    TEST_ASSERT(conflicts.front().HasAmbulance("AMB-1"), "Should include AMB-1");
    TEST_ASSERT(conflicts.front().HasAmbulance("AMB-2"), "Should include AMB-2");
    TEST_ASSERT(listenerTriggered, "Conflict listener callback should have fired");
    TEST_ASSERT(detectedIntId == "INT_A", "Listener intersection mismatch");

    // Verify history and query
    TEST_ASSERT(engine.HasConflictAt("INT_A"), "Should report conflict at INT_A");
    TEST_ASSERT(!engine.HasConflictAt("INT_B"), "Should not report conflict at INT_B");
    TEST_ASSERT(engine.GetConflictHistory().size() == 1, "History count should be 1");

    // Test arrivals outside window (ETA 10s and 30s -> delta 20s > 10s)
    engine.Clear();
    engine.RegisterArrival("AMB-4", "INT_C", 10, 100.0);
    engine.RegisterArrival("AMB-5", "INT_C", 30, 400.0);
    auto noConflicts = engine.DetectConflicts();
    TEST_ASSERT(noConflicts.empty(), "Should not detect conflict outside time window");

    std::cout << "  PASS: ConflictDetectionEngine window contention and event generation verified.\n";
    return true;
}

// =============================================================================
// Test 4: IntelligentPriorityScheduler Multi-Factor Decision Engine
// =============================================================================
bool TestIntelligentPriorityScheduler() {
    std::cout << "[TEST] 4. IntelligentPriorityScheduler Multi-Factor Arbitration & Determinism...\n";

    scheduler::IntelligentPriorityScheduler scheduler(15);

    // Case A: Medical Priority Score Dominates
    // Candidate 1: Cardiac Arrest (score 92), ETA 20s
    // Candidate 2: Heart Attack (score 70), ETA 12s
    {
        scheduler::SchedulingCandidate c1("AMB-CARDIAC", 92.0, medical::PriorityLevel::CRITICAL, 20);
        scheduler::SchedulingCandidate c2("AMB-HEART", 70.0, medical::PriorityLevel::HIGH, 12);

        auto dec = scheduler.ScheduleIntersection("INT_X", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-CARDIAC", "Higher medical score must win despite later ETA");
        TEST_ASSERT(dec.HasDelayedAmbulance("AMB-HEART"), "AMB-HEART must be delayed");
        TEST_ASSERT(dec.GetAssignedDelay("AMB-HEART") == 15, "Delay should be 15s");
    }

    // Case B: Clinical Score Tie -> Estimated Arrival Time (ETA) Arbitrates
    // Candidate 1: Stroke (score 70), ETA 15s
    // Candidate 2: Heart Attack (score 70), ETA 25s
    {
        scheduler::SchedulingCandidate c1("AMB-STROKE", 70.0, medical::PriorityLevel::HIGH, 15);
        scheduler::SchedulingCandidate c2("AMB-HEART", 70.0, medical::PriorityLevel::HIGH, 25);

        auto dec = scheduler.ScheduleIntersection("INT_Y", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-STROKE", "Earlier arrival (15s vs 25s) must win on score tie");
    }

    // Case C: Waiting Time Urgency Prevents Starvation
    // Candidate 1: Major Accident (score 65), Waiting: 0s -> Effective = 65
    // Candidate 2: Pediatric Emergency (score 55), Waiting: 30s -> Effective = 55 + (30 * 0.5) = 70
    {
        scheduler::SchedulingCandidate c1("AMB-ACCIDENT", 65.0, medical::PriorityLevel::HIGH, 15, 0);
        scheduler::SchedulingCandidate c2("AMB-PEDIATRIC", 55.0, medical::PriorityLevel::HIGH, 15, 30);

        auto dec = scheduler.ScheduleIntersection("INT_Z", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-PEDIATRIC", "Waiting bonus (70 vs 65) must elevate starving unit");
    }

    // Case D: Manual Operator Override Strictly Dominates Everything
    // Candidate 1: Cardiac Arrest (score 95, CRITICAL), Manual Override 0
    // Candidate 2: Organ Transport (score 50, MEDIUM), Manual Override 2
    {
        scheduler::SchedulingCandidate c1("AMB-CARDIAC", 95.0, medical::PriorityLevel::CRITICAL, 10, 0, 0);
        scheduler::SchedulingCandidate c2("AMB-ORGAN", 50.0, medical::PriorityLevel::MEDIUM, 15, 0, 2);

        auto dec = scheduler.ScheduleIntersection("INT_W", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-ORGAN", "Manual override level 2 must strictly dominate");
    }

    // Case E: Corridor Occupancy Right-of-Way
    // Candidate 1: Already inside intersection corridor (occupied = true)
    // Candidate 2: Approaching (occupied = false, higher score)
    {
        scheduler::SchedulingCandidate c1("AMB-OCCUPYING", 60.0, medical::PriorityLevel::HIGH, 5, 0, 0, true);
        scheduler::SchedulingCandidate c2("AMB-APPROACH", 80.0, medical::PriorityLevel::CRITICAL, 10, 0, 0, false);

        auto dec = scheduler.ScheduleIntersection("INT_V", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-OCCUPYING", "Occupying unit must clear junction first");
    }

    // Case F: Mission Status Priority (ACTIVE over WAITING / PAUSED)
    {
        scheduler::SchedulingCandidate c1("AMB-ACTIVE", 60.0, medical::PriorityLevel::HIGH, 10, 0, 0, false, scheduler::MissionState::ACTIVE);
        scheduler::SchedulingCandidate c2("AMB-PAUSED", 80.0, medical::PriorityLevel::CRITICAL, 10, 0, 0, false, scheduler::MissionState::PAUSED);

        auto dec = scheduler.ScheduleIntersection("INT_U", {c1, c2});
        TEST_ASSERT(dec.winnerAmbulanceId == "AMB-ACTIVE", "ACTIVE mission must take precedence over PAUSED mission");
    }

    // Case G: Deterministic Tie-Breaking (Identical clinical, ETA, distance)
    {
        scheduler::SchedulingCandidate cA("AMB-A", 70.0, medical::PriorityLevel::HIGH, 15, 0, 0, false, scheduler::MissionState::ACTIVE, 200.0);
        scheduler::SchedulingCandidate cB("AMB-B", 70.0, medical::PriorityLevel::HIGH, 15, 0, 0, false, scheduler::MissionState::ACTIVE, 200.0);

        auto dec1 = scheduler.ScheduleIntersection("INT_T", {cA, cB});
        auto dec2 = scheduler.ScheduleIntersection("INT_T", {cB, cA});
        TEST_ASSERT(dec1.winnerAmbulanceId == "AMB-A", "Deterministic tie-breaking winner AMB-A");
        TEST_ASSERT(dec2.winnerAmbulanceId == "AMB-A", "Deterministic tie-breaking consistent regardless of input order");
    }

    // Case H: Strict Single Green Grant Guarantee
    TEST_ASSERT(scheduler.IsGreenGranted("AMB-A", "INT_T"), "AMB-A should hold green grant");
    TEST_ASSERT(!scheduler.IsGreenGranted("AMB-B", "INT_T"), "AMB-B must NOT hold green grant");
    TEST_ASSERT(scheduler.GetGrantedAmbulance("INT_T") == "AMB-A", "Granted ambulance mismatch");

    std::cout << "  PASS: IntelligentPriorityScheduler deterministic multi-factor arbitration verified.\n";
    return true;
}

// =============================================================================
// Test 5: ConflictResolutionManager Command Generation & Coordinator Integration
// =============================================================================
bool TestConflictResolutionManager() {
    std::cout << "[TEST] 5. ConflictResolutionManager Commands (PROCEED, HOLD, DELAY, RESUME, ALTERNATE_ROUTE)...\n";

    auto coordinator = std::make_shared<coordinator::CoordinatorManager>();
    scheduler::ConflictResolutionManager resManager(coordinator);
    TEST_ASSERT(resManager.HasCoordinator(), "Coordinator should be linked");

    // Test explicit command generators
    auto proceedCmd = resManager.GenerateProceedCommand("AMB-01", "INT_A", "Corridor clear");
    TEST_ASSERT(proceedCmd.action == scheduler::ResolutionAction::PROCEED, "Action should be PROCEED");
    TEST_ASSERT(proceedCmd.ambulanceId == "AMB-01", "Ambulance ID mismatch");

    auto holdCmd = resManager.GenerateHoldCommand("AMB-02", "INT_A", "Stop at stopline");
    TEST_ASSERT(holdCmd.action == scheduler::ResolutionAction::HOLD, "Action should be HOLD");

    auto delayCmd = resManager.GenerateDelayCommand("AMB-03", "INT_A", 25, "Decelerate window");
    TEST_ASSERT(delayCmd.action == scheduler::ResolutionAction::DELAY, "Action should be DELAY");
    TEST_ASSERT(delayCmd.delaySeconds == 25, "Delay should be 25s");

    auto resumeCmd = resManager.GenerateResumeCommand("AMB-02", "INT_A", "Corridor resumed");
    TEST_ASSERT(resumeCmd.action == scheduler::ResolutionAction::RESUME, "Action should be RESUME");

    std::vector<std::string> altRoute = {"INT_A_BYPASS", "INT_B"};
    auto altCmd = resManager.GenerateAlternateRouteCommand("AMB-04", "INT_A", altRoute, "Heavy congestion");
    TEST_ASSERT(altCmd.action == scheduler::ResolutionAction::ALTERNATE_ROUTE, "Action should be ALTERNATE_ROUTE");
    TEST_ASSERT(altCmd.alternateRoute.size() == 2, "Alternate route stops mismatch");

    // Test resolution commands generated from a scheduling decision
    scheduler::IntelligentScheduleDecision decision;
    decision.intersectionId = "INT_NODE_1";
    decision.winnerAmbulanceId = "AMB_WINNER";
    decision.delayedAmbulanceIds = {"AMB_HOLD_UNIT", "AMB_DELAY_UNIT", "AMB_DIVERT_UNIT"};
    decision.staggeredDelaysSeconds["AMB_HOLD_UNIT"] = 15;
    decision.staggeredDelaysSeconds["AMB_DELAY_UNIT"] = 30;
    decision.staggeredDelaysSeconds["AMB_DIVERT_UNIT"] = 60; // Exceeds threshold (45s)

    std::unordered_map<std::string, std::vector<std::string>> routes;
    routes["AMB_DIVERT_UNIT"] = {"INT_BYPASS_1", "INT_HOSPITAL"};

    auto commands = resManager.GenerateResolutionCommands(decision, routes);
    TEST_ASSERT(commands.size() == 4, "Should generate 4 commands (1 proceed + 3 delayed)");

    TEST_ASSERT(commands[0].action == scheduler::ResolutionAction::PROCEED, "Winner gets PROCEED");
    TEST_ASSERT(commands[0].ambulanceId == "AMB_WINNER", "Winner ID mismatch");

    TEST_ASSERT(commands[1].action == scheduler::ResolutionAction::HOLD, "15s delay gets HOLD");
    TEST_ASSERT(commands[1].ambulanceId == "AMB_HOLD_UNIT", "Ambulance ID mismatch");

    TEST_ASSERT(commands[2].action == scheduler::ResolutionAction::DELAY, "30s delay gets DELAY");
    TEST_ASSERT(commands[2].ambulanceId == "AMB_DELAY_UNIT", "Ambulance ID mismatch");

    TEST_ASSERT(commands[3].action == scheduler::ResolutionAction::ALTERNATE_ROUTE, "60s delay gets ALTERNATE_ROUTE");
    TEST_ASSERT(commands[3].ambulanceId == "AMB_DIVERT_UNIT", "Ambulance ID mismatch");

    TEST_ASSERT(resManager.GetTotalCommandCount() >= 9, "Total commands recorded mismatch");

    std::cout << "  PASS: ConflictResolutionManager all 5 action commands verified.\n";
    return true;
}

// =============================================================================
// Test 6: ManualOverrideManager & EventLogger Integration
// =============================================================================
bool TestManualOverrideManager() {
    std::cout << "[TEST] 6. ManualOverrideManager Operator Interventions & EventLogger Auditing...\n";

    auto registry = std::make_shared<ambulance::AmbulanceRegistry>();
    ambulance::AmbulanceProfile amb("AMB-OVERRIDE", ambulance::AmbulanceStatus::TRANSPORTING, "Trauma Center", 8);
    amb.SetPriority(medical::PriorityLevel::LOW);
    registry->RegisterAmbulance(amb);

    scheduler::ManualOverrideManager overrideMgr(registry);

    // Apply priority override
    bool ok = overrideMgr.OverridePriority("AMB-OVERRIDE", medical::PriorityLevel::CRITICAL, "OP-99", "Mass casualty incident nearby");
    TEST_ASSERT(ok, "OverridePriority failed");
    TEST_ASSERT(overrideMgr.HasOverride("AMB-OVERRIDE"), "Should have active override");
    TEST_ASSERT(overrideMgr.GetOverrideLevel("AMB-OVERRIDE") == 1, "Level should be 1");

    // Check registry was updated
    auto updated = registry->SearchAmbulance("AMB-OVERRIDE");
    TEST_ASSERT(updated.has_value(), "Ambulance should be found");
    TEST_ASSERT(updated->GetPriority() == medical::PriorityLevel::CRITICAL, "Priority should be elevated to CRITICAL");
    TEST_ASSERT(updated->GetManualOverrideLevel() == 1, "Ambulance manual override level should be 1");

    // Apply level override
    overrideMgr.ApplyOverrideLevel("AMB-OVERRIDE", 3, "OP-99", "State Governor escort");
    TEST_ASSERT(overrideMgr.GetOverrideLevel("AMB-OVERRIDE") == 3, "Level should be 3");

    // Clear override
    overrideMgr.ClearOverride("AMB-OVERRIDE", "OP-99");
    TEST_ASSERT(!overrideMgr.HasOverride("AMB-OVERRIDE"), "Override should be cleared");
    updated = registry->SearchAmbulance("AMB-OVERRIDE");
    TEST_ASSERT(updated->GetManualOverrideLevel() == 0, "Level should be reset to 0");

    // Verify history preserved
    TEST_ASSERT(overrideMgr.GetOverrideHistory().size() >= 2, "History should be preserved");

    std::cout << "  PASS: ManualOverrideManager operator control and registry synchronization verified.\n";
    return true;
}

// =============================================================================
// Test 7: AmbulanceAwarenessService Peer Proximity Advisories
// =============================================================================
bool TestAmbulanceAwarenessService() {
    std::cout << "[TEST] 7. AmbulanceAwarenessService Peer Approach Advisories...\n";

    scheduler::AmbulanceAwarenessService service;

    ambulance::AmbulanceProfile p1("AMB-A", ambulance::AmbulanceStatus::TRANSPORTING);
    p1.SetPriority(medical::PriorityLevel::CRITICAL);
    p1.SetPriorityScore(95.0);

    ambulance::AmbulanceProfile p2("AMB-B", ambulance::AmbulanceStatus::TRANSPORTING);
    p2.SetPriority(medical::PriorityLevel::HIGH);
    p2.SetPriorityScore(70.0);

    scheduler::AmbulanceArrival arr1("AMB-A", "INT_SHARED", 12, 200.0, p1);
    scheduler::AmbulanceArrival arr2("AMB-B", "INT_SHARED", 15, 250.0, p2);

    bool received = false;
    bool notifValid = true;
    service.AddListener([&](const scheduler::AwarenessNotification& notif) {
        received = true;
        if (notif.targetAmbulanceId.empty() || notif.approachingAmbulanceId.empty() || notif.intersectionId != "INT_SHARED") {
            notifValid = false;
        }
    });

    auto notifs = service.GenerateAwarenessFromArrivals("INT_SHARED", {arr1, arr2}, "CONFLICT_DETECTED");
    TEST_ASSERT(notifs.size() == 2, "Should generate 2 mutual notifications (A->B and B->A)");
    TEST_ASSERT(received, "Observer listener should have fired");
    TEST_ASSERT(notifValid, "Notification attributes should be valid");

    // Notification for AMB-A should advise about AMB-B
    auto notifsA = service.GetNotificationsForAmbulance("AMB-A");
    TEST_ASSERT(notifsA.size() == 1, "AMB-A should have 1 notification");
    TEST_ASSERT(notifsA.front().approachingAmbulanceId == "AMB-B", "AMB-A should be notified about AMB-B");
    TEST_ASSERT(notifsA.front().approachingPriority == medical::PriorityLevel::HIGH, "Approaching priority mismatch");
    TEST_ASSERT(notifsA.front().approachingEtaSeconds == 15, "Approaching ETA mismatch");

    // Notification for AMB-B should advise about AMB-A
    auto notifsB = service.GetNotificationsForAmbulance("AMB-B");
    TEST_ASSERT(notifsB.size() == 1, "AMB-B should have 1 notification");
    TEST_ASSERT(notifsB.front().approachingAmbulanceId == "AMB-A", "AMB-B should be notified about AMB-A");
    TEST_ASSERT(notifsB.front().approachingPriority == medical::PriorityLevel::CRITICAL, "Approaching priority mismatch");
    TEST_ASSERT(notifsB.front().approachingEtaSeconds == 12, "Approaching ETA mismatch");

    std::cout << "  PASS: AmbulanceAwarenessService peer proximity notifications verified.\n";
    return true;
}

// =============================================================================
// Test 8: MultiAmbulanceSimulation 8 Simultaneous Clinical Emergencies
// =============================================================================
bool TestMultiAmbulanceSimulationEightEmergencies() {
    std::cout << "[TEST] 8. MultiAmbulanceSimulation Demonstrating 8 Clinical Emergency Models Simultaneously...\n";

    scheduler::MultiAmbulanceSimulation sim;

    // Verify 8 clinical emergencies setup
    sim.SetupEightClinicalEmergencies();
    const auto& ambulances = sim.GetSimulatedAmbulances();
    TEST_ASSERT(ambulances.size() == 8, "Must setup exactly 8 emergency ambulances");

    std::vector<std::string> expectedDiseases = {
        "Cardiac Arrest", "Internal Bleeding", "Heart Attack", "Stroke",
        "Major Accident", "Pediatric Emergency", "Organ Transport", "Pregnancy"
    };

    for (const auto& expected : expectedDiseases) {
        bool found = false;
        for (const auto& amb : ambulances) {
            if (amb.diseaseName == expected) {
                found = true;
                break;
            }
        }
        TEST_ASSERT(found, ("Expected emergency disease not found: " + expected).c_str());
    }

    // Run Full Simulation Suite
    auto result = sim.RunFullSimulation();
    TEST_ASSERT(result.success, "Simulation run failed");
    TEST_ASSERT(result.totalAmbulances == 8, "Total ambulances simulated should be 8");
    TEST_ASSERT(result.totalConflictsDetected >= 3, "At least 3 junction conflicts should be detected");
    TEST_ASSERT(result.totalConflictsResolved >= 3, "At least 3 conflicts should be resolved");
    TEST_ASSERT(result.totalCommandsGenerated >= 8, "At least 8 commands should be generated");
    TEST_ASSERT(result.totalAwarenessNotifications >= 12, "Awareness notifications should be generated");

    // Verify Scenario 1: AMB_CARDIAC (score ~90) won at INT_A over Stroke, Heart Attack, Pregnancy
    bool cardiacWon = false;
    for (const auto& dec : result.decisions) {
        if (dec.intersectionId == "INT_A" && dec.winnerAmbulanceId == "AMB_CARDIAC") {
            cardiacWon = true;
            break;
        }
    }
    TEST_ASSERT(cardiacWon, "AMB_CARDIAC must win contention at INT_A");

    // Verify Scenario 2: AMB_ORGAN won at INT_B due to manual override level 2
    bool organWon = false;
    for (const auto& dec : result.decisions) {
        if (dec.intersectionId == "INT_B" && dec.winnerAmbulanceId == "AMB_ORGAN") {
            organWon = true;
            break;
        }
    }
    TEST_ASSERT(organWon, "AMB_ORGAN must win contention at INT_B via manual override");

    // Verify Scenario 3: AMB_PEDIATRIC won at INT_C due to waiting time bonus
    bool pediatricWon = false;
    for (const auto& dec : result.decisions) {
        if (dec.intersectionId == "INT_C" && dec.winnerAmbulanceId == "AMB_PEDIATRIC") {
            pediatricWon = true;
            break;
        }
    }
    TEST_ASSERT(pediatricWon, "AMB_PEDIATRIC must win contention at INT_C via waiting time urgency");

    // Verify ALTERNATE_ROUTE command was generated for AMB_ACCIDENT
    bool altRouteIssued = false;
    for (const auto& cmd : result.generatedCommands) {
        if (cmd.ambulanceId == "AMB_ACCIDENT" && cmd.action == scheduler::ResolutionAction::ALTERNATE_ROUTE) {
            altRouteIssued = true;
            break;
        }
    }
    TEST_ASSERT(altRouteIssued, "ALTERNATE_ROUTE command must be issued for AMB_ACCIDENT");

    std::cout << "  PASS: MultiAmbulanceSimulation executed 8 clinical emergencies cleanly and deterministically.\n";
    return true;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 8 Multi-Ambulance Conflict Resolution\n";
    std::cout << " & Intelligent Priority Scheduling Test Suite\n";
    std::cout << "==========================================================\n";

    bool allPassed = true;

    allPassed &= TestAmbulanceRegistryPhase8();
    allPassed &= TestMissionManagerLifecycle();
    allPassed &= TestConflictDetectionEngine();
    allPassed &= TestIntelligentPriorityScheduler();
    allPassed &= TestConflictResolutionManager();
    allPassed &= TestManualOverrideManager();
    allPassed &= TestAmbulanceAwarenessService();
    allPassed &= TestMultiAmbulanceSimulationEightEmergencies();

    std::cout << "==========================================================\n";
    if (allPassed) {
        std::cout << " [ALL PASS] Phase 8 Intelligent Priority Scheduling 100% Verified!\n";
        std::cout << "==========================================================\n";
        return 0;
    } else {
        std::cerr << " [FAILED] Some Phase 8 tests failed!\n";
        std::cout << "==========================================================\n";
        return 1;
    }
}
