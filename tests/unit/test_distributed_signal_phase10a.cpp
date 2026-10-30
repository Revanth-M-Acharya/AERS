/**
 * @file test_distributed_signal_phase10a.cpp
 * @brief Comprehensive verification suite for Phase 10A: Distributed Traffic Signal Synchronization Engine.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <iomanip>

#include "distributed_signal/DistributedSignalSynchronizer.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "coordinator/CoordinatorManager.h"

using namespace aegis;
using namespace aegis::distributed_signal;
using namespace aegis::green_wave;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << (msg) << " (" << #cond << ") at " \
                      << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

namespace {

GreenWavePlan BuildSamplePlan(const std::string& ambId,
                              const std::vector<std::string>& intersections,
                              double startOffset = 0.0,
                              double windowDuration = 15.0,
                              double spacing = 5.0) {
    GreenWavePlan plan;
    plan.ambulanceId = ambId;
    plan.routeId = "ROUTE_" + ambId;
    plan.isValid = !intersections.empty();

    double currentStart = startOffset;
    for (const auto& intId : intersections) {
        GreenWindow w;
        w.intersectionId = intId;
        w.startTimeSeconds = currentStart;
        w.endTimeSeconds = currentStart + windowDuration;
        w.command = "GREEN";
        w.safetyBufferSeconds = 3.0;
        w.clearanceSeconds = 5.0;
        w.etaSeconds = currentStart + 3.0;
        plan.windows.push_back(w);
        currentStart += spacing;
    }
    plan.totalDurationSeconds = currentStart + windowDuration;
    return plan;
}

// =============================================================================
// TEST 1: Register Intersections
// =============================================================================
void Test1_RegisterIntersections() {
    std::cout << "[TEST 1] Register intersections..." << std::endl;

    DistributedSignalSynchronizer sync;

    // Register single intersection
    bool reg1 = sync.RegisterIntersection("INT_A", "RED");
    TEST_ASSERT(reg1, "Registration of INT_A should succeed");
    TEST_ASSERT(sync.HasIntersection("INT_A"), "sync should contain INT_A");
    TEST_ASSERT(sync.GetRegisteredIntersectionCount() == 1, "Count should be 1");

    auto statusA = sync.GetIntersectionStatus("INT_A");
    TEST_ASSERT(statusA.intersectionId == "INT_A", "Id should match");
    TEST_ASSERT(statusA.currentSignalState == "RED", "Initial state should be RED");
    TEST_ASSERT(statusA.executionStatus == "IDLE", "Initial status should be IDLE");
    TEST_ASSERT(!statusA.ackReceived, "ackReceived should initially be false");
    TEST_ASSERT(statusA.isOnline, "Node should initially be online");

    // Register multiple intersections
    bool reg2 = sync.RegisterIntersection("INT_B", "RED");
    bool reg3 = sync.RegisterIntersection("INT_C", "YELLOW");
    TEST_ASSERT(reg2 && reg3, "Multiple registrations should succeed");
    TEST_ASSERT(sync.GetRegisteredIntersectionCount() == 3, "Count should be 3");

    auto list = sync.GetRegisteredIntersections();
    TEST_ASSERT(list.size() == 3, "List size should be 3");
    TEST_ASSERT(list[0] == "INT_A" && list[1] == "INT_B" && list[2] == "INT_C", "List sorted");

    // Remove intersection
    bool removed = sync.RemoveIntersection("INT_B");
    TEST_ASSERT(removed, "INT_B removal should return true");
    TEST_ASSERT(!sync.HasIntersection("INT_B"), "INT_B should no longer exist");
    TEST_ASSERT(sync.GetRegisteredIntersectionCount() == 2, "Count should be 2 after removal");

    // Remove non-existent
    bool removedFalse = sync.RemoveIntersection("NON_EXISTENT");
    TEST_ASSERT(!removedFalse, "Removing non-existent intersection should return false");

    std::cout << "  ✓ PASS: Single & multiple registration, retrieval, and removal verified.\n";
}

// =============================================================================
// TEST 2: Broadcast Execution Commands
// =============================================================================
void Test2_BroadcastExecutionCommands() {
    std::cout << "[TEST 2] Broadcast execution commands..." << std::endl;

    DistributedSignalSynchronizer sync;
    sync.RegisterIntersection("INT_A", "RED");
    sync.RegisterIntersection("INT_B", "RED");

    GreenWavePlan plan = BuildSamplePlan("AMB_01", {"INT_A", "INT_B"}, 5.0, 20.0, 8.0);
    auto cmds = sync.SendExecutionPlan(plan, "CRITICAL", 1, "AMB_01");

    TEST_ASSERT(cmds.size() == 2, "2 commands should be generated for 2 intersections");
    TEST_ASSERT(cmds[0].intersectionId == "INT_A", "First command should target INT_A");
    TEST_ASSERT(cmds[0].signalPhase == "GREEN", "Phase should be GREEN");
    TEST_ASSERT(cmds[0].executionTime == 5.0, "Execution time should be 5.0s");
    TEST_ASSERT(cmds[0].duration == 20.0, "Duration should be 20.0s");
    TEST_ASSERT(cmds[0].priority == "CRITICAL", "Priority should be CRITICAL");
    TEST_ASSERT(cmds[0].routeVersion == 1, "Route version should be 1");

    // Verify queue and state transitioned to PENDING
    TEST_ASSERT(sync.GetExecutionQueueSize() == 2, "Queue size should be 2");
    auto stateA = sync.GetIntersectionStatus("INT_A");
    TEST_ASSERT(stateA.executionStatus == "PENDING", "State should be PENDING prior to broadcast");

    // Broadcast execution
    auto results = sync.BroadcastExecution();
    TEST_ASSERT(results.size() == 2, "Broadcast should return 2 execution results");
    TEST_ASSERT(results[0].success, "Result 0 should be successful");
    TEST_ASSERT(results[1].success, "Result 1 should be successful");
    TEST_ASSERT(results[0].intersectionId == "INT_A", "Result 0 target INT_A");

    // Verify state transitioned to EXECUTING
    auto updatedA = sync.GetIntersectionStatus("INT_A");
    TEST_ASSERT(updatedA.executionStatus == "EXECUTING", "Status should transition to EXECUTING");
    TEST_ASSERT(updatedA.currentSignalState == "GREEN", "Signal state should be GREEN");

    // Test structured log formatting
    std::string log = DistributedSignalSynchronizer::FormatExecutionLog(cmds[0], results[0]);
    TEST_ASSERT(log.find("[DISTRIBUTED SIGNAL]") != std::string::npos, "Log must contain header");
    TEST_ASSERT(log.find("Command: " + cmds[0].commandId) != std::string::npos, "Log must contain command ID");
    TEST_ASSERT(log.find("Phase: GREEN") != std::string::npos, "Log must contain phase");

    std::cout << "  ✓ PASS: Plan splitting, command queueing, broadcasting, and audit logging verified.\n";
}

// =============================================================================
// TEST 3: Receive Acknowledgements
// =============================================================================
void Test3_ReceiveAcknowledgements() {
    std::cout << "[TEST 3] Receive acknowledgements..." << std::endl;

    DistributedSignalSynchronizer sync;
    sync.RegisterIntersection("INT_X", "RED");

    GreenWavePlan plan = BuildSamplePlan("AMB_02", {"INT_X"}, 2.0, 15.0);
    auto cmds = sync.SendExecutionPlan(plan);
    sync.BroadcastExecution();

    auto stateBeforeAck = sync.GetIntersectionStatus("INT_X");
    TEST_ASSERT(!stateBeforeAck.ackReceived, "ackReceived should be false before ack");

    // Deliver positive acknowledgement
    bool ackProcessed = sync.ReceiveAcknowledgement("INT_X", cmds[0].commandId, true);
    TEST_ASSERT(ackProcessed, "Positive acknowledgement should be processed");

    auto stateAfterAck = sync.GetIntersectionStatus("INT_X");
    TEST_ASSERT(stateAfterAck.ackReceived, "ackReceived must be true after ack");
    TEST_ASSERT(stateAfterAck.executionStatus == "COMPLETED", "Status should transition to COMPLETED");

    // Verify execution history recorded
    auto history = sync.GetExecutionHistory();
    TEST_ASSERT(!history.empty(), "Execution history should contain results");
    bool foundAck = false;
    for (const auto& res : history) {
        if (res.intersectionId == "INT_X" && res.acknowledged) {
            foundAck = true;
            TEST_ASSERT(res.success, "Result must indicate success");
            TEST_ASSERT(res.latency >= 0.0, "Latency must be non-negative");
            break;
        }
    }
    TEST_ASSERT(foundAck, "History must contain acknowledged result");

    // Deliver negative acknowledgement for failed phase switch
    sync.RegisterIntersection("INT_Y", "RED");
    GreenWavePlan planY = BuildSamplePlan("AMB_02", {"INT_Y"}, 10.0, 15.0);
    auto cmdsY = sync.SendExecutionPlan(planY);
    sync.BroadcastExecution();

    bool ackNeg = sync.ReceiveAcknowledgement("INT_Y", cmdsY[0].commandId, false, "Phase conflict detected");
    TEST_ASSERT(ackNeg, "Negative acknowledgement processed");
    auto stateY = sync.GetIntersectionStatus("INT_Y");
    TEST_ASSERT(!stateY.ackReceived, "ackReceived should reflect false on failure");
    TEST_ASSERT(stateY.executionStatus == "FAILED", "Status should be FAILED");

    std::cout << "  ✓ PASS: Positive & negative acknowledgements and latency tracking verified.\n";
}

// =============================================================================
// TEST 4: Heartbeat Monitoring
// =============================================================================
void Test4_HeartbeatMonitoring() {
    std::cout << "[TEST 4] Heartbeat monitoring..." << std::endl;

    DistributedSignalSynchronizer sync;
    sync.RegisterIntersection("INT_HB1", "RED");
    sync.RegisterIntersection("INT_HB2", "RED");

    const uint64_t t1 = 10000;
    bool hb1 = sync.ReceiveHeartbeat("INT_HB1", t1);
    TEST_ASSERT(hb1, "Heartbeat 1 should succeed");

    auto status1 = sync.GetIntersectionStatus("INT_HB1");
    TEST_ASSERT(status1.lastHeartbeat == t1, "Heartbeat timestamp must match t1");
    TEST_ASSERT(status1.isOnline, "Node must be marked online");

    // Monotonic timestamp advancement
    const uint64_t t2 = 15000;
    bool hb2 = sync.ReceiveHeartbeat("INT_HB1", t2);
    TEST_ASSERT(hb2, "Heartbeat update should succeed");

    auto status2 = sync.GetIntersectionStatus("INT_HB1");
    TEST_ASSERT(status2.lastHeartbeat == t2, "Heartbeat timestamp must update to t2");

    // Auto-registration on heartbeat from unannounced node
    bool hbNew = sync.ReceiveHeartbeat("INT_HB_UNANNOUNCED", 12000);
    TEST_ASSERT(hbNew, "Unannounced node heartbeat should succeed and auto-register");
    TEST_ASSERT(sync.HasIntersection("INT_HB_UNANNOUNCED"), "Node must be registered");

    std::cout << "  ✓ PASS: Heartbeat timestamp tracking and auto-registration verified.\n";
}

// =============================================================================
// TEST 5: Retry Failed Command
// =============================================================================
void Test5_RetryFailedCommand() {
    std::cout << "[TEST 5] Retry failed command..." << std::endl;

    DistributedSignalSynchronizer sync;
    sync.RegisterIntersection("INT_RETRY", "RED");

    // Force failure on first attempt
    sync.SetSimulateFailure(true);

    GreenWavePlan plan = BuildSamplePlan("AMB_03", {"INT_RETRY"}, 0.0, 10.0);
    sync.SendExecutionPlan(plan);
    auto initialResults = sync.BroadcastExecution();

    TEST_ASSERT(!initialResults.empty(), "Results must be returned");
    TEST_ASSERT(!initialResults[0].success, "First broadcast should fail");
    auto stateFailed = sync.GetIntersectionStatus("INT_RETRY");
    TEST_ASSERT(stateFailed.executionStatus == "FAILED", "Status should be FAILED");

    // Now restore transmission and trigger retry
    sync.SetSimulateFailure(false);

    auto retried = sync.RetryFailedCommands(3);
    TEST_ASSERT(retried.size() == 1, "Exactly 1 command should be retried");
    TEST_ASSERT(retried[0].retryCount == 1, "retryCount should increment to 1");
    TEST_ASSERT(retried[0].executed, "Command should be successfully executed on retry");

    auto stateRecovered = sync.GetIntersectionStatus("INT_RETRY");
    TEST_ASSERT(stateRecovered.executionStatus == "EXECUTING", "Status should recover to EXECUTING");

    // Acknowledge the retried command
    bool ack = sync.ReceiveAcknowledgement("INT_RETRY", retried[0].commandId, true);
    TEST_ASSERT(ack, "Ack should succeed");

    // Retry again: command is already acknowledged, should NOT be retried
    auto retriedAgain = sync.RetryFailedCommands(3);
    TEST_ASSERT(retriedAgain.empty(), "Acknowledged command should not be retried");

    std::cout << "  ✓ PASS: Command retry logic, count increment, and recovery verified.\n";
}

// =============================================================================
// TEST 6: Offline Node Detection
// =============================================================================
void Test6_OfflineNodeDetection() {
    std::cout << "[TEST 6] Offline node detection..." << std::endl;

    DistributedSignalSynchronizer sync;
    sync.RegisterIntersection("INT_ACTIVE", "RED");
    sync.RegisterIntersection("INT_DEAD", "RED");

    // Send recent heartbeat to INT_ACTIVE
    const uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    sync.ReceiveHeartbeat("INT_ACTIVE", nowMs);

    // Set stale heartbeat for INT_DEAD (6000 ms ago)
    const uint64_t staleTime = (nowMs >= 6000) ? (nowMs - 6000) : 0;
    sync.ReceiveHeartbeat("INT_DEAD", staleTime);

    // Check with 5000 ms timeout
    auto offlineNodes = sync.CheckOfflineNodes(5000);

    TEST_ASSERT(offlineNodes.size() == 1, "Exactly 1 node should be marked offline");
    TEST_ASSERT(offlineNodes[0] == "INT_DEAD", "INT_DEAD should be detected offline");

    auto deadStatus = sync.GetIntersectionStatus("INT_DEAD");
    TEST_ASSERT(!deadStatus.isOnline, "INT_DEAD isOnline must be false");
    TEST_ASSERT(deadStatus.executionStatus == "OFFLINE", "Status must be OFFLINE");

    auto activeStatus = sync.GetIntersectionStatus("INT_ACTIVE");
    TEST_ASSERT(activeStatus.isOnline, "INT_ACTIVE must remain online");
    TEST_ASSERT(activeStatus.executionStatus != "OFFLINE", "INT_ACTIVE status != OFFLINE");

    // Node sends a new heartbeat and comes back online
    sync.ReceiveHeartbeat("INT_DEAD", nowMs + 100);
    auto revivedStatus = sync.GetIntersectionStatus("INT_DEAD");
    TEST_ASSERT(revivedStatus.isOnline, "Revived node must be online");
    TEST_ASSERT(revivedStatus.executionStatus == "IDLE", "Status should recover from OFFLINE");

    std::cout << "  ✓ PASS: Heartbeat timeout detection and online recovery verified.\n";
}

// =============================================================================
// TEST 7: 100 Concurrent Execution Updates
// =============================================================================
void Test7_100ConcurrentExecutionUpdates() {
    std::cout << "[TEST 7] 100 concurrent execution updates (zero race conditions)..." << std::endl;

    DistributedSignalSynchronizer sync;
    const int NUM_THREADS = 100;
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(NUM_THREADS));

    std::atomic<int> readyCount{0};
    std::atomic<bool> startFlag{false};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&sync, i, &readyCount, &startFlag]() {
            std::string intId = "INT_CONC_" + std::to_string(i);
            readyCount.fetch_add(1);

            while (!startFlag.load()) {
                std::this_thread::yield();
            }

            // Register
            sync.RegisterIntersection(intId, "RED");

            // Heartbeat
            sync.ReceiveHeartbeat(intId, 1000ULL + static_cast<uint64_t>(i));

            // Plan & broadcast
            GreenWavePlan plan = BuildSamplePlan("AMB_" + std::to_string(i), {intId}, 0.0, 10.0);
            auto cmds = sync.SendExecutionPlan(plan, "HIGH", 1, "AMB_" + std::to_string(i));
            sync.BroadcastExecution();

            // Acknowledge
            if (!cmds.empty()) {
                sync.ReceiveAcknowledgement(intId, cmds[0].commandId, true);
            }

            // Status query
            auto st = sync.GetIntersectionStatus(intId);
            TEST_ASSERT(st.intersectionId == intId, "Status ID must match");
        });
    }

    // Wait until all threads ready
    while (readyCount.load() < NUM_THREADS) {
        std::this_thread::yield();
    }
    startFlag.store(true);

    for (auto& th : threads) {
        th.join();
    }

    TEST_ASSERT(sync.GetRegisteredIntersectionCount() == static_cast<size_t>(NUM_THREADS),
                "All 100 intersections registered");
    TEST_ASSERT(sync.GetExecutionQueueSize() == static_cast<size_t>(NUM_THREADS),
                "All 100 commands scheduled in queue");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 race conditions.\n";
}

// =============================================================================
// TEST 8: Performance Benchmark (100 Intersections Scheduling < 5 ms)
// =============================================================================
void Test8_PerformanceBenchmark() {
    std::cout << "[TEST 8] Performance benchmark: 100 intersections execution scheduling..." << std::endl;

    DistributedSignalSynchronizer sync;
    const size_t NUM_INTERSECTIONS = 100;
    std::vector<std::string> intersectionIds;
    intersectionIds.reserve(NUM_INTERSECTIONS);

    for (size_t i = 1; i <= NUM_INTERSECTIONS; ++i) {
        std::string id = "INT_BENCH_" + std::to_string(i);
        intersectionIds.push_back(id);
        sync.RegisterIntersection(id, "RED");
    }

    GreenWavePlan massivePlan = BuildSamplePlan(
        "AMB_BENCHMARK", intersectionIds, 0.0, 15.0, 2.0);

    // Warmup
    sync.SendExecutionPlan(massivePlan);
    sync.ClearAll();
    for (const auto& id : intersectionIds) {
        sync.RegisterIntersection(id, "RED");
    }

    // Benchmark SendExecutionPlan + BroadcastExecution for 100 intersections
    auto start = std::chrono::high_resolution_clock::now();

    auto commands = sync.SendExecutionPlan(massivePlan, "CRITICAL", 1, "AMB_BENCHMARK");
    auto results = sync.BroadcastExecution();

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> Generated " << commands.size() << " commands across "
              << results.size() << " intersections in: "
              << std::fixed << std::setprecision(3) << elapsedMs << " ms\n";

    TEST_ASSERT(commands.size() == NUM_INTERSECTIONS, "Expected 100 commands generated");
    TEST_ASSERT(results.size() == NUM_INTERSECTIONS, "Expected 100 broadcast results");
    TEST_ASSERT(elapsedMs < 5.0, "Scheduling and broadcast of 100 intersections must be < 5.0 ms");

    std::cout << "  ✓ PASS: 100 intersections execution scheduling completed in "
              << elapsedMs << " ms (< 5.0 ms target achieved).\n";
}

} // namespace

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 10A Distributed Signal Synchronizer" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_RegisterIntersections();
    Test2_BroadcastExecutionCommands();
    Test3_ReceiveAcknowledgements();
    Test4_HeartbeatMonitoring();
    Test5_RetryFailedCommand();
    Test6_OfflineNodeDetection();
    Test7_100ConcurrentExecutionUpdates();
    Test8_PerformanceBenchmark();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 8 PHASE 10A UNIT TESTS PASSED SUCCESSFULLY (100%)" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
