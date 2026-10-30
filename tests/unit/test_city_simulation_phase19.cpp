/**
 * @file test_city_simulation_phase19.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Master Municipal City Simulation Engine Test Suite.
 */

#include "simulator/SimulationEngine.h"
#include "dashboard/DashboardDataModel.h"
#include "dashboard/traffic/TrafficDashboardDataModel.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <thread>
#include <chrono>

using namespace aegis::simulator;

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  ✓ PASS: " << msg << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "  ✗ FAIL: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            g_failCount++; \
        } \
    } while (0)

// Test 1: Simulation Initialization
void Test_SimulationInitialization() {
    std::cout << "[TEST 1] Simulation Initialization & Topology Verification..." << std::endl;
    SimulationEngine engine("config/simulation.json");

    TEST_ASSERT(engine.Initialize("config/simulation.json"), "Engine initialized successfully");
    TEST_ASSERT(!engine.IsRunning(), "Engine is not running initially");
    TEST_ASSERT(!engine.IsPaused(), "Engine clock is not paused initially");

    const auto& city = engine.GetCityModel();
    TEST_ASSERT(city.GetAllIntersections().size() == 3, "Loaded 3 virtual intersections");
    TEST_ASSERT(city.GetAllHospitals().size() == 3, "Loaded 3 virtual hospitals");
    TEST_ASSERT(city.GetAllAmbulances().size() == 3, "Loaded 3 virtual ambulances");

    auto intA = city.GetIntersection("INT_A");
    auto intB = city.GetIntersection("INT_B");
    auto intC = city.GetIntersection("INT_C");
    TEST_ASSERT(intA != nullptr && intB != nullptr && intC != nullptr, "All intersection nodes resolved");

    auto hospTrauma = city.GetHospital("HOSP_CITY_TRAUMA");
    TEST_ASSERT(hospTrauma != nullptr, "City Trauma Center hospital resolved");
}

// Test 2: Road Graph & Routing
void Test_RoadGraphAndRouting() {
    std::cout << "[TEST 2] Road Graph & Dijkstra Shortest Path..." << std::endl;
    SimulationEngine engine;
    auto& graph = engine.GetCityModel().GetRoadGraph();

    TEST_ASSERT(graph.GetAllNodes().size() >= 6, "Road graph contains at least 6 nodes");
    TEST_ASSERT(graph.GetAllEdges().size() >= 10, "Road graph contains at least 10 directed edges");

    auto path = graph.FindShortestPath("AMB_STATION_1", "HOSP_CITY_TRAUMA");
    TEST_ASSERT(!path.empty(), "Shortest path from Depot to Trauma Center found");
    TEST_ASSERT(path.front() == "AMB_STATION_1", "Path starts at AMB_STATION_1");
    TEST_ASSERT(path.back() == "HOSP_CITY_TRAUMA", "Path ends at HOSP_CITY_TRAUMA");

    double travelTime = graph.CalculatePathTravelTimeSeconds(path);
    TEST_ASSERT(travelTime > 0.0, "Path travel time is positive");
}

// Test 3: Vehicle Spawning & Kinematics
void Test_VehicleSpawningAndKinematics() {
    std::cout << "[TEST 3] Vehicle Spawning & Kinematic Updates..." << std::endl;
    SimulationEngine engine;
    auto& city = engine.GetCityModel();

    city.SpawnCivilianTraffic(15);
    auto vehicles = city.GetAllVehicles();
    TEST_ASSERT(vehicles.size() == 15, "15 civilian vehicles spawned");

    auto v0 = vehicles[0];
    double initLat = v0->GetLatitude();
    double initLon = v0->GetLongitude();

    // Advance 5 seconds of kinematic simulation
    for (int i = 0; i < 50; ++i) {
        city.Update(0.1, 8.5);
    }

    double finalLat = v0->GetLatitude();
    double finalLon = v0->GetLongitude();
    TEST_ASSERT(finalLat != initLat || finalLon != initLon || v0->HasArrived(),
                "Vehicle kinematic movement progressed along waypoints");
}

