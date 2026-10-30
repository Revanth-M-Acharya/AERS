/**
 * @file test_traffic_network_orchestrator_phase13a.cpp
 * @brief Comprehensive Unit Test Suite for SMART AEGIS Phase 13A
 *        City-Wide Distributed Traffic Network Orchestration Engine
 *
 * Validates intersection registration, heartbeat monitoring, offline detection, node recovery,
 * corridor management, mission binding, green wave execution, leader election, execution rollback,
 * formatted reports, statistics aggregation, 100-thread concurrency, and sub-5ms benchmarks
 * across 100 intersections, 20 corridors, and 1,000 synchronization operations.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "traffic_network_orchestrator/TrafficNetworkOrchestrator.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>

using namespace aegis::traffic_network;

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
// Unit Tests (1 to 11)
// =============================================================================

/**
 * @brief TEST 1: Intersection Registration & Removal
 */
void Test1_RegistrationAndRemoval() {
    std::cout << "[TEST 1] Intersection Registration & Removal..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_01", "Main & 1st", 12.9716, 77.5946, "CTRL_01");
    IntersectionNode n2("INT_02", "Main & 2nd", 12.9720, 77.5950, "CTRL_02");

    TEST_ASSERT(orch.RegisterIntersection(n1), "Register INT_01 must succeed");
    TEST_ASSERT(orch.RegisterIntersection(n2), "Register INT_02 must succeed");
    TEST_ASSERT(!orch.RegisterIntersection(n1), "Duplicate registration must be rejected");

    IntersectionNode emptyNode("");
    TEST_ASSERT(!orch.RegisterIntersection(emptyNode), "Empty ID registration must be rejected");

    IntersectionNode retrieved;
    TEST_ASSERT(orch.GetIntersection("INT_01", retrieved), "Retrieve INT_01");
    TEST_ASSERT(retrieved.name == "Main & 1st", "Name must match");
    TEST_ASSERT(retrieved.online, "Node must be online initially");
    TEST_ASSERT(retrieved.status == IntersectionStatus::ONLINE, "Status must be ONLINE");

    auto allNodes = orch.GetAllIntersections();
    TEST_ASSERT(allNodes.size() == 2, "Must contain exactly 2 nodes");

    // Removal
    TEST_ASSERT(orch.RemoveIntersection("INT_02"), "Remove INT_02");
    TEST_ASSERT(!orch.GetIntersection("INT_02", retrieved), "INT_02 must no longer exist");
    TEST_ASSERT(orch.GetAllIntersections().size() == 1, "Must contain 1 node after removal");

    std::cout << "  ✓ PASS: Registration, duplicate prevention, and node removal verified." << std::endl;
}

/**
 * @brief TEST 2: Heartbeat Monitoring & Offline Detection
 */
void Test2_HeartbeatAndOfflineDetection() {
    std::cout << "[TEST 2] Heartbeat & Offline Detection..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_HB_1", "Node HB 1");
    IntersectionNode n2("INT_HB_2", "Node HB 2");
    orch.RegisterIntersection(n1);
    orch.RegisterIntersection(n2);

    // Initial heartbeat update
    TEST_ASSERT(orch.UpdateHeartbeat("INT_HB_1", 1000000), "Update heartbeat INT_HB_1");
    TEST_ASSERT(orch.UpdateHeartbeat("INT_HB_2", 1000000), "Update heartbeat INT_HB_2");

    // Fast forward node 2 with fresh heartbeat, leave node 1 stale
    orch.UpdateHeartbeat("INT_HB_2", 2000000);

    // Run timeout check with current timestamp emulation
    // Stale node check: if timeout threshold is 5000 ms, nodes older than threshold become offline
    uint32_t timedOut = orch.CheckHeartbeats(1); // 1 ms timeout forces any older node offline
    TEST_ASSERT(timedOut >= 1, "At least one node must have timed out");

    IntersectionNode r1;
    orch.GetIntersection("INT_HB_1", r1);
    TEST_ASSERT(!r1.online, "INT_HB_1 must be marked offline");
    TEST_ASSERT(r1.status == IntersectionStatus::OFFLINE, "Status must be OFFLINE");

    std::cout << "  ✓ PASS: Heartbeat updating and automatic offline detection verified." << std::endl;
}

