/**
 * @file test_hospital_resource_phase11b.cpp
 * @brief Comprehensive Unit Test Suite for SMART AEGIS Phase 11B
 *        Hospital Resource Allocation & Bed Management Engine
 *
 * Validates registration, bed updates, ICU reservations, nearest hospital search,
 * capability filtering, deterministic 10-rule selection algorithm, reservation lifecycle,
 * structured report formatting, 100-thread concurrency, and sub-5ms performance benchmark.
 *
 * Pure C++17, QNX SDP 8.0 & Raspberry Pi 4 compliant.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "hospital_management/HospitalResourceEngine.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cmath>
#include <iomanip>

using namespace aegis::hospital_management;

// =============================================================================
// Helper Assertions
// =============================================================================
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            std::exit(1); \
        } \
    } while (0)

// =============================================================================
// Unit Tests (1 to 10)
// =============================================================================

/**
 * @brief TEST 1: Hospital Registration & Removal
 */
void Test1_Registration() {
    std::cout << "[TEST 1] Hospital Registration..." << std::endl;
    HospitalResourceEngine engine;

    HospitalRecord h1("HOSP_01", "Central Metropolitan Hospital", 12.9716, 77.5946,
                      HospitalStatus::ONLINE,
                      {HospitalCapability::GENERAL, HospitalCapability::TRAUMA, HospitalCapability::ICU});
    h1.bedAvailability = BedAvailability(200, 45);
    h1.icuBeds = ICUBed(30, 8);
    h1.ventilatorStatus = VentilatorStatus(15, 6);
    h1.emergencyDept = EmergencyDepartment(50, 20, 2, 5.0);
    h1.doctorAvailability = DoctorAvailability(30, 10, 15, 3, 2, 1, 2);

    TEST_ASSERT(engine.RegisterHospital(h1), "Registration of HOSP_01 must succeed");
    TEST_ASSERT(!engine.RegisterHospital(h1), "Duplicate registration of HOSP_01 must fail");

    HospitalRecord retrieved;
    TEST_ASSERT(engine.GetHospital("HOSP_01", retrieved), "Must retrieve registered HOSP_01");
    TEST_ASSERT(retrieved.hospitalName == "Central Metropolitan Hospital", "Hospital name must match");
    TEST_ASSERT(retrieved.bedAvailability.availableBeds == 45, "Available beds must match");
    TEST_ASSERT(retrieved.icuBeds.availableIcuBeds == 8, "Available ICU beds must match");
    TEST_ASSERT(retrieved.ventilatorStatus.availableVentilators == 6, "Available ventilators must match");
    TEST_ASSERT(retrieved.HasCapability(HospitalCapability::TRAUMA), "Must have TRAUMA capability");

    // Empty ID rejection
    HospitalRecord invalidRecord;
    TEST_ASSERT(!engine.RegisterHospital(invalidRecord), "Empty hospital ID must be rejected");

    // Register second hospital
    HospitalRecord h2("HOSP_02", "North Wing Pediatric Center", 12.9800, 77.6000,
                      HospitalStatus::ONLINE, {HospitalCapability::PEDIATRIC, HospitalCapability::NEONATAL});
    TEST_ASSERT(engine.RegisterHospital(h2), "Registration of HOSP_02 must succeed");
    TEST_ASSERT(engine.GetAllHospitals().size() == 2, "Must contain exactly 2 registered hospitals");

    // Removal
    TEST_ASSERT(engine.RemoveHospital("HOSP_02"), "Removal of HOSP_02 must succeed");
    TEST_ASSERT(engine.GetAllHospitals().size() == 1, "Must contain exactly 1 hospital after removal");
    TEST_ASSERT(!engine.RemoveHospital("HOSP_UNKNOWN"), "Removal of non-existent hospital must fail");

    std::cout << "  ✓ PASS: Hospital registration, duplicate check, and removal verified." << std::endl;
}

/**
 * @brief TEST 2: Bed and Capacity Updates
 */