// Test 4: Emergency Generator
void Test_EmergencyGenerator() {
    std::cout << "[TEST 4] Emergency Generator & Clinical Records..." << std::endl;
    SimulationEngine engine;
    auto& gen = engine.GetEmergencyGenerator();

    auto cardiac = gen.GenerateCardiacArrest();
    TEST_ASSERT(cardiac.type == EmergencyType::CARDIAC_ARREST, "Generated cardiac arrest emergency");
    TEST_ASSERT(cardiac.priorityLevel == "P1_CRITICAL", "Cardiac arrest marked P1_CRITICAL");
    TEST_ASSERT(cardiac.patient.heartRateBpm > 100, "Patient vitals show tachycardia");
    TEST_ASSERT(cardiac.requiresIcu, "Cardiac arrest requires ICU bed");

    auto stroke = gen.GenerateStroke();
    TEST_ASSERT(stroke.type == EmergencyType::ACUTE_STROKE, "Generated acute stroke emergency");
    TEST_ASSERT(stroke.clinicalPriorityScore > 80.0, "Stroke clinical score > 80");

    auto accident = gen.GenerateRoadAccident();
    TEST_ASSERT(accident.type == EmergencyType::ROAD_ACCIDENT, "Generated road accident");
}

// Test 5: Hospital Assignment & Capacities
void Test_HospitalAssignmentAndCapacities() {
    std::cout << "[TEST 5] Hospital Bed Capacities & Load Tracking..." << std::endl;
    SimulationEngine engine;
    auto hosp = engine.GetCityModel().GetHospital("HOSP_CITY_TRAUMA");
    TEST_ASSERT(hosp != nullptr, "Trauma hospital located");

    uint32_t initIcu = hosp->GetAvailableIcuBeds();
    uint32_t initEd = hosp->GetAvailableEdBeds();
    TEST_ASSERT(initIcu > 0 && initEd > 0, "Hospital has initial capacity");

    bool reserved = hosp->ReserveBed(true);
    TEST_ASSERT(reserved, "ICU bed reserved successfully");
    TEST_ASSERT(hosp->GetAvailableIcuBeds() == initIcu - 1, "Available ICU beds decremented");

    hosp->SetDiversion(true);
    TEST_ASSERT(hosp->IsDiverting(), "Hospital marked on diversion");
    hosp->SetDiversion(false);
    TEST_ASSERT(!hosp->IsDiverting(), "Hospital diversion cleared");
}

// Test 6: Ambulance Dispatch & Lifecycle
void Test_AmbulanceDispatchAndLifecycle() {
    std::cout << "[TEST 6] Ambulance Dispatch & Complete Lifecycle..." << std::endl;
    SimulationEngine engine;
    auto amb = engine.GetCityModel().GetAmbulance("AMB_01");
    TEST_ASSERT(amb != nullptr, "Ambulance AMB_01 located");
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::IDLE, "Ambulance starts in IDLE");

    SimulatedPatientVitals vitals;
    vitals.patientId = "PAT_TEST_99";
    vitals.name = "John Doe";

    std::vector<std::pair<double, double>> route = {
        {12.9680, 77.5900},
        {12.9716, 77.5946},
        {12.9750, 77.5985}
    };

    amb->Dispatch("MISSION_001", "INC_001",
                  12.9716, 77.5946,
                  "HOSP_CITY_TRAUMA",
                  12.9750, 77.5985,
                  vitals,
                  route);

    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::DISPATCH, "Ambulance transitioned to DISPATCH");
    TEST_ASSERT(amb->GetCurrentMissionId() == "MISSION_001", "Mission ID assigned");

    // Progress through lifecycle stages
    amb->SetLifecycle(AmbulanceLifecycle::EN_ROUTE_TO_PATIENT);
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::EN_ROUTE_TO_PATIENT, "EN_ROUTE_TO_PATIENT");

    amb->SetLifecycle(AmbulanceLifecycle::PATIENT_PICKUP);
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::PATIENT_PICKUP, "PATIENT_PICKUP");

    amb->SetLifecycle(AmbulanceLifecycle::TRANSPORT);
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::TRANSPORT, "TRANSPORT");

    amb->SetLifecycle(AmbulanceLifecycle::HOSPITAL_ARRIVAL);
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::HOSPITAL_ARRIVAL, "HOSPITAL_ARRIVAL");

    amb->SetLifecycle(AmbulanceLifecycle::MISSION_COMPLETE);
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::MISSION_COMPLETE, "MISSION_COMPLETE");

    amb->ResetToBase();
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::IDLE, "Ambulance reset to IDLE at base");
}