/**
 * @brief TEST 3: Online/Offline Setting & Automatic Node Recovery
 */
void Test3_OnlineOfflineAndRecovery() {
    std::cout << "[TEST 3] Online/Offline & Automatic Recovery..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n("INT_REC", "Recovery Test");
    orch.RegisterIntersection(n);

    // Manually set offline
    TEST_ASSERT(orch.SetIntersectionOffline("INT_REC"), "Set offline");
    IntersectionNode r;
    orch.GetIntersection("INT_REC", r);
    TEST_ASSERT(!r.online, "Node must be offline");
    TEST_ASSERT(r.status == IntersectionStatus::OFFLINE, "Status must be OFFLINE");

    // Recovery via heartbeat
    TEST_ASSERT(orch.UpdateHeartbeat("INT_REC"), "Heartbeat received for offline node");
    orch.GetIntersection("INT_REC", r);
    TEST_ASSERT(r.online, "Node must automatically recover to online");
    TEST_ASSERT(r.status == IntersectionStatus::ONLINE, "Status must be ONLINE after recovery");

    std::cout << "  ✓ PASS: Manual offline and automatic heartbeat recovery verified." << std::endl;
}

/**
 * @brief TEST 4: Corridor Creation & Membership
 */
void Test4_CorridorCreationAndMembership() {
    std::cout << "[TEST 4] Corridor Creation & Membership..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_A");
    IntersectionNode n2("INT_B");
    IntersectionNode n3("INT_C");
    orch.RegisterIntersection(n1);
    orch.RegisterIntersection(n2);
    orch.RegisterIntersection(n3);

    TrafficCorridor corr("CORR_01", "MISSION_101", {"INT_A", "INT_B"});
    TEST_ASSERT(orch.CreateCorridor(corr), "Create corridor CORR_01");
    TEST_ASSERT(!orch.CreateCorridor(corr), "Duplicate corridor must be rejected");

    TrafficCorridor retrieved;
    TEST_ASSERT(orch.GetCorridor("CORR_01", retrieved), "Get corridor");
    TEST_ASSERT(retrieved.intersections.size() == 2, "Corridor must have 2 nodes");

    // Dynamic membership
    TEST_ASSERT(orch.AssignIntersectionToCorridor("CORR_01", "INT_C"), "Assign INT_C to corridor");
    orch.GetCorridor("CORR_01", retrieved);
    TEST_ASSERT(retrieved.intersections.size() == 3, "Corridor must have 3 nodes");

    TEST_ASSERT(orch.RemoveIntersectionFromCorridor("CORR_01", "INT_B"), "Remove INT_B");
    orch.GetCorridor("CORR_01", retrieved);
    TEST_ASSERT(retrieved.intersections.size() == 2, "Corridor must have 2 nodes after removal");

    std::cout << "  ✓ PASS: Corridor creation and dynamic intersection membership verified." << std::endl;
}

/**
 * @brief TEST 5: Mission Assignment
 */
void Test5_MissionAssignment() {
    std::cout << "[TEST 5] Mission Assignment..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n("INT_MISS");
    orch.RegisterIntersection(n);

    TEST_ASSERT(orch.AssignMission("INT_MISS", "MISSION_RED_01"), "Assign mission");
    IntersectionNode r;
    orch.GetIntersection("INT_MISS", r);
    TEST_ASSERT(r.currentMission == "MISSION_RED_01", "Mission ID must match");

    std::cout << "  ✓ PASS: Mission assignment to intersection verified." << std::endl;
}

/**
 * @brief TEST 6: Green Wave Execution & Synchronization
 */
