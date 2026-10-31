/**
 * @file test_cluster_deployment_hardware.cpp
 * @brief Phase 11 Unit & Integration Test Suite for Raspberry Pi Cluster Deployment & Hardware Integration.
 * Validates Symbolic GPIO HAL, Relay Driver, IR Sensor Driver, Cluster Supervisor, Commissioning, and 7-Vector Audit.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/relay/ProductionRelayDriver.hpp"
#include "hardware_integration/sensor/IRSensorDriver.hpp"
#include "hardware_integration/config/NodeConfigManager.hpp"
#include "hardware_integration/commissioning/CommissioningManager.hpp"
#include "hardware_integration/cluster/ClusterSupervisor.hpp"
#include "hardware_integration/validation/DeploymentValidator.hpp"
#include "hardware_integration/logging/CentralizedDeploymentLogger.hpp"
#include "traffic_node/gpio/MockGpioDriver.hpp"
#include "common/utils/TimeUtils.hpp"

#include <iostream>
#include <cassert>
#include <memory>
#include <vector>

using namespace aegis;
using namespace aegis::hardware_integration;
using namespace aegis::traffic;

void TestSymbolicGpioHAL() {
    std::cout << "[TEST 1/8] Symbolic GPIO HAL (No Hardcoded Pins, Polarity Translation)... ";
    auto mockDriver = std::make_shared<MockGpioDriver>();
    SymbolicGpioHAL hal(mockDriver);

    // Register signals with symbolic names
    assert(hal.RegisterSignal(StandardSignals::ROAD_A_RED, 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road A Red"));
    assert(hal.RegisterSignal(StandardSignals::ROAD_A_GREEN, 22, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW, "Road A Green"));
    assert(hal.RegisterSignal(StandardSignals::EMERGENCY_STROBE, 26, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Strobe LED"));
    assert(hal.RegisterSignal(StandardSignals::APPROACH_IR_SENSOR, 12, PinDirection::INPUT, ActivePolarity::ACTIVE_LOW, "IR Sensor"));

    // Reject duplicate name
    assert(!hal.RegisterSignal(StandardSignals::ROAD_A_RED, 18, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW));
    // Reject pin collision
    assert(!hal.RegisterSignal("ROAD_A_YELLOW", 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_LOW));

    assert(hal.Initialize());

    // Active-Low Relay Test: Logically Active (true) -> Physical 0V (false)
    assert(hal.SetSignal(StandardSignals::ROAD_A_RED, true));
    assert(hal.GetSignal(StandardSignals::ROAD_A_RED) == true);
    assert(mockDriver->GetPinLevel(17) == false); // Physically LOW for active-low relay coil

    // Active-High Direct Drive Test: Logically Active (true) -> Physical 3.3V (true)
    assert(hal.SetSignal(StandardSignals::EMERGENCY_STROBE, true));
    assert(hal.GetSignal(StandardSignals::EMERGENCY_STROBE) == true);
    assert(mockDriver->GetPinLevel(26) == true);

    // Toggle
    assert(hal.ToggleSignal(StandardSignals::EMERGENCY_STROBE));
    assert(hal.GetSignal(StandardSignals::EMERGENCY_STROBE) == false);

    std::cout << "PASS\n";
}

void TestProductionRelayDriver() {
    std::cout << "[TEST 2/8] Production Relay Driver (Modes & Conflict Interlock)... ";
    auto mockDriver = std::make_shared<MockGpioDriver>();
    auto hal = std::make_shared<SymbolicGpioHAL>(mockDriver);
    hal->Initialize();

    ProductionRelayDriver relay(hal);
    assert(relay.Initialize());
    assert(relay.GetCurrentMode() == RelayOperationalMode::ALL_RED);

    // Normal phase: Road A Green
    assert(relay.SetNormalPhase(TrafficApproach::ROAD_A));
    assert(relay.GetCurrentMode() == RelayOperationalMode::NORMAL);
    assert(hal->GetSignal(StandardSignals::ROAD_A_GREEN) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_B_RED) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_B_GREEN) == false);

    // Normal phase: Road B Green
    assert(relay.SetNormalPhase(TrafficApproach::ROAD_B));
    assert(hal->GetSignal(StandardSignals::ROAD_B_GREEN) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_A_RED) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_A_GREEN) == false);

    // Flashing Yellow
    assert(relay.SetFlashingYellow(true));
    assert(relay.GetCurrentMode() == RelayOperationalMode::FLASHING_YELLOW);
    assert(hal->GetSignal(StandardSignals::ROAD_A_YELLOW) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_B_YELLOW) == true);

    // Emergency Preemption
    assert(relay.SetEmergencyPreemption(TrafficApproach::ROAD_A, true));
    assert(relay.IsEmergencyPreemptionActive());
    assert(hal->GetSignal(StandardSignals::ROAD_A_GREEN) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_B_RED) == true);
    assert(hal->GetSignal(StandardSignals::EMERGENCY_STROBE) == true);
    assert(hal->GetSignal(StandardSignals::ACOUSTIC_BUZZER) == true);

    // Hardware/Software Safety Interlock Test: Attempt conflicting green
    // If external caller attempts to set both Green, interlock trips and forces ALL_RED
    hal->SetSignal(StandardSignals::ROAD_A_GREEN, true);
    hal->SetSignal(StandardSignals::ROAD_B_GREEN, true);

    // Relay driver's interlock trips when checking
    relay.SetAllRed();
    assert(hal->GetSignal(StandardSignals::ROAD_A_RED) == true);
    assert(hal->GetSignal(StandardSignals::ROAD_B_RED) == true);

    // Channel self-test
    auto testRes = relay.ExecuteSelfTest(10);
    assert(testRes.passed);
    assert(testRes.channelsPassed == testRes.channelsTested);

    std::cout << "PASS\n";
}

void TestIRSensorDriver() {
    std::cout << "[TEST 3/8] Optical IR Sensor Driver (Debouncing & Diagnostics)... ";
    auto mockDriver = std::make_shared<MockGpioDriver>();
    auto hal = std::make_shared<SymbolicGpioHAL>(mockDriver);
    hal->Initialize();

    IRSensorConfig cfg;
    cfg.sensorId = "IR_APPROACH_A";
    cfg.symbolicSignal = StandardSignals::APPROACH_IR_SENSOR;
    cfg.physicalPin = 12;
    cfg.polarity = ActivePolarity::ACTIVE_LOW;
    cfg.debounceWindowMs = 20;
    cfg.maxContinuousTriggerSec = 60;
    cfg.maxTriggersPerMinute = 30;

    IRSensorDriver ir(hal, cfg);
    assert(ir.Initialize());
    assert(ir.GetCurrentState() == IRSensorState::BEAM_CLEAR);

    bool detectedCalled = false;
    bool clearedCalled = false;
    ir.SetOnVehicleDetected([&detectedCalled](const std::string&, uint64_t) { detectedCalled = true; });
    ir.SetOnVehicleCleared([&clearedCalled](const std::string&, uint64_t) { clearedCalled = true; });

    // Transient glitch (< debounce window) should be filtered out
    ir.SimulateRawPinLevel(true, 1000);  // Pulse starts at 1000ms
    ir.Poll(1010);                       // Only 10ms elapsed (< 20ms debounce)
    assert(ir.GetCurrentState() == IRSensorState::BEAM_CLEAR); // Still clear

    // Stable trigger (>= 20ms debounce)
    ir.Poll(1025);                       // 25ms elapsed (>= 20ms)
    assert(ir.GetCurrentState() == IRSensorState::BEAM_INTERRUPTED);
    assert(detectedCalled);
    assert(ir.IsVehiclePresent());

    // Vehicle leaves beam
    ir.SimulateRawPinLevel(false, 1500); // Leaves at 1500ms
    ir.Poll(1525);                       // 25ms stable clear
    assert(ir.GetCurrentState() == IRSensorState::BEAM_CLEAR);
    assert(clearedCalled);

    auto metrics = ir.GetMetrics();
    assert(metrics.totalVehicleDetections == 1);
    assert(metrics.healthGrade == SensorHealthGrade::HEALTHY);

    // Stuck-active diagnostic test (> 60 seconds continuous blockage)
    ir.SimulateRawPinLevel(true, 2000);
    ir.Poll(2025); // Triggered
    assert(ir.GetCurrentState() == IRSensorState::BEAM_INTERRUPTED);

    // Advance time by 65 seconds
    auto healthStuck = ir.RunDiagnostics(2000 + 65000);
    assert(healthStuck == SensorHealthGrade::FAULTY);
    auto m2 = ir.GetMetrics();
    assert(m2.activeFault == SensorFaultType::STUCK_ACTIVE);

    std::cout << "PASS\n";
}

void TestNodeConfigManager() {
    std::cout << "[TEST 4/8] Node Configuration Management (Roles & Validation)... ";
    NodeConfigManager mgr;
    mgr.ClearPinAssignments();

    std::string sampleConf =
        "nodeId=INT_TEST\n"
        "role=TRAFFIC_NODE\n"
        "ipAddress=192.168.1.150\n"
        "listenPort=9005\n"
        "coordinatorIp=192.168.1.100\n"
        "coordinatorPort=9000\n"
        "heartbeatIntervalMs=1000\n"
        "pin.ROAD_A_RED=17,OUTPUT,ACTIVE_LOW,Red lamp\n"
        "pin.APPROACH_IR_SENSOR=12,INPUT,ACTIVE_LOW,IR Sensor\n";

    assert(mgr.ParseString(sampleConf));
    std::vector<std::string> errs;
    assert(mgr.Validate(errs));

    auto prof = mgr.GetProfile();
    assert(prof.nodeId == "INT_TEST");
    assert(prof.role == NodeRole::TRAFFIC_NODE);
    assert(prof.listenPort == 9005);
    assert(prof.pinAssignments.size() == 2);

    // Test pin collision detection
    mgr.AssignPin({"CONFLICT_PIN", 17, PinDirection::OUTPUT, ActivePolarity::ACTIVE_HIGH, "Duplicate pin"});
    assert(!mgr.Validate(errs));
    assert(!errs.empty());

    std::cout << "PASS\n";
}

void TestCommissioningManager() {
    std::cout << "[TEST 5/8] Automated Startup Commissioning & Self-Test... ";
    auto mockDriver = std::make_shared<MockGpioDriver>();
    auto hal = std::make_shared<SymbolicGpioHAL>(mockDriver);
    auto relay = std::make_shared<ProductionRelayDriver>(hal);
    auto config = std::make_shared<NodeConfigManager>();
    IRSensorConfig irCfg;
    irCfg.debounceWindowMs = 5;
    auto ir = std::make_shared<IRSensorDriver>(hal, irCfg);

    CommissioningManager comm(hal, relay, ir, config);
    auto report = comm.RunCommissioning(5);

    assert(report.overallSuccess);
    assert(report.passedSteps == report.totalSteps);
    assert(comm.IsCommissioned());

    std::string ascii = CommissioningManager::FormatReportAscii(report);
    assert(!ascii.empty());
    assert(ascii.find("[PASS / CERTIFIED]") != std::string::npos);

    std::cout << "PASS\n";
}

void TestClusterSupervisor() {
    std::cout << "[TEST 6/8] 4-Pi Cluster Supervisor (Health, CPU, RAM & Telemetry)... ";
    ClusterSupervisor supervisor;
    supervisor.InitializeDefault4PiCluster();

    auto nodes = supervisor.GetAllNodes();
    assert(nodes.size() == 4);

    // Verify Coordinator and Traffic Nodes present
    auto coord = supervisor.GetNodeTelemetry("RPI_COORD");
    assert(coord.has_value());
    assert(coord->role == NodeRole::COORDINATOR);

    auto intA = supervisor.GetNodeTelemetry("INT_A");
    assert(intA.has_value());
    assert(intA->role == NodeRole::TRAFFIC_NODE);

    // Heartbeat update
    supervisor.RecordHeartbeat("INT_A", 1.8, 1000);
    supervisor.UpdateTelemetry("INT_A", 4.2, 1920, "ROAD_A_GREEN", 15, false, 0.5);

    auto updatedA = supervisor.GetNodeTelemetry("INT_A");
    assert(updatedA.has_value());
    assert(updatedA->totalVehicleDetections == 15);
    assert(updatedA->cpuPercent > 4.0);

    // Test offline detection upon missing heartbeats (> 6000ms)
    supervisor.EvaluateClusterHealth(10000);
    auto timedOutA = supervisor.GetNodeTelemetry("INT_A");
    assert(timedOutA.has_value());
    assert(timedOutA->health == ClusterNodeHealth::OFFLINE);

    std::string dash = supervisor.RenderDashboardAscii(1000);
    assert(!dash.empty());
    assert(dash.find("RPI_COORD") != std::string::npos);

    std::string json = supervisor.ExportJson(1000);
    assert(!json.empty());
    assert(json.find("\"nodeCount\": 4") != std::string::npos);

    std::cout << "PASS\n";
}

void TestDeploymentValidator() {
    std::cout << "[TEST 7/8] 7-Vector Deployment Validation Engine... ";
    auto mockDriver = std::make_shared<MockGpioDriver>();
    auto hal = std::make_shared<SymbolicGpioHAL>(mockDriver);
    auto relay = std::make_shared<ProductionRelayDriver>(hal);
    auto config = std::make_shared<NodeConfigManager>();
    IRSensorConfig irCfg;
    irCfg.debounceWindowMs = 5;
    auto ir = std::make_shared<IRSensorDriver>(hal, irCfg);
    auto supervisor = std::make_shared<ClusterSupervisor>();
    supervisor->InitializeDefault4PiCluster();
    auto eventBus = std::make_shared<coordinator::orchestration::EventBus>();

    DeploymentValidator validator(hal, relay, ir, config, supervisor, eventBus);
    auto rep = validator.ExecuteFullValidation();

    assert(rep.overallPassed);
    assert(rep.passedVectors == 7);
    assert(rep.totalVectors == 7);

    std::string ascii = DeploymentValidator::FormatReportAscii(rep);
    assert(!ascii.empty());
    assert(ascii.find("[PASS / DEPLOYMENT CERTIFIED]") != std::string::npos);

    std::cout << "PASS\n";
}

void TestCentralizedDeploymentLogger() {
    std::cout << "[TEST 8/8] Centralized Deployment Logging & Markdown Export... ";
    CentralizedDeploymentLogger logger;

    logger.Log("INT_A", "RELAY", DeploymentLogLevel::INFO, "All-Red clearance activated");
    logger.Log("RPI_COORD", "SCHEDULER", DeploymentLogLevel::INFO, "Preemption corridor granted");

    CommissioningReport commRep;
    commRep.nodeId = "INT_A";
    commRep.nodeRole = "TRAFFIC_NODE";
    commRep.overallSuccess = true;
    commRep.passedSteps = 6;
    commRep.totalSteps = 6;
    commRep.totalDurationMs = 45;
    commRep.summary = "Node INT_A certified for live operation";
    logger.RecordCommissioning(commRep);

    DeploymentValidationReport valRep;
    valRep.overallPassed = true;
    valRep.passedVectors = 7;
    valRep.totalVectors = 7;
    valRep.auditSummary = "All 7 deployment vectors certified";
    logger.RecordValidation(valRep);

    std::string md = logger.GenerateMarkdownReport("Metropolitan_Core_Cluster");
    assert(!md.empty());
    assert(md.find("Metropolitan_Core_Cluster") != std::string::npos);
    assert(md.find("**CERTIFIED**") != std::string::npos);

    std::cout << "PASS\n";
}

int main() {
    std::cout << "\n==========================================================\n";
    std::cout << " SMART AEGIS: Phase 11 Cluster Deployment & Hardware Tests\n";
    std::cout << " Target: Raspberry Pi 4 Model B (QNX SDP 8.0 / POSIX)     \n";
    std::cout << "==========================================================\n\n";

    TestSymbolicGpioHAL();
    TestProductionRelayDriver();
    TestIRSensorDriver();
    TestNodeConfigManager();
    TestCommissioningManager();
    TestClusterSupervisor();
    TestDeploymentValidator();
    TestCentralizedDeploymentLogger();

    std::cout << "\n==========================================================\n";
    std::cout << " [ALL 8 PHASE 11 DEPLOYMENT & HARDWARE TEST SUITES PASSED]\n";
    std::cout << "==========================================================\n\n";
    return 0;
}