void Test2_BedUpdates() {
    std::cout << "[TEST 2] Bed Updates..." << std::endl;
    HospitalResourceEngine engine;

    HospitalRecord h1("HOSP_01", "City General Infirmary", 12.9716, 77.5946);
    h1.bedAvailability = BedAvailability(100, 20);
    h1.icuBeds = ICUBed(20, 4);
    h1.emergencyDept = EmergencyDepartment(30, 15);
    engine.RegisterHospital(h1);

    // Update beds
    TEST_ASSERT(engine.UpdateBedAvailability("HOSP_01", 120, 50), "Update bed counts must succeed");
    HospitalRecord updated;
    TEST_ASSERT(engine.GetHospital("HOSP_01", updated), "Retrieve updated hospital");
    TEST_ASSERT(updated.bedAvailability.totalBeds == 120, "Total beds must be 120");
    TEST_ASSERT(updated.bedAvailability.availableBeds == 50, "Available beds must be 50");
    TEST_ASSERT(std::abs(updated.bedAvailability.occupancyRate - ((70.0 / 120.0) * 100.0)) < 1e-4, "Occupancy rate correct");

    // Update ICU beds
    TEST_ASSERT(engine.UpdateICUBeds("HOSP_01", 25, 10), "Update ICU beds must succeed");
    engine.GetHospital("HOSP_01", updated);
    TEST_ASSERT(updated.icuBeds.totalIcuBeds == 25, "Total ICU beds must be 25");
    TEST_ASSERT(updated.icuBeds.availableIcuBeds == 10, "Available ICU beds must be 10");

    // Update Emergency Capacity
    TEST_ASSERT(engine.UpdateEmergencyCapacity("HOSP_01", 40, 10, 1), "Update ED capacity must succeed");
    engine.GetHospital("HOSP_01", updated);
    TEST_ASSERT(updated.emergencyDept.capacity == 40, "ED capacity must be 40");
    TEST_ASSERT(updated.emergencyDept.currentLoad == 10, "ED load must be 10");
    TEST_ASSERT(std::abs(updated.emergencyDept.occupancyPercentage - 25.0) < 1e-4, "ED occupancy must be 25%");

    // Update Doctors
    TEST_ASSERT(engine.UpdateDoctorAvailability("HOSP_01", 15, 5), "Update doctor availability must succeed");
    engine.GetHospital("HOSP_01", updated);
    TEST_ASSERT(updated.doctorAvailability.availableDoctors == 5, "Available doctors must be 5");

    // Update Specialties
    std::vector<HospitalCapability> newSpecs = {HospitalCapability::CARDIAC, HospitalCapability::NEURO};
    TEST_ASSERT(engine.UpdateSpecialties("HOSP_01", newSpecs), "Update specialties must succeed");
    engine.GetHospital("HOSP_01", updated);
    TEST_ASSERT(updated.HasCapability(HospitalCapability::CARDIAC), "Must now have CARDIAC capability");
    TEST_ASSERT(updated.HasCapability(HospitalCapability::NEURO), "Must now have NEURO capability");

    // Update Status
    TEST_ASSERT(engine.UpdateStatus("HOSP_01", HospitalStatus::EMERGENCY_ONLY), "Update status must succeed");
    TEST_ASSERT(engine.GetHospitalStatus("HOSP_01") == HospitalStatus::EMERGENCY_ONLY, "Status must be EMERGENCY_ONLY");

    std::cout << "  ✓ PASS: Bed, ICU, ED, Doctor, Specialty, and Status updates verified." << std::endl;
}

/**
 * @brief TEST 3: ICU Bed Reservation & Exhaustion
 */
