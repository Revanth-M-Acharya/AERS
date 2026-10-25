/**
 * @file test_ambulance_node.cpp
 * @brief Comprehensive automated unit test suite for Phase 4: Ambulance Node (OBU).
 */

#include <iostream>
#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

#include "aegis/edge/gps_tracker.hpp"
#include "aegis/edge/vitals_monitor.hpp"
#include "aegis/edge/v2v_module.hpp"
#include "aegis/edge/ambulance_node.hpp"
#include "aegis/edge/ambulance_mission_simulator.hpp"
#include "aegis/core/coordinator.hpp"
#include "aegis/network/network_manager.hpp"
#include "aegis/network/serializer.hpp"
#include "aegis/common/geo_utils.hpp"
#include "aegis/common/time_utils.hpp"

#define AEGIS_TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[ASSERTION FAILED] " << #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

using namespace aegis;
using namespace aegis::edge::ambulance;

// ==============================================================================
// 1. GpsTracker Unit Tests
// ==============================================================================
void TestGpsTracker() {
    std::cout << "[TEST] 1. GpsTracker NMEA Parsing, Simulation & Dead-Reckoning... " << std::flush;

    GpsTracker gps;
    AEGIS_TEST_ASSERT(gps.Initialize());

    // 1.1 Checksum validation
    std::string validRmc = "$GPRMC,123519,A,1258.2959,N,07735.6737,E,35.0,88.5,230326,,,A*72";
    AEGIS_TEST_ASSERT(GpsTracker::ValidateNmeaChecksum(validRmc));

    std::string badChecksumRmc = "$GPRMC,123519,A,1258.2959,N,07735.6737,E,35.0,88.5,230326,,,A*00";
    AEGIS_TEST_ASSERT(!GpsTracker::ValidateNmeaChecksum(badChecksumRmc));

    // 1.2 Parse valid $GPRMC
    AEGIS_TEST_ASSERT(gps.ParseNmeaSentence(validRmc));
    VehicleKinematics k1 = gps.GetLatestKinematics();
    // 12 deg 58.2959 min = 12 + 58.2959/60 = 12.971598
    AEGIS_TEST_ASSERT(std::abs(k1.position.latitude - 12.971598) < 0.001);
    // 77 deg 35.6737 min = 77 + 35.6737/60 = 77.594562
    AEGIS_TEST_ASSERT(std::abs(k1.position.longitude - 77.594562) < 0.001);
    // 35 knots * 1.852 = 64.82 km/h
    AEGIS_TEST_ASSERT(std::abs(k1.speedKmH - 64.82f) < 0.5f);
    AEGIS_TEST_ASSERT(std::abs(k1.bearingDeg - 88.5f) < 0.1f);
    AEGIS_TEST_ASSERT(k1.fixQuality == 1);

    // 1.3 Void/Invalid $GPRMC should be rejected
    std::string voidRmc = "$GPRMC,123519,V,1258.2959,N,07735.6737,E,0.0,0.0,230326,,,N*69";
    AEGIS_TEST_ASSERT(!gps.ParseNmeaSentence(voidRmc));

    // 1.4 Parse valid $GPGGA
    std::string validGga = "$GPGGA,123519,1258.4047,N,07735.8272,E,1,08,0.9,920.0,M,0.0,M,,*75";
    AEGIS_TEST_ASSERT(gps.ParseNmeaSentence(validGga));
    VehicleKinematics k2 = gps.GetLatestKinematics();
    AEGIS_TEST_ASSERT(std::abs(k2.position.latitude - 12.973412) < 0.001);
    AEGIS_TEST_ASSERT(std::abs(k2.position.longitude - 77.597120) < 0.001);
    AEGIS_TEST_ASSERT(k2.fixQuality == 1);
    AEGIS_TEST_ASSERT(std::abs(k2.hdop - 0.9f) < 0.05f);
    AEGIS_TEST_ASSERT(std::abs(k2.position.altitude - 920.0) < 0.1);

    // 1.5 Direct Position Injection
    bool callbackFired = false;
    gps.RegisterKinematicsCallback([&](const VehicleKinematics& kin) {
        if (std::abs(kin.position.latitude - 12.980000) < 0.0001) {
            callbackFired = true;
        }
    });
    gps.InjectPosition(12.980000, 77.600000, 72.0f, 90.0f, 915.0);
    AEGIS_TEST_ASSERT(callbackFired);
    VehicleKinematics k3 = gps.GetLatestKinematics();
    AEGIS_TEST_ASSERT(std::abs(k3.position.latitude - 12.980000) < 0.0001);
    AEGIS_TEST_ASSERT(std::abs(k3.speedKmH - 72.0f) < 0.1f);

    // 1.6 Dead-Reckoning (EKF Fallback)
    uint64_t now = common::GetUtcMilliseconds();
    gps.UpdateDeadReckoning(now + 2000); // 2 seconds elapsed at 72 km/h heading 90 deg
    VehicleKinematics kDead = gps.GetLatestKinematics();
    AEGIS_TEST_ASSERT(kDead.fixQuality == 6); // Dead-reckoning NMEA code
    AEGIS_TEST_ASSERT(kDead.position.longitude > 77.600000); // Traveled East
    AEGIS_TEST_ASSERT(kDead.hdop > 0.8f);

    // 1.7 Waypoint Simulation Step
    std::vector<common::GeoCoordinate> simRoute = {
        {12.971598, 77.594562, 0.0},
        {12.975000, 77.599000, 0.0}
    };
    gps.SetSimulatedRoute(simRoute, 60.0f);
    AEGIS_TEST_ASSERT(gps.UpdateSimulationStep(1.0f));
    VehicleKinematics kSim = gps.GetLatestKinematics();
    AEGIS_TEST_ASSERT(kSim.position.latitude > 12.971598);

    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// 2. VitalsMonitor Unit Tests
// ==============================================================================
void TestVitalsMonitor() {
    std::cout << "[TEST] 2. VitalsMonitor Physiological Simulation & Alarms... " << std::flush;

    VitalsMonitor vitals;
    AEGIS_TEST_ASSERT(vitals.Initialize());

    // 2.1 Normal Baseline Sampling
    PatientVitals vNorm = vitals.SampleVitals();
    AEGIS_TEST_ASSERT(vNorm.heartRate >= 60 && vNorm.heartRate <= 100);
    AEGIS_TEST_ASSERT(vNorm.spO2 >= 95 && vNorm.spO2 <= 100);
    AEGIS_TEST_ASSERT(vNorm.systolicBP >= 100 && vNorm.systolicBP <= 140);
    AEGIS_TEST_ASSERT(vNorm.temperatureCelsius >= 36.0f && vNorm.temperatureCelsius <= 37.5f);
    AEGIS_TEST_ASSERT(!vitals.DetectAcuteDeterioration(vNorm));

    // 2.2 Alarm Evaluation: Hypoxia (Low SpO2)
    PatientVitals vHypoxia = vNorm;
    vHypoxia.spO2 = 82;
    AEGIS_TEST_ASSERT(vitals.DetectAcuteDeterioration(vHypoxia));
    auto alarmsHypoxia = vitals.EvaluateAlarms(vHypoxia);
    AEGIS_TEST_ASSERT(!alarmsHypoxia.empty());
    AEGIS_TEST_ASSERT(alarmsHypoxia[0].alarmType == VitalsAlarmType::LOW_SPO2);
    AEGIS_TEST_ASSERT(alarmsHypoxia[0].severity == "CRITICAL");

    // 2.3 Alarm Evaluation: Cardiac Arrest (HR == 0)
    PatientVitals vCardiac = vNorm;
    vCardiac.heartRate = 0;
    AEGIS_TEST_ASSERT(vitals.DetectAcuteDeterioration(vCardiac));
    auto alarmsCardiac = vitals.EvaluateAlarms(vCardiac);
    AEGIS_TEST_ASSERT(!alarmsCardiac.empty());
    AEGIS_TEST_ASSERT(alarmsCardiac[0].alarmType == VitalsAlarmType::CARDIAC_ARREST);

    // 2.4 Alarm Evaluation: Critical Blood Pressure
    PatientVitals vShock = vNorm;
    vShock.systolicBP = 72;
    vShock.diastolicBP = 45;
    AEGIS_TEST_ASSERT(vitals.DetectAcuteDeterioration(vShock));
    auto alarmsShock = vitals.EvaluateAlarms(vShock);
    AEGIS_TEST_ASSERT(!alarmsShock.empty());
    AEGIS_TEST_ASSERT(alarmsShock[0].alarmType == VitalsAlarmType::CRITICAL_BP);

    // 2.5 Alarm Evaluation: Severe Pyrexia / Fever
    PatientVitals vFever = vNorm;
    vFever.temperatureCelsius = 40.2f;
    AEGIS_TEST_ASSERT(vitals.DetectAcuteDeterioration(vFever));
    auto alarmsFever = vitals.EvaluateAlarms(vFever);
    AEGIS_TEST_ASSERT(!alarmsFever.empty());
    AEGIS_TEST_ASSERT(alarmsFever[0].alarmType == VitalsAlarmType::HIGH_TEMPERATURE);

    // 2.6 TriggerAcuteEvent with Callback
    VitalsAlert capturedAlert;
    bool callbackInvoked = false;
    vitals.RegisterAlarmCallback([&](const VitalsAlert& alert) {
        capturedAlert = alert;
        callbackInvoked = true;
    });

    vitals.TriggerAcuteEvent(VitalsAlarmType::CARDIAC_ARREST);
    AEGIS_TEST_ASSERT(callbackInvoked);
    AEGIS_TEST_ASSERT(capturedAlert.alarmType == VitalsAlarmType::CARDIAC_ARREST);
    AEGIS_TEST_ASSERT(vitals.GetLatestVitals().heartRate == 0);

    // 2.7 Simulation Profiles
    vitals.SetSimulationProfile(PatientCondition::HYPOXIA);
    PatientVitals vProfile = vitals.SampleVitals();
    AEGIS_TEST_ASSERT(vProfile.spO2 < 88);

    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// 3. V2vModule Unit Tests
// ==============================================================================
void TestV2vModule() {
    std::cout << "[TEST] 3. V2vModule Basic Safety Messages & Preemption Beacons... " << std::flush;

    auto netManager = std::make_shared<network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "V2V-TEST");

    V2vModule v2v;
    AEGIS_TEST_ASSERT(v2v.Initialize(8999, netManager));
    AEGIS_TEST_ASSERT(v2v.Start());

    // 3.1 BSM Serialization & Deserialization
    BasicSafetyMessage bsmOut;
    bsmOut.vehicleId = "AMB-01";
    bsmOut.position = {12.971598, 77.594562, 0.0};
    bsmOut.speedKmH = 68.5f;
    bsmOut.bearingDeg = 92.4f;
    bsmOut.state = common::AmbulanceState::EN_ROUTE_PATIENT;
    bsmOut.timestampMs = common::GetUtcMilliseconds();

    std::string bsmJson = V2vModule::SerializeBsm(bsmOut);
    auto bsmInRes = V2vModule::DeserializeBsm(bsmJson);
    AEGIS_TEST_ASSERT(bsmInRes.IsOk());
    const auto& bsmIn = bsmInRes.Value();
    AEGIS_TEST_ASSERT(bsmIn.vehicleId == "AMB-01");
    AEGIS_TEST_ASSERT(std::abs(bsmIn.speedKmH - 68.5f) < 0.1f);
    AEGIS_TEST_ASSERT(bsmIn.state == common::AmbulanceState::EN_ROUTE_PATIENT);

    // 3.2 BSM Broadcast
    VehicleKinematics kin;
    kin.position = bsmOut.position;
    kin.speedKmH = bsmOut.speedKmH;
    kin.bearingDeg = bsmOut.bearingDeg;
    kin.fixQuality = 1;
    kin.timestampMs = bsmOut.timestampMs;

    size_t prevTx = v2v.GetTransmittedBsmCount();
    v2v.BroadcastSafetyBeacon("AMB-01", kin, common::AmbulanceState::EN_ROUTE_PATIENT);
    AEGIS_TEST_ASSERT(v2v.GetTransmittedBsmCount() == prevTx + 1);

    // 3.3 Direct Preemption Beacon
    size_t prevPreempt = v2v.GetTransmittedPreemptionCount();
    v2v.SendDirectPreemptionBeacon("INT-101", "AMB-01", 12);
    AEGIS_TEST_ASSERT(v2v.GetTransmittedPreemptionCount() == prevPreempt + 1);

    // 3.4 BSM Reception Callback
    BasicSafetyMessage receivedBsm;
    bool bsmReceived = false;
    v2v.RegisterBsmCallback([&](const BasicSafetyMessage& b) {
        receivedBsm = b;
        bsmReceived = true;
    });

    v2v.HandleIncomingBsm(bsmJson);
    AEGIS_TEST_ASSERT(bsmReceived);
    AEGIS_TEST_ASSERT(receivedBsm.vehicleId == "AMB-01");

    v2v.Stop();
    netManager->Shutdown();
    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// 4. AmbulanceNode State Machine & Store-and-Forward Tests
// ==============================================================================
void TestAmbulanceNode() {
    std::cout << "[TEST] 4. AmbulanceNode State Machine, Telemetry & Buffering... " << std::flush;

    auto netManager = std::make_shared<network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "AMB-NODE-TEST");

    AmbulanceNode ambulance(netManager);
    AEGIS_TEST_ASSERT(ambulance.Initialize("AMB-01", "config/default.json"));
    AEGIS_TEST_ASSERT(ambulance.Start());

    // 4.1 State Machine Lifecycle
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::AVAILABLE);
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "AVAILABLE");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::DISPATCHED));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "DISPATCHED");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::EN_ROUTE_PATIENT));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "EN_ROUTE_PATIENT");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::PATIENT_PICKED));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "ON_SCENE");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::EN_ROUTE_HOSPITAL));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "EN_ROUTE_HOSPITAL");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::ARRIVED));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "ARRIVED");

    AEGIS_TEST_ASSERT(ambulance.TransitionState(common::AmbulanceState::AVAILABLE));
    AEGIS_TEST_ASSERT(ambulance.GetStateString() == "AVAILABLE");

    // 4.2 State string helpers
    AEGIS_TEST_ASSERT(AmbulanceNode::StringToState("DISPATCHED") == common::AmbulanceState::DISPATCHED);
    AEGIS_TEST_ASSERT(AmbulanceNode::StringToState("ON_SCENE") == common::AmbulanceState::PATIENT_PICKED);
    AEGIS_TEST_ASSERT(AmbulanceNode::StringToState("NORMAL") == common::AmbulanceState::AVAILABLE);

    // 4.3 Telemetry Publications
    ambulance.Gps().InjectPosition(12.973412, 77.597120, 65.0f, 88.0f);
    ambulance.SetPatientId("P-101");
    AEGIS_TEST_ASSERT(ambulance.PublishKinematicsOnce());
    AEGIS_TEST_ASSERT(ambulance.PublishVitalsOnce());
    AEGIS_TEST_ASSERT(ambulance.PublishHeartbeatOnce());
    AEGIS_TEST_ASSERT(ambulance.PublishStateOnce());

    // 4.4 Store-and-Forward Offline Telemetry Buffering
    // Simulate WAN disconnect by stopping network transport
    netManager->Shutdown();

    size_t initialBufferCount = ambulance.GetBufferedTelemetryCount();
    // In disconnected state, publications must be buffered
    ambulance.PublishKinematicsOnce();
    ambulance.PublishVitalsOnce();
    ambulance.PublishHeartbeatOnce();
    ambulance.PublishStateOnce();

    size_t bufferedAfterOffline = ambulance.GetBufferedTelemetryCount();
    AEGIS_TEST_ASSERT(bufferedAfterOffline >= initialBufferCount + 4);

    // Restore network connectivity and drain buffer
    netManager->Initialize("127.0.0.1", 1883, false, "AMB-RECONNECTED");
    size_t flushed = ambulance.FlushBufferedTelemetry();
    AEGIS_TEST_ASSERT(flushed > 0);
    AEGIS_TEST_ASSERT(ambulance.GetBufferedTelemetryCount() == 0);

    ambulance.Stop();
    netManager->Shutdown();
    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// 5. Coordinator Core Unmodified Integration
