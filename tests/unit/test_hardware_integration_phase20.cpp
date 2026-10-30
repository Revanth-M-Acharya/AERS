/**
 * @file test_hardware_integration_phase20.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Physical Hardware Integration Comprehensive Test Suite.
 */

#include "hardware/HardwareController.h"
#include "hardware/HardwareConfiguration.h"
#include "hardware/RelayController.h"
#include "hardware/TrafficLightController.h"
#include "hardware/IntersectionHardware.h"
#include "hardware/WirelessNodeManager.h"
#include "hardware/HardwareHealthMonitor.h"
#include "hardware/HardwareDiagnostics.h"
#include "hardware/HardwareEventLogger.h"

#include <iostream>
#include <cassert>
#include <cmath>
#include <string>

using namespace aegis::hardware;

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

// Test 1: Hardware Configuration Loading & Parsing
void Test_HardwareConfiguration() {
    std::cout << "[TEST 1] Hardware Configuration & JSON Schema Parsing..." << std::endl;
    HardwareConfiguration config;
    bool ok = config.LoadFromFile("config/hardware.json");
    TEST_ASSERT(ok, "config/hardware.json loaded successfully");

    TEST_ASSERT(config.GetNodes().size() == 4, "Loaded 4 wireless node records");

    WifiNodeRecord coord;
    TEST_ASSERT(config.GetNodeRecord("COORDINATOR", coord), "Resolved COORDINATOR node record");
    TEST_ASSERT(coord.staticIp == "192.168.1.100", "Coordinator static IP is 192.168.1.100");
    TEST_ASSERT(coord.port == 9000, "Coordinator port is 9000");

    WifiNodeRecord intA;
    TEST_ASSERT(config.GetNodeRecord("INT_A", intA), "Resolved INT_A node record");
    TEST_ASSERT(intA.staticIp == "192.168.1.101", "INT_A static IP is 192.168.1.101");
    TEST_ASSERT(intA.coordinatorIp == "192.168.1.100", "INT_A points to Coordinator static IP");

    const auto& pins = config.GetSignalPins();
    TEST_ASSERT(pins.roadARedPin == 17, "Road A Red mapped to GPIO 17");
    TEST_ASSERT(pins.roadAYellowPin == 27, "Road A Yellow mapped to GPIO 27");
    TEST_ASSERT(pins.roadAGreenPin == 22, "Road A Green mapped to GPIO 22");
    TEST_ASSERT(pins.roadBRedPin == 23, "Road B Red mapped to GPIO 23");
    TEST_ASSERT(pins.roadBYellowPin == 24, "Road B Yellow mapped to GPIO 24");
    TEST_ASSERT(pins.roadBGreenPin == 25, "Road B Green mapped to GPIO 25");

    TEST_ASSERT(config.GetRelays().size() == 4, "Configured 4 relay channels");
    TEST_ASSERT(config.IsRelayActiveLow(), "Relay module configured as Active-Low");

    std::string jsonStr = config.ToJson();
    TEST_ASSERT(!jsonStr.empty(), "Serialized hardware configuration to JSON");
}

// Test 2: GPIO Initialization & Pin Reservation
void Test_GpioInitialization() {
    std::cout << "[TEST 2] GPIO Pin Reservations & Direction Configuration..." << std::endl;
    HardwareConfiguration config;
    config.LoadFromFile("config/hardware.json");

    IntersectionHardware hardware("INT_A");
    bool initOk = hardware.Initialize(config.GetSignalPins(),
                                      config.GetRelays(),
                                      config.GetIrSensors(),
                                      config.GetTimings());
    TEST_ASSERT(initOk, "IntersectionHardware initialized with BCM2711 BSP");

    auto bsp = hardware.GetBsp();
    TEST_ASSERT(bsp != nullptr && bsp->IsBoardInitialized(), "BCM2711 BSP initialized");

    TEST_ASSERT(bsp->IsGPIOReserved(17), "GPIO 17 reserved for Road A Red");
    TEST_ASSERT(bsp->IsGPIOReserved(27), "GPIO 27 reserved for Road A Yellow");
    TEST_ASSERT(bsp->IsGPIOReserved(22), "GPIO 22 reserved for Road A Green");
    TEST_ASSERT(bsp->IsGPIOReserved(23), "GPIO 23 reserved for Road B Red");
    TEST_ASSERT(bsp->IsGPIOReserved(24), "GPIO 24 reserved for Road B Yellow");
    TEST_ASSERT(bsp->IsGPIOReserved(25), "GPIO 25 reserved for Road B Green");
    TEST_ASSERT(bsp->IsGPIOReserved(12), "GPIO 12 reserved for Approach IR sensor");
    TEST_ASSERT(bsp->IsGPIOReserved(13), "GPIO 13 reserved for Departure IR sensor");
}