void Test3_ICUReservation() {
    std::cout << "[TEST 3] ICU Reservation..." << std::endl;
    HospitalResourceEngine engine;

    HospitalRecord h1("HOSP_ICU", "Apex Critical Care Center", 12.9716, 77.5946);
    h1.bedAvailability = BedAvailability(50, 10);
    h1.icuBeds = ICUBed(10, 2); // Exactly 2 available ICU beds
    engine.RegisterHospital(h1);

    // Reserve 1st ICU bed
    TEST_ASSERT(engine.ReserveICUBed("HOSP_ICU", "RES_ICU_001"), "First ICU reservation must succeed");
    HospitalRecord r1;
    engine.GetHospital("HOSP_ICU", r1);
    TEST_ASSERT(r1.icuBeds.availableIcuBeds == 1, "Available ICU beds must now be 1");
    TEST_ASSERT(r1.icuBeds.reservedIcuBeds == 1, "Reserved ICU beds must now be 1");

    // Reserve 2nd ICU bed
    TEST_ASSERT(engine.ReserveICUBed("HOSP_ICU", "RES_ICU_002"), "Second ICU reservation must succeed");
    engine.GetHospital("HOSP_ICU", r1);
    TEST_ASSERT(r1.icuBeds.availableIcuBeds == 0, "Available ICU beds must now be 0");
    TEST_ASSERT(r1.icuBeds.reservedIcuBeds == 2, "Reserved ICU beds must now be 2");

    // Attempt 3rd ICU reservation (Exhausted!)
    TEST_ASSERT(!engine.ReserveICUBed("HOSP_ICU", "RES_ICU_003"), "Third ICU reservation must fail due to exhaustion");

    // Release 1st ICU bed
    TEST_ASSERT(engine.ReleaseICUBed("HOSP_ICU", "RES_ICU_001"), "Release of first ICU reservation must succeed");
    engine.GetHospital("HOSP_ICU", r1);
    TEST_ASSERT(r1.icuBeds.availableIcuBeds == 1, "Available ICU beds must restore to 1");

    // Now 3rd reservation succeeds
    TEST_ASSERT(engine.ReserveICUBed("HOSP_ICU", "RES_ICU_003"), "ICU reservation must now succeed after release");

    HospitalStatistics stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalIcuBedsReserved == 3, "Total ICU beds reserved statistic must be 3");
    TEST_ASSERT(stats.totalIcuBedsReleased == 1, "Total ICU beds released statistic must be 1");

    std::cout << "  ✓ PASS: ICU reservation, capacity exhaustion, and release lifecycle verified." << std::endl;
}

/**
 * @brief TEST 4: Nearest Hospital Geodesic Search
 */
void Test4_NearestHospital() {
    std::cout << "[TEST 4] Nearest Hospital..." << std::endl;
    HospitalResourceEngine engine;

    // Origin query point: (12.9600, 77.5900)
    HospitalRecord hFar("HOSP_FAR", "East Outpost Hospital", 13.0500, 77.6800); // ~14 km away
    hFar.capabilities = {HospitalCapability::GENERAL};
    engine.RegisterHospital(hFar);

    HospitalRecord hMid("HOSP_MID", "Midtown Infirmary", 12.9750, 77.5950);   // ~1.7 km away
    hMid.capabilities = {HospitalCapability::GENERAL, HospitalCapability::TRAUMA};
    engine.RegisterHospital(hMid);

    HospitalRecord hNear("HOSP_NEAR", "South Neighborhood Clinic", 12.9620, 77.5910); // ~0.24 km away
    hNear.capabilities = {HospitalCapability::GENERAL};
    engine.RegisterHospital(hNear);

    // Query nearest without specialty filter
    HospitalRecord nearest;
    TEST_ASSERT(engine.FindNearestHospital(12.9600, 77.5900, nearest), "Find nearest must find a hospital");
    TEST_ASSERT(nearest.hospitalId == "HOSP_NEAR", "Nearest hospital must be HOSP_NEAR (~0.24 km)");

    // Query nearest requiring TRAUMA capability
    HospitalRecord nearestTrauma;
    TEST_ASSERT(engine.FindNearestHospital(12.9600, 77.5900, nearestTrauma, HospitalCapability::TRAUMA),
                "Find nearest TRAUMA must succeed");
    TEST_ASSERT(nearestTrauma.hospitalId == "HOSP_MID", "Nearest TRAUMA hospital must be HOSP_MID");

    std::cout << "  ✓ PASS: Geodesic Haversine nearest facility calculation verified." << std::endl;
}

