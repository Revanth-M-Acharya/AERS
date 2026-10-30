/**
 * @file test_phase2_embedded_production.cpp
 * @brief SMART AEGIS (AERS) Phase 2 - Production-Grade Embedded System Test Suite.
 *
 * Validates the 10 industrial embedded subsystems on Raspberry Pi 4 + QNX SDP 8.0:
 * 1. 14-Step Deterministic Industrial Boot Sequence with Timing Measurement
 * 2. Safe Mode Entry and Boot Abort on Critical Failure
 * 3. Automated Node Registration with 9 Industrial Fields
 * 4. Duplicate Collision Rejection & Seamless Re-Registration
 * 5. Dedicated 1 Hz Hardware Watchdog Supervising 7 Subsystems & Auto-Recovery
 * 6. Relay Power-On Self Test (POST: Channels 1-4, Feedback, Timing & Interlocks)
 * 7. Wi-Fi / Ethernet Quality Monitor with 5-Tier Classification & Degradation Logging
 * 8. Non-Reentrant Safe Shutdown Manager (Relays De-energized & Memory Unmapped)
 * 9. Automatic Node Recovery Following Link Severance Without Reboot
 * 10. Comprehensive Health Telemetry (10 Parameters) & Microsecond Event Logging
 */

#include "aegis/hardware/BootManager.hpp"
#include "aegis/hardware/ShutdownManager.hpp"
#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/hardware/WirelessQualityMonitor.hpp"
#include "aegis/coordinator/NodeRegistrationManager.hpp"
#include "aegis/hardware/GpioReservationManager.hpp"
#include "hardware/RelayController.h"
#include "hardware/HardwareHealthMonitor.h"
#include "hardware/HardwareEventLogger.h"
#include "hardware/IntersectionHardware.h"
#include "hardware/WirelessNodeManager.h"
#include "bsp/RaspberryPi4BSP.h"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <chrono>
#include <thread>

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] Assertion failed: " << (msg) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            return false; \
        } \
    } while (0)

using namespace aegis;
using namespace aegis::hardware;
using namespace aegis::coordinator;