// Test 3: Relay Actuation & Active-Low Inversion
void Test_RelayActuation() {
    std::cout << "[TEST 3] Relay Module Actuation & Active-Low Inversion..." << std::endl;
    auto bsp = std::make_shared<aegis::bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    RelayController relay(bsp);
    std::vector<RelayChannelDef> channels = {
        {1, 17, "ROAD_A_RED", true, false},
        {2, 22, "ROAD_A_GREEN", true, false},
        {3, 23, "ROAD_B_RED", true, false},
        {4, 25, "ROAD_B_GREEN", true, false}
    };

    TEST_ASSERT(relay.Initialize(channels, true), "RelayController initialized with active-low logic");
    TEST_ASSERT(!relay.IsRelayEnergized(1), "Channel 1 initially de-energized");

    // Under Active-Low, de-energized = PinState::HIGH (3.3V)
    aegis::bsp::PinState pin1 = aegis::bsp::PinState::UNKNOWN;
    bsp->ReadGPIO(17, pin1);
    TEST_ASSERT(pin1 == aegis::bsp::PinState::HIGH, "De-energized active-low relay drives GPIO HIGH (off)");

    // Energize channel 1
    relay.SetRelay(1, true);
    TEST_ASSERT(relay.IsRelayEnergized(1), "Channel 1 marked energized");
    bsp->ReadGPIO(17, pin1);
    TEST_ASSERT(pin1 == aegis::bsp::PinState::LOW, "Energized active-low relay drives GPIO LOW (coil on)");

    // Verify switch count
    TEST_ASSERT(relay.GetSwitchCycleCount(1) == 1, "Switch count incremented to 1");

    // Toggle
    relay.ToggleRelay(1);
    TEST_ASSERT(!relay.IsRelayEnergized(1), "Channel 1 toggled to de-energized");
    TEST_ASSERT(relay.GetSwitchCycleCount(1) == 2, "Switch count incremented to 2");

    // Run self-test
    TEST_ASSERT(relay.RunSelfTest(), "Relay self-test completed with 100% readback success");
    relay.DeenergizeAll();
}

// Test 4: Traffic Signal Sequencing & Phases
void Test_TrafficSignalSequencing() {
    std::cout << "[TEST 4] Dual-Module Signal Sequencing & Phases..." << std::endl;
    auto bsp = std::make_shared<aegis::bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    TrafficLightController tl(bsp);
    DualSignalPins pins;
    tl.Initialize(pins);

    // Phase: ROAD_A_GREEN_ROAD_B_RED
    tl.SetPhase(DualPhase::ROAD_A_GREEN_ROAD_B_RED);
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::GREEN, "Road A is GREEN");
    TEST_ASSERT(tl.GetRoadBColor() == LightColor::RED, "Road B is complementary RED");

    // Phase: ROAD_A_YELLOW_ROAD_B_RED
    tl.SetPhase(DualPhase::ROAD_A_YELLOW_ROAD_B_RED);
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::YELLOW, "Road A transitioned to YELLOW clearance");
    TEST_ASSERT(tl.GetRoadBColor() == LightColor::RED, "Road B holds RED");

    // Phase: ALL_RED_CLEARANCE
    tl.SetPhase(DualPhase::ALL_RED_CLEARANCE);
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::RED, "Road A is RED");
    TEST_ASSERT(tl.GetRoadBColor() == LightColor::RED, "Road B is RED (All-Red clearance)");

    // Phase: ROAD_A_RED_ROAD_B_GREEN
    tl.SetPhase(DualPhase::ROAD_A_RED_ROAD_B_GREEN);
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::RED, "Road A holds RED");
    TEST_ASSERT(tl.GetRoadBColor() == LightColor::GREEN, "Road B transitioned to GREEN");

    // Phase: ROAD_A_RED_ROAD_B_YELLOW
    tl.SetPhase(DualPhase::ROAD_A_RED_ROAD_B_YELLOW);
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::RED, "Road A holds RED");
    TEST_ASSERT(tl.GetRoadBColor() == LightColor::YELLOW, "Road B transitioned to YELLOW clearance");
}

