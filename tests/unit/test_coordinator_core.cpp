#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "aegis/common/types.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/core/emergency_call_handler.hpp"
#include "aegis/core/incident_manager.hpp"
#include "aegis/core/ambulance_registry.hpp"
#include "aegis/core/fleet_manager.hpp"
#include "aegis/core/dispatch_engine.hpp"
#include "aegis/core/route_planner.hpp"
#include "aegis/core/green_corridor_engine.hpp"
#include "aegis/core/coordinator.hpp"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n[FAIL] Assertion failed: (" #cond ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (false)

using namespace aegis;
using namespace aegis::core;
using namespace aegis::core::coordinator;
using namespace aegis::core::routing;

// =============================================================================
// Unit Tests: Coordinator Core (Phase 3)
// =============================================================================

static void TestEmergencyCallHandler() {
    std::cout << "[TEST] 1. EmergencyCallHandler Qualification & Geofencing... ";

    EmergencyCallHandler handler;
    handler.SetGeofence(12.70, 13.30, 77.30, 77.90);

    // 1. Critical Cardiac call -> CODE_RED
    EmergencyCallInput input1;
    input1.callerName = "John Doe";
    input1.callerContact = "+91-9876543210";
    input1.location = {12.9750, 77.6050, 0.0};
    input1.description = "Elderly patient collapsed, severe cardiac arrest, unconscious!";

    auto res1 = handler.QualifyCall(input1);
    AEGIS_TEST_ASSERT(res1.IsOk());
    const auto& rep1 = res1.Value();
    AEGIS_TEST_ASSERT(!rep1.incidentId.empty());
    AEGIS_TEST_ASSERT(rep1.triage == common::TriageCategory::CODE_RED);
    AEGIS_TEST_ASSERT(rep1.callerContact == input1.callerContact);

    // Check inferred capabilities for CODE_RED cardiac
    bool hasDefib = false;
    for (const auto& cap : rep1.requiredCapabilities) {
        if (cap == "DEFIBRILLATOR") hasDefib = true;
    }
    AEGIS_TEST_ASSERT(hasDefib);

    // 2. Fracture call -> CODE_AMBER
    EmergencyCallInput input2;
    input2.callerName = "Jane Smith";
    input2.callerContact = "+91-9123456789";
    input2.location = {12.9620, 77.6380, 0.0};
    input2.description = "Bicycle accident, severe fracture and moderate bleeding.";

    auto res2 = handler.QualifyCall(input2);
    AEGIS_TEST_ASSERT(res2.IsOk());
    AEGIS_TEST_ASSERT(res2.Value().triage == common::TriageCategory::CODE_AMBER);

    // 3. Out-of-bounds geofence rejection
    EmergencyCallInput inputBadGeo;
    inputBadGeo.callerName = "Out of Towner";
    inputBadGeo.callerContact = "+91-9999999999";
    inputBadGeo.location = {15.5000, 78.0000, 0.0}; // Far outside Bangalore
    inputBadGeo.description = "Emergency in another city.";

    auto resBadGeo = handler.QualifyCall(inputBadGeo);
    AEGIS_TEST_ASSERT(resBadGeo.IsErr());

    // 4. Empty caller contact rejection
    EmergencyCallInput inputNoContact;
    inputNoContact.callerContact = "";
    inputNoContact.location = {12.9750, 77.6050, 0.0};
    inputNoContact.description = "Valid location but no contact info.";

    auto resNoContact = handler.QualifyCall(inputNoContact);
    AEGIS_TEST_ASSERT(resNoContact.IsErr());

    // 5. Backward-compatible IngestCall
    auto compatRep = handler.IngestCall("Citizen", {12.9750, 77.6050, 0.0}, "Car crash on main road");
    AEGIS_TEST_ASSERT(!compatRep.incidentId.empty());
    AEGIS_TEST_ASSERT(compatRep.triage == common::TriageCategory::CODE_RED);

    std::cout << "PASSED" << std::endl;
}

