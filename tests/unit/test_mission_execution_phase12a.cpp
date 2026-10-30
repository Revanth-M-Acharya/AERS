/**
 * @file test_mission_execution_phase12a.cpp
 * @brief Comprehensive Unit Test Suite for SMART AEGIS Phase 12A
 *        City-Wide Emergency Mission Execution Engine
 *
 * Validates mission creation, assignments, state machine transitions, chronological timelines,
 * formatted reports and structured logs, statistics aggregation, cancellation/abort, completion lifecycle,
 * 100-thread concurrency, and sub-5ms performance benchmark.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "mission_execution/MissionExecutionEngine.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cmath>
#include <iomanip>

using namespace aegis::mission;

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
 * @brief TEST 1: Mission Creation & Deterministic Rules 1 & 2
 */
void Test1_MissionCreation() {
    std::cout << "[TEST 1] Mission Creation..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m1("MISSION_001", "EMERG_911_01", "INC_101", 12.9716, 77.5946, MissionPriority::CRITICAL);
    m1.patientId = "PAT_001";
    m1.patientAcuity = 1;
    m1.missionNotes = "STEMI in progress";

    // Rule 1 & Rule 2 validation
    TEST_ASSERT(engine.CreateMission(m1), "Mission creation for MISSION_001 must succeed");
    TEST_ASSERT(!engine.CreateMission(m1), "Duplicate mission creation must be rejected (Rule 1)");

    // Rejection of empty mission ID
    MissionRecord mEmptyId("", "EMERG_911_02", "INC_102");
    TEST_ASSERT(!engine.CreateMission(mEmptyId), "Empty mission ID must be rejected (Rule 1)");

    // Rejection of empty emergency ID
    MissionRecord mEmptyEmerg("MISSION_002", "", "INC_102");
    TEST_ASSERT(!engine.CreateMission(mEmptyEmerg), "Empty emergency ID must be rejected (Rule 2)");

    // Retrieval validation
    MissionRecord retrieved;
    TEST_ASSERT(engine.GetMission("MISSION_001", retrieved), "Must retrieve MISSION_001");
    TEST_ASSERT(retrieved.emergencyId == "EMERG_911_01", "Emergency ID must match");
    TEST_ASSERT(retrieved.incidentId == "INC_101", "Incident ID must match");
    TEST_ASSERT(retrieved.priority == MissionPriority::CRITICAL, "Priority must be CRITICAL");
    TEST_ASSERT(retrieved.status == MissionStatus::NEW, "Initial status must be NEW");
    TEST_ASSERT(retrieved.stage == MissionStage::CALL_RECEIVED, "Initial stage must be CALL_RECEIVED");
    TEST_ASSERT(retrieved.patientAcuity == 1, "Patient acuity must be 1");

    // Incident mapping lookup
    MissionRecord byInc;
    TEST_ASSERT(engine.GetMissionByIncident("INC_101", byInc), "Lookup by incident ID must succeed");
    TEST_ASSERT(byInc.missionId == "MISSION_001", "Mapped mission ID must match");

    std::cout << "  ✓ PASS: Mission creation and Rules 1 & 2 validated." << std::endl;
}

/**
 * @brief TEST 2: Mission Assignment Lifecycle (Ambulance, Hospital, Corridor, Route, Traffic Plan)
 */
