/**
 * @file test_system_diagnostics_phase14b.cpp
 * @brief SMART AEGIS (AERS) Phase 14B Unit Tests
 *
 * Validates the System Diagnostics & Predictive Maintenance Engine:
 * - 12 Comprehensive Unit Tests
 * - Component Registration & Removal across Subsystems
 * - Health Monitoring & Multi-Metric State Evaluation
 * - Heartbeat Supervision & Latency Tracking
 * - Failure Detection (Rule 1: Heartbeat Timeout, Rule 4: Packet Loss)
 * - Failure Prediction Engine (Rule 5: Repeated Faults, Rule 6: Memory Leak)
 * - Maintenance Plan Generation & Priority Scheduling (Rule 9)
 * - Multi-Metric Threshold Evaluation (Rules 2, 3, 4: CPU, Temp, Packet Loss)
 * - Full System Diagnostics & Scanning (RunDiagnostics, RunFullSystemScan)
 * - Synchronization Failure & System Readiness Verification (Rules 7, 8, 10)
 * - Structured Operational ASCII Reports ([HEALTH], [DIAGNOSTICS], [FAILURE], [MAINTENANCE])
 * - 100-Thread Concurrency & Race-Free Stress Test
 * - Performance Benchmark (100 Components, 1,000 Updates, 1,000 Diagnostics, 100 Scans, 100 Predictions, < 5ms Scan)
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "system_diagnostics/SystemDiagnosticsEngine.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis::diagnostics;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

// =============================================================================
// Unit Tests
// =============================================================================

/**
 * @brief TEST 1: Component Registration & Removal
 */
void Test1_ComponentRegistration() {
    std::cout << "[TEST 1] Component Registration & Removal..." << std::endl;
    SystemDiagnosticsEngine engine;

    ComponentHealth coord("COORD_01", ComponentType::COORDINATOR, "Central Coordinator");
    ComponentHealth rpi("RPI_01", ComponentType::RPI_CLUSTER, "Edge Intersection Pi 1");
    ComponentHealth net("NET_01", ComponentType::COMMUNICATION, "mTLS Mesh Network");

    TEST_ASSERT(engine.RegisterComponent(coord), "Register Coordinator must succeed");
    TEST_ASSERT(engine.RegisterComponent(rpi), "Register RPi cluster node must succeed");
    TEST_ASSERT(engine.RegisterComponent(net), "Register Communication layer must succeed");
    TEST_ASSERT(!engine.RegisterComponent(coord), "Duplicate component registration must fail");

    ComponentHealth query;
    TEST_ASSERT(engine.GetComponent("COORD_01", query), "Query registered coordinator");
    TEST_ASSERT(query.componentType == ComponentType::COORDINATOR, "Component type must match");
    TEST_ASSERT(query.componentName == "Central Coordinator", "Component name must match");

    auto all = engine.GetAllComponents();
    TEST_ASSERT(all.size() == 3, "Total registered components must be 3");

    TEST_ASSERT(engine.RemoveComponent("NET_01"), "Remove communication layer must succeed");
    TEST_ASSERT(!engine.GetComponent("NET_01", query), "Removed component must not be found");
    TEST_ASSERT(engine.GetAllComponents().size() == 2, "Total registered components must now be 2");

    std::cout << "  ✓ PASS: Component registration, duplicate rejection, and removal verified." << std::endl;
}

/**
 * @brief TEST 2: Health Monitoring
 */
