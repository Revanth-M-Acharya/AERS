/**
 * @file test_qnx_hardening_phase20_7.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - QNX Production Hardening Validation Suite.
 *
 * Comprehensive integration and stress validation harness covering:
 *  1. GPIO Reservation Manager (BCM validation, duplicate rejection, conflict detection, release)
 *  2. Relay Power-On Self Test (POST: Relay1 PASS .. Relay4 PASS, failure abort)
 *  3. Hardware Watchdog Thread (1 Hz supervision, heartbeat expiry, stuck sensor, thread check-in)
 *  4. Hardware Watchdog Subsystem Health Checks (relays, GPIOs, Wi-Fi link drops)
 *  5. Wi-Fi Link Quality Monitor (RSSI, latency, packet loss rolling average, trends, warnings)
 *  6. Production Deterministic Boot Sequence (10-step nominal progression to ACCEPT_COMMANDS)
 *  7. Production Boot Sequence Failure Handling (safe mode ALL_RED triggering, step halt)
 *  8. Safe Shutdown Manager (10-step graceful termination sequence, GPIO release, sockets)
 *  9. Safe Shutdown Manager Multi-Trigger (SIGINT, SIGTERM, Fatal Error, non-reentrancy)
 * 10. Node Registration Manager (validation, duplicate ID rejection, capability tracking)
 * 11. Node Registration Manager Stale Eviction (heartbeat monitoring, 5.0s offline transition, recovery)
 * 12. CoordinatorRuntime Node Registration Integration (TCP REGISTER & HEARTBEAT ingestion)
 * 13. HardwareController Diagnostics & Recovery (POST integration, fail-safe recovery)
 * 14. End-to-End Cluster Hardened Production Workflow (Boot -> Register -> Watchdog -> Shutdown)
 */

#include "aegis/hardware/GpioReservationManager.hpp"
#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/hardware/WirelessQualityMonitor.hpp"
#include "aegis/hardware/BootManager.hpp"
#include "aegis/hardware/ShutdownManager.hpp"
#include "aegis/coordinator/NodeRegistrationManager.hpp"

#include "hardware/HardwareController.h"
#include "hardware/IntersectionHardware.h"
#include "hardware/RelayController.h"
#include "hardware/TrafficLightController.h"
#include "hardware/WirelessNodeManager.h"
#include "hardware/HardwareDiagnostics.h"
#include "runtime/CoordinatorRuntime.h"
#include "aegis/common/time_utils.hpp"

#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

using namespace aegis;
using namespace aegis::hardware;
using namespace aegis::coordinator;
using namespace aegis::runtime;

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

// =============================================================================
// TEST 1: GPIO Reservation Manager
// =============================================================================
void Test_GpioReservationManager() {
    std::cout << "\n[TEST 1] GPIO Reservation Manager (BCM Validation & Mutual Exclusion)..." << std::endl;

    auto& resMgr = GpioReservationManager::Instance();
    resMgr.Reset();
    TEST_ASSERT(resMgr.GetReservedPinCount() == 0, "Reservation table initially empty");

    // 1. Validate BCM pin numbering
    TEST_ASSERT(resMgr.ValidateBcmPin(0), "GPIO 0 is valid BCM pin");
    TEST_ASSERT(resMgr.ValidateBcmPin(17), "GPIO 17 is valid BCM pin");
    TEST_ASSERT(resMgr.ValidateBcmPin(27), "GPIO 27 is valid BCM pin");
    TEST_ASSERT(!resMgr.ValidateBcmPin(28), "GPIO 28 is invalid (exceeds header range 0..27)");
    TEST_ASSERT(!resMgr.ValidateBcmPin(99), "GPIO 99 is invalid");

    // 2. Reserve valid pin
    bool ok1 = resMgr.ReservePin(17, "RelayController", "Road A Red", bsp::PinMode::OUTPUT);
    TEST_ASSERT(ok1, "Reserved GPIO 17 for RelayController [Road A Red]");
    TEST_ASSERT(resMgr.IsPinReserved(17), "GPIO 17 reports reserved");
    TEST_ASSERT(!resMgr.IsPinReserved(18), "GPIO 18 reports free");

    // 3. Duplicate reservation must fail
    bool dup = resMgr.ReservePin(17, "OpticalSensorFilter", "Approach IR", bsp::PinMode::INPUT);
    TEST_ASSERT(!dup, "Duplicate reservation on GPIO 17 strictly rejected!");

    // 4. Reserve IR sensor pins
    bool ok2 = resMgr.ReservePin(12, "OpticalSensorFilter", "Approach IR Sensor", bsp::PinMode::INPUT);
    bool ok3 = resMgr.ReservePin(13, "OpticalSensorFilter", "Departure IR Sensor", bsp::PinMode::INPUT);
    TEST_ASSERT(ok2 && ok3, "Reserved GPIO 12 & 13 for OpticalSensorFilter");
    TEST_ASSERT(resMgr.GetReservedPinCount() == 3, "Active reservation count is 3");

    // 5. Conflict verification
    std::string conflict;
    bool canRes = resMgr.CanReservePins({22, 23, 24}, conflict);
    TEST_ASSERT(canRes && conflict.empty(), "Can reserve free pins 22, 23, 24");
    bool cannotRes = resMgr.CanReservePins({22, 17, 24}, conflict);
    TEST_ASSERT(!cannotRes && !conflict.empty(), "Conflict detected when candidate set includes GPIO 17");

    // 6. Release pin by unauthorized owner must fail
    bool badRelease = resMgr.ReleasePin(17, "UnauthorizedModule");
    TEST_ASSERT(!badRelease, "Release rejected on owner mismatch");
    TEST_ASSERT(resMgr.IsPinReserved(17), "GPIO 17 remains reserved after unauthorized release attempt");

    // 7. Release pin by valid owner
    bool goodRelease = resMgr.ReleasePin(17, "RelayController");
    TEST_ASSERT(goodRelease, "Release accepted for valid owner");
    TEST_ASSERT(!resMgr.IsPinReserved(17), "GPIO 17 is now free");

    // 8. Diagnostics output
    std::string diag = resMgr.GenerateDiagnostics();
    TEST_ASSERT(diag.find("GPIO12") != std::string::npos, "Diagnostics lists GPIO 12");
    TEST_ASSERT(diag.find("OpticalSensorFilter") != std::string::npos, "Diagnostics lists owner");

    resMgr.Reset();
    TEST_ASSERT(resMgr.GetReservedPinCount() == 0, "Reservation table cleared cleanly");
}