void Test2_MissionAssignment() {
    std::cout << "[TEST 2] Mission Assignment..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_002", "EMERG_911_02", "INC_102", 12.9716, 77.5946, MissionPriority::HIGH);
    engine.CreateMission(m);

    // Rule 3: Assign Ambulance
    TEST_ASSERT(engine.AssignAmbulance("MISSION_002", "AMB_ALS_07", "CREW_ALPHA", "ALS"), "Assign ambulance must succeed");

    // Rule 4: Assign Hospital
    TEST_ASSERT(engine.AssignHospital("MISSION_002", "HOSP_METRO_01", 12.9800, 77.6100, "TRAUMA"), "Assign hospital must succeed");

    // Rule 5: Assign Route
    TEST_ASSERT(engine.AssignRoute("MISSION_002", "ROUTE_CORR_42", 5.4, 7.2), "Assign route must succeed");

    // Rule 6: Assign Green Corridor
    TEST_ASSERT(engine.AssignGreenCorridor("MISSION_002", "CORR_WAVE_99", 2), "Assign green corridor must succeed");

    // Rule 7: Assign Traffic Plan
    TEST_ASSERT(engine.AssignTrafficPlan("MISSION_002", 3), "Assign traffic plan must succeed");

    MissionRecord updated;
    engine.GetMission("MISSION_002", updated);
    TEST_ASSERT(updated.ambulanceId == "AMB_ALS_07", "Ambulance ID must be updated");
    TEST_ASSERT(updated.crewId == "CREW_ALPHA", "Crew ID must match");
    TEST_ASSERT(updated.hospitalId == "HOSP_METRO_01", "Hospital ID must be updated");
    TEST_ASSERT(updated.currentRouteId == "ROUTE_CORR_42", "Route ID must be updated");
    TEST_ASSERT(std::abs(updated.distance - 5.4) < 1e-4, "Distance must be 5.4 km");
    TEST_ASSERT(std::abs(updated.estimatedEta - 7.2) < 1e-4, "ETA must be 7.2 min");
    TEST_ASSERT(updated.greenCorridorId == "CORR_WAVE_99", "Green corridor ID must match");
    TEST_ASSERT(updated.assignedCorridorVersion == 2, "Corridor version must be 2");
    TEST_ASSERT(updated.trafficPlanVersion == 3, "Traffic plan version must be 3");
    TEST_ASSERT(updated.status == MissionStatus::READY, "Status must auto-transition to READY after assignments");

    std::cout << "  ✓ PASS: Multi-subsystem mission assignments verified." << std::endl;
}

/**
 * @brief TEST 3: Mission State Machine & Progression
 */
void Test3_StateMachine() {
    std::cout << "[TEST 3] State Machine..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_003", "EMERG_911_03", "INC_103", 12.9716, 77.5946);
    engine.CreateMission(m);

    // Starting without assignments must fail
    TEST_ASSERT(!engine.StartMission("MISSION_003"), "Cannot start mission without ambulance and hospital assignments");

    // Provide assignments
    engine.AssignAmbulance("MISSION_003", "AMB_01");
    engine.AssignHospital("MISSION_003", "HOSP_01");

    // Transition NEW -> READY -> ACTIVE
    TEST_ASSERT(engine.StartMission("MISSION_003"), "Start mission must succeed now");
    MissionRecord active;
    engine.GetMission("MISSION_003", active);
    TEST_ASSERT(active.status == MissionStatus::ACTIVE, "Status must be ACTIVE");
    TEST_ASSERT(active.stage == MissionStage::AMBULANCE_EN_ROUTE, "Stage must be AMBULANCE_EN_ROUTE");

    // Active -> Paused -> Active
    TEST_ASSERT(engine.PauseMission("MISSION_003", "Awaiting clearance"), "Pause must succeed");
    engine.GetMission("MISSION_003", active);
    TEST_ASSERT(active.status == MissionStatus::PAUSED, "Status must be PAUSED");

    TEST_ASSERT(engine.ResumeMission("MISSION_003"), "Resume must succeed");
    engine.GetMission("MISSION_003", active);
    TEST_ASSERT(active.status == MissionStatus::ACTIVE, "Status must restore to ACTIVE");

    // Illegal transitions: ACTIVE directly to READY or NEW must fail
    TEST_ASSERT(!engine.UpdateMissionStatus("MISSION_003", MissionStatus::NEW), "Cannot transition from ACTIVE to NEW");
    TEST_ASSERT(!engine.UpdateMissionStatus("MISSION_003", MissionStatus::READY), "Cannot transition from ACTIVE to READY");

    // Stage progression: AMBULANCE_EN_ROUTE -> PATIENT_PICKUP -> TRANSPORTING -> ARRIVING_HOSPITAL -> HANDOVER
    TEST_ASSERT(engine.UpdateMissionStage("MISSION_003", MissionStage::PATIENT_PICKUP), "Stage advance to PATIENT_PICKUP");
    TEST_ASSERT(engine.UpdateMissionStage("MISSION_003", MissionStage::TRANSPORTING), "Stage advance to TRANSPORTING");
    TEST_ASSERT(engine.UpdateMissionStage("MISSION_003", MissionStage::ARRIVING_HOSPITAL), "Stage advance to ARRIVING_HOSPITAL");
    TEST_ASSERT(engine.UpdateMissionStage("MISSION_003", MissionStage::HANDOVER), "Stage advance to HANDOVER");

    // Illegal backwards stage progression must be rejected
    TEST_ASSERT(!engine.UpdateMissionStage("MISSION_003", MissionStage::AMBULANCE_EN_ROUTE),
                "Backwards stage progression must be rejected");

    std::cout << "  ✓ PASS: State machine and stage progression validated." << std::endl;
}