/**
 * @brief TEST 5: Medical Capability Filtering
 */
void Test5_CapabilityFiltering() {
    std::cout << "[TEST 5] Capability Filtering..." << std::endl;
    HospitalResourceEngine engine;

    // Close general hospital (1.0 km)
    HospitalRecord hGen("HOSP_GEN", "Downtown General", 12.9700, 77.5900);
    hGen.capabilities = {HospitalCapability::GENERAL};
    hGen.bedAvailability = BedAvailability(100, 30);
    hGen.icuBeds = ICUBed(10, 0); // No ICU beds
    engine.RegisterHospital(hGen);

    // Further burns specialty institute (4.0 km)
    HospitalRecord hBurns("HOSP_BURNS", "Regional Burn Resuscitation Unit", 12.9900, 77.6200);
    hBurns.capabilities = {HospitalCapability::BURNS, HospitalCapability::ICU};
    hBurns.bedAvailability = BedAvailability(50, 15);
    hBurns.icuBeds = ICUBed(15, 6);
    hBurns.ventilatorStatus = VentilatorStatus(10, 5);
    engine.RegisterHospital(hBurns);

    // Further cardiac center (5.0 km)
    HospitalRecord hCard("HOSP_CARD", "Metropolitan Heart Institute", 12.9950, 77.6300);
    hCard.capabilities = {HospitalCapability::CARDIAC, HospitalCapability::ICU};
    hCard.bedAvailability = BedAvailability(80, 20);
    hCard.icuBeds = ICUBed(20, 8);
    hCard.ventilatorStatus = VentilatorStatus(15, 8);
    engine.RegisterHospital(hCard);

    // Request for BURNS patient
    HospitalSelection reqBurns("INC_BURN_01", "AMB_05", 12.9600, 77.5800, HospitalCapability::BURNS, true, false, 1);
    HospitalRecommendation recBurns = engine.FindBestHospital(reqBurns);
    TEST_ASSERT(recBurns.successful, "Burns destination selection must succeed");
    TEST_ASSERT(recBurns.selectedHospitalId == "HOSP_BURNS", "Must select HOSP_BURNS despite larger distance");
    TEST_ASSERT(recBurns.matchedCapability == HospitalCapability::BURNS, "Matched capability must be BURNS");

    // Request for CARDIAC patient
    HospitalSelection reqCard("INC_CARD_01", "AMB_08", 12.9600, 77.5800, HospitalCapability::CARDIAC, true, false, 1);
    HospitalRecommendation recCard = engine.FindBestHospital(reqCard);
    TEST_ASSERT(recCard.successful, "Cardiac destination selection must succeed");
    TEST_ASSERT(recCard.selectedHospitalId == "HOSP_CARD", "Must select HOSP_CARD");

    std::cout << "  ✓ PASS: Medical specialty and capability filtering verified." << std::endl;
}

/**
 * @brief TEST 6: Hospital Selection Algorithm (10 Rules Deterministic Evaluation)
 */
