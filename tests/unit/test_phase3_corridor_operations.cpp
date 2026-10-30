/**
 * @file test_phase3_corridor_operations.cpp
 * @brief SMART AEGIS (AERS) Phase 3 - Real-Time Corridor Operations & Fault Recovery Test Suite.
 *
 * Validates the 7 Phase 3 subsystems on Raspberry Pi 4 + QNX SDP 8.0:
 * 1. Corridor Deadline Monitor (Tracking, remaining time, elapsed metrics)
 * 2. Corridor Timeout Detection & Autonomous Recovery Trigger
 * 3. Multi-Domain Emergency Timeout Supervision & Escalation (4 Domains, Deadlock-Free)
 * 4. Deterministic Dynamic Corridor Release (Immediate node-by-node clearance)
 * 5. Dynamic Release Multi-Vehicle Overlap Reservation Safety
 * 6. Real-Time Route Progress Tracking & Dynamic ETA Updates
 * 7. In-Flight Route Recalculation on Downstream Node Failure
 * 8. Deterministic Signal Timeline Generation, Validation & Replay Engine
 * 9. Raspberry Pi Cluster Health Monitor (4 Nodes, Composite Score, Degradation Alerts)
 * 10. Autonomous Fault Recovery Across 5 Failure Classes & Safe Mode Boundary
 */

#include "aegis/corridor/CorridorDeadlineMonitor.hpp"
#include "aegis/corridor/EmergencyTimeoutManager.hpp"
#include "aegis/corridor/DynamicCorridorReleaseManager.hpp"
#include "aegis/corridor/RouteProgressTracker.hpp"
#include "aegis/corridor/SignalTimelineGenerator.hpp"
#include "aegis/cluster/ClusterHealthMonitor.hpp"
#include "aegis/recovery/FaultRecoveryManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] Assertion failed: " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

using namespace aegis;
using namespace aegis::corridor;
using namespace aegis::cluster;
using namespace aegis::recovery;