// Test 7: Intersection Signal Cycles & Queues
void Test_IntersectionSignalCyclesAndQueues() {
    std::cout << "[TEST 7] Intersection 4-Phase Cycles & Queues..." << std::endl;
    SimulationEngine engine;
    auto inter = engine.GetCityModel().GetIntersection("INT_A");
    TEST_ASSERT(inter != nullptr, "Intersection INT_A located");

    SignalPhaseState p0 = inter->GetCurrentPhase();
    TEST_ASSERT(p0 == SignalPhaseState::NORTH_SOUTH_GREEN, "Starts in NORTH_SOUTH_GREEN");

    inter->SetQueueCount(8);
    TEST_ASSERT(inter->GetQueueCount() == 8, "Queue count set to 8");

    inter->AddVehiclesToQueue(4);
    TEST_ASSERT(inter->GetQueueCount() == 12, "Queue count incremented to 12");

    inter->RemoveVehiclesFromQueue(5);
    TEST_ASSERT(inter->GetQueueCount() == 7, "Queue count decremented to 7");

    // Advance 30 seconds to force phase transition
    for (int i = 0; i < 300; ++i) {
        inter->Update(0.1);
    }
    TEST_ASSERT(inter->GetSignalState() != "", "Signal state is valid non-empty string");
}

// Test 8: Optical IR Detection
void Test_OpticalIrDetection() {
    std::cout << "[TEST 8] Optical IR Beam Triggers..." << std::endl;
    SimulationEngine engine;
    auto inter = engine.GetCityModel().GetIntersection("INT_B");
    TEST_ASSERT(inter != nullptr, "Intersection INT_B located");

    TEST_ASSERT(!inter->IsApproachIrTriggered(), "Approach IR initially inactive");
    TEST_ASSERT(!inter->IsDepartureIrTriggered(), "Departure IR initially inactive");

    inter->TriggerApproachIR();
    TEST_ASSERT(inter->IsApproachIrTriggered(), "Approach IR triggered");

    inter->TriggerDepartureIR();
    TEST_ASSERT(inter->IsDepartureIrTriggered(), "Departure IR triggered");

    // Advance 5 seconds to test auto-reset of IR pulse
    for (int i = 0; i < 50; ++i) {
        inter->Update(0.1);
    }
    TEST_ASSERT(!inter->IsApproachIrTriggered(), "Approach IR auto-reset after pulse duration");
    TEST_ASSERT(!inter->IsDepartureIrTriggered(), "Departure IR auto-reset after pulse duration");
}

// Test 9: Traffic Dynamics & Diurnal Curves
void Test_TrafficDynamicsAndDiurnalCurves() {
    std::cout << "[TEST 9] Traffic Dynamics & Diurnal Congestion Curves..." << std::endl;
    SimulationEngine engine;
    auto& city = engine.GetCityModel();

    // Morning peak (08:30)
    city.Update(1.0, 8.5);
    double morningDensity = city.GetTrafficDensity();

    // Night low traffic (02:00)
    city.Update(1.0, 2.0);
    double nightDensity = city.GetTrafficDensity();

    TEST_ASSERT(morningDensity > nightDensity, "Morning peak density exceeds night valley density");

    // Evening peak (18:00)
    city.Update(1.0, 18.0);
    double eveningDensity = city.GetTrafficDensity();
    TEST_ASSERT(eveningDensity > nightDensity, "Evening peak density exceeds night valley density");
}

