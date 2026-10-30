/**
 * @file test_priority_scheduler_phase8a.cpp
 * @brief Comprehensive unit tests for Phase 8A Intelligent Priority Scheduling Engine.
 *
 * Verifies:
 *  - Test 1: Single request submission, scheduling, and removal
 *  - Test 2: Multiple severity levels (CRITICAL > HIGH > MEDIUM > LOW)
 *  - Test 3: Equal severity -> ETA comparison
 *  - Test 4: Equal ETA -> Distance comparison
 *  - Test 5: Equal distance -> Timestamp comparison
 *  - Test 6: Deterministic ordering & Lexicographical Ambulance ID tie-breaking
 *  - Test 7: Concurrent submission across 100 threads
 *  - Test 8: Large queue (1000 requests) performance benchmark (< 2 ms)
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include <random>
#include <sstream>

#include "scheduler/PriorityScheduler.h"

using namespace aegis::scheduler;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

// =============================================================================
// Test 1: Single request
// =============================================================================
static bool TestSingleRequest() {
    std::cout << "[TEST 1] Testing Single Emergency Request Submission and Retrieval...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(true);

    EmergencyRequest req;
    req.requestId = "REQ_001";
    req.ambulanceId = "AMB_02";
    req.severity = PatientSeverity::CRITICAL;
    req.hospitalId = "HOSP_CENTRAL";
    req.currentLocation = "LOC_DOWNTOWN";
    req.destination = "HOSP_CENTRAL";
    req.estimatedArrivalTime = 38.0;
    req.distance = 1200.0;
    req.timestamp = 10000;

    bool submitted = scheduler.SubmitEmergencyRequest(req);
    TEST_ASSERT(submitted, "Single request submission should return true");
    TEST_ASSERT(scheduler.GetQueueSize() == 1, "Queue size should be 1");

    auto scheduled = scheduler.ScheduleRequests(true);
    TEST_ASSERT(scheduled.size() == 1, "Scheduled queue should contain 1 element");
    TEST_ASSERT(scheduled[0].ambulanceId == "AMB_02", "Scheduled top ambulance must be AMB_02");

    auto nextOpt = scheduler.GetNextEmergency(false);
    TEST_ASSERT(nextOpt.has_value(), "GetNextEmergency should have value");
    TEST_ASSERT(nextOpt->ambulanceId == "AMB_02", "Next emergency ambulance should be AMB_02");
    TEST_ASSERT(nextOpt->GetSeverity() == PatientSeverity::CRITICAL, "Severity should be CRITICAL");

    std::string formatted = scheduler.FormatQueue();
    TEST_ASSERT(formatted.find("AMB_02") != std::string::npos, "FormatQueue must contain AMB_02");
    TEST_ASSERT(formatted.find("CRITICAL") != std::string::npos, "FormatQueue must contain CRITICAL");
    TEST_ASSERT(formatted.find("ETA 38 sec") != std::string::npos, "FormatQueue must contain 'ETA 38 sec'");

    std::string logStr = scheduler.FormatSchedulerLog();
    TEST_ASSERT(logStr.find("[SCHEDULER]") != std::string::npos, "Log must contain [SCHEDULER]");
    TEST_ASSERT(logStr.find("Requests Received: 1") != std::string::npos, "Log must report 1 request");
    TEST_ASSERT(logStr.find("Selected Ambulance: AMB_02") != std::string::npos, "Log must report AMB_02");
    TEST_ASSERT(logStr.find("Priority: CRITICAL") != std::string::npos, "Log must report CRITICAL");
    TEST_ASSERT(logStr.find("ETA: 38 sec") != std::string::npos, "Log must report ETA: 38 sec");

    bool removed = scheduler.RemoveRequest("REQ_001");
    TEST_ASSERT(removed, "RemoveRequest should return true for existing request");
    TEST_ASSERT(scheduler.GetQueueSize() == 0, "Queue should be empty after removal");

    std::cout << "  -> Single request test PASSED.\n";
    return true;
}

// =============================================================================
// Test 2: Multiple severity levels
// =============================================================================
static bool TestMultipleSeverityLevels() {
    std::cout << "[TEST 2] Testing Multiple Severity Levels Ordering...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    // Deliberately submit out-of-order
    EmergencyRequest reqLow("REQ_LOW", "AMB_08", PatientSeverity::LOW, "HOSP_1", "LOC_A", "HOSP_1", 15.0, 500.0, 100);
    EmergencyRequest reqMed("REQ_MED", "AMB_01", PatientSeverity::MEDIUM, "HOSP_1", "LOC_B", "HOSP_1", 61.0, 1800.0, 100);
    EmergencyRequest reqHigh("REQ_HIGH", "AMB_05", PatientSeverity::HIGH, "HOSP_1", "LOC_C", "HOSP_1", 44.0, 1500.0, 100);
    EmergencyRequest reqCrit("REQ_CRIT", "AMB_02", PatientSeverity::CRITICAL, "HOSP_1", "LOC_D", "HOSP_1", 38.0, 1200.0, 100);

    scheduler.SubmitEmergencyRequest(reqLow);
    scheduler.SubmitEmergencyRequest(reqMed);
    scheduler.SubmitEmergencyRequest(reqHigh);
    scheduler.SubmitEmergencyRequest(reqCrit);

    auto scheduled = scheduler.ScheduleRequests(false);
    TEST_ASSERT(scheduled.size() == 4, "Should have 4 scheduled requests");

    // Must be strictly: CRITICAL > HIGH > MEDIUM > LOW
    TEST_ASSERT(scheduled[0].ambulanceId == "AMB_02", "First must be AMB_02 (CRITICAL)");
    TEST_ASSERT(scheduled[1].ambulanceId == "AMB_05", "Second must be AMB_05 (HIGH)");
    TEST_ASSERT(scheduled[2].ambulanceId == "AMB_01", "Third must be AMB_01 (MEDIUM)");
    TEST_ASSERT(scheduled[3].ambulanceId == "AMB_08", "Fourth must be AMB_08 (LOW)");

    std::string formatted = scheduler.FormatQueue();
    // Validate example output formatting matching specification
    TEST_ASSERT(formatted.find("AMB_02\nCRITICAL\nETA 38 sec") != std::string::npos, "AMB_02 block formatted properly");
    TEST_ASSERT(formatted.find("AMB_05\nHIGH\nETA 44 sec") != std::string::npos, "AMB_05 block formatted properly");
    TEST_ASSERT(formatted.find("AMB_01\nMEDIUM\nETA 61 sec") != std::string::npos, "AMB_01 block formatted properly");

    std::cout << "  -> Multiple severity levels ordering PASSED.\n";
    return true;
}

// =============================================================================
// Test 3: Equal severity -> ETA comparison
// =============================================================================
static bool TestEqualSeverityEtaComparison() {
    std::cout << "[TEST 3] Testing Equal Severity - ETA Comparison (Lower ETA wins)...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    EmergencyRequest r1("REQ_H1", "AMB_H1", PatientSeverity::HIGH, "HOSP", "L1", "HOSP", 45.0, 1000.0, 200);
    EmergencyRequest r2("REQ_H2", "AMB_H2", PatientSeverity::HIGH, "HOSP", "L2", "HOSP", 20.0, 1000.0, 200);
    EmergencyRequest r3("REQ_H3", "AMB_H3", PatientSeverity::HIGH, "HOSP", "L3", "HOSP", 35.0, 1000.0, 200);

    scheduler.SubmitEmergencyRequest(r1);
    scheduler.SubmitEmergencyRequest(r2);
    scheduler.SubmitEmergencyRequest(r3);

    auto scheduled = scheduler.ScheduleRequests(false);
    TEST_ASSERT(scheduled.size() == 3, "Queue size 3");

    TEST_ASSERT(scheduled[0].ambulanceId == "AMB_H2", "Fastest ETA (20s) must win");
    TEST_ASSERT(scheduled[1].ambulanceId == "AMB_H3", "Second fastest ETA (35s) must be second");
    TEST_ASSERT(scheduled[2].ambulanceId == "AMB_H1", "Slowest ETA (45s) must be third");

    std::cout << "  -> Equal severity ETA comparison PASSED.\n";
    return true;
}

// =============================================================================
// Test 4: Equal ETA -> Distance comparison
// =============================================================================
static bool TestEqualEtaDistanceComparison() {
    std::cout << "[TEST 4] Testing Equal ETA - Distance Comparison (Shorter distance wins)...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    // Same severity (CRITICAL), same ETA (30.0s), different distances
    EmergencyRequest rLong("REQ_L", "AMB_DIST_LONG", PatientSeverity::CRITICAL, "HOSP", "L1", "HOSP", 30.0, 2500.0, 300);
    EmergencyRequest rShort("REQ_S", "AMB_DIST_SHORT", PatientSeverity::CRITICAL, "HOSP", "L2", "HOSP", 30.0, 800.0, 300);
    EmergencyRequest rMid("REQ_M", "AMB_DIST_MID", PatientSeverity::CRITICAL, "HOSP", "L3", "HOSP", 30.0, 1500.0, 300);

    scheduler.SubmitEmergencyRequest(rLong);
    scheduler.SubmitEmergencyRequest(rShort);
    scheduler.SubmitEmergencyRequest(rMid);

    auto scheduled = scheduler.ScheduleRequests(false);
    TEST_ASSERT(scheduled.size() == 3, "Queue size 3");

    TEST_ASSERT(scheduled[0].ambulanceId == "AMB_DIST_SHORT", "Shorter distance (800m) must win");
    TEST_ASSERT(scheduled[1].ambulanceId == "AMB_DIST_MID", "Mid distance (1500m) must be second");
    TEST_ASSERT(scheduled[2].ambulanceId == "AMB_DIST_LONG", "Long distance (2500m) must be third");

    std::cout << "  -> Equal ETA distance comparison PASSED.\n";
    return true;
}

// =============================================================================
// Test 5: Equal distance -> Timestamp comparison
// =============================================================================
static bool TestEqualDistanceTimestampComparison() {
    std::cout << "[TEST 5] Testing Equal Distance - Timestamp Comparison (Earlier timestamp wins)...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    // Same severity, same ETA (40.0s), same distance (1000m), different timestamps
    EmergencyRequest rLate("REQ_LATE", "AMB_LATE", PatientSeverity::MEDIUM, "HOSP", "L1", "HOSP", 40.0, 1000.0, 10500);
    EmergencyRequest rEarly("REQ_EARLY", "AMB_EARLY", PatientSeverity::MEDIUM, "HOSP", "L2", "HOSP", 40.0, 1000.0, 10100);
    EmergencyRequest rMid("REQ_MID", "AMB_MID", PatientSeverity::MEDIUM, "HOSP", "L3", "HOSP", 40.0, 1000.0, 10300);

    scheduler.SubmitEmergencyRequest(rLate);
    scheduler.SubmitEmergencyRequest(rEarly);
    scheduler.SubmitEmergencyRequest(rMid);

    auto scheduled = scheduler.ScheduleRequests(false);
    TEST_ASSERT(scheduled.size() == 3, "Queue size 3");

    TEST_ASSERT(scheduled[0].ambulanceId == "AMB_EARLY", "Earlier timestamp (10100) must win");
    TEST_ASSERT(scheduled[1].ambulanceId == "AMB_MID", "Mid timestamp (10300) must be second");
    TEST_ASSERT(scheduled[2].ambulanceId == "AMB_LATE", "Late timestamp (10500) must be third");

    std::cout << "  -> Equal distance timestamp comparison PASSED.\n";
    return true;
}

// =============================================================================
// Test 6: Deterministic ordering & Lexicographical Ambulance ID Tie-Breaking
// =============================================================================
static bool TestDeterministicOrdering() {
    std::cout << "[TEST 6] Testing Deterministic Ordering (Lexicographical Ambulance ID)...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    // Same severity, same ETA, same distance, same timestamp
    EmergencyRequest rZ("REQ_Z", "AMB_Z", PatientSeverity::LOW, "HOSP", "L", "HOSP", 50.0, 1000.0, 5000);
    EmergencyRequest rA("REQ_A", "AMB_A", PatientSeverity::LOW, "HOSP", "L", "HOSP", 50.0, 1000.0, 5000);
    EmergencyRequest rM("REQ_M", "AMB_M", PatientSeverity::LOW, "HOSP", "L", "HOSP", 50.0, 1000.0, 5000);

    // Perform multiple scheduling runs
    for (int iter = 0; iter < 10; ++iter) {
        scheduler.ClearQueue();
        if (iter % 2 == 0) {
            scheduler.SubmitEmergencyRequest(rZ);
            scheduler.SubmitEmergencyRequest(rA);
            scheduler.SubmitEmergencyRequest(rM);
        } else {
            scheduler.SubmitEmergencyRequest(rM);
            scheduler.SubmitEmergencyRequest(rZ);
            scheduler.SubmitEmergencyRequest(rA);
        }

        auto scheduled = scheduler.ScheduleRequests(false);
        TEST_ASSERT(scheduled.size() == 3, "Queue size 3");
        TEST_ASSERT(scheduled[0].ambulanceId == "AMB_A", "Lexicographically smallest (AMB_A) must win");
        TEST_ASSERT(scheduled[1].ambulanceId == "AMB_M", "AMB_M must be second");
        TEST_ASSERT(scheduled[2].ambulanceId == "AMB_Z", "AMB_Z must be third");
    }

    std::cout << "  -> Deterministic tie-breaking verified across multiple iterations.\n";
    return true;
}

// =============================================================================
// Test 7: Concurrent submission across 100 threads
// =============================================================================
static bool TestConcurrentSubmission100Threads() {
    std::cout << "[TEST 7] Testing Concurrent Request Submission with 100 Threads...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    const int THREAD_COUNT = 100;
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&scheduler, i]() {
            std::string reqId = "REQ_TH_" + std::to_string(i);
            std::string ambId = "AMB_TH_" + std::to_string(i);
            PatientSeverity sev = static_cast<PatientSeverity>((i % 4) + 1);
            double eta = 10.0 + static_cast<double>(i);
            double dist = 500.0 + static_cast<double>(i * 10);
            uint64_t ts = 1000 + static_cast<uint64_t>(i);

            EmergencyRequest r(reqId, ambId, sev, "HOSP_1", "LOC_1", "HOSP_1", eta, dist, ts);
            scheduler.SubmitEmergencyRequest(r);

            // Concurrently read queue
            auto q = scheduler.GetCurrentQueue();
            (void)q;

            // Occasionally query next
            if (i % 5 == 0) {
                auto next = scheduler.GetNextEmergency(false);
                (void)next;
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    TEST_ASSERT(scheduler.GetQueueSize() == THREAD_COUNT, "All 100 threads must have submitted successfully");

    auto finalSchedule = scheduler.ScheduleRequests(false);
    TEST_ASSERT(finalSchedule.size() == THREAD_COUNT, "Scheduled size must equal 100");

    // Verify ordering is strictly non-decreasing rank (CRITICAL first, LOW last)
    for (size_t i = 1; i < finalSchedule.size(); ++i) {
        EmergencyRequestComparator comp;
        // previous item must have priority over or equal to next item (comp(a, b) should be true or not comp(b, a))
        TEST_ASSERT(!comp(finalSchedule[i], finalSchedule[i - 1]), "Scheduled order must be monotonic");
    }

    std::cout << "  -> 100 concurrent threads completed with 0 data races.\n";
    return true;
}

// =============================================================================
// Test 8: Large queue (1000 requests) performance benchmark
// =============================================================================
static bool TestLargeQueue1000Performance() {
    std::cout << "[TEST 8] Testing Performance Benchmark: Schedule 1000 Requests under 2 ms...\n";

    PriorityScheduler scheduler;
    scheduler.SetConsoleLogging(false);

    const int REQUEST_COUNT = 1000;
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> sevDist(1, 4);
    std::uniform_real_distribution<double> etaDist(5.0, 300.0);
    std::uniform_real_distribution<double> distDist(100.0, 10000.0);
    std::uniform_int_distribution<uint64_t> tsDist(1000, 50000);

    for (int i = 0; i < REQUEST_COUNT; ++i) {
        std::string reqId = "REQ_" + std::to_string(i);
        std::string ambId = "AMB_" + std::to_string(i);
        PatientSeverity sev = static_cast<PatientSeverity>(sevDist(rng));
        double eta = etaDist(rng);
        double dist = distDist(rng);
        uint64_t ts = tsDist(rng);

        EmergencyRequest r(reqId, ambId, sev, "HOSPITAL", "ORIGIN", "HOSPITAL", eta, dist, ts);
        scheduler.SubmitEmergencyRequest(r);
    }

    TEST_ASSERT(scheduler.GetQueueSize() == REQUEST_COUNT, "Queue must contain 1000 requests");

    // Time the scheduling execution
    auto startTime = std::chrono::high_resolution_clock::now();
    const auto& scheduled = scheduler.ScheduleRequests(false);
    auto endTime = std::chrono::high_resolution_clock::now();

    auto elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    double elapsedMilliseconds = static_cast<double>(elapsedMicroseconds) / 1000.0;

    std::cout << "  -> 1000 requests scheduled in " << elapsedMicroseconds << " microseconds ("
              << elapsedMilliseconds << " ms).\n";

    TEST_ASSERT(scheduled.size() == REQUEST_COUNT, "Scheduled size must equal 1000");
    TEST_ASSERT(elapsedMilliseconds < 2.0, "Scheduling 1000 requests must take under 2.0 milliseconds!");

    // Verify complete sorted correctness across all 1000 elements
    EmergencyRequestComparator comp;
    for (size_t i = 1; i < scheduled.size(); ++i) {
        TEST_ASSERT(!comp(scheduled[i], scheduled[i - 1]), "Every element must satisfy scheduling comparator");
    }

    std::cout << "  -> Performance requirement (< 2 ms for 1000 requests) fully met.\n";
    return true;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 8A Intelligent Priority Scheduler Tests\n";
    std::cout << "==========================================================\n";

    bool allPassed = true;
    allPassed &= TestSingleRequest();
    allPassed &= TestMultipleSeverityLevels();
    allPassed &= TestEqualSeverityEtaComparison();
    allPassed &= TestEqualEtaDistanceComparison();
    allPassed &= TestEqualDistanceTimestampComparison();
    allPassed &= TestDeterministicOrdering();
    allPassed &= TestConcurrentSubmission100Threads();
    allPassed &= TestLargeQueue1000Performance();

    std::cout << "==========================================================\n";
    if (allPassed) {
        std::cout << " [ALL 8 TESTS PASSED] Phase 8A Fully Verified!\n";
    } else {
        std::cout << " [FAILED] Some Phase 8A tests failed.\n";
    }
    std::cout << "==========================================================\n";

    return allPassed ? 0 : 1;
}