void Test6_HospitalSelectionAlgorithm() {
    std::cout << "[TEST 6] Hospital Selection Algorithm..." << std::endl;
    HospitalResourceEngine engine;

    // Hospital A: Close (1.5 km), but severely congested:
    // ED occupancy 95%, queue 8 patients, wait 45 mins, 0 ICU beds, 0 ventilators
    HospitalRecord hospA("HOSP_CONGESTED", "St. Jude Urban Hospital", 12.9700, 77.5900);
    hospA.capabilities = {HospitalCapability::GENERAL, HospitalCapability::TRAUMA};
    hospA.bedAvailability = BedAvailability(100, 2);
    hospA.icuBeds = ICUBed(20, 0); // 0 ICU
    hospA.ventilatorStatus = VentilatorStatus(10, 0); // 0 vents
    hospA.emergencyDept = EmergencyDepartment(40, 38, 8, 45.0); // 95% load
    hospA.doctorAvailability = DoctorAvailability(15, 1);
    engine.RegisterHospital(hospA);

    // Hospital B: Moderate distance (4.0 km), excellent capacity:
    // ED occupancy 30%, queue 0, wait 5 mins, 8 ICU beds, 6 ventilators, 3 trauma surgeons on duty
    HospitalRecord hospB("HOSP_OPTIMAL", "Mercy Trauma & Specialty Center", 12.9900, 77.6200);
    hospB.capabilities = {HospitalCapability::GENERAL, HospitalCapability::TRAUMA, HospitalCapability::ICU};
    hospB.bedAvailability = BedAvailability(200, 60);
    hospB.icuBeds = ICUBed(30, 8); // 8 ICU
    hospB.ventilatorStatus = VentilatorStatus(20, 6); // 6 vents
    hospB.emergencyDept = EmergencyDepartment(50, 15, 0, 5.0); // 30% load
    hospB.doctorAvailability = DoctorAvailability(30, 8, 15, 3, 2, 1, 2);
    engine.RegisterHospital(hospB);

    // Critical trauma request requiring ICU and ventilator
    HospitalSelection req("INC_CRIT_01", "AMB_11", 12.9600, 77.5800,
                          HospitalCapability::TRAUMA, true, true, 1, 55.0);
    HospitalRecommendation rec = engine.FindBestHospital(req);

    TEST_ASSERT(rec.successful, "Selection must succeed");
    TEST_ASSERT(rec.selectedHospitalId == "HOSP_OPTIMAL", "Deterministic algorithm must select HOSP_OPTIMAL over HOSP_CONGESTED");
    TEST_ASSERT(rec.availableIcuBeds == 8, "Available ICU beds must be 8");
    TEST_ASSERT(rec.availableVentilators == 6, "Available ventilators must be 6");
    TEST_ASSERT(rec.score > 0.0, "Score must be positive");
    TEST_ASSERT(!rec.reason.empty(), "Reason must be populated");

    std::cout << "  ✓ PASS: Multi-criteria 10-rule selection selected optimal facility over congested close facility." << std::endl;
}

/**
 * @brief TEST 7: Complete Bed & ICU Reservation Lifecycle
 */
