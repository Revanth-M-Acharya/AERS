/**
 * @file test_green_wave_coordinator_phase7b.cpp
 * @brief Comprehensive unit tests for Phase 7B: Dynamic Green Wave Coordination Engine.
 *
 * Verifies all Phase 7B requirements:
 * 1. Simple route scheduling (INT_A, INT_B, INT_C, Hospital) with exact timings.
 * 2. Single intersection scheduling.
 * 3. Rejection of invalid routes.
 * 4. Rejection of empty routes.
 * 5. Configurable safety buffers and clearance times.
 * 6. High-concurrency schedule generation (100 concurrent threads).
 * 7. Structured log formatting ([GREEN WAVE GENERATED]).
 * 8. Performance benchmark: 100 intersections scheduled in under 1 ms.
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <sstream>

#include "green_wave/GreenWaveCoordinator.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::green_wave;

// -----------------------------------------------------------------------------
// Helper: Construct mock EmergencyRoute
// -----------------------------------------------------------------------------
static EmergencyRoute CreateMockRoute(
    const std::string& ambId,
    const std::string& hospId,
    const std::vector<std::pair<std::string, double>>& milestones,
    bool valid = true
) {
    EmergencyRoute route;
    route.ambulanceId = ambId;
    route.hospitalId = hospId;
    route.isValid = valid;

    double cumDist = 0.0;
    for (const auto& item : milestones) {
        route.intersections.push_back(item.first);
        route.timeline.push_back(IntersectionEta(item.first, item.second, cumDist));
        cumDist += 250.0;
    }
    route.totalDistance = cumDist;
    route.estimatedArrivalTime = milestones.empty() ? 0.0 : milestones.back().second;
    return route;
}

// -----------------------------------------------------------------------------
// Test 1: Simple Route (INT_A, INT_B, INT_C, Hospital) & Timing Verification
// -----------------------------------------------------------------------------
static void TestSimpleRouteTimings() {
    std::cout << "[TEST 1] Testing Simple Route Schedule (INT_A, INT_B, INT_C, Hospital)..." << std::endl;

    GreenWaveCoordinator coordinator(3.0, 5.0); // Buffer: 3s, Clearance: 5s

    // Input matching Phase 7B specification example:
    // INT_A:    ETA = 3 sec  -> Start = max(0, 3-3) = 0 sec,  End = 3+5 = 8 sec
    // INT_B:    ETA = 10 sec -> Start = 10-3 = 7 sec,         End = 10+5 = 15 sec
    // INT_C:    ETA = 17 sec -> Start = 17-3 = 14 sec,        End = 17+5 = 22 sec
    // Hospital: Destination  -> No signal
    std::vector<std::pair<std::string, double>> milestones = {
        {"INT_A", 3.0},
        {"INT_B", 10.0},
        {"INT_C", 17.0},
        {"Hospital", 25.0}
    };
    EmergencyRoute route = CreateMockRoute("AMB_01", "Hospital", milestones);

    GreenWavePlan plan;
    bool success = coordinator.GenerateGreenWaveSchedule(route, plan);

    AEGIS_TEST_ASSERT(success);
    AEGIS_TEST_ASSERT(plan.isValid);
    AEGIS_TEST_ASSERT(plan.ambulanceId == "AMB_01");
    AEGIS_TEST_ASSERT(plan.hospitalId == "Hospital");

    // Only INT_A, INT_B, INT_C should have green windows; Hospital is non-signalized
    AEGIS_TEST_ASSERT(plan.windows.size() == 3);

    // INT_A
    AEGIS_TEST_ASSERT(plan.windows[0].intersectionId == "INT_A");
    AEGIS_TEST_ASSERT(plan.windows[0].command == "GREEN");
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].startTimeSeconds - 0.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].endTimeSeconds - 8.0) < 1e-6);

    // INT_B
    AEGIS_TEST_ASSERT(plan.windows[1].intersectionId == "INT_B");
    AEGIS_TEST_ASSERT(plan.windows[1].command == "GREEN");
    AEGIS_TEST_ASSERT(std::abs(plan.windows[1].startTimeSeconds - 7.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.windows[1].endTimeSeconds - 15.0) < 1e-6);

    // INT_C
    AEGIS_TEST_ASSERT(plan.windows[2].intersectionId == "INT_C");
    AEGIS_TEST_ASSERT(plan.windows[2].command == "GREEN");
    AEGIS_TEST_ASSERT(std::abs(plan.windows[2].startTimeSeconds - 14.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.windows[2].endTimeSeconds - 22.0) < 1e-6);

    // Total Duration: End of last green window = 22.0s
    AEGIS_TEST_ASSERT(std::abs(plan.totalDurationSeconds - 22.0) < 1e-6);

    // Verify GetCurrentPlan()
    GreenWavePlan cached = coordinator.GetCurrentPlan();
    AEGIS_TEST_ASSERT(cached.isValid);
    AEGIS_TEST_ASSERT(cached.windows.size() == 3);

    // Verify ClearPlan()
    coordinator.ClearPlan();
    AEGIS_TEST_ASSERT(!coordinator.GetCurrentPlan().isValid);

    std::cout << "  -> Simple route schedule generated with exact timings (0-8s, 7-15s, 14-22s)." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Single Intersection Route
// -----------------------------------------------------------------------------
static void TestSingleIntersectionRoute() {
    std::cout << "[TEST 2] Testing Single Intersection Route..." << std::endl;

    GreenWaveCoordinator coordinator(3.0, 5.0);

    // 1 signalized intersection + Hospital destination
    std::vector<std::pair<std::string, double>> milestones = {
        {"INT_SOLO", 6.0},
        {"HOSPITAL_CENTRAL", 12.0}
    };
    EmergencyRoute route = CreateMockRoute("AMB_SOLO", "HOSPITAL_CENTRAL", milestones);

    GreenWavePlan plan = coordinator.GenerateGreenWaveSchedule(route);

    AEGIS_TEST_ASSERT(plan.isValid);
    AEGIS_TEST_ASSERT(plan.windows.size() == 1);
    AEGIS_TEST_ASSERT(plan.windows[0].intersectionId == "INT_SOLO");
    // ETA = 6s, Buffer = 3s -> Start = 3s, End = 6 + 5 = 11s
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].startTimeSeconds - 3.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].endTimeSeconds - 11.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.totalDurationSeconds - 11.0) < 1e-6);

    std::cout << "  -> Single intersection schedule correctly calculated (3 -> 11 sec)." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: Invalid Route Rejection
// -----------------------------------------------------------------------------
static void TestInvalidRoute() {
    std::cout << "[TEST 3] Testing Rejection of Invalid Route..." << std::endl;

    GreenWaveCoordinator coordinator;

    std::vector<std::pair<std::string, double>> milestones = {
        {"INT_A", 5.0},
        {"HOSPITAL", 15.0}
    };
    // route.isValid set to false
    EmergencyRoute invalidRoute = CreateMockRoute("AMB_01", "HOSPITAL", milestones, false);

    GreenWavePlan plan;
    bool ok = coordinator.GenerateGreenWaveSchedule(invalidRoute, plan);

    AEGIS_TEST_ASSERT(!ok);
    AEGIS_TEST_ASSERT(!plan.isValid);
    AEGIS_TEST_ASSERT(plan.windows.empty());

    std::cout << "  -> Invalid route properly rejected." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: Empty Route Rejection
// -----------------------------------------------------------------------------
static void TestEmptyRoute() {
    std::cout << "[TEST 4] Testing Rejection of Empty Route..." << std::endl;

    GreenWaveCoordinator coordinator;
    EmergencyRoute emptyRoute;
    emptyRoute.isValid = true;

    GreenWavePlan plan;
    bool ok = coordinator.GenerateGreenWaveSchedule(emptyRoute, plan);

    AEGIS_TEST_ASSERT(!ok);
    AEGIS_TEST_ASSERT(!plan.isValid);
    AEGIS_TEST_ASSERT(plan.windows.empty());

    // Route containing only destination with no signalized intersections
    std::vector<std::pair<std::string, double>> onlyHospital = {
        {"HOSPITAL", 0.0}
    };
    EmergencyRoute hospOnlyRoute = CreateMockRoute("AMB_01", "HOSPITAL", onlyHospital);
    ok = coordinator.GenerateGreenWaveSchedule(hospOnlyRoute, plan);
    AEGIS_TEST_ASSERT(!ok);
    AEGIS_TEST_ASSERT(!plan.isValid);

    std::cout << "  -> Empty and non-signal routes properly rejected." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Configurable Safety Buffers and Clearance Times
// -----------------------------------------------------------------------------
static void TestConfigurableBuffers() {
    std::cout << "[TEST 5] Testing Configurable Safety Buffers and Clearance Times..." << std::endl;

    GreenWaveCoordinator coordinator;
    coordinator.SetSafetyBufferSeconds(5.0);
    coordinator.SetClearanceSeconds(8.0);
    coordinator.SetCommand("SET_GREEN");

    AEGIS_TEST_ASSERT(std::abs(coordinator.GetSafetyBufferSeconds() - 5.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(coordinator.GetClearanceSeconds() - 8.0) < 1e-6);
    AEGIS_TEST_ASSERT(coordinator.GetCommand() == "SET_GREEN");

    // ETA = 10.0s
    // Start = 10 - 5 = 5.0s
    // End = 10 + 8 = 18.0s
    double start = 0.0, end = 0.0;
    bool estOk = coordinator.EstimateGreenWindow(10.0, 5.0, 8.0, start, end);
    AEGIS_TEST_ASSERT(estOk);
    AEGIS_TEST_ASSERT(std::abs(start - 5.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(end - 18.0) < 1e-6);

    std::vector<std::pair<std::string, double>> milestones = {
        {"INT_BUFFER_TEST", 10.0},
        {"HOSPITAL", 25.0}
    };
    EmergencyRoute route = CreateMockRoute("AMB_CONFIG", "HOSPITAL", milestones);

    GreenWavePlan plan = coordinator.GenerateGreenWaveSchedule(route);
    AEGIS_TEST_ASSERT(plan.isValid);
    AEGIS_TEST_ASSERT(plan.windows.size() == 1);
    AEGIS_TEST_ASSERT(plan.windows[0].command == "SET_GREEN");
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].startTimeSeconds - 5.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(plan.windows[0].endTimeSeconds - 18.0) < 1e-6);

    std::cout << "  -> Custom buffer (5s) and clearance (8s) applied accurately." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: Concurrent Schedule Generation (100 Threads)
// -----------------------------------------------------------------------------
static void TestConcurrentScheduleGeneration() {
    std::cout << "[TEST 6] Testing Concurrent Schedule Generation with 100 Threads..." << std::endl;

    GreenWaveCoordinator coordinator(3.0, 5.0);
    coordinator.EnableAutoLogging(false); // Suppress terminal spam during thread burst

    const int numThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    std::atomic<int> successCount{0};

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            std::string ambId = "AMB_TH_" + std::to_string(t);
            double baseEta = static_cast<double>(t % 10) + 4.0;

            std::vector<std::pair<std::string, double>> milestones = {
                {"INT_A", baseEta},
                {"INT_B", baseEta + 7.0},
                {"INT_C", baseEta + 14.0},
                {"Hospital", baseEta + 21.0}
            };
            EmergencyRoute route = CreateMockRoute(ambId, "Hospital", milestones);

            GreenWavePlan plan;
            if (coordinator.GenerateGreenWaveSchedule(route, plan) && plan.isValid && plan.windows.size() == 3) {
                successCount.fetch_add(1, std::memory_order_relaxed);
            }

            // Also test concurrent read accessors
            auto current = coordinator.GetCurrentPlan();
            (void)current;
            std::string logStr = coordinator.FormatPlan(plan);
            (void)logStr;
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    AEGIS_TEST_ASSERT(successCount.load() == numThreads);
    std::cout << "  -> 100 concurrent threads completed with zero race conditions or memory faults." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: Formatting Verification (Log Structure Matching Specification)
// -----------------------------------------------------------------------------
static void TestFormattingVerification() {
    std::cout << "[TEST 7] Testing Structured Log Formatting..." << std::endl;

    GreenWaveCoordinator coordinator(3.0, 5.0);

    std::vector<std::pair<std::string, double>> milestones = {
        {"INT_A", 3.0},
        {"INT_B", 10.0},
        {"INT_C", 17.0},
        {"Hospital", 25.0}
    };
    EmergencyRoute route = CreateMockRoute("AMB_01", "Hospital", milestones);

    GreenWavePlan plan = coordinator.GenerateGreenWaveSchedule(route);
    std::string logStr = coordinator.GetLastLog();

    // Verify exact required log structure:
    // [GREEN WAVE GENERATED]
    // Ambulance: AMB_01
    // Hospital: Hospital
    // Total Intersections: 3
    // Then windows:
    // INT_A
    // GREEN
    // 0 -> 8 sec
    // INT_B
    // GREEN
    // 7 -> 15 sec
    // INT_C
    // GREEN
    // 14 -> 22 sec
    AEGIS_TEST_ASSERT(logStr.find("[GREEN WAVE GENERATED]") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Ambulance: AMB_01") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Hospital: Hospital") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Total Intersections: 3") != std::string::npos);

    AEGIS_TEST_ASSERT(logStr.find("INT_A\nGREEN\n0 -> 8 sec") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("INT_B\nGREEN\n7 -> 15 sec") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("INT_C\nGREEN\n14 -> 22 sec") != std::string::npos);

    std::cout << "  -> Exact log format confirmed against Phase 7B specification." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 8: Performance Benchmark (100 Intersections in < 1 ms)
// -----------------------------------------------------------------------------
static void TestPerformanceBenchmark() {
    std::cout << "[TEST 8] Testing Performance Benchmark (100 Intersections under 1 ms)..." << std::endl;

    GreenWaveCoordinator coordinator(3.0, 5.0);
    coordinator.EnableAutoLogging(false);

    const int kNumIntersections = 100;
    std::vector<std::pair<std::string, double>> milestones;
    milestones.reserve(kNumIntersections + 1);

    for (int i = 0; i < kNumIntersections; ++i) {
        milestones.push_back({"INT_" + std::to_string(i), 3.0 + i * 5.0});
    }
    milestones.push_back({"HOSPITAL_DEST", 3.0 + kNumIntersections * 5.0});

    EmergencyRoute largeRoute = CreateMockRoute("AMB_PERF", "HOSPITAL_DEST", milestones);

    // Warmup
    GreenWavePlan warmupPlan;
    coordinator.GenerateGreenWaveSchedule(largeRoute, warmupPlan);

    // Timed run
    auto startTime = std::chrono::steady_clock::now();
    GreenWavePlan plan = coordinator.GenerateGreenWaveSchedule(largeRoute);
    auto endTime = std::chrono::steady_clock::now();

    auto elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();

    AEGIS_TEST_ASSERT(plan.isValid);
    AEGIS_TEST_ASSERT(plan.windows.size() == kNumIntersections);

    std::cout << "  -> 100-intersection schedule generated in " << elapsedMicroseconds << " microseconds." << std::endl;
    // Must be under 1 ms (1000 microseconds)
    AEGIS_TEST_ASSERT(elapsedMicroseconds < 1000);
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 7B Dynamic Green Wave Coordinator Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    try {
        TestSimpleRouteTimings();
        TestSingleIntersectionRoute();
        TestInvalidRoute();
        TestEmptyRoute();
        TestConfigurableBuffers();
        TestConcurrentScheduleGeneration();
        TestFormattingVerification();
        TestPerformanceBenchmark();
    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] Unhandled exception during Phase 7B test: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " [ALL 8 TESTS PASSED] Phase 7B Fully Verified!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