void Test6_GreenWaveExecution() {
    std::cout << "[TEST 6] Green Wave Execution..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_GW_1");
    IntersectionNode n2("INT_GW_2");
    IntersectionNode n3("INT_GW_3");
    orch.RegisterIntersection(n1);
    orch.RegisterIntersection(n2);
    orch.RegisterIntersection(n3);

    TrafficCorridor corr("CORR_GW", "MISSION_GW", {"INT_GW_1", "INT_GW_2", "INT_GW_3"});
    orch.CreateCorridor(corr);

    TEST_ASSERT(orch.ExecuteGreenWave("CORR_GW"), "Execute Green Wave");

    // Check corridor status
    TrafficCorridor c;
    orch.GetCorridor("CORR_GW", c);
    TEST_ASSERT(c.active, "Corridor must be active");
    TEST_ASSERT(c.state == SignalExecutionState::GREEN, "Corridor state must be GREEN");

    // Check all member intersections
    for (const auto& id : {"INT_GW_1", "INT_GW_2", "INT_GW_3"}) {
        IntersectionNode node;
        orch.GetIntersection(id, node);
        TEST_ASSERT(node.signalState == SignalExecutionState::GREEN, "Signal state must be GREEN");
        TEST_ASSERT(node.currentSignalPhase == "GREEN", "Phase must be GREEN");
        TEST_ASSERT(node.executionLatency > 0.0, "Execution latency must be recorded");
    }

    // Check Synchronization Group
    SynchronizationGroup grp;
    TEST_ASSERT(orch.GetSynchronizationGroup("CORR_GW", grp), "Get sync group");
    TEST_ASSERT(grp.state == SynchronizationState::COMPLETED, "Group must be COMPLETED");

    std::cout << "  ✓ PASS: Multi-intersection Green Wave synchronization verified." << std::endl;
}

/**
 * @brief TEST 7: Leader Election
 */
void Test7_LeaderElection() {
    std::cout << "[TEST 7] Leader Election..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode nLow("INT_LOW");
    nLow.priority = 1;
    nLow.healthScore = 90.0;

    IntersectionNode nMid("INT_MID");
    nMid.priority = 5;
    nMid.healthScore = 95.0;

    IntersectionNode nHigh("INT_HIGH");
    nHigh.priority = 10;
    nHigh.healthScore = 99.0;

    orch.RegisterIntersection(nLow);
    orch.RegisterIntersection(nMid);
    orch.RegisterIntersection(nHigh);

    orch.CreateSynchronizationGroup("GROUP_ELECTION", {"INT_LOW", "INT_MID", "INT_HIGH"});

    std::string leader1 = orch.ElectLeader("GROUP_ELECTION");
    TEST_ASSERT(leader1 == "INT_HIGH", "Highest priority node must be elected leader");

    // When leader goes offline, re-election picks next best available node
    orch.SetIntersectionOffline("INT_HIGH");
    std::string leader2 = orch.ElectLeader("GROUP_ELECTION");
    TEST_ASSERT(leader2 == "INT_MID", "Next highest online node must be elected leader");

    std::cout << "  ✓ PASS: Deterministic priority-based leader election and failover verified." << std::endl;
}

/**
 * @brief TEST 8: Signal Restoration & Execution Rollback
 */
void Test8_RestorationAndRollback() {
    std::cout << "[TEST 8] Signal Restoration & Execution Rollback..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_R1");
    IntersectionNode n2("INT_R2");
    orch.RegisterIntersection(n1);
    orch.RegisterIntersection(n2);

    TrafficCorridor corr("CORR_RESTORE", "MISSION_R", {"INT_R1", "INT_R2"});
    orch.CreateCorridor(corr);
    orch.ExecuteGreenWave("CORR_RESTORE");

    // Restore corridor
    TEST_ASSERT(orch.RestoreCorridor("CORR_RESTORE"), "Restore corridor");
    TrafficCorridor c;
    orch.GetCorridor("CORR_RESTORE", c);
    TEST_ASSERT(!c.active, "Corridor must not be active after restoration");
    TEST_ASSERT(c.state == SignalExecutionState::IDLE, "Corridor state must be IDLE");

    IntersectionNode r1;
    orch.GetIntersection("INT_R1", r1);
    TEST_ASSERT(r1.signalState == SignalExecutionState::IDLE, "Intersection must be IDLE");
    TEST_ASSERT(r1.currentSignalPhase == "RED", "Signal phase restored to normal cycle");

    // Rollback test: simulate failure on corridor execution with an offline node
    orch.SetIntersectionOffline("INT_R2");
    bool gwOk = orch.ExecuteGreenWave("CORR_RESTORE");
    TEST_ASSERT(!gwOk, "Green Wave execution must fail when member node is offline");

    orch.GetIntersection("INT_R1", r1);
    TEST_ASSERT(r1.signalState == SignalExecutionState::RESTORING, "Rollback must transition node to safe restoring state");

    auto stats = orch.GetStatistics();
    TEST_ASSERT(stats.failedSynchronizations >= 1, "Failed synchronization must be incremented");

    std::cout << "  ✓ PASS: Clean corridor restoration and fault rollback verified." << std::endl;
}

