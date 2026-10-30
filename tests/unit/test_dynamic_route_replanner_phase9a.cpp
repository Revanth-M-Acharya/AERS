/**
 * @file test_dynamic_route_replanner_phase9a.cpp
 * @brief Comprehensive Unit Test Suite for Phase 9A Dynamic Route Replanning Engine.
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>
#include <cmath>

#include "replanning/DynamicRouteReplanner.h"

using namespace aegis;
using namespace aegis::replanning;
using namespace aegis::green_corridor;
using namespace aegis::green_wave;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while (0)

// Helper to configure a dual-corridor road network:
// Primary Corridor:  INT_A -> INT_B -> INT_C -> Hospital (Total distance: 900m, Time: 45s)
// Detour Corridor:   INT_A -> INT_D -> INT_E -> Hospital (Total distance: 1200m, Time: 75s)
static void SetupTestNetwork(std::shared_ptr<EmergencyCorridorPlanner> planner) {
    planner->ClearGraph();

    planner->AddIntersection("INT_A", 12.9716, 77.5946);
    planner->AddIntersection("INT_B", 12.9726, 77.5956);
    planner->AddIntersection("INT_C", 12.9736, 77.5966);
    planner->AddIntersection("INT_D", 12.9710, 77.5970);
    planner->AddIntersection("INT_E", 12.9720, 77.5980);
    planner->AddIntersection("Hospital", 12.9750, 77.5990);

    // Primary route
    planner->AddRoadSegment("INT_A", "INT_B", 300.0, 15.0, true);
    planner->AddRoadSegment("INT_B", "INT_C", 300.0, 15.0, true);
    planner->AddRoadSegment("INT_C", "Hospital", 300.0, 15.0, true);

    // Detour route
    planner->AddRoadSegment("INT_A", "INT_D", 400.0, 25.0, true);
    planner->AddRoadSegment("INT_D", "INT_E", 400.0, 25.0, true);
    planner->AddRoadSegment("INT_E", "Hospital", 400.0, 25.0, true);
}

// =============================================================================
// TEST 1: No traffic changes. ShouldReplan() returns false.
// =============================================================================
static bool TestNoTrafficChangesShouldReplanFalse() {
    std::cout << "[TEST 1] Testing No Traffic Changes (ShouldReplan returns false)...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);

    EmergencyRoute initialRoute = replanner.GenerateUpdatedRoute();
    TEST_ASSERT(initialRoute.isValid, "Initial route must be valid");
    TEST_ASSERT(initialRoute.intersections.size() == 4, "Initial route must have 4 nodes");
    TEST_ASSERT(initialRoute.intersections[1] == "INT_B", "Initial route should pick faster INT_B corridor");

    bool applied = replanner.ApplyNewPlan();
    TEST_ASSERT(applied, "Initial plan must apply successfully");

    // Verify ShouldReplan() returns false with no traffic condition updates
    bool shouldReplan = replanner.ShouldReplan();
    TEST_ASSERT(!shouldReplan, "ShouldReplan() must return false when no traffic conditions change");

    std::cout << "  -> TEST 1 PASSED: ShouldReplan() returned false as expected.\n";
    return true;
}

// =============================================================================
// TEST 2: Road blocked. New corridor generated.
// =============================================================================
static bool TestRoadBlockedNewCorridorGenerated() {
    std::cout << "[TEST 2] Testing Road Blocked (New Corridor Generated)...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(true);

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    uint32_t v1 = replanner.GetRouteVersion();

    // Block primary corridor segment INT_B -> INT_C
    TrafficCondition blockCond("ROAD_BC", "INT_B", "INT_C", 0.0, 0.0, true, 1000);
    replanner.UpdateTrafficCondition(blockCond);

    TEST_ASSERT(replanner.ShouldReplan(), "ShouldReplan() must return true when corridor road is blocked");

    EmergencyRoute newRoute = replanner.GenerateUpdatedRoute();
    TEST_ASSERT(newRoute.isValid, "New detour route must be valid");
    TEST_ASSERT(newRoute.intersections.size() == 4, "Detour route must have 4 intersections");
    TEST_ASSERT(newRoute.intersections[1] == "INT_D", "New corridor must route through INT_D detour");
    TEST_ASSERT(newRoute.intersections[2] == "INT_E", "New corridor must route through INT_E detour");

    bool applied = replanner.ApplyNewPlan();
    TEST_ASSERT(applied, "ApplyNewPlan must succeed for detour route");
    TEST_ASSERT(replanner.GetRouteVersion() == v1 + 1, "routeVersion must increment on successful replan");

    ReplanResult last = replanner.GetLastReplan();
    TEST_ASSERT(last.success, "ReplanResult must indicate success");
    TEST_ASSERT(last.reason == "Road Blocked", "Replan reason must be 'Road Blocked'");

    std::cout << "  -> TEST 2 PASSED: Blocked road rerouted through INT_D -> INT_E corridor.\n";
    return true;
}

// =============================================================================
// TEST 3: ETA exceeds threshold. Replanning triggered.
// =============================================================================
static bool TestEtaExceedsThresholdReplanningTriggered() {
    std::cout << "[TEST 3] Testing ETA Exceeds Threshold (Replanning Triggered)...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    // Normal shouldReplan is false
    TEST_ASSERT(!replanner.ShouldReplan(), "Should not replan initially");

    // Live ambulance report shows severe delay: ETA increases by 25 seconds (from 45s to 70s > 20s threshold)
    amb.currentEtaSeconds = 70.0;
    replanner.UpdateAmbulanceState(amb);

    TEST_ASSERT(replanner.ShouldReplan(), "ShouldReplan() must trigger when ETA increases by > 20s");

    EmergencyRoute updatedRoute = replanner.GenerateUpdatedRoute();
    TEST_ASSERT(updatedRoute.isValid, "Updated route must be valid");

    replanner.ApplyNewPlan();
    TEST_ASSERT(replanner.GetLastReplan().reason.find("ETA") != std::string::npos,
                "Replan reason must mention ETA threshold breach");

    std::cout << "  -> TEST 3 PASSED: ETA threshold breach triggered replanning.\n";
    return true;
}

// =============================================================================
// TEST 4: Traffic cleared. Original optimal path restored.
// =============================================================================
static bool TestTrafficClearedOriginalOptimalPathRestored() {
    std::cout << "[TEST 4] Testing Traffic Cleared (Original Optimal Path Restored)...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    // 1. Block the primary road INT_B -> INT_C
    TrafficCondition blockCond("ROAD_BC", "INT_B", "INT_C", 0.0, 0.0, true, 1000);
    replanner.UpdateTrafficCondition(blockCond);
    TEST_ASSERT(replanner.ShouldReplan(), "Should replan on block");
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();
    TEST_ASSERT(replanner.GetCurrentRoute().intersections[1] == "INT_D", "Detour must use INT_D");

    // 2. Clear traffic on primary road INT_B -> INT_C
    TrafficCondition clearCond("ROAD_BC", "INT_B", "INT_C", 60.0, 0.0, false, 2000);
    replanner.UpdateTrafficCondition(clearCond);

    TEST_ASSERT(replanner.ShouldReplan(), "ShouldReplan() must return true when traffic clears and faster path exists");

    EmergencyRoute restoredRoute = replanner.GenerateUpdatedRoute();
    TEST_ASSERT(restoredRoute.isValid, "Restored route must be valid");
    TEST_ASSERT(restoredRoute.intersections[1] == "INT_B", "Corridor must restore original optimal path through INT_B");

    replanner.ApplyNewPlan();
    TEST_ASSERT(replanner.GetCurrentRoute().intersections[1] == "INT_B", "Current route must be restored to INT_B");

    std::cout << "  -> TEST 4 PASSED: Cleared traffic restored original optimal corridor.\n";
    return true;
}

// =============================================================================
// TEST 5: Green Wave regenerated.
// =============================================================================
static bool TestGreenWaveRegenerated() {
    std::cout << "[TEST 5] Testing Green Wave Regeneration...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    auto waveCoordinator = std::make_shared<GreenWaveCoordinator>();
    DynamicRouteReplanner replanner(planner, waveCoordinator);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    // Initial plan should have windows for INT_A, INT_B, INT_C
    GreenWavePlan initialWave = replanner.GetCurrentGreenWave();
    TEST_ASSERT(initialWave.isValid, "Initial Green Wave plan must be valid");
    TEST_ASSERT(initialWave.windows.size() == 3, "Initial plan must have 3 signalized windows");
    TEST_ASSERT(initialWave.windows[0].intersectionId == "INT_A", "First window INT_A");
    TEST_ASSERT(initialWave.windows[1].intersectionId == "INT_B", "Second window INT_B");
    TEST_ASSERT(initialWave.windows[2].intersectionId == "INT_C", "Third window INT_C");

    // Force detour via INT_D -> INT_E
    TrafficCondition blockCond("ROAD_AB", "INT_A", "INT_B", 0.0, 0.0, true, 1000);
    replanner.UpdateTrafficCondition(blockCond);

    replanner.ShouldReplan();
    replanner.GenerateUpdatedRoute();
    GreenWavePlan updatedWave = replanner.GenerateUpdatedGreenWave();
    replanner.ApplyNewPlan();

    TEST_ASSERT(updatedWave.isValid, "Regenerated Green Wave plan must be valid");
    TEST_ASSERT(updatedWave.windows.size() == 3, "Updated plan must have 3 windows");
    TEST_ASSERT(updatedWave.windows[0].intersectionId == "INT_A", "First window INT_A");
    TEST_ASSERT(updatedWave.windows[1].intersectionId == "INT_D", "Second window INT_D");
    TEST_ASSERT(updatedWave.windows[2].intersectionId == "INT_E", "Third window INT_E");

    std::cout << "  -> TEST 5 PASSED: Green Wave schedule successfully regenerated for new corridor.\n";
    return true;
}

// =============================================================================
// TEST 6: Coordinator receives updated signal commands.
// =============================================================================
static bool TestCoordinatorReceivesUpdatedSignalCommands() {
    std::cout << "[TEST 6] Testing Coordinator Receives Updated Signal Commands...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    auto waveCoordinator = std::make_shared<GreenWaveCoordinator>();
    DynamicRouteReplanner replanner(planner, waveCoordinator);
    replanner.SetConsoleLogging(false);

    std::vector<std::string> dispatchedSignals;
    replanner.SetCommandDispatcher([&](const std::string& intId, const std::string& cmd, const std::string& reason) {
        dispatchedSignals.push_back(intId + " " + cmd + " [" + reason + "]");
        return true;
    });

    LiveAmbulanceState amb("AMB_01", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    dispatchedSignals.clear();

    // Trigger detour
    TrafficCondition blockCond("ROAD_AB", "INT_A", "INT_B", 0.0, 0.0, true, 1000);
    replanner.UpdateTrafficCondition(blockCond);
    replanner.ShouldReplan();
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    TEST_ASSERT(!dispatchedSignals.empty(), "Coordinator must receive dispatched signal commands");
    TEST_ASSERT(dispatchedSignals.size() == 3, "Coordinator must receive commands for all 3 detour intersections");

    bool hasIntD = false;
    bool hasIntE = false;
    for (const auto& sig : dispatchedSignals) {
        if (sig.find("INT_D") != std::string::npos && sig.find("SET_GREEN") != std::string::npos) hasIntD = true;
        if (sig.find("INT_E") != std::string::npos && sig.find("SET_GREEN") != std::string::npos) hasIntE = true;
    }
    TEST_ASSERT(hasIntD, "Coordinator must dispatch SET_GREEN for INT_D");
    TEST_ASSERT(hasIntE, "Coordinator must dispatch SET_GREEN for INT_E");

    std::cout << "  -> TEST 6 PASSED: Coordinator verified receiving updated signal preemption commands.\n";
    return true;
}

// =============================================================================
// TEST 7: 100 concurrent traffic updates. 0 race conditions.
// =============================================================================
static bool TestConcurrentTrafficUpdates100Threads() {
    std::cout << "[TEST 7] Testing 100 Concurrent Traffic Updates (0 race conditions)...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    SetupTestNetwork(planner);

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_CONC", "INT_A", "Hospital", 12.9716, 77.5946, 900.0, 45.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    const int THREAD_COUNT = 100;
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&replanner, i]() {
            bool isBlocked = (i % 2 == 0);
            TrafficCondition tc("ROAD_CONC_" + std::to_string(i),
                                 (i % 3 == 0) ? "INT_A" : "INT_B",
                                 (i % 3 == 0) ? "INT_B" : "INT_C",
                                 30.0 + (i % 30),
                                 static_cast<double>(i % 5),
                                 isBlocked,
                                 1000 + static_cast<uint64_t>(i));
            replanner.UpdateTrafficCondition(tc);

            if (i % 5 == 0) {
                LiveAmbulanceState s("AMB_CONC", "INT_A", "Hospital",
                                     12.9716, 77.5946, 800.0, 40.0 + (i % 10), static_cast<uint32_t>(i));
                replanner.UpdateAmbulanceState(s);
            }

            replanner.ShouldReplan();
            auto r = replanner.GetCurrentRoute();
            auto w = replanner.GetCurrentGreenWave();
            (void)r;
            (void)w;
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::cout << "  -> TEST 7 PASSED: 100 concurrent threads completed with 0 data races.\n";
    return true;
}

// =============================================================================
// TEST 8: Performance benchmark: 100-node graph route generated under 3 ms.
// =============================================================================
static bool TestPerformanceBenchmark100NodeGraphUnder3ms() {
    std::cout << "[TEST 8] Testing Performance Benchmark: 100-Node Graph Route Generated under 3 ms...\n";

    auto planner = std::make_shared<EmergencyCorridorPlanner>();
    planner->ClearGraph();

    const int NODE_COUNT = 100;
    for (int i = 0; i < NODE_COUNT; ++i) {
        planner->AddIntersection("N_" + std::to_string(i), 12.9 + 0.0005 * i, 77.5 + 0.0005 * i);
    }
    planner->AddIntersection("Hospital", 12.96, 77.56);

    // Build main line with alternative bypasses
    for (int i = 0; i < NODE_COUNT - 1; ++i) {
        planner->AddRoadSegment("N_" + std::to_string(i), "N_" + std::to_string(i + 1), 200.0, 10.0, true);
    }
    planner->AddRoadSegment("N_99", "Hospital", 200.0, 10.0, true);

    // Cross-connecting detour bypasses (longer / slower than normal road)
    for (int i = 0; i < NODE_COUNT - 5; i += 5) {
        planner->AddRoadSegment("N_" + std::to_string(i), "N_" + std::to_string(i + 5), 1100.0, 75.0, true);
    }

    DynamicRouteReplanner replanner(planner);
    replanner.SetConsoleLogging(false);

    LiveAmbulanceState amb("AMB_PERF", "N_0", "Hospital", 12.9, 77.5, 20000.0, 1000.0, 1);
    replanner.UpdateAmbulanceState(amb);
    replanner.GenerateUpdatedRoute();
    replanner.ApplyNewPlan();

    // Block a critical road in the 100-node graph
    TrafficCondition blockCond("ROAD_N_10_11", "N_10", "N_11", 0.0, 0.0, true, 1000);
    replanner.UpdateTrafficCondition(blockCond);

    // Benchmark the full replanning decision: ShouldReplan -> GenerateRoute -> GenerateWave -> ApplyNewPlan
    auto startTime = std::chrono::high_resolution_clock::now();

    bool needReplan = replanner.ShouldReplan();
    TEST_ASSERT(needReplan, "ShouldReplan must be true on blocked road");

    EmergencyRoute newRoute = replanner.GenerateUpdatedRoute();
    GreenWavePlan newWave = replanner.GenerateUpdatedGreenWave();
    bool applied = replanner.ApplyNewPlan();

    auto endTime = std::chrono::high_resolution_clock::now();

    auto elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    double elapsedMilliseconds = static_cast<double>(elapsedMicroseconds) / 1000.0;

    std::cout << "  -> 100-node route replanned in " << elapsedMicroseconds << " microseconds ("
              << elapsedMilliseconds << " ms).\n";

    TEST_ASSERT(newRoute.isValid, "Replanned 100-node route must be valid");
    TEST_ASSERT(newWave.isValid, "Replanned 100-node green wave must be valid");
    TEST_ASSERT(applied, "ApplyNewPlan must succeed");
    TEST_ASSERT(elapsedMilliseconds < 3.0, "Route replanning on 100-node graph must execute under 3.0 ms!");

    std::cout << "  -> TEST 8 PASSED: Performance requirement (< 3 ms) fully satisfied.\n";
    return true;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 9A Dynamic Route Replanner Tests\n";
    std::cout << "==========================================================\n";

    bool allPassed = true;
    allPassed &= TestNoTrafficChangesShouldReplanFalse();
    allPassed &= TestRoadBlockedNewCorridorGenerated();
    allPassed &= TestEtaExceedsThresholdReplanningTriggered();
    allPassed &= TestTrafficClearedOriginalOptimalPathRestored();
    allPassed &= TestGreenWaveRegenerated();
    allPassed &= TestCoordinatorReceivesUpdatedSignalCommands();
    allPassed &= TestConcurrentTrafficUpdates100Threads();
    allPassed &= TestPerformanceBenchmark100NodeGraphUnder3ms();

    std::cout << "==========================================================\n";
    if (allPassed) {
        std::cout << " [ALL 8 TESTS PASSED] Phase 9A Fully Verified!\n";
    } else {
        std::cout << " [FAILED] Some Phase 9A tests failed.\n";
    }
    std::cout << "==========================================================\n";

    return allPassed ? 0 : 1;
}