static void TestIncidentManagerLifecycle() {
    std::cout << "[TEST] 2. IncidentManager Lifecycle & Queries... ";

    IncidentManager mgr;
    AEGIS_TEST_ASSERT(mgr.GetIncidentCount() == 0);

    IncidentReport rep;
    rep.incidentId = "INC-TEST-001";
    rep.callerName = "Alice";
    rep.callerContact = "+91-9876543210";
    rep.location = {12.9750, 77.6050, 0.0};
    rep.triage = common::TriageCategory::CODE_RED;
    rep.incidentType = "CARDIAC_ARREST";
    rep.requiredCapabilities = {"ECG", "VENTILATOR"};
    rep.description = "Emergency cardiac call";

    auto createRes = mgr.CreateIncident(rep);
    AEGIS_TEST_ASSERT(createRes.IsOk());
    AEGIS_TEST_ASSERT(createRes.Value() == "INC-TEST-001");
    AEGIS_TEST_ASSERT(mgr.GetIncidentCount() == 1);

    // Query record
    auto getRes = mgr.GetIncident("INC-TEST-001");
    AEGIS_TEST_ASSERT(getRes.IsOk());
    auto rec = getRes.Value();
    AEGIS_TEST_ASSERT(rec.status == IncidentStatus::REPORTED);
    AEGIS_TEST_ASSERT(rec.assignedUnitId.empty());

    // Assign Ambulance
    bool assignOk = mgr.AssignAmbulance("INC-TEST-001", "AMB-01", "ROUTE-99");
    AEGIS_TEST_ASSERT(assignOk);

    auto recAssigned = mgr.GetIncident("INC-TEST-001").Value();
    AEGIS_TEST_ASSERT(recAssigned.status == IncidentStatus::ASSIGNED);
    AEGIS_TEST_ASSERT(recAssigned.assignedUnitId == "AMB-01");
    AEGIS_TEST_ASSERT(recAssigned.activeRouteId == "ROUTE-99");

    // Lifecycle transitions
    AEGIS_TEST_ASSERT(mgr.UpdateStatus("INC-TEST-001", IncidentStatus::EN_ROUTE_SCENE));
    AEGIS_TEST_ASSERT(mgr.UpdateStatus("INC-TEST-001", IncidentStatus::ON_SCENE));
    AEGIS_TEST_ASSERT(mgr.UpdateStatus("INC-TEST-001", IncidentStatus::EN_ROUTE_HOSPITAL));
    AEGIS_TEST_ASSERT(mgr.GetActiveIncidents().size() == 1);

    // Resolve
    AEGIS_TEST_ASSERT(mgr.UpdateStatus("INC-TEST-001", IncidentStatus::RESOLVED));
    AEGIS_TEST_ASSERT(mgr.GetActiveIncidents().empty());
    AEGIS_TEST_ASSERT(mgr.GetIncident("INC-TEST-001").Value().resolvedAtMs > 0);

    std::cout << "PASSED" << std::endl;
}

static void TestAmbulanceRegistryAndFleetManager() {
    std::cout << "[TEST] 3. FleetManager Atomic Reservation & Telemetry... ";

    FleetManager fleet;
    fleet.PopulateSimulatedFleet();

    AEGIS_TEST_ASSERT(fleet.Registry().GetTotalFleetCount() == 4);
    auto avail = fleet.GetAvailableUnits();
    AEGIS_TEST_ASSERT(avail.size() == 4);

    // Atomic reservation of AMB-01
    bool res1 = fleet.ReserveUnit("AMB-01", "INC-001");
    AEGIS_TEST_ASSERT(res1);
    AEGIS_TEST_ASSERT(fleet.GetUnit("AMB-01").Value().state == common::AmbulanceState::DISPATCHED);
    AEGIS_TEST_ASSERT(fleet.GetUnit("AMB-01").Value().assignedIncidentId == "INC-001");

    // Double reservation must fail
    bool res2 = fleet.ReserveUnit("AMB-01", "INC-002");
    AEGIS_TEST_ASSERT(!res2);

    // Available count should now be 3
    AEGIS_TEST_ASSERT(fleet.GetAvailableUnits().size() == 3);

    // Update telemetry
    common::GeoCoordinate newPos{12.9730, 77.6010, 0.0};
    AEGIS_TEST_ASSERT(fleet.UpdateTelemetry("AMB-01", newPos, 62.5f, 90.0f));
    auto updatedAmb = fleet.GetUnit("AMB-01").Value();
    AEGIS_TEST_ASSERT(std::abs(updatedAmb.location.latitude - 12.9730) < 0.0001);
    AEGIS_TEST_ASSERT(std::abs(updatedAmb.speedKmH - 62.5f) < 0.1f);

    // Release unit
    AEGIS_TEST_ASSERT(fleet.ReleaseUnit("AMB-01"));
    AEGIS_TEST_ASSERT(fleet.GetUnit("AMB-01").Value().state == common::AmbulanceState::AVAILABLE);
    AEGIS_TEST_ASSERT(fleet.GetAvailableUnits().size() == 4);

    std::cout << "PASSED" << std::endl;
}