// Test 10: Live Disruptions
void Test_LiveDisruptions() {
    std::cout << "[TEST 10] Live Municipal Disruptions..." << std::endl;
    SimulationEngine engine;

    LiveDisruptionEvent ev;
    ev.id = "DISRUPT_01";
    ev.type = "ROAD_CLOSURE";
    ev.targetEntityId = "EDGE_A_B";
    ev.description = "Emergency Gas Main Repair";
    ev.durationSec = 100.0;
    ev.isActive = true;

    engine.TriggerDisruption(ev);
    auto active = engine.GetCityModel().GetActiveEvents();
    TEST_ASSERT(active.size() == 1, "1 active disruption event registered");
    TEST_ASSERT(active[0].type == "ROAD_CLOSURE", "Disruption type is ROAD_CLOSURE");

    engine.ClearDisruptions();
    TEST_ASSERT(engine.GetCityModel().GetActiveEvents().empty(), "Disruptions cleared cleanly");
}

// Test 11: Weather Conditions
void Test_WeatherConditions() {
    std::cout << "[TEST 11] Weather Condition Transitions..." << std::endl;
    SimulationEngine engine;

    TEST_ASSERT(engine.GetWeather() == WeatherCondition::CLEAR, "Initial weather is CLEAR");
    engine.SetWeather(WeatherCondition::HEAVY_RAIN);
    TEST_ASSERT(engine.GetWeather() == WeatherCondition::HEAVY_RAIN, "Weather set to HEAVY_RAIN");
    TEST_ASSERT(WeatherConditionToString(engine.GetWeather()) == "HEAVY_RAIN", "Weather string verified");

    engine.SetWeather(WeatherCondition::THUNDERSTORM);
    TEST_ASSERT(engine.GetWeather() == WeatherCondition::THUNDERSTORM, "Weather set to THUNDERSTORM");
}

// Test 12: Simulation Speed & Clock Controls
void Test_SimulationSpeedAndClockControls() {
    std::cout << "[TEST 12] Simulation Speed Multipliers & Clock Controls..." << std::endl;
    SimulationEngine engine;

    engine.SetSpeedMultiplier(SimulationSpeedMultiplier::SPEED_2X);
    TEST_ASSERT(engine.GetSpeedMultiplier() == SimulationSpeedMultiplier::SPEED_2X, "2x speed set");

    engine.SetSpeedMultiplier(SimulationSpeedMultiplier::SPEED_5X);
    TEST_ASSERT(engine.GetSpeedMultiplier() == SimulationSpeedMultiplier::SPEED_5X, "5x speed set");

    engine.SetSpeedMultiplier(SimulationSpeedMultiplier::SPEED_10X);
    TEST_ASSERT(engine.GetSpeedMultiplier() == SimulationSpeedMultiplier::SPEED_10X, "10x speed set");

    engine.Pause();
    TEST_ASSERT(engine.IsPaused(), "Engine paused");

    engine.Resume();
    TEST_ASSERT(!engine.IsPaused(), "Engine resumed");

    engine.TogglePause();
    TEST_ASSERT(engine.IsPaused(), "Toggle pause paused");
    engine.TogglePause();
    TEST_ASSERT(!engine.IsPaused(), "Toggle pause resumed");

    engine.Step(0.1);
    TEST_ASSERT(engine.GetClock().GetElapsedSimSeconds() > 0.0, "Single step advanced elapsed seconds");

    engine.Reset();
    TEST_ASSERT(engine.GetClock().GetElapsedSimSeconds() == 0.0, "Clock reset to 0 elapsed seconds");
}

