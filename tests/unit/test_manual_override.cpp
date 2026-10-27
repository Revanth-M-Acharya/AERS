/**
 * @file test_manual_override.cpp
 * @brief Unit tests for Control Center Manual Override and Emergency Operator Console in SMART AEGIS.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <memory>

#include "control/ManualOverride.h"
#include "control/EmergencyOperatorConsole.h"
#include "ambulance/AmbulanceProfile.h"
#include "ambulance/AmbulanceRegistry.h"
#include "scheduler/PriorityScheduler.h"
#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::control;
using namespace aegis::ambulance;
using namespace aegis::scheduler;
using namespace aegis::medical;

// Helper to construct test ambulance profiles
static AmbulanceProfile CreateAmbulance(const std::string& id,
                                       const std::string& intersection,
                                       uint32_t etaSec,
                                       EmergencyType emergencyType,
                                       PriorityLevel priority) {
    AmbulanceProfile amb(id, AmbulanceStatus::TRANSPORTING, "Metropolitan Hospital", (etaSec + 59) / 60);
    amb.SetCurrentIntersection(intersection);
    amb.SetETASeconds(etaSec);
    amb.SetPriority(priority);

    PatientVitals vitals;
    if (emergencyType == EmergencyType::CARDIAC_ARREST) {
        vitals.heartRate = 0;
        vitals.bloodPressure = BloodPressure{0, 0};
        vitals.spO2 = 65;
        vitals.respirationRate = 0;
        vitals.isConscious = false;
    } else {
        vitals.heartRate = 85;
        vitals.spO2 = 97;
        vitals.respirationRate = 18;
        vitals.isConscious = true;
    }

    PatientProfile patient("PT-" + id, "Test Patient", 45, Gender::MALE, emergencyType, vitals);
    amb.SetPatient(patient);
    return amb;
}

// -----------------------------------------------------------------------------
// Test 1: Priority Escalation and De-escalation with Audit Logging
// -----------------------------------------------------------------------------
static void TestPriorityIncreaseAndDecrease() {
    std::cout << "[TEST 1] Testing Priority Increase and Decrease with Audit Trail..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>();
    EmergencyOperatorConsole console(registry, scheduler, "DISPATCHER_42");

    AmbulanceProfile amb = CreateAmbulance("AMB_E01", "INT_A", 40, EmergencyType::TRAUMA, PriorityLevel::LOW);
    registry->RegisterAmbulance(amb);

    // Initial state
    auto searchInit = registry->SearchAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(searchInit.has_value());
    AEGIS_TEST_ASSERT(searchInit->GetPriority() == PriorityLevel::LOW);
    AEGIS_TEST_ASSERT(searchInit->GetManualOverrideLevel() == 0);

    // 1. Escalate LOW -> MEDIUM
    bool ok1 = console.IncreasePriority("AMB_E01", "Patient reported worsening shock");
    AEGIS_TEST_ASSERT(ok1);
    auto after1 = registry->SearchAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(after1.has_value());
    AEGIS_TEST_ASSERT(after1->GetPriority() == PriorityLevel::MEDIUM);
    AEGIS_TEST_ASSERT(after1->GetManualOverrideLevel() == 1);

    // 2. Escalate MEDIUM -> HIGH
    bool ok2 = console.IncreasePriority("AMB_E01", "Unstable vitals noted");
    AEGIS_TEST_ASSERT(ok2);
    auto after2 = registry->SearchAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(after2.has_value());
    AEGIS_TEST_ASSERT(after2->GetPriority() == PriorityLevel::HIGH);
    AEGIS_TEST_ASSERT(after2->GetManualOverrideLevel() == 2);

    // 3. Escalate HIGH -> CRITICAL
    bool ok3 = console.IncreasePriority("AMB_E01", "Full clinical crisis");
    AEGIS_TEST_ASSERT(ok3);
    auto after3 = registry->SearchAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(after3.has_value());
    AEGIS_TEST_ASSERT(after3->GetPriority() == PriorityLevel::CRITICAL);
    AEGIS_TEST_ASSERT(after3->GetManualOverrideLevel() == 3);

    // 4. De-escalate CRITICAL -> HIGH
    bool ok4 = console.DecreasePriority("AMB_E01", "Paramedic initiated IV stabilization");
    AEGIS_TEST_ASSERT(ok4);
    auto after4 = registry->SearchAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(after4.has_value());
    AEGIS_TEST_ASSERT(after4->GetPriority() == PriorityLevel::HIGH);
    AEGIS_TEST_ASSERT(after4->GetManualOverrideLevel() == 2);

    // Verify Audit Logs
    auto ambLogs = console.GetLogsForAmbulance("AMB_E01");
    AEGIS_TEST_ASSERT(ambLogs.size() == 4);
    AEGIS_TEST_ASSERT(ambLogs[0].type == OverrideType::INCREASE_PRIORITY);
    AEGIS_TEST_ASSERT(ambLogs[0].previousPriority == PriorityLevel::LOW);
    AEGIS_TEST_ASSERT(ambLogs[0].newPriority == PriorityLevel::MEDIUM);
    AEGIS_TEST_ASSERT(ambLogs[0].operatorId == "DISPATCHER_42");

    AEGIS_TEST_ASSERT(ambLogs[3].type == OverrideType::DECREASE_PRIORITY);
    AEGIS_TEST_ASSERT(ambLogs[3].previousPriority == PriorityLevel::CRITICAL);
    AEGIS_TEST_ASSERT(ambLogs[3].newPriority == PriorityLevel::HIGH);

    std::cout << "  -> Priority escalation and de-escalation verified successfully." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Manual Override Priority Dominates Automatic Scheduling
// -----------------------------------------------------------------------------
static void TestManualOverrideDominatesAutomaticScheduling() {
    std::cout << "[TEST 2] Testing Manual Override Dominates Automatic Scheduling..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>(10, 15);
    EmergencyOperatorConsole console(registry, scheduler, "CHIEF_OPERATOR");

    // AmbAuto: Cardiac arrest, automatic CRITICAL priority (score 100), manual override level 0
    AmbulanceProfile ambAuto = CreateAmbulance("AMB_AUTO", "INT_B", 30, EmergencyType::CARDIAC_ARREST, PriorityLevel::CRITICAL);

    // AmbManual: Mild Trauma, automatic LOW priority (score 30), manual override level 0
    AmbulanceProfile ambManual = CreateAmbulance("AMB_MANUAL", "INT_B", 30, EmergencyType::TRAUMA, PriorityLevel::LOW);

    registry->RegisterAmbulance(ambAuto);
    registry->RegisterAmbulance(ambManual);

    // Initially, automatic scheduling arbitrates in favor of AMB_AUTO
    auto resolutionsInit = scheduler->ScheduleAmbulances(registry->GetAllAmbulances());
    AEGIS_TEST_ASSERT(!resolutionsInit.empty());
    AEGIS_TEST_ASSERT(resolutionsInit[0].winnerAmbulanceId == "AMB_AUTO");
    AEGIS_TEST_ASSERT(scheduler->IsAmbulanceGrantedGreen("AMB_AUTO", "INT_B"));
    AEGIS_TEST_ASSERT(!scheduler->IsAmbulanceGrantedGreen("AMB_MANUAL", "INT_B"));

    // Operator intervenes: manual priority escalation on AMB_MANUAL
    bool escalated = console.IncreasePriority("AMB_MANUAL", "VIP Organ Transplant Protocol Directive");
    AEGIS_TEST_ASSERT(escalated);

    // Verify AMB_MANUAL has manualOverrideLevel > 0
    auto ambManualUpdated = registry->SearchAmbulance("AMB_MANUAL");
    AEGIS_TEST_ASSERT(ambManualUpdated.has_value());
    AEGIS_TEST_ASSERT(ambManualUpdated->GetManualOverrideLevel() >= 1);

    // Verify that the scheduler now selects AMB_MANUAL as the winner due to manual override precedence
    auto activeRes = scheduler->GetResolutionForIntersection("INT_B");
    AEGIS_TEST_ASSERT(activeRes.has_value());
    AEGIS_TEST_ASSERT(activeRes->winnerAmbulanceId == "AMB_MANUAL");
    AEGIS_TEST_ASSERT(scheduler->IsAmbulanceGrantedGreen("AMB_MANUAL", "INT_B"));
    AEGIS_TEST_ASSERT(!scheduler->IsAmbulanceGrantedGreen("AMB_AUTO", "INT_B"));

    std::cout << "  -> Manual override strictly dominates automatic clinical ranking." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: Manual Corridor Lifecycle (Create, Pause, Resume, Cancel)
// -----------------------------------------------------------------------------
static void TestManualCorridorLifecycle() {
    std::cout << "[TEST 3] Testing Manual Corridor Lifecycle..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>();
    EmergencyOperatorConsole console(registry, scheduler, "OPERATOR_07");

    std::vector<std::string> route = {"INT_101", "INT_102", "INT_103"};
    const std::string corridorId = "CORR_MANUAL_01";
    const std::string ambId = "AMB_VIP";

    // 1. Create Corridor Manually
    bool created = console.CreateCorridorManually(corridorId, ambId, route, "Mass casualty incident clearance");
    AEGIS_TEST_ASSERT(created);

    auto corrOpt = console.GetManualCorridor(corridorId);
    AEGIS_TEST_ASSERT(corrOpt.has_value());
    AEGIS_TEST_ASSERT(corrOpt->status == ManualCorridorStatus::ACTIVE);
    AEGIS_TEST_ASSERT(corrOpt->ambulanceId == ambId);
    AEGIS_TEST_ASSERT(corrOpt->route.size() == 3);
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedGreen("INT_101"));

    // 2. Pause Corridor
    bool paused = console.PauseCorridor(corridorId, "Rail crossing blockade");
    AEGIS_TEST_ASSERT(paused);
    corrOpt = console.GetManualCorridor(corridorId);
    AEGIS_TEST_ASSERT(corrOpt.has_value());
    AEGIS_TEST_ASSERT(corrOpt->status == ManualCorridorStatus::PAUSED);

    // 3. Resume Corridor
    bool resumed = console.ResumeCorridor(corridorId, "Rail gate opened");
    AEGIS_TEST_ASSERT(resumed);
    corrOpt = console.GetManualCorridor(corridorId);
    AEGIS_TEST_ASSERT(corrOpt.has_value());
    AEGIS_TEST_ASSERT(corrOpt->status == ManualCorridorStatus::ACTIVE);

    // 4. Cancel Corridor
    bool cancelled = console.CancelCorridor(corridorId, "Ambulance arrived at destination");
    AEGIS_TEST_ASSERT(cancelled);
    corrOpt = console.GetManualCorridor(corridorId);
    AEGIS_TEST_ASSERT(corrOpt.has_value());
    AEGIS_TEST_ASSERT(corrOpt->status == ManualCorridorStatus::CANCELLED);
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedGreen("INT_101"));

    // Verify logs
    auto logs = console.GetAuditLogs();
    AEGIS_TEST_ASSERT(logs.size() >= 4);

    std::cout << "  -> Manual corridor lifecycle operations verified successfully." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: Force Green, Force Red, and Clear Overrides
// -----------------------------------------------------------------------------
static void TestForceGreenAndForceRed() {
    std::cout << "[TEST 4] Testing Force Green, Force Red, and Clear Overrides..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>();
    EmergencyOperatorConsole console(registry, scheduler, "OPERATOR_09");

    const std::string intersectionId = "INT_NODE_C";

    // 1. Force Green
    bool greenSet = console.ForceGreen(intersectionId, "AMB_EXPRESS", 60, "Immediate convoy transit");
    AEGIS_TEST_ASSERT(greenSet);
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedGreen(intersectionId));
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedRed(intersectionId));
    AEGIS_TEST_ASSERT(scheduler->IsAmbulanceGrantedGreen("AMB_EXPRESS", intersectionId));

    // 2. Force Red (overrides Force Green immediately)
    bool redSet = console.ForceRed(intersectionId, 45, "Hazardous material spill across junction");
    AEGIS_TEST_ASSERT(redSet);
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedRed(intersectionId));
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedGreen(intersectionId));
    AEGIS_TEST_ASSERT(!scheduler->IsAmbulanceGrantedGreen("AMB_EXPRESS", intersectionId));

    // 3. Clear Intersection Override
    bool cleared = console.ClearIntersectionOverride(intersectionId);
    AEGIS_TEST_ASSERT(cleared);
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedGreen(intersectionId));
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedRed(intersectionId));

    // Verify logs for this intersection
    auto intLogs = console.GetLogsForIntersection(intersectionId);
    AEGIS_TEST_ASSERT(intLogs.size() == 3);
    AEGIS_TEST_ASSERT(intLogs[0].type == OverrideType::FORCE_GREEN);
    AEGIS_TEST_ASSERT(intLogs[1].type == OverrideType::FORCE_RED);
    AEGIS_TEST_ASSERT(intLogs[2].status == OverrideStatus::CLEARED);

    std::cout << "  -> Force Green, Force Red, and Clear Overrides verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Network-wide Emergency Stop
// -----------------------------------------------------------------------------
static void TestEmergencyStop() {
    std::cout << "[TEST 5] Testing Network-wide Emergency Stop..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>();
    EmergencyOperatorConsole console(registry, scheduler, "SUPERVISOR");

    // Create an active corridor and a forced green
    console.CreateCorridorManually("CORR_TEST", "AMB_99", {"INT_X", "INT_Y"}, "Routine preemption");
    console.ForceGreen("INT_Z", "AMB_99", 30, "Holding green");

    AEGIS_TEST_ASSERT(console.IsIntersectionForcedGreen("INT_X"));
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedGreen("INT_Z"));
    AEGIS_TEST_ASSERT(!console.IsEmergencyStopActive());

    // Trigger Emergency Stop
    bool eStop = console.EmergencyStop("Major seismic tremor detected - all corridors abort");
    AEGIS_TEST_ASSERT(eStop);
    AEGIS_TEST_ASSERT(console.IsEmergencyStopActive());

    // Corridors must be cancelled
    auto corr = console.GetManualCorridor("CORR_TEST");
    AEGIS_TEST_ASSERT(corr.has_value());
    AEGIS_TEST_ASSERT(corr->status == ManualCorridorStatus::CANCELLED);

    // Forced green must be cleared
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedGreen("INT_X"));
    AEGIS_TEST_ASSERT(!console.IsIntersectionForcedGreen("INT_Z"));

    // Every intersection is considered forced red under emergency stop
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedRed("INT_X"));
    AEGIS_TEST_ASSERT(console.IsIntersectionForcedRed("INT_ANY"));

    // Scheduler green grants must be revoked
    AEGIS_TEST_ASSERT(!scheduler->IsAmbulanceGrantedGreen("AMB_99", "INT_X"));

    // Release Emergency Stop
    console.ClearEmergencyStop();
    AEGIS_TEST_ASSERT(!console.IsEmergencyStopActive());

    std::cout << "  -> Network-wide Emergency Stop fail-safe verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: Immediate Recalculation Notification Callback
// -----------------------------------------------------------------------------
static void TestImmediateRecalculationCallback() {
    std::cout << "[TEST 6] Testing Immediate Recalculation Notification Callback..." << std::endl;

    auto registry = std::make_shared<AmbulanceRegistry>();
    auto scheduler = std::make_shared<PriorityScheduler>();
    EmergencyOperatorConsole console(registry, scheduler, "DISPATCH_UNIT");

    AmbulanceProfile amb = CreateAmbulance("AMB_NOTIF", "INT_A", 20, EmergencyType::STROKE, PriorityLevel::MEDIUM);
    registry->RegisterAmbulance(amb);

    size_t callbackCount = 0;
    OverrideType lastType = OverrideType::INCREASE_PRIORITY;

    console.SetRecalculateCallback([&](const ManualOverrideRecord& record) {
        ++callbackCount;
        lastType = record.type;
    });

    console.IncreasePriority("AMB_NOTIF", "Callback test 1");
    AEGIS_TEST_ASSERT(callbackCount == 1);
    AEGIS_TEST_ASSERT(lastType == OverrideType::INCREASE_PRIORITY);

    console.DecreasePriority("AMB_NOTIF", "Callback test 2");
    AEGIS_TEST_ASSERT(callbackCount == 2);
    AEGIS_TEST_ASSERT(lastType == OverrideType::DECREASE_PRIORITY);

    console.ForceGreen("INT_A", "AMB_NOTIF", 30, "Callback test 3");
    AEGIS_TEST_ASSERT(callbackCount == 3);
    AEGIS_TEST_ASSERT(lastType == OverrideType::FORCE_GREEN);

    console.ForceRed("INT_A", 30, "Callback test 4");
    AEGIS_TEST_ASSERT(callbackCount == 4);
    AEGIS_TEST_ASSERT(lastType == OverrideType::FORCE_RED);

    console.EmergencyStop("Callback test 5");
    AEGIS_TEST_ASSERT(callbackCount == 5);
    AEGIS_TEST_ASSERT(lastType == OverrideType::EMERGENCY_STOP);

    std::cout << "  -> Immediate recalculation callback dispatches on every override action." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: Audit Record Formatting and JSON Serialization
// -----------------------------------------------------------------------------
static void TestAuditRecordSerialization() {
    std::cout << "[TEST 7] Testing Audit Record Serialization..." << std::endl;

    ManualOverrideRecord record;
    record.overrideId = "OVR-999";
    record.operatorId = "OP_ALPHA";
    record.type = OverrideType::FORCE_GREEN;
    record.targetIntersectionId = "INT_NODE_B";
    record.targetAmbulanceId = "AMB_007";
    record.durationSeconds = 45;
    record.reason = "Manual override test";
    record.timestampEpochMs = 1700000000000;
    record.status = OverrideStatus::ACTIVE;

    std::string str = record.ToString();
    AEGIS_TEST_ASSERT(str.find("OVR-999") != std::string::npos);
    AEGIS_TEST_ASSERT(str.find("OP_ALPHA") != std::string::npos);
    AEGIS_TEST_ASSERT(str.find("FORCE_GREEN") != std::string::npos);

    std::string json = record.ToJson();
    AEGIS_TEST_ASSERT(json.find("\"override_id\":\"OVR-999\"") != std::string::npos);
    AEGIS_TEST_ASSERT(json.find("\"operator_id\":\"OP_ALPHA\"") != std::string::npos);
    AEGIS_TEST_ASSERT(json.find("\"type\":\"FORCE_GREEN\"") != std::string::npos);
    AEGIS_TEST_ASSERT(json.find("\"target_intersection\":\"INT_NODE_B\"") != std::string::npos);

    ManualOverride overrideObj(record);
    AEGIS_TEST_ASSERT(overrideObj.GetOverrideId() == "OVR-999");
    AEGIS_TEST_ASSERT(overrideObj.GetType() == OverrideType::FORCE_GREEN);
    AEGIS_TEST_ASSERT(overrideObj.IsActive());

    overrideObj.Clear();
    AEGIS_TEST_ASSERT(!overrideObj.IsActive());

    std::cout << "  -> Audit record serialization validated." << std::endl;
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - Manual Intervention & Operator Console Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestPriorityIncreaseAndDecrease();
    TestManualOverrideDominatesAutomaticScheduling();
    TestManualCorridorLifecycle();
    TestForceGreenAndForceRed();
    TestEmergencyStop();
    TestImmediateRecalculationCallback();
    TestAuditRecordSerialization();

    std::cout << "==========================================================" << std::endl;
    std::cout << " [PASS] All 7 Manual Intervention Test Suites Passed!    " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
