/**
 * @file test_conflict_detection.cpp
 * @brief Comprehensive unit tests for Multi-Ambulance Conflict Detection & Priority Scheduling.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>

#include "ambulance/AmbulanceProfile.h"
#include "scheduler/ConflictDetector.h"
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

using namespace aegis::ambulance;
using namespace aegis::scheduler;
using namespace aegis::medical;

// Helper to create a critical cardiac arrest profile
static AmbulanceProfile CreateCriticalAmbulance(const std::string& id, const std::string& intersection, uint32_t etaSec) {
    AmbulanceProfile amb(id, AmbulanceStatus::TRANSPORTING, "City Hospital", (etaSec + 59) / 60);
    amb.SetCurrentIntersection(intersection);
    amb.SetETASeconds(etaSec);

    PatientVitals vitals;
    vitals.heartRate = 0;
    vitals.bloodPressure = BloodPressure{0, 0};
    vitals.spO2 = 65;
    vitals.respirationRate = 0;
    vitals.isConscious = false;

    PatientProfile patient("PT-CRIT-" + id, "Critical Patient", 62, Gender::MALE, EmergencyType::CARDIAC_ARREST, vitals);
    amb.SetPatient(patient);
    return amb;
}

// Helper to create a high priority STEMI heart attack profile
static AmbulanceProfile CreateHighPriorityAmbulance(const std::string& id, const std::string& intersection, uint32_t etaSec) {
    AmbulanceProfile amb(id, AmbulanceStatus::TRANSPORTING, "City Hospital", (etaSec + 59) / 60);
    amb.SetCurrentIntersection(intersection);
    amb.SetETASeconds(etaSec);

    PatientVitals vitals;
    vitals.spO2 = 89;
    vitals.heartRate = 125;
    vitals.estimatedTimeSinceIncidentMinutes = 45;

    PatientProfile patient("PT-HIGH-" + id, "High Patient", 66, Gender::MALE, EmergencyType::HEART_ATTACK, vitals);
    amb.SetPatient(patient);
    return amb;
}

// Helper to create a medium priority burn injury profile
static AmbulanceProfile CreateMediumPriorityAmbulance(const std::string& id, const std::string& intersection, uint32_t etaSec) {
    AmbulanceProfile amb(id, AmbulanceStatus::TRANSPORTING, "City Hospital", (etaSec + 59) / 60);
    amb.SetCurrentIntersection(intersection);
    amb.SetETASeconds(etaSec);

    PatientVitals vitals;
    vitals.spO2 = 88;
    vitals.respirationRate = 32;
    vitals.heartRate = 132;

    PatientProfile patient("PT-MED-" + id, "Medium Patient", 30, Gender::FEMALE, EmergencyType::BURN_INJURY, vitals);
    amb.SetPatient(patient);
    return amb;
}

// Helper to create a low priority routine pregnancy profile
static AmbulanceProfile CreateLowPriorityAmbulance(const std::string& id, const std::string& intersection, uint32_t etaSec) {
    AmbulanceProfile amb(id, AmbulanceStatus::TRANSPORTING, "City Hospital", (etaSec + 59) / 60);
    amb.SetCurrentIntersection(intersection);
    amb.SetETASeconds(etaSec);

    PatientVitals vitals;
    PatientProfile patient("PT-LOW-" + id, "Low Patient", 26, Gender::FEMALE, EmergencyType::PREGNANCY, vitals);
    amb.SetPatient(patient);
    return amb;
}

// =============================================================================
// Test 1: Two Ambulances - Different Priorities (CRITICAL vs HIGH)
// =============================================================================
static void TestTwoAmbulancesDifferentPriority() {
    std::cout << "[TEST] 1. Two Ambulances Contention: Different Priorities (CRITICAL vs HIGH)... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateCriticalAmbulance("AMB-1", "INT_A", 20); // CRITICAL, ETA 20s
    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_A", 22); // HIGH, ETA 22s

    std::vector<AmbulanceProfile> fleet = {amb1, amb2};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    AEGIS_TEST_ASSERT(resolutions.size() == 1);
    const auto& res = resolutions[0];
    AEGIS_TEST_ASSERT(res.intersectionId == "INT_A");
    AEGIS_TEST_ASSERT(res.winnerAmbulanceId == "AMB-1"); // CRITICAL wins
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds.size() == 1);
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[0] == "AMB-2");
    AEGIS_TEST_ASSERT(res.delayDurationSeconds == 15);
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-2") == 15);

    // Verify Green Grants: AMB-1 granted GREEN, AMB-2 denied
    AEGIS_TEST_ASSERT(scheduler.IsAmbulanceGrantedGreen("AMB-1", "INT_A"));
    AEGIS_TEST_ASSERT(!scheduler.IsAmbulanceGrantedGreen("AMB-2", "INT_A"));

    std::cout << "PASS (Winner: " << res.winnerAmbulanceId << ", Delay: 15s)\n";
}

// =============================================================================
// Test 2: Two Ambulances - Equal Priorities, Different ETA
// =============================================================================
static void TestTwoAmbulancesEqualPriorityDifferentETA() {
    std::cout << "[TEST] 2. Two Ambulances Contention: Equal Priorities, Different ETA... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateHighPriorityAmbulance("AMB-1", "INT_B", 30); // HIGH, ETA 30s
    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_B", 36); // HIGH, ETA 36s (delta 6s <= 10s)

    std::vector<AmbulanceProfile> fleet = {amb1, amb2};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    AEGIS_TEST_ASSERT(resolutions.size() == 1);
    const auto& res = resolutions[0];
    AEGIS_TEST_ASSERT(res.winnerAmbulanceId == "AMB-1"); // Earlier arrival wins
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[0] == "AMB-2");

    AEGIS_TEST_ASSERT(scheduler.IsAmbulanceGrantedGreen("AMB-1", "INT_B"));
    AEGIS_TEST_ASSERT(!scheduler.IsAmbulanceGrantedGreen("AMB-2", "INT_B"));

    std::cout << "PASS (Winner: AMB-1 by ETA 30s vs 36s)\n";
}

// =============================================================================
// Test 3: Two Ambulances - Equal Priorities, Identical ETA, Different Distance
// =============================================================================
static void TestTwoAmbulancesEqualPriorityIdenticalETA() {
    std::cout << "[TEST] 3. Two Ambulances Contention: Equal Priority & Identical ETA (Distance Arbitration)... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateHighPriorityAmbulance("AMB-1", "INT_C", 40); // HIGH, ETA 40s
    amb1.SetDistanceRemainingMeters(320.0); // 320m to junction

    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_C", 40); // HIGH, ETA 40s
    amb2.SetDistanceRemainingMeters(450.0); // 450m to junction

    std::vector<AmbulanceProfile> fleet = {amb1, amb2};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    AEGIS_TEST_ASSERT(resolutions.size() == 1);
    const auto& res = resolutions[0];
    AEGIS_TEST_ASSERT(res.winnerAmbulanceId == "AMB-1"); // Closer distance (320m < 450m) wins!
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[0] == "AMB-2");

    std::cout << "PASS (Winner: AMB-1 by distance 320m vs 450m)\n";
}

// =============================================================================
// Test 4: Two Ambulances - All Factors Tied, Deterministic ID Arbitration
// =============================================================================
static void TestTwoAmbulancesAllTiedDeterministic() {
    std::cout << "[TEST] 4. Two Ambulances Contention: Complete Clinical Tie (Deterministic ID)... ";

    PriorityScheduler scheduler(10, 15);

    auto ambA = CreateHighPriorityAmbulance("AMB-ALPHA", "INT_D", 25);
    ambA.SetDistanceRemainingMeters(300.0);

    auto ambB = CreateHighPriorityAmbulance("AMB-BETA", "INT_D", 25);
    ambB.SetDistanceRemainingMeters(300.0);

    // Test run 1: insertion [A, B]
    auto res1 = scheduler.ScheduleAmbulances({ambA, ambB});
    AEGIS_TEST_ASSERT(res1.size() == 1);
    AEGIS_TEST_ASSERT(res1[0].winnerAmbulanceId == "AMB-ALPHA");

    // Test run 2: insertion [B, A]
    auto res2 = scheduler.ScheduleAmbulances({ambB, ambA});
    AEGIS_TEST_ASSERT(res2.size() == 1);
    AEGIS_TEST_ASSERT(res2[0].winnerAmbulanceId == "AMB-ALPHA"); // Must strictly remain AMB-ALPHA

    std::cout << "PASS (Deterministic winner: AMB-ALPHA across both insertion orders)\n";
}

// =============================================================================
// Test 5: Three Ambulances Contention
// =============================================================================
static void TestThreeAmbulancesConflict() {
    std::cout << "[TEST] 5. Three Ambulances Contention (CRITICAL, HIGH, MEDIUM)... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateCriticalAmbulance("AMB-1", "INT_E", 15);  // CRITICAL, ETA 15s
    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_E", 18); // HIGH, ETA 18s
    auto amb3 = CreateMediumPriorityAmbulance("AMB-3", "INT_E", 22); // MEDIUM, ETA 22s

    std::vector<AmbulanceProfile> fleet = {amb1, amb2, amb3};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    AEGIS_TEST_ASSERT(resolutions.size() == 1);
    const auto& res = resolutions[0];
    AEGIS_TEST_ASSERT(res.intersectionId == "INT_E");
    AEGIS_TEST_ASSERT(res.winnerAmbulanceId == "AMB-1"); // Highest priority wins
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds.size() == 2);
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[0] == "AMB-2");
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[1] == "AMB-3");

    // Verify staggered delay assignments: 1st delayed = 15s, 2nd delayed = 30s
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-2") == 15);
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-3") == 30);

    // Verify only the winner has green
    AEGIS_TEST_ASSERT(scheduler.IsAmbulanceGrantedGreen("AMB-1", "INT_E"));
    AEGIS_TEST_ASSERT(!scheduler.IsAmbulanceGrantedGreen("AMB-2", "INT_E"));
    AEGIS_TEST_ASSERT(!scheduler.IsAmbulanceGrantedGreen("AMB-3", "INT_E"));

    std::cout << "PASS (Winner: AMB-1, Delays: AMB-2 +15s, AMB-3 +30s)\n";
}

// =============================================================================
// Test 6: Four Ambulances Contention with Manual Override
// =============================================================================
static void TestFourAmbulancesConflictWithManualOverride() {
    std::cout << "[TEST] 6. Four Ambulances Contention with Manual Dispatcher Override... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateCriticalAmbulance("AMB-1", "INT_F", 50); // CRITICAL, no override
    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_F", 53); // HIGH, but Supervisor Override Level 2!
    amb2.SetManualOverrideLevel(2);
    auto amb3 = CreateMediumPriorityAmbulance("AMB-3", "INT_F", 55); // MEDIUM
    auto amb4 = CreateLowPriorityAmbulance("AMB-4", "INT_F", 58); // LOW

    std::vector<AmbulanceProfile> fleet = {amb1, amb2, amb3, amb4};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    AEGIS_TEST_ASSERT(resolutions.size() == 1);
    const auto& res = resolutions[0];
    AEGIS_TEST_ASSERT(res.winnerAmbulanceId == "AMB-2"); // Manual Override level 2 trumps regular triage!
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds.size() == 3);

    // Verify delayed ordering: CRITICAL (AMB-1) is next in line, then MEDIUM (AMB-3), then LOW (AMB-4)
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[0] == "AMB-1");
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[1] == "AMB-3");
    AEGIS_TEST_ASSERT(res.delayedAmbulanceIds[2] == "AMB-4");

    // Staggered delays: 15s, 30s, 45s
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-1") == 15);
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-3") == 30);
    AEGIS_TEST_ASSERT(res.assignedDelaysSeconds.at("AMB-4") == 45);

    std::cout << "PASS (Override Winner: AMB-2, Staggered 15s/30s/45s)\n";
}

// =============================================================================
// Test 7: Strict Mutual Exclusion Guarantee
// =============================================================================
static void TestMutualExclusionGuarantee() {
    std::cout << "[TEST] 7. Strict Mutual Exclusion: Never Allow Two Green Grants Simultaneously... ";

    PriorityScheduler scheduler(10, 15);

    auto amb1 = CreateCriticalAmbulance("AMB-X", "INT_G", 25);
    auto amb2 = CreateHighPriorityAmbulance("AMB-Y", "INT_G", 26);
    auto amb3 = CreateMediumPriorityAmbulance("AMB-Z", "INT_G", 27);

    scheduler.ScheduleAmbulances({amb1, amb2, amb3});

    int greenCount = 0;
    if (scheduler.IsAmbulanceGrantedGreen("AMB-X", "INT_G")) greenCount++;
    if (scheduler.IsAmbulanceGrantedGreen("AMB-Y", "INT_G")) greenCount++;
    if (scheduler.IsAmbulanceGrantedGreen("AMB-Z", "INT_G")) greenCount++;

    // Exactly one ambulance must be granted GREEN
    AEGIS_TEST_ASSERT(greenCount == 1);
    AEGIS_TEST_ASSERT(scheduler.GetGrantedAmbulanceForIntersection("INT_G") == "AMB-X");

    std::cout << "PASS (Active green grants for INT_G: exactly 1)\n";
}

// =============================================================================
// Test 8: Non-Conflicting Independent Routes & Corridors
// =============================================================================
static void TestNonConflictingAmbulances() {
    std::cout << "[TEST] 8. Non-Conflicting Concurrent Junction Corridors... ";

    PriorityScheduler scheduler(10, 15);

    // Approaching separate junctions at the same time
    auto amb1 = CreateCriticalAmbulance("AMB-1", "INT_A", 20);
    auto amb2 = CreateHighPriorityAmbulance("AMB-2", "INT_B", 20);

    // Approaching same junction INT_A, but widely separated in time (|80 - 20| = 60s > 10s)
    auto amb3 = CreateMediumPriorityAmbulance("AMB-3", "INT_A", 80);

    std::vector<AmbulanceProfile> fleet = {amb1, amb2, amb3};
    auto resolutions = scheduler.ScheduleAmbulances(fleet);

    // No conflicts detected
    AEGIS_TEST_ASSERT(resolutions.empty());

    // Both independent intersections granted green to their approaching units
    AEGIS_TEST_ASSERT(scheduler.IsAmbulanceGrantedGreen("AMB-1", "INT_A"));
    AEGIS_TEST_ASSERT(scheduler.IsAmbulanceGrantedGreen("AMB-2", "INT_B"));

    std::cout << "PASS\n";
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Multi-Ambulance Conflict Detection & Priority Scheduler Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestTwoAmbulancesDifferentPriority();
    TestTwoAmbulancesEqualPriorityDifferentETA();
    TestTwoAmbulancesEqualPriorityIdenticalETA();
    TestTwoAmbulancesAllTiedDeterministic();
    TestThreeAmbulancesConflict();
    TestFourAmbulancesConflictWithManualOverride();
    TestMutualExclusionGuarantee();
    TestNonConflictingAmbulances();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All Multi-Ambulance Conflict Detection Tests Passed Successfully!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