// =============================================================================
// TEST 2: Relay Power-On Self Test (POST)
// =============================================================================
void Test_RelayPowerOnSelfTest() {
    std::cout << "\n[TEST 2] Relay Power-On Self Test (POST: Relay1 PASS .. Relay4 PASS)..." << std::endl;

    auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
    RelayController relay(bsp);

    std::vector<RelayChannelDef> channels = {
        {1, 17, "ROAD_A_RED", true, false},
        {2, 27, "ROAD_A_YELLOW", true, false},
        {3, 22, "ROAD_A_GREEN", true, false},
        {4, 23, "ROAD_B_RED", true, false}
    };

    TEST_ASSERT(relay.Initialize(channels, true), "Initialized 4-channel active-low relay module");

    // 1. Nominal POST Execution
    auto postReport = relay.ExecutePowerOnSelfTest();
    TEST_ASSERT(postReport.allPassed, "Relay Power-On Self Test overall status: PASSED");
    TEST_ASSERT(postReport.channelResults.size() == 4, "POST evaluated all 4 channels");

    TEST_ASSERT(postReport.channelResults[0].statusText == "Relay1 PASS", "Channel 1 status: Relay1 PASS");
    TEST_ASSERT(postReport.channelResults[1].statusText == "Relay2 PASS", "Channel 2 status: Relay2 PASS");
    TEST_ASSERT(postReport.channelResults[2].statusText == "Relay3 PASS", "Channel 3 status: Relay3 PASS");
    TEST_ASSERT(postReport.channelResults[3].statusText == "Relay4 PASS", "Channel 4 status: Relay4 PASS");

    TEST_ASSERT(postReport.channelResults[0].onTestPassed, "Channel 1 Turn ON verified (readback LOW)");
    TEST_ASSERT(postReport.channelResults[0].offTestPassed, "Channel 1 Turn OFF verified (readback HIGH)");

    // 2. Report Formatting
    std::string reportStr = postReport.GenerateReport();
    TEST_ASSERT(reportStr.find("Relay1 PASS") != std::string::npos, "Report includes Relay1 PASS");
    TEST_ASSERT(reportStr.find("Relay4 PASS") != std::string::npos, "Report includes Relay4 PASS");
    TEST_ASSERT(reportStr.find("ALL RELAYS PASSED") != std::string::npos, "Report summary confirmed");

    relay.Shutdown();
}

