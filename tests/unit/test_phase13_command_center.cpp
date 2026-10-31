/**
 * @file test_phase13_command_center.cpp
 * @brief Comprehensive unit and integration test suite for Phase 13 Operations, Monitoring & Command Center.
 * Verifies EMS Fleet Management, Live Ambulance Tracking, Mission Database, Hospital Operations,
 * Traffic Operations, Raspberry Pi Cluster Monitor, System Health Monitor, Real-Time Analytics,
 * Mission Replay Controller, EventBus Bridge, ASCII Dashboard Console, and CommandCenterFacade.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/fleet/EmsFleetManager.hpp"
#include "command_center/tracking/LiveAmbulanceTracker.hpp"
#include "command_center/mission/MissionDatabase.hpp"
#include "command_center/hospital/HospitalOperationsDashboard.hpp"
#include "command_center/traffic/TrafficOperationsDashboard.hpp"
#include "command_center/cluster/ClusterMonitor.hpp"
#include "command_center/health/SystemHealthMonitor.hpp"
#include "command_center/analytics/RealTimeAnalyticsEngine.hpp"
#include "command_center/replay/MissionReplayController.hpp"
#include "command_center/dashboard/CommandCenterConsole.hpp"
#include "command_center/bridge/CommandCenterEventBridge.hpp"
#include "command_center/CommandCenterFacade.hpp"
#include "coordinator/orchestration/event_bus/EventBus.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <sstream>
#include <cmath>

using namespace aegis;
using namespace aegis::command_center;
using namespace aegis::coordinator::orchestration;

static void LogPass(const std::string& testName) {
    std::cout << "  [PASS] " << testName << std::endl;
}

// ==============================================================================
// TEST 1: EMS Fleet Management (12 Vehicles, Statuses, Types, Assignments)
// ==============================================================================
static void TestEmsFleetManager() {
    std::cout << "[TEST 1] EMS Fleet Management..." << std::endl;

    EmsFleetManager fleet;
    auto summary = fleet.GetInventorySummary();

    // Verify initial 12 vehicles matching Command Center specification
    assert(summary.totalAmbulances == 12);
    assert(summary.availableCount == 8);
    assert(summary.assignedCount == 3);
    assert(summary.maintenanceCount == 1);

    // Verify vehicle lookup
    auto amb1 = fleet.GetAmbulance("AMB-01");
    assert(amb1.has_value());
    assert(amb1->registrationNumber == "KA-01-EA-1001");
    assert(amb1->vehicleType == AmbulanceVehicleType::CARDIAC_AMBULANCE);
    assert(amb1->currentStatus == AmbulanceStatus::AVAILABLE);
    assert(amb1->currentDriver == "S. Kumar");
    assert(amb1->fuelLevelPercent > 80.0);

    auto amb9 = fleet.GetAmbulance("AMB-09");
    assert(amb9.has_value());
    assert(amb9->vehicleType == AmbulanceVehicleType::ICU_AMBULANCE);
    assert(amb9->currentStatus == AmbulanceStatus::ASSIGNED);

    // Verify status transition
    assert(fleet.UpdateStatus("AMB-01", AmbulanceStatus::EN_ROUTE_PATIENT));
    assert(fleet.GetAmbulance("AMB-01")->currentStatus == AmbulanceStatus::EN_ROUTE_PATIENT);

    assert(fleet.UpdateStatus("AMB-01", AmbulanceStatus::PATIENT_ON_BOARD));
    assert(fleet.GetAmbulance("AMB-01")->currentStatus == AmbulanceStatus::PATIENT_ON_BOARD);

    assert(fleet.UpdateStatus("AMB-01", AmbulanceStatus::EN_ROUTE_HOSPITAL));
    assert(fleet.GetAmbulance("AMB-01")->currentStatus == AmbulanceStatus::EN_ROUTE_HOSPITAL);

    // Test assign an available ambulance
    auto availList = fleet.GetAmbulancesByStatus(AmbulanceStatus::AVAILABLE);
    assert(!availList.empty());
    std::string testAmb = availList[0].ambulanceId;
    assert(fleet.AssignToMission(testAmb, "INC-TEST-001", "HOSP-01", 30.0));
    assert(fleet.GetAmbulance(testAmb)->currentStatus == AmbulanceStatus::ASSIGNED);

    // Test mission completion
    assert(fleet.CompleteMission(testAmb));
    assert(fleet.GetAmbulance(testAmb)->currentStatus == AmbulanceStatus::AVAILABLE);

    // Telemetry updates
    assert(fleet.UpdateTelemetry(testAmb, 12.9716, 77.5946, 0.0, 75.0));
    assert(std::abs(fleet.GetAmbulance(testAmb)->fuelLevelPercent - 75.0) < 0.01);

    LogPass("EMS Fleet Inventory, Status Lifecycle, & Mission Assignment verified.");
}

// ==============================================================================
// TEST 2: Live Ambulance Tracking (Route, Intersections, Speed, ETA, Corridor)
// ==============================================================================
static void TestLiveAmbulanceTracker() {
    std::cout << "[TEST 2] Live Ambulance Tracking..." << std::endl;

    LiveAmbulanceTracker tracker;
    AmbulanceTrackingState state;
    state.ambulanceId = "AMB-01";
    state.missionId = "INC-2026-089";
    state.currentRoute = {"HOME", "INT_1", "INT_2", "INT_3", "HOSPITAL"};
    state.currentIntersection = "HOME";
    state.nextIntersection = "INT_1";
    state.remainingDistanceMeters = 1500.0;
    state.currentSpeedKmh = 48.0;
    state.estimatedArrivalSec = 45.0;
    state.missionPriority = "CRITICAL (Code Red)";
    state.greenCorridorStatus = "ACTIVE";

    tracker.UpdateTracking(state);

    auto active = tracker.GetTracking("AMB-01");
    assert(active.has_value());
    assert(active->nextIntersection == "INT_1");
    assert(active->currentSpeedKmh == 48.0);
    assert(active->greenCorridorStatus == "ACTIVE");

    // Advance intersection
    assert(tracker.UpdateIntersectionProgress("AMB-01", "INT_1", "INT_2", 1000.0, 55.0, 30.0));
    active = tracker.GetTracking("AMB-01");
    assert(active->currentIntersection == "INT_1");
    assert(active->nextIntersection == "INT_2");
    assert(active->remainingDistanceMeters == 1000.0);

    // Update corridor status
    assert(tracker.UpdateCorridorStatus("AMB-01", "RESTORED"));
    assert(tracker.GetTracking("AMB-01")->greenCorridorStatus == "RESTORED");

    // Remove tracking
    assert(tracker.RemoveTracking("AMB-01"));
    assert(!tracker.GetTracking("AMB-01").has_value());

    LogPass("Live Route Tracking, Dynamic ETA, & Corridor Synchronization verified.");
}

// ==============================================================================
// TEST 3: Mission Database (In-Memory History, Replans, Police Advisories, Averages)
// ==============================================================================
static void TestMissionDatabase() {
    std::cout << "[TEST 3] Mission Database & Historical Repository..." << std::endl;

    MissionDatabase db;

    // Verify 15 historical missions initialized
    assert(db.GetTotalMissionsCount() == 15);
    auto allMissions = db.GetAllMissions();
    assert(allMissions.size() == 15);

    // Verify average response time ~138s (2m 18s)
    double avgResponse = db.GetAverageResponseTimeSec();
    assert(std::abs(avgResponse - 138.0) < 2.0);

    // Create a new mission
    HistoricalMissionRecord m;
    m.missionId = "INC-TEST-999";
    m.emergencyType = "STROKE";
    m.priority = "URGENT (Code Yellow)";
    m.assignedAmbulance = "AMB-04";
    m.assignedHospital = "Apollo Emergency Care";
    m.routeUsed = {"HOME", "INT_1", "INT_2", "HOSPITAL"};
    m.dispatchTimeMonoMs = 1000;
    m.missionStatus = "ACTIVE";
    assert(db.CreateMission(m));

    // Record dynamic replan
    assert(db.RecordReplanning("INC-TEST-999", {"HOME", "INT_1", "INT_ALT", "HOSPITAL"}, "Heavy construction on link 2-3"));
    auto retrieved = db.GetMission("INC-TEST-999");
    assert(retrieved.has_value());
    assert(retrieved->wasReplanned);
    assert(retrieved->replanReason == "Heavy construction on link 2-3");

    // Record police advisory
    assert(db.RecordPoliceAdvisory("INC-TEST-999", "Traffic Police unit reporting oil slick cleared"));
    assert(db.GetMission("INC-TEST-999")->manualPoliceAdvisory);

    // Complete mission
    assert(db.CompleteMission("INC-TEST-999", 140000, "SUCCESSFUL_PATIENT_HANDOVER"));
    assert(db.GetMission("INC-TEST-999")->missionStatus == "COMPLETED");

    LogPass("Mission Storage, Dynamic Replanning Audit, & Police Advisories verified.");
}

// ==============================================================================
// TEST 4: Hospital Operations Dashboard (6 Hospitals, Capacity, Load %, Status)
// ==============================================================================
static void TestHospitalOperationsDashboard() {
    std::cout << "[TEST 4] Hospital Operations Dashboard..." << std::endl;

    HospitalOperationsDashboard hospitals;

    // 6 regional hospitals registered
    assert(hospitals.GetHospitalCount() == 6);
    assert(hospitals.GetTotalIncomingPatientsCount() == 3);

    auto h1 = hospitals.GetHospital("HOSP-01");
    assert(h1.has_value());
    assert(h1->hospitalName == "City Trauma Hospital");
    assert(h1->totalBeds == 120);
    assert(h1->availableBeds == 36);
    assert(h1->availableIcuBeds == 7);
    assert(h1->status == HospitalConditionStatus::GREEN); // ~70.0% load

    // Ingest capacity reduction to trigger YELLOW
    assert(hospitals.UpdateCapacity("HOSP-01", 15, 2, 2, 1, 10)); // 105/120 = 87.5% load
    assert(hospitals.GetHospital("HOSP-01")->status == HospitalConditionStatus::YELLOW);

    // Ingest capacity reduction to trigger RED (Diversion)
    assert(hospitals.UpdateCapacity("HOSP-01", 5, 0, 0, 0, 8)); // 115/120 = 95.8% load
    assert(hospitals.GetHospital("HOSP-01")->status == HospitalConditionStatus::RED);

    // Ingest ambulance arrival
    assert(hospitals.AddIncomingAmbulance("HOSP-01", "AMB-10", 25.0, "CRITICAL (Code Red)"));
    assert(hospitals.GetHospital("HOSP-01")->incomingAmbulances.size() == 3);

    assert(hospitals.RemoveIncomingAmbulance("HOSP-01", "AMB-10"));
    assert(hospitals.GetHospital("HOSP-01")->incomingAmbulances.size() == 2);

    LogPass("6 Hospitals, Load Categorization (GREEN/YELLOW/RED), & Inbound Patients verified.");
}

// ==============================================================================
// TEST 5: Traffic Operations Dashboard (Intersections 1, 2, 3, Signals, Modes)
// ==============================================================================
static void TestTrafficOperationsDashboard() {
    std::cout << "[TEST 5] Traffic Operations Dashboard..." << std::endl;

    TrafficOperationsDashboard traffic;
    auto intersections = traffic.GetAllIntersections();
    assert(intersections.size() == 3);

    auto int1 = traffic.GetIntersection("INT_1");
    assert(int1.has_value());
    assert(int1->intersectionName == "Intersection 1 (RPi Traffic Node 1)");
    assert(int1->currentMode == IntersectionControlMode::NORMAL);

    // Actuate Emergency Corridor preemption
    assert(traffic.UpdateSignalState("INT_1", "ROAD_A_GREEN", IntersectionControlMode::EMERGENCY_CORRIDOR, "AMB-01", 5000));
    assert(traffic.GetIntersection("INT_1")->currentSignal == "ROAD_A_GREEN");
    assert(traffic.GetIntersection("INT_1")->currentMode == IntersectionControlMode::EMERGENCY_CORRIDOR);
    assert(traffic.GetIntersection("INT_1")->currentAmbulanceId == "AMB-01");

    // Vehicle detection from IR sensor
    assert(traffic.RecordVehicleDetection("INT_1", 3));
    assert(traffic.GetIntersection("INT_1")->irSensorCount == 53);
    assert(traffic.GetIntersection("INT_1")->currentQueue == 3);

    LogPass("Roadside Intersections, Signal Lamp States, & Sensor Counts verified.");
}

// ==============================================================================
// TEST 6: Raspberry Pi Cluster Monitor (Coordinator, Node A, B, C, CPU, RAM, Latency)
// ==============================================================================
static void TestClusterMonitor() {
    std::cout << "[TEST 6] Raspberry Pi Cluster Monitor..." << std::endl;

    ClusterMonitor cluster;
    auto nodes = cluster.GetAllNodes();
    assert(nodes.size() == 4);
    assert(cluster.AreAllNodesOnline());

    auto coord = cluster.GetNode("RPI_COORD");
    assert(coord.has_value());
    assert(coord->role == "COORDINATOR");
    assert(coord->networkLatencyMs < 1.0);
    assert(coord->syncStatus == "SYNCHRONIZED");

    // Update heartbeat telemetry
    assert(cluster.UpdateHeartbeat("INT_1", 18.5, 30.2, 12, 0.75, 4, 12000));
    auto n1 = cluster.GetNode("INT_1");
    assert(std::abs(n1->cpuUsagePercent - 18.5) < 0.01);
    assert(n1->currentThreadCount == 12);
    assert(n1->clockDriftMicroseconds == 4);

    // Simulate node offline
    assert(cluster.SetNodeOnline("INT_3", false));
    assert(!cluster.AreAllNodesOnline());
    assert(!cluster.GetNode("INT_3")->isOnline);

    assert(cluster.SetNodeOnline("INT_3", true));
    assert(cluster.AreAllNodesOnline());

    LogPass("4-Node Cluster Health, CPU/RAM, Latency, & Clock Drift verified.");
}

// ==============================================================================
// TEST 7: System Health Monitor (11 Subsystems, HEALTHY/WARNING/FAILED, Composite)
// ==============================================================================
static void TestSystemHealthMonitor() {
    std::cout << "[TEST 7] System Health Monitor..." << std::endl;

    SystemHealthMonitor health;
    auto reports = health.GetAllReports();
    assert(reports.size() == 11);
    assert(health.GetOverallHealth() == HealthStatusGrade::HEALTHY);

    // Update individual sensor health
    assert(health.ReportHealth("IR Sensors", HealthStatusGrade::WARNING, "Optical beam intermittent jitter on Node C"));
    assert(health.GetSubsystemHealth("IR Sensors")->status == HealthStatusGrade::WARNING);
    assert(health.GetOverallHealth() == HealthStatusGrade::WARNING);

    // Report critical relay failure
    assert(health.ReportHealth("Relay Status", HealthStatusGrade::FAILED, "Relay coil 2 open circuit"));
    assert(health.GetOverallHealth() == HealthStatusGrade::FAILED);

    // Restore to healthy
    assert(health.ReportHealth("Relay Status", HealthStatusGrade::HEALTHY, "Nominal"));
    assert(health.ReportHealth("IR Sensors", HealthStatusGrade::HEALTHY, "Nominal"));
    assert(health.GetOverallHealth() == HealthStatusGrade::HEALTHY);

    LogPass("11 Monitored Subsystems & Composite Health Evaluation verified.");
}

// ==============================================================================
// TEST 8: Real-Time Operational Analytics (15 Missions, 138s Avg Response, KPIs)
// ==============================================================================
static void TestRealTimeAnalyticsEngine() {
    std::cout << "[TEST 8] Real-Time Operational Analytics..." << std::endl;

    RealTimeAnalyticsEngine analytics;
    auto snap = analytics.GetSnapshot();

    assert(snap.todaysMissionsCount == 15);
    assert(snap.completedMissionsCount == 14);
    assert(std::abs(snap.averageResponseTimeSec - 138.0) < 0.1);
    assert(std::abs(snap.averageCorridorTimeSec - 58.4) < 0.1);
    assert(snap.averageEtaAccuracyPercent > 95.0);
    assert(snap.greenCorridorsCount == 18);
    assert(snap.policeAdvisoriesCount == 2);
    assert(snap.dynamicReplansCount == 2);

    // Record new completion
    analytics.RecordMissionCompletion(130.0, 52.0, 98.0);
    snap = analytics.GetSnapshot();
    assert(snap.completedMissionsCount == 15);
    assert(snap.averageResponseTimeSec < 138.0);

    // Increment KPIs
    analytics.IncrementGreenCorridor();
    analytics.IncrementPoliceAdvisory();
    analytics.IncrementDynamicReplan();
    analytics.IncrementSignalActivation();
    snap = analytics.GetSnapshot();
    assert(snap.greenCorridorsCount == 19);
    assert(snap.policeAdvisoriesCount == 3);
    assert(snap.dynamicReplansCount == 3);
    assert(snap.trafficSignalActivations == 55);

    LogPass("Operational KPIs, Corridor Duration, & SLA Aggregations verified.");
}

// ==============================================================================
// TEST 9: Mission Replay Controller (9 Chronological Execution Stages)
// ==============================================================================
static void TestMissionReplayController() {
    std::cout << "[TEST 9] Mission Replay Controller..." << std::endl;

    MissionDatabase db;
    auto mOpt = db.GetMission("MSN-2026-007");
    assert(mOpt.has_value());

    MissionReplayController replay;
    assert(replay.LoadMission(*mOpt));
    assert(replay.GetTotalSteps() == 9);
    assert(replay.GetCurrentStepIndex() == 0);

    // Verify stage 0
    auto ev0 = replay.GetCurrentEvent();
    assert(ev0.stageName == "DISPATCH");
    assert(!replay.IsAtEnd());

    // Step forward through all 9 stages
    std::vector<std::string> expectedStages = {
        "DISPATCH",
        "AMBULANCE_ASSIGNMENT",
        "ROUTE",
        "GREEN_CORRIDOR",
        "PRIORITY_SCHEDULING",
        "POLICE_ADVISORY",
        "DYNAMIC_REPLANNING",
        "HOSPITAL_ARRIVAL",
        "MISSION_COMPLETE"
    };

    for (size_t i = 0; i < expectedStages.size(); ++i) {
        assert(replay.GetCurrentEvent().stageName == expectedStages[i]);
        std::string asciiView = replay.RenderCurrentStageAscii();
        assert(!asciiView.empty());
        assert(asciiView.find(expectedStages[i]) != std::string::npos);

        if (i + 1 < expectedStages.size()) {
            assert(replay.StepForward());
        }
    }
    assert(replay.IsAtEnd());

    // Step backward
    assert(replay.StepBackward());
    assert(replay.GetCurrentEvent().stageName == "HOSPITAL_ARRIVAL");

    // Rewind
    replay.Rewind();
    assert(replay.GetCurrentStepIndex() == 0);
    assert(replay.GetCurrentEvent().stageName == "DISPATCH");

    // Seek
    assert(replay.SeekToStep(6));
    assert(replay.GetCurrentEvent().stageName == "DYNAMIC_REPLANNING");

    LogPass("9-Stage Mission Replay, Bidirectional Step, & ASCII Stage Render verified.");
}

// ==============================================================================
// TEST 10: EventBus Bridge (Transparent Subscription & Telemetry Ingestion)
// ==============================================================================
static void TestCommandCenterEventBridge() {
    std::cout << "[TEST 10] CommandCenter EventBridge Integration..." << std::endl;

    auto eventBus = std::make_shared<EventBus>();
    auto fleet = std::make_shared<EmsFleetManager>();
    auto tracker = std::make_shared<LiveAmbulanceTracker>();
    auto missions = std::make_shared<MissionDatabase>();
    auto hospitals = std::make_shared<HospitalOperationsDashboard>();
    auto traffic = std::make_shared<TrafficOperationsDashboard>();
    auto cluster = std::make_shared<ClusterMonitor>();
    auto health = std::make_shared<SystemHealthMonitor>();
    auto analytics = std::make_shared<RealTimeAnalyticsEngine>();

    CommandCenterEventBridge bridge(
        eventBus, fleet, tracker, missions, hospitals, traffic, cluster, health, analytics
    );

    assert(!bridge.IsConnected());
    bridge.Connect();
    assert(bridge.IsConnected());
    assert(bridge.GetActiveSubscriptionCount() >= 15);

    // Publish EmergencyCreated event
    Event evCreate(EventType::EmergencyCreated, "INC-EV-001", "", "CARDIAC_ARREST", 1000);
    eventBus->Publish(evCreate);

    auto m = missions->GetMission("INC-EV-001");
    assert(m.has_value());
    assert(m->emergencyType == "CARDIAC_ARREST");

    // Publish EmergencyAssigned event
    Event evAssign(EventType::EmergencyAssigned, "INC-EV-001", "AMB-02", "", 1200);
    eventBus->Publish(evAssign);
    assert(fleet->GetAmbulance("AMB-02")->currentStatus == AmbulanceStatus::ASSIGNED);
    assert(tracker->GetTracking("AMB-02").has_value());

    // Publish ExecutionStarted event
    Event evExecStart(EventType::ExecutionStarted, "AMB-02", "", "", 1500);
    eventBus->Publish(evExecStart);
    assert(traffic->GetIntersection("INT_1")->currentMode == IntersectionControlMode::EMERGENCY_CORRIDOR);
    assert(tracker->GetTracking("AMB-02")->greenCorridorStatus == "ACTIVE");

    // Publish HeartbeatReceived event
    Event evHeartbeat(EventType::HeartbeatReceived, "INT_2", "COORDINATOR", "", 2000);
    eventBus->Publish(evHeartbeat);
    assert(cluster->GetNode("INT_2")->isOnline);

    bridge.Disconnect();
    assert(!bridge.IsConnected());

    LogPass("EventBus Read-Only Subscription, Event Dispatch, & State Synchronization verified.");
}

// ==============================================================================
// TEST 11: Command Center Console ASCII Formatting
// ==============================================================================
static void TestCommandCenterConsole() {
    std::cout << "[TEST 11] Command Center ASCII Console..." << std::endl;

    auto fleet = std::make_shared<EmsFleetManager>();
    auto tracker = std::make_shared<LiveAmbulanceTracker>();
    auto missions = std::make_shared<MissionDatabase>();
    auto hospitals = std::make_shared<HospitalOperationsDashboard>();
    auto traffic = std::make_shared<TrafficOperationsDashboard>();
    auto cluster = std::make_shared<ClusterMonitor>();
    auto health = std::make_shared<SystemHealthMonitor>();
    auto analytics = std::make_shared<RealTimeAnalyticsEngine>();

    CommandCenterConsole console(
        fleet, tracker, missions, hospitals, traffic, cluster, health, analytics
    );

    // Test Executive Header
    std::string header = console.RenderExecutiveHeader();
    assert(header.find("SMART AEGIS COMMAND CENTER") != std::string::npos);
    assert(header.find("Coordinator\nONLINE") != std::string::npos);
    assert(header.find("Traffic Nodes\nONLINE") != std::string::npos);
    assert(header.find("Ambulances\n12") != std::string::npos);
    assert(header.find("Available\n8") != std::string::npos);
    assert(header.find("Assigned\n3") != std::string::npos);
    assert(header.find("Maintenance\n1") != std::string::npos);
    assert(header.find("Hospitals\n6") != std::string::npos);
    assert(header.find("Incoming Patients\n3") != std::string::npos);
    assert(header.find("Today's Missions\n15") != std::string::npos);
    assert(header.find("Average Response\n2m 18s") != std::string::npos);

    // Test Mission Queue
    std::string queue = console.RenderMissionQueue();
    assert(queue.find("Mission Queue") != std::string::npos);

    // Test Intersections
    std::string intStatus = console.RenderIntersectionStatus();
    assert(intStatus.find("Intersection 1") != std::string::npos);

    // Test Hospitals
    std::string hospStatus = console.RenderHospitalStatus();
    assert(hospStatus.find("City Trauma Hospital") != std::string::npos);

    // Test Cluster
    std::string clusterStatus = console.RenderClusterStatus();
    assert(clusterStatus.find("RPI_COORD") != std::string::npos);

    // Test Health
    std::string healthStatus = console.RenderSystemHealth();
    assert(healthStatus.find("Relay Status") != std::string::npos);

    // Test Analytics
    std::string analyticsStatus = console.RenderAnalytics();
    assert(analyticsStatus.find("Average Response Time") != std::string::npos);

    // Test Full Dashboard
    std::string fullDash = console.RenderFullDashboard();
    assert(fullDash.size() > 500);

    // Test stream printing
    std::ostringstream oss;
    console.Print(oss);
    assert(oss.str() == fullDash);

    LogPass("ASCII Operations Console Formatting & Exact Header Matching verified.");
}

// ==============================================================================
// TEST 12: Command Center Facade & End-to-End Orchestration
// ==============================================================================
static void TestCommandCenterFacade() {
    std::cout << "[TEST 12] Command Center Facade & End-to-End Lifecycle..." << std::endl;

    auto eventBus = std::make_shared<EventBus>();
    CommandCenterFacade facade(eventBus);

    assert(!facade.IsRunning());
    facade.Start();
    assert(facade.IsRunning());

    // Verify sub-components accessible
    assert(facade.GetFleetManager() != nullptr);
    assert(facade.GetAmbulanceTracker() != nullptr);
    assert(facade.GetMissionDatabase() != nullptr);
    assert(facade.GetHospitalDashboard() != nullptr);
    assert(facade.GetTrafficDashboard() != nullptr);
    assert(facade.GetClusterMonitor() != nullptr);
    assert(facade.GetHealthMonitor() != nullptr);
    assert(facade.GetAnalyticsEngine() != nullptr);
    assert(facade.GetReplayController() != nullptr);
    assert(facade.GetConsole() != nullptr);
    assert(facade.GetEventBridge() != nullptr);

    // Load mission for replay via facade
    assert(facade.LoadMissionForReplay("MSN-2026-007"));
    assert(facade.GetReplayController()->GetCurrentStepIndex() == 0);

    // Render full dashboard via facade
    std::string dash = facade.RenderDashboard();
    assert(!dash.empty());
    assert(dash.find("SMART AEGIS COMMAND CENTER") != std::string::npos);

    facade.Stop();
    assert(!facade.IsRunning());

    LogPass("CommandCenterFacade Lifecycle & Unified Operations Integration verified.");
}

// ==============================================================================
// MAIN ENTRY POINT
// ==============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS - PHASE 13 COMMAND CENTER TEST SUITE" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestEmsFleetManager();
    TestLiveAmbulanceTracker();
    TestMissionDatabase();
    TestHospitalOperationsDashboard();
    TestTrafficOperationsDashboard();
    TestClusterMonitor();
    TestSystemHealthMonitor();
    TestRealTimeAnalyticsEngine();
    TestMissionReplayController();
    TestCommandCenterEventBridge();
    TestCommandCenterConsole();
    TestCommandCenterFacade();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 12 PHASE 13 COMMAND CENTER SUITES PASSED (100% OK)" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