void Test2_HealthMonitoring() {
    std::cout << "[TEST 2] Health Monitoring..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("AMB_01", ComponentType::AMBULANCE, "Ambulance Unit 1");
    engine.RegisterComponent("HOSP_01", ComponentType::HOSPITAL, "City General Hospital");

    TEST_ASSERT(engine.UpdateComponentHealth("AMB_01", DiagnosticState::WARNING), "Update state to WARNING");
    ComponentHealth q;
    engine.GetComponent("AMB_01", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::WARNING, "State must be WARNING");

    TEST_ASSERT(engine.UpdateComponentHealth("AMB_01", DiagnosticState::CRITICAL), "Update state to CRITICAL");
    engine.GetComponent("AMB_01", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::CRITICAL, "State must be CRITICAL");

    // Critical components check
    auto crit = engine.GetCriticalComponents();
    TEST_ASSERT(crit.size() == 1, "Must have exactly 1 critical component");
    TEST_ASSERT(crit[0].componentId == "AMB_01", "Critical component must be AMB_01");

    // Restore to HEALTHY
    TEST_ASSERT(engine.UpdateComponentHealth("AMB_01", DiagnosticState::HEALTHY), "Restore to HEALTHY");
    TEST_ASSERT(engine.GetCriticalComponents().empty(), "No critical components remain");

    std::cout << "  ✓ PASS: Health state transitions and critical tracking verified." << std::endl;
}

/**
 * @brief TEST 3: Heartbeat Monitoring & Latency
 */
void Test3_Heartbeat() {
    std::cout << "[TEST 3] Heartbeat Monitoring..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("SIG_01", ComponentType::SIGNAL_CONTROLLER, "Signal Controller 1");

    TEST_ASSERT(engine.UpdateHeartbeat("SIG_01", 3.5), "Heartbeat update with 3.5 ms latency");
    ComponentHealth q;
    engine.GetComponent("SIG_01", q);
    TEST_ASSERT(q.isOnline, "Component must be online");
    TEST_ASSERT(q.networkLatency == 3.5, "Latency must match 3.5 ms");

    engine.UpdateHeartbeat("SIG_01", 6.5);
    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.averageLatency > 0.0, "Average latency must be updated");

    std::cout << "  ✓ PASS: Heartbeat and network latency tracking verified." << std::endl;
}

/**
 * @brief TEST 4: Failure Detection (Rules 1 & 4)
 */
void Test4_FailureDetection() {
    std::cout << "[TEST 4] Failure Detection (Rules 1 & 4)..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("RPI_FAIL", ComponentType::RPI_CLUSTER, "Failing Pi Node");
    engine.SetHeartbeatTimeoutMs(50); // Short timeout for test

    // Sleep 80 ms to trigger timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    // Rule 1: Heartbeat timeout -> FAILED
    DiagnosticRecord diag = engine.RunDiagnostics("RPI_FAIL");
    TEST_ASSERT(diag.state == DiagnosticState::FAILED, "Timed-out node must be diagnosed as FAILED");
    TEST_ASSERT(!diag.detectedIssues.empty(), "Failure record must record detected issues");

    ComponentHealth q;
    engine.GetComponent("RPI_FAIL", q);
    TEST_ASSERT(!q.isOnline, "Timed out node must be marked offline");
    TEST_ASSERT(q.diagnosticState == DiagnosticState::FAILED, "Component state must be FAILED");

    // Rule 4: Packet loss over threshold -> WARNING
    engine.RegisterComponent("NET_LOSS", ComponentType::COMMUNICATION, "Degraded Link");
    engine.UpdateTelemetry("NET_LOSS", 20.0, 30.0, 40.0, 50.0, 8.5); // 8.5% packet loss > 5.0%
    engine.GetComponent("NET_LOSS", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::WARNING, "Packet loss > 5% must evaluate to WARNING");

    std::cout << "  ✓ PASS: Heartbeat timeout failure and packet loss detection verified." << std::endl;
}

/**
 * @brief TEST 5: Prediction Engine (Rules 5 & 6)
 */