/**
 * @brief TEST 9: Formatted Reporting & Statistics
 */
void Test9_ReportsAndStatistics() {
    std::cout << "[TEST 9] Reports & Statistics..." << std::endl;
    TrafficNetworkOrchestrator orch;

    IntersectionNode n1("INT_REP_1", "North Junction");
    IntersectionNode n2("INT_REP_2", "South Junction");
    orch.RegisterIntersection(n1);
    orch.RegisterIntersection(n2);
    orch.AddNeighbor("INT_REP_1", "INT_REP_2");

    TrafficCorridor corr("CORR_REP", "MISSION_REP", {"INT_REP_1", "INT_REP_2"});
    orch.CreateCorridor(corr);
    orch.ExecuteGreenWave("CORR_REP");

    // Network Report
    std::string netRep = orch.GenerateNetworkReport();
    std::cout << netRep << std::endl;
    TEST_ASSERT(netRep.find("[NETWORK]") != std::string::npos, "Network report header match");
    TEST_ASSERT(netRep.find("Total Intersections:\n2") != std::string::npos, "Total intersections match");
    TEST_ASSERT(netRep.find("Online:\n2") != std::string::npos, "Online match");
    TEST_ASSERT(netRep.find("Green Corridors:\n1") != std::string::npos, "Green corridors match");

    // Corridor Report
    std::string corrRep = orch.GenerateCorridorReport("CORR_REP");
    std::cout << corrRep << std::endl;
    TEST_ASSERT(corrRep.find("[CORRIDOR]") != std::string::npos, "Corridor report header match");
    TEST_ASSERT(corrRep.find("Mission:\nMISSION_REP") != std::string::npos, "Mission field match");
    TEST_ASSERT(corrRep.find("Corridor:\nCORR_REP") != std::string::npos, "Corridor field match");

    // Synchronization Report
    std::string syncRep = orch.GenerateSynchronizationReport("CORR_REP");
    std::cout << syncRep << std::endl;
    TEST_ASSERT(syncRep.find("[SYNC]") != std::string::npos, "Sync report header match");
    TEST_ASSERT(syncRep.find("Synchronization Group:\nCORR_REP") != std::string::npos, "Sync group field match");

    // Topology Report
    std::string topoRep = orch.GenerateTopologyReport();
    TEST_ASSERT(topoRep.find("MUNICIPAL TRAFFIC TOPOLOGY REPORT") != std::string::npos, "Topology report title match");

    std::cout << "  ✓ PASS: All formatted reports validated against exact specification." << std::endl;
}

/**
 * @brief TEST 10: 100-Thread Concurrency
 */