static void TestDispatchEngineSelection() {
    std::cout << "[TEST] 4. DispatchEngine Capability Matching & Proximity... ";

    FleetManager fleet;
    fleet.PopulateSimulatedFleet();
    DispatchEngine engine;

    // 1. Incident requiring Ventilator and Defibrillator (requires ALS)
    IncidentRecord incCritical;
    incCritical.incidentId = "INC-CRIT-01";
    incCritical.triageCategory = common::TriageCategory::CODE_RED;
    incCritical.location = {12.9750, 77.6050, 0.0}; // Near Central Depot (AMB-01)
    incCritical.requiredCapabilities = {"VENTILATOR", "DEFIBRILLATOR", "ECG"};

    auto dec1 = engine.SelectBestAmbulance(incCritical, fleet);
    AEGIS_TEST_ASSERT(dec1.IsOk());
    // AMB-01 is ALS and closest to (12.9750, 77.6050)
    AEGIS_TEST_ASSERT(dec1.Value().unitId == "AMB-01");
    AEGIS_TEST_ASSERT(dec1.Value().estimatedDistanceMeters > 0.0);
    AEGIS_TEST_ASSERT(dec1.Value().estimatedArrivalSec > 0.0);

    // 2. Incident in East sector requiring only basic kit
    IncidentRecord incBasic;
    incBasic.incidentId = "INC-BASIC-02";
    incBasic.triageCategory = common::TriageCategory::CODE_GREEN;
    incBasic.location = {12.9770, 77.6340, 0.0}; // Right next to Indiranagar (AMB-02)
    incBasic.requiredCapabilities = {"OXYGEN"};

    auto dec2 = engine.SelectBestAmbulance(incBasic, fleet);
    AEGIS_TEST_ASSERT(dec2.IsOk());
    AEGIS_TEST_ASSERT(dec2.Value().unitId == "AMB-02");

    // 3. When all matching units are reserved
    fleet.ReserveUnit("AMB-01", "INC-X");
    fleet.ReserveUnit("AMB-03", "INC-Y"); // Both ALS units reserved
    auto decNoMatch = engine.SelectBestAmbulance(incCritical, fleet);
    AEGIS_TEST_ASSERT(decNoMatch.IsErr()); // No ALS with Ventilator available

    std::cout << "PASSED" << std::endl;
}

static void TestRoutePlannerAStarAndDijkstra() {
    std::cout << "[TEST] 5. RoutePlanner A* Pathfinding & Intersections... ";

    RoutePlanner planner;
    AEGIS_TEST_ASSERT(planner.GetNodeCount() >= 20);
    AEGIS_TEST_ASSERT(planner.GetEdgeCount() > 0);

    // Origin: Central Ambulance Depot (INT-STA-01: 12.9700, 77.5900)
    // Destination: Central Trauma Hospital (INT-HOSP-01: 12.9800, 77.6000)
    common::GeoCoordinate startCoord{12.9700, 77.5900, 0.0};
    common::GeoCoordinate destCoord{12.9800, 77.6000, 0.0};

    auto route = planner.CalculateRoute(startCoord, destCoord);
    AEGIS_TEST_ASSERT(!route.routeId.empty());
    AEGIS_TEST_ASSERT(route.waypoints.size() >= 3);
    AEGIS_TEST_ASSERT(route.totalDistanceMeters > 500.0);
    AEGIS_TEST_ASSERT(route.estimatedTimeSec > 10.0);

    // Check that intersection passage vector is populated in traversal order
    AEGIS_TEST_ASSERT(!route.intersectionIds.empty());
    AEGIS_TEST_ASSERT(route.intersectionIds.front() == "INT-STA-01");
    AEGIS_TEST_ASSERT(route.intersectionIds.back() == "INT-HOSP-01");

    // Nearest node snapping
    std::string nearest = planner.FindNearestNode({12.9752, 77.6051, 0.0});
    AEGIS_TEST_ASSERT(nearest == "INT-MG-01");

    // ETA helper
    double eta = planner.CalculateETA(1000.0, 50.0); // 1000m at 50 km/h (~13.89 m/s) = 72s
    AEGIS_TEST_ASSERT(eta > 70.0 && eta < 75.0);

    std::cout << "PASSED" << std::endl;
}