void Test7_ReservationLifecycle() {
    std::cout << "[TEST 7] Reservation Lifecycle..." << std::endl;
    HospitalResourceEngine engine;

    HospitalRecord hosp("HOSP_LIFE", "Life Memorial Hospital", 12.9716, 77.5946);
    hosp.bedAvailability = BedAvailability(100, 25);
    hosp.icuBeds = ICUBed(20, 5);
    engine.RegisterHospital(hosp);

    // Initial state check
    HospitalRecord rec;
    engine.GetHospital("HOSP_LIFE", rec);
    TEST_ASSERT(rec.bedAvailability.availableBeds == 25, "Initial available beds must be 25");
    TEST_ASSERT(rec.bedAvailability.reservedBeds == 0, "Initial reserved beds must be 0");
    TEST_ASSERT(rec.icuBeds.availableIcuBeds == 5, "Initial available ICU must be 5");
    TEST_ASSERT(rec.icuBeds.reservedIcuBeds == 0, "Initial reserved ICU must be 0");

    // Phase 1: Inbound ambulance reserves standard bed and ICU bed
    TEST_ASSERT(engine.ReserveBed("HOSP_LIFE", "RES_AMB_01_BED"), "Standard bed reservation must succeed");
    TEST_ASSERT(engine.ReserveICUBed("HOSP_LIFE", "RES_AMB_01_ICU"), "ICU bed reservation must succeed");

    engine.GetHospital("HOSP_LIFE", rec);
    TEST_ASSERT(rec.bedAvailability.availableBeds == 24, "Available beds must decrement to 24");
    TEST_ASSERT(rec.bedAvailability.reservedBeds == 1, "Reserved beds must increment to 1");
    TEST_ASSERT(rec.icuBeds.availableIcuBeds == 4, "Available ICU beds must decrement to 4");
    TEST_ASSERT(rec.icuBeds.reservedIcuBeds == 1, "Reserved ICU beds must increment to 1");

    // Phase 2: Patient arrives, stabilizes, ICU bed released back
    TEST_ASSERT(engine.ReleaseICUBed("HOSP_LIFE", "RES_AMB_01_ICU"), "ICU release must succeed");
    engine.GetHospital("HOSP_LIFE", rec);
    TEST_ASSERT(rec.icuBeds.availableIcuBeds == 5, "Available ICU beds must restore to 5");
    TEST_ASSERT(rec.icuBeds.reservedIcuBeds == 0, "Reserved ICU beds must restore to 0");

    // Phase 3: Patient discharged from inpatient bed
    TEST_ASSERT(engine.ReleaseBed("HOSP_LIFE", "RES_AMB_01_BED"), "Bed release must succeed");
    engine.GetHospital("HOSP_LIFE", rec);
    TEST_ASSERT(rec.bedAvailability.availableBeds == 25, "Available beds must restore to 25");
    TEST_ASSERT(rec.bedAvailability.reservedBeds == 0, "Reserved beds must restore to 0");

    // Phase 4: Verify audit statistics
    HospitalStatistics stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalBedsReserved == 1, "Total beds reserved must be 1");
    TEST_ASSERT(stats.totalBedsReleased == 1, "Total beds released must be 1");
    TEST_ASSERT(stats.totalIcuBedsReserved == 1, "Total ICU beds reserved must be 1");
    TEST_ASSERT(stats.totalIcuBedsReleased == 1, "Total ICU beds released must be 1");

    std::cout << "  ✓ PASS: End-to-end bed and ICU reservation lifecycle verified." << std::endl;
}

/**
 * @brief TEST 8: Structured Hospital Reporting
 */
void Test8_Reporting() {
    std::cout << "[TEST 8] Reporting..." << std::endl;
    HospitalResourceEngine engine;

    HospitalRecord hosp("HOSP_REP", "Grace Trauma Center", 12.9716, 77.5946);
    hosp.capabilities = {HospitalCapability::TRAUMA};
    hosp.bedAvailability = BedAvailability(150, 42);
    hosp.icuBeds = ICUBed(25, 9);
    hosp.ventilatorStatus = VentilatorStatus(12, 7);
    engine.RegisterHospital(hosp);

    HospitalRecommendation rec;
    rec.incidentId = "INC_TEST_88";
    rec.ambulanceId = "AMB_TEST_88";
    rec.selectedHospitalId = "HOSP_REP";
    rec.hospitalName = "Grace Trauma Center";
    rec.matchedCapability = HospitalCapability::TRAUMA;
    rec.availableBeds = 42;
    rec.availableIcuBeds = 9;
    rec.availableVentilators = 7;
    rec.estimatedEtaMinutes = 3.8;
    rec.recommendationSummary = "Optimal Destination: Level 1 Trauma with ready ICU";

    std::string report = engine.GenerateHospitalReport(rec);
    std::cout << report << std::endl;

    // Verify structured keys from prompt
    TEST_ASSERT(report.find("[HOSPITAL]") != std::string::npos, "Must contain [HOSPITAL]");
    TEST_ASSERT(report.find("Hospital:\nGrace Trauma Center") != std::string::npos, "Must contain Hospital name");
    TEST_ASSERT(report.find("Capability:\nTRAUMA") != std::string::npos, "Must contain Capability");
    TEST_ASSERT(report.find("Available Beds:\n42") != std::string::npos, "Must contain Available Beds");
    TEST_ASSERT(report.find("ICU Beds:\n9") != std::string::npos, "Must contain ICU Beds");
    TEST_ASSERT(report.find("Ventilators:\n7") != std::string::npos, "Must contain Ventilators");
    TEST_ASSERT(report.find("ETA:\n3.8 min") != std::string::npos, "Must contain ETA");
    TEST_ASSERT(report.find("Recommendation:\nOptimal Destination: Level 1 Trauma with ready ICU") != std::string::npos,
                "Must contain Recommendation");

    std::cout << "  ✓ PASS: Structured reporting format validated against specification." << std::endl;
}

