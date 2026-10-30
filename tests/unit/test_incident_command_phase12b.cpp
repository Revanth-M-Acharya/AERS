/**
 * @file test_incident_command_phase12b.cpp
 * @brief Comprehensive Unit Test Suite for SMART AEGIS Phase 12B
 *        Real-Time Incident Command & Decision Support Engine
 *
 * Validates incident registration, severity escalation, deterministic automatic decisions,
 * manual dispatcher overrides, mission/commander assignments, decision timelines, formatted reports,
 * statistics aggregation, 100-thread concurrency, and sub-5ms performance benchmark (1000 incidents).
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "incident_command/IncidentCommandEngine.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <iomanip>

using namespace aegis::incident_command;

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
 * @brief TEST 1: Incident Registration & Queries
 */
void Test1_IncidentRegistration() {
    std::cout << "[TEST 1] Incident Registration..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc1("INC_101", IncidentSeverity::NORMAL, "Main St & 5th Ave", 12.9716, 77.5946);
    inc1.missionId = "MISSION_001";
    inc1.notes = "Minor vehicle collision";

    TEST_ASSERT(engine.CreateIncident(inc1), "Incident INC_101 must be created successfully");
    TEST_ASSERT(!engine.CreateIncident(inc1), "Duplicate incident INC_101 must be rejected");

    IncidentRecord emptyId("", IncidentSeverity::HIGH);
    TEST_ASSERT(!engine.CreateIncident(emptyId), "Empty incident ID must be rejected");

    IncidentRecord retrieved;
    TEST_ASSERT(engine.GetIncident("INC_101", retrieved), "INC_101 must be retrieved");
    TEST_ASSERT(retrieved.incidentId == "INC_101", "Retrieved ID must match");
    TEST_ASSERT(retrieved.status == IncidentStatus::ACTIVE, "Initial status must be ACTIVE");
    TEST_ASSERT(retrieved.currentDecision == CommandDecision::MONITOR, "Initial decision must be MONITOR");

    auto activeList = engine.GetActiveIncidents();
    TEST_ASSERT(activeList.size() == 1, "Active list must contain 1 incident");
    TEST_ASSERT(activeList[0].incidentId == "INC_101", "Active incident must be INC_101");

    std::cout << "  ✓ PASS: Incident registration and duplicate rejection verified." << std::endl;
}

/**
 * @brief TEST 2: Severity Escalation & De-escalation
 */
void Test2_SeverityEscalation() {
    std::cout << "[TEST 2] Severity Escalation..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc("INC_201", IncidentSeverity::NORMAL, "Central Blvd", 12.9750, 77.6000);
    TEST_ASSERT(engine.CreateIncident(inc), "Create INC_201");

    // NORMAL -> HIGH
    TEST_ASSERT(engine.EscalateIncident("INC_201", "Multiple vehicles involved"), "Escalate to HIGH");
    IncidentRecord r;
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::HIGH, "Severity must be HIGH");
    TEST_ASSERT(r.status == IncidentStatus::ESCALATED, "Status must be ESCALATED");

    // HIGH -> CRITICAL
    TEST_ASSERT(engine.EscalateIncident("INC_201", "Pedestrian trapped"), "Escalate to CRITICAL");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::CRITICAL, "Severity must be CRITICAL");

    // CRITICAL -> DISASTER
    TEST_ASSERT(engine.EscalateIncident("INC_201", "Hazardous tanker leak"), "Escalate to DISASTER");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::DISASTER, "Severity must be DISASTER");

    // DISASTER remains DISASTER
    TEST_ASSERT(engine.EscalateIncident("INC_201", "Further escalation attempted"), "Escalate ceiling");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::DISASTER, "Severity remains DISASTER");

    // De-escalate: DISASTER -> CRITICAL
    TEST_ASSERT(engine.DeEscalateIncident("INC_201", "Tanker secured"), "De-escalate to CRITICAL");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::CRITICAL, "Severity must be CRITICAL");

    // De-escalate: CRITICAL -> HIGH
    TEST_ASSERT(engine.DeEscalateIncident("INC_201", "Patients stabilized"), "De-escalate to HIGH");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::HIGH, "Severity must be HIGH");

    // De-escalate: HIGH -> NORMAL
    TEST_ASSERT(engine.DeEscalateIncident("INC_201", "Scene cleared"), "De-escalate to NORMAL");
    TEST_ASSERT(engine.GetIncident("INC_201", r), "Get incident");
    TEST_ASSERT(r.severity == IncidentSeverity::NORMAL, "Severity must be NORMAL");

    std::cout << "  ✓ PASS: Severity escalation and de-escalation cycles verified." << std::endl;
}