static void TestGreenCorridorEngine() {
    std::cout << "[TEST] 6. GreenCorridorEngine Preemption Scheduling... ";

    GreenCorridorEngine corridor;
    std::string lastTriggeredIntersection;
    std::string lastReleasedIntersection;

    corridor.SetTriggerHandler([&](const std::string& intId, const network::TrafficPreemptionRequestMsg& req) {
        lastTriggeredIntersection = intId;
        AEGIS_TEST_ASSERT(req.corridorPriority == 2);
    });

    corridor.SetReleaseHandler([&](const std::string& intId, const std::string&) {
        lastReleasedIntersection = intId;
    });

    std::vector<std::string> intersections = {"INT-MG-01", "INT-MG-02", "INT-MG-03"};
    AEGIS_TEST_ASSERT(corridor.RegisterCorridor("CORR-001", "INC-001", "AMB-01", intersections));
    AEGIS_TEST_ASSERT(corridor.GetActiveCorridorCount() == 1);

    // Evaluate vehicle approaching first intersection
    corridor.EvaluateVehicleProgress("AMB-01", {12.9740, 77.6040, 0.0}, 55.0f, 45.0f);
    AEGIS_TEST_ASSERT(lastTriggeredIntersection == "INT-MG-01");
    AEGIS_TEST_ASSERT(corridor.IsIntersectionPreempted("INT-MG-01"));

    // Release intersection once crossed
    corridor.ReleaseCorridor("INT-MG-01");
    AEGIS_TEST_ASSERT(lastReleasedIntersection == "INT-MG-01");
    AEGIS_TEST_ASSERT(!corridor.IsIntersectionPreempted("INT-MG-01"));

    // Clear entire corridor
    corridor.ClearCorridor("CORR-001");
    AEGIS_TEST_ASSERT(corridor.GetActiveCorridorCount() == 0);

    std::cout << "PASSED" << std::endl;
}

static void TestCoordinatorEndToEndWorkflow() {
    std::cout << "[TEST] 7. Coordinator End-to-End Emergency Workflow... ";

    CentralCoordinator coordinator;
    AEGIS_TEST_ASSERT(coordinator.Start());
    AEGIS_TEST_ASSERT(coordinator.IsRunning());

    // Submit emergency call
    EmergencyCallInput call;
    call.callerName = "Security Officer";
    call.callerContact = "+91-9888877777";
    call.location = {12.9750, 77.6100, 0.0}; // Near MG Road / Residency
    call.description = "Pedestrian hit by vehicle, severe trauma, unconscious, critical emergency!";
    call.incidentTypeHint = "MAJOR_TRAUMA";

    auto dispatchRes = coordinator.SubmitEmergencyCall(call);
    AEGIS_TEST_ASSERT(dispatchRes.IsOk());
    const auto& result = dispatchRes.Value();

    AEGIS_TEST_ASSERT(result.success);
    AEGIS_TEST_ASSERT(!result.incidentId.empty());
    AEGIS_TEST_ASSERT(!result.assignedUnitId.empty());
    AEGIS_TEST_ASSERT(!result.corridorId.empty());
    AEGIS_TEST_ASSERT(!result.route.routeId.empty());
    AEGIS_TEST_ASSERT(!result.route.intersectionIds.empty());

    // Verify incident record in IncidentManager
    auto incRec = coordinator.Incidents().GetIncident(result.incidentId).Value();
    AEGIS_TEST_ASSERT(incRec.status == IncidentStatus::ASSIGNED);
    AEGIS_TEST_ASSERT(incRec.assignedUnitId == result.assignedUnitId);

    // Verify fleet unit is locked / reserved
    auto unitRec = coordinator.Fleet().GetUnit(result.assignedUnitId).Value();
    AEGIS_TEST_ASSERT(unitRec.state == common::AmbulanceState::DISPATCHED);
    AEGIS_TEST_ASSERT(unitRec.assignedIncidentId == result.incidentId);

    // Simulate GPS stream from vehicle
    coordinator.ProcessGpsUpdate(result.assignedUnitId, {12.9745, 77.6080, 0.0}, 60.0f, 85.0f);
    auto unitAfterGps = coordinator.Fleet().GetUnit(result.assignedUnitId).Value();
    AEGIS_TEST_ASSERT(std::abs(unitAfterGps.speedKmH - 60.0f) < 0.1f);

    coordinator.Stop();
    AEGIS_TEST_ASSERT(!coordinator.IsRunning());

    std::cout << "PASSED" << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " Running SMART AEGIS Coordinator Core (Phase 3) Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestEmergencyCallHandler();
    TestIncidentManagerLifecycle();
    TestAmbulanceRegistryAndFleetManager();
    TestDispatchEngineSelection();
    TestRoutePlannerAStarAndDijkstra();
    TestGreenCorridorEngine();
    TestCoordinatorEndToEndWorkflow();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All 7 Coordinator Core Test Suites Passed! " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
