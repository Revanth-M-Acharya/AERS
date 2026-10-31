/**
 * @file test_coordinator_orchestration.cpp
 * @brief Comprehensive Unit & Integration Tests for Phase 5 Coordinator Orchestration & Distributed Runtime.
 * Target: Raspberry Pi 4 Model B (QNX Neutrino RTOS 8.0 / POSIX).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/EventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "coordinator/orchestration/fsm/CoordinatorFSM.hpp"
#include "coordinator/orchestration/dispatcher/ExecutionDispatcher.hpp"
#include "coordinator/orchestration/sync/CoordinatorClock.hpp"
#include "coordinator/orchestration/sync/NodeClockTracker.hpp"
#include "coordinator/orchestration/monitor/NodeHealthMonitor.hpp"
#include "coordinator/orchestration/monitor/ExecutionMonitor.hpp"
#include "coordinator/orchestration/recovery/RecoveryManager.hpp"
#include "coordinator/orchestration/config/OrchestrationConfig.hpp"
#include "coordinator/intelligence/execution/ExecutionPlanGenerator.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"

#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis;
using namespace aegis::coordinator;
using namespace aegis::coordinator::orchestration;
using namespace aegis::coordinator::intelligence;

// Helper to construct a valid test execution plan
static ExecutionPlan CreateTestExecutionPlan(const std::string& planId,
                                             const std::string& ambulanceId,
                                             const std::vector<std::string>& intersections,
                                             uint64_t startMonoMs) {
    ExecutionPlan plan;
    plan.planId = planId;
    plan.ambulanceId = ambulanceId;
    plan.priorityLevel = 1;
    plan.timestampMs = startMonoMs;
    plan.orderedIntersections = intersections;
    plan.rollbackPlan = "RESTORE_NORMAL_MODE";

    uint64_t stageTime = startMonoMs;
    for (const auto& intId : intersections) {
        CorridorScheduleItem item;
        item.intersectionId = intId;
        item.signalCommand = "SET_GREEN";
        item.activationTimeMonoMs = stageTime;
        item.holdDurationMs = 15000;
        item.releaseCondition = "AMBULANCE_PASSED_OR_TIMEOUT";
        plan.signalCommands.push_back(item);
        stageTime += 10000;
    }

    plan.checksum = plan.ComputeChecksum();
    return plan;
}

int main() {
    std::cout << "==========================================================\n"
              << " SMART AEGIS: Coordinator Orchestration (Phase 5) Tests   \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n"
              << "==========================================================\n\n";

    CoordinatorLogger::Instance().Initialize("", false, LogLevel::INFO);

    // ------------------------------------------------------------------------
    // TEST 1: Coordinator FSM Transitions (Legal and Illegal)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 1] Coordinator FSM Legal & Illegal Transitions...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        CoordinatorFSM fsm(stateMgr);

        assert(fsm.GetCurrentState() == CoordinatorState::IDLE);
        assert(stateMgr->GetSystemState() == "IDLE");

        // Illegal: IDLE -> EXECUTING directly
        assert(!fsm.CanTransitionTo(CoordinatorState::EXECUTING));
        assert(!fsm.TransitionTo(CoordinatorState::EXECUTING));
        assert(fsm.GetCurrentState() == CoordinatorState::IDLE);

        // Legal: IDLE -> REGISTERING -> IDLE
        assert(fsm.TransitionTo(CoordinatorState::REGISTERING));
        assert(fsm.GetCurrentState() == CoordinatorState::REGISTERING);
        assert(fsm.TransitionTo(CoordinatorState::IDLE));

        // Full legal emergency progression
        assert(fsm.TransitionTo(CoordinatorState::ROUTE_PLANNING));
        assert(fsm.TransitionTo(CoordinatorState::PLAN_GENERATION));
        assert(fsm.TransitionTo(CoordinatorState::PLAN_VALIDATION));
        assert(fsm.TransitionTo(CoordinatorState::DISPATCHING));
        assert(fsm.TransitionTo(CoordinatorState::EXECUTING));
        assert(fsm.TransitionTo(CoordinatorState::MONITORING));
        assert(fsm.TransitionTo(CoordinatorState::COMPLETED));
        assert(fsm.TransitionTo(CoordinatorState::IDLE));

        // Replanning flow: MONITORING -> REPLANNING -> ROUTE_PLANNING
        assert(fsm.TransitionTo(CoordinatorState::ROUTE_PLANNING));
        assert(fsm.TransitionTo(CoordinatorState::PLAN_GENERATION));
        assert(fsm.TransitionTo(CoordinatorState::PLAN_VALIDATION));
        assert(fsm.TransitionTo(CoordinatorState::DISPATCHING));
        assert(fsm.TransitionTo(CoordinatorState::EXECUTING));
        assert(fsm.TransitionTo(CoordinatorState::MONITORING));
        assert(fsm.TransitionTo(CoordinatorState::REPLANNING));
        assert(fsm.TransitionTo(CoordinatorState::ROUTE_PLANNING));

        fsm.Reset();
        assert(fsm.GetCurrentState() == CoordinatorState::IDLE);

        // State Change Callback Verification
        bool callbackTriggered = false;
        CoordinatorState cbOld = CoordinatorState::IDLE;
        CoordinatorState cbNew = CoordinatorState::IDLE;
        std::string cbReason;
        fsm.SetStateChangeCallback([&](CoordinatorState from, CoordinatorState to, const std::string& reason) {
            callbackTriggered = true;
            cbOld = from;
            cbNew = to;
            cbReason = reason;
        });

        assert(fsm.TransitionTo(CoordinatorState::REGISTERING, "Nodes connected"));
        assert(callbackTriggered);
        assert(cbOld == CoordinatorState::IDLE);
        assert(cbNew == CoordinatorState::REGISTERING);
        assert(cbReason == "Nodes connected");

        std::cout << "  ✓ PASS: Legal transitions, strict illegal transition rejection, and transition callbacks verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 2: Emergency Session Lifecycle
    // ------------------------------------------------------------------------
    std::cout << "[TEST 2] Emergency Session Lifecycle Management...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        EmergencySessionManager sessionMgr(stateMgr, eventBus);

        uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
        assert(sessionMgr.CreateSession("EMG-001", "AMB-01", "HOSP-01", 1, now));

        // Duplicate creation rejected
        assert(!sessionMgr.CreateSession("EMG-001", "AMB-01", "HOSP-01", 1, now));

        auto optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess.has_value());
        assert(optSess->sessionState == SessionState::CREATED);

        // Activate
        assert(sessionMgr.ActivateSession("EMG-001", now + 100));
        optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess->sessionState == SessionState::ACTIVE);
        assert(optSess->corridorState == CorridorState::ACTIVE);

        // Pause and Resume
        assert(sessionMgr.PauseSession("EMG-001", "Traffic congestion pause", now + 200));
        optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess->sessionState == SessionState::PAUSED);

        assert(sessionMgr.ResumeSession("EMG-001", now + 300));
        optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess->sessionState == SessionState::ACTIVE);

        // Complete
        assert(sessionMgr.CompleteSession("EMG-001", now + 1000));
        optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess->sessionState == SessionState::COMPLETED);
        assert(optSess->corridorState == CorridorState::CLEARED);

        // Archive
        assert(sessionMgr.ArchiveSession("EMG-001", now + 1100));
        optSess = sessionMgr.GetSession("EMG-001");
        assert(optSess->sessionState == SessionState::ARCHIVED);

        // Cancelled Mission Lifecycle Path
        assert(sessionMgr.CreateSession("EMG-CANCEL-01", "AMB-99", "HOSP-01", 1, now + 1200));
        assert(sessionMgr.ActivateSession("EMG-CANCEL-01", now + 1250));
        assert(sessionMgr.CancelSession("EMG-CANCEL-01", "False alarm", now + 1300));
        optSess = sessionMgr.GetSession("EMG-CANCEL-01");
        assert(optSess.has_value());
        assert(optSess->sessionState == SessionState::CANCELLED);
        assert(sessionMgr.ArchiveSession("EMG-CANCEL-01", now + 1400));
        assert(sessionMgr.GetSession("EMG-CANCEL-01")->sessionState == SessionState::ARCHIVED);

        std::cout << "  ✓ PASS: Complete emergency lifecycle (Create->Activate->Pause->Resume->Cancel->Complete->Archive) verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 3: State Manager Single Source of Truth & Concurrency
    // ------------------------------------------------------------------------
    std::cout << "[TEST 3] State Manager Single Source of Truth & Concurrency...\n";
    {
        CoordinatorStateManager stateMgr;

        TrafficNodeRecord nodeA{"INT_A", "192.168.1.10", 8080, 1000, 1000, true, "RED"};
        TrafficNodeRecord nodeB{"INT_B", "192.168.1.11", 8080, 1000, 1000, true, "RED"};
        stateMgr.RegisterTrafficNode(nodeA);
        stateMgr.RegisterTrafficNode(nodeB);

        HospitalRecord hosp{"HOSP_CITY", "City Hospital", "Central", 100, 25, 1};
        stateMgr.RegisterHospital(hosp);

        AmbulanceRecord amb{"AMB_01", "Paramedic-1", 1, "INT_A", "HOSP_CITY", 60.0, true};
        stateMgr.RegisterAmbulance(amb);

        assert(stateMgr.GetTrafficNodeCount() == 2);
        assert(stateMgr.HasTrafficNode("INT_A"));
        assert(stateMgr.GetHospital("HOSP_CITY").has_value());
        assert(stateMgr.GetAmbulance("AMB_01").has_value());

        // Multi-threaded concurrent reads and updates
        std::atomic<bool> run{true};
        std::vector<std::thread> readers;
        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&stateMgr, &run]() {
                while (run.load()) {
                    auto nodes = stateMgr.GetAllTrafficNodes();
                    auto health = stateMgr.GetAllNodeHealth();
                    auto hosps = stateMgr.GetAllHospitals();
                    assert(hosps.size() >= 1);
                }
            });
        }

        for (int i = 0; i < 100; ++i) {
            stateMgr.UpdateHospitalCapacity("HOSP_CITY", static_cast<uint32_t>(20 + (i % 10)));
        }

        run.store(false);
        for (auto& t : readers) {
            t.join();
        }

        assert(stateMgr.GetHospital("HOSP_CITY")->availableBeds >= 20);
        std::cout << "  ✓ PASS: Thread-safe read/write and single source of truth queries verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 4: Dispatcher Acknowledgement & Packet Tracking
    // ------------------------------------------------------------------------
    std::cout << "[TEST 4] Dispatcher Acknowledgement & Packet Tracking...\n";
    {
        std::vector<std::pair<std::string, protocol::Packet>> sentPackets;
        auto sender = [&sentPackets](const std::string& nodeId, const protocol::Packet& pkt) -> bool {
            sentPackets.push_back({nodeId, pkt});
            return true;
        };

        auto eventBus = std::make_shared<EventBus>();
        ExecutionDispatcher dispatcher(sender, eventBus);

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-001", "AMB-01", {"INT_A", "INT_B"}, 1000);
        assert(dispatcher.DispatchPlan(plan, 1000));
        assert(sentPackets.size() == 2);
        assert(dispatcher.GetPendingAckCount("PLAN-001") == 2);
        assert(!dispatcher.IsPlanFullyAcknowledged("PLAN-001"));

        // ACK first node via wire Packet (INT_A, seq=1)
        protocol::Packet ackPacket(protocol::MessageType::ACK, "INT_A", "SEQ=1;CRC=" + std::to_string(plan.checksum), 1050);
        assert(dispatcher.ProcessAckPacket("INT_A", ackPacket, 1050));
        assert(dispatcher.GetPendingAckCount("PLAN-001") == 1);
        assert(!dispatcher.IsPlanFullyAcknowledged("PLAN-001"));

        // ACK second node (INT_B, seq=2)
        assert(dispatcher.ProcessAck("INT_B", 2, plan.checksum, 1100));
        assert(dispatcher.GetPendingAckCount("PLAN-001") == 0);
        assert(dispatcher.IsPlanFullyAcknowledged("PLAN-001"));

        std::cout << "  ✓ PASS: Execution plan staged dispatch and complete ACK tracking verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 5: Dispatcher Timeout & Max Retry Count
    // ------------------------------------------------------------------------
    std::cout << "[TEST 5] Dispatcher Timeout & Max Retry Limit...\n";
    {
        uint32_t transmitCount = 0;
        auto sender = [&transmitCount](const std::string&, const protocol::Packet&) -> bool {
            transmitCount++;
            return true;
        };

        auto eventBus = std::make_shared<EventBus>();
        ExecutionDispatcher dispatcher(sender, eventBus, nullptr, 3, 1000, 200);

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-RETRY", "AMB-02", {"INT_X"}, 1000);
        assert(dispatcher.DispatchPlan(plan, 1000));
        assert(transmitCount == 1);

        // Advance 1100ms -> 1st retry
        dispatcher.CheckTimeoutsAndRetry(2100);
        assert(transmitCount == 2);

        // Advance another 1100ms -> 2nd retry
        dispatcher.CheckTimeoutsAndRetry(3200);
        assert(transmitCount == 3);

        // Advance another 1100ms -> 3rd retry
        dispatcher.CheckTimeoutsAndRetry(4300);
        assert(transmitCount == 4);

        // Advance another 1100ms -> exceeds maxRetries (3 retries total)
        dispatcher.CheckTimeoutsAndRetry(5400);
        assert(dispatcher.HasPlanDispatchFailed("PLAN-RETRY"));

        std::cout << "  ✓ PASS: Automatic retries and max retry limit enforcement verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 6: Dispatcher Duplicate Rejection & CRC Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 6] Dispatcher Duplicate Rejection & CRC Validation...\n";
    {
        auto sender = [](const std::string&, const protocol::Packet&) -> bool { return true; };
        ExecutionDispatcher dispatcher(sender);

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-CRC", "AMB-03", {"INT_Y"}, 1000);

        // Corrupt plan checksum
        plan.checksum = 0xDEAD;
        assert(!dispatcher.DispatchPlan(plan, 1000)); // Rejected due to invalid CRC

        // Restore valid checksum
        plan.checksum = plan.ComputeChecksum();
        assert(dispatcher.DispatchPlan(plan, 1000));

        // Process ACK
        assert(dispatcher.ProcessAck("INT_Y", 1, plan.checksum, 1050));

        // Duplicate ACK with same sequence number rejected
        assert(!dispatcher.ProcessAck("INT_Y", 1, plan.checksum, 1060));

        std::cout << "  ✓ PASS: CRC16 integrity validation and duplicate ACK rejection verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 7: Time Synchronization & Node Clock Tracking
    // ------------------------------------------------------------------------
    std::cout << "[TEST 7] Time Synchronization & Node Clock Tracking...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        NodeClockTracker tracker(stateMgr, nullptr, 500);

        // Symmetric sync sample: t1=1000, t2=1010, t3=1020, t4=1030
        // Offset = ((1010-1000) + (1020-1030))/2 = (10 - 10)/2 = 0 ms, RTT = 30 - 10 = 20 ms
        tracker.RecordPtpSample("INT_A", 1000, 1010, 1020, 1030);
        assert(tracker.GetClockOffsetMs("INT_A") == 0);
        assert(tracker.GetRoundTripTimeMs("INT_A") == 20);
        assert(tracker.IsNodeSynchronized("INT_A"));
        assert(!tracker.IsClockDriftDetected("INT_A"));

        // Skewed clock: client clock is 600ms ahead
        // t1=1600, t2=1010, t3=1020, t4=1630
        // diff1 = 1010 - 1600 = -590
        // diff2 = 1020 - 1630 = -610
        // Offset = (-590 + -610)/2 = -600 ms
        tracker.RecordPtpSample("INT_SKEWED", 1600, 1010, 1020, 1630);
        assert(tracker.GetClockOffsetMs("INT_SKEWED") == -600);
        assert(tracker.IsClockDriftDetected("INT_SKEWED"));
        assert(!tracker.IsNodeSynchronized("INT_SKEWED"));

        // 2-Way Ping-Pong Sample: send=1000, recv=1040, remote=1010
        // RTT = 40ms, estimated remote on recv = 1010 + 20 = 1030
        // Offset = 1040 - 1030 = 10ms
        tracker.RecordPingSample("INT_PING", 1000, 1040, 1010);
        assert(tracker.GetRoundTripTimeMs("INT_PING") == 40);
        assert(tracker.GetClockOffsetMs("INT_PING") == 10);
        assert(tracker.IsNodeSynchronized("INT_PING"));

        // Verify ExecutionTimeTags generation
        auto tags = CoordinatorClock::CreateTimeTags(100, 5000, 15000, 3000);
        assert(tags.dispatchTimeMonoMs >= tags.creationTimeMonoMs + 100);
        assert(tags.executionTimeMonoMs == tags.dispatchTimeMonoMs + 5000);
        assert(tags.expirationTimeMonoMs == tags.executionTimeMonoMs + 15000 + 3000);

        std::cout << "  ✓ PASS: PTP offset/RTT calculation, clock drift detection, and time tags verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 8: Node Health Monitor Status Transitions (ONLINE -> OFFLINE -> RECOVERING)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 8] Node Health Monitor Status Transitions...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        NodeHealthMonitor monitor(stateMgr, eventBus, 5000);

        uint64_t t = 10000;
        monitor.RecordHeartbeat("INT_01", 15.0, 25.0, 45.0, 10, t);
        assert(monitor.GetNodeHealthState("INT_01") == NodeHealthState::ONLINE);

        // Advance 5500ms without heartbeat -> OFFLINE
        monitor.CheckHealthTimeouts(t + 5500);
        assert(monitor.GetNodeHealthState("INT_01") == NodeHealthState::OFFLINE);

        // Resume heartbeat -> transitions to RECOVERING
        monitor.RecordHeartbeat("INT_01", 14.0, 24.0, 44.0, 10, t + 6000);
        assert(monitor.GetNodeHealthState("INT_01") == NodeHealthState::RECOVERING);

        // Subsequent heartbeat -> transitions to ONLINE
        monitor.RecordHeartbeat("INT_01", 14.0, 24.0, 44.0, 10, t + 7000);
        assert(monitor.GetNodeHealthState("INT_01") == NodeHealthState::ONLINE);

        std::cout << "  ✓ PASS: ONLINE -> OFFLINE -> RECOVERING -> ONLINE state progression verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 9: Heartbeat Loss Detection & Degradation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 9] Heartbeat Loss Detection & Degradation to FAILED...\n";
    {
        NodeHealthMonitor monitor(nullptr, nullptr, 1000);

        uint64_t t = 1000;
        monitor.RecordHeartbeat("INT_FAULTY", 10.0, 20.0, 40.0, 5, t);
        assert(monitor.GetNodeHealthState("INT_FAULTY") == NodeHealthState::ONLINE);

        // 1st missed heartbeat -> OFFLINE
        monitor.CheckHealthTimeouts(t + 1100);
        assert(monitor.GetNodeHealthState("INT_FAULTY") == NodeHealthState::OFFLINE);

        // 2nd missed interval
        monitor.CheckHealthTimeouts(t + 2200);

        // 3rd consecutive missed interval -> degraded to FAILED
        monitor.CheckHealthTimeouts(t + 3300);
        assert(monitor.GetNodeHealthState("INT_FAULTY") == NodeHealthState::FAILED);

        auto failedList = monitor.GetFailedNodes();
        assert(failedList.size() == 1 && failedList[0] == "INT_FAULTY");

        std::cout << "  ✓ PASS: Node unresponsiveness degradation to FAILED status verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 10: Execution Monitor Plan Progress & Signal Completion
    // ------------------------------------------------------------------------
    std::cout << "[TEST 10] Execution Monitor Plan Progress & Signal Completion...\n";
    {
        auto eventBus = std::make_shared<EventBus>();
        ExecutionMonitor monitor(nullptr, eventBus);

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-PROG", "AMB-01", {"INT_A", "INT_B"}, 1000);
        monitor.StartSupervision(plan, 30.0, 1000);

        assert(!monitor.IsPlanCompleted("PLAN-PROG"));

        // Stage 1 Green Confirmed
        monitor.RecordSignalConfirmation("PLAN-PROG", "INT_A", "GREEN", 1050);
        monitor.RecordAmbulancePassage("PLAN-PROG", "INT_A", 1500);
        assert(monitor.IsStageCompleted("PLAN-PROG", "INT_A"));
        assert(!monitor.IsPlanCompleted("PLAN-PROG"));

        // Stage 2 Green Confirmed
        monitor.RecordSignalConfirmation("PLAN-PROG", "INT_B", "GREEN", 2000);
        monitor.RecordAmbulancePassage("PLAN-PROG", "INT_B", 2500);
        assert(monitor.IsStageCompleted("PLAN-PROG", "INT_B"));
        assert(monitor.IsPlanCompleted("PLAN-PROG"));

        // Rollback Confirmation Tracking
        assert(!monitor.IsRollbackCompleted("PLAN-PROG"));
        monitor.RecordRollbackCompletion("PLAN-PROG", "INT_A");
        assert(!monitor.IsRollbackCompleted("PLAN-PROG"));
        monitor.RecordRollbackCompletion("PLAN-PROG", "INT_B");
        assert(monitor.IsRollbackCompleted("PLAN-PROG"));

        std::cout << "  ✓ PASS: Signal confirmation, stage progression, and rollback completion verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 11: Execution Monitor Missed Deadline & Late Execution Detection
    // ------------------------------------------------------------------------
    std::cout << "[TEST 11] Execution Monitor Missed Deadline Detection...\n";
    {
        auto eventBus = std::make_shared<EventBus>();
        ExecutionMonitor monitor(nullptr, eventBus, 2000); // 2000ms deadline tolerance

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-DEADLINE", "AMB-05", {"INT_A"}, 1000);
        monitor.StartSupervision(plan, 20.0, 1000);

        // Stage scheduled at 1000ms. Advance to 2500ms (within tolerance 1000 + 2000 = 3000ms)
        monitor.CheckExecutionDeadlines(2500);
        assert(!monitor.IsReplanningRequired("PLAN-DEADLINE"));

        // Advance to 3500ms without activation confirmation -> Missed Deadline!
        monitor.CheckExecutionDeadlines(3500);
        assert(monitor.IsReplanningRequired("PLAN-DEADLINE"));
        assert(monitor.GetReplanningReason("PLAN-DEADLINE").find("Missed activation deadline") != std::string::npos);

        std::cout << "  ✓ PASS: Missed execution deadline detected and replanning triggered.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 12: ETA Drift & Replanning Trigger
    // ------------------------------------------------------------------------
    std::cout << "[TEST 12] ETA Drift & Dynamic Replanning Trigger...\n";
    {
        ExecutionMonitor monitor(nullptr, nullptr, 3000, 5000); // 5000ms ETA drift threshold

        ExecutionPlan plan = CreateTestExecutionPlan("PLAN-ETA", "AMB-06", {"INT_A", "INT_B"}, 1000);
        monitor.StartSupervision(plan, 50.0, 1000); // Initial planned ETA: 50.0 seconds

        // Minor drift: updated ETA = 52.0s (drift = 2s <= 5s)
        monitor.RecordUpdatedEta("PLAN-ETA", 52.0, 2000);
        assert(!monitor.IsReplanningRequired("PLAN-ETA"));

        // Severe drift: updated ETA = 60.0s (drift = 10s > 5s threshold)
        monitor.RecordUpdatedEta("PLAN-ETA", 60.0, 3000);
        assert(monitor.IsReplanningRequired("PLAN-ETA"));
        assert(monitor.GetReplanningReason("PLAN-ETA").find("ETA drift exceeded threshold") != std::string::npos);

        std::cout << "  ✓ PASS: Dynamic ETA drift detected and corridor replanning triggered.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 13: Recovery Manager Node Failure & Corridor Recomputation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 13] Recovery Manager Node Failure & Corridor Recomputation...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto healthMon = std::make_shared<NodeHealthMonitor>(stateMgr);
        auto eventBus = std::make_shared<EventBus>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);

        sessionMgr->CreateSession("EMG-FAIL", "AMB-07", "HOSP-01", 1, 1000);
        sessionMgr->UpdateRoute("EMG-FAIL", {"INT_A", "INT_B", "INT_C"}, 60.0, 1000);
        sessionMgr->ActivateSession("EMG-FAIL", 1000);

        RecoveryManager recoveryMgr(stateMgr, healthMon, sessionMgr, eventBus);

        // Node INT_B fails
        assert(recoveryMgr.HandleNodeFailure("INT_B", "Hardware failure", 1100));
        assert(healthMon->GetNodeHealthState("INT_B") == NodeHealthState::FAILED);
        assert(recoveryMgr.HasActiveRecovery("EMG-FAIL"));

        // Apply bypass corridor: INT_A -> INT_D -> INT_C
        ExecutionPlan bypassPlan = CreateTestExecutionPlan("PLAN-BYPASS", "AMB-07", {"INT_A", "INT_D", "INT_C"}, 1200);
        assert(recoveryMgr.ApplyBypassCorridor("EMG-FAIL", "INT_B", {"INT_A", "INT_D", "INT_C"}, bypassPlan, 1200));

        auto sess = sessionMgr->GetSession("EMG-FAIL");
        assert(sess->route.size() == 3);
        assert(sess->route[1] == "INT_D");
        assert(!recoveryMgr.HasActiveRecovery("EMG-FAIL"));

        std::cout << "  ✓ PASS: Node failure failover and bypass corridor recomputation verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 14: Recovery Manager Safe Rollback Notification
    // ------------------------------------------------------------------------
    std::cout << "[TEST 14] Recovery Manager Safe Rollback Notification...\n";
    {
        std::vector<std::pair<std::string, std::string>> rollbacks;
        auto rollbackSender = [&rollbacks](const std::string& nodeId, const std::string& cmd) {
            rollbacks.push_back({nodeId, cmd});
        };

        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, nullptr);
        sessionMgr->CreateSession("EMG-RB", "AMB-08", "HOSP-01", 1, 1000);
        sessionMgr->UpdateRoute("EMG-RB", {"INT_1", "INT_2", "INT_3"}, 40.0, 1000);
        sessionMgr->ActivateSession("EMG-RB", 1000);

        RecoveryManager recoveryMgr(stateMgr, nullptr, sessionMgr, nullptr, rollbackSender);

        // When INT_2 fails, unaffected nodes INT_1 and INT_3 receive safe rollback
        assert(recoveryMgr.HandleNodeFailure("INT_2", "Network timeout", 1100));
        assert(rollbacks.size() == 2);
        assert(rollbacks[0].first == "INT_1" && rollbacks[0].second == "RESTORE_NORMAL_MODE");
        assert(rollbacks[1].first == "INT_3" && rollbacks[1].second == "RESTORE_NORMAL_MODE");

        std::cout << "  ✓ PASS: Safe rollback command dispatched to surviving intersections.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 15: Node Rejoin & State Resynchronization
    // ------------------------------------------------------------------------
    std::cout << "[TEST 15] Node Rejoin & State Resynchronization...\n";
    {
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto healthMon = std::make_shared<NodeHealthMonitor>(stateMgr);
        auto eventBus = std::make_shared<EventBus>();
        RecoveryManager recoveryMgr(stateMgr, healthMon, nullptr, eventBus);

        TrafficNodeRecord node{"INT_REBOOT", "192.168.1.50", 8080, 1000, 1000, false, "RED"};
        stateMgr->RegisterTrafficNode(node);
        stateMgr->SetNodeConnectionStatus("INT_REBOOT", ConnectionStatus::DISCONNECTED);

        // Node completes POST and rejoins
        assert(recoveryMgr.HandleNodeRejoin("INT_REBOOT", 2000));
        assert(stateMgr->GetNodeConnectionStatus("INT_REBOOT") == ConnectionStatus::CONNECTED);
        assert(stateMgr->GetTrafficNode("INT_REBOOT")->isOperational);
        assert(healthMon->GetNodeHealthState("INT_REBOOT") == NodeHealthState::RECOVERING);

        std::cout << "  ✓ PASS: Post-reboot node rejoin, self-check verification, and state resync verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 16: Event Bus Pub/Sub Decoupled Delivery
    // ------------------------------------------------------------------------
    std::cout << "[TEST 16] Event Bus Pub/Sub Decoupled Delivery...\n";
    {
        EventBus bus;
        uint32_t emergencyCreatedCalls = 0;
        uint32_t allEventsCalls = 0;

        auto sub1 = bus.Subscribe(EventType::EmergencyCreated, [&emergencyCreatedCalls](const Event& ev) {
            assert(ev.type == EventType::EmergencyCreated);
            emergencyCreatedCalls++;
        });

        auto sub2 = bus.SubscribeAll([&allEventsCalls](const Event&) {
            allEventsCalls++;
        });
        (void)sub2;

        assert(bus.GetSubscriberCount(EventType::EmergencyCreated) == 2);

        // Publish EmergencyCreated
        bus.Publish(Event(EventType::EmergencyCreated, "EMG-01", "AMB-01"));
        assert(emergencyCreatedCalls == 1);
        assert(allEventsCalls == 1);

        // Publish NodeConnected -> only sub2 receives
        bus.Publish(Event(EventType::NodeConnected, "INT_A", ""));
        assert(emergencyCreatedCalls == 1);
        assert(allEventsCalls == 2);

        // Unsubscribe sub1
        bus.Unsubscribe(sub1);
        bus.Publish(Event(EventType::EmergencyCreated, "EMG-02", "AMB-02"));
        assert(emergencyCreatedCalls == 1); // Unsubscribed, unchanged
        assert(allEventsCalls == 3);

        std::cout << "  ✓ PASS: Type-safe EventBus subscription, filtering, and decoupled delivery verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 17: OrchestrationConfig Loader & Safety Range Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 17] OrchestrationConfig Loader & Safety Validation...\n";
    {
        OrchestrationConfig config;
        assert(config.Validate());

        assert(config.LoadFromFile("coordinator/orchestration/config/Orchestration.conf"));
        assert(config.heartbeatTimeoutMs == 5000);
        assert(config.retryCount == 3);
        assert(config.retryDelayMs == 500);
        assert(config.dispatchTimeoutMs == 2000);
        assert(config.clockDriftThresholdMs == 500);
        assert(config.maxNodeDelayMs == 3000);
        assert(config.recoveryTimeoutMs == 4000);
        assert(config.fsmTimeoutMs == 10000);
        assert(config.eventQueueSize == 256);
        assert(config.threadPoolSize == 4);
        assert(config.Validate());

        // Validate range checks
        config.heartbeatTimeoutMs = 10; // below 500ms limit
        assert(!config.Validate());

        std::cout << "  ✓ PASS: Configuration parser, default parameters, and boundary validation verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 18: End-to-End Orchestrated Distributed Runtime Simulation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 18] End-to-End Orchestrated Distributed Runtime Simulation...\n";
    {
        // 1. Initialize full orchestration subsystem
        auto stateMgr = std::make_shared<CoordinatorStateManager>();
        auto eventBus = std::make_shared<EventBus>();
        auto sessionMgr = std::make_shared<EmergencySessionManager>(stateMgr, eventBus);
        auto healthMon = std::make_shared<NodeHealthMonitor>(stateMgr, eventBus, 5000);
        auto clockTracker = std::make_shared<NodeClockTracker>(stateMgr, eventBus, 500);
        auto execMon = std::make_shared<ExecutionMonitor>(stateMgr, eventBus, 3000, 5000, 3000);
        CoordinatorFSM fsm(stateMgr, eventBus);

        // Wire mock network sender
        std::vector<std::pair<std::string, protocol::Packet>> wirePackets;
        auto nodeSender = [&wirePackets](const std::string& nodeId, const protocol::Packet& pkt) -> bool {
            wirePackets.push_back({nodeId, pkt});
            return true;
        };
        ExecutionDispatcher dispatcher(nodeSender, eventBus, stateMgr);
        RecoveryManager recoveryMgr(stateMgr, healthMon, sessionMgr, eventBus);

        uint64_t simClock = 10000;

        // Step 1: Nodes register and send heartbeats
        TrafficNodeRecord nodeA{"INT_1", "192.168.1.101", 8080, simClock, simClock, true, "RED"};
        TrafficNodeRecord nodeB{"INT_2", "192.168.1.102", 8080, simClock, simClock, true, "RED"};
        stateMgr->RegisterTrafficNode(nodeA);
        stateMgr->RegisterTrafficNode(nodeB);
        healthMon->RecordHeartbeat("INT_1", 12.0, 20.0, 42.0, 8, simClock);
        healthMon->RecordHeartbeat("INT_2", 15.0, 22.0, 44.0, 9, simClock);

        assert(healthMon->GetNodeHealthState("INT_1") == NodeHealthState::ONLINE);
        assert(healthMon->GetNodeHealthState("INT_2") == NodeHealthState::ONLINE);

        // Step 2: Emergency Request arrives -> FSM transitions to ROUTE_PLANNING
        assert(fsm.TransitionTo(CoordinatorState::ROUTE_PLANNING));
        assert(sessionMgr->CreateSession("EMG-MISSION", "AMB-99", "HOSP-CENTRAL", 1, simClock));

        // Step 3: Plan generated and approved
        assert(fsm.TransitionTo(CoordinatorState::PLAN_GENERATION));
        ExecutionPlan missionPlan = CreateTestExecutionPlan("PLAN-MISSION", "AMB-99", {"INT_1", "INT_2"}, simClock);
        sessionMgr->UpdateRoute("EMG-MISSION", {"INT_1", "INT_2"}, 40.0, simClock);
        sessionMgr->UpdateExecutionPlan("EMG-MISSION", missionPlan, simClock);

        assert(fsm.TransitionTo(CoordinatorState::PLAN_VALIDATION));
        assert(fsm.TransitionTo(CoordinatorState::DISPATCHING));

        // Step 4: Dispatch plan over network
        assert(dispatcher.DispatchPlan(missionPlan, simClock));
        assert(wirePackets.size() == 2);

        // Step 5: Intersections ACK stages
        assert(dispatcher.ProcessAck("INT_1", 1, missionPlan.checksum, simClock + 50));
        assert(dispatcher.ProcessAck("INT_2", 2, missionPlan.checksum, simClock + 60));
        assert(dispatcher.IsPlanFullyAcknowledged("PLAN-MISSION"));

        // Step 6: Begin execution and runtime monitoring
        assert(fsm.TransitionTo(CoordinatorState::EXECUTING));
        assert(sessionMgr->ActivateSession("EMG-MISSION", simClock + 100));
        assert(fsm.TransitionTo(CoordinatorState::MONITORING));
        execMon->StartSupervision(missionPlan, 40.0, simClock + 100);

        // Step 7: Ambulance progresses through INT_1
        simClock += 5000;
        execMon->RecordSignalConfirmation("PLAN-MISSION", "INT_1", "GREEN", simClock);
        execMon->RecordAmbulancePassage("PLAN-MISSION", "INT_1", simClock + 500);
        assert(execMon->IsStageCompleted("PLAN-MISSION", "INT_1"));

        // Step 8: Ambulance progresses through INT_2 and reaches destination
        simClock += 10000;
        execMon->RecordSignalConfirmation("PLAN-MISSION", "INT_2", "GREEN", simClock);
        execMon->RecordAmbulancePassage("PLAN-MISSION", "INT_2", simClock + 500);
        assert(execMon->IsPlanCompleted("PLAN-MISSION"));

        // Step 9: Mission completed and archived
        assert(sessionMgr->CompleteSession("EMG-MISSION", simClock + 1000));
        assert(fsm.TransitionTo(CoordinatorState::COMPLETED));

        assert(sessionMgr->ArchiveSession("EMG-MISSION", simClock + 1100));
        assert(fsm.TransitionTo(CoordinatorState::IDLE));

        assert(fsm.GetCurrentState() == CoordinatorState::IDLE);
        std::cout << "  ✓ PASS: End-to-end distributed orchestration mission completed with 100% success.\n";
    }

    std::cout << "\n==========================================================\n"
              << " Phase 5 Coordinator Orchestration Validation:            \n"
              << " Total Tests Passed: 18 / 18 (100%)                       \n"
              << " Total Failures:     0                                    \n"
              << "==========================================================\n"
              << ">>> [SUCCESS] Coordinator Orchestration Verified 100%!   \n\n";

    return 0;
}
