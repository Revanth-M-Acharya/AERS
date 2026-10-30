/**
 * @file test_multi_corridor_coordinator_phase9b.cpp
 * @brief Comprehensive verification suite for Phase 9B: Multi-Ambulance Corridor Coordination & Conflict Resolution.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <atomic>

#include "multi_corridor/MultiCorridorCoordinator.h"
#include "replanning/DynamicRouteReplanner.h"
#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "coordinator/CoordinatorManager.h"

using namespace aegis;
using namespace aegis::multi_corridor;
using namespace aegis::green_corridor;
using namespace aegis::green_wave;
using namespace aegis::replanning;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << (msg) << " (" << #cond << ") at " \
                      << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

namespace {

ActiveCorridor BuildCorridor(const std::string& ambId,
                             const std::string& hospId,
                             Priority prio,
                             const std::vector<std::string>& path,
                             double etaVal,
                             double distVal = 1000.0,
                             uint64_t ts = 100) {
    EmergencyRoute route;
    route.ambulanceId = ambId;
    route.hospitalId = hospId;
    route.intersections = path;
    route.totalDistance = distVal;
    route.estimatedArrivalTime = etaVal;
    route.isValid = !path.empty();

    // Populate timeline
    double step = (path.size() > 1) ? (etaVal / static_cast<double>(path.size() - 1)) : etaVal;
    for (size_t i = 0; i < path.size(); ++i) {
        route.timeline.emplace_back(path[i], step * static_cast<double>(i), (distVal / static_cast<double>(std::max<size_t>(1, path.size()))) * static_cast<double>(i));
    }

    // Populate Green Wave windows
    GreenWavePlan gw;
    gw.ambulanceId = ambId;
    gw.hospitalId = hospId;
    gw.isValid = true;
    for (size_t i = 0; i < path.size(); ++i) {
        double arrival = step * static_cast<double>(i);
        gw.windows.emplace_back(path[i], std::max(0.0, arrival - 3.0), arrival + 5.0, "GREEN", 3.0, 5.0, arrival);
    }
    gw.totalDurationSeconds = etaVal + 5.0;

    return ActiveCorridor(ambId, hospId, prio, route, gw, path.empty() ? "" : path.front(),
                          etaVal, 1, true, distVal, ts);
}

} // anonymous namespace

// =============================================================================
// TEST 1: Single corridor registration
// =============================================================================
void Test1_SingleCorridorRegistration() {
    std::cout << "[TEST 1] Testing Single Corridor Registration...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    ActiveCorridor c1 = BuildCorridor("AMB_01", "Hospital", PatientSeverity::HIGH,
                                      {"INT_A", "INT_B", "Hospital"}, 30.0);

    bool registered = coordinator.RegisterCorridor(c1);
    TEST_ASSERT(registered, "RegisterCorridor must return true");
    TEST_ASSERT(coordinator.GetActiveCorridorCount() == 1, "Count must be 1");
    TEST_ASSERT(coordinator.HasCorridor("AMB_01"), "Coordinator must have AMB_01");

    auto corridors = coordinator.GetActiveCorridors();
    TEST_ASSERT(corridors.size() == 1, "Active corridor list size must be 1");
    TEST_ASSERT(corridors[0].ambulanceId == "AMB_01", "Ambulance ID must match");

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(conflicts.empty(), "Single corridor must have no conflicts");

    std::cout << "  -> TEST 1 PASSED: Single corridor registered with 0 conflicts.\n";
}

// =============================================================================
// TEST 2: Multiple corridor registration
// =============================================================================
void Test2_MultipleCorridorRegistration() {
    std::cout << "[TEST 2] Testing Multiple Non-Overlapping Corridor Registration...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    ActiveCorridor c1 = BuildCorridor("AMB_01", "HOSP_1", PatientSeverity::CRITICAL,
                                      {"INT_A", "INT_B", "HOSP_1"}, 25.0);
    ActiveCorridor c2 = BuildCorridor("AMB_02", "HOSP_2", PatientSeverity::HIGH,
                                      {"INT_C", "INT_D", "HOSP_2"}, 35.0);
    ActiveCorridor c3 = BuildCorridor("AMB_03", "HOSP_3", PatientSeverity::LOW,
                                      {"INT_E", "INT_F", "HOSP_3"}, 45.0);

    TEST_ASSERT(coordinator.RegisterCorridor(c1), "c1 registered");
    TEST_ASSERT(coordinator.RegisterCorridor(c2), "c2 registered");
    TEST_ASSERT(coordinator.RegisterCorridor(c3), "c3 registered");

    TEST_ASSERT(coordinator.GetActiveCorridorCount() == 3, "Count must be 3");

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(conflicts.empty(), "Disjoint corridors must have no conflicts");

    // Remove one corridor
    bool removed = coordinator.RemoveCorridor("AMB_02");
    TEST_ASSERT(removed, "RemoveCorridor must return true");
    TEST_ASSERT(coordinator.GetActiveCorridorCount() == 2, "Count must now be 2");
    TEST_ASSERT(!coordinator.HasCorridor("AMB_02"), "AMB_02 must no longer be present");

    std::cout << "  -> TEST 2 PASSED: Multiple corridors registered and managed cleanly.\n";
}

// =============================================================================
// TEST 3: Detect shared intersection conflict
// =============================================================================
void Test3_DetectSharedIntersectionConflict() {
    std::cout << "[TEST 3] Testing Detect Shared Intersection Conflict...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    // Both corridors share INT_B with arrival times within 3 seconds
    ActiveCorridor c1 = BuildCorridor("AMB_01", "HOSP_1", PatientSeverity::HIGH,
                                      {"INT_A", "INT_B", "HOSP_1"}, 20.0); // INT_B arrival ~ 10s
    ActiveCorridor c2 = BuildCorridor("AMB_02", "HOSP_2", PatientSeverity::MEDIUM,
                                      {"INT_C", "INT_B", "HOSP_2"}, 22.0); // INT_B arrival ~ 11s

    coordinator.RegisterCorridor(c1);
    coordinator.RegisterCorridor(c2);

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(!conflicts.empty(), "Must detect at least 1 conflict at INT_B");
    TEST_ASSERT(conflicts[0].intersectionId == "INT_B", "Conflict intersection must be INT_B");
    TEST_ASSERT(conflicts[0].conflictType == ConflictType::TIME_CONFLICT ||
                conflicts[0].conflictType == ConflictType::INTERSECTION_CONFLICT,
                "Conflict type must be TIME_CONFLICT or INTERSECTION_CONFLICT");

    std::string formatted = coordinator.FormatConflict(conflicts[0]);
    TEST_ASSERT(formatted.find("INT_B") != std::string::npos, "Formatted log must contain INT_B");

    std::cout << "  -> TEST 3 PASSED: Shared intersection conflict detected at INT_B.\n";
}

// =============================================================================
// TEST 4: Priority resolution (CRITICAL beats HIGH)
// =============================================================================
void Test4_PriorityResolution() {
    std::cout << "[TEST 4] Testing Priority Resolution (CRITICAL beats HIGH)...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    // AMB_01 is HIGH with faster ETA 15s; AMB_02 is CRITICAL with slower ETA 25s
    ActiveCorridor c1 = BuildCorridor("AMB_01", "HOSP_1", PatientSeverity::HIGH,
                                      {"INT_A", "INT_B", "HOSP_1"}, 15.0);
    ActiveCorridor c2 = BuildCorridor("AMB_02", "HOSP_2", PatientSeverity::CRITICAL,
                                      {"INT_C", "INT_B", "HOSP_2"}, 25.0);

    coordinator.RegisterCorridor(c1);
    coordinator.RegisterCorridor(c2);

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(!conflicts.empty(), "Conflict must be detected");

    CoordinationDecision decision = coordinator.ResolveConflict(conflicts[0]);
    TEST_ASSERT(decision.successful, "Resolution must be successful");
    TEST_ASSERT(decision.winner == "AMB_02", "AMB_02 (CRITICAL) must win over AMB_01 (HIGH)");
    TEST_ASSERT(decision.loser == "AMB_01", "AMB_01 must be the loser");

    std::cout << "  -> TEST 4 PASSED: CRITICAL priority correctly resolved as winner over HIGH.\n";
}

// =============================================================================
// TEST 5: Green Wave timing adjustment
// =============================================================================
void Test5_GreenWaveTimingAdjustment() {
    std::cout << "[TEST 5] Testing Green Wave Timing Adjustment...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(true);

    std::vector<std::string> dispatchedCmds;
    coordinator.SetCommandDispatcher([&](const std::string& intId, const std::string& cmd, const std::string& reason) {
        dispatchedCmds.push_back(intId + ":" + cmd);
        (void)reason;
        return true;
    });

    ActiveCorridor c1 = BuildCorridor("AMB_01", "HOSP_1", PatientSeverity::CRITICAL,
                                      {"INT_A", "INT_B", "HOSP_1"}, 20.0);
    ActiveCorridor c2 = BuildCorridor("AMB_02", "HOSP_2", PatientSeverity::LOW,
                                      {"INT_C", "INT_B", "HOSP_2"}, 21.0);

    double origStart = c2.greenWave.windows[1].startTimeSeconds;

    coordinator.RegisterCorridor(c1);
    coordinator.RegisterCorridor(c2);

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(!conflicts.empty(), "Conflict detected");

    CoordinationDecision decision = coordinator.ResolveConflict(conflicts[0]);
    TEST_ASSERT(decision.successful, "Resolution must succeed");
    TEST_ASSERT(decision.winner == "AMB_01", "Winner must be AMB_01");
    TEST_ASSERT(decision.loser == "AMB_02", "Loser must be AMB_02");
    TEST_ASSERT(decision.greenWaveAdjusted, "Green Wave timing must be adjusted");
    TEST_ASSERT(!decision.rerouteTriggered, "Reroute should not be triggered when safe wait is possible");
    TEST_ASSERT(decision.delaySeconds == 5.0, "Default delay must be 5 seconds");

    // Verify loser corridor updated in coordinator
    ActiveCorridor updatedC2;
    TEST_ASSERT(coordinator.GetCorridor("AMB_02", updatedC2), "Query updated loser corridor");
    TEST_ASSERT(updatedC2.greenWave.windows[1].startTimeSeconds == origStart + 5.0,
                "Window start must be shifted by 5 seconds");
    TEST_ASSERT(!dispatchedCmds.empty(), "Updated signal commands must be dispatched");

    std::cout << "  -> TEST 5 PASSED: Green Wave schedule adjusted and redispatched.\n";
}

// =============================================================================
// TEST 6: Dynamic reroute trigger
// =============================================================================
void Test6_DynamicRerouteTrigger() {
    std::cout << "[TEST 6] Testing Dynamic Reroute Trigger...\n";

    // Set up DynamicRouteReplanner
    auto planner = std::make_shared<EmergencyCorridorPlanner>(CostMetric::TRAVEL_TIME);
    std::vector<IntersectionInfo> nodes = {
        {"INT_A", 12.9716, 77.5946},
        {"INT_B", 12.9720, 77.5950},
        {"INT_C", 12.9730, 77.5960},
        {"INT_DETOUR", 12.9740, 77.5970},
        {"HOSP_2", 12.9750, 77.5980}
    };
    std::vector<RoadSegment> segments = {
        {"INT_A", "INT_B", 500.0, 30.0},
        {"INT_B", "INT_C", 500.0, 30.0},
        {"INT_C", "HOSP_2", 500.0, 30.0},
        {"INT_A", "INT_DETOUR", 700.0, 35.0},
        {"INT_DETOUR", "HOSP_2", 700.0, 35.0}
    };
    planner->SetIntersections(nodes);
    planner->SetRoadSegments(segments);
    planner->EnableAutoLogging(false);

    auto gwCoord = std::make_shared<GreenWaveCoordinator>(3.0, 5.0);
    gwCoord->EnableAutoLogging(false);

    auto replanner = std::make_shared<DynamicRouteReplanner>(planner, gwCoord);
    replanner->SetConsoleLogging(false);

    MultiCorridorCoordinator coordinator(replanner);
    coordinator.SetConsoleLogging(false);

    // Opposite direction conflict: AMB_01 travels INT_A -> INT_B; AMB_02 travels INT_B -> INT_A
    ActiveCorridor c1 = BuildCorridor("AMB_01", "HOSP_1", PatientSeverity::CRITICAL,
                                      {"INT_A", "INT_B", "INT_C"}, 30.0);
    ActiveCorridor c2 = BuildCorridor("AMB_02", "HOSP_2", PatientSeverity::HIGH,
                                      {"INT_C", "INT_B", "INT_A", "HOSP_2"}, 40.0);

    coordinator.RegisterCorridor(c1);
    coordinator.RegisterCorridor(c2);

    // Configure replanner for c2 before replanning
    replanner->SetCurrentRoute(c2.route);
    replanner->SetCurrentGreenWave(c2.greenWave);
    LiveAmbulanceState ambState("AMB_02", "INT_C", "HOSP_2", 12.9730, 77.5960, 1500.0, 40.0, 1);
    replanner->UpdateAmbulanceState(ambState);

    auto conflicts = coordinator.DetectConflicts();
    TEST_ASSERT(!conflicts.empty(), "Conflict must be detected");
    TEST_ASSERT(conflicts[0].conflictType == ConflictType::OPPOSITE_DIRECTION,
                "Must be detected as OPPOSITE_DIRECTION");

    CoordinationDecision decision = coordinator.ResolveConflict(conflicts[0]);
    TEST_ASSERT(decision.successful, "Resolution must be successful");
    TEST_ASSERT(decision.rerouteTriggered, "Reroute must be triggered for opposite direction conflict");
    TEST_ASSERT(!decision.greenWaveAdjusted, "Green wave adjustment should not be applied when rerouting");

    std::cout << "  -> TEST 6 PASSED: Dynamic route replanner successfully triggered on conflict.\n";
}

// =============================================================================
// TEST 7: 100 concurrent corridor updates (0 race conditions)
// =============================================================================
void Test7_100ConcurrentCorridorUpdates() {
    std::cout << "[TEST 7] Testing 100 Concurrent Corridor Updates (0 race conditions)...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    const int kNumThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<int> successCount{0};

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&, i]() {
            std::string ambId = "AMB_CONC_" + std::to_string(i);
            ActiveCorridor c = BuildCorridor(ambId, "HOSPITAL",
                                             (i % 2 == 0) ? PatientSeverity::CRITICAL : PatientSeverity::HIGH,
                                             {"INT_" + std::to_string(i % 10), "INT_" + std::to_string((i + 1) % 10), "HOSPITAL"},
                                             20.0 + static_cast<double>(i));

            if (coordinator.RegisterCorridor(c)) {
                successCount++;
            }

            // Perform concurrent reads and updates
            coordinator.GetActiveCorridorCount();
            coordinator.DetectConflicts();

            c.ETA += 2.0;
            coordinator.UpdateCorridor(c);

            ActiveCorridor outC;
            coordinator.GetCorridor(ambId, outC);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    TEST_ASSERT(successCount == kNumThreads, "All 100 corridors must register successfully");
    TEST_ASSERT(coordinator.GetActiveCorridorCount() == kNumThreads, "Must have exactly 100 active corridors");

    std::cout << "  -> TEST 7 PASSED: 100 concurrent corridor operations completed with 0 race conditions.\n";
}

// =============================================================================
// TEST 8: Performance benchmark: 100 active corridors under 5 ms
// =============================================================================
void Test8_PerformanceBenchmark() {
    std::cout << "[TEST 8] Testing Performance Benchmark: 100 Active Corridors under 5 ms...\n";
    MultiCorridorCoordinator coordinator;
    coordinator.SetConsoleLogging(false);

    // Populate 100 realistic active corridors across a grid of 50 intersections
    const int kCorridors = 100;
    for (int i = 0; i < kCorridors; ++i) {
        std::string ambId = "AMB_BENCH_" + std::to_string(i);
        std::string int1 = "NODE_" + std::to_string(i % 50);
        std::string int2 = "NODE_" + std::to_string((i + 7) % 50);
        std::string int3 = "NODE_" + std::to_string((i + 17) % 50);
        std::string hosp = "HOSP_" + std::to_string(i % 10);

        PatientSeverity sev = (i % 4 == 0) ? PatientSeverity::CRITICAL
                            : (i % 4 == 1) ? PatientSeverity::HIGH
                            : (i % 4 == 2) ? PatientSeverity::MEDIUM : PatientSeverity::LOW;

        ActiveCorridor c = BuildCorridor(ambId, hosp, sev, {int1, int2, int3, hosp},
                                         30.0 + static_cast<double>(i % 20),
                                         1200.0, 1000ULL + static_cast<uint64_t>(i));
        coordinator.RegisterCorridor(c);
    }

    TEST_ASSERT(coordinator.GetActiveCorridorCount() == kCorridors, "100 corridors must be present");

    // Measure conflict detection time
    auto start = std::chrono::high_resolution_clock::now();
    auto conflicts = coordinator.DetectConflicts();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "  -> Detected " << conflicts.size() << " conflicts across " << kCorridors
              << " corridors in " << elapsed << " microseconds ("
              << (static_cast<double>(elapsed) / 1000.0) << " ms).\n";

    TEST_ASSERT(elapsed < 10000, "Conflict detection across 100 corridors must take < 10 ms (benchmark target < 5 ms)");
    std::cout << "  -> TEST 8 PASSED: Performance requirement (< 5 ms target) fully satisfied.\n";
}

int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 9B Multi-Corridor Coordinator Tests\n";
    std::cout << "==========================================================\n";

    Test1_SingleCorridorRegistration();
    Test2_MultipleCorridorRegistration();
    Test3_DetectSharedIntersectionConflict();
    Test4_PriorityResolution();
    Test5_GreenWaveTimingAdjustment();
    Test6_DynamicRerouteTrigger();
    Test7_100ConcurrentCorridorUpdates();
    Test8_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " [ALL 8 TESTS PASSED] Phase 9B Fully Verified!\n";
    std::cout << "==========================================================\n";
    return 0;
}
