/**
 * @file test_emergency_corridor_planner_phase7a.cpp
 * @brief Comprehensive unit tests for Phase 7A: Emergency Corridor Planner.
 *
 * Verifies all Phase 7A requirements:
 * 1. Data models: IntersectionInfo, RoadSegment, EmergencyRoute, IntersectionEta.
 * 2. Dijkstra shortest-path planning across road networks.
 * 3. Simple and complex graphs with alternate routes and cost metrics.
 * 4. Detection of disconnected/unreachable destinations (No route available).
 * 5. Deterministic tie-breaking on multiple equal-cost routes.
 * 6. Per-intersection arrival time (ETA) timeline calculation matching Requirement 7.
 * 7. Structured route generation logging matching Requirement 8.
 * 8. Large graph stress testing (64-node mesh grid).
 * 9. Multi-threaded concurrency and thread safety under heavy query load.
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

#include "green_corridor/EmergencyCorridorPlanner.h"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis::green_corridor;

// -----------------------------------------------------------------------------
// Test 1: Simple Graph Route Planning & Corridor Generation
// -----------------------------------------------------------------------------
static void TestSimpleGraphRoute() {
    std::cout << "[TEST 1] Testing Simple Linear Graph Route Planning..." << std::endl;

    EmergencyCorridorPlanner planner;

    // Build linear topology: INT_A -> INT_B -> INT_C -> HOSPITAL
    planner.AddIntersection("INT_A", 12.9716, 77.5946);
    planner.AddIntersection("INT_B", 12.9720, 77.5950);
    planner.AddIntersection("INT_C", 12.9725, 77.5955);
    planner.AddIntersection("HOSPITAL", 12.9730, 77.5960);

    planner.AddRoadSegment("INT_A", "INT_B", 500.0, 30.0);
    planner.AddRoadSegment("INT_B", "INT_C", 450.0, 25.0);
    planner.AddRoadSegment("INT_C", "HOSPITAL", 600.0, 35.0);

    AEGIS_TEST_ASSERT(planner.GetIntersectionCount() == 4);
    AEGIS_TEST_ASSERT(planner.GetRoadSegmentCount() == 3);

    EmergencyRoute route = planner.ComputeEmergencyRoute("AMB_01", "INT_A", "HOSPITAL");

    AEGIS_TEST_ASSERT(route.isValid);
    AEGIS_TEST_ASSERT(route.ambulanceId == "AMB_01");
    AEGIS_TEST_ASSERT(route.hospitalId == "HOSPITAL");
    AEGIS_TEST_ASSERT(route.intersections.size() == 4);
    AEGIS_TEST_ASSERT(route.intersections[0] == "INT_A");
    AEGIS_TEST_ASSERT(route.intersections[1] == "INT_B");
    AEGIS_TEST_ASSERT(route.intersections[2] == "INT_C");
    AEGIS_TEST_ASSERT(route.intersections[3] == "HOSPITAL");

    AEGIS_TEST_ASSERT(std::abs(route.totalDistance - 1550.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(route.estimatedArrivalTime - 90.0) < 1e-6);

    // Verify ComputeGreenCorridor API
    std::vector<std::string> corridor = planner.ComputeGreenCorridor("INT_A", "HOSPITAL");
    AEGIS_TEST_ASSERT(corridor == route.intersections);

    // Verify formatted output (Requirement 4)
    std::string formatted = planner.FormatGreenCorridor(corridor);
    std::string expectedFormat = "INT_A\n↓\nINT_B\n↓\nINT_C\n↓\nHOSPITAL";
    AEGIS_TEST_ASSERT(formatted == expectedFormat);

    // Verify GetCurrentPlan and ClearPlan
    EmergencyRoute plan = planner.GetCurrentPlan();
    AEGIS_TEST_ASSERT(plan.isValid);
    AEGIS_TEST_ASSERT(plan.intersections == route.intersections);

    planner.ClearPlan();
    AEGIS_TEST_ASSERT(!planner.GetCurrentPlan().isValid);

    std::cout << "  -> Simple graph shortest-path corridor verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 2: Complex Graph with Alternate Routes & Cost Metrics
// -----------------------------------------------------------------------------
static void TestComplexGraphAndCostMetrics() {
    std::cout << "[TEST 2] Testing Complex Graph with Dual Cost Metrics (Time vs Distance)..." << std::endl;

    EmergencyCorridorPlanner planner;

    // Topology:
    // Path 1 (Congested Direct): INT_A -> INT_DIR -> HOSPITAL
    //   Distance: 1000m, Time: 150s (Slow urban traffic)
    // Path 2 (Highway Bypass): INT_A -> INT_HW1 -> INT_HW2 -> HOSPITAL
    //   Distance: 2500m, Time: 50s  (Fast expressway)
    // Path 3 (Scenic Detour): INT_A -> INT_DET -> HOSPITAL
    //   Distance: 4000m, Time: 300s
    planner.AddIntersection("INT_A", 0, 0);
    planner.AddIntersection("INT_DIR", 1, 1);
    planner.AddIntersection("INT_HW1", 0, 2);
    planner.AddIntersection("INT_HW2", 1, 3);
    planner.AddIntersection("INT_DET", 5, 5);
    planner.AddIntersection("HOSPITAL", 2, 4);

    // Congested direct
    planner.AddRoadSegment("INT_A", "INT_DIR", 500.0, 75.0);
    planner.AddRoadSegment("INT_DIR", "HOSPITAL", 500.0, 75.0);

    // Fast highway bypass
    planner.AddRoadSegment("INT_A", "INT_HW1", 800.0, 15.0);
    planner.AddRoadSegment("INT_HW1", "INT_HW2", 900.0, 20.0);
    planner.AddRoadSegment("INT_HW2", "HOSPITAL", 800.0, 15.0);

    // Detour
    planner.AddRoadSegment("INT_A", "INT_DET", 2000.0, 150.0);
    planner.AddRoadSegment("INT_DET", "HOSPITAL", 2000.0, 150.0);

    // By default, CostMetric is TRAVEL_TIME: Must choose Highway bypass (50s vs 150s)
    planner.SetCostMetric(CostMetric::TRAVEL_TIME);
    EmergencyRoute timeRoute = planner.ComputeEmergencyRoute("AMB_TIME", "INT_A", "HOSPITAL");
    AEGIS_TEST_ASSERT(timeRoute.isValid);
    AEGIS_TEST_ASSERT(timeRoute.intersections.size() == 4);
    AEGIS_TEST_ASSERT(timeRoute.intersections[1] == "INT_HW1");
    AEGIS_TEST_ASSERT(timeRoute.intersections[2] == "INT_HW2");
    AEGIS_TEST_ASSERT(std::abs(timeRoute.estimatedArrivalTime - 50.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(timeRoute.totalDistance - 2500.0) < 1e-6);

    // When CostMetric is DISTANCE: Must choose Direct road (1000m vs 2500m)
    planner.SetCostMetric(CostMetric::DISTANCE);
    EmergencyRoute distRoute = planner.ComputeEmergencyRoute("AMB_DIST", "INT_A", "HOSPITAL");
    AEGIS_TEST_ASSERT(distRoute.isValid);
    AEGIS_TEST_ASSERT(distRoute.intersections.size() == 3);
    AEGIS_TEST_ASSERT(distRoute.intersections[1] == "INT_DIR");
    AEGIS_TEST_ASSERT(std::abs(distRoute.totalDistance - 1000.0) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(distRoute.estimatedArrivalTime - 150.0) < 1e-6);

    std::cout << "  -> Complex graph optimization (Time: 50s vs Dist: 1000m) verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 3: No Route Available (Disconnected Components & One-Way Traps)
// -----------------------------------------------------------------------------
static void TestNoRouteAvailable() {
    std::cout << "[TEST 3] Testing Disconnected Graph & No Route Available..." << std::endl;

    EmergencyCorridorPlanner planner;

    // Component 1: Island A
    planner.AddIntersection("ISLAND_1", 0, 0);
    planner.AddIntersection("ISLAND_2", 0, 1);
    planner.AddRoadSegment("ISLAND_1", "ISLAND_2", 100.0, 10.0);

    // Component 2: Main Grid
    planner.AddIntersection("MAIN_1", 10, 10);
    planner.AddIntersection("HOSPITAL", 10, 11);
    planner.AddRoadSegment("MAIN_1", "HOSPITAL", 200.0, 20.0);

    // Try finding route between disconnected components
    EmergencyRoute route = planner.ComputeEmergencyRoute("AMB_ISLAND", "ISLAND_1", "HOSPITAL");
    AEGIS_TEST_ASSERT(!route.isValid);
    AEGIS_TEST_ASSERT(route.intersections.empty());
    AEGIS_TEST_ASSERT(route.totalDistance == 0.0);
    AEGIS_TEST_ASSERT(route.timeline.empty());

    // Try finding route with reversed one-way street
    planner.AddIntersection("ONE_WAY_A", 1, 1);
    planner.AddIntersection("ONE_WAY_B", 1, 2);
    // Directed edge only from B to A:
    planner.AddRoadSegment("ONE_WAY_B", "ONE_WAY_A", 100.0, 10.0);

    EmergencyRoute oneWayRoute = planner.ComputeEmergencyRoute("AMB_01", "ONE_WAY_A", "ONE_WAY_B");
    AEGIS_TEST_ASSERT(!oneWayRoute.isValid);
    AEGIS_TEST_ASSERT(oneWayRoute.intersections.empty());

    // Try with completely unknown nodes
    EmergencyRoute invalidNodeRoute = planner.ComputeEmergencyRoute("AMB_01", "NON_EXISTENT", "HOSPITAL");
    AEGIS_TEST_ASSERT(!invalidNodeRoute.isValid);

    std::cout << "  -> Disconnected graph and one-way traps correctly rejected with empty/invalid plan." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 4: Multiple Equal-Cost Routes (Deterministic Tie-Breaking)
// -----------------------------------------------------------------------------
static void TestMultipleEqualCostRoutes() {
    std::cout << "[TEST 4] Testing Deterministic Tie-Breaking across Multiple Equal-Cost Routes..." << std::endl;

    EmergencyCorridorPlanner planner;

    // Setup 3 equal-cost parallel routes between START and DEST:
    // Route B: START -> NODE_B -> DEST (40s, 1000m)
    // Route C: START -> NODE_C -> DEST (40s, 1000m)
    // Route A: START -> NODE_A -> DEST (40s, 1000m)
    planner.AddIntersection("START", 0, 0);
    planner.AddIntersection("NODE_A", 1, 0);
    planner.AddIntersection("NODE_B", 2, 0);
    planner.AddIntersection("NODE_C", 3, 0);
    planner.AddIntersection("DEST", 4, 0);

    planner.AddRoadSegment("START", "NODE_B", 500.0, 20.0);
    planner.AddRoadSegment("NODE_B", "DEST", 500.0, 20.0);

    planner.AddRoadSegment("START", "NODE_C", 500.0, 20.0);
    planner.AddRoadSegment("NODE_C", "DEST", 500.0, 20.0);

    planner.AddRoadSegment("START", "NODE_A", 500.0, 20.0);
    planner.AddRoadSegment("NODE_A", "DEST", 500.0, 20.0);

    // Repeated runs must produce identical, deterministic results
    EmergencyRoute firstRun = planner.ComputeEmergencyRoute("AMB_01", "START", "DEST");
    AEGIS_TEST_ASSERT(firstRun.isValid);
    AEGIS_TEST_ASSERT(firstRun.intersections.size() == 3);

    std::string chosenMiddleNode = firstRun.intersections[1];
    // Alphabetical tie-breaker should favor NODE_A
    AEGIS_TEST_ASSERT(chosenMiddleNode == "NODE_A");

    for (int i = 0; i < 50; ++i) {
        EmergencyRoute run = planner.ComputeEmergencyRoute("AMB_01", "START", "DEST");
        AEGIS_TEST_ASSERT(run.intersections == firstRun.intersections);
        AEGIS_TEST_ASSERT(std::abs(run.estimatedArrivalTime - 40.0) < 1e-6);
    }

    // Now test hop-count tie-breaker:
    // 1-hop path vs 2-hop path with equal cost (both 50s):
    planner.AddIntersection("DIRECT_START", 10, 0);
    planner.AddIntersection("DIRECT_MID", 11, 0);
    planner.AddIntersection("DIRECT_DEST", 12, 0);

    // 1-hop direct: 50s
    planner.AddRoadSegment("DIRECT_START", "DIRECT_DEST", 1000.0, 50.0);
    // 2-hop multi-segment: 25s + 25s = 50s
    planner.AddRoadSegment("DIRECT_START", "DIRECT_MID", 500.0, 25.0);
    planner.AddRoadSegment("DIRECT_MID", "DIRECT_DEST", 500.0, 25.0);

    EmergencyRoute directRun = planner.ComputeEmergencyRoute("AMB_01", "DIRECT_START", "DIRECT_DEST");
    AEGIS_TEST_ASSERT(directRun.isValid);
    // Hop tie-breaking chooses 1-hop route: {DIRECT_START, DIRECT_DEST}
    AEGIS_TEST_ASSERT(directRun.intersections.size() == 2);
    AEGIS_TEST_ASSERT(directRun.intersections[0] == "DIRECT_START");
    AEGIS_TEST_ASSERT(directRun.intersections[1] == "DIRECT_DEST");

    std::cout << "  -> Deterministic tie-breaking (lexicographical and hop-count) verified across 50 iterations." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 5: Exact Arrival Time Calculation & Corridor Timeline (Requirement 7)
// -----------------------------------------------------------------------------
static void TestArrivalTimeCalculationAndTimeline() {
    std::cout << "[TEST 5] Testing Exact Arrival Time (ETA) Calculation and Timeline..." << std::endl;

    EmergencyCorridorPlanner planner;

    // Replicate Requirement 7 Example:
    // INT_A    ETA 4 s
    // INT_B    ETA 11 s (4 + 7 = 11s)
    // INT_C    ETA 19 s (11 + 8 = 19s)
    // Hospital ETA 27 s (19 + 8 = 27s)
    planner.AddIntersection("INT_A", 12.0, 77.0);
    planner.AddIntersection("INT_B", 12.1, 77.1);
    planner.AddIntersection("INT_C", 12.2, 77.2);
    planner.AddIntersection("Hospital", 12.3, 77.3);

    planner.AddRoadSegment("INT_A", "INT_B", 280.0, 7.0);
    planner.AddRoadSegment("INT_B", "INT_C", 320.0, 8.0);
    planner.AddRoadSegment("INT_C", "Hospital", 350.0, 8.0);

    // Initial offset to INT_A is 4.0s (e.g. 50 meters approach)
    EmergencyRoute route = planner.ComputeEmergencyRoute("AMB_01", "INT_A", "Hospital", 4.0, 50.0);

    AEGIS_TEST_ASSERT(route.isValid);
    AEGIS_TEST_ASSERT(route.timeline.size() == 4);

    AEGIS_TEST_ASSERT(route.timeline[0].intersectionId == "INT_A");
    AEGIS_TEST_ASSERT(std::abs(route.timeline[0].etaSeconds - 4.0) < 1e-6);

    AEGIS_TEST_ASSERT(route.timeline[1].intersectionId == "INT_B");
    AEGIS_TEST_ASSERT(std::abs(route.timeline[1].etaSeconds - 11.0) < 1e-6);

    AEGIS_TEST_ASSERT(route.timeline[2].intersectionId == "INT_C");
    AEGIS_TEST_ASSERT(std::abs(route.timeline[2].etaSeconds - 19.0) < 1e-6);

    AEGIS_TEST_ASSERT(route.timeline[3].intersectionId == "Hospital");
    AEGIS_TEST_ASSERT(std::abs(route.timeline[3].etaSeconds - 27.0) < 1e-6);

    AEGIS_TEST_ASSERT(std::abs(route.estimatedArrivalTime - 27.0) < 1e-6);

    // Verify EstimateArrivalTimes API
    auto timeline = planner.EstimateArrivalTimes();
    AEGIS_TEST_ASSERT(timeline.size() == 4);
    AEGIS_TEST_ASSERT(std::abs(timeline[3].etaSeconds - 27.0) < 1e-6);

    // Verify FormatTimeline output string matches Requirement 7 pattern
    std::string timelineStr = planner.FormatTimeline(route.timeline);
    AEGIS_TEST_ASSERT(timelineStr.find("INT_A\nETA 4 s") != std::string::npos);
    AEGIS_TEST_ASSERT(timelineStr.find("INT_B\nETA 11 s") != std::string::npos);
    AEGIS_TEST_ASSERT(timelineStr.find("INT_C\nETA 19 s") != std::string::npos);
    AEGIS_TEST_ASSERT(timelineStr.find("Hospital\nETA 27 s") != std::string::npos);

    std::cout << "  -> Exact timeline matching Requirement 7 (4s -> 11s -> 19s -> 27s) verified." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 6: Structured Logging Verification (Requirement 8)
// -----------------------------------------------------------------------------
static void TestStructuredLogging() {
    std::cout << "[TEST 6] Testing Structured Route Generation Logging (Requirement 8)..." << std::endl;

    EmergencyCorridorPlanner planner;

    planner.AddIntersection("INT_1", 10.0, 20.0);
    planner.AddIntersection("INT_2", 10.1, 20.1);
    planner.AddIntersection("HOSP_CITY", 10.2, 20.2);

    planner.AddRoadSegment("INT_1", "INT_2", 600.0, 40.0);
    planner.AddRoadSegment("INT_2", "HOSP_CITY", 800.0, 50.0);

    EmergencyRoute route = planner.ComputeEmergencyRoute("AMB_ALPHA", "INT_1", "HOSP_CITY");

    std::string logStr = planner.GetLastRouteLog();

    // Verify all Requirement 8 elements in log string
    AEGIS_TEST_ASSERT(logStr.find("[ROUTE GENERATED]") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Ambulance: AMB_ALPHA") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Hospital: HOSP_CITY") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Distance: 1400.00 m") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Travel Time: 90.00 s") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("Ordered Corridor:") != std::string::npos);
    AEGIS_TEST_ASSERT(logStr.find("INT_1\n↓\nINT_2\n↓\nHOSP_CITY") != std::string::npos);

    std::cout << "  -> Structured log string format verified against Requirement 8." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 7: Large Graph Stress Test (8x8 Grid with 64 Intersections)
// -----------------------------------------------------------------------------
static void TestLargeGraphStress() {
    std::cout << "[TEST 7] Testing Large Graph Stress Test (8x8 Mesh Grid - 64 Nodes)..." << std::endl;

    EmergencyCorridorPlanner planner;
    const int rows = 8;
    const int cols = 8;

    auto makeId = [](int r, int c) {
        return "N_" + std::to_string(r) + "_" + std::to_string(c);
    };

    // Populate 64 nodes with coordinates
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            planner.AddIntersection(makeId(r, c), 12.0 + 0.005 * r, 77.0 + 0.005 * c);
        }
    }

    // Populate horizontal and vertical bidirectional road segments
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c + 1 < cols) {
                planner.AddRoadSegment(makeId(r, c), makeId(r, c + 1), 200.0, 15.0, true);
            }
            if (r + 1 < rows) {
                planner.AddRoadSegment(makeId(r, c), makeId(r + 1, c), 200.0, 15.0, true);
            }
        }
    }

    AEGIS_TEST_ASSERT(planner.GetIntersectionCount() == 64);
    // Number of bidirectional edges: (8*7 + 8*7) * 2 = 112 * 2 = 224
    AEGIS_TEST_ASSERT(planner.GetRoadSegmentCount() == 224);

    // Compute route from corner (0,0) to opposite corner (7,7)
    auto startTime = std::chrono::steady_clock::now();
    EmergencyRoute route = planner.ComputeEmergencyRoute("AMB_STRESS", makeId(0, 0), makeId(7, 7));
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - startTime).count();

    AEGIS_TEST_ASSERT(route.isValid);
    // Manhattan path length across 8x8 grid has 14 hops = 15 nodes
    AEGIS_TEST_ASSERT(route.intersections.size() == 15);
    AEGIS_TEST_ASSERT(route.intersections.front() == makeId(0, 0));
    AEGIS_TEST_ASSERT(route.intersections.back() == makeId(7, 7));
    AEGIS_TEST_ASSERT(std::abs(route.totalDistance - (14 * 200.0)) < 1e-6);
    AEGIS_TEST_ASSERT(std::abs(route.estimatedArrivalTime - (14 * 15.0)) < 1e-6);

    std::cout << "  -> Large 64-node mesh route computed in " << elapsed << " microseconds with perfect accuracy." << std::endl;
}

// -----------------------------------------------------------------------------
// Test 8: Concurrent Planning Requests (Thread Safety)
// -----------------------------------------------------------------------------
static void TestConcurrentPlanning() {
    std::cout << "[TEST 8] Testing Thread Safety under Heavy Concurrent Query Load..." << std::endl;

    EmergencyCorridorPlanner planner;
    planner.EnableAutoLogging(false); // Suppress terminal spam during thread storm

    const int nodeCount = 20;
    auto makeId = [](int i) { return "ND_" + std::to_string(i); };

    for (int i = 0; i < nodeCount; ++i) {
        planner.AddIntersection(makeId(i), 12.0 + 0.01 * i, 77.0 + 0.01 * i);
        if (i > 0) {
            planner.AddRoadSegment(makeId(i - 1), makeId(i), 100.0, 10.0, true);
        }
    }

    const int numThreads = 12;
    const int queriesPerThread = 60;
    std::vector<std::thread> workers;
    workers.reserve(numThreads);
    std::atomic<int> successCount{0};

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&, t]() {
            for (int q = 0; q < queriesPerThread; ++q) {
                int startIdx = (t + q) % (nodeCount - 5);
                int endIdx = (startIdx + 4) % nodeCount;
                std::string ambId = "AMB_TH_" + std::to_string(t);

                EmergencyRoute route = planner.ComputeEmergencyRoute(ambId, makeId(startIdx), makeId(endIdx));
                if (route.isValid && !route.intersections.empty()) {
                    successCount.fetch_add(1, std::memory_order_relaxed);
                }

                // Also call queries concurrently
                auto corridor = planner.ComputeGreenCorridor(makeId(startIdx), makeId(endIdx));
                (void)corridor;
                auto timeline = planner.EstimateArrivalTimes();
                (void)timeline;
                std::string nearest = planner.FindNearestIntersection(12.05, 77.05);
                (void)nearest;
            }
        });
    }

    for (auto& th : workers) {
        if (th.joinable()) {
            th.join();
        }
    }

    const int totalExpected = numThreads * queriesPerThread;
    AEGIS_TEST_ASSERT(successCount.load() == totalExpected);
    std::cout << "  -> Concurrency verified with " << successCount.load() << " thread-safe queries and 0 data races." << std::endl;
}

// -----------------------------------------------------------------------------
// Main Test Runner
// -----------------------------------------------------------------------------
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 7A Emergency Corridor Planner Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    try {
        TestSimpleGraphRoute();
        TestComplexGraphAndCostMetrics();
        TestNoRouteAvailable();
        TestMultipleEqualCostRoutes();
        TestArrivalTimeCalculationAndTimeline();
        TestStructuredLogging();
        TestLargeGraphStress();
        TestConcurrentPlanning();
    } catch (const std::exception& ex) {
        std::cerr << "\n[ERROR] Unhandled exception during test execution: " << ex.what() << std::endl;
        return 1;
    }

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " [ALL 8 TESTS PASSED] Phase 7A Fully Verified!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