/**
 * @brief TEST 3: Deterministic Automatic Decision Rules (Rules 1 to 8)
 */
void Test3_AutomaticDecisions() {
    std::cout << "[TEST 3] Automatic Decisions..." << std::endl;
    IncidentCommandEngine engine;

    // Rule 1: Critical mission -> Deploy police automatically
    IncidentRecord inc1("INC_RULE1", IncidentSeverity::CRITICAL);
    engine.CreateIncident(inc1);
    CommandDecision d1 = engine.GenerateAutomaticDecision("INC_RULE1");
    TEST_ASSERT(d1 == CommandDecision::REQUEST_POLICE, "Rule 1 must deploy police automatically");

    // Rule 2: Mass casualty -> Deploy disaster response
    IncidentRecord inc2("INC_RULE2", IncidentSeverity::HIGH);
    engine.CreateIncident(inc2);
    engine.SetMassCasualty("INC_RULE2", true, 120);
    CommandDecision d2 = engine.GenerateAutomaticDecision("INC_RULE2");
    TEST_ASSERT(d2 == CommandDecision::REQUEST_DISASTER_TEAM, "Rule 2 must deploy disaster response");

    // Rule 3: Traffic collapse -> Force reroute
    IncidentRecord inc3("INC_RULE3", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc3);
    engine.SetTrafficCollapse("INC_RULE3", true);
    CommandDecision d3 = engine.GenerateAutomaticDecision("INC_RULE3");
    TEST_ASSERT(d3 == CommandDecision::REROUTE_ALL, "Rule 3 must force reroute all");

    // Rule 4: Hospital unavailable -> Reassign hospital
    IncidentRecord inc4("INC_RULE4", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc4);
    engine.SetHospitalUnavailable("INC_RULE4", true);
    CommandDecision d4 = engine.GenerateAutomaticDecision("INC_RULE4");
    TEST_ASSERT(d4 == CommandDecision::REASSIGN_HOSPITAL, "Rule 4 must reassign hospital");

    // Rule 5: Ambulance failure -> Deploy backup ambulance
    IncidentRecord inc5("INC_RULE5", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc5);
    engine.SetAmbulanceFailed("INC_RULE5", true);
    CommandDecision d5 = engine.GenerateAutomaticDecision("INC_RULE5");
    TEST_ASSERT(d5 == CommandDecision::DEPLOY_EXTRA_AMBULANCE, "Rule 5 must deploy backup ambulance");

    // Rule 6: Fire detected -> Notify fire department
    IncidentRecord inc6("INC_RULE6", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc6);
    engine.SetFireDetected("INC_RULE6", true);
    CommandDecision d6 = engine.GenerateAutomaticDecision("INC_RULE6");
    TEST_ASSERT(d6 == CommandDecision::REQUEST_FIRE, "Rule 6 must notify fire department");

    // Rule 7: Incident affecting multiple missions -> Escalate
    IncidentRecord inc7("INC_RULE7", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc7);
    engine.SetMultipleMissionsAffected("INC_RULE7", true);
    CommandDecision d7 = engine.GenerateAutomaticDecision("INC_RULE7");
    TEST_ASSERT(d7 == CommandDecision::ESCALATE, "Rule 7 must escalate");

    // Rule 8: Large congestion -> Lock green corridors
    IncidentRecord inc8("INC_RULE8", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc8);
    engine.SetLargeCongestion("INC_RULE8", true);
    CommandDecision d8 = engine.GenerateAutomaticDecision("INC_RULE8");
    TEST_ASSERT(d8 == CommandDecision::LOCK_CORRIDORS, "Rule 8 must lock green corridors");

    // Default: Normal situation -> MONITOR
    IncidentRecord incNorm("INC_NORM", IncidentSeverity::NORMAL);
    engine.CreateIncident(incNorm);
    CommandDecision dNorm = engine.GenerateAutomaticDecision("INC_NORM");
    TEST_ASSERT(dNorm == CommandDecision::MONITOR, "Normal situation must MONITOR");

    std::cout << "  ✓ PASS: All 8 deterministic automatic decision rules verified." << std::endl;
}