// =============================================================================
// TEST 3: Hardware Watchdog Thread (1 Hz Supervision)
// =============================================================================
void Test_HardwareWatchdog_1HzSupervision() {
    std::cout << "\n[TEST 3] Hardware Watchdog Thread (1 Hz Supervision & Heartbeat Expiry)..." << std::endl;

    HardwareWatchdog watchdog;
    watchdog.SetCoordinatorTimeoutMs(100); // 100ms for fast testing
    watchdog.SetNodeTimeoutMs(100);

    bool failsafeTriggered = false;
    std::string failsafeReason;
    watchdog.SetFailsafeCallback([&](const std::string& reason) {
        failsafeTriggered = true;
        failsafeReason = reason;
    });

    bool notifySent = false;
    std::string notifyAlert;
    watchdog.SetCoordinatorNotifyCallback([&](const std::string& alert) {
        notifySent = true;
        notifyAlert = alert;
    });

    // 1. Nominal Tick with fresh heartbeats
    watchdog.KickCoordinatorHeartbeat();
    watchdog.KickNodeHeartbeat();
    bool nominalTick = watchdog.TickOnce();
    TEST_ASSERT(nominalTick, "Watchdog tick passed with fresh heartbeats");
    TEST_ASSERT(!watchdog.HasFault(), "Watchdog reports no fault initially");
    TEST_ASSERT(!failsafeTriggered, "Fail-safe not triggered under nominal conditions");

    // 2. Let coordinator heartbeat expire
    std::this_thread::sleep_for(std::chrono::milliseconds(120));
    watchdog.KickNodeHeartbeat(); // Keep node alive, only coordinator expires

    bool expiredTick = watchdog.TickOnce();
    TEST_ASSERT(!expiredTick, "Watchdog tick tripped on coordinator heartbeat expiry");
    TEST_ASSERT(watchdog.HasFault(), "Watchdog flagged fault");
    TEST_ASSERT(failsafeTriggered, "Watchdog invoked failsafe callback");
    TEST_ASSERT(failsafeReason.find("Coordinator heartbeat expired") != std::string::npos,
                "Failsafe reason indicates coordinator timeout");
    TEST_ASSERT(notifySent, "Coordinator alert dispatched");
    TEST_ASSERT(watchdog.GetRecoveryAttemptCount() == 1, "Recovery attempt count is 1");

    // 3. Clear fault & resume
    watchdog.ClearFault();
    TEST_ASSERT(!watchdog.HasFault(), "Watchdog fault cleared");
    watchdog.KickCoordinatorHeartbeat();
    watchdog.KickNodeHeartbeat();
    TEST_ASSERT(watchdog.TickOnce(), "Watchdog recovered and ticks nominally");
}

// =============================================================================
// TEST 4: Hardware Watchdog Subsystem Health Checks
// =============================================================================
void Test_HardwareWatchdog_SubsystemHealthChecks() {
    std::cout << "\n[TEST 4] Hardware Watchdog Subsystem Health Checks (Relays, GPIO, IR)..." << std::endl;

    HardwareWatchdog watchdog;
    watchdog.SetCoordinatorTimeoutMs(5000);
    watchdog.SetNodeTimeoutMs(5000);

    bool relayHealthy = true;
    watchdog.SetRelayHealthChecker([&]() { return relayHealthy; });

    bool gpioHealthy = true;
    watchdog.SetGpioStateChecker([&]() { return gpioHealthy; });

    bool irHealthy = true;
    watchdog.SetIrSensorActivityChecker([&]() { return irHealthy; });

    watchdog.KickCoordinatorHeartbeat();
    watchdog.KickNodeHeartbeat();
    TEST_ASSERT(watchdog.TickOnce(), "All subsystem checkers report healthy");

    // 1. Inject Relay fault
    relayHealthy = false;
    TEST_ASSERT(!watchdog.TickOnce(), "Watchdog trips when relay health checker fails");
    TEST_ASSERT(watchdog.GetFaultReason().find("Relay health check failure") != std::string::npos,
                "Fault reason correctly identifies relay issue");
    relayHealthy = true;
    watchdog.ClearFault();

    // 2. Inject IR sensor stuck fault
    irHealthy = false;
    TEST_ASSERT(!watchdog.TickOnce(), "Watchdog trips when IR sensor activity checker fails");
    TEST_ASSERT(watchdog.GetFaultReason().find("IR optical sensor fault") != std::string::npos,
                "Fault reason correctly identifies IR sensor issue");
    irHealthy = true;
    watchdog.ClearFault();

    // 3. Worker Thread Liveness Check
    watchdog.SetThreadTimeoutMs(50);
    watchdog.KickThread("NetworkWorker");
    TEST_ASSERT(watchdog.TickOnce(), "Thread 'NetworkWorker' healthy with fresh tick");

    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    TEST_ASSERT(!watchdog.TickOnce(), "Watchdog trips when thread 'NetworkWorker' misses deadline");
    TEST_ASSERT(watchdog.GetFaultReason().find("NetworkWorker") != std::string::npos,
                "Fault reason identifies stalled thread name");
}