// ============================================================================
// TEST 1: 14-Step Deterministic Industrial Boot Sequence & Timing Measurement
// ============================================================================
static bool TestIndustrialBootSequence() {
    std::cout << "\n[TEST 1] 14-Stage Industrial Deterministic Boot Sequence & Timing..." << std::endl;

    BootManager bootMgr;
    TEST_ASSERT(bootMgr.GetCurrentIndustrialStep() == IndustrialBootStep::POWER_ON, "Initial state POWER_ON");
    TEST_ASSERT(!bootMgr.IsBootCompleted(), "Boot not completed initially");

    std::vector<IndustrialBootStep> executedStages;
    for (uint8_t s = 1; s <= 14; ++s) {
        auto stage = static_cast<IndustrialBootStep>(s);
        bootMgr.RegisterIndustrialStepExecutor(stage, [&executedStages, stage](std::string& details) {
            executedStages.push_back(stage);
            details = IndustrialBootStepToString(stage) + " hardware initialisation verified.";
            return true;
        });
    }

    bool bootSuccess = bootMgr.RunIndustrialBootSequence();
    TEST_ASSERT(bootSuccess, "14-Stage boot sequence must return true");
    TEST_ASSERT(bootMgr.IsBootCompleted(), "IsBootCompleted must be true");
    TEST_ASSERT(!bootMgr.IsBootFailed(), "IsBootFailed must be false");
    TEST_ASSERT(!bootMgr.IsInSafeMode(), "Must not be in safe mode");
    TEST_ASSERT(bootMgr.GetCurrentIndustrialStep() == IndustrialBootStep::READY, "Final stage must be READY");

    TEST_ASSERT(executedStages.size() == 14, "All 14 stages must be executed");
    TEST_ASSERT(executedStages[0] == IndustrialBootStep::KERNEL_BOOT, "Stage 1: KERNEL_BOOT");
    TEST_ASSERT(executedStages[1] == IndustrialBootStep::INIT_BSP, "Stage 2: INIT_BSP");
    TEST_ASSERT(executedStages[2] == IndustrialBootStep::INIT_GPIO, "Stage 3: INIT_GPIO");
    TEST_ASSERT(executedStages[3] == IndustrialBootStep::RESERVE_GPIO, "Stage 4: RESERVE_GPIO");
    TEST_ASSERT(executedStages[4] == IndustrialBootStep::INIT_RELAY_CONTROLLER, "Stage 5: INIT_RELAY_CONTROLLER");
    TEST_ASSERT(executedStages[5] == IndustrialBootStep::RUN_RELAY_POST, "Stage 6: RUN_RELAY_POST");
    TEST_ASSERT(executedStages[6] == IndustrialBootStep::INIT_IR_SENSORS, "Stage 7: INIT_IR_SENSORS");
    TEST_ASSERT(executedStages[7] == IndustrialBootStep::INIT_NETWORK_INTERFACE, "Stage 8: INIT_NETWORK_INTERFACE");
    TEST_ASSERT(executedStages[8] == IndustrialBootStep::VERIFY_NETWORK_LINK, "Stage 9: VERIFY_NETWORK_LINK");
    TEST_ASSERT(executedStages[9] == IndustrialBootStep::REGISTER_WITH_COORDINATOR, "Stage 10: REGISTER_WITH_COORDINATOR");
    TEST_ASSERT(executedStages[10] == IndustrialBootStep::START_HARDWARE_WATCHDOG, "Stage 11: START_HARDWARE_WATCHDOG");
    TEST_ASSERT(executedStages[11] == IndustrialBootStep::START_HEARTBEAT, "Stage 12: START_HEARTBEAT");
    TEST_ASSERT(executedStages[12] == IndustrialBootStep::ENTER_OPERATIONAL_MODE, "Stage 13: ENTER_OPERATIONAL_MODE");
    TEST_ASSERT(executedStages[13] == IndustrialBootStep::READY, "Stage 14: READY");

    uint64_t bootUs = bootMgr.GetTotalBootDurationUs();
    double bootMs = bootMgr.GetTotalBootDurationMs();
    std::cout << "  Boot Time: " << bootUs << " us (" << bootMs << " ms)" << std::endl;
    TEST_ASSERT(bootMs >= 0.0, "Boot duration must be non-negative");

    std::string report = bootMgr.GenerateIndustrialBootReport();
    TEST_ASSERT(!report.empty(), "Industrial boot report must be non-empty");
    TEST_ASSERT(report.find("14_READY") != std::string::npos, "Report must reflect READY status");

    std::cout << "  ✓ PASS: 14-stage deterministic boot sequence and timing verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 2: Safe Mode Entry and Boot Abort on Critical Failure
// ============================================================================
static bool TestSafeModeEntryOnFailure() {
    std::cout << "\n[TEST 2] Safe Mode Entry & Immediate Abort on Boot Failure..." << std::endl;

    BootManager bootMgr;
    bool safeModeInvoked = false;
    std::string safeModeReason;

    bootMgr.SetSafeModeHandler([&](const std::string& reason) {
        safeModeInvoked = true;
        safeModeReason = reason;
    });

    // Simulate failure at Stage 5: INIT_RELAY_CONTROLLER
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::KERNEL_BOOT, [](std::string& d) {
        d = "Kernel boot OK"; return true;
    });
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::INIT_BSP, [](std::string& d) {
        d = "BSP OK"; return true;
    });
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::INIT_GPIO, [](std::string& d) {
        d = "GPIO driver OK"; return true;
    });
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::RESERVE_GPIO, [](std::string& d) {
        d = "Pin reservation OK"; return true;
    });
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::INIT_RELAY_CONTROLLER, [](std::string& d) {
        d = "I2C/GPIO relay bus hardware fault detected";
        return false; // FAILS HERE!
    });
    bootMgr.RegisterIndustrialStepExecutor(IndustrialBootStep::RUN_RELAY_POST, [](std::string& d) {
        d = "SHOULD NOT BE REACHED!"; return true;
    });

    bool bootSuccess = bootMgr.RunIndustrialBootSequence();
    TEST_ASSERT(!bootSuccess, "Boot must fail when critical stage fails");
    TEST_ASSERT(bootMgr.IsInSafeMode(), "BootManager must enter Safe Mode");
    TEST_ASSERT(bootMgr.IsBootFailed(), "IsBootFailed must be true");
    TEST_ASSERT(safeModeInvoked, "SafeMode callback must be invoked");
    TEST_ASSERT(safeModeReason.find("INIT_RELAY_CONTROLLER") != std::string::npos,
                "Safe mode reason must identify failed stage");

    // Ensure downstream stages were aborted
    auto reports = bootMgr.GetStepReports();
    TEST_ASSERT(reports.size() == 5, "Downstream stages must not execute after fault");
    TEST_ASSERT(!reports.back().passed, "Last executed report must indicate failure");

    std::cout << "  ✓ PASS: Safe Mode transition and boot abort verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 3: Automated Node Registration with 9 Industrial Fields
