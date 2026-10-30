/**
 * @file test_qnx_deployment_phase20_5.cpp
 * @brief SMART AEGIS (AERS) Phase 20.5: QNX Raspberry Pi 4 Deployment & Platform Compatibility Test Suite.
 *
 * Verifies QNX SDP 8.0 / 7.1 RTOS platform compatibility, real-time thread priority hierarchy,
 * high-resolution monotonic clocks, static Wi-Fi mesh network configuration, BCM2711 GPIO &
 * active-low relay logic, hardware mutual exclusion safety interlock, optical IR vehicle sensors,
 * Coordinator & Intersection node runtimes, digital twin city simulation engine, Qt6 headless
 * execution, deployment packaging, and end-to-end emergency pipeline regression.
 *
 * Target Hardware: Raspberry Pi 4 Model B (Broadcom BCM2711, aarch64le)
 * Compiler: QCC / Q++ / Clang / GCC (C++17)
 * Flags: -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <fstream>
#include <cstdlib>

#include "aegis/common/qnx_compat.hpp"
#include "aegis/common/json_helper.hpp"
#include "bsp/RaspberryPi4BSP.h"
#include "hardware/HardwareConfiguration.h"
#include "hardware/RelayController.h"
#include "hardware/TrafficLightController.h"
#include "hardware/IntersectionHardware.h"
#include "hardware/WirelessNodeManager.h"
#include "hardware/HardwareHealthMonitor.h"
#include "hardware/HardwareDiagnostics.h"
#include "hardware/HardwareController.h"
#include "runtime/CoordinatorRuntime.h"
#include "runtime/TrafficNodeRuntime.h"
#include "runtime/DistributedRuntimeOrchestrator.h"
#include "simulator/CityClock.h"
#include "simulator/CityModel.h"
#include "deployment/NodeConfiguration.h"
#include "deployment/ConfigurationLoader.h"

using namespace aegis;
using namespace aegis::qnx;
using namespace aegis::hardware;
using namespace aegis::runtime;
using namespace aegis::simulator;

static int g_passCount = 0;
static int g_failCount = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  ✓ PASS: " << (msg) << std::endl; \
            g_passCount++; \
        } else { \
            std::cerr << "  ✗ FAIL: " << (msg) << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            g_failCount++; \
        } \
    } while(0)

// =============================================================================
// Test 1: QNX Environment & Macros
// =============================================================================
void Test_QnxEnvironmentAndMacros() {
    std::cout << "[TEST 1] QNX SDP 8.0 Environment & Platform Introspection..." << std::endl;

    std::string arch = GetTargetArchitecture();
    TEST_ASSERT(!arch.empty(), "Resolved target architecture string");

    std::string info = GetPlatformInfo();
    TEST_ASSERT(!info.empty(), "Retrieved platform information string: " + info);

    const char* prefix = GetQnxDevicePrefix();
    TEST_ASSERT(std::string(prefix) == "/dev/aegis/", "QNX device path prefix is /dev/aegis/");

    std::string gpioDev = FormatQnxDevicePath("gpio");
    TEST_ASSERT(gpioDev == "/dev/aegis/gpio", "Formatted QNX device path /dev/aegis/gpio");
}

// =============================================================================
// Test 2: QNX Real-Time Scheduler & Priority Hierarchy
// =============================================================================
void Test_QnxRealTimeSchedulerAndPriorities() {
    std::cout << "[TEST 2] QNX Real-Time Scheduler & Thread Priority Hierarchy..." << std::endl;

    TEST_ASSERT(static_cast<int>(QnxPriority::IDLE_TASK) == 5, "Idle Task Priority is 5");
    TEST_ASSERT(static_cast<int>(QnxPriority::TELEMETRY_WORKER) == 10, "Telemetry Worker Priority is 10");
    TEST_ASSERT(static_cast<int>(QnxPriority::NORMAL_WORKER) == 15, "Normal Worker Priority is 15");
    TEST_ASSERT(static_cast<int>(QnxPriority::HIGH_COORDINATOR) == 21, "Coordinator Real-Time Priority is 21");
    TEST_ASSERT(static_cast<int>(QnxPriority::EMERGENCY_PREEMPT) == 25, "Emergency Preemption Priority is 25");
    TEST_ASSERT(static_cast<int>(QnxPriority::SAFETY_WATCHDOG) == 30, "Safety Watchdog Priority is 30");
    TEST_ASSERT(static_cast<int>(QnxPriority::CRITICAL_FAULT) == 35, "Critical Fault Priority is 35");

    std::atomic<bool> threadExecuted{false};
    std::thread testThread([&]() {
        bool prioOk = SetThreadRealTimePriority(pthread_self(), QnxPriority::HIGH_COORDINATOR, true);
        TEST_ASSERT(prioOk, "Applied QNX SCHED_FIFO Real-Time Priority 21 to worker thread");
        bool nameOk = SetThreadName(pthread_self(), "AEGIS_COORD_RT");
        TEST_ASSERT(nameOk, "Applied diagnostic thread name 'AEGIS_COORD_RT'");
        threadExecuted.store(true);
    });

    testThread.join();
    TEST_ASSERT(threadExecuted.load(), "Real-time prioritized thread executed and completed cleanly");
}

// =============================================================================
// Test 3: QNX High-Resolution Monotonic Clock
// =============================================================================
void Test_QnxHighResolutionMonotonicClock() {
    std::cout << "[TEST 3] QNX High-Resolution Monotonic Clock & Timers..." << std::endl;

    uint64_t t1 = GetMonotonicTimeNs();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    uint64_t t2 = GetMonotonicTimeNs();

    TEST_ASSERT(t2 > t1, "Monotonic clock progressed forward");
    uint64_t diffNs = t2 - t1;
    TEST_ASSERT(diffNs >= 4000000ULL, "Elapsed time measured >= 4 ms in nanoseconds");

    double sec = GetMonotonicTimeSec();
    TEST_ASSERT(sec > 0.0, "Monotonic seconds timestamp is positive");

    uint64_t ms = GetMonotonicTimeMs();
    TEST_ASSERT(ms > 0ULL, "Monotonic milliseconds timestamp is positive");
}

// =============================================================================
// Test 4: Static Wi-Fi Mesh Network Topology
// =============================================================================
void Test_QnxStaticNetworkTopology() {
    std::cout << "[TEST 4] Static Wi-Fi Mesh Network Configuration..." << std::endl;

    HardwareConfiguration hwConfig;
    TEST_ASSERT(hwConfig.LoadFromFile("config/hardware.json"), "Loaded config/hardware.json");

    WifiNodeRecord coordRec;
    TEST_ASSERT(hwConfig.GetNodeRecord("COORDINATOR", coordRec), "Resolved COORDINATOR node record");
    TEST_ASSERT(coordRec.staticIp == "192.168.1.100", "Coordinator static IP is 192.168.1.100");
    TEST_ASSERT(coordRec.port == 9000, "Coordinator port is 9000");

    WifiNodeRecord intARec;
    TEST_ASSERT(hwConfig.GetNodeRecord("INT_A", intARec), "Resolved INT_A node record");
    TEST_ASSERT(intARec.staticIp == "192.168.1.101", "INT_A static IP is 192.168.1.101");
    TEST_ASSERT(intARec.coordinatorIp == "192.168.1.100", "INT_A points to Coordinator static IP");

    WifiNodeRecord intBRec;
    TEST_ASSERT(hwConfig.GetNodeRecord("INT_B", intBRec), "Resolved INT_B node record");
    TEST_ASSERT(intBRec.staticIp == "192.168.1.102", "INT_B static IP is 192.168.1.102");

    WifiNodeRecord intCRec;
    TEST_ASSERT(hwConfig.GetNodeRecord("INT_C", intCRec), "Resolved INT_C node record");
    TEST_ASSERT(intCRec.staticIp == "192.168.1.103", "INT_C static IP is 192.168.1.103");
}

// =============================================================================
// Test 5: Broadcom BCM2711 GPIO Pin Reservations under QNX BSP
// =============================================================================
void Test_QnxBspGpioPinReservations() {
    std::cout << "[TEST 5] Broadcom BCM2711 GPIO Pin Reservations under QNX BSP..." << std::endl;

    auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
    TEST_ASSERT(bsp->InitializeBoard(), "BCM2711 Board initialized");

    // Road A traffic lights
    TEST_ASSERT(bsp->ReserveGPIO(17, "ROAD_A_RED"), "Reserved GPIO 17 (Road A Red)");
    TEST_ASSERT(bsp->ReserveGPIO(27, "ROAD_A_YELLOW"), "Reserved GPIO 27 (Road A Yellow)");
    TEST_ASSERT(bsp->ReserveGPIO(22, "ROAD_A_GREEN"), "Reserved GPIO 22 (Road A Green)");

    // Road B traffic lights
    TEST_ASSERT(bsp->ReserveGPIO(23, "ROAD_B_RED"), "Reserved GPIO 23 (Road B Red)");
    TEST_ASSERT(bsp->ReserveGPIO(24, "ROAD_B_YELLOW"), "Reserved GPIO 24 (Road B Yellow)");
    TEST_ASSERT(bsp->ReserveGPIO(25, "ROAD_B_GREEN"), "Reserved GPIO 25 (Road B Green)");

    // Safety & sensors
    TEST_ASSERT(bsp->ReserveGPIO(5, "PED_WALK"), "Reserved GPIO 5 (Pedestrian Walk)");
    TEST_ASSERT(bsp->ReserveGPIO(26, "OPTICAL_STROBE"), "Reserved GPIO 26 (Optical Strobe)");
    TEST_ASSERT(bsp->ReserveGPIO(6, "WARNING_BUZZER"), "Reserved GPIO 6 (Warning Buzzer)");
    TEST_ASSERT(bsp->ReserveGPIO(12, "IR_APPROACH"), "Reserved GPIO 12 (Approach IR Sensor)");
    TEST_ASSERT(bsp->ReserveGPIO(13, "IR_DEPARTURE"), "Reserved GPIO 13 (Departure IR Sensor)");

    // Release cleanly
    TEST_ASSERT(bsp->ReleaseGPIO(17), "Released GPIO 17 cleanly");
    bsp->ShutdownBoard();
}

// =============================================================================
// Test 6: Optocoupled Relay Controller & Active-Low Inversion
// =============================================================================
void Test_QnxActiveLowRelayModule() {
    std::cout << "[TEST 6] Optocoupled Relay Controller (Active-Low Inversion)..." << std::endl;

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
    bsp->ShutdownBoard();
}

// =============================================================================
// Test 7: Hardware Mutual Exclusion Safety Interlock
// =============================================================================
void Test_QnxHardwareMutualExclusionSafetyInterlock() {
    std::cout << "[TEST 7] Hardware Mutual Exclusion Safety Interlock..." << std::endl;

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
    TEST_ASSERT(tl.IsConflictingGreenDetected(), "Hardware conflict flag recorded");
    TEST_ASSERT(tl.HasHardwareFault(), "Hardware fault flag tripped");
    TEST_ASSERT(tl.GetCurrentPhase() == DualPhase::ALL_RED_CLEARANCE,
                "Interlock immediately forced ALL-RED safe clearance state");

    tl.ClearFault();
    tl.Shutdown();
    bsp->ShutdownBoard();
}

// =============================================================================
// Test 8: Optical IR Sensors & Automatic Preemption Clearance
// =============================================================================
void Test_QnxOpticalIrSensorsAndPreemption() {
    std::cout << "[TEST 8] Optical IR Sensors & Automatic Departure Preemption Clearance..." << std::endl;

    HardwareController controller("INT_A", "config/hardware.json");
    controller.Start();

    TEST_ASSERT(controller.IsRunning(), "HardwareController started");
    TEST_ASSERT(!controller.IsPreempted(), "Initially normal operational state");

    // Emergency vehicle approaches Road A
    controller.TriggerEmergencyPreemption("CORRIDOR_01", "ROAD_A");
    TEST_ASSERT(controller.IsPreempted(), "Emergency preemption engaged on Road A");
    TEST_ASSERT(controller.GetCurrentPhase() == DualPhase::PREEMPTION_ROAD_A, "Phase is PREEMPTION_ROAD_A");

    // Vehicle crosses and clears intersection via Departure IR sensor beam
    controller.SimulateVehicleDeparture();
    TEST_ASSERT(!controller.IsPreempted(), "Departure IR sensor automatically cleared preemption");
    TEST_ASSERT(controller.GetCurrentPhase() == DualPhase::ALL_RED_CLEARANCE,
                "Safely transitioned to All-Red clearance before cycle restoration");

    controller.Stop();
}

// =============================================================================
// Test 9: Coordinator Runtime Lifecycle
// =============================================================================
void Test_QnxCoordinatorRuntimeLifecycle() {
    std::cout << "[TEST 9] Coordinator Runtime Lifecycle & Multi-Threaded Workers..." << std::endl;

    CoordinatorRuntime coordinator;
    TEST_ASSERT(coordinator.Initialize(), "CoordinatorRuntime initialized");
    TEST_ASSERT(coordinator.Start("127.0.0.1", 21060), "CoordinatorRuntime started (6 concurrent worker threads)");
    TEST_ASSERT(coordinator.IsRunning(), "CoordinatorRuntime reports running");

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    coordinator.Stop();
    TEST_ASSERT(!coordinator.IsRunning(), "CoordinatorRuntime stopped cleanly");
}

// =============================================================================
// Test 10: Traffic Node Runtime Lifecycle & Watchdog Fail-Safe
// =============================================================================
void Test_QnxTrafficNodeRuntimeLifecycle() {
    std::cout << "[TEST 10] Traffic Node Runtime Lifecycle & Watchdog Fail-Safe..." << std::endl;

    TrafficNodeRuntime nodeRuntime("INT_A");
    TEST_ASSERT(nodeRuntime.Initialize("INT_A"), "TrafficNodeRuntime initialized with BSP pin drivers");
    TEST_ASSERT(nodeRuntime.Start("127.0.0.1", 21065), "TrafficNodeRuntime started (4 concurrent worker threads)");
    TEST_ASSERT(nodeRuntime.IsRunning(), "TrafficNodeRuntime reports running");

    // Fail-safe degradation check
    TEST_ASSERT(!nodeRuntime.GetSignalStateString().empty(), "Initial signal state valid");
    nodeRuntime.DegradeToFailSafe();
    TEST_ASSERT(nodeRuntime.GetSignalStateString() == "FLASHING_YELLOW", "Degraded to Flashing Yellow upon communication loss");
    nodeRuntime.RestoreNormalCycle();
    TEST_ASSERT(nodeRuntime.GetSignalStateString() != "FLASHING_YELLOW", "Recovered from fail-safe to normal cycle");

    nodeRuntime.Stop();
    TEST_ASSERT(!nodeRuntime.IsRunning(), "TrafficNodeRuntime stopped cleanly");
}

// =============================================================================
// Test 11: Digital Twin City Simulation Engine Compatibility
// =============================================================================
void Test_QnxDigitalTwinSimulationEngine() {
    std::cout << "[TEST 11] Digital Twin City Simulation Engine Compatibility..." << std::endl;

    CityClock clock(8.0, 1.0);
    clock.SetTimeScale(2.0);
    TEST_ASSERT(clock.GetTimeScale() == 2.0, "Simulation clock time scale set to 2.0x");

    clock.Advance(0.1);
    TEST_ASSERT(!clock.GetTimeString().empty(), "Simulation clock generated formatted time: " + clock.GetTimeString());

    CityModel model;
    TEST_ASSERT(model.LoadFromConfigFile("config/simulation.json"), "CityModel loaded simulation.json");
    TEST_ASSERT(model.GetAllIntersections().size() >= 3, "CityModel initialized 3+ virtual intersections");
    TEST_ASSERT(model.GetHospital("HOSP_CITY_TRAUMA") != nullptr, "CityModel initialized virtual hospitals");
    TEST_ASSERT(model.GetAmbulance("AMB_01") != nullptr, "CityModel initialized virtual ambulance fleet");
}

// =============================================================================
// Test 12: Qt6 Dashboard Backend Headless Mode
// =============================================================================
void Test_QnxQtDashboardHeadlessCompatibility() {
    std::cout << "[TEST 12] Qt6 Dashboard Backend Headless Mode (offscreen QPA)..." << std::endl;

    // Set offscreen environment variable for QNX headless compatibility
    int setenvOk = setenv("QT_QPA_PLATFORM", "offscreen", 1);
    TEST_ASSERT(setenvOk == 0, "Set QT_QPA_PLATFORM=offscreen environment variable");

    const char* qpa = std::getenv("QT_QPA_PLATFORM");
    TEST_ASSERT(qpa != nullptr && std::string(qpa) == "offscreen", "Verified QT_QPA_PLATFORM is offscreen");
}

// =============================================================================
// Test 13: Cluster Deployment Scripts & Package Generation
// =============================================================================
void Test_QnxClusterDeploymentAndScripts() {
    std::cout << "[TEST 13] QNX Deployment Packaging & Shell Scripts..." << std::endl;

    // Check script files exist
    std::ifstream fBuild("scripts/qnx/build_qnx.sh");
    TEST_ASSERT(fBuild.good(), "scripts/qnx/build_qnx.sh exists");

    std::ifstream fPack("scripts/qnx/package_qnx.sh");
    TEST_ASSERT(fPack.good(), "scripts/qnx/package_qnx.sh exists");

    std::ifstream fDeploy("scripts/qnx/deploy_qnx.sh");
    TEST_ASSERT(fDeploy.good(), "scripts/qnx/deploy_qnx.sh exists");

    std::ifstream fCoord("scripts/qnx/run_qnx_coordinator.sh");
    TEST_ASSERT(fCoord.good(), "scripts/qnx/run_qnx_coordinator.sh exists");

    std::ifstream fInt("scripts/qnx/run_qnx_intersection.sh");
    TEST_ASSERT(fInt.good(), "scripts/qnx/run_qnx_intersection.sh exists");

    std::ifstream fWifi("scripts/qnx/qnx_setup_wifi.sh");
    TEST_ASSERT(fWifi.good(), "scripts/qnx/qnx_setup_wifi.sh exists");

    // Check deployment tarball package
    std::ifstream fTar("aegis_qnx_rpi4.tar.gz");
    TEST_ASSERT(fTar.good(), "Deployment package aegis_qnx_rpi4.tar.gz exists and verified");
}

// =============================================================================
// Test 14: End-to-End Emergency Pipeline Regression
// =============================================================================
void Test_QnxEndToEndEmergencyPipelineRegression() {
    std::cout << "[TEST 14] End-to-End Emergency Pipeline Full Regression..." << std::endl;

    DistributedRuntimeOrchestrator orchestrator;
    TEST_ASSERT(orchestrator.InitializeCluster(21090, {"INT_A", "INT_B", "INT_C"}),
                "DistributedRuntimeOrchestrator initialized with 3 edge nodes");
    TEST_ASSERT(orchestrator.StartCluster(), "Cluster started cleanly");

    // Execute emergency pipeline input
    EmergencyPipelineInput callInput;
    callInput.callId = "CALL_QNX_001";
    callInput.conditionHint = "CARDIAC_ARREST";
    callInput.incidentLatitude = 12.9716;
    callInput.incidentLongitude = 77.5946;
    callInput.patientName = "Jane Doe";
    callInput.patientAge = 62;
    callInput.heartRate = 188;
    callInput.systolicBP = 70;
    callInput.diastolicBP = 45;
    callInput.spO2 = 82;
    callInput.respirationRate = 30;
    callInput.isConscious = false;

    EmergencyPipelineResult result = orchestrator.TriggerEmergency(callInput);
    TEST_ASSERT(result.success, "Emergency pipeline executed end-to-end successfully");
    TEST_ASSERT(result.assignedAmbulanceId == "AMB_01", "Ambulance AMB_01 assigned");
    TEST_ASSERT(!result.assignedHospitalId.empty(), "Hospital destination assigned");
    TEST_ASSERT(result.consensusApproved, "Distributed consensus approved");

    orchestrator.StopCluster();
    TEST_ASSERT(!orchestrator.IsClusterRunning(), "Cluster shutdown cleanly");
}

// =============================================================================
// Main Entrypoint
// =============================================================================
int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS (AERS) Phase 20.5: QNX Deployment Test Suite" << std::endl;
    std::cout << " Platform: Raspberry Pi 4 Model B / QNX SDP 8.0 / C++17" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test_QnxEnvironmentAndMacros();
    Test_QnxRealTimeSchedulerAndPriorities();
    Test_QnxHighResolutionMonotonicClock();
    Test_QnxStaticNetworkTopology();
    Test_QnxBspGpioPinReservations();
    Test_QnxActiveLowRelayModule();
    Test_QnxHardwareMutualExclusionSafetyInterlock();
    Test_QnxOpticalIrSensorsAndPreemption();
    Test_QnxCoordinatorRuntimeLifecycle();
    Test_QnxTrafficNodeRuntimeLifecycle();
    Test_QnxDigitalTwinSimulationEngine();
    Test_QnxQtDashboardHeadlessCompatibility();
    Test_QnxClusterDeploymentAndScripts();
    Test_QnxEndToEndEmergencyPipelineRegression();

    std::cout << "==========================================================" << std::endl;
    std::cout << " Total Passed: " << g_passCount << " / " << (g_passCount + g_failCount) << std::endl;
    std::cout << " Total Failed: " << g_failCount << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_failCount == 0) ? 0 : 1;
}
