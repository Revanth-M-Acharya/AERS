/**
 * @file test_ambulance_registry.cpp
 * @brief Unit tests for AmbulanceProfile and AmbulanceRegistry in SMART AEGIS.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "ambulance/AmbulanceProfile.h"
#include "ambulance/AmbulanceRegistry.h"
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
using namespace aegis::medical;

// =============================================================================
// Test 1: AmbulanceProfile Initialization and Accessors
// =============================================================================
static void TestAmbulanceProfileInitialization() {
    std::cout << "[TEST] 1. AmbulanceProfile Initialization & Attributes... ";

    AmbulanceProfile amb("AMB-101", AmbulanceStatus::AVAILABLE, "City Central Hospital", 15);
    AEGIS_TEST_ASSERT(amb.IsValid());
    AEGIS_TEST_ASSERT(amb.GetAmbulanceId() == "AMB-101");
    AEGIS_TEST_ASSERT(amb.GetStatus() == AmbulanceStatus::AVAILABLE);
    AEGIS_TEST_ASSERT(amb.GetDestinationHospital() == "City Central Hospital");
    AEGIS_TEST_ASSERT(amb.GetETA() == 15);
    AEGIS_TEST_ASSERT(!amb.HasPatient());
    AEGIS_TEST_ASSERT(amb.GetPriority() == PriorityLevel::LOW);

    // Route configuration
    amb.AddRouteIntersection("INT_A");
    amb.AddRouteIntersection("INT_B");
    amb.AddRouteIntersection("INT_C");
    AEGIS_TEST_ASSERT(amb.GetCurrentRoute().size() == 3);
    AEGIS_TEST_ASSERT(amb.GetCurrentRoute()[0] == "INT_A");
    AEGIS_TEST_ASSERT(amb.GetCurrentRoute()[2] == "INT_C");

    // Position & Coordinates
    amb.SetCurrentIntersection("INT_A");
    amb.SetCoordinates(12.9716, 77.5946);
    AEGIS_TEST_ASSERT(amb.GetCurrentIntersection() == "INT_A");
    AEGIS_TEST_ASSERT(amb.GetLatitude() == 12.9716);
    AEGIS_TEST_ASSERT(amb.GetLongitude() == 77.5946);

    // Status transitions
    amb.SetStatus(AmbulanceStatus::TRANSPORTING);
    AEGIS_TEST_ASSERT(amb.GetStatus() == AmbulanceStatus::TRANSPORTING);
    AEGIS_TEST_ASSERT(amb.GetStatusString() == "TRANSPORTING");

    std::cout << "PASS\n";
}

// =============================================================================
// Test 2: Patient Assignment & Dynamic Priority Binding
// =============================================================================
static void TestPatientAssignmentAndPriority() {
    std::cout << "[TEST] 2. Patient Assignment & Dynamic Priority Calculation... ";

    AmbulanceProfile amb("AMB-102");
    AEGIS_TEST_ASSERT(amb.GetPriority() == PriorityLevel::LOW);

    // Assign a critical Cardiac Arrest patient
    PatientVitals arrestVitals;
    arrestVitals.heartRate = 0;
    arrestVitals.bloodPressure = BloodPressure{0, 0};
    arrestVitals.spO2 = 68;
    arrestVitals.isConscious = false;

    PatientProfile cardiacPatient(
        "PT-501",
        "Johnathan Archer",
        58,
        Gender::MALE,
        EmergencyType::CARDIAC_ARREST,
        arrestVitals
    );

    amb.SetPatient(cardiacPatient);
    AEGIS_TEST_ASSERT(amb.HasPatient());
    AEGIS_TEST_ASSERT(amb.GetPatient()->GetPatientId() == "PT-501");
    // Priority should be automatically calculated as CRITICAL
    AEGIS_TEST_ASSERT(amb.GetPriority() == PriorityLevel::CRITICAL);

    // Clear patient
    amb.ClearPatient();
    AEGIS_TEST_ASSERT(!amb.HasPatient());
    AEGIS_TEST_ASSERT(amb.GetPriority() == PriorityLevel::LOW);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 3: AmbulanceRegistry Register & Remove Operations
// =============================================================================
static void TestRegistryRegistrationAndRemoval() {
    std::cout << "[TEST] 3. AmbulanceRegistry Registration & Removal... ";

    AmbulanceRegistry registry;
    AEGIS_TEST_ASSERT(registry.GetAmbulanceCount() == 0);

    AmbulanceProfile amb1("AMB-201", AmbulanceStatus::AVAILABLE, "St. Mary Hospital", 10);
    AmbulanceProfile amb2("AMB-202", AmbulanceStatus::ON_CALL, "General Hospital", 20);
    AmbulanceProfile invalidAmb(""); // Invalid ID

    AEGIS_TEST_ASSERT(registry.RegisterAmbulance(amb1));
    AEGIS_TEST_ASSERT(registry.RegisterAmbulance(amb2));
    AEGIS_TEST_ASSERT(!registry.RegisterAmbulance(invalidAmb));
    AEGIS_TEST_ASSERT(registry.GetAmbulanceCount() == 2);
    AEGIS_TEST_ASSERT(registry.HasAmbulance("AMB-201"));
    AEGIS_TEST_ASSERT(registry.HasAmbulance("AMB-202"));
    AEGIS_TEST_ASSERT(!registry.HasAmbulance("AMB-999"));

    // Removal
    AEGIS_TEST_ASSERT(registry.RemoveAmbulance("AMB-201"));
    AEGIS_TEST_ASSERT(!registry.HasAmbulance("AMB-201"));
    AEGIS_TEST_ASSERT(registry.GetAmbulanceCount() == 1);
    AEGIS_TEST_ASSERT(!registry.RemoveAmbulance("AMB-201")); // Non-existent now

    std::cout << "PASS\n";
}

// =============================================================================
// Test 4: Registry Telemetry Mutators (Position, ETA, Patient, Priority)
// =============================================================================
static void TestRegistryTelemetryMutators() {
    std::cout << "[TEST] 4. Telemetry Mutators (Position, ETA, Patient, Priority)... ";

    AmbulanceRegistry registry;
    AmbulanceProfile amb("AMB-301", AmbulanceStatus::ON_CALL, "City Hospital", 25);
    registry.RegisterAmbulance(amb);

    // 1. Update Position (Intersection & Coordinates)
    AEGIS_TEST_ASSERT(registry.UpdatePosition("AMB-301", "INT_NORTH_01"));
    auto searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched.has_value());
    AEGIS_TEST_ASSERT(searched->GetCurrentIntersection() == "INT_NORTH_01");

    AEGIS_TEST_ASSERT(registry.UpdatePosition("AMB-301", 12.9800, 77.6100, "INT_NORTH_02"));
    searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched->GetCurrentIntersection() == "INT_NORTH_02");
    AEGIS_TEST_ASSERT(searched->GetLatitude() == 12.9800);
    AEGIS_TEST_ASSERT(searched->GetLongitude() == 77.6100);

    // 2. Update ETA
    AEGIS_TEST_ASSERT(registry.UpdateETA("AMB-301", 8));
    searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched->GetETA() == 8);

    // 3. Update Patient
    PatientVitals vitals;
    vitals.bloodPressure = BloodPressure{190, 115};
    vitals.spO2 = 88;
    vitals.estimatedTimeSinceIncidentMinutes = 65;
    PatientProfile strokePatient("PT-303", "Mary Watson", 76, Gender::FEMALE, EmergencyType::STROKE, vitals);

    AEGIS_TEST_ASSERT(registry.UpdatePatient("AMB-301", strokePatient));
    searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched->HasPatient());
    AEGIS_TEST_ASSERT(searched->GetPatient()->GetEmergencyType() == EmergencyType::STROKE);
    AEGIS_TEST_ASSERT(searched->GetPriority() == PriorityLevel::HIGH);

    // 4. Update Priority explicitly
    AEGIS_TEST_ASSERT(registry.UpdatePriority("AMB-301", PriorityLevel::CRITICAL));
    searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched->GetPriority() == PriorityLevel::CRITICAL);

    // 5. Update Status and Destination
    AEGIS_TEST_ASSERT(registry.UpdateStatus("AMB-301", AmbulanceStatus::TRANSPORTING));
    AEGIS_TEST_ASSERT(registry.UpdateDestination("AMB-301", "Metro Trauma Center"));
    searched = registry.SearchAmbulance("AMB-301");
    AEGIS_TEST_ASSERT(searched->GetStatus() == AmbulanceStatus::TRANSPORTING);
    AEGIS_TEST_ASSERT(searched->GetDestinationHospital() == "Metro Trauma Center");

    // Negative tests on non-existent unit
    AEGIS_TEST_ASSERT(!registry.UpdatePosition("AMB_GHOST", "INT_A"));
    AEGIS_TEST_ASSERT(!registry.UpdateETA("AMB_GHOST", 10));
    AEGIS_TEST_ASSERT(!registry.UpdatePriority("AMB_GHOST", PriorityLevel::LOW));

    std::cout << "PASS\n";
}

// =============================================================================
// Test 5: Search and Highest Priority Ambulance Selection
// =============================================================================
static void TestHighestPriorityAmbulance() {
    std::cout << "[TEST] 5. Search & Highest Priority Ambulance Arbitration... ";

    AmbulanceRegistry registry;
    AEGIS_TEST_ASSERT(!registry.GetHighestPriorityAmbulance().has_value()); // Empty registry

    // Unit 1: LOW priority, ETA 15 mins
    AmbulanceProfile amb1("AMB-401", AmbulanceStatus::AVAILABLE, "Hospital A", 15);
    amb1.SetPriority(PriorityLevel::LOW);

    // Unit 2: MEDIUM priority (Burn Injury with smoke inhalation), ETA 12 mins
    AmbulanceProfile amb2("AMB-402", AmbulanceStatus::TRANSPORTING, "Hospital B", 12);
    PatientVitals burnVitals;
    burnVitals.spO2 = 88;
    burnVitals.respirationRate = 32;
    burnVitals.heartRate = 132;
    PatientProfile burnPatient("PT-402", "Alex Vance", 28, Gender::MALE, EmergencyType::BURN_INJURY, burnVitals);
    amb2.SetPatient(burnPatient); // Calculates MEDIUM (score: 43.0)

    // Unit 3: HIGH priority (Heart Attack), ETA 10 mins
    AmbulanceProfile amb3("AMB-403", AmbulanceStatus::TRANSPORTING, "Hospital C", 10);
    PatientVitals haVitals;
    haVitals.spO2 = 89;
    haVitals.heartRate = 125;
    haVitals.estimatedTimeSinceIncidentMinutes = 45;
    PatientProfile haPatient("PT-403", "Robert Paulson", 66, Gender::MALE, EmergencyType::HEART_ATTACK, haVitals);
    amb3.SetPatient(haPatient); // Calculates HIGH (score: 56.0)

    // Unit 4: CRITICAL priority (Cardiac Arrest), ETA 6 mins
    AmbulanceProfile amb4("AMB-404", AmbulanceStatus::TRANSPORTING, "Hospital D", 6);
    PatientVitals caVitals;
    caVitals.heartRate = 0;
    caVitals.bloodPressure = BloodPressure{0, 0};
    caVitals.spO2 = 65;
    caVitals.respirationRate = 0;
    caVitals.isConscious = false;
    PatientProfile caPatient("PT-404", "Jane Doe", 55, Gender::FEMALE, EmergencyType::CARDIAC_ARREST, caVitals);
    amb4.SetPatient(caPatient); // Calculates CRITICAL

    registry.RegisterAmbulance(amb1);
    registry.RegisterAmbulance(amb2);
    registry.RegisterAmbulance(amb3);
    registry.RegisterAmbulance(amb4);

    // Should pick AMB-404 (CRITICAL)
    auto highest = registry.GetHighestPriorityAmbulance();
    AEGIS_TEST_ASSERT(highest.has_value());
    AEGIS_TEST_ASSERT(highest->GetAmbulanceId() == "AMB-404");
    AEGIS_TEST_ASSERT(highest->GetPriority() == PriorityLevel::CRITICAL);

    // Remove AMB-404, should pick AMB-403 (HIGH)
    registry.RemoveAmbulance("AMB-404");
    highest = registry.GetHighestPriorityAmbulance();
    AEGIS_TEST_ASSERT(highest.has_value());
    AEGIS_TEST_ASSERT(highest->GetAmbulanceId() == "AMB-403");
    AEGIS_TEST_ASSERT(highest->GetPriority() == PriorityLevel::HIGH);

    // Tie-breaker: Add another HIGH unit with lower ETA
    AmbulanceProfile amb5("AMB-405", AmbulanceStatus::TRANSPORTING, "Hospital E", 4);
    amb5.SetPatient(haPatient); // Same patient, but ETA 4 mins instead of 10 mins
    registry.RegisterAmbulance(amb5);

    highest = registry.GetHighestPriorityAmbulance();
    AEGIS_TEST_ASSERT(highest.has_value());
    AEGIS_TEST_ASSERT(highest->GetAmbulanceId() == "AMB-405"); // ETA 4 beats ETA 10

    std::cout << "PASS\n";
}

// =============================================================================
// Test 6: Bulk Queries & Status Filtering
// =============================================================================
static void TestBulkQueriesAndFiltering() {
    std::cout << "[TEST] 6. Bulk Queries, Filtering, and JSON Serialization... ";

    AmbulanceRegistry registry;
    registry.RegisterAmbulance(AmbulanceProfile("AMB-501", AmbulanceStatus::AVAILABLE));
    registry.RegisterAmbulance(AmbulanceProfile("AMB-502", AmbulanceStatus::ON_CALL));
    registry.RegisterAmbulance(AmbulanceProfile("AMB-503", AmbulanceStatus::TRANSPORTING));
    registry.RegisterAmbulance(AmbulanceProfile("AMB-504", AmbulanceStatus::AT_HOSPITAL));
    registry.RegisterAmbulance(AmbulanceProfile("AMB-505", AmbulanceStatus::RETURNING));

    AEGIS_TEST_ASSERT(registry.GetAllAmbulances().size() == 5);
    AEGIS_TEST_ASSERT(registry.GetAmbulancesByStatus(AmbulanceStatus::AVAILABLE).size() == 1);
    AEGIS_TEST_ASSERT(registry.GetAmbulancesByStatus(AmbulanceStatus::TRANSPORTING).size() == 1);
    AEGIS_TEST_ASSERT(registry.GetAmbulancesByStatus(AmbulanceStatus::RETURNING).size() == 1);

    auto ambOpt = registry.SearchAmbulance("AMB-503");
    AEGIS_TEST_ASSERT(ambOpt.has_value());
    std::string jsonStr = ambOpt->ToJson();
    AEGIS_TEST_ASSERT(jsonStr.find("\"ambulance_id\":\"AMB-503\"") != std::string::npos);
    AEGIS_TEST_ASSERT(jsonStr.find("\"status\":\"TRANSPORTING\"") != std::string::npos);

    registry.Clear();
    AEGIS_TEST_ASSERT(registry.GetAmbulanceCount() == 0);

    std::cout << "PASS\n";
}

// =============================================================================
// Test 7: Multi-threaded Concurrency Safety
// =============================================================================
static void TestThreadSafety() {
    std::cout << "[TEST] 7. Multi-threaded Concurrent Mutators & Queries... ";

    AmbulanceRegistry registry;

    // Seed 20 ambulances
    for (int i = 0; i < 20; ++i) {
        registry.RegisterAmbulance(AmbulanceProfile("AMB-" + std::to_string(i), AmbulanceStatus::AVAILABLE));
    }

    std::vector<std::thread> workers;

    // Worker 1: Rapid position updates
    workers.emplace_back([&]() {
        for (int i = 0; i < 100; ++i) {
            std::string id = "AMB-" + std::to_string(i % 20);
            registry.UpdatePosition(id, "INT_" + std::to_string(i));
        }
    });

    // Worker 2: Rapid ETA updates
    workers.emplace_back([&]() {
        for (int i = 0; i < 100; ++i) {
            std::string id = "AMB-" + std::to_string(i % 20);
            registry.UpdateETA(id, static_cast<uint32_t>(i % 30));
        }
    });

    // Worker 3: Concurrent highest priority queries
    workers.emplace_back([&]() {
        for (int i = 0; i < 100; ++i) {
            auto best = registry.GetHighestPriorityAmbulance();
            (void)best;
        }
    });

    // Worker 4: Concurrent registrations and status updates
    workers.emplace_back([&]() {
        for (int i = 20; i < 40; ++i) {
            registry.RegisterAmbulance(AmbulanceProfile("AMB-" + std::to_string(i), AmbulanceStatus::ON_CALL));
        }
    });

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }

    AEGIS_TEST_ASSERT(registry.GetAmbulanceCount() >= 20);
    std::cout << "PASS (Final units: " << registry.GetAmbulanceCount() << ")\n";
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: AmbulanceProfile & AmbulanceRegistry Unit Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestAmbulanceProfileInitialization();
    TestPatientAssignmentAndPriority();
    TestRegistryRegistrationAndRemoval();
    TestRegistryTelemetryMutators();
    TestHighestPriorityAmbulance();
    TestBulkQueriesAndFiltering();
    TestThreadSafety();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All Ambulance Registry Tests Passed Successfully!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
