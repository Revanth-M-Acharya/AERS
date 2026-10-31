/**
 * @file test_coordinator_intelligence.cpp
 * @brief Comprehensive Unit & Integration Tests for Phase 4 Coordinator Intelligence Engine.
 * Target: Raspberry Pi 4 Model B (QNX SDP 8.0 Starter Kit Image / POSIX).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/graph/RoadGraph.hpp"
#include "coordinator/intelligence/planner/IRoutePlanner.hpp"
#include "coordinator/intelligence/planner/DijkstraRoutePlanner.hpp"
#include "coordinator/intelligence/eta/IETAEngine.hpp"
#include "coordinator/intelligence/eta/ContinuousETAEngine.hpp"
#include "coordinator/intelligence/scheduler/IPriorityScheduler.hpp"
#include "coordinator/intelligence/scheduler/PriorityScheduler.hpp"
#include "coordinator/intelligence/corridor/ICorridorPlanner.hpp"
#include "coordinator/intelligence/corridor/GreenCorridorPlanner.hpp"
#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include "coordinator/intelligence/execution/ExecutionPlanGenerator.hpp"
#include "coordinator/intelligence/safety/ISafetyPolicy.hpp"
#include "coordinator/intelligence/safety/SafetyPolicyEngine.hpp"
#include "coordinator/intelligence/config/IntelligenceConfig.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iostream>
#include <cassert>
#include <fstream>
#include <cmath>

using namespace aegis;
using namespace aegis::coordinator;
using namespace aegis::coordinator::intelligence;

int main() {
    std::cout << "==========================================================\n"
              << " SMART AEGIS: Coordinator Intelligence (Phase 4) Tests    \n"
              << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n"
              << "==========================================================\n\n";

    CoordinatorLogger::Instance().Initialize("", false, LogLevel::INFO);

    // ------------------------------------------------------------------------
    // TEST 1: Road Network Graph Construction & Topological Adjacency
    // ------------------------------------------------------------------------
    std::cout << "[TEST 1] Road Network Graph Construction & Topology...\n";
    {
        RoadGraph graph;
        assert(graph.GetIntersectionCount() == 0);

        Intersection a{"INT_A", "North Junction", 12.9716, 77.5946, true, {}};
        Intersection b{"INT_B", "Central Square", 12.9725, 77.5955, true, {}};
        Intersection c{"INT_C", "Tech Park", 12.9735, 77.5965, true, {}};

        assert(graph.AddIntersection(a));
        assert(graph.AddIntersection(b));
        assert(graph.AddIntersection(c));
        assert(graph.GetIntersectionCount() == 3);

        RoadSegment s1{"SEG_AB", "INT_A", "INT_B", 500.0, 50.0, 50.0, LaneDirection::BIDIRECTIONAL, false, false, 1.0, 2};
        RoadSegment s2{"SEG_BC", "INT_B", "INT_C", 400.0, 40.0, 40.0, LaneDirection::FORWARD, false, false, 1.0, 2};

        assert(graph.AddRoadSegment(s1));
        assert(graph.AddRoadSegment(s2));

        // Outgoing from A should have SEG_AB
        auto outA = graph.GetOutgoingSegments("INT_A");
        assert(outA.size() == 1);
        assert(outA[0].segmentId == "SEG_AB");
        assert(outA[0].toIntersectionId == "INT_B");

        // Outgoing from B should have SEG_BC and reverse of SEG_AB
        auto outB = graph.GetOutgoingSegments("INT_B");
        assert(outB.size() == 2);

        std::cout << "  ✓ PASS: Graph construction, bidirectional links, and outgoing queries verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 2: Dynamic Graph Mutability (Blockages, Closures, AI Cost Multipliers)
    // ------------------------------------------------------------------------
    std::cout << "[TEST 2] Dynamic Graph Mutability & Edge Travel Cost...\n";
    {
        RoadGraph graph;
        graph.AddIntersection({"INT_1", "Node 1", 0, 0, true, {}});
        graph.AddIntersection({"INT_2", "Node 2", 0, 0, true, {}});
        RoadSegment seg{"S12", "INT_1", "INT_2", 1000.0, 50.0, 50.0, LaneDirection::FORWARD, false, false, 1.0, 2};
        graph.AddRoadSegment(seg);

        // Base travel time: 1000m at 50 km/h (13.888 m/s) = ~72 sec
        const RoadSegment* s = graph.GetSegment("S12");
        assert(s != nullptr);
        double t1 = s->GetEffectiveTravelTimeSec();
        assert(std::abs(t1 - 72.0) < 1.0);

        // AI Cost multiplier: double cost
        assert(graph.SetAiCostMultiplier("S12", 2.0));
        s = graph.GetSegment("S12");
        double t2 = s->GetEffectiveTravelTimeSec();
        assert(std::abs(t2 - 144.0) < 2.0);

        // Block segment
        assert(graph.SetSegmentBlocked("S12", true));
        s = graph.GetSegment("S12");
        assert(s->GetEffectiveTravelTimeSec() >= 1e8);

        // Unblock and verify recovery
        assert(graph.SetSegmentBlocked("S12", false));
        s = graph.GetSegment("S12");
        assert(s->GetEffectiveTravelTimeSec() < 1e8);

        std::cout << "  ✓ PASS: Dynamic road blockage, speed adjustment, and AI edge multiplier verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 3: Dijkstra Route Planner Shortest Path
    // ------------------------------------------------------------------------
    std::cout << "[TEST 3] Dijkstra Shortest Path Computation...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        assert(graph.GetIntersectionCount() >= 6);

        DijkstraRoutePlanner planner;
        RouteResult route = planner.ComputeShortestPath("INT_A", "INT_H", graph);
        assert(route.success);
        assert(!route.pathIntersectionIds.empty());
        assert(route.pathIntersectionIds.front() == "INT_A");
        assert(route.pathIntersectionIds.back() == "INT_H");
        assert(route.totalDistanceMeters > 0.0);
        assert(route.estimatedTravelTimeSec > 0.0);

        std::cout << "  ✓ PASS: Shortest path from INT_A to INT_H computed: "
                  << route.pathIntersectionIds.size() << " hops (" << route.totalDistanceMeters << "m, "
                  << route.estimatedTravelTimeSec << "s).\n";
    }

    // ------------------------------------------------------------------------
    // TEST 4: Dynamic Route Replanning on Road Closure & Destination Divert
    // ------------------------------------------------------------------------
    std::cout << "[TEST 4] Dynamic Route Replanning & Avoidance...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        DijkstraRoutePlanner planner;

        RouteResult routeOriginal = planner.ComputeShortestPath("INT_A", "INT_H", graph);
        assert(routeOriginal.success);

        // Block primary segment on route (e.g., SEG_BC)
        graph.SetSegmentBlocked("SEG_BC", true);
        RouteResult routeReplanned = planner.RecomputeRoute("INT_A", "INT_H", graph, {"SEG_BC"});
        assert(routeReplanned.success);

        // Must avoid SEG_BC
        for (const auto& segId : routeReplanned.pathSegmentIds) {
            assert(segId != "SEG_BC");
        }

        // Destination divert to INT_D
        RouteResult routeDivert = planner.RecomputeRoute("INT_A", "INT_D", graph);
        assert(routeDivert.success);
        assert(routeDivert.pathIntersectionIds.back() == "INT_D");

        std::cout << "  ✓ PASS: Route replanning dynamically routed around blockage and diverted destinations.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 5: Continuous ETA Engine & Kinematics
    // ------------------------------------------------------------------------
    std::cout << "[TEST 5] Continuous ETA Engine & Kinematic Estimation...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        DijkstraRoutePlanner planner;
        RouteResult route = planner.ComputeShortestPath("INT_A", "INT_H", graph);
        assert(route.success);

        ContinuousETAEngine etaEngine(0.3);
        ETAReport report = etaEngine.EstimateETA("AMB_101", route.pathSegmentIds, 60.0, graph);

        assert(report.etaSeconds > 0.0);
        assert(report.remainingDistanceMeters == route.totalDistanceMeters);
        assert(report.arrivalConfidencePercent >= 15.0 && report.arrivalConfidencePercent <= 99.0);
        assert(report.predictedArrivalTimeUnixMs > common::TimeUtils::GetCurrentTimestampMs());

        std::cout << "  ✓ PASS: Continuous ETA calculated: " << report.etaSeconds
                  << "s, confidence: " << report.arrivalConfidencePercent << "%.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 6: Historical Edge Traversal EMA Updating
    // ------------------------------------------------------------------------
    std::cout << "[TEST 6] Historical Edge Traversal EMA Statistics...\n";
    {
        ContinuousETAEngine etaEngine(0.5);
        assert(etaEngine.GetHistoricalTraversalTime("SEG_TEST") == 0.0);

        etaEngine.UpdateHistoricalEdgeTraversal("SEG_TEST", 100.0);
        assert(std::abs(etaEngine.GetHistoricalTraversalTime("SEG_TEST") - 100.0) < 0.001);

        // EMA with alpha=0.5: 0.5 * 50 + 0.5 * 100 = 75.0
        etaEngine.UpdateHistoricalEdgeTraversal("SEG_TEST", 50.0);
        assert(std::abs(etaEngine.GetHistoricalTraversalTime("SEG_TEST") - 75.0) < 0.001);

        std::cout << "  ✓ PASS: Historical traversal time EMA updates verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 7: Priority Scheduler Multi-Ambulance Ordering & FIFO Tie-Breaking
    // ------------------------------------------------------------------------
    std::cout << "[TEST 7] Priority Scheduler Ranking & FIFO Tie-Breaking...\n";
    {
        PriorityScheduler scheduler(10000);

        AmbulanceRequest r1{"AMB_LOW", EmergencyPriorityLevel::LOW, "INT_A", "INT_H", 1000, 0, 0};
        AmbulanceRequest r2{"AMB_CRIT_EARLY", EmergencyPriorityLevel::CRITICAL, "INT_B", "INT_H", 1050, 0, 0};
        AmbulanceRequest r3{"AMB_MED", EmergencyPriorityLevel::MEDIUM, "INT_C", "INT_H", 1100, 0, 0};
        AmbulanceRequest r4{"AMB_CRIT_LATE", EmergencyPriorityLevel::CRITICAL, "INT_D", "INT_H", 1200, 0, 0};

        assert(scheduler.RegisterAmbulance(r1));
        assert(scheduler.RegisterAmbulance(r2));
        assert(scheduler.RegisterAmbulance(r3));
        assert(scheduler.RegisterAmbulance(r4));

        auto schedule = scheduler.ScheduleAmbulances(1300);
        assert(schedule.size() == 4);

        // Priority order: CRIT_EARLY (FIFO first) -> CRIT_LATE -> MED -> LOW
        assert(schedule[0].ambulanceId == "AMB_CRIT_EARLY");
        assert(schedule[1].ambulanceId == "AMB_CRIT_LATE");
        assert(schedule[2].ambulanceId == "AMB_MED");
        assert(schedule[3].ambulanceId == "AMB_LOW");

        std::cout << "  ✓ PASS: Priority hierarchy and FIFO tie-breaking validated.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 8: Starvation Prevention Aging Mechanism
    // ------------------------------------------------------------------------
    std::cout << "[TEST 8] Starvation Prevention Priority Aging...\n";
    {
        PriorityScheduler scheduler(5000); // 5s starvation threshold

        AmbulanceRequest rLow{"AMB_STARVED", EmergencyPriorityLevel::LOW, "INT_A", "INT_H", 1000, 0, 0};
        AmbulanceRequest rHigh{"AMB_NEW_HIGH", EmergencyPriorityLevel::HIGH, "INT_B", "INT_H", 25000, 0, 0};

        scheduler.RegisterAmbulance(rLow);
        scheduler.RegisterAmbulance(rHigh);

        // At time 25000ms: AMB_STARVED waited 24000ms (boost: 24000/5000 = +4 levels!)
        // Base LOW (1) + 4 = 5 > HIGH (3)
        auto schedule = scheduler.ScheduleAmbulances(25000);
        assert(schedule[0].ambulanceId == "AMB_STARVED");

        std::cout << "  ✓ PASS: Starvation prevention aging promoted long-waiting emergency request.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 9: Priority Inheritance & Reservation Conflict Resolution
    // ------------------------------------------------------------------------
    std::cout << "[TEST 9] Priority Inheritance & Reservation Management...\n";
    {
        PriorityScheduler scheduler(10000);

        AmbulanceRequest low{"AMB_SLOW", EmergencyPriorityLevel::LOW, "INT_A", "INT_B", 900, 0, 0};
        AmbulanceRequest crit{"AMB_FAST", EmergencyPriorityLevel::CRITICAL, "INT_B", "INT_C", 1000, 0, 0};

        scheduler.RegisterAmbulance(low);
        scheduler.RegisterAmbulance(crit);

        // AMB_SLOW reserves INT_B from 2000ms to 8000ms
        assert(scheduler.ReserveIntersection("INT_B", "AMB_SLOW", 2000, 8000));
        assert(scheduler.IsIntersectionReserved("INT_B", 5000, "AMB_OTHER"));

        // AMB_FAST attempts conflicting reservation on INT_B: triggers Priority Inheritance!
        bool granted = scheduler.ReserveIntersection("INT_B", "AMB_FAST", 4000, 7000);
        assert(!granted); // Conflict actively held

        // AMB_SLOW inherits CRITICAL priority to expedite clearance
        auto schedule = scheduler.ScheduleAmbulances(2000);
        assert(!schedule.empty());
        const AmbulanceRequest& slowReq = (schedule[0].ambulanceId == "AMB_SLOW") ? schedule[0] : schedule[1];
        assert(slowReq.priorityInheritanceLevel >= static_cast<uint32_t>(EmergencyPriorityLevel::CRITICAL));

        // When AMB_SLOW clears and releases INT_B
        scheduler.ReleaseIntersection("INT_B", "AMB_SLOW");
        assert(!scheduler.IsIntersectionReserved("INT_B", 5000, "AMB_OTHER"));

        // Now AMB_FAST successfully reserves INT_B
        assert(scheduler.ReserveIntersection("INT_B", "AMB_FAST", 4000, 7000));

        std::cout << "  ✓ PASS: Priority inheritance resolved resource contention without deadlock.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 10: Green Corridor Planner Staged Wave Generation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 10] Green Corridor Wave Planner Generation...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        DijkstraRoutePlanner planner;
        RouteResult route = planner.ComputeShortestPath("INT_A", "INT_H", graph);
        assert(route.success);

        ContinuousETAEngine etaEngine;
        ETAReport eta = etaEngine.EstimateETA("AMB_777", route.pathSegmentIds, 50.0, graph);

        GreenCorridorPlanner corridorPlanner(4000, 15000);
        AmbulanceRequest req{"AMB_777", EmergencyPriorityLevel::HIGH, "INT_A", "INT_H", 1000, 0, 0};

        CorridorPlan plan = corridorPlanner.GenerateCorridorPlan(req, route, eta, graph, 5000);
        assert(plan.ambulanceId == "AMB_777");
        assert(plan.schedule.size() == route.pathIntersectionIds.size());
        assert(plan.rollbackMode == "RESTORE_NORMAL_MODE");

        // Staged activation times must be monotonic non-decreasing
        for (size_t i = 1; i < plan.schedule.size(); ++i) {
            assert(plan.schedule[i].activationTimeMonoMs >= plan.schedule[i - 1].activationTimeMonoMs);
        }

        std::cout << "  ✓ PASS: Time-staged green corridor propagation plan generated.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 11: Execution Plan Generator Immutability & CRC16 Integrity
    // ------------------------------------------------------------------------
    std::cout << "[TEST 11] Execution Plan Generator Immutability & CRC16...\n";
    {
        CorridorPlan cplan;
        cplan.planId = "PLAN_AMB_01_TEST";
        cplan.ambulanceId = "AMB_01";
        cplan.rollbackMode = "RESTORE_NORMAL_MODE";
        cplan.schedule.push_back({"INT_A", "SET_GREEN", 5000, 12000, "AMBULANCE_PASSED"});
        cplan.schedule.push_back({"INT_B", "SET_GREEN", 9000, 12000, "AMBULANCE_PASSED"});

        ExecutionPlanGenerator generator;
        ExecutionPlan plan = generator.CreateExecutionPlan(cplan, 3);

        assert(plan.planId == "PLAN_AMB_01_TEST");
        assert(plan.orderedIntersections.size() == 2);
        assert(plan.checksum != 0);
        assert(plan.VerifyChecksum());

        // Tamper with plan content -> Verify checksum failure
        plan.signalCommands[0].signalCommand = "SET_RED";
        assert(!plan.VerifyChecksum());

        std::cout << "  ✓ PASS: Execution plan immutability and tamper-detection checksum verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 12: Safety Policy Engine Conflicting Green & Priority Rejection
    // ------------------------------------------------------------------------
    std::cout << "[TEST 12] Safety Policy Conflicting Green Rejection...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        SafetyPolicyEngine safetyEngine;

        // Plan 1: Active CRITICAL corridor on INT_B
        ExecutionPlan plan1;
        plan1.planId = "PLAN_ACTIVE_CRIT";
        plan1.ambulanceId = "AMB_CRIT";
        plan1.priorityLevel = 4; // CRITICAL
        plan1.orderedIntersections = {"INT_B"};
        plan1.rollbackPlan = "RESTORE_NORMAL_MODE";
        plan1.signalCommands.push_back({"INT_B", "SET_GREEN", 10000, 15000, "PASS"});
        plan1.checksum = plan1.ComputeChecksum();

        // Plan 2: Incoming MEDIUM corridor on INT_B with overlapping time [12000, 27000]
        ExecutionPlan plan2;
        plan2.planId = "PLAN_INCOMING_MED";
        plan2.ambulanceId = "AMB_MED";
        plan2.priorityLevel = 2; // MEDIUM
        plan2.orderedIntersections = {"INT_B"};
        plan2.rollbackPlan = "RESTORE_NORMAL_MODE";
        plan2.signalCommands.push_back({"INT_B", "SET_GREEN", 12000, 15000, "PASS"});
        plan2.checksum = plan2.ComputeChecksum();

        // Validating Plan 2 against active Plan 1 must REJECT due to conflicting green!
        SafetyValidationResult val = safetyEngine.ValidatePlan(plan2, graph, {plan1}, 8000);
        assert(!val.isValid);
        assert(val.rejectionReason.find("CONFLICTING_GREEN") != std::string::npos);

        std::cout << "  ✓ PASS: Safety Policy Engine strictly rejected conflicting orthogonal green corridor.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 13: Safety Policy Timing Bounds & Rollback Validation
    // ------------------------------------------------------------------------
    std::cout << "[TEST 13] Safety Policy Timing & Rollback Policy Checks...\n";
    {
        RoadGraph graph;
        graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf");
        SafetyPolicyEngine safetyEngine(2000, 30000);

        // Plan with unsafe hold duration (100ms < 2000ms minimum)
        ExecutionPlan pUnsafeTiming;
        pUnsafeTiming.planId = "P_UNSAFE";
        pUnsafeTiming.orderedIntersections = {"INT_A"};
        pUnsafeTiming.rollbackPlan = "RESTORE_NORMAL_MODE";
        pUnsafeTiming.signalCommands.push_back({"INT_A", "SET_GREEN", 5000, 100, "PASS"});
        pUnsafeTiming.checksum = pUnsafeTiming.ComputeChecksum();

        SafetyValidationResult resTiming = safetyEngine.ValidatePlan(pUnsafeTiming, graph, {}, 1000);
        assert(!resTiming.isValid);
        assert(resTiming.rejectionReason.find("INVALID_TIMING") != std::string::npos);

        // Plan with missing safe rollback
        ExecutionPlan pNoRollback;
        pNoRollback.planId = "P_NO_ROLLBACK";
        pNoRollback.orderedIntersections = {"INT_A"};
        pNoRollback.rollbackPlan = "UNSPECIFIED_MODE";
        pNoRollback.signalCommands.push_back({"INT_A", "SET_GREEN", 5000, 10000, "PASS"});
        pNoRollback.checksum = pNoRollback.ComputeChecksum();

        SafetyValidationResult resRollback = safetyEngine.ValidatePlan(pNoRollback, graph, {}, 1000);
        assert(!resRollback.isValid);
        assert(resRollback.rejectionReason.find("UNSAFE_ROLLBACK") != std::string::npos);

        std::cout << "  ✓ PASS: Timing violation and unsafe rollback plans successfully rejected.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 14: Intelligence Configuration Model & File Loader
    // ------------------------------------------------------------------------
    std::cout << "[TEST 14] Intelligence Configuration Model & Parser...\n";
    {
        IntelligenceConfig cfg;
        assert(cfg.LoadFromFile("coordinator/intelligence/config/Intelligence.conf"));
        assert(cfg.Validate());
        assert(cfg.defaultSpeedLimitKmh > 0.0);
        assert(cfg.greenLeadTimeMs >= 500);
        assert(cfg.greenHoldDurationMs >= 1000);
        assert(!cfg.ToString().empty());

        std::cout << "  ✓ PASS: Intelligence configuration loader and validation verified.\n";
    }

    // ------------------------------------------------------------------------
    // TEST 15: End-to-End Orchestrated Intelligence Pipeline
    // ------------------------------------------------------------------------
    std::cout << "[TEST 15] End-to-End Intelligence Pipeline Orchestration...\n";
    {
        // 1. Graph
        RoadGraph graph;
        assert(graph.LoadFromFile("coordinator/intelligence/config/RoadNetwork.conf"));

        // 2. Scheduler
        PriorityScheduler scheduler;
        AmbulanceRequest ambReq{"AMB_ALPHA", EmergencyPriorityLevel::CRITICAL, "INT_A", "INT_H", 5000, 0, 0};
        assert(scheduler.RegisterAmbulance(ambReq));
        auto ranked = scheduler.ScheduleAmbulances(5000);
        assert(!ranked.empty());

        // 3. Planner
        DijkstraRoutePlanner planner;
        RouteResult route = planner.ComputeShortestPath(ambReq.startIntersection, ambReq.destinationIntersection, graph);
        assert(route.success);

        // 4. ETA
        ContinuousETAEngine etaEngine;
        ETAReport eta = etaEngine.EstimateETA(ambReq.ambulanceId, route.pathSegmentIds, 55.0, graph);
        assert(eta.etaSeconds > 0.0);

        // 5. Corridor Planner
        GreenCorridorPlanner corridorPlanner;
        CorridorPlan corridor = corridorPlanner.GenerateCorridorPlan(ambReq, route, eta, graph, 5000);
        assert(!corridor.schedule.empty());

        // 6. Execution Plan Generator
        ExecutionPlanGenerator execGen;
        ExecutionPlan execPlan = execGen.CreateExecutionPlan(corridor, static_cast<uint32_t>(ambReq.priority));
        assert(execPlan.VerifyChecksum());

        // 7. Safety Policy Engine
        SafetyPolicyEngine safetyEngine;
        SafetyValidationResult safetyRes = safetyEngine.ValidatePlan(execPlan, graph, {}, 5000);
        assert(safetyRes.isValid);

        std::cout << "  ✓ PASS: Full pipeline (Request -> Graph -> Route -> ETA -> Schedule -> Corridor -> Plan -> Safety -> Approved) verified 100%.\n";
    }

    std::cout << "\n==========================================================\n"
              << " Phase 4 Coordinator Intelligence Engine Validation:      \n"
              << " Total Tests Passed: 15 / 15 (100%)                       \n"
              << " Total Failures:     0                                    \n"
              << "==========================================================\n"
              << ">>> [SUCCESS] Coordinator Intelligence Verified 100%!     \n\n";

    return 0;
}