// Test 13: Demonstration Mode 1 (Software Simulation)
void Test_DemonstrationMode1() {
    std::cout << "[TEST 13] Mode 1: Full Municipal Software Simulation..." << std::endl;
    SimulationEngine engine;
    engine.SetDemonstrationMode(DemonstrationMode::MODE_1_SOFTWARE_SIMULATION);

    TEST_ASSERT(engine.GetDemonstrationMode() == DemonstrationMode::MODE_1_SOFTWARE_SIMULATION, "Mode 1 set");
    TEST_ASSERT(engine.GetCityModel().GetAllVehicles().size() >= 15, "Full civilian vehicle fleet active");
    TEST_ASSERT(engine.GetCityModel().GetAllIntersections().size() == 3, "All 3 intersections active in Mode 1");

    // Dispatch emergency in Mode 1
    auto emg = engine.DispatchEmergency(EmergencyType::CARDIAC_ARREST, "AMB_01", "HOSP_CITY_TRAUMA");
    TEST_ASSERT(!emg.incidentId.empty(), "Emergency dispatched in Mode 1");
    auto amb = engine.GetCityModel().GetAmbulance("AMB_01");
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::DISPATCH, "Ambulance dispatched");
}

// Test 14: Demonstration Mode 2 (Single Ambulance Hardware Mode)
void Test_DemonstrationMode2() {
    std::cout << "[TEST 14] Mode 2: Single Ambulance Hardware Mode..." << std::endl;
    SimulationEngine engine;
    engine.SetDemonstrationMode(DemonstrationMode::MODE_2_SINGLE_AMBULANCE_HARDWARE);

    TEST_ASSERT(engine.GetDemonstrationMode() == DemonstrationMode::MODE_2_SINGLE_AMBULANCE_HARDWARE, "Mode 2 set");

    auto amb = engine.GetCityModel().GetAmbulance("AMB_01");
    TEST_ASSERT(amb != nullptr, "AMB_01 active");
    TEST_ASSERT(amb->GetLifecycle() == AmbulanceLifecycle::DISPATCH, "AMB_01 dispatched along green corridor");

    auto intA = engine.GetCityModel().GetIntersection("INT_A");
    auto intB = engine.GetCityModel().GetIntersection("INT_B");
    auto intC = engine.GetCityModel().GetIntersection("INT_C");
    TEST_ASSERT(intA->IsPreempted() && intB->IsPreempted() && intC->IsPreempted(),
                "Corridor intersections preempted for single ambulance hardware mode");
}

// Test 15: Demonstration Mode 3 (Dual Ambulance Conflict & Dynamic Arbitration)
void Test_DemonstrationMode3() {
    std::cout << "[TEST 15] Mode 3: Dual Ambulance Conflict Resolution & Arbitration..." << std::endl;
    SimulationEngine engine;
    engine.SetDemonstrationMode(DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT);

    TEST_ASSERT(engine.GetDemonstrationMode() == DemonstrationMode::MODE_3_DUAL_AMBULANCE_CONFLICT, "Mode 3 set");

    auto status = engine.GetConflictStatus();
    TEST_ASSERT(status.conflictDetected, "Intersection conflict detected at INT_B");
    TEST_ASSERT(status.intersectionId == "INT_B", "Conflict location is INT_B");
    TEST_ASSERT(status.grantedAmbulanceId == "AMB_01", "AMB_01 granted priority (Cardiac 95.0 > Trauma 80.0)");
    TEST_ASSERT(status.heldAmbulanceId == "AMB_02", "AMB_02 held in queue waiting clearance");

    auto intB = engine.GetCityModel().GetIntersection("INT_B");
    TEST_ASSERT(intB->IsPreempted(), "INT_B preempted for high priority ambulance");
    TEST_ASSERT(intB->GetActiveCorridorId() == "CORRIDOR_AMB_01", "Active corridor is CORRIDOR_AMB_01");

    // Advance simulation to simulate AMB_01 passing through and clearing INT_B
    auto amb1 = engine.GetCityModel().GetAmbulance("AMB_01");
    // Manually progress AMB_01 position past INT_B
    amb1->SetRoute({{12.9750, 77.5985}}); // At hospital

    engine.Step(1.0);

    auto updatedStatus = engine.GetConflictStatus();
    TEST_ASSERT(updatedStatus.isCleared, "Conflict cleared after AMB_01 departed INT_B");
    TEST_ASSERT(intB->GetActiveCorridorId() == "CORRIDOR_AMB_02",
                "Dynamic Corridor Arbitration switched green corridor to AMB_02");
}