// =============================================================================
// TEST 5: Wi-Fi Link Quality Monitor
// =============================================================================
void Test_WirelessQualityMonitor() {
    std::cout << "\n[TEST 5] Wi-Fi Link Quality Monitor (RSSI, Latency, Loss, Trends & Warnings)..." << std::endl;

    WirelessQualityMonitor mon(10);
    TEST_ASSERT(mon.GetQualityLevel() == LinkQualityLevel::DISCONNECTED, "Initial status: DISCONNECTED");
    TEST_ASSERT(mon.HasWarning(), "Disconnected link raises warning");

    mon.SetConnected(true);

    // 1. Excellent Quality Ingestion
    for (int i = 0; i < 5; ++i) {
        mon.RecordSample(-52, 1.5, false);
    }
    TEST_ASSERT(mon.GetQualityLevel() == LinkQualityLevel::EXCELLENT, "Link quality is EXCELLENT");
    TEST_ASSERT(!mon.HasWarning(), "No warning under excellent connection");
    TEST_ASSERT(mon.GetPacketLossPercent() == 0.0, "Packet loss is 0.0%");

    // 2. Suboptimal RSSI -> POOR Warning
    for (int i = 0; i < 10; ++i) {
        mon.RecordSample(-78, 4.0, false);
    }
    TEST_ASSERT(mon.GetQualityLevel() == LinkQualityLevel::POOR, "Degraded RSSI (-78 dBm) trips POOR level");
    TEST_ASSERT(mon.HasWarning(), "Warning raised for POOR level");
    TEST_ASSERT(mon.GetWarningMessage().find("POOR") != std::string::npos, "Warning text indicates POOR link");

    // 3. High Packet Loss -> CRITICAL Warning
    for (int i = 0; i < 10; ++i) {
        mon.RecordSample(-80, 50.0, true); // Drops packets
    }
    TEST_ASSERT(mon.GetQualityLevel() == LinkQualityLevel::CRITICAL, "Excessive loss (>= 20%) trips CRITICAL level");
    TEST_ASSERT(mon.GetWarningMessage().find("CRITICAL") != std::string::npos, "Warning text indicates CRITICAL");

    // 4. Reconnect Tracking
    mon.RecordReconnect();
    mon.RecordReconnect();
    TEST_ASSERT(mon.GetReconnectCount() == 2, "Reconnect counter is 2");

    // 5. Telemetry & JSON Export
    std::string jsonStr = mon.ToJson();
    TEST_ASSERT(jsonStr.find("\"quality_level\":\"CRITICAL\"") != std::string::npos, "JSON includes quality level");
    TEST_ASSERT(jsonStr.find("\"reconnect_count\":2") != std::string::npos, "JSON includes reconnect count");

    std::string report = mon.GenerateReport();
    TEST_ASSERT(report.find("WI-FI LINK QUALITY REPORT") != std::string::npos, "Report generated successfully");

    mon.Reset();
    TEST_ASSERT(mon.GetReconnectCount() == 0, "Monitor reset cleanly");
}

// =============================================================================
// TEST 6: Production Deterministic Boot Sequence (Success Path)
// =============================================================================
void Test_ProductionBootSequence_SuccessPath() {
    std::cout << "\n[TEST 6] Production Deterministic Boot Sequence (10-Step Success Path)..." << std::endl;

    BootManager bootMgr;
    TEST_ASSERT(bootMgr.GetCurrentStep() == HardwareBootStep::POWER_ON, "Initial state: POWER_ON");

    std::vector<HardwareBootStep> executedSteps;
    for (uint8_t s = 1; s <= 10; ++s) {
        auto step = static_cast<HardwareBootStep>(s);
        bootMgr.RegisterStepExecutor(step, [step, &executedSteps](std::string& details) {
            executedSteps.push_back(step);
            details = HardwareBootStepToString(step) + " completed verified.";
            return true;
        });
    }

    bool bootSuccess = bootMgr.RunFullBootSequence();
    TEST_ASSERT(bootSuccess, "10-Step Deterministic Boot Sequence returned TRUE");
    TEST_ASSERT(bootMgr.IsBootCompleted(), "Boot completed state flagged");
    TEST_ASSERT(!bootMgr.IsInSafeMode(), "Node NOT in safe mode");
    TEST_ASSERT(executedSteps.size() == 10, "All 10 steps executed in sequence");

    TEST_ASSERT(executedSteps[0] == HardwareBootStep::BSP_INITIALIZATION, "Step 1: BSP Initialization");
    TEST_ASSERT(executedSteps[1] == HardwareBootStep::GPIO_RESERVATION, "Step 2: GPIO Reservation");
    TEST_ASSERT(executedSteps[2] == HardwareBootStep::RELAY_SELF_TEST, "Step 3: Relay Self Test");
    TEST_ASSERT(executedSteps[3] == HardwareBootStep::HARDWARE_DIAGNOSTICS, "Step 4: Hardware Diagnostics");
    TEST_ASSERT(executedSteps[4] == HardwareBootStep::WIFI_STARTUP, "Step 5: Wi-Fi Startup");
    TEST_ASSERT(executedSteps[5] == HardwareBootStep::NODE_REGISTRATION, "Step 6: Node Registration");
    TEST_ASSERT(executedSteps[6] == HardwareBootStep::COORDINATOR_HANDSHAKE, "Step 7: Coordinator Handshake");
    TEST_ASSERT(executedSteps[7] == HardwareBootStep::WATCHDOG_STARTUP, "Step 8: Watchdog Activation");
    TEST_ASSERT(executedSteps[8] == HardwareBootStep::HARDWARE_READY, "Step 9: Hardware Ready");
    TEST_ASSERT(executedSteps[9] == HardwareBootStep::ACCEPT_TRAFFIC_COMMANDS, "Step 10: Accept Traffic Commands");

    auto reports = bootMgr.GetStepReports();
    TEST_ASSERT(reports.size() == 10, "Generated exactly 10 step execution reports");

    std::string report = bootMgr.GenerateBootReport();
    TEST_ASSERT(report.find("COMPLETED / READY") != std::string::npos, "Boot report reflects COMPLETED");
}