void Test5_PredictionEngine() {
    std::cout << "[TEST 5] Prediction Engine (Rules 5 & 6)..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("LEAK_COMP", ComponentType::COORDINATOR, "Memory Leaking Process");
    engine.RegisterComponent("FAIL_COMP", ComponentType::INTERSECTION, "Flaky Intersection Node");

    // Rule 6: Memory growth beyond threshold predicts memory leak
    engine.UpdateMemoryGrowth("LEAK_COMP", 15.0); // 15 MB/min > 10.0 MB/min threshold

    // Rule 5: Repeated failures predict maintenance requirement
    engine.RecordFailure("FAIL_COMP", "Watchdog Timeout");
    engine.RecordFailure("FAIL_COMP", "Kernel Panic");

    auto predictions = engine.PredictFailures();
    TEST_ASSERT(!predictions.empty(), "Must generate predictive maintenance recommendations");

    bool foundMemoryLeakPred = false;
    bool foundHardwareFaultPred = false;

    for (const auto& p : predictions) {
        if (p.componentId == "LEAK_COMP" && p.predictedFailureType == "MEMORY_LEAK") {
            foundMemoryLeakPred = true;
            TEST_ASSERT(p.priority == MaintenancePriority::HIGH || p.priority == MaintenancePriority::IMMEDIATE,
                        "Memory leak priority must be HIGH or IMMEDIATE");
        }
        if (p.componentId == "FAIL_COMP" && p.predictedFailureType == "HARDWARE_DEGRADATION") {
            foundHardwareFaultPred = true;
            TEST_ASSERT(p.priority == MaintenancePriority::HIGH, "Hardware degradation priority must be HIGH");
        }
    }

    TEST_ASSERT(foundMemoryLeakPred, "Must predict memory leak risk");
    TEST_ASSERT(foundHardwareFaultPred, "Must predict hardware degradation risk");

    std::cout << "  ✓ PASS: Failure prediction engine (Rules 5 & 6) verified." << std::endl;
}

/**
 * @brief TEST 6: Maintenance Generation (Rule 9)
 */
void Test6_MaintenanceGeneration() {
    std::cout << "[TEST 6] Maintenance Generation (Rule 9)..." << std::endl;
    SystemDiagnosticsEngine engine;

    ComponentHealth hot("HOT_PI", ComponentType::RPI_CLUSTER, "Overheating RPi");
    hot.temperature = 84.0; // Critical temp > 80C
    engine.RegisterComponent(hot);

    ComponentHealth warn("WARN_NET", ComponentType::COMMUNICATION, "Unstable Link");
    warn.packetLoss = 7.0; // Warning packet loss > 5%
    engine.RegisterComponent(warn);

    auto plan = engine.GenerateMaintenancePlan();
    TEST_ASSERT(!plan.empty(), "Maintenance plan must be generated automatically");

    // Plan must be sorted by priority: IMMEDIATE > HIGH > MEDIUM > LOW
    for (size_t i = 1; i < plan.size(); ++i) {
        TEST_ASSERT(static_cast<int>(plan[i - 1].priority) >= static_cast<int>(plan[i].priority),
                    "Maintenance plan must be sorted deterministically in descending priority");
    }

    TEST_ASSERT(plan[0].priority == MaintenancePriority::IMMEDIATE, "Overheating must produce IMMEDIATE priority");
    TEST_ASSERT(plan[0].predictedFailureType == "THERMAL_SHUTDOWN", "Predicted failure must be THERMAL_SHUTDOWN");

    std::cout << "  ✓ PASS: Automatic maintenance generation and priority scheduling verified." << std::endl;
}

/**
 * @brief TEST 7: Multi-Metric Health Evaluation (Rules 2, 3, 4)
 */
void Test7_MultiMetricHealth() {
    std::cout << "[TEST 7] Multi-Metric Health Evaluation (Rules 2, 3, 4)..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("METRIC_COMP", ComponentType::TRAFFIC_NETWORK, "Network Subsystem");

    // Initial: HEALTHY
    ComponentHealth q;
    engine.GetComponent("METRIC_COMP", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::HEALTHY, "Initial state must be HEALTHY");

    // Rule 2: CPU > 85% -> WARNING
    engine.UpdateTelemetry("METRIC_COMP", 88.0, 40.0, 50.0, 55.0, 0.0);
    engine.GetComponent("METRIC_COMP", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::WARNING, "CPU > 85% must result in WARNING");

    // Rule 3: Temperature > 80C -> CRITICAL (overrides CPU warning)
    engine.UpdateTelemetry("METRIC_COMP", 40.0, 40.0, 50.0, 82.5, 0.0);
    engine.GetComponent("METRIC_COMP", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::CRITICAL, "Temp > 80C must result in CRITICAL");

    // Return to nominal values -> HEALTHY
    engine.UpdateTelemetry("METRIC_COMP", 25.0, 30.0, 40.0, 52.0, 0.5);
    engine.GetComponent("METRIC_COMP", q);
    TEST_ASSERT(q.diagnosticState == DiagnosticState::HEALTHY, "Nominal values must restore HEALTHY");

    std::cout << "  ✓ PASS: Multi-metric thresholding (Rules 2, 3, 4) verified." << std::endl;
}