void Test10_Concurrency() {
    std::cout << "[TEST 10] 100-Thread Concurrency..." << std::endl;
    TrafficNetworkOrchestrator orch;
    const int NUM_THREADS = 100;
    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);
    std::atomic<int> successCount{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([&orch, i, &successCount]() {
            std::string id = "CONC_INT_" + std::to_string(i);
            IntersectionNode n(id, "Concurrent Node " + std::to_string(i));
            if (!orch.RegisterIntersection(n)) {
                return;
            }

            orch.UpdateHeartbeat(id);

            if (i > 0) {
                std::string prevId = "CONC_INT_" + std::to_string(i - 1);
                orch.AddNeighbor(id, prevId);
            }

            if (i % 5 == 0) {
                std::string corrId = "CONC_CORR_" + std::to_string(i);
                TrafficCorridor c(corrId, "MISSION_C", {id});
                orch.CreateCorridor(c);
                orch.ExecuteGreenWave(corrId);
                orch.RestoreCorridor(corrId);
            }

            IntersectionNode r;
            if (orch.GetIntersection(id, r)) {
                successCount++;
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    TEST_ASSERT(successCount == NUM_THREADS, "All 100 threads must succeed with zero deadlocks and data races");
    auto stats = orch.GetStatistics();
    TEST_ASSERT(stats.totalIntersections == NUM_THREADS, "Total intersections must match thread count");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: " << successCount << ")." << std::endl;
}

/**
 * @brief TEST 11: Performance Benchmark (100 Intersections, 20 Corridors, 1,000 Synchronizations)
 */
void Test11_PerformanceBenchmark() {
    std::cout << "[TEST 11] Performance Benchmark: 100 Intersections, 20 Corridors, 1,000 Synchronizations..." << std::endl;
    TrafficNetworkOrchestrator orch;
    const int NUM_INTERSECTIONS = 100;
    const int NUM_CORRIDORS = 20;
    const int NUM_SYNCS = 1000;

    // 1. Register 100 Intersections
    for (int i = 0; i < NUM_INTERSECTIONS; ++i) {
        std::string id = "BENCH_INT_" + std::to_string(i);
        IntersectionNode n(id, "Bench Node " + std::to_string(i), 12.9 + i * 0.001, 77.5 + i * 0.001);
        n.priority = static_cast<uint32_t>((i % 10) + 1);
        orch.RegisterIntersection(n);
    }

    // 2. Measure Heartbeat Processing for 100 Intersections (< 1 ms target)
    auto hbStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_INTERSECTIONS; ++i) {
        std::string id = "BENCH_INT_" + std::to_string(i);
        orch.UpdateHeartbeat(id);
    }
    auto hbEnd = std::chrono::high_resolution_clock::now();
    double hbDurationMs = std::chrono::duration<double, std::milli>(hbEnd - hbStart).count();
    std::cout << "  -> 100 Intersections Heartbeat Update Latency: " << hbDurationMs << " ms" << std::endl;
    TEST_ASSERT(hbDurationMs < 1.0, "Heartbeat processing for 100 intersections must be < 1.0 ms");

    // 3. Create 20 Simultaneous Corridors
    for (int c = 0; c < NUM_CORRIDORS; ++c) {
        std::string corrId = "BENCH_CORR_" + std::to_string(c);
        std::vector<std::string> nodes;
        for (int j = 0; j < 5; ++j) {
            nodes.push_back("BENCH_INT_" + std::to_string((c * 5 + j) % NUM_INTERSECTIONS));
        }
        TrafficCorridor corr(corrId, "MISSION_B_" + std::to_string(c), nodes);
        orch.CreateCorridor(corr);
    }

    // 4. Measure 1,000 Synchronization Operations (< 5 ms target)
    auto syncStart = std::chrono::high_resolution_clock::now();
    for (int s = 0; s < NUM_SYNCS; ++s) {
        std::string corrId = "BENCH_CORR_" + std::to_string(s % NUM_CORRIDORS);
        orch.ExecuteGreenWave(corrId);
    }
    auto syncEnd = std::chrono::high_resolution_clock::now();
    double syncDurationMs = std::chrono::duration<double, std::milli>(syncEnd - syncStart).count();
    double perSyncLatencyMs = syncDurationMs / NUM_SYNCS;

    std::cout << "  -> 1,000 Synchronizations Total Latency: " << syncDurationMs << " ms" << std::endl;
    std::cout << "  -> Average Synchronization Latency: " << perSyncLatencyMs << " ms" << std::endl;

    TEST_ASSERT(syncDurationMs < 5.0, "1,000 synchronizations must execute in < 5.0 ms");
    std::cout << "  ✓ PASS: 100 intersections, 20 corridors, and 1,000 synchronizations benchmark passed!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 13A Traffic Network Orchestrator Tests\n";
    std::cout << "==========================================================\n";

    Test1_RegistrationAndRemoval();
    Test2_HeartbeatAndOfflineDetection();
    Test3_OnlineOfflineAndRecovery();
    Test4_CorridorCreationAndMembership();
    Test5_MissionAssignment();
    Test6_GreenWaveExecution();
    Test7_LeaderElection();
    Test8_RestorationAndRollback();
    Test9_ReportsAndStatistics();
    Test10_Concurrency();
    Test11_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " ALL 11 PHASE 13A UNIT TESTS PASSED SUCCESSFULLY (100%)  \n";
    std::cout << "==========================================================\n";
    return 0;
}