// =============================================================================
// TEST 7: Production Boot Sequence Failure Handling
// =============================================================================
void Test_ProductionBootSequence_FailureAndSafeMode() {
    std::cout << "\n[TEST 7] Production Boot Sequence Failure Handling (Safe Mode & Abort)..." << std::endl;

    BootManager bootMgr;
    bool safeModeInvoked = false;
    std::string safeModeReason;
    bootMgr.SetSafeModeHandler([&](const std::string& reason) {
        safeModeInvoked = true;
        safeModeReason = reason;
    });

    // Step 1: BSP passed
    bootMgr.RegisterStepExecutor(HardwareBootStep::BSP_INITIALIZATION, [](std::string& d) {
        d = "BSP OK"; return true;
    });

    // Step 2: GPIO passed
    bootMgr.RegisterStepExecutor(HardwareBootStep::GPIO_RESERVATION, [](std::string& d) {
        d = "GPIO OK"; return true;
    });

    // Step 3: Relay POST FAILS!
    bootMgr.RegisterStepExecutor(HardwareBootStep::RELAY_SELF_TEST, [](std::string& d) {
        d = "Relay 2 coil readback mismatch"; return false;
    });

    bool bootSuccess = bootMgr.RunFullBootSequence();
    TEST_ASSERT(!bootSuccess, "Boot sequence returned FALSE upon step failure");
    TEST_ASSERT(bootMgr.IsBootFailed(), "Boot manager marked BOOT_FAILED");
    TEST_ASSERT(bootMgr.IsInSafeMode(), "Boot manager entered SAFE MODE");
    TEST_ASSERT(safeModeInvoked, "Safe mode callback invoked");
    TEST_ASSERT(safeModeReason.find("Relay 2 coil readback mismatch") != std::string::npos,
                "Safe mode reason captures exact failure details");

    // Ensure step 4 was never executed
    auto reports = bootMgr.GetStepReports();
    TEST_ASSERT(reports.size() == 3, "Boot sequence halted immediately after Step 3 failure");
}

// =============================================================================
// TEST 8: Safe Shutdown Manager (10-Step Graceful Termination)
// =============================================================================
void Test_SafeShutdownManager_Graceful10Steps() {
    std::cout << "\n[TEST 8] Safe Shutdown Manager (10-Step Graceful Termination Sequence)..." << std::endl;

    auto& sMgr = ShutdownManager::Instance();
    sMgr.Reset();
    TEST_ASSERT(!sMgr.IsShuttingDown(), "Initially not shutting down");
    TEST_ASSERT(!sMgr.IsShutdownComplete(), "Initially not terminated");

    std::vector<ShutdownStep> executedSteps;
    for (uint8_t s = 1; s <= 10; ++s) {
        auto step = static_cast<ShutdownStep>(s);
        sMgr.SetStepHook(step, [step, &executedSteps](std::string& details) {
            executedSteps.push_back(step);
            details = ShutdownStepToString(step) + " completed.";
            return true;
        });
    }

    bool shutdownSuccess = sMgr.ExecuteShutdown(ShutdownReason::NORMAL);
    TEST_ASSERT(shutdownSuccess, "Safe Shutdown executed successfully");
    TEST_ASSERT(sMgr.IsShutdownComplete(), "Shutdown marked complete");
    TEST_ASSERT(executedSteps.size() == 10, "All 10 shutdown steps executed");

    TEST_ASSERT(executedSteps[0] == ShutdownStep::STOP_COORDINATOR_COMMANDS, "Step 1: Stop Coordinator Commands");
    TEST_ASSERT(executedSteps[1] == ShutdownStep::NOTIFY_PEERS, "Step 2: Notify Peers");
    TEST_ASSERT(executedSteps[2] == ShutdownStep::STOP_WATCHDOG, "Step 3: Stop Watchdog");
    TEST_ASSERT(executedSteps[3] == ShutdownStep::SAVE_LOGS, "Step 4: Save Logs");
    TEST_ASSERT(executedSteps[4] == ShutdownStep::SAVE_DIAGNOSTICS, "Step 5: Save Diagnostics");
    TEST_ASSERT(executedSteps[5] == ShutdownStep::TURN_OFF_RELAYS, "Step 6: Turn OFF Relays");
    TEST_ASSERT(executedSteps[6] == ShutdownStep::FORCE_ALL_RED, "Step 7: Force ALL RED");
    TEST_ASSERT(executedSteps[7] == ShutdownStep::RELEASE_GPIO, "Step 8: Release GPIO");
    TEST_ASSERT(executedSteps[8] == ShutdownStep::CLOSE_SOCKETS, "Step 9: Close Sockets");
    TEST_ASSERT(executedSteps[9] == ShutdownStep::SHUTDOWN_COMPLETE, "Step 10: Shutdown Complete");

    std::string report = sMgr.GenerateShutdownReport();
    TEST_ASSERT(report.find("COMPLETED") != std::string::npos, "Shutdown report confirmed");
    sMgr.Reset();
}