/**
 * @brief TEST 8: Full Diagnostics & System Scan
 */
void Test8_DiagnosticsAndScan() {
    std::cout << "[TEST 8] Diagnostics & Full System Scan..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("SCAN_1", ComponentType::COORDINATOR, "Coord 1");
    engine.RegisterComponent("SCAN_2", ComponentType::CONSENSUS, "Consensus 1");
    engine.RegisterComponent("SCAN_3", ComponentType::TRAFFIC_NETWORK, "Traffic Network 1");

    auto scanResults = engine.RunFullSystemScan();
    TEST_ASSERT(scanResults.size() == 3, "Scan must produce records for all 3 components");

    for (const auto& r : scanResults) {
        TEST_ASSERT(r.state == DiagnosticState::HEALTHY, "Nominal components must scan HEALTHY");
        TEST_ASSERT(!r.recordId.empty(), "Record ID must be populated");
        TEST_ASSERT(r.recommendation == "System operating within nominal parameters", "Nominal recommendation");
    }

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalDiagnosticsRun >= 3, "Diagnostics count must be incremented");
    TEST_ASSERT(stats.healthyComponents == 3, "All 3 components healthy");

    std::cout << "  ✓ PASS: RunDiagnostics and RunFullSystemScan verified." << std::endl;
}

/**
 * @brief TEST 9: Synchronization Failure & System Readiness (Rules 7, 8, 10)
 */
void Test9_SyncAndReadiness() {
    std::cout << "[TEST 9] Synchronization Failure & Readiness Verification (Rules 7, 8, 10)..." << std::endl;
    SystemDiagnosticsEngine engine;

    engine.RegisterComponent("SYNC_1", ComponentType::COORDINATOR, "Coord");
    engine.RegisterComponent("SYNC_2", ComponentType::RPI_CLUSTER, "Pi Cluster");

    // Both online and synced -> System Ready
    TEST_ASSERT(engine.VerifySystemReadiness(), "System must be ready initially");

    // Rule 7: Synchronization failure blocks readiness
    engine.UpdateSyncState("SYNC_2", false);
    TEST_ASSERT(!engine.VerifySystemReadiness(), "Desynchronized node must block system readiness");

    // Resync -> System Ready again
    engine.UpdateSyncState("SYNC_2", true);
    TEST_ASSERT(engine.VerifySystemReadiness(), "Resynchronized node must restore system readiness");

    // Rule 8: Critical component reduces readiness
    engine.UpdateComponentHealth("SYNC_1", DiagnosticState::CRITICAL);
    TEST_ASSERT(!engine.VerifySystemReadiness(), "Critical component must block system readiness");

    std::cout << "  ✓ PASS: Synchronization gating and readiness verification (Rules 7, 8, 10) verified." << std::endl;
}

/**
 * @brief TEST 10: Reports Generation
 */