// Test 5: Hardware Mutual Exclusion Safety Interlock
void Test_HardwareMutualExclusion() {
    std::cout << "[TEST 5] Hardware Mutual Exclusion Safety Interlock..." << std::endl;
    auto bsp = std::make_shared<aegis::bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    TrafficLightController tl(bsp);
    DualSignalPins pins;
    tl.Initialize(pins);

    // Normal safe setting
    bool safeOk = tl.SetSignals(LightColor::GREEN, LightColor::RED);
    TEST_ASSERT(safeOk, "Allowed complementary setting (Road A Green, Road B Red)");

    // Unsafe conflicting green attempt (both roads Green simultaneously)
    bool unsafeOk = tl.SetSignals(LightColor::GREEN, LightColor::GREEN, false);
    TEST_ASSERT(!unsafeOk, "Safety interlock REJECTED simultaneous green signals!");
    TEST_ASSERT(tl.IsConflictingGreenDetected(), "Conflict flag recorded by hardware interlock");
    TEST_ASSERT(tl.HasHardwareFault(), "Hardware fault flag tripped");
    TEST_ASSERT(tl.GetRoadAColor() == LightColor::RED && tl.GetRoadBColor() == LightColor::RED,
                "Interlock immediately forced ALL-RED safe clearance state");

    tl.ClearFault();
    TEST_ASSERT(!tl.HasHardwareFault(), "Hardware fault cleared");
}

// Test 6: Pedestrian & Auxiliary Outputs
void Test_PedestrianAndAlertOutputs() {
    std::cout << "[TEST 6] Pedestrian Signals & Warning Alerts..." << std::endl;
    auto bsp = std::make_shared<aegis::bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    TrafficLightController tl(bsp);
    DualSignalPins pins;
    tl.Initialize(pins);

    tl.SetPedestrianSignal(true);
    aegis::bsp::PinState walkState = aegis::bsp::PinState::UNKNOWN;
    bsp->ReadGPIO(pins.pedWalkPin, walkState);
    TEST_ASSERT(walkState == aegis::bsp::PinState::HIGH, "Pedestrian Walk pin driven HIGH");

    tl.SetPedestrianSignal(false);
    bsp->ReadGPIO(pins.pedWalkPin, walkState);
    TEST_ASSERT(walkState == aegis::bsp::PinState::LOW, "Pedestrian Walk pin driven LOW");

    tl.SetPreemptionStrobe(true);
    aegis::bsp::PinState strobeState = aegis::bsp::PinState::UNKNOWN;
    bsp->ReadGPIO(pins.strobePin, strobeState);
    TEST_ASSERT(strobeState == aegis::bsp::PinState::HIGH, "Preemption strobe driven HIGH");

    tl.SetBuzzer(true);
    aegis::bsp::PinState buzzerState = aegis::bsp::PinState::UNKNOWN;
    bsp->ReadGPIO(pins.buzzerPin, buzzerState);
    TEST_ASSERT(buzzerState == aegis::bsp::PinState::HIGH, "Acoustic warning buzzer driven HIGH");
}

// Test 7: IR Sensor Detection & Debounce
void Test_IrSensorDetection() {
    std::cout << "[TEST 7] Optical IR Sensors & Vehicle Detection..." << std::endl;
    HardwareConfiguration config;
    config.LoadFromFile("config/hardware.json");

    IntersectionHardware hardware("INT_A");
    hardware.Initialize(config.GetSignalPins(),
                        config.GetRelays(),
                        config.GetIrSensors(),
                        config.GetTimings());

    bool detected = false;
    hardware.ReadApproachIrSensor(detected);
    TEST_ASSERT(!detected, "Approach sensor initially clear");

    hardware.InjectApproachDetection();
    hardware.ReadApproachIrSensor(detected);
    TEST_ASSERT(detected, "Approach sensor detected incoming vehicle");

    auto appStatus = hardware.GetApproachSensorStatus();
    TEST_ASSERT(appStatus.detectionCount == 1, "Approach detection counter incremented");
    TEST_ASSERT(appStatus.isHealthy, "Sensor health is HEALTHY");

    // Advance 2 seconds to verify auto-reset of pulse
    hardware.Update(2.0);
    hardware.ReadApproachIrSensor(detected);
    TEST_ASSERT(!detected, "Approach sensor auto-cleared after pulse interval");
}