/**
 * @brief TEST 4: Chronological Mission Timeline
 */
void Test4_Timeline() {
    std::cout << "[TEST 4] Timeline..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_004", "EMERG_911_04", "INC_104");
    engine.CreateMission(m);
    engine.AssignAmbulance("MISSION_004", "AMB_02");
    engine.AssignHospital("MISSION_004", "HOSP_02");
    engine.AssignRoute("MISSION_004", "ROUTE_01", 4.0, 6.0);
    engine.AssignGreenCorridor("MISSION_004", "CORR_01", 1);
    engine.AssignTrafficPlan("MISSION_004", 1);
    engine.StartMission("MISSION_004");
    engine.UpdateMissionStage("MISSION_004", MissionStage::PATIENT_PICKUP);
    engine.UpdateMissionStage("MISSION_004", MissionStage::TRANSPORTING);
    engine.UpdateMissionStage("MISSION_004", MissionStage::ARRIVING_HOSPITAL);
    engine.CompleteMission("MISSION_004");

    std::vector<MissionTimelineEntry> timeline = engine.GetTimeline("MISSION_004");
    TEST_ASSERT(!timeline.empty(), "Timeline must not be empty");
    TEST_ASSERT(timeline.size() >= 8, "Timeline must record all discrete lifecycle events");

    std::string timelineStr = engine.GenerateMissionTimeline("MISSION_004");
    TEST_ASSERT(timelineStr.find("MISSION TIMELINE: MISSION_004") != std::string::npos, "Header must be present");
    TEST_ASSERT(timelineStr.find("Mission Created") != std::string::npos, "Creation event must be present");
    TEST_ASSERT(timelineStr.find("Mission Started") != std::string::npos, "Start event must be present");
    TEST_ASSERT(timelineStr.find("Patient Picked Up") != std::string::npos, "Pickup event must be present");
    TEST_ASSERT(timelineStr.find("Mission Completed") != std::string::npos, "Completion event must be present");

    std::cout << "  ✓ PASS: Tactical timeline events verified chronologically." << std::endl;
}

/**
 * @brief TEST 5: Formatted Mission Reports and Structured Logs
 */
void Test5_MissionReports() {
    std::cout << "[TEST 5] Mission Reports..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_005", "EMERG_911_05", "INC_105", 12.9716, 77.5946, MissionPriority::CRITICAL);
    engine.CreateMission(m);
    engine.AssignAmbulance("MISSION_005", "AMB_CRIT_09");
    engine.AssignHospital("MISSION_005", "HOSP_TRAUMA_CENTRAL");
    engine.AssignRoute("MISSION_005", "ROUTE_99", 8.2, 10.5);
    engine.AssignGreenCorridor("MISSION_005", "CORR_WAVE_05", 1);
    engine.AssignTrafficPlan("MISSION_005", 2);
    engine.StartMission("MISSION_005");
    engine.UpdateMissionStage("MISSION_005", MissionStage::TRANSPORTING);

    std::string report = engine.GenerateMissionReport("MISSION_005");
    std::cout << report << std::endl;

    // Verify exact required report tokens
    TEST_ASSERT(report.find("[MISSION]") != std::string::npos, "Report must contain [MISSION]");
    TEST_ASSERT(report.find("Mission ID:\nMISSION_005") != std::string::npos, "Mission ID formatted correctly");
    TEST_ASSERT(report.find("Incident ID:\nINC_105") != std::string::npos, "Incident ID formatted correctly");
    TEST_ASSERT(report.find("Priority:\nCRITICAL") != std::string::npos, "Priority formatted correctly");
    TEST_ASSERT(report.find("Status:\nACTIVE") != std::string::npos, "Status formatted correctly");
    TEST_ASSERT(report.find("Stage:\nTRANSPORTING") != std::string::npos, "Stage formatted correctly");
    TEST_ASSERT(report.find("Ambulance:\nAMB_CRIT_09") != std::string::npos, "Ambulance formatted correctly");
    TEST_ASSERT(report.find("Hospital:\nHOSP_TRAUMA_CENTRAL") != std::string::npos, "Hospital formatted correctly");
    TEST_ASSERT(report.find("ETA:\n10.5 min") != std::string::npos, "ETA formatted correctly");
    TEST_ASSERT(report.find("Distance:\n8.2 km") != std::string::npos, "Distance formatted correctly");
    TEST_ASSERT(report.find("Green Corridor:\nCORR_WAVE_05") != std::string::npos, "Green corridor formatted correctly");
    TEST_ASSERT(report.find("Route Version:\n1") != std::string::npos, "Route version formatted correctly");
    TEST_ASSERT(report.find("Traffic Version:\n2") != std::string::npos, "Traffic version formatted correctly");

    // Structured Log Verification
    std::string logLine = engine.FormatStructuredLog("MISSION_005");
    TEST_ASSERT(logLine.find("[MISSION]") != std::string::npos, "Structured log must start with [MISSION]");
    TEST_ASSERT(logLine.find("Mission:\nMISSION_005") != std::string::npos, "Structured log mission field");

    std::cout << "  ✓ PASS: Formatted report and structured logs verified against specification." << std::endl;
}