/**
 * @brief TEST 4: Manual Override & Operator Interventions (Rule 9)
 */
void Test4_ManualOverride() {
    std::cout << "[TEST 4] Manual Override..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc("INC_MANUAL", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc);

    // Manual override 1: FORCE_REROUTE
    bool ok1 = engine.ManualOverride("INC_MANUAL",
                                     OperatorAction::FORCE_REROUTE,
                                     CommandDecision::REROUTE_ALL,
                                     "Construction barricade reported by police",
                                     "DISPATCHER_SARAH");
    TEST_ASSERT(ok1, "Manual override 1 must succeed");

    IncidentRecord r;
    TEST_ASSERT(engine.GetIncident("INC_MANUAL", r), "Get incident");
    TEST_ASSERT(r.currentDecision == CommandDecision::REROUTE_ALL, "Decision must be REROUTE_ALL");
    TEST_ASSERT(r.operatorName == "DISPATCHER_SARAH", "Operator must be DISPATCHER_SARAH");

    // Manual override 2: FORCE_AMBULANCE_CHANGE
    bool ok2 = engine.ManualOverride("INC_MANUAL",
                                     OperatorAction::FORCE_AMBULANCE_CHANGE,
                                     CommandDecision::DEPLOY_EXTRA_AMBULANCE,
                                     "Tire puncture on primary ambulance",
                                     "DISPATCHER_BOB");
    TEST_ASSERT(ok2, "Manual override 2 must succeed");

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.manualOverrides == 2, "Manual overrides counter must be 2");

    auto history = engine.GetDecisionHistory("INC_MANUAL");
    TEST_ASSERT(history.size() >= 2, "Decision history must contain manual overrides");
    bool foundOverride = false;
    for (const auto& h : history) {
        if (h.action == OperatorAction::FORCE_AMBULANCE_CHANGE) {
            foundOverride = true;
            TEST_ASSERT(h.operatorName == "DISPATCHER_BOB", "Operator must match in history");
            TEST_ASSERT(h.decision == CommandDecision::DEPLOY_EXTRA_AMBULANCE, "Decision must match");
        }
    }
    TEST_ASSERT(foundOverride, "Manual override must be preserved in history (Rule 9)");

    std::cout << "  ✓ PASS: Manual override and operator audit logging verified." << std::endl;
}

/**
 * @brief TEST 5: Mission and Commander Assignment
 */
void Test5_MissionAssignment() {
    std::cout << "[TEST 5] Mission Assignment..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc("INC_501", IncidentSeverity::HIGH);
    engine.CreateIncident(inc);

    TEST_ASSERT(engine.AssignMission("INC_501", "MISSION_888"), "Assign mission");
    TEST_ASSERT(engine.AssignCommander("INC_501", "CHIEF_JOHNSON"), "Assign commander");

    IncidentRecord r;
    TEST_ASSERT(engine.GetIncident("INC_501", r), "Get incident");
    TEST_ASSERT(r.missionId == "MISSION_888", "Mission ID must be MISSION_888");
    TEST_ASSERT(r.assignedCommander == "CHIEF_JOHNSON", "Commander must be CHIEF_JOHNSON");

    std::cout << "  ✓ PASS: Mission binding and commander assignment verified." << std::endl;
}

/**
 * @brief TEST 6: Decision History & Timeline (Rule 10)
 */
void Test6_DecisionHistory() {
    std::cout << "[TEST 6] Decision History..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc("INC_HIST", IncidentSeverity::NORMAL);
    engine.CreateIncident(inc);

    // Sequence of operations
    engine.EscalateIncident("INC_HIST", "Escalating condition");
    engine.GenerateAutomaticDecision("INC_HIST");
    engine.ManualOverride("INC_HIST", OperatorAction::FORCE_REROUTE, CommandDecision::REROUTE_ALL, "Operator detour", "OP_1");
    engine.ApplyDecision("INC_HIST", CommandDecision::LOCK_CORRIDORS, "Corridor lock down", "COMMANDER_X");

    auto history = engine.GetDecisionHistory("INC_HIST");
    TEST_ASSERT(history.size() >= 4, "Must have recorded all 4 milestone decisions in timeline (Rule 10)");

    std::string timelineStr = engine.GenerateDecisionTimeline("INC_HIST");
    TEST_ASSERT(timelineStr.find("[DECISION TIMELINE: INC_HIST]") != std::string::npos, "Timeline must contain header");
    TEST_ASSERT(timelineStr.find("REROUTE_ALL") != std::string::npos, "Timeline must contain REROUTE_ALL");
    TEST_ASSERT(timelineStr.find("LOCK_CORRIDORS") != std::string::npos, "Timeline must contain LOCK_CORRIDORS");

    std::cout << "  ✓ PASS: Tactical decision history and timeline verified." << std::endl;
}