// ============================================================================
// TEST 1: Corridor Deadline Monitor Tracking & Remaining Margins
// ============================================================================
static bool TestCorridorDeadlineTracking() {
    std::cout << "\n[TEST 1] Corridor Deadline Monitor Tracking & Remaining Margins..." << std::endl;

    CorridorDeadlineConfig config;
    config.slackFactor = 1.5; // ETA 20s -> Deadline 30s
    CorridorDeadlineMonitor monitor(config);

    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C"};
    uint64_t baseTimeUs = 1000000000ULL; // Simulated base epoch

    bool registered = monitor.RegisterCorridor("CORR_01", "AMB_101", route, 20.0, 0.0, baseTimeUs);
    TEST_ASSERT(registered, "Corridor registration should succeed");
    TEST_ASSERT(monitor.GetActiveCorridorCount() == 1, "Should have 1 active corridor");

    CorridorTrackingRecord rec;
    TEST_ASSERT(monitor.GetCorridorInfo("CORR_01", rec), "Must retrieve corridor record");
    TEST_ASSERT(rec.corridorId == "CORR_01", "Corridor ID mismatch");
    TEST_ASSERT(rec.ambulanceId == "AMB_101", "Ambulance ID mismatch");
    TEST_ASSERT(rec.deadlineDurationSec == 30.0, "Deadline duration should be 30.0s (20.0s * 1.5)");
    TEST_ASSERT(rec.status == CorridorDeadlineStatus::ACTIVE, "Initial status must be ACTIVE");

    // Advance to INT_B after 10 seconds
    uint64_t t1Us = baseTimeUs + 10000000ULL;
    monitor.UpdateProgress("CORR_01", "INT_B", t1Us);
    monitor.CheckDeadlines(t1Us);

    TEST_ASSERT(monitor.GetCorridorInfo("CORR_01", rec), "Retrieve updated record");
    TEST_ASSERT(rec.currentIntersection == "INT_B", "Current intersection must be INT_B");
    TEST_ASSERT(std::abs(rec.elapsedTimeSec - 10.0) < 0.01, "Elapsed time must be ~10.0s");
    TEST_ASSERT(std::abs(rec.remainingTimeSec - 20.0) < 0.01, "Remaining time must be ~20.0s");
    TEST_ASSERT(!monitor.IsCorridorTimedOut("CORR_01"), "Corridor must not be timed out");

    // Complete corridor safely at INT_C
    uint64_t tCompleteUs = baseTimeUs + 18000000ULL;
    monitor.CompleteCorridor("CORR_01", tCompleteUs);
    TEST_ASSERT(monitor.GetCorridorInfo("CORR_01", rec), "Retrieve record after complete");
    TEST_ASSERT(rec.status == CorridorDeadlineStatus::COMPLETED, "Status must be COMPLETED");
    TEST_ASSERT(monitor.GetActiveCorridorCount() == 0, "Zero active corridors after completion");

    std::cout << "  ✓ PASS: Corridor deadline tracking and margins verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 2: Corridor Timeout Detection & Autonomous Recovery Trigger
// ============================================================================
static bool TestCorridorTimeoutDetectionAndRecovery() {
    std::cout << "\n[TEST 2] Corridor Timeout Detection & Autonomous Recovery Trigger..." << std::endl;

    CorridorDeadlineConfig config;
    config.autoTriggerRecovery = true;
    CorridorDeadlineMonitor monitor(config);

    bool recoveryHandlerInvoked = false;
    CorridorTimeoutEvent receivedEvent;

    monitor.SetTimeoutRecoveryHandler([&](const CorridorTimeoutEvent& ev) {
        recoveryHandlerInvoked = true;
        receivedEvent = ev;
    });

    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C"};
    uint64_t baseTimeUs = 2000000000ULL;
    // Set fixed deadline of 15.0s
    monitor.RegisterCorridor("CORR_OVERDUE", "AMB_102", route, 10.0, 15.0, baseTimeUs);

    // Simulate clock advancing to 16.5 seconds (overrun by 1.5s)
    uint64_t overdueTimeUs = baseTimeUs + 16500000ULL;
    size_t timedOutCount = monitor.CheckDeadlines(overdueTimeUs);

    TEST_ASSERT(timedOutCount == 1, "Exactly 1 corridor should have timed out");
    TEST_ASSERT(monitor.IsCorridorTimedOut("CORR_OVERDUE"), "Corridor must be flagged TIMED_OUT");
    TEST_ASSERT(recoveryHandlerInvoked, "Autonomous recovery handler must be triggered");
    TEST_ASSERT(receivedEvent.corridorId == "CORR_OVERDUE", "Event corridorId matches");
    TEST_ASSERT(receivedEvent.ambulanceId == "AMB_102", "Event ambulanceId matches");
    TEST_ASSERT(receivedEvent.overrunSec >= 1.49, "Overrun duration must be >= 1.5s");
    TEST_ASSERT(receivedEvent.actionTaken == "TRIGGER_CORRIDOR_RECOVERY", "Recovery action must match");

    auto events = monitor.GetTimeoutEvents();
    TEST_ASSERT(events.size() == 1, "Must store 1 timeout audit event");

    std::string report = monitor.GenerateStatusReport(overdueTimeUs);
    TEST_ASSERT(!report.empty(), "Status report must not be empty");

    std::cout << "  ✓ PASS: Corridor timeout detection and autonomous recovery trigger verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 3: Multi-Domain Emergency Timeout Supervision & Escalation (4 Domains)
// ============================================================================
static bool TestEmergencyTimeoutManagerDomains() {
    std::cout << "\n[TEST 3] Multi-Domain Emergency Timeout Supervision & Escalation (4 Domains)..." << std::endl;

    EmergencyTimeoutPolicy policy;
    policy.ambulanceStallTimeoutSec = 10.0;
    policy.coordinatorInactivityTimeoutSec = 4.0;
    policy.commLinkTimeoutSec = 2.0;
    policy.intersectionAckTimeoutMs = 500.0;
    policy.maxAckRetries = 2;
    policy.autoFailsafeOnTier3 = true;

    EmergencyTimeoutManager timeoutMgr(policy);

    std::vector<TimeoutEscalationEvent> escalationEvents;
    timeoutMgr.SetEscalationHandler([&](const TimeoutEscalationEvent& ev) {
        escalationEvents.push_back(ev);
    });

    uint64_t baseUs = 3000000000ULL;
    timeoutMgr.RegisterRequest("REQ_01", "AMB_103", "INT_A", baseUs);
    timeoutMgr.RecordCoordinatorActivity(baseUs);
    timeoutMgr.RecordNodeCommunication("INT_A", baseUs);
    timeoutMgr.RecordNodeCommunication("INT_B", baseUs);

    // 1. Test Intersection ACK timeout and retries
    timeoutMgr.ExpectIntersectionAck("REQ_01", "INT_A", baseUs);

    // Advance 600ms -> Retry 1
    uint64_t tAck1Us = baseUs + 600000ULL;
    timeoutMgr.CheckTimeouts(tAck1Us);
    TEST_ASSERT(!escalationEvents.empty(), "ACK timeout should generate escalation event");
    TEST_ASSERT(escalationEvents.back().domain == TimeoutDomain::INTERSECTION_ACK, "Domain must be INTERSECTION_ACK");
    TEST_ASSERT(escalationEvents.back().tier == EscalationTier::TIER_1_RETRY, "First retry should be TIER_1_RETRY");

    // Advance another 600ms -> Retry 2
    uint64_t tAck2Us = tAck1Us + 600000ULL;
    timeoutMgr.CheckTimeouts(tAck2Us);

    // Advance another 600ms -> Retries exhausted -> Escalates to TIER 3
    uint64_t tAck3Us = tAck2Us + 600000ULL;
    timeoutMgr.CheckTimeouts(tAck3Us);
    TEST_ASSERT(escalationEvents.back().tier == EscalationTier::TIER_3_FAILSAFE_RECOVERY, "Exhausted retries must escalate to TIER 3");

    // 2. Test Ambulance Stall Detection
    uint64_t tStallUs = baseUs + 12000000ULL; // 12 seconds with no progress
    timeoutMgr.CheckTimeouts(tStallUs);
    TEST_ASSERT(timeoutMgr.IsAmbulanceStalled("REQ_01"), "Ambulance must be flagged stalled");

    // Clear stall by reporting progress
    timeoutMgr.RecordAmbulanceProgress("REQ_01", "INT_B", tStallUs);
    TEST_ASSERT(!timeoutMgr.IsAmbulanceStalled("REQ_01"), "Stall should clear on progress update");

    // 3. Test Coordinator Inactivity
    uint64_t tCoordSilentUs = baseUs + 5000000ULL; // 5 seconds silence > 4s
    TEST_ASSERT(timeoutMgr.IsCoordinatorInactive(tCoordSilentUs), "Coordinator must be flagged inactive");

    // 4. Test Communication Link Timeout
    uint64_t tCommLossUs = baseUs + 3000000ULL; // 3 seconds silence > 2s
    TEST_ASSERT(timeoutMgr.IsNodeCommunicationTimedOut("INT_A", tCommLossUs), "Node communication must time out");

    std::cout << "  ✓ PASS: Multi-domain timeout supervision and deadlock-free escalation verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 4: Deterministic Dynamic Corridor Release (Passage & Clearance Sequence)
// ============================================================================
static bool TestDynamicCorridorReleaseSequence() {
    std::cout << "\n[TEST 4] Deterministic Dynamic Corridor Release (Passage & Clearance Sequence)..." << std::endl;

    DynamicReleaseConfig config;
    config.yellowDurationSec = 3.0;
    config.allRedClearanceSec = 2.0;

    DynamicCorridorReleaseManager releaseMgr(config);

    std::vector<std::pair<std::string, std::string>> signalCommands;
    releaseMgr.SetSignalActionCallback([&](const std::string& node, const std::string& cmd) {
        signalCommands.push_back({node, cmd});
    });

    uint64_t baseUs = 4000000000ULL;

    // Lock INT_A, INT_B, INT_C for corridor CORR_DYN
    releaseMgr.LockIntersection("CORR_DYN", "AMB_104", "INT_A", 3.0, 2.0, baseUs);
    releaseMgr.LockIntersection("CORR_DYN", "AMB_104", "INT_B", 3.0, 2.0, baseUs);
    releaseMgr.LockIntersection("CORR_DYN", "AMB_104", "INT_C", 3.0, 2.0, baseUs);

    TEST_ASSERT(releaseMgr.GetActiveLockedCount() == 3, "All 3 intersections must be locked");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_A") == DynamicReleaseState::LOCKED_EMERGENCY, "INT_A locked");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_B") == DynamicReleaseState::LOCKED_EMERGENCY, "INT_B locked");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_C") == DynamicReleaseState::LOCKED_EMERGENCY, "INT_C locked");

    // Ambulance passes INT_A at t = 5.0s
    uint64_t tPassA = baseUs + 5000000ULL;
    bool passedA = releaseMgr.OnAmbulancePassed("CORR_DYN", "INT_A", tPassA);
    TEST_ASSERT(passedA, "OnAmbulancePassed for INT_A must succeed");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_A") == DynamicReleaseState::CLEARING_YELLOW, "INT_A in CLEARING_YELLOW");

    // Step clock to t = 5 + 3.1s (yellow expires)
    uint64_t tYellowExpired = tPassA + 3100000ULL;
    size_t transitions = releaseMgr.ProcessTransitions(tYellowExpired);
    TEST_ASSERT(transitions == 1, "Must execute 1 transition (Yellow -> AllRed)");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_A") == DynamicReleaseState::ALL_RED_CLEARANCE, "INT_A in ALL_RED_CLEARANCE");

    // Crucial safety check: INT_B and INT_C must still remain locked in emergency!
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_B") == DynamicReleaseState::LOCKED_EMERGENCY, "INT_B STILL LOCKED");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_C") == DynamicReleaseState::LOCKED_EMERGENCY, "INT_C STILL LOCKED");

    // Step clock to t = 5 + 3.1 + 2.1s (all-red expires)
    uint64_t tAllRedExpired = tYellowExpired + 2100000ULL;
    transitions = releaseMgr.ProcessTransitions(tAllRedExpired);
    TEST_ASSERT(transitions == 1, "Must execute 1 transition (AllRed -> RELEASED_NOMINAL)");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_A") == DynamicReleaseState::RELEASED_NOMINAL, "INT_A must be RELEASED_NOMINAL");

    // Verify INT_B and INT_C are STILL locked!
    TEST_ASSERT(releaseMgr.IsIntersectionLocked("INT_B"), "INT_B is still locked for approaching ambulance");
    TEST_ASSERT(releaseMgr.IsIntersectionLocked("INT_C"), "INT_C is still locked for approaching ambulance");
    TEST_ASSERT(!releaseMgr.IsIntersectionLocked("INT_A"), "INT_A is no longer locked");

    std::cout << "  ✓ PASS: Deterministic dynamic corridor release sequence verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 5: Dynamic Corridor Release Multi-Vehicle Overlap Safety