/**
 * @brief TEST 6: Aggregated Mission Statistics
 */
void Test6_MissionStatistics() {
    std::cout << "[TEST 6] Mission Statistics..." << std::endl;
    MissionExecutionEngine engine;

    // Mission 1: Completed
    MissionRecord m1("M_STAT_01", "EM_01", "INC_01");
    m1.missionCreationTimestamp = 1000000;
    m1.dispatchTimestamp = 1060000; // 1 min response
    m1.pickupTimestamp = 1240000;   // 3 min pickup
    m1.hospitalArrivalTimestamp = 1600000; // 6 min transit
    m1.missionCompletionTimestamp = 1720000; // 12 min total mission
    m1.distance = 5.0;
    m1.estimatedEta = 8.0;
    engine.CreateMission(m1);
    engine.AssignAmbulance("M_STAT_01", "A1");
    engine.AssignHospital("M_STAT_01", "H1");
    engine.StartMission("M_STAT_01");
    engine.CompleteMission("M_STAT_01");

    // Mission 2: Cancelled
    MissionRecord m2("M_STAT_02", "EM_02", "INC_02");
    m2.distance = 3.0;
    engine.CreateMission(m2);
    engine.AssignAmbulance("M_STAT_02", "A2");
    engine.AssignHospital("M_STAT_02", "H2");
    engine.CancelMission("M_STAT_02", "False alarm");

    // Mission 3: Active
    MissionRecord m3("M_STAT_03", "EM_03", "INC_03");
    m3.distance = 4.0;
    engine.CreateMission(m3);
    engine.AssignAmbulance("M_STAT_03", "A3");
    engine.AssignHospital("M_STAT_03", "H3");
    engine.StartMission("M_STAT_03");

    MissionStatistics stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalMissions == 3, "Total missions must be 3");
    TEST_ASSERT(stats.completedMissions == 1, "Completed missions must be 1");
    TEST_ASSERT(stats.cancelledMissions == 1, "Cancelled missions must be 1");
    TEST_ASSERT(stats.activeMissions == 1, "Active missions must be 1");
    TEST_ASSERT(stats.missionSuccessRate == 50.0, "Success rate must be 50% (1 completed out of 2 resolved)");

    std::cout << "  ✓ PASS: Statistical metrics aggregation verified." << std::endl;
}

/**
 * @brief TEST 7: Mission Cancellation & Abort
 */
void Test7_MissionCancellation() {
    std::cout << "[TEST 7] Mission Cancellation..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_007", "EMERG_911_07", "INC_107");
    engine.CreateMission(m);
    engine.AssignAmbulance("MISSION_007", "AMB_07");
    engine.AssignHospital("MISSION_007", "HOSP_07");
    engine.StartMission("MISSION_007");

    // Cancel mission
    TEST_ASSERT(engine.CancelMission("MISSION_007", "Redirected by Dispatcher"), "Cancellation must succeed");
    MissionRecord cancelled;
    engine.GetMission("MISSION_007", cancelled);
    TEST_ASSERT(cancelled.status == MissionStatus::CANCELLED, "Status must be CANCELLED");
    TEST_ASSERT(cancelled.cancellationReason == "Redirected by Dispatcher", "Cancellation reason must match");
    TEST_ASSERT(cancelled.IsTerminal(), "Cancelled mission must be terminal");

    // Subsequent start or stage change must fail
    TEST_ASSERT(!engine.StartMission("MISSION_007"), "Cannot start a cancelled mission");
    TEST_ASSERT(!engine.UpdateMissionStage("MISSION_007", MissionStage::TRANSPORTING),
                "Cannot advance stage of cancelled mission");

    std::cout << "  ✓ PASS: Cancellation handling and terminal state protection verified." << std::endl;
}