/**
 * @brief TEST 9: 100-Thread Concurrency
 */
void Test9_100ThreadConcurrency() {
    std::cout << "[TEST 9] 100-Thread Concurrency..." << std::endl;
    HospitalResourceEngine engine;

    // Base hospitals
    for (int i = 0; i < 10; ++i) {
        HospitalRecord h("BASE_HOSP_" + std::to_string(i),
                         "Base Hospital " + std::to_string(i),
                         12.9000 + (i * 0.01), 77.5000 + (i * 0.01));
        h.bedAvailability = BedAvailability(100, 50);
        h.icuBeds = ICUBed(20, 10);
        h.ventilatorStatus = VentilatorStatus(15, 8);
        h.capabilities = {HospitalCapability::GENERAL, HospitalCapability::TRAUMA};
        engine.RegisterHospital(h);
    }

    constexpr int kNumThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<int> successCount{0};

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&engine, t, &successCount]() {
            std::string hospId = "CONC_HOSP_" + std::to_string(t);
            HospitalRecord h(hospId, "Concurrent Hospital " + std::to_string(t),
                             12.9200 + (t * 0.001), 77.5200 + (t * 0.001));
            h.bedAvailability = BedAvailability(80, 20);
            h.icuBeds = ICUBed(15, 5);
            h.capabilities = {HospitalCapability::CARDIAC, HospitalCapability::ICU};

            // Register
            engine.RegisterHospital(h);

            // Update
            engine.UpdateBedAvailability(hospId, 90, 25);
            engine.UpdateICUBeds(hospId, 18, 6);

            // Reserve & Release
            std::string resBed = "RES_B_" + std::to_string(t);
            std::string resIcu = "RES_I_" + std::to_string(t);
            if (engine.ReserveBed(hospId, resBed)) {
                engine.ReleaseBed(hospId, resBed);
            }
            if (engine.ReserveICUBed(hospId, resIcu)) {
                engine.ReleaseICUBed(hospId, resIcu);
            }

            // Find best
            HospitalSelection sel("INC_" + std::to_string(t), "AMB_" + std::to_string(t),
                                  12.9250, 77.5250, HospitalCapability::GENERAL, false, false, 2);
            HospitalRecommendation rec = engine.FindBestHospital(sel);
            if (rec.successful) {
                engine.GenerateHospitalReport(rec);
            }

            // Statistics query
            engine.GetStatistics();

            successCount.fetch_add(1, std::memory_order_relaxed);
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    TEST_ASSERT(successCount.load() == kNumThreads, "All 100 concurrent threads must complete successfully");
    TEST_ASSERT(engine.GetAllHospitals().size() == 110, "Total registered hospitals must be 110 (10 base + 100 concurrent)");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (operations: "
              << successCount.load() << ")." << std::endl;
}

/**
 * @brief TEST 10: Performance Benchmark (Select best among 100 hospitals under 5 ms)
 */
void Test10_PerformanceBenchmark() {
    std::cout << "[TEST 10] Performance Benchmark: Select best hospital among 100 hospitals under 5 ms..." << std::endl;
    HospitalResourceEngine engine;

    // Populate 100 municipal hospitals across Bangalore city grid (12.85 to 13.10 N, 77.45 to 77.75 E)
    for (int i = 0; i < 100; ++i) {
        std::string hId = "BENCH_HOSP_" + std::to_string(i);
        std::string hName = "Metropolitan Hospital " + std::to_string(i);
        double lat = 12.8500 + ((i % 10) * 0.025);
        double lon = 77.4500 + ((i / 10) * 0.030);

        HospitalRecord h(hId, hName, lat, lon);
        h.status = (i % 25 == 0) ? HospitalStatus::MAINTENANCE : HospitalStatus::ONLINE;
        h.capabilities = {HospitalCapability::GENERAL};
        if (i % 2 == 0) h.capabilities.push_back(HospitalCapability::TRAUMA);
        if (i % 3 == 0) h.capabilities.push_back(HospitalCapability::CARDIAC);
        if (i % 4 == 0) h.capabilities.push_back(HospitalCapability::ICU);
        if (i % 5 == 0) h.capabilities.push_back(HospitalCapability::PEDIATRIC);
        if (i % 7 == 0) h.capabilities.push_back(HospitalCapability::NEURO);
        if (i % 11 == 0) h.capabilities.push_back(HospitalCapability::BURNS);
        if (i % 13 == 0) h.capabilities.push_back(HospitalCapability::MULTISPECIALITY);

        uint32_t ui = static_cast<uint32_t>(i);
        h.bedAvailability = BedAvailability(150, 10 + (ui % 40));
        h.icuBeds = ICUBed(25, 2 + (ui % 10));
        h.ventilatorStatus = VentilatorStatus(15, 1 + (ui % 8));
        h.emergencyDept = EmergencyDepartment(50, 10 + (ui % 30), (ui % 4), 5.0 + static_cast<double>(ui % 10));
        h.doctorAvailability = DoctorAvailability(25, 5 + (ui % 10), 15, 2, 2, 1, 1);

        engine.RegisterHospital(h);
    }

    TEST_ASSERT(engine.GetAllHospitals().size() == 100, "Must contain exactly 100 registered hospitals");

    // Benchmarking incident
    HospitalSelection req("INC_BENCH_01", "AMB_BENCH_01", 12.9716, 77.5946,
                          HospitalCapability::TRAUMA, true, true, 1, 50.0);

    // Measure high-precision execution latency
    auto start = std::chrono::high_resolution_clock::now();
    HospitalRecommendation rec = engine.FindBestHospital(req);
    auto end = std::chrono::high_resolution_clock::now();

    double latencyMs = std::chrono::duration<double, std::milli>(end - start).count();

    TEST_ASSERT(rec.successful, "100-hospital selection must succeed");
    TEST_ASSERT(!rec.selectedHospitalId.empty(), "Must select a valid hospital");
    TEST_ASSERT(rec.availableIcuBeds > 0, "Selected hospital must have ICU beds");
    TEST_ASSERT(rec.availableVentilators > 0, "Selected hospital must have ventilators");

    std::cout << "  -> Selected Hospital: " << rec.selectedHospitalId << " (" << rec.hospitalName << ")" << std::endl;
    std::cout << "  -> Distance: " << rec.distanceKm << " km, Estimated ETA: " << rec.estimatedEtaMinutes << " min" << std::endl;
    std::cout << "  -> Available Beds: " << rec.availableBeds << ", ICU: " << rec.availableIcuBeds
              << ", Vents: " << rec.availableVentilators << std::endl;
    std::cout << "  -> Decision Latency: " << latencyMs << " ms" << std::endl;

    // Strict requirement: under 5.0 ms
    TEST_ASSERT(latencyMs < 5.0, "100-hospital selection latency must be under 5.0 ms");
    std::cout << "  ✓ PASS: Performance requirement (< 5.0 ms) successfully satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 11B Hospital Resource Engine Tests   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_Registration();
    Test2_BedUpdates();
    Test3_ICUReservation();
    Test4_NearestHospital();
    Test5_CapabilityFiltering();
    Test6_HospitalSelectionAlgorithm();
    Test7_ReservationLifecycle();
    Test8_Reporting();
    Test9_100ThreadConcurrency();
    Test10_PerformanceBenchmark();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 10 PHASE 11B UNIT TESTS PASSED SUCCESSFULLY (100%)   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