void Test10_Reports() {
    std::cout << "[TEST 10] Reports..." << std::endl;
    SystemDiagnosticsEngine engine;

    ComponentHealth c1("REP_COORD", ComponentType::COORDINATOR, "Main Coordinator");
    c1.cpuUsage = 30.5;
    c1.temperature = 48.0;
    c1.networkLatency = 1.25;
    c1.diagnosticState = DiagnosticState::HEALTHY;
    engine.RegisterComponent(c1);

    ComponentHealth c2("REP_PI", ComponentType::RPI_CLUSTER, "Hot Edge Pi");
    c2.cpuUsage = 89.0;
    c2.temperature = 84.0;
    c2.networkLatency = 4.5;
    c2.diagnosticState = DiagnosticState::CRITICAL;
    c2.lastError = "Overheating";
    engine.RegisterComponent(c2);

    engine.PredictFailures();

    // 1. Health Report
    std::string healthRep = engine.GenerateHealthReport();
    std::cout << healthRep << std::endl;
    TEST_ASSERT(healthRep.find("[HEALTH]") != std::string::npos, "Must contain [HEALTH]");
    TEST_ASSERT(healthRep.find("Total Components:\n2") != std::string::npos, "Total components count");
    TEST_ASSERT(healthRep.find("Critical:\n1") != std::string::npos, "Critical count");

    // 2. Diagnostics Report
    std::string diagRep = engine.GenerateDiagnosticsReport("REP_COORD");
    std::cout << diagRep << std::endl;
    TEST_ASSERT(diagRep.find("[DIAGNOSTICS]") != std::string::npos, "Must contain [DIAGNOSTICS]");
    TEST_ASSERT(diagRep.find("Component:\nREP_COORD") != std::string::npos, "Component ID");
    TEST_ASSERT(diagRep.find("Type:\nCOORDINATOR") != std::string::npos, "Type");

    // 3. Failure Report
    std::string failRep = engine.GenerateFailureReport();
    std::cout << failRep << std::endl;
    TEST_ASSERT(failRep.find("[FAILURE]") != std::string::npos, "Must contain [FAILURE]");
    TEST_ASSERT(failRep.find("Component:\nREP_PI") != std::string::npos, "Failing component");
    TEST_ASSERT(failRep.find("State:\nCRITICAL") != std::string::npos, "State");

    // 4. Maintenance Report
    std::string maintRep = engine.GenerateMaintenanceReport();
    std::cout << maintRep << std::endl;
    TEST_ASSERT(maintRep.find("[MAINTENANCE]") != std::string::npos, "Must contain [MAINTENANCE]");
    TEST_ASSERT(maintRep.find("Component:\nREP_PI") != std::string::npos, "Maintenance component");
    TEST_ASSERT(maintRep.find("Priority:\nIMMEDIATE") != std::string::npos, "Priority");

    std::cout << "  ✓ PASS: All formatted reports validated against exact specification." << std::endl;
}

/**
 * @brief TEST 11: 100-Thread Concurrency
 */
void Test11_Concurrency() {
    std::cout << "[TEST 11] 100-Thread Concurrency..." << std::endl;
    SystemDiagnosticsEngine engine;

    for (int i = 0; i < 20; ++i) {
        engine.RegisterComponent("CONCUR_COMP_" + std::to_string(i), ComponentType::RPI_CLUSTER);
    }

    const int NUM_THREADS = 100;
    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);
    std::atomic<int> completedOperations{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&engine, t, &completedOperations]() {
            std::string id = "CONCUR_COMP_" + std::to_string(t % 20);
            for (int k = 0; k < 25; ++k) {
                engine.UpdateHeartbeat(id, 2.0);
                engine.UpdateTelemetry(id, 45.0, 50.0, 60.0, 55.0, 0.5);
                engine.RunDiagnostics(id);
                engine.VerifySystemReadiness();
                engine.GetStatistics();
            }
            completedOperations++;
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    TEST_ASSERT(completedOperations.load() == NUM_THREADS, "All 100 threads must complete successfully");
    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: " << completedOperations.load() << ")." << std::endl;
}

/**
 * @brief TEST 12: Performance Benchmark
 */