// Test 8: Wireless Mesh Communication & Static IPs
void Test_WirelessMeshCommunication() {
    std::cout << "[TEST 8] Wi-Fi TCP/IP Mesh Communication & Static IPs..." << std::endl;
    HardwareConfiguration config;
    config.LoadFromFile("config/hardware.json");

    WifiNodeRecord rec;
    config.GetNodeRecord("INT_B", rec);

    WirelessNodeManager wireless("INT_B");
    wireless.ConfigureWirelessNode(rec);

    TEST_ASSERT(wireless.Connect(), "Wi-Fi link established using static IP 192.168.1.102");
    TEST_ASSERT(wireless.IsConnected(), "WirelessNodeManager reports connected");

    wireless.SendHeartbeat();
    wireless.SendHeartbeat();
    auto stats = wireless.GetLinkStats();
    TEST_ASSERT(stats.heartbeatCount == 2, "2 heartbeats transmitted over Wi-Fi");
    TEST_ASSERT(stats.rssiDbm > -80, "Wi-Fi signal strength RSSI within acceptable range");
    TEST_ASSERT(stats.rttLatencyMs < 10.0, "Wi-Fi RTT latency < 10 ms");

    wireless.Disconnect();
    TEST_ASSERT(!wireless.IsConnected(), "Disconnected cleanly");
}

// Test 9: Coordinator Live Command Execution
void Test_CoordinatorLiveCommands() {
    std::cout << "[TEST 9] Coordinator Live Traffic Commands..." << std::endl;
    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    // Verify initial state
    TEST_ASSERT(controller.IsRunning(), "HardwareController running");
    TEST_ASSERT(!controller.IsPreempted(), "Initially not preempted");

    // Remote PREEMPTION command from Coordinator
    controller.GetWirelessManager()->SendCommand("INT_A", "PREEMPTION", "CORRIDOR_01");
    TEST_ASSERT(controller.IsPreempted(), "Emergency preemption engaged via wireless command");

    // Remote FORCE_GREEN command
    controller.GetWirelessManager()->SendCommand("INT_A", "FORCE_GREEN", "ROAD_B");
    TEST_ASSERT(controller.GetCurrentPhase() == DualPhase::ROAD_A_RED_ROAD_B_GREEN,
                "Force Green executed for Road B");

    // Remote RESTORE command
    controller.GetWirelessManager()->SendCommand("INT_A", "RESTORE");
    TEST_ASSERT(!controller.IsPreempted(), "Normal cycle restored via wireless command");
}

// Test 10: Green Corridor Preemption & Departure Clearance
void Test_GreenCorridorPreemption() {
    std::cout << "[TEST 10] Green Corridor Preemption & Clearance Recovery..." << std::endl;
    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    // Trigger preemption on Road A
    controller.TriggerEmergencyPreemption("CORRIDOR_GREEN_WAVE_01", "ROAD_A");
    TEST_ASSERT(controller.IsPreempted(), "Preemption active for CORRIDOR_GREEN_WAVE_01");
    TEST_ASSERT(controller.GetCurrentPhase() == DualPhase::PREEMPTION_ROAD_A, "Phase is PREEMPTION_ROAD_A");

    // Simulate ambulance traversing and clearing intersection via departure IR sensor
    controller.SimulateVehicleDeparture();
    TEST_ASSERT(!controller.IsPreempted(), "Departure IR pulse automatically cleared emergency preemption!");
    TEST_ASSERT(controller.GetCurrentPhase() == DualPhase::ALL_RED_CLEARANCE,
                "Transitioned into All-Red clearance before returning to normal cycle");
}