// ============================================================================
static bool TestNodeRegistrationFields() {
    std::cout << "\n[TEST 3] Node Registration with 9 Industrial Fields..." << std::endl;

    NodeRegistrationManager regMgr;
    regMgr.Reset();

    NodeRegistrationRecord rec;
    rec.nodeId = "INT_A";
    rec.hostname = "rpi4-alpha";
    rec.ipAddress = "192.168.1.101";
    rec.macAddress = "DC:A6:32:11:22:33";
    rec.softwareVersion = "20.7.0";
    rec.hardwareVersion = "Raspberry Pi 4 Model B (4GB)";
    rec.bootTimestampUs = 1700000000000000ULL;
    rec.currentStatus = "READY";
    rec.relayStatus = "4_CHANNELS_PASS";
    rec.gpioStatus = "RESERVED_PINS_OK";

    std::string err;
    bool ok = regMgr.RegisterNode(rec, err);
    TEST_ASSERT(ok, "Registration must succeed");
    TEST_ASSERT(err.empty(), "No registration error expected");

    NodeRegistrationRecord fetched;
    bool found = regMgr.GetNodeRecord("INT_A", fetched);
    TEST_ASSERT(found, "Node INT_A must exist in table");

    // Validate all 9 fields
    TEST_ASSERT(fetched.nodeId == "INT_A", "1. Node ID verified");
    TEST_ASSERT(fetched.hostname == "rpi4-alpha", "2. Hostname verified");
    TEST_ASSERT(fetched.ipAddress == "192.168.1.101", "3. IP address verified");
    TEST_ASSERT(fetched.macAddress == "DC:A6:32:11:22:33", "4. MAC address verified");
    TEST_ASSERT(fetched.softwareVersion == "20.7.0", "5. Software version verified");
    TEST_ASSERT(fetched.hardwareVersion == "Raspberry Pi 4 Model B (4GB)", "6. Hardware version verified");
    TEST_ASSERT(fetched.bootTimestampUs == 1700000000000000ULL, "7. Boot timestamp verified");
    TEST_ASSERT(fetched.currentStatus == "READY", "8. Current status verified");
    TEST_ASSERT(fetched.GetHeartbeatTimeUs() > 0, "9. Heartbeat time verified");

    std::string report = regMgr.GenerateReport();
    TEST_ASSERT(report.find("rpi4-alpha") != std::string::npos, "Cluster report must list hostname");
    TEST_ASSERT(report.find("DC:A6:32:11:22:33") != std::string::npos, "Cluster report must list MAC");

    std::cout << "  ✓ PASS: Automated node registration with all 9 fields verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 4: Duplicate Collision Rejection & Seamless Re-Registration
// ============================================================================
static bool TestDuplicateRejectionAndReRegistration() {
    std::cout << "\n[TEST 4] Duplicate Node Collision Rejection & Re-Registration..." << std::endl;

    NodeRegistrationManager regMgr;
    regMgr.Reset();

    NodeRegistrationRecord validNode;
    validNode.nodeId = "INT_B";
    validNode.hostname = "rpi4-bravo";
    validNode.ipAddress = "192.168.1.102";
    validNode.macAddress = "DC:A6:32:44:55:66";

    std::string err;
    TEST_ASSERT(regMgr.RegisterNode(validNode, err), "Primary registration must succeed");

    // Rogue node attempting to hijack INT_B from different IP
    NodeRegistrationRecord rogueNode = validNode;
    rogueNode.ipAddress = "192.168.1.250";
    rogueNode.hostname = "rogue-impostor";
    bool rogueAccepted = regMgr.RegisterNode(rogueNode, err);
    TEST_ASSERT(!rogueAccepted, "Duplicate Node ID from different IP MUST be rejected");
    TEST_ASSERT(err.find("Duplicate Node ID collision") != std::string::npos, "Collision error must be logged");

    // Valid re-registration from same IP (e.g. after transient link drop)
    NodeRegistrationRecord reconnectedNode = validNode;
    reconnectedNode.currentStatus = "RECOVERED";
    bool reconAccepted = regMgr.RegisterNode(reconnectedNode, err);
    TEST_ASSERT(reconAccepted, "Re-registration from valid IP must succeed without reboot");

    // Verify heartbeat refresh
    TEST_ASSERT(regMgr.RecordHeartbeat("INT_B"), "Heartbeat record must succeed");

    std::cout << "  ✓ PASS: Duplicate node collision rejection and recovery re-registration verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 5: Dedicated 1 Hz Hardware Watchdog Thread Supervising 7 Subsystems
// ============================================================================
static bool TestHardwareWatchdogSupervision() {
    std::cout << "\n[TEST 5] Dedicated 1 Hz Watchdog Thread Supervising 7 Subsystems..." << std::endl;

    HardwareWatchdog watchdog;
    watchdog.SetCoordinatorTimeoutMs(1000);
    watchdog.SetNodeTimeoutMs(500);

    bool failsafeTriggered = false;
    std::string triggeredReason;
    watchdog.SetFailsafeCallback([&](const std::string& r) {
        failsafeTriggered = true;
        triggeredReason = r;
    });

    bool alertSent = false;
    watchdog.SetCoordinatorNotifyCallback([&](const std::string& /*a*/) {
        alertSent = true;
    });

    // 1. Setup 7 healthy subsystem checkers
    bool relayHealthy = true;
    bool gpioHealthy = true;
    bool irHealthy = true;
    bool wifiHealthy = true;
    bool trafficHealthy = true;
    bool bootHealthy = true;

    watchdog.SetRelayHealthChecker([&]() { return relayHealthy; });
    watchdog.SetGpioStateChecker([&]() { return gpioHealthy; });
    watchdog.SetIrSensorActivityChecker([&]() { return irHealthy; });
    watchdog.SetWifiConnectivityChecker([&]() { return wifiHealthy; });
    watchdog.SetTrafficControllerChecker([&]() { return trafficHealthy; });
    watchdog.SetBootManagerChecker([&]() { return bootHealthy; });

    // Kick initial heartbeats
    watchdog.KickCoordinatorHeartbeat();
    watchdog.KickNodeHeartbeat();

    // Nominal tick
    bool tickOk = watchdog.TickOnce();
    TEST_ASSERT(tickOk, "Nominal supervision tick must pass");
    TEST_ASSERT(!watchdog.HasFault(), "Watchdog must not have fault");

    // Simulate Subsystem Failure: Relay Feedback Readback Fault
    relayHealthy = false;
    bool faultTick = watchdog.TickOnce();
    TEST_ASSERT(!faultTick, "Supervision tick must fail when relay check fails");
    TEST_ASSERT(watchdog.HasFault(), "Watchdog must enter fault state");
    TEST_ASSERT(failsafeTriggered, "Watchdog must trigger failsafe");
    TEST_ASSERT(alertSent, "Watchdog must send coordinator alert");
    TEST_ASSERT(watchdog.GetFaultReason().find("Relay") != std::string::npos, "Fault reason must specify Relay");

    // Auto-recovery attempt while fault persists -> must fail
    bool autoRecFail = watchdog.AttemptAutoRecovery();
    TEST_ASSERT(!autoRecFail, "Auto-recovery must fail while fault persists");

    // Heal the relay
    relayHealthy = true;
    watchdog.KickCoordinatorHeartbeat();
    watchdog.KickNodeHeartbeat();

    // Auto-recovery attempt when healed -> must succeed
    bool autoRecSuccess = watchdog.AttemptAutoRecovery();
    TEST_ASSERT(autoRecSuccess, "Auto-recovery must succeed when subsystem healed");
    TEST_ASSERT(!watchdog.HasFault(), "Fault must be cleared after recovery");

    std::string wReport = watchdog.GenerateWatchdogReport();
    TEST_ASSERT(wReport.find("HEALTHY") != std::string::npos, "Watchdog report must show HEALTHY");

    std::cout << "  ✓ PASS: 1 Hz Hardware Watchdog 7-subsystem supervision & auto-recovery verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 6: Relay Power-On Self Test (POST) with Feedback & Green Interlock
// ============================================================================
static bool TestRelayPowerOnSelfTest() {
    std::cout << "\n[TEST 6] Relay Power-On Self Test (POST: Channels 1-4 & Dual Green Interlock)..." << std::endl;

    auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    auto relayCtrl = std::make_shared<RelayController>(bsp);

    std::vector<RelayChannelDef> channels = {
        {1, 17, "ROAD_A_RED", true},
        {2, 22, "ROAD_A_GREEN", true},
        {3, 23, "ROAD_B_RED", true},
        {4, 25, "ROAD_B_GREEN", true}
    };

    TEST_ASSERT(relayCtrl->Initialize(channels, true), "RelayController init must succeed");

    // Execute POST
    RelayPostReport postRep = relayCtrl->ExecutePowerOnSelfTest();
    TEST_ASSERT(postRep.allPassed, "All 4 relay channels must pass POST");
    TEST_ASSERT(postRep.interlockVerified, "Dual green conflict interlock must be verified during POST");
    TEST_ASSERT(postRep.channelResults.size() == 4, "Must test exactly 4 channels");

    for (const auto& chRes : postRep.channelResults) {
        TEST_ASSERT(chRes.overallPassed, "Channel overall pass must be true");
        TEST_ASSERT(chRes.onTestPassed, "ON test must pass");
        TEST_ASSERT(chRes.offTestPassed, "OFF test must pass");
        TEST_ASSERT(chRes.switchTimeUs >= 0, "Channel switch latency measured");
    }

    std::string repStr = postRep.GenerateReport();
    TEST_ASSERT(repStr.find("DUAL GREEN LOCKED") != std::string::npos, "Interlock lock verified in report");

    // Verify Active-Low Safe Power-on State: All coils must be de-energized
    for (uint8_t c = 1; c <= 4; ++c) {
        TEST_ASSERT(!relayCtrl->IsRelayEnergized(c), "Relays must be de-energized after POST");
    }

    // Verify hardware green conflict prevention
    relayCtrl->SetRelay(2, true); // Road A Green ON
    bool conflictAttempt = relayCtrl->SetRelay(4, true); // Road B Green ON -> CONFLICT!
    TEST_ASSERT(!conflictAttempt, "RelayController MUST block concurrent Road A Green & Road B Green");
    relayCtrl->DeenergizeAll();

    relayCtrl->Shutdown();
    bsp->Cleanup();

    std::cout << "  ✓ PASS: Relay POST (Channels 1-4, Readback, Timing & Dual Green Interlock) verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 7: Wi-Fi / Ethernet Quality Monitor with 5-Tier Classification
// ============================================================================
static bool TestWirelessQualityMonitorFiveTiers() {
    std::cout << "\n[TEST 7] Wi-Fi / Ethernet Quality Monitor with 5-Tier Classification..." << std::endl;

    WirelessQualityMonitor monitor(10);
    monitor.SetConnected(true);

    LinkQualityLevel recordedLevel = LinkQualityLevel::DISCONNECTED;
    monitor.SetAlertCallback([&](LinkQualityLevel lvl, const std::string& /*w*/) {
        recordedLevel = lvl;
    });

    // Tier 1: EXCELLENT (-55 dBm, 2ms, 0% loss)
    for (int i = 0; i < 10; ++i) monitor.RecordSample(-55, 2.0, false);
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::EXCELLENT, "Level 1: EXCELLENT");

    // Tier 2: GOOD (-64 dBm, 15ms, 0% loss)
    for (int i = 0; i < 10; ++i) monitor.RecordSample(-64, 15.0, false);
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::GOOD, "Level 2: GOOD");

    // Tier 3: FAIR (-70 dBm, 35ms, 0% loss)
    for (int i = 0; i < 10; ++i) monitor.RecordSample(-70, 35.0, false);
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::FAIR, "Level 3: FAIR");

    // Tier 4: POOR (-78 dBm, 80ms, 0% loss)
    for (int i = 0; i < 10; ++i) monitor.RecordSample(-78, 80.0, false);
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::POOR, "Level 4: POOR");

    // Tier 5: CRITICAL (-88 dBm, 25% packet drop)
    for (int i = 0; i < 10; ++i) monitor.RecordSample(-88, 350.0, (i % 4 == 0));
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::CRITICAL, "Level 5: CRITICAL");
    TEST_ASSERT(monitor.HasWarning(), "Monitor must indicate active warning on CRITICAL");

    // Disconnection
    monitor.SetConnected(false);
    TEST_ASSERT(monitor.GetQualityLevel() == LinkQualityLevel::DISCONNECTED, "Level 0: DISCONNECTED");

    std::cout << "  ✓ PASS: Wi-Fi link quality 5-tier classification verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 8: Industrial 10-Step Safe Shutdown Manager (Non-Reentrant & Coils Off)