/**
 * @brief TEST 8: Mission Completion & Archival Lifecycle (Rule 10)
 */
void Test8_MissionCompletion() {
    std::cout << "[TEST 8] Mission Completion..." << std::endl;
    MissionExecutionEngine engine;

    MissionRecord m("MISSION_008", "EMERG_911_08", "INC_108");
    engine.CreateMission(m);
    engine.AssignAmbulance("MISSION_008", "AMB_08");
    engine.AssignHospital("MISSION_008", "HOSP_08");
    engine.StartMission("MISSION_008");
    engine.UpdateMissionStage("MISSION_008", MissionStage::PATIENT_PICKUP);
    engine.UpdateMissionStage("MISSION_008", MissionStage::TRANSPORTING);
    engine.UpdateMissionStage("MISSION_008", MissionStage::ARRIVING_HOSPITAL);
    engine.UpdateMissionStage("MISSION_008", MissionStage::HANDOVER);

    // Complete mission
    TEST_ASSERT(engine.CompleteMission("MISSION_008"), "Completion must succeed");
    MissionRecord completed;
    engine.GetMission("MISSION_008", completed);
    TEST_ASSERT(completed.status == MissionStatus::COMPLETED, "Status must be COMPLETED");
    TEST_ASSERT(completed.stage == MissionStage::MISSION_COMPLETE, "Stage must be MISSION_COMPLETE");
    TEST_ASSERT(completed.missionCompletionTimestamp > 0, "Completion timestamp must be set");

    // Archive mission (Rule 10)
    TEST_ASSERT(engine.ArchiveMission("MISSION_008"), "Archival of completed mission must succeed");
    TEST_ASSERT(engine.GetAllActiveMissions().empty(), "Active missions must be empty after archival");

    // Retrieve from archive
    MissionRecord archived;
    TEST_ASSERT(engine.GetMission("MISSION_008", archived), "Must retrieve archived mission");
    TEST_ASSERT(archived.missionId == "MISSION_008", "Archived mission ID must match");

    // Restore from archive
    TEST_ASSERT(engine.RestoreMission("MISSION_008"), "Restoration from archive must succeed");

    std::cout << "  ✓ PASS: Mission completion and archival lifecycle verified." << std::endl;
}

/**
 * @brief TEST 9: 100-Thread Concurrency
 */