// Test 11: Fail-Safe Degradation on Wi-Fi Interruption & Auto-Recovery
void Test_FailSafeBehavior() {
    std::cout << "[TEST 11] Wi-Fi Link Severance, Fail-Safe Degradation & Recovery..." << std::endl;
    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    TEST_ASSERT(!controller.IsInFailSafe(), "Hardware initially in normal operational state");

    // Sever Wi-Fi connection
    controller.SimulateWifiDrop(true);
    controller.Update(0.1);
    TEST_ASSERT(controller.IsInFailSafe(), "Node degraded to FAILSAFE_FLASHING_YELLOW upon Wi-Fi interruption");

    // Restore Wi-Fi link
    controller.SimulateWifiDrop(false);
    controller.Update(0.1);
    TEST_ASSERT(!controller.IsInFailSafe(), "Node auto-reconnected and safely recovered from fail-safe!");
}

// Test 12: Hardware Health Monitoring
void Test_HardwareHealthMonitoring() {
    std::cout << "[TEST 12] Hardware Health Monitoring & Telemetry..." << std::endl;
    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    auto health = controller.GetHealth();
    TEST_ASSERT(health.nodeId == "INT_A", "Health record reflects INT_A");
    TEST_ASSERT(health.grade == HardwareHealthGrade::HEALTHY, "Health grade is HEALTHY");
    TEST_ASSERT(health.socTemperatureC > 30.0 && health.socTemperatureC < 85.0, "BCM2711 temperature nominal");
    TEST_ASSERT(health.coreVoltageV > 1.0 && health.coreVoltageV < 1.4, "Core voltage nominal");
    TEST_ASSERT(health.gpioHealthy && health.relaysHealthy && health.irSensorsHealthy,
                "All hardware subsystems healthy");

    std::string report = controller.GetHealthReport();
    TEST_ASSERT(!report.empty(), "Generated structured ASCII hardware health report");
}

// Test 13: Hardware Diagnostics Self-Test Suite
void Test_HardwareDiagnostics() {
    std::cout << "[TEST 13] Hardware Diagnostic Self-Test Suite..." << std::endl;
    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    bool diagPassed = controller.RunDiagnostics();
    TEST_ASSERT(diagPassed, "All 6 hardware diagnostic tests passed (100% operational)");

    std::string report = controller.GetDiagnosticReport();
    TEST_ASSERT(!report.empty(), "Generated structured hardware diagnostic report");
}

// Test 14: Hardware Event Logger
void Test_HardwareEventLogger() {
    std::cout << "[TEST 14] Forensic Hardware Event Logger..." << std::endl;
    HardwareEventLogger logger(100);

    logger.Info("RELAY", "Relay channel 1 energized");
    logger.Warn("SIGNAL", "Yellow clearance phase active");
    logger.Critical("PREEMPTION", "Emergency preemption engaged");

    TEST_ASSERT(logger.GetLogCount() == 3, "Recorded 3 log entries");
    auto recent = logger.GetRecentLogs(2);
    TEST_ASSERT(recent.size() == 2, "Retrieved 2 recent log entries");
    TEST_ASSERT(recent.back().severity == "CRITICAL", "Latest log is CRITICAL");

    std::string csv = logger.ExportToCsv();
    TEST_ASSERT(!csv.empty(), "Exported logs to CSV format");

    std::string json = logger.ExportToJson();
    TEST_ASSERT(!json.empty(), "Exported logs to JSON format");
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS (AERS) Phase 20: Hardware Integration Tests" << std::endl;
    std::cout << " Platform: Raspberry Pi 4 Model B / QNX SDP 8.0 / C++17" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test_HardwareConfiguration();
    Test_GpioInitialization();
    Test_RelayActuation();
    Test_TrafficSignalSequencing();
    Test_HardwareMutualExclusion();
    Test_PedestrianAndAlertOutputs();
    Test_IrSensorDetection();
    Test_WirelessMeshCommunication();
    Test_CoordinatorLiveCommands();
    Test_GreenCorridorPreemption();
    Test_FailSafeBehavior();
    Test_HardwareHealthMonitoring();
    Test_HardwareDiagnostics();
    Test_HardwareEventLogger();

    std::cout << "==========================================================" << std::endl;
    std::cout << " Total Passed: " << g_passCount << " / " << (g_passCount + g_failCount) << std::endl;
    std::cout << " Total Failed: " << g_failCount << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_failCount == 0) ? 0 : 1;
}