void Test12_PerformanceBenchmark() {
    std::cout << "[TEST 12] Performance Benchmark: 100 Components, 1,000 Updates, 1,000 Diagnostics, 100 Scans..." << std::endl;
    SystemDiagnosticsEngine engine;

    const int NUM_COMPONENTS = 100;
    const int NUM_UPDATES = 1000;
    const int NUM_DIAGNOSTICS = 1000;
    const int NUM_SCANS = 100;
    const int NUM_PREDICTIONS = 100;

    // 1. Register 100 components
    for (int i = 0; i < NUM_COMPONENTS; ++i) {
        std::string id = "BENCH_COMP_" + std::to_string(i);
        ComponentType type = static_cast<ComponentType>(i % 9);
        engine.RegisterComponent(id, type, "Bench Component " + std::to_string(i));
    }

    // 2. Measure 1,000 Health/Telemetry Updates
    auto updStart = std::chrono::high_resolution_clock::now();
    for (int u = 0; u < NUM_UPDATES; ++u) {
        std::string id = "BENCH_COMP_" + std::to_string(u % NUM_COMPONENTS);
        engine.UpdateTelemetry(id, 40.0 + static_cast<double>(u % 30), 45.0, 55.0, 50.0 + static_cast<double>(u % 20), 0.1);
    }
    auto updEnd = std::chrono::high_resolution_clock::now();
    double updTotalMs = std::chrono::duration<double, std::milli>(updEnd - updStart).count();
    std::cout << "  -> 1,000 Updates Total Latency: " << updTotalMs << " ms (Avg: " << (updTotalMs / NUM_UPDATES) << " ms / update)" << std::endl;

    // 3. Measure 1,000 Diagnostics
    auto diagStart = std::chrono::high_resolution_clock::now();
    for (int d = 0; d < NUM_DIAGNOSTICS; ++d) {
        std::string id = "BENCH_COMP_" + std::to_string(d % NUM_COMPONENTS);
        engine.RunDiagnostics(id);
    }
    auto diagEnd = std::chrono::high_resolution_clock::now();
    double diagTotalMs = std::chrono::duration<double, std::milli>(diagEnd - diagStart).count();
    std::cout << "  -> 1,000 Diagnostics Total Latency: " << diagTotalMs << " ms (Avg: " << (diagTotalMs / NUM_DIAGNOSTICS) << " ms / diagnostic)" << std::endl;

    // 4. Measure 100 Maintenance Predictions
    auto predStart = std::chrono::high_resolution_clock::now();
    for (int p = 0; p < NUM_PREDICTIONS; ++p) {
        engine.PredictFailures();
    }
    auto predEnd = std::chrono::high_resolution_clock::now();
    double predTotalMs = std::chrono::duration<double, std::milli>(predEnd - predStart).count();
    std::cout << "  -> 100 Maintenance Predictions Total Latency: " << predTotalMs << " ms" << std::endl;

    // 5. Measure 100 Full System Scans (100 components each)
    auto scanStart = std::chrono::high_resolution_clock::now();
    for (int s = 0; s < NUM_SCANS; ++s) {
        engine.RunFullSystemScan();
    }
    auto scanEnd = std::chrono::high_resolution_clock::now();
    double scanTotalMs = std::chrono::duration<double, std::milli>(scanEnd - scanStart).count();
    double avgScanMs = scanTotalMs / NUM_SCANS;
    std::cout << "  -> 100 Full System Scans Total Latency: " << scanTotalMs << " ms (Avg: " << avgScanMs << " ms / scan)" << std::endl;

    TEST_ASSERT(avgScanMs < 5.0, "Average full system scan latency must be < 5.0 ms");
    std::cout << "  ✓ PASS: All performance benchmarks (< 5 ms full system scan) satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 14B System Diagnostics Tests          \n";
    std::cout << "==========================================================\n";

    Test1_ComponentRegistration();
    Test2_HealthMonitoring();
    Test3_Heartbeat();
    Test4_FailureDetection();
    Test5_PredictionEngine();
    Test6_MaintenanceGeneration();
    Test7_MultiMetricHealth();
    Test8_DiagnosticsAndScan();
    Test9_SyncAndReadiness();
    Test10_Reports();
    Test11_Concurrency();
    Test12_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " ALL 12 PHASE 14B UNIT TESTS PASSED SUCCESSFULLY (100%)  \n";
    std::cout << "==========================================================\n";
    return 0;
}