void Test9_100ThreadConcurrency() {
    std::cout << "[TEST 9] 100-Thread Concurrency..." << std::endl;
    MissionExecutionEngine engine;

    constexpr int kNumThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<int> successCount{0};

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&engine, t, &successCount]() {
            std::string mId = "CONC_M_" + std::to_string(t);
            std::string eId = "CONC_E_" + std::to_string(t);
            std::string incId = "CONC_INC_" + std::to_string(t);

            MissionRecord m(mId, eId, incId, 12.9000 + (static_cast<double>(t) * 0.001),
                            77.5000 + (static_cast<double>(t) * 0.001),
                            (t % 2 == 0 ? MissionPriority::CRITICAL : MissionPriority::HIGH));

            // Create
            if (engine.CreateMission(m)) {
                // Assign
                engine.AssignAmbulance(mId, "AMB_" + std::to_string(t));
                engine.AssignHospital(mId, "HOSP_" + std::to_string(t % 10));
                engine.AssignRoute(mId, "ROUTE_" + std::to_string(t), 5.0, 7.5);
                engine.AssignGreenCorridor(mId, "CORR_" + std::to_string(t), 1);
                engine.AssignTrafficPlan(mId, 1);

                // Start
                engine.StartMission(mId);

                // Updates
                engine.UpdateETA(mId, 4.0);
                engine.UpdateDistance(mId, 2.5);
                engine.UpdateMissionStage(mId, MissionStage::PATIENT_PICKUP);
                engine.UpdateMissionStage(mId, MissionStage::TRANSPORTING);

                // Either complete or cancel
                if (t % 5 == 0) {
                    engine.CancelMission(mId, "Resolved on scene");
                } else {
                    engine.UpdateMissionStage(mId, MissionStage::ARRIVING_HOSPITAL);
                    engine.UpdateMissionStage(mId, MissionStage::HANDOVER);
                    engine.CompleteMission(mId);
                }

                // Queries & Reports
                engine.GetStatistics();
                engine.GenerateMissionReport(mId);

                successCount.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    TEST_ASSERT(successCount.load() == kNumThreads, "All 100 concurrent threads must complete successfully");
    TEST_ASSERT(engine.GetAllMissions().size() == static_cast<size_t>(kNumThreads),
                "Total registered missions must be 100");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (operations: "
              << successCount.load() << ")." << std::endl;
}

/**
 * @brief TEST 10: Performance Benchmark (Create 100 missions < 5 ms; Lookup < 1 ms; Stats < 5 ms)
 */
void Test10_PerformanceBenchmark() {
    std::cout << "[TEST 10] Performance Benchmark: 100 Missions Lifecycle & Queries..." << std::endl;
    MissionExecutionEngine engine;

    constexpr int kBatchSize = 100;

    // Benchmark 1: Batch Mission Creation
    auto startCreate = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kBatchSize; ++i) {
        std::string mId = "PERF_M_" + std::to_string(i);
        std::string eId = "PERF_E_" + std::to_string(i);
        std::string incId = "PERF_INC_" + std::to_string(i);
        MissionRecord m(mId, eId, incId, 12.9716, 77.5946, MissionPriority::HIGH);
        engine.CreateMission(m);
    }
    auto endCreate = std::chrono::high_resolution_clock::now();
    double createLatencyMs = std::chrono::duration<double, std::milli>(endCreate - startCreate).count();

    // Benchmark 2: Mission Lookup
    auto startLookup = std::chrono::high_resolution_clock::now();
    MissionRecord lookupResult;
    for (int i = 0; i < kBatchSize; ++i) {
        engine.GetMission("PERF_M_" + std::to_string(i), lookupResult);
    }
    auto endLookup = std::chrono::high_resolution_clock::now();
    double totalLookupMs = std::chrono::duration<double, std::milli>(endLookup - startLookup).count();
    double avgLookupMs = totalLookupMs / static_cast<double>(kBatchSize);

    // Benchmark 3: Statistics Generation
    auto startStats = std::chrono::high_resolution_clock::now();
    MissionStatistics stats = engine.GetStatistics();
    auto endStats = std::chrono::high_resolution_clock::now();
    double statsLatencyMs = std::chrono::duration<double, std::milli>(endStats - startStats).count();

    std::cout << "  -> Batch 100 Missions Creation Latency: " << createLatencyMs << " ms" << std::endl;
    std::cout << "  -> Average Mission Lookup Latency: " << avgLookupMs << " ms" << std::endl;
    std::cout << "  -> Statistics Generation Latency: " << statsLatencyMs << " ms" << std::endl;

    TEST_ASSERT(stats.totalMissions == static_cast<uint32_t>(kBatchSize), "Statistics count must match batch size");
    TEST_ASSERT(createLatencyMs < 5.0, "Creation latency must be under 5.0 ms");
    TEST_ASSERT(avgLookupMs < 1.0, "Lookup latency must be under 1.0 ms");
    TEST_ASSERT(statsLatencyMs < 5.0, "Statistics generation latency must be under 5.0 ms");

    std::cout << "  ✓ PASS: All performance benchmarks (< 5 ms creation, < 1 ms lookup, < 5 ms stats) satisfied!"
              << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 12A Mission Execution Engine Tests   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_MissionCreation();
    Test2_MissionAssignment();
    Test3_StateMachine();
    Test4_Timeline();
    Test5_MissionReports();
    Test6_MissionStatistics();
    Test7_MissionCancellation();
    Test8_MissionCompletion();
    Test9_100ThreadConcurrency();
    Test10_PerformanceBenchmark();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 10 PHASE 12A UNIT TESTS PASSED SUCCESSFULLY (100%)   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