// ==============================================================================
void TestCoordinatorIntegration() {
    std::cout << "[TEST] 5. Unmodified Coordinator Telemetry Ingestion... " << std::flush;

    auto sharedNet = std::make_shared<network::NetworkManager>();
    sharedNet->Initialize("127.0.0.1", 1883, false, "COORD-INTEG-NET");

    core::coordinator::CentralCoordinator coordinator(sharedNet);
    AEGIS_TEST_ASSERT(coordinator.Start());

    AmbulanceNode ambulance(sharedNet);
    AEGIS_TEST_ASSERT(ambulance.Initialize("AMB-01", "config/default.json"));
    AEGIS_TEST_ASSERT(ambulance.Start());

    // Ambulance publishes kinematics
    double testLat = 12.975500;
    double testLon = 77.598800;
    ambulance.Gps().InjectPosition(testLat, testLon, 62.0f, 75.0f);
    ambulance.PublishKinematicsOnce();

    // Verify Coordinator receives and updates fleet registry without modification
    auto unitOpt = coordinator.Fleet().GetUnit("AMB-01");
    AEGIS_TEST_ASSERT(unitOpt.IsOk());
    const auto& unitRec = unitOpt.Value();
    AEGIS_TEST_ASSERT(std::abs(unitRec.location.latitude - testLat) < 0.0001);
    AEGIS_TEST_ASSERT(std::abs(unitRec.location.longitude - testLon) < 0.0001);
    AEGIS_TEST_ASSERT(std::abs(unitRec.speedKmH - 62.0f) < 0.1f);

    ambulance.Stop();
    coordinator.Stop();
    sharedNet->Shutdown();
    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// 6. AmbulanceMissionSimulator Full Workflow
// ==============================================================================
void TestMissionSimulator() {
    std::cout << "[TEST] 6. AmbulanceMissionSimulator Full Mission Lifecycle... " << std::flush;

    auto netManager = std::make_shared<network::NetworkManager>();
    netManager->Initialize("127.0.0.1", 1883, false, "SIM-MISSION-NET");

    AmbulanceNode ambulance(netManager);
    AEGIS_TEST_ASSERT(ambulance.Initialize("AMB-02", "config/default.json"));

    AmbulanceMissionSimulator sim(ambulance);

    common::GeoCoordinate station{12.971598, 77.594562, 0.0};
    common::GeoCoordinate patient{12.976000, 77.601000, 0.0};
    common::GeoCoordinate hospital{12.982000, 77.610000, 0.0};

    std::vector<common::GeoCoordinate> toPatient = {
        station,
        {12.973500, 77.597500, 0.0},
        patient
    };

    std::vector<common::GeoCoordinate> toHospital = {
        patient,
        {12.979000, 77.605000, 0.0},
        hospital
    };

    sim.ConfigureMission("INC-9001", station, patient, hospital, toPatient, toHospital);
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::IDLE);
    AEGIS_TEST_ASSERT(sim.GetMissionProgress() == 0.0f);

    // Step 1: Dispatch
    AEGIS_TEST_ASSERT(sim.StepSimulation(1.0f));
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::DISPATCHED);
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::DISPATCHED);

    // Step 2: En Route Patient
    AEGIS_TEST_ASSERT(sim.StepSimulation(1.0f));
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::EN_ROUTE_PATIENT);
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::EN_ROUTE_PATIENT);

    // Run until on-scene
    size_t safetyCounter = 0;
    while (sim.GetCurrentPhase() == MissionPhase::EN_ROUTE_PATIENT && safetyCounter++ < 200) {
        sim.StepSimulation(2.0f);
    }
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::ON_SCENE);
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::PATIENT_PICKED);

    // Dwell on-scene
    safetyCounter = 0;
    while (sim.GetCurrentPhase() == MissionPhase::ON_SCENE && safetyCounter++ < 20) {
        sim.StepSimulation(1.0f);
    }
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::EN_ROUTE_HOSPITAL);
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::EN_ROUTE_HOSPITAL);

    // Travel to hospital
    safetyCounter = 0;
    while (sim.GetCurrentPhase() == MissionPhase::EN_ROUTE_HOSPITAL && safetyCounter++ < 200) {
        sim.StepSimulation(2.0f);
    }
    AEGIS_TEST_ASSERT(sim.GetCurrentPhase() == MissionPhase::ARRIVED);
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::ARRIVED);

    // Complete mission
    safetyCounter = 0;
    while (!sim.IsCompleted() && safetyCounter++ < 20) {
        sim.StepSimulation(1.0f);
    }
    AEGIS_TEST_ASSERT(sim.IsCompleted());
    AEGIS_TEST_ASSERT(ambulance.GetState() == common::AmbulanceState::AVAILABLE);
    AEGIS_TEST_ASSERT(sim.GetMissionProgress() == 1.0f);

    netManager->Shutdown();
    std::cout << "PASSED" << std::endl;
}

// ==============================================================================
// Main Test Runner
// ==============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " Running SMART AEGIS Ambulance Node (Phase 4) Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    TestGpsTracker();
    TestVitalsMonitor();
    TestV2vModule();
    TestAmbulanceNode();
    TestCoordinatorIntegration();
    TestMissionSimulator();

    std::cout << "==========================================================" << std::endl;
    std::cout << " All 6 Ambulance Node (Phase 4) Test Suites Passed! " << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