// ============================================================================
static bool TestDynamicReleaseMultiVehicleOverlap() {
    std::cout << "\n[TEST 5] Dynamic Corridor Release Multi-Vehicle Overlap Safety..." << std::endl;

    DynamicReleaseConfig config;
    config.yellowDurationSec = 2.0;
    config.allRedClearanceSec = 1.0;
    DynamicCorridorReleaseManager releaseMgr(config);

    uint64_t baseUs = 5000000000ULL;

    // Ambulance 1 locks INT_B
    releaseMgr.LockIntersection("CORR_AMB1", "AMB_101", "INT_B", 2.0, 1.0, baseUs);
    // Ambulance 2 also claims INT_B (overlap contention)
    releaseMgr.LockIntersection("CORR_AMB2", "AMB_102", "INT_B", 2.0, 1.0, baseUs);

    IntersectionReleaseRecord rec;
    TEST_ASSERT(releaseMgr.GetIntersectionRecord("INT_B", rec), "Must retrieve INT_B record");
    TEST_ASSERT(rec.queuedCorridorClaims.size() == 1, "INT_B must have 1 queued claim for AMB2");

    // Ambulance 1 passes INT_B
    uint64_t tPass = baseUs + 4000000ULL;
    releaseMgr.OnAmbulancePassed("CORR_AMB1", "INT_B", tPass);

    // Step 1: Yellow clearance completes (2.0s duration) -> transitions to ALL_RED_CLEARANCE
    uint64_t tYellowEnd = tPass + 2100000ULL;
    size_t trans1 = releaseMgr.ProcessTransitions(tYellowEnd);
    TEST_ASSERT(trans1 == 1, "Must transition Yellow -> AllRed");
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_B") == DynamicReleaseState::ALL_RED_CLEARANCE,
                "INT_B must be in ALL_RED_CLEARANCE");

    // Step 2: All-Red clearance completes (1.0s duration) -> hands off to queued CORR_AMB2
    uint64_t tAllRedEnd = tYellowEnd + 1100000ULL;
    size_t trans2 = releaseMgr.ProcessTransitions(tAllRedEnd);
    TEST_ASSERT(trans2 == 1, "Must transition AllRed -> Handoff to queued AMB2");

    // Instead of releasing to nominal, INT_B should hand off to AMB2!
    TEST_ASSERT(releaseMgr.GetIntersectionState("INT_B") == DynamicReleaseState::LOCKED_EMERGENCY,
                "INT_B must hand off to queued AMB2 and remain LOCKED_EMERGENCY");
    TEST_ASSERT(releaseMgr.GetIntersectionRecord("INT_B", rec), "Retrieve record");
    TEST_ASSERT(rec.holdingCorridorId == "CORR_AMB2", "Holding corridor must now be CORR_AMB2");

    std::cout << "  ✓ PASS: Multi-vehicle corridor release overlap safety verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 6: Real-Time Route Progress Tracking & Dynamic ETA Updates