// Test 16: Dashboard Integration
void Test_DashboardIntegration() {
    std::cout << "[TEST 16] Dashboard Integration (Phase 17 & Phase 18 Models)..." << std::endl;
    SimulationEngine engine;

    auto controlCentreModel = std::make_shared<aegis::dashboard::DashboardDataModel>();
    auto trafficModel = std::make_shared<aegis::dashboard::traffic::TrafficDashboardDataModel>();

    engine.SetControlCentreDataModel(controlCentreModel);
    engine.SetTrafficDashboardDataModel(trafficModel);

    // Step the simulation to trigger event-driven publishing
    engine.Step(0.2);

    aegis::dashboard::IntersectionRecord interA;
    bool foundA = controlCentreModel->GetIntersection("INT_A", interA);
    TEST_ASSERT(foundA, "INT_A pushed to Phase 17 Control Centre model");

    auto trafficNodes = trafficModel->GetAllIntersections();
    TEST_ASSERT(trafficNodes.size() == 3, "3 intersections pushed to Phase 18 Traffic model");

    aegis::dashboard::FleetVehicleRecord amb1;
    bool foundAmb1 = controlCentreModel->GetFleetVehicle("AMB_01", amb1);
    TEST_ASSERT(foundAmb1, "AMB_01 telemetry pushed to Phase 17 Control Centre model");
}

// Test 17: Multi-Threaded Execution Stability
void Test_MultiThreadedExecutionStability() {
    std::cout << "[TEST 17] Multi-Threaded Execution Stability & Clean Shutdown..." << std::endl;
    SimulationEngine engine;

    engine.Start();
    TEST_ASSERT(engine.IsRunning(), "Engine successfully started independent worker threads");

    // Let threads execute concurrently for 200 milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    TEST_ASSERT(engine.GetClock().GetElapsedSimSeconds() > 0.0,
                "Independent clock thread advanced virtual simulation time");

    engine.Stop();
    TEST_ASSERT(!engine.IsRunning(), "Engine stopped and all 6 worker threads joined cleanly");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS (AERS) Phase 19: City Simulation Engine Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test_SimulationInitialization();
    Test_RoadGraphAndRouting();
    Test_VehicleSpawningAndKinematics();
    Test_EmergencyGenerator();
    Test_HospitalAssignmentAndCapacities();
    Test_AmbulanceDispatchAndLifecycle();
    Test_IntersectionSignalCyclesAndQueues();
    Test_OpticalIrDetection();
    Test_TrafficDynamicsAndDiurnalCurves();
    Test_LiveDisruptions();
    Test_WeatherConditions();
    Test_SimulationSpeedAndClockControls();
    Test_DemonstrationMode1();
    Test_DemonstrationMode2();
    Test_DemonstrationMode3();
    Test_DashboardIntegration();
    Test_MultiThreadedExecutionStability();

    std::cout << "==========================================================" << std::endl;
    std::cout << " Total Passed: " << g_passCount << " / " << (g_passCount + g_failCount) << std::endl;
    std::cout << " Total Failed: " << g_failCount << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_failCount == 0) ? 0 : 1;
}