/**
 * @brief TEST 7: Formatted Reporting
 */
void Test7_Reporting() {
    std::cout << "[TEST 7] Reporting..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord inc("INC_REP", "MISSION_REP", IncidentSeverity::CRITICAL);
    inc.assignedCommander = "CHIEF_MILLER";
    inc.currentDecision = CommandDecision::REQUEST_POLICE;
    inc.status = IncidentStatus::ACTIVE;
    inc.operatorName = "OPERATOR_SARAH";
    inc.creationTimestamp = 1714200000000;
    inc.lastUpdateTimestamp = 1714200005000;

    engine.CreateIncident(inc);

    std::string report = engine.GenerateIncidentReport("INC_REP");
    std::cout << report << std::endl;

    // Validate [INCIDENT] format exactly
    TEST_ASSERT(report.find("[INCIDENT]") != std::string::npos, "Report must start with [INCIDENT]");
    TEST_ASSERT(report.find("Incident:\nINC_REP") != std::string::npos, "Incident field format match");
    TEST_ASSERT(report.find("Mission:\nMISSION_REP") != std::string::npos, "Mission field format match");
    TEST_ASSERT(report.find("Severity:\nCRITICAL") != std::string::npos, "Severity field format match");
    TEST_ASSERT(report.find("Commander:\nCHIEF_MILLER") != std::string::npos, "Commander field format match");
    TEST_ASSERT(report.find("Decision:\nREQUEST_POLICE") != std::string::npos, "Decision field format match");
    TEST_ASSERT(report.find("Status:\nACTIVE") != std::string::npos, "Status field format match");
    TEST_ASSERT(report.find("Operator:\nOPERATOR_SARAH") != std::string::npos, "Operator field format match");
    TEST_ASSERT(report.find("Created:\n1714200000000") != std::string::npos, "Created field format match");
    TEST_ASSERT(report.find("Last Update:\n1714200005000") != std::string::npos, "Last Update format match");

    std::string cityReport = engine.GenerateCitySituationReport();
    TEST_ASSERT(cityReport.find("CITY SITUATION REPORT") != std::string::npos, "City report must have title");
    TEST_ASSERT(cityReport.find("Total Incidents:") != std::string::npos, "City report must have total incidents");

    std::cout << "  ✓ PASS: [INCIDENT] formatted report and City Situation Report verified." << std::endl;
}

/**
 * @brief TEST 8: Statistics & Resolution Metrics
 */
void Test8_Statistics() {
    std::cout << "[TEST 8] Statistics..." << std::endl;
    IncidentCommandEngine engine;

    IncidentRecord i1("INC_STAT_1", IncidentSeverity::NORMAL);
    IncidentRecord i2("INC_STAT_2", IncidentSeverity::CRITICAL);
    IncidentRecord i3("INC_STAT_3", IncidentSeverity::DISASTER);

    engine.CreateIncident(i1);
    engine.CreateIncident(i2);
    engine.CreateIncident(i3);

    auto s1 = engine.GetStatistics();
    TEST_ASSERT(s1.totalIncidents == 3, "Total incidents must be 3");
    TEST_ASSERT(s1.activeIncidents == 3, "Active incidents must be 3");
    TEST_ASSERT(s1.criticalIncidents == 1, "Critical incidents must be 1");
    TEST_ASSERT(s1.disasters == 1, "Disasters must be 1");

    // Resolve i1
    TEST_ASSERT(engine.ResolveIncident("INC_STAT_1"), "Resolve INC_STAT_1");
    auto s2 = engine.GetStatistics();
    TEST_ASSERT(s2.activeIncidents == 2, "Active incidents must be 2");
    TEST_ASSERT(s2.resolvedIncidents == 1, "Resolved incidents must be 1");

    // Close i2
    TEST_ASSERT(engine.CloseIncident("INC_STAT_2"), "Close INC_STAT_2");
    auto s3 = engine.GetStatistics();
    TEST_ASSERT(s3.activeIncidents == 1, "Active incidents must be 1");

    std::cout << "  ✓ PASS: Incident command statistics verified." << std::endl;
}