// =============================================================================
// TEST 9: Safe Shutdown Manager Multi-Trigger & Signals
// =============================================================================
void Test_SafeShutdownManager_MultiTrigger() {
    std::cout << "\n[TEST 9] Safe Shutdown Manager Multi-Trigger (Signals & Non-Reentrancy)..." << std::endl;

    auto& sMgr = ShutdownManager::Instance();
    sMgr.Reset();

    // Verify non-reentrancy
    sMgr.SetStepHook(ShutdownStep::STOP_COORDINATOR_COMMANDS, [](std::string& d) {
        // Attempt nested call while shutdown is running
        bool nested = ShutdownManager::Instance().ExecuteShutdown(ShutdownReason::POWER_OFF_REQUEST);
        TEST_ASSERT(!nested, "Nested/re-entrant shutdown request strictly rejected!");
        d = "Step OK";
        return true;
    });

    bool ok = sMgr.ExecuteShutdown(ShutdownReason::SIGINT_RECEIVED);
    TEST_ASSERT(ok, "SIGINT shutdown executed");
    TEST_ASSERT(sMgr.GetShutdownReason() == ShutdownReason::SIGINT_RECEIVED, "Shutdown reason recorded as SIGINT");

    sMgr.Reset();
    bool okTerm = sMgr.ExecuteShutdown(ShutdownReason::SIGTERM_RECEIVED);
    TEST_ASSERT(okTerm, "SIGTERM shutdown executed");
    TEST_ASSERT(sMgr.GetShutdownReason() == ShutdownReason::SIGTERM_RECEIVED, "Shutdown reason recorded as SIGTERM");

    sMgr.Reset();
    bool okFatal = sMgr.ExecuteShutdown(ShutdownReason::FATAL_ERROR);
    TEST_ASSERT(okFatal, "FATAL_ERROR shutdown executed");
    TEST_ASSERT(sMgr.GetShutdownReason() == ShutdownReason::FATAL_ERROR, "Shutdown reason recorded as FATAL_ERROR");

    sMgr.Reset();
}

// =============================================================================
// TEST 10: Node Registration Manager (Validation & Duplicate Rejection)
// =============================================================================
void Test_NodeRegistrationManager_ValidationAndDuplicateRejection() {
    std::cout << "\n[TEST 10] Node Registration Manager (Validation & Duplicate Rejection)..." << std::endl;

    NodeRegistrationManager regMgr;
    TEST_ASSERT(regMgr.GetTotalNodeCount() == 0, "No nodes registered initially");

    // 1. Valid Registration
    NodeRegistrationRecord recA;
    recA.nodeId = "INT_A";
    recA.ipAddress = "192.168.1.101";
    recA.softwareVersion = "20.7.0";
    recA.hardwareVersion = "Raspberry Pi 4 Model B Rev 1.4";
    recA.relayStatus = "Relay1_PASS..Relay4_PASS";
    recA.gpioStatus = "BCM_PINS_RESERVED_OK";
    recA.readyState = true;
    recA.capabilities = {"RELAY_4CH", "OPTICAL_IR_DUAL", "GREEN_WAVE"};

    std::string err;
    bool okA = regMgr.RegisterNode(recA, err);
    TEST_ASSERT(okA && err.empty(), "Registered node INT_A successfully");
    TEST_ASSERT(regMgr.IsNodeRegistered("INT_A"), "Node INT_A is registered");
    TEST_ASSERT(regMgr.IsNodeOnline("INT_A"), "Node INT_A is online");
    TEST_ASSERT(regMgr.GetOnlineNodeCount() == 1, "Online node count is 1");

    // 2. Duplicate Node ID Collision from DIFFERENT IP must be rejected!
    NodeRegistrationRecord recDup = recA;
    recDup.ipAddress = "192.168.1.199"; // Imposter IP
    bool okDup = regMgr.RegisterNode(recDup, err);
    TEST_ASSERT(!okDup, "Duplicate registration from conflicting IP strictly rejected!");
    TEST_ASSERT(err.find("Duplicate Node ID collision") != std::string::npos, "Collision error details provided");

    // 3. Reconnection from SAME IP is permitted
    NodeRegistrationRecord recRecon = recA;
    recRecon.softwareVersion = "20.7.1";
    bool okRecon = regMgr.RegisterNode(recRecon, err);
    TEST_ASSERT(okRecon, "Reconnection from verified IP accepted");

    NodeRegistrationRecord fetched;
    TEST_ASSERT(regMgr.GetNodeRecord("INT_A", fetched), "Retrieved node record");
    TEST_ASSERT(fetched.softwareVersion == "20.7.1", "Node record updated on re-registration");

    // 4. Register Nodes B and C
    NodeRegistrationRecord recB = recA;
    recB.nodeId = "INT_B";
    recB.ipAddress = "192.168.1.102";
    regMgr.RegisterNode(recB, err);

    NodeRegistrationRecord recC = recA;
    recC.nodeId = "INT_C";
    recC.ipAddress = "192.168.1.103";
    regMgr.RegisterNode(recC, err);

    TEST_ASSERT(regMgr.GetTotalNodeCount() == 3, "Total registered nodes: 3");
    TEST_ASSERT(regMgr.GetOnlineNodeCount() == 3, "All 3 nodes online");

    std::string report = regMgr.GenerateReport();
    TEST_ASSERT(report.find("INT_A") != std::string::npos, "Report includes INT_A");
    TEST_ASSERT(report.find("INT_C") != std::string::npos, "Report includes INT_C");
}