// ============================================================================
static bool TestRouteProgressTracking() {
    std::cout << "\n[TEST 6] Real-Time Route Progress Tracking & Dynamic ETA Updates..." << std::endl;

    RouteProgressTracker tracker;
    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C"};
    uint64_t baseUs = 6000000000ULL;

    bool init = tracker.InitializeRoute("AMB_TRACK", "CORR_TRACK", route, 1200.0, 60.0, baseUs);
    TEST_ASSERT(init, "Route tracker init must succeed");
    TEST_ASSERT(tracker.GetCurrentIntersection("AMB_TRACK") == "INT_A", "Current pos is INT_A");
    TEST_ASSERT(tracker.GetNextIntersection("AMB_TRACK") == "INT_B", "Next pos is INT_B");
    TEST_ASSERT(tracker.GetPercentageCompletion("AMB_TRACK") == 0.0, "Initial completion is 0.0%");

    // Arrive at INT_B
    uint64_t tArrB = baseUs + 20000000ULL;
    tracker.ReportArrival("AMB_TRACK", "INT_B", tArrB);
    TEST_ASSERT(tracker.GetCurrentIntersection("AMB_TRACK") == "INT_B", "Current pos updated to INT_B");

    // Depart INT_B
    uint64_t tDepB = baseUs + 25000000ULL;
    tracker.ReportDeparture("AMB_TRACK", "INT_B", tDepB);
    double pct = tracker.GetPercentageCompletion("AMB_TRACK");
    TEST_ASSERT(pct >= 49.0 && pct <= 51.0, "Progress should be 50.0% after clearing 1 of 2 hops");
    TEST_ASSERT(tracker.GetNextIntersection("AMB_TRACK") == "INT_C", "Next intersection is now INT_C");

    // Dynamic speed update
    tracker.UpdateVelocity("AMB_TRACK", 25.0); // 25 m/s (~90 km/h)
    double updatedEta = tracker.GetUpdatedEtaSeconds("AMB_TRACK");
    TEST_ASSERT(updatedEta > 0.0 && updatedEta < 60.0, "Updated ETA must be lower due to higher speed");

    // Depart final node INT_C (arrival at destination)
    uint64_t tDepC = baseUs + 45000000ULL;
    tracker.ReportDeparture("AMB_TRACK", "INT_C", tDepC);
    TEST_ASSERT(tracker.IsRouteCompleted("AMB_TRACK"), "Route must be completed");
    TEST_ASSERT(tracker.GetPercentageCompletion("AMB_TRACK") == 100.0, "Percentage completion must be 100.0%");

    std::cout << "  ✓ PASS: Route progress tracking and dynamic ETA updates verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 7: In-Flight Route Recalculation on Downstream Node Failure
// ============================================================================
static bool TestInFlightRouteRecalculation() {
    std::cout << "\n[TEST 7] In-Flight Route Recalculation on Downstream Node Failure..." << std::endl;

    RouteProgressTracker tracker;
    std::vector<std::string> route = {"INT_A", "INT_B", "INT_C"};
    uint64_t baseUs = 7000000000ULL;

    tracker.InitializeRoute("AMB_REROUTE", "CORR_REROUTE", route, 1000.0, 50.0, baseUs);

    // Ambulance reaches INT_A
    tracker.ReportArrival("AMB_REROUTE", "INT_A", baseUs + 1000000ULL);
    tracker.ReportDeparture("AMB_REROUTE", "INT_A", baseUs + 2000000ULL);

    // Downstream INT_B fails! Install custom replanner that routes around INT_B via BYPASS_1
    tracker.SetRouteReplanner([](const std::string& current, const std::string& dest, const std::string& failed) {
        (void)failed;
        std::vector<std::string> alt;
        alt.push_back(current);
        alt.push_back("INT_BYPASS_1");
        alt.push_back(dest);
        return alt;
    });

    bool replanned = tracker.RecalculateRouteOnFailure("AMB_REROUTE", "INT_B", baseUs + 3000000ULL);
    TEST_ASSERT(replanned, "In-flight route recalculation must succeed");

    RouteProgressState state;
    TEST_ASSERT(tracker.GetProgress("AMB_REROUTE", state), "Retrieve progress state");
    TEST_ASSERT(state.isRerouted, "isRerouted flag must be true");
    TEST_ASSERT(state.recalculationCount == 1, "Recalculation count must be 1");
    TEST_ASSERT(state.nextIntersection == "INT_BYPASS_1", "Next intersection must now be INT_BYPASS_1");

    auto remaining = tracker.GetRemainingIntersections("AMB_REROUTE");
    TEST_ASSERT(remaining.size() == 2, "Remaining hops should be INT_BYPASS_1 and INT_C");
    TEST_ASSERT(remaining[0] == "INT_BYPASS_1", "First remaining is INT_BYPASS_1");
    TEST_ASSERT(remaining[1] == "INT_C", "Second remaining is INT_C");

    std::cout << "  ✓ PASS: In-flight route recalculation on failure verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 8: Deterministic Signal Timeline Generation, Validation & Replay Engine
// ============================================================================
static bool TestSignalTimelineGenerationAndReplay() {
    std::cout << "\n[TEST 8] Deterministic Signal Timeline Generation, Validation & Replay Engine..." << std::endl;

    SignalTimelineGenerator generator;
    std::vector<std::string> nodes = {"INT_A", "INT_B", "INT_C"};
    std::vector<double> etas = {10.0, 25.0, 40.0};
    uint64_t baseUs = 8000000000ULL;

    CorridorTimelineSchedule sched = generator.GenerateTimeline(
        "CORR_TIME", "AMB_105", nodes, etas, 10.0, 3.0, 2.0, baseUs);

    TEST_ASSERT(sched.isValid, "Generated timeline schedule must be valid");
    TEST_ASSERT(!sched.events.empty(), "Events list must not be empty");
    TEST_ASSERT(sched.events.size() == 18, "3 intersections * 6 phases = 18 total events");

    std::string failureReason;
    TEST_ASSERT(generator.ValidateTimeline(sched, failureReason), "Timeline safety validation must pass");

    // Test record execution & drift calculation
    uint64_t event1Id = sched.events[0].eventId;
    uint64_t actualExecutionUs = sched.events[0].scheduledStartUs + 420; // 420 us drift
    generator.RecordExecution(sched.scheduleId, event1Id, actualExecutionUs);

    CorridorTimelineSchedule retrieved;
    TEST_ASSERT(generator.GetSchedule(sched.scheduleId, retrieved), "Retrieve schedule");
    TEST_ASSERT(retrieved.events[0].isExecuted, "Event 1 must be marked executed");
    TEST_ASSERT(retrieved.events[0].driftUs == 420, "Drift must be exactly 420 us");

    // Test Replay functionality
    size_t replayedCount = 0;
    bool replayOk = generator.ReplayTimeline(sched.scheduleId, [&](const SignalTimelineEvent& ev) {
        (void)ev;
        replayedCount++;
    });
    TEST_ASSERT(replayOk, "Replay must succeed");
    TEST_ASSERT(replayedCount == 18, "All 18 events must be replayed in order");

    std::string jsonExport = generator.ExportToJson(sched.scheduleId);
    TEST_ASSERT(jsonExport.find("scheduleId") != std::string::npos, "JSON export contains scheduleId");
    TEST_ASSERT(jsonExport.find("events") != std::string::npos, "JSON export contains events");

    std::string asciiTable = generator.ExportToAsciiTable(sched.scheduleId);
    TEST_ASSERT(!asciiTable.empty(), "ASCII table export must not be empty");

    std::cout << "  ✓ PASS: Signal timeline generation, validation, drift and replay verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 9: Raspberry Pi Cluster Health Monitor (4 Nodes, Composite Score)
// ============================================================================
static bool TestClusterHealthMonitoring() {
    std::cout << "\n[TEST 9] Raspberry Pi Cluster Health Monitor (4 Nodes, Composite Score)..." << std::endl;

    ClusterHealthMonitor clusterMon;

    // Register 4 Raspberry Pi boards: 1 Coordinator + 3 Intersections
    clusterMon.RegisterNode("COORD_01", "rpi4-coord", "192.168.1.100", true);
    clusterMon.RegisterNode("INT_A", "rpi4-alpha", "192.168.1.101", false);
    clusterMon.RegisterNode("INT_B", "rpi4-bravo", "192.168.1.102", false);
    clusterMon.RegisterNode("INT_C", "rpi4-charlie", "192.168.1.103", false);

    uint64_t baseUs = 9000000000ULL;
    auto summary = clusterMon.EvaluateCluster(baseUs);

    TEST_ASSERT(summary.totalNodes == 4, "Total nodes must be 4");
    TEST_ASSERT(summary.healthyNodes == 4, "All 4 nodes initially healthy");
    TEST_ASSERT(summary.degradedNodes == 0, "Zero degraded nodes initially");
    TEST_ASSERT(summary.overallClusterScore >= 95.0, "Cluster score must be >= 95.0%");
    TEST_ASSERT(summary.state == ClusterState::OPTIMAL, "Cluster state must be OPTIMAL");

    // Inject degradation on INT_C: High CPU, High Temp, and Network Loss
    NodeHealthTelemetry telC;
    telC.nodeId = "INT_C";
    telC.hostname = "rpi4-charlie";
    telC.ipAddress = "192.168.1.103";
    telC.isCoordinator = false;
    telC.cpuUsagePercent = 94.0;   // High CPU penalty
    telC.cpuTemperatureC = 82.5;   // High Temp penalty
    telC.memoryUsagePercent = 40.0;
    telC.networkLevel = aegis::hardware::LinkQualityLevel::POOR; // Network QoS penalty
    telC.gpioHealthy = true;
    telC.relayHealthy = true;
    telC.watchdogHealthy = true;

    clusterMon.UpdateNodeTelemetry(telC, baseUs + 500000ULL);
    summary = clusterMon.EvaluateCluster(baseUs + 500000ULL);

    TEST_ASSERT(clusterMon.IsClusterDegraded(), "Cluster must now detect degraded condition");
    TEST_ASSERT(summary.degradedNodes == 1, "Exactly 1 node must be degraded");

    NodeHealthTelemetry retrievedC;
    clusterMon.GetNodeTelemetry("INT_C", retrievedC);
    TEST_ASSERT(retrievedC.isDegraded, "INT_C must be flagged degraded");
    TEST_ASSERT(retrievedC.nodeScore < 75.0, "INT_C score must have dropped below 75.0");

    std::string dashboard = clusterMon.GenerateAsciiDashboard(baseUs + 500000ULL);
    TEST_ASSERT(dashboard.find("DEGRADED") != std::string::npos, "Dashboard reflects degradation");

    std::cout << "  ✓ PASS: Cluster health monitoring (4 nodes, composite scoring) verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 10: Autonomous Fault Recovery Across 5 Failure Classes & Safe Mode
// ============================================================================
static bool TestAutonomousFaultRecoveryEngine() {
    std::cout << "\n[TEST 10] Autonomous Fault Recovery Across 5 Failure Classes & Safe Mode..." << std::endl;

    FaultRecoveryManager recMgr;

    // 1. Recover from Communication Failure (No restart)
    bool recComm = recMgr.RecoverCommunicationFailure("INT_B");
    TEST_ASSERT(recComm, "Communication failure recovery must succeed");
    TEST_ASSERT(recMgr.CanResumeWithoutSystemRestart(FaultCategory::COMMUNICATION_FAILURE), "Can resume without restart");

    // 2. Recover from Watchdog Event (Subsystem reset without restart)
    bool recWatchdog = recMgr.RecoverWatchdogEvent("RelayController", "Feedback readback transient mismatch");
    TEST_ASSERT(recWatchdog, "Watchdog event recovery must succeed");
    TEST_ASSERT(recMgr.CanResumeWithoutSystemRestart(FaultCategory::WATCHDOG_EVENT), "Can resume without restart");

    // 3. Recover from Node Reboot (Corridor state push without restart)
    bool recReboot = recMgr.RecoverNodeReboot("INT_C", 1000000ULL);
    TEST_ASSERT(recReboot, "Node reboot recovery must succeed");
    TEST_ASSERT(recMgr.CanResumeWithoutSystemRestart(FaultCategory::NODE_REBOOT), "Can resume without restart");

    // 4. Recover from Temporary Network Loss (Autonomous optical fallback and resync)
    bool recNetLoss = recMgr.RecoverTemporaryNetworkLoss("INT_A", false); // Dropped link
    TEST_ASSERT(recNetLoss, "Network loss handling must succeed");
    bool recNetRestore = recMgr.RecoverTemporaryNetworkLoss("INT_A", true); // Restored link
    TEST_ASSERT(recNetRestore, "Network restoration resync must succeed");
    TEST_ASSERT(recMgr.CanResumeWithoutSystemRestart(FaultCategory::TEMPORARY_NETWORK_LOSS), "Can resume without restart");

    // 5. Recover from Corridor Interruption (Replan corridor and release obsolete locks)
    bool recCorridor = recMgr.RecoverCorridorInterruption("CORR_INT", "INT_B");
    TEST_ASSERT(recCorridor, "Corridor interruption recovery must succeed");
    TEST_ASSERT(recMgr.CanResumeWithoutSystemRestart(FaultCategory::CORRIDOR_INTERRUPTION), "Can resume without restart");

    TEST_ASSERT(!recMgr.IsInSafeMode(), "System must NOT be in Safe Mode after recoverable faults");
    TEST_ASSERT(recMgr.GetTotalRecoveries() == 6, "Total recoveries must be 6");
    TEST_ASSERT(recMgr.GetSuccessfulRecoveries() == 6, "All 6 recoveries must be successful");

    // 6. Safe Mode Boundary Verification: Fatal unrecoverable hardware destruction
    bool safeModeTriggered = false;
    recMgr.SetSafeModeAction([&](const std::string& target, const std::string& reason) {
        (void)target;
        (void)reason;
        safeModeTriggered = true;
    });

    recMgr.TriggerSafeMode("INT_ALL", "Catastrophic unrecoverable bus destruction");
    TEST_ASSERT(recMgr.IsInSafeMode(), "System MUST enter Safe Mode upon unrecoverable fault");
    TEST_ASSERT(safeModeTriggered, "Safe mode action callback must be invoked");
    TEST_ASSERT(!recMgr.CanResumeWithoutSystemRestart(FaultCategory::FATAL_HARDWARE_FAILURE), "Fatal failure cannot resume without restart");

    std::string report = recMgr.GenerateRecoveryAuditReport();
    TEST_ASSERT(!report.empty(), "Audit report must be generated");

    std::cout << "  ✓ PASS: Autonomous fault recovery (5 classes & Safe Mode boundary) verified." << std::endl;
    return true;
}

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 3 Real-Time Corridor Operations       " << std::endl;
    std::cout << " Raspberry Pi 4 + QNX SDP 8.0 Real-Time Subsystem Tests   " << std::endl;
    std::cout << "==========================================================" << std::endl;

    int passed = 0;
    int failed = 0;

    auto RunTest = [&](bool (*testFunc)(), const char* testName) {
        if (testFunc()) {
            passed++;
        } else {
            failed++;
            std::cerr << ">>> [FAILED] " << testName << std::endl;
        }
    };

    RunTest(TestCorridorDeadlineTracking, "TestCorridorDeadlineTracking");
    RunTest(TestCorridorTimeoutDetectionAndRecovery, "TestCorridorTimeoutDetectionAndRecovery");
    RunTest(TestEmergencyTimeoutManagerDomains, "TestEmergencyTimeoutManagerDomains");
    RunTest(TestDynamicCorridorReleaseSequence, "TestDynamicCorridorReleaseSequence");
    RunTest(TestDynamicReleaseMultiVehicleOverlap, "TestDynamicReleaseMultiVehicleOverlap");
    RunTest(TestRouteProgressTracking, "TestRouteProgressTracking");
    RunTest(TestInFlightRouteRecalculation, "TestInFlightRouteRecalculation");
    RunTest(TestSignalTimelineGenerationAndReplay, "TestSignalTimelineGenerationAndReplay");
    RunTest(TestClusterHealthMonitoring, "TestClusterHealthMonitoring");
    RunTest(TestAutonomousFaultRecoveryEngine, "TestAutonomousFaultRecoveryEngine");

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " Phase 3 Corridor Operations Validation Summary:          " << std::endl;
    std::cout << " Total Passed: " << passed << "/10                        " << std::endl;
    std::cout << " Total Failed: " << failed << "/10                        " << std::endl;
    std::cout << "==========================================================" << std::endl;

    if (failed == 0) {
        std::cout << ">>> [SUCCESS] All 10 Phase 3 Subsystem Tests Passed 100%!" << std::endl;
        return 0;
    } else {
        std::cerr << ">>> [FAILURE] Some Phase 3 Subsystem Tests Failed!" << std::endl;
        return 1;
    }
}
