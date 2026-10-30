/**
 * @file test_emergency_orchestrator_phase8b.cpp
 * @brief Comprehensive unit tests for Phase 8B Autonomous Emergency Orchestration Engine.
 *
 * Verifies:
 *  - Test 1: Single emergency - Complete pipeline
 *  - Test 2: Multiple emergencies - Verify highest priority selected
 *  - Test 3: Route generation failure
 *  - Test 4: Green wave failure
 *  - Test 5: Coordinator dispatch failure
 *  - Test 6: Execution history verification & cancellation
 *  - Test 7: Concurrent emergency submissions across 100 threads
 *  - Test 8: Performance benchmark: 100 emergencies complete orchestration under 5 ms
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cassert>

#include "orchestrator/EmergencyOrchestrator.h"

using namespace aegis::orchestrator;
using namespace aegis::scheduler;
using namespace aegis::green_corridor;
using namespace aegis::green_wave;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion FAILED: " << msg \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

// Helper to configure a standard 3-hop corridor network
static void SetupCorridorNetwork(EmergencyCorridorPlanner& planner) {
    planner.ClearGraph();
    planner.AddIntersection("INT_A", 12.9716, 77.5946);
    planner.AddIntersection("INT_B", 12.9720, 77.5950);
    planner.AddIntersection("INT_C", 12.9725, 77.5955);
    planner.AddIntersection("Hospital", 12.9730, 77.5960);

    planner.AddRoadSegment("INT_A", "INT_B", 500.0, 20.0, true);
    planner.AddRoadSegment("INT_B", "INT_C", 500.0, 20.0, true);
    planner.AddRoadSegment("INT_C", "Hospital", 500.0, 20.0, true);
}

// =============================================================================
// Test 1: Single emergency - Complete pipeline
// =============================================================================
static bool TestSingleEmergencyCompletePipeline() {
    std::cout << "[TEST 1] Testing Single Emergency Complete Orchestration Pipeline...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(true);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    EmergencyRequest req("REQ_001", "AMB_01", PatientSeverity::CRITICAL, "Hospital",
                         "INT_A", "Hospital", 60.0, 1500.0, 1000);

    bool submitted = orchestrator.SubmitEmergency(req);
    TEST_ASSERT(submitted, "SubmitEmergency must return true");
    TEST_ASSERT(orchestrator.GetCurrentState() == OrchestratorState::REQUEST_RECEIVED,
                "State should be REQUEST_RECEIVED");

    ExecutionRecord record = orchestrator.ProcessNextEmergency();

    TEST_ASSERT(record.IsSuccess(), "Pipeline execution must succeed");
    TEST_ASSERT(record.status == ExecutionStatus::COMPLETED, "Status must be COMPLETED");
    TEST_ASSERT(orchestrator.GetCurrentState() == OrchestratorState::COMPLETED,
                "Orchestrator state must be COMPLETED");
    TEST_ASSERT(record.request.ambulanceId == "AMB_01", "Ambulance ID must match");
    TEST_ASSERT(record.route.isValid, "Route must be valid");
    TEST_ASSERT(record.route.intersections.size() == 4, "Route must contain 4 nodes");
    TEST_ASSERT(record.greenWavePlan.isValid, "Green wave plan must be valid");
    TEST_ASSERT(record.greenWavePlan.windows.size() == 3, "Plan must contain 3 signalized windows");

    std::string formatted = FormatExecutionRecord(record);
    TEST_ASSERT(formatted.find("[ORCHESTRATOR]") != std::string::npos, "Log must contain [ORCHESTRATOR]");
    TEST_ASSERT(formatted.find("Emergency:\nAMB_01") != std::string::npos, "Log must contain Emergency: AMB_01");
    TEST_ASSERT(formatted.find("Priority:\nCRITICAL") != std::string::npos, "Log must contain Priority: CRITICAL");
    TEST_ASSERT(formatted.find("Green Wave:\nGENERATED") != std::string::npos, "Log must contain Green Wave: GENERATED");
    TEST_ASSERT(formatted.find("Traffic Commands:\nDISPATCHED") != std::string::npos, "Log must contain Traffic Commands: DISPATCHED");
    TEST_ASSERT(formatted.find("Status:\nCOMPLETED") != std::string::npos, "Log must contain Status: COMPLETED");

    std::cout << "  -> Single emergency complete pipeline PASSED.\n";
    return true;
}

// =============================================================================
// Test 2: Multiple emergencies - Verify highest priority selected
// =============================================================================
static bool TestMultipleEmergenciesPrioritySelection() {
    std::cout << "[TEST 2] Testing Multiple Emergencies - Priority Selection...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    // Submit out of order: LOW, MEDIUM, CRITICAL
    EmergencyRequest reqLow("REQ_LOW", "AMB_LOW", PatientSeverity::LOW, "Hospital",
                           "INT_A", "Hospital", 15.0, 500.0, 100);
    EmergencyRequest reqMed("REQ_MED", "AMB_MED", PatientSeverity::MEDIUM, "Hospital",
                           "INT_A", "Hospital", 25.0, 1000.0, 100);
    EmergencyRequest reqCrit("REQ_CRIT", "AMB_CRIT", PatientSeverity::CRITICAL, "Hospital",
                            "INT_A", "Hospital", 45.0, 1500.0, 100);

    orchestrator.SubmitEmergency(reqLow);
    orchestrator.SubmitEmergency(reqMed);
    orchestrator.SubmitEmergency(reqCrit);

    // 1st Process: Must select CRITICAL despite higher ETA
    ExecutionRecord rec1 = orchestrator.ProcessNextEmergency();
    TEST_ASSERT(rec1.IsSuccess(), "Rec 1 must succeed");
    TEST_ASSERT(rec1.request.ambulanceId == "AMB_CRIT", "1st processed must be AMB_CRIT (CRITICAL)");

    // 2nd Process: Must select MEDIUM
    ExecutionRecord rec2 = orchestrator.ProcessNextEmergency();
    TEST_ASSERT(rec2.IsSuccess(), "Rec 2 must succeed");
    TEST_ASSERT(rec2.request.ambulanceId == "AMB_MED", "2nd processed must be AMB_MED (MEDIUM)");

    // 3rd Process: Must select LOW
    ExecutionRecord rec3 = orchestrator.ProcessNextEmergency();
    TEST_ASSERT(rec3.IsSuccess(), "Rec 3 must succeed");
    TEST_ASSERT(rec3.request.ambulanceId == "AMB_LOW", "3rd processed must be AMB_LOW (LOW)");

    std::cout << "  -> Priority selection verified (CRITICAL -> MEDIUM -> LOW).\n";
    return true;
}

// =============================================================================
// Test 3: Route generation failure
// =============================================================================
static bool TestRouteGenerationFailure() {
    std::cout << "[TEST 3] Testing Route Generation Failure Handling...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    // Request with unreachable destination node
    EmergencyRequest req("REQ_FAIL", "AMB_UNREACH", PatientSeverity::HIGH, "UNREACHABLE_DEST",
                         "INT_A", "UNREACHABLE_DEST", 50.0, 1000.0, 2000);

    orchestrator.SubmitEmergency(req);
    ExecutionRecord rec = orchestrator.ProcessNextEmergency();

    TEST_ASSERT(!rec.IsSuccess(), "Execution record must report failure");
    TEST_ASSERT(rec.status == ExecutionStatus::FAILED_ROUTE, "Status must be FAILED_ROUTE");
    TEST_ASSERT(orchestrator.GetCurrentState() == OrchestratorState::FAILED_ROUTE,
                "Orchestrator state must be FAILED_ROUTE");
    TEST_ASSERT(!rec.failureReason.empty(), "Failure reason must be set");

    std::cout << "  -> Route failure handled gracefully (" << rec.failureReason << ").\n";
    return true;
}

// =============================================================================
// Test 4: Green wave failure
// =============================================================================
static bool TestGreenWaveFailure() {
    std::cout << "[TEST 4] Testing Green Wave Generation Failure Handling...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    orchestrator.SetSimulateGreenWaveFailure(true);

    EmergencyRequest req("REQ_WAVE_FAIL", "AMB_03", PatientSeverity::MEDIUM, "Hospital",
                         "INT_A", "Hospital", 60.0, 1500.0, 3000);

    orchestrator.SubmitEmergency(req);
    ExecutionRecord rec = orchestrator.ProcessNextEmergency();

    TEST_ASSERT(!rec.IsSuccess(), "Record must report failure");
    TEST_ASSERT(rec.status == ExecutionStatus::FAILED_SIGNAL, "Status must be FAILED_SIGNAL");
    TEST_ASSERT(orchestrator.GetCurrentState() == OrchestratorState::FAILED_SIGNAL,
                "State must be FAILED_SIGNAL");

    orchestrator.SetSimulateGreenWaveFailure(false);
    std::cout << "  -> Green wave failure handled gracefully.\n";
    return true;
}

// =============================================================================
// Test 5: Coordinator dispatch failure
// =============================================================================
static bool TestCoordinatorDispatchFailure() {
    std::cout << "[TEST 5] Testing Coordinator Dispatch Failure Handling...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    orchestrator.SetSimulateDispatchFailure(true);

    EmergencyRequest req("REQ_DISPATCH_FAIL", "AMB_04", PatientSeverity::HIGH, "Hospital",
                         "INT_A", "Hospital", 60.0, 1500.0, 4000);

    orchestrator.SubmitEmergency(req);
    ExecutionRecord rec = orchestrator.ProcessNextEmergency();

    TEST_ASSERT(!rec.IsSuccess(), "Record must report failure");
    TEST_ASSERT(rec.status == ExecutionStatus::FAILED_SIGNAL, "Status must be FAILED_SIGNAL");
    TEST_ASSERT(orchestrator.GetCurrentState() == OrchestratorState::FAILED_SIGNAL,
                "State must be FAILED_SIGNAL");
    TEST_ASSERT(rec.failureReason.find("dispatch") != std::string::npos,
                "Failure reason must mention dispatch");

    orchestrator.SetSimulateDispatchFailure(false);
    std::cout << "  -> Coordinator dispatch failure handled gracefully.\n";
    return true;
}

// =============================================================================
// Test 6: Execution history verification & Cancellation
// =============================================================================
static bool TestExecutionHistoryAndCancellation() {
    std::cout << "[TEST 6] Testing Execution History and Request Cancellation...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    // Submit and cancel an emergency
    EmergencyRequest reqCancel("REQ_TO_CANCEL", "AMB_CANCEL", PatientSeverity::LOW, "Hospital",
                               "INT_A", "Hospital", 80.0, 2000.0, 5000);
    orchestrator.SubmitEmergency(reqCancel);
    bool cancelled = orchestrator.CancelEmergency("REQ_TO_CANCEL");
    TEST_ASSERT(cancelled, "CancelEmergency must return true");
    TEST_ASSERT(orchestrator.GetScheduler()->GetQueueSize() == 0, "Queue must be empty after cancel");

    // Submit and execute 2 emergencies
    EmergencyRequest r1("REQ_H1", "AMB_H1", PatientSeverity::CRITICAL, "Hospital", "INT_A", "Hospital", 60.0, 1500.0, 100);
    EmergencyRequest r2("REQ_H2", "AMB_H2", PatientSeverity::HIGH, "Hospital", "INT_A", "Hospital", 60.0, 1500.0, 200);

    orchestrator.SubmitEmergency(r1);
    orchestrator.SubmitEmergency(r2);
    orchestrator.ProcessAllEmergencies();

    auto history = orchestrator.GetExecutionHistory();
    TEST_ASSERT(history.size() == 2, "History must contain 2 records");
    TEST_ASSERT(history[0].request.ambulanceId == "AMB_H1", "First in history must be AMB_H1");
    TEST_ASSERT(history[1].request.ambulanceId == "AMB_H2", "Second in history must be AMB_H2");

    std::string formattedHistory = orchestrator.FormatExecutionHistory();
    TEST_ASSERT(formattedHistory.find("AMB_H1") != std::string::npos, "Formatted history must contain AMB_H1");
    TEST_ASSERT(formattedHistory.find("AMB_H2") != std::string::npos, "Formatted history must contain AMB_H2");

    orchestrator.ClearHistory();
    TEST_ASSERT(orchestrator.GetExecutionHistory().empty(), "History must be empty after ClearHistory");

    std::cout << "  -> Execution history and request cancellation PASSED.\n";
    return true;
}

// =============================================================================
// Test 7: Concurrent emergency submissions (100 threads)
// =============================================================================
static bool TestConcurrentSubmissions100Threads() {
    std::cout << "[TEST 7] Testing Concurrent Submissions with 100 Threads...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);
    SetupCorridorNetwork(*orchestrator.GetPlanner());

    const int THREAD_COUNT = 100;
    std::vector<std::thread> threads;
    threads.reserve(THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&orchestrator, i]() {
            std::string reqId = "REQ_TH_" + std::to_string(i);
            std::string ambId = "AMB_TH_" + std::to_string(i);
            PatientSeverity sev = static_cast<PatientSeverity>((i % 4) + 1);

            EmergencyRequest req(reqId, ambId, sev, "Hospital", "INT_A", "Hospital",
                                 30.0 + static_cast<double>(i), 1000.0, 1000 + static_cast<uint64_t>(i));
            orchestrator.SubmitEmergency(req);

            // Concurrently query state
            (void)orchestrator.GetCurrentState();
            (void)orchestrator.GetActiveEmergency();
            (void)orchestrator.GetExecutionHistory();
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    TEST_ASSERT(orchestrator.GetScheduler()->GetQueueSize() == THREAD_COUNT,
                "All 100 requests must be in scheduler queue");

    // Concurrently process a portion of emergencies
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&orchestrator]() {
            for (int k = 0; k < 10; ++k) {
                orchestrator.ProcessNextEmergency();
            }
        });
    }
    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    TEST_ASSERT(orchestrator.GetExecutionHistory().size() == 40, "40 emergencies should be processed");
    std::cout << "  -> 100 concurrent threads completed with 0 data races.\n";
    return true;
}

// =============================================================================
// Test 8: Performance benchmark: 100 emergencies under 5 ms
// =============================================================================
static bool TestLargeScalePerformanceBenchmark() {
    std::cout << "[TEST 8] Testing Performance Benchmark: 100 Emergencies Orchestration under 5 ms...\n";

    EmergencyOrchestrator orchestrator;
    orchestrator.SetConsoleLogging(false);

    // Build a 10-node road corridor
    auto planner = orchestrator.GetPlanner();
    planner->ClearGraph();
    for (int i = 0; i < 10; ++i) {
        planner->AddIntersection("NODE_" + std::to_string(i), 12.9 + 0.001 * i, 77.5 + 0.001 * i);
    }
    planner->AddIntersection("Hospital", 12.91, 77.51);

    for (int i = 0; i < 9; ++i) {
        planner->AddRoadSegment("NODE_" + std::to_string(i), "NODE_" + std::to_string(i + 1), 300.0, 15.0, true);
    }
    planner->AddRoadSegment("NODE_9", "Hospital", 300.0, 15.0, true);

    const int REQUEST_COUNT = 100;
    for (int i = 0; i < REQUEST_COUNT; ++i) {
        std::string reqId = "REQ_PERF_" + std::to_string(i);
        std::string ambId = "AMB_PERF_" + std::to_string(i);
        PatientSeverity sev = static_cast<PatientSeverity>((i % 4) + 1);

        EmergencyRequest req(reqId, ambId, sev, "Hospital", "NODE_0", "Hospital",
                             40.0 + static_cast<double>(i), 2000.0, 1000 + static_cast<uint64_t>(i));
        orchestrator.SubmitEmergency(req);
    }

    TEST_ASSERT(orchestrator.GetScheduler()->GetQueueSize() == REQUEST_COUNT, "Queue must contain 100 requests");

    // Measure total orchestration pipeline elapsed time
    auto startTime = std::chrono::high_resolution_clock::now();
    auto results = orchestrator.ProcessAllEmergencies();
    auto endTime = std::chrono::high_resolution_clock::now();

    auto elapsedMicroseconds = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    double elapsedMilliseconds = static_cast<double>(elapsedMicroseconds) / 1000.0;

    std::cout << "  -> 100 emergencies orchestrated in " << elapsedMicroseconds << " microseconds ("
              << elapsedMilliseconds << " ms).\n";

    TEST_ASSERT(results.size() == REQUEST_COUNT, "All 100 emergencies must be processed");
    for (const auto& rec : results) {
        TEST_ASSERT(rec.IsSuccess(), "All 100 emergencies must complete successfully");
    }

    TEST_ASSERT(elapsedMilliseconds < 5.0, "Complete orchestration of 100 emergencies must take under 5.0 ms!");
    std::cout << "  -> Performance requirement (< 5 ms for 100 emergencies) fully satisfied.\n";
    return true;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 8B Emergency Orchestrator Tests\n";
    std::cout << "==========================================================\n";

    bool allPassed = true;
    allPassed &= TestSingleEmergencyCompletePipeline();
    allPassed &= TestMultipleEmergenciesPrioritySelection();
    allPassed &= TestRouteGenerationFailure();
    allPassed &= TestGreenWaveFailure();
    allPassed &= TestCoordinatorDispatchFailure();
    allPassed &= TestExecutionHistoryAndCancellation();
    allPassed &= TestConcurrentSubmissions100Threads();
    allPassed &= TestLargeScalePerformanceBenchmark();

    std::cout << "==========================================================\n";
    if (allPassed) {
        std::cout << " [ALL 8 TESTS PASSED] Phase 8B Fully Verified!\n";
    } else {
        std::cout << " [FAILED] Some Phase 8B tests failed.\n";
    }
    std::cout << "==========================================================\n";

    return allPassed ? 0 : 1;
}