// =============================================================================
// TEST 11: Node Registration Manager Stale Eviction & Recovery
// =============================================================================
void Test_NodeRegistrationManager_StaleEviction() {
    std::cout << "\n[TEST 11] Node Registration Manager Stale Eviction (5.0s Timeout)..." << std::endl;

    NodeRegistrationManager regMgr;
    std::string err;

    NodeRegistrationRecord recA;
    recA.nodeId = "INT_A";
    recA.ipAddress = "192.168.1.101";
    regMgr.RegisterNode(recA, err);

    NodeRegistrationRecord recB;
    recB.nodeId = "INT_B";
    recB.ipAddress = "192.168.1.102";
    regMgr.RegisterNode(recB, err);

    bool alertFired = false;
    std::string alertNode;
    NodeOnlineState alertState = NodeOnlineState::ONLINE;
    regMgr.SetAlertCallback([&](const std::string& nId, NodeOnlineState s, const std::string&) {
        alertFired = true;
        alertNode = nId;
        alertState = s;
    });

    // 1. Fresh heartbeat keeps nodes online
    regMgr.RecordHeartbeat("INT_A");
    regMgr.RecordHeartbeat("INT_B");
    size_t pruned0 = regMgr.PruneStaleNodes(5.0);
    TEST_ASSERT(pruned0 == 0, "No nodes pruned with fresh heartbeats");
    TEST_ASSERT(regMgr.GetOnlineNodeCount() == 2, "Both nodes remain online");

    // 2. Prune with microsecond timeout to simulate > 5.0s lapse on INT_B
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    regMgr.RecordHeartbeat("INT_A"); // Only kick A

    size_t pruned = regMgr.PruneStaleNodes(0.002); // 2ms timeout
    TEST_ASSERT(pruned == 1, "Exactly 1 stale node pruned");
    TEST_ASSERT(regMgr.IsNodeOnline("INT_A"), "INT_A remains ONLINE");
    TEST_ASSERT(!regMgr.IsNodeOnline("INT_B"), "INT_B transitioned to OFFLINE");
    TEST_ASSERT(alertFired && alertNode == "INT_B" && alertState == NodeOnlineState::OFFLINE,
                "Node offline alert dispatched for INT_B");

    // 3. Heartbeat from INT_B recovers it back to ONLINE
    alertFired = false;
    regMgr.RecordHeartbeat("INT_B");
    TEST_ASSERT(regMgr.IsNodeOnline("INT_B"), "INT_B recovered to ONLINE on fresh heartbeat");
    TEST_ASSERT(alertFired && alertNode == "INT_B" && alertState == NodeOnlineState::ONLINE,
                "Node online recovery alert dispatched for INT_B");
}

// =============================================================================
// TEST 12: CoordinatorRuntime Node Registration Integration
// =============================================================================
void Test_CoordinatorRuntime_NodeRegistrationIntegration() {
    std::cout << "\n[TEST 12] CoordinatorRuntime Node Registration Integration..." << std::endl;

    CoordinatorRuntime coordinator;
    TEST_ASSERT(coordinator.Initialize("config/simulation.json"), "CoordinatorRuntime initialized");

    auto regMgr = coordinator.GetNodeRegistrationManager();
    TEST_ASSERT(regMgr != nullptr, "Coordinator provides NodeRegistrationManager instance");

    // Simulate incoming REGISTER message
    aegis::network::Message regMsg(aegis::network::MessageType::REGISTER, "INT_A", "192.168.1.101");
    coordinator.ProcessIncomingMessage("INT_A", regMsg);

    TEST_ASSERT(regMgr->IsNodeRegistered("INT_A"), "Node INT_A automatically registered via network message");
    TEST_ASSERT(regMgr->IsNodeOnline("INT_A"), "Node INT_A marked online");

    // Simulate incoming HEARTBEAT message
    aegis::network::Message hbMsg(aegis::network::MessageType::HEARTBEAT, "INT_A", "HEARTBEAT");
    coordinator.ProcessIncomingMessage("INT_A", hbMsg);

    NodeRegistrationRecord rec;
    TEST_ASSERT(regMgr->GetNodeRecord("INT_A", rec), "Retrieved node record from coordinator");
    TEST_ASSERT(rec.lastHeartbeatTimestampUs > 0, "Last heartbeat timestamp recorded");
}