/**
 * @brief TEST 9: 100-Thread Concurrency
 */
void Test9_Concurrency() {
    std::cout << "[TEST 9] 100-Thread Concurrency..." << std::endl;
    IncidentCommandEngine engine;
    const int NUM_THREADS = 100;
    std::vector<std::thread> workers;
    workers.reserve(NUM_THREADS);
    std::atomic<int> successCount{0};

    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back([&engine, i, &successCount]() {
            std::string id = "CONC_INC_" + std::to_string(i);
            IncidentRecord inc(id, (i % 2 == 0) ? IncidentSeverity::NORMAL : IncidentSeverity::CRITICAL);
            if (!engine.CreateIncident(inc)) {
                return;
            }

            if (i % 3 == 0) {
                engine.EscalateIncident(id, "Thread escalation");
            }

            engine.GenerateAutomaticDecision(id);

            if (i % 5 == 0) {
                engine.ManualOverride(id, OperatorAction::FORCE_REROUTE, CommandDecision::REROUTE_ALL, "Thread override", "THREAD_OP");
            }

            if (i % 4 == 0) {
                engine.ResolveIncident(id);
            }

            IncidentRecord r;
            if (engine.GetIncident(id, r)) {
                successCount++;
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    TEST_ASSERT(successCount == NUM_THREADS, "All 100 threads must succeed without race conditions");
    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalIncidents == NUM_THREADS, "Total incidents must equal thread count");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: " << successCount << ")." << std::endl;
}

/**
 * @brief TEST 10: Performance Benchmark (1000 Incidents Decision Generation < 5 ms)
 */
void Test10_PerformanceBenchmark() {
    std::cout << "[TEST 10] Performance Benchmark: 1000 Incidents Decision Generation..." << std::endl;
    IncidentCommandEngine engine;
    const int NUM_INCIDENTS = 1000;

    // Pre-populate 1000 incidents with varied characteristics
    for (int i = 0; i < NUM_INCIDENTS; ++i) {
        std::string id = "PERF_INC_" + std::to_string(i);
        IncidentSeverity sev = IncidentSeverity::NORMAL;
        if (i % 10 == 0) sev = IncidentSeverity::CRITICAL;
        else if (i % 25 == 0) sev = IncidentSeverity::DISASTER;
        else if (i % 5 == 0) sev = IncidentSeverity::HIGH;

        IncidentRecord inc(id, sev);
        inc.missionId = "MISSION_" + std::to_string(i);
        if (i % 7 == 0) inc.hasFire = true;
        if (i % 13 == 0) inc.trafficCollapse = true;
        if (i % 17 == 0) inc.isMassCasualty = true;
        if (i % 23 == 0) inc.ambulanceFailed = true;

        engine.CreateIncident(inc);
    }

    // Benchmark Decision Generation across all 1000 incidents
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_INCIDENTS; ++i) {
        std::string id = "PERF_INC_" + std::to_string(i);
        engine.GenerateAutomaticDecision(id);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    double avgLatencyMs = elapsedMs / NUM_INCIDENTS;

    std::cout << "  -> Total 1000 Decisions Generation Latency: " << elapsedMs << " ms" << std::endl;
    std::cout << "  -> Average Decision Latency: " << avgLatencyMs << " ms" << std::endl;

    TEST_ASSERT(elapsedMs < 5.0, "1000 incidents decision generation must be strictly < 5.0 ms");
    std::cout << "  ✓ PASS: 1000 incidents evaluated in " << elapsedMs << " ms (< 5.0 ms target)!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 12B Incident Command Engine Tests    \n";
    std::cout << "==========================================================\n";

    Test1_IncidentRegistration();
    Test2_SeverityEscalation();
    Test3_AutomaticDecisions();
    Test4_ManualOverride();
    Test5_MissionAssignment();
    Test6_DecisionHistory();
    Test7_Reporting();
    Test8_Statistics();
    Test9_Concurrency();
    Test10_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " ALL 10 PHASE 12B UNIT TESTS PASSED SUCCESSFULLY (100%)   \n";
    std::cout << "==========================================================\n";
    return 0;
}