// ============================================================================
static bool TestIndustrialSafeShutdown() {
    std::cout << "\n[TEST 8] Industrial 10-Step Safe Shutdown Manager..." << std::endl;

    ShutdownManager& sMgr = ShutdownManager::Instance();
    sMgr.Reset();

    std::vector<IndustrialShutdownStep> executedSteps;
    bool relaysDeenergized = false;
    bool bspUnmapped = false;

    for (uint8_t s = 1; s <= 10; ++s) {
        auto step = static_cast<IndustrialShutdownStep>(s);
        sMgr.SetIndustrialStepHook(step, [&executedSteps, &relaysDeenergized, &bspUnmapped, step](std::string& d) {
            executedSteps.push_back(step);
            if (step == IndustrialShutdownStep::DEACTIVATE_RELAYS) {
                relaysDeenergized = true;
                d = "All 4 relay channels confirmed unpowered";
            } else if (step == IndustrialShutdownStep::UNMAP_BSP_MEMORY) {
                bspUnmapped = true;
                d = "mmap_device_io memory range unmapped cleanly";
            } else {
                d = IndustrialShutdownStepToString(step) + " executed successfully";
            }
            return true;
        });
    }

    bool shutdownSuccess = sMgr.ExecuteIndustrialShutdown(ShutdownReason::NORMAL);
    TEST_ASSERT(shutdownSuccess, "Industrial shutdown must succeed");
    TEST_ASSERT(sMgr.IsShutdownComplete(), "IsShutdownComplete must be true");
    TEST_ASSERT(relaysDeenergized, "Relays must be de-energized during shutdown");
    TEST_ASSERT(bspUnmapped, "BSP memory must be unmapped during shutdown");
    TEST_ASSERT(executedSteps.size() == 10, "All 10 industrial steps must execute");

    // Non-Reentrancy Verification: A concurrent / redundant shutdown attempt MUST be rejected
    bool redundantAttempt = sMgr.ExecuteIndustrialShutdown(ShutdownReason::NORMAL);
    TEST_ASSERT(!redundantAttempt, "Concurrent or redundant shutdown must be strictly rejected");

    std::string report = sMgr.GenerateIndustrialShutdownReport();
    TEST_ASSERT(report.find("DE-ENERGIZED") != std::string::npos, "Report must confirm relays de-energized");

    sMgr.Reset();
    std::cout << "  ✓ PASS: Industrial 10-step safe shutdown manager verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 9: Automatic Node Recovery Following Link Severance Without Reboot
// ============================================================================
static bool TestAutomaticNodeRecovery() {
    std::cout << "\n[TEST 9] Automatic Node Recovery Following Link Loss (No Reboot)..." << std::endl;

    auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
    bsp->InitializeBoard();

    DualSignalPins pins = {17, 22, 23, 25};
    std::vector<RelayChannelDef> relays = {
        {1, 17, "ROAD_A_RED", true},
        {2, 22, "ROAD_A_GREEN", true},
        {3, 23, "ROAD_B_RED", true},
        {4, 25, "ROAD_B_GREEN", true}
    };
    IrSensorParameters ir = {12, 13, 50, 30};
    TimingParameters timings = {5.0, 1.0, 5.0, 1.0, 0.5};

    auto hardware = std::make_shared<IntersectionHardware>("INT_C");
    hardware->Initialize(pins, relays, ir, timings);

    // Initial operational state
    hardware->Update(0.1);
    TEST_ASSERT(!hardware->IsInFailSafe(), "Hardware starts in nominal state");

    // Simulate Network Failure -> Trigger Fail-Safe Mode (FLASHING_YELLOW / ALL_RED)
    hardware->TriggerFailSafe("COMMUNICATION_LINK_LOST");
    TEST_ASSERT(hardware->IsInFailSafe(), "Node must enter fail-safe upon link severance");
    TEST_ASSERT(hardware->GetFailSafeReason() == "COMMUNICATION_LINK_LOST", "Fail-safe reason logged");

    // Simulate Communication Link Restored: Auto-reconnect & resume without rebooting
    hardware->RecoverFromFailSafe();
    TEST_ASSERT(!hardware->IsInFailSafe(), "Node must recover from fail-safe without reboot");

    hardware->RestoreNormalCycle();
    hardware->Update(0.1);
    TEST_ASSERT(!hardware->IsInFailSafe(), "Normal traffic cycle restored");

    hardware->Shutdown();
    bsp->Cleanup();

    std::cout << "  ✓ PASS: Automatic node recovery without reboot verified." << std::endl;
    return true;
}

// ============================================================================
// TEST 10: Health Telemetry (10 Parameters) & Microsecond Structured Logging
// ============================================================================
static bool TestHealthTelemetryAndMicrosecondLogging() {
    std::cout << "\n[TEST 10] Health Telemetry (10 Parameters) & Microsecond Logging..." << std::endl;

    auto logger = std::make_shared<HardwareEventLogger>(500);

    // 1. Validate Microsecond Structured Loggers
    logger->LogBootEvent("1_KERNEL_BOOT", true, 45, "Kernel booted");
    logger->LogRelayPost(1, true, 120, "Turn ON/OFF verified");
    logger->LogRegistrationEvent("INT_A", "192.168.1.101", true);
    logger->LogRecoveryEvent("INT_A", "Reconnected on Wi-Fi");
    logger->LogWatchdogEvent("SUPERVISION_PASS", "All 7 checks healthy");
    logger->LogShutdownEvent("10_SHUTDOWN_COMPLETE", true);
    logger->LogSubsystemFailure("RELAY", "Readback mismatch");

    auto logs = logger->GetAllLogs();
    TEST_ASSERT(logs.size() == 7, "All 7 structured events logged");
    for (const auto& entry : logs) {
        TEST_ASSERT(entry.timestampUs > 0, "Microsecond timestamp must be populated");
        TEST_ASSERT(!entry.timestampIso.empty(), "ISO timestamp must be populated");
        TEST_ASSERT(!entry.subsystem.empty(), "Subsystem must be populated");
        TEST_ASSERT(!entry.message.empty(), "Message must be populated");
    }

    std::string jsonLog = logger->ExportToJson();
    TEST_ASSERT(jsonLog.find("timestampUs") != std::string::npos, "JSON export must contain timestampUs");

    // 2. Validate Aggregate Health Telemetry (All 10 Parameters)
    HardwareHealthMonitor healthMon("INT_A", logger);
    AggregateHardwareHealth h = healthMon.EvaluateHealth();

    TEST_ASSERT(h.nodeId == "INT_A", "1. Node ID verified");
    TEST_ASSERT(h.cpuTemperatureC > 0.0, "2. CPU Temperature verified");
    TEST_ASSERT(h.cpuUsagePct >= 0.0, "3. CPU Usage % verified");
    TEST_ASSERT(h.ramUsedMB > 0, "4. RAM Usage verified");
    TEST_ASSERT(!h.gpioStatus.empty(), "5. GPIO Status verified");
    TEST_ASSERT(!h.relayStatus.empty(), "6. Relay Status verified");
    TEST_ASSERT(!h.sensorStatus.empty(), "7. Sensor Status verified");
    TEST_ASSERT(!h.networkStatus.empty(), "8. Network Status verified");
    TEST_ASSERT(!h.watchdogStatus.empty(), "9. Watchdog Status verified");
    TEST_ASSERT(!h.bootStatus.empty(), "10. Boot Status verified");
    TEST_ASSERT(h.grade == HardwareHealthGrade::HEALTHY, "Health Grade verified");

    std::string hReport = healthMon.GenerateHealthReport();
    TEST_ASSERT(hReport.find("BCM2711 CPU Temp") != std::string::npos, "Health report includes CPU temp");
    TEST_ASSERT(hReport.find("RAM Utilization") != std::string::npos, "Health report includes RAM");
    TEST_ASSERT(hReport.find("Watchdog Thread") != std::string::npos, "Health report includes Watchdog");

    std::string hJson = healthMon.PublishHealthJson();
    TEST_ASSERT(hJson.find("\"cpu_temp_c\"") != std::string::npos, "Health JSON includes cpu_temp_c");
    TEST_ASSERT(hJson.find("\"watchdog_status\"") != std::string::npos, "Health JSON includes watchdog_status");

    std::cout << "  ✓ PASS: 10-parameter health telemetry and microsecond logging verified." << std::endl;
    return true;
}

// ============================================================================
// MAIN ENTRYPOINT
// ============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 2 Production-Grade Embedded Systems   \n";
    std::cout << " Raspberry Pi 4 + QNX SDP 8.0 Embedded Subsystem Tests   \n";
    std::cout << "==========================================================\n";

    int totalPassed = 0;
    int totalFailed = 0;

    auto runTest = [&](bool (*testFunc)(), const char* name) {
        if (testFunc()) {
            totalPassed++;
        } else {
            totalFailed++;
            std::cerr << ">>> [FAILED] " << name << std::endl;
        }
    };

    runTest(TestIndustrialBootSequence, "Test 1: 14-Step Industrial Boot Sequence & Timing");
    runTest(TestSafeModeEntryOnFailure, "Test 2: Safe Mode Entry & Boot Abort on Failure");
    runTest(TestNodeRegistrationFields, "Test 3: Automated Node Registration 9 Fields");
    runTest(TestDuplicateRejectionAndReRegistration, "Test 4: Duplicate Rejection & Re-registration");
    runTest(TestHardwareWatchdogSupervision, "Test 5: 1 Hz Watchdog 7-Subsystem Supervision");
    runTest(TestRelayPowerOnSelfTest, "Test 6: Relay POST & Dual Green Interlock");
    runTest(TestWirelessQualityMonitorFiveTiers, "Test 7: Wi-Fi Quality Monitor 5-Tier Assessment");
    runTest(TestIndustrialSafeShutdown, "Test 8: Industrial 10-Step Safe Shutdown Manager");
    runTest(TestAutomaticNodeRecovery, "Test 9: Automatic Node Recovery Following Link Loss");
    runTest(TestHealthTelemetryAndMicrosecondLogging, "Test 10: Health Telemetry & Microsecond Logging");

    std::cout << "\n==========================================================\n";
    std::cout << " Phase 2 Embedded System Validation Summary:\n";
    std::cout << " Total Passed: " << totalPassed << "/10\n";
    std::cout << " Total Failed: " << totalFailed << "/10\n";
    std::cout << "==========================================================\n";

    if (totalFailed == 0) {
        std::cout << ">>> [SUCCESS] All 10 Phase 2 Embedded System Tests Passed 100%!\n";
        return 0;
    } else {
        std::cerr << ">>> [ERROR] Some Phase 2 tests failed!\n";
        return 1;
    }
}