// =============================================================================
// TEST 13: HardwareController Diagnostics & Recovery
// =============================================================================
void Test_HardwareController_DiagnosticsAndRecovery() {
    std::cout << "\n[TEST 13] HardwareController Diagnostics, POST & Fail-Safe Recovery..." << std::endl;

    HardwareController controller("INT_A", "config/hardware.json");
    TEST_ASSERT(controller.GetNodeId() == "INT_A", "HardwareController initialized for INT_A");

    // 1. Verify subsystem attachments
    TEST_ASSERT(controller.GetWatchdog() != nullptr, "Watchdog attached to controller");
    TEST_ASSERT(controller.GetBootManager() != nullptr, "BootManager attached to controller");
    TEST_ASSERT(controller.GetDiagnostics() != nullptr, "Diagnostics attached to controller");
    TEST_ASSERT(controller.GetWirelessManager() != nullptr, "WirelessManager attached to controller");

    // 2. Run diagnostics suite including POST
    bool diagOk = controller.RunDiagnostics();
    TEST_ASSERT(diagOk, "Hardware diagnostics suite (including POST) PASSED");

    std::string diagRep = controller.GetDiagnosticReport();
    TEST_ASSERT(diagRep.find("Relay Power-On Self Test (POST)") != std::string::npos,
                "Diagnostic report includes Relay POST");

    // 3. Trigger fail-safe and recover
    controller.TriggerFailSafe("SIMULATED_OPTICAL_FAULT");
    TEST_ASSERT(controller.IsInFailSafe(), "Controller entered fail-safe mode");

    controller.RecoverFromFailSafe();
    TEST_ASSERT(!controller.IsInFailSafe(), "Controller recovered cleanly from fail-safe");
}

// =============================================================================
// TEST 14: End-to-End Cluster Hardened Production Workflow
// =============================================================================
void Test_IntegratedCluster_E2EHardenedWorkflow() {
    std::cout << "\n[TEST 14] End-to-End Cluster Hardened Production Workflow (3 Nodes + Coord)..." << std::endl;

    // 1. Boot Coordinator Runtime
    CoordinatorRuntime coordinator;
    TEST_ASSERT(coordinator.Initialize("config/simulation.json"), "Coordinator initialized");
    auto regMgr = coordinator.GetNodeRegistrationManager();

    // 2. Boot Intersection Node A through 10-step Boot Sequence
    BootManager bootA;
    TEST_ASSERT(bootA.RunFullBootSequence(), "Intersection A completed 10-step production boot");

    // 3. Register Node A with Coordinator
    NodeRegistrationRecord recA;
    recA.nodeId = "INT_A";
    recA.ipAddress = "192.168.1.101";
    std::string err;
    TEST_ASSERT(regMgr->RegisterNode(recA, err), "Node A registered with Central Coordinator");

    // 4. Activate Hardware Watchdog for Node A
    HardwareWatchdog watchdogA;
    watchdogA.KickCoordinatorHeartbeat();
    watchdogA.KickNodeHeartbeat();
    TEST_ASSERT(watchdogA.TickOnce(), "Node A watchdog active and healthy");

    // 5. Track Wi-Fi quality
    WirelessQualityMonitor wifiMonA;
    wifiMonA.SetConnected(true);
    wifiMonA.RecordSample(-55, 1.8, false);
    TEST_ASSERT(wifiMonA.GetQualityLevel() == LinkQualityLevel::EXCELLENT, "Node A Wi-Fi quality EXCELLENT");

    // 6. Graceful Safe Shutdown for Node A
    auto& shutMgr = ShutdownManager::Instance();
    shutMgr.Reset();
    bool shutdownOk = shutMgr.ExecuteShutdown(ShutdownReason::NORMAL);
    TEST_ASSERT(shutdownOk, "Node A executed graceful 10-step safe shutdown");
    TEST_ASSERT(shutMgr.IsShutdownComplete(), "Node A termination confirmed");
    shutMgr.Reset();
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS (AERS) Phase 20.7: QNX Production Hardening\n";
    std::cout << " Production Hardware & System Validation Test Harness\n";
    std::cout << " Target: 4 Raspberry Pi 4 Model B / QNX SDP 8.0 (aarch64le)\n";
    std::cout << "==========================================================\n";

    Test_GpioReservationManager();
    Test_RelayPowerOnSelfTest();
    Test_HardwareWatchdog_1HzSupervision();
    Test_HardwareWatchdog_SubsystemHealthChecks();
    Test_WirelessQualityMonitor();
    Test_ProductionBootSequence_SuccessPath();
    Test_ProductionBootSequence_FailureAndSafeMode();
    Test_SafeShutdownManager_Graceful10Steps();
    Test_SafeShutdownManager_MultiTrigger();
    Test_NodeRegistrationManager_ValidationAndDuplicateRejection();
    Test_NodeRegistrationManager_StaleEviction();
    Test_CoordinatorRuntime_NodeRegistrationIntegration();
    Test_HardwareController_DiagnosticsAndRecovery();
    Test_IntegratedCluster_E2EHardenedWorkflow();

    std::cout << "\n==========================================================\n";
    std::cout << " Phase 20.7 Validation Summary:\n";
    std::cout << " Total Passed: " << g_passCount << "\n";
    std::cout << " Total Failed: " << g_failCount << "\n";
    std::cout << "==========================================================\n";

    if (g_failCount > 0) {
        std::cerr << ">>> [FAIL] Phase 20.7 verification tests failed!" << std::endl;
        return 1;
    }

    std::cout << ">>> [SUCCESS] All Phase 20.7 Production Hardening Tests Passed 100%!" << std::endl;
    return 0;
}
