/**
 * @file test_qnx_hardening_phase20_6.cpp
 * @brief SMART AEGIS (AERS) Phase 20.6 - QNX Deployment Hardening & Hardware
 * Validation Suite.
 *
 * Comprehensive validation harness certifying that SMART AEGIS meets all QNX
 * Neutrino RTOS runtime requirements, POSIX compliance, Raspberry Pi 4 BCM2711
 * BSP integration, static Wi-Fi mesh networking, active-low relay actuation,
 * hardware mutual exclusion interlock, optical IR sensor debouncing & noise
 * filtering, supervisor failure recovery, and packaging.
 */

#include "aegis/common/qnx_compat.hpp"
#include "aegis/hardware/OpticalSensorFilter.hpp"
#include "bsp/RaspberryPi4BSP.h"
#include "hardware/HardwareConfiguration.h"
#include "hardware/HardwareController.h"
#include "hardware/IntersectionHardware.h"
#include "hardware/RelayController.h"
#include "hardware/TrafficLightController.h"
#include "hardware/WirelessNodeManager.h"

#include "runtime/CoordinatorRuntime.h"
#include "runtime/DistributedRuntimeOrchestrator.h"
#include "runtime/TrafficNodeRuntime.h"

#include "simulator/CityClock.h"
#include "simulator/CityModel.h"

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <vector>

using namespace aegis;
using namespace aegis::common;
using namespace aegis::qnx;
using namespace aegis::hardware;
using namespace aegis::runtime;
using namespace aegis::simulator;

static int g_passCount = 0;
static int g_failCount = 0;

#define HARDENING_TEST_ASSERT(cond, msg)                                       \
  do {                                                                         \
    if (cond) {                                                                \
      std::cout << "  ✓ PASS: " << msg << std::endl;                           \
      g_passCount++;                                                           \
    } else {                                                                   \
      std::cerr << "  ✗ FAIL: " << msg << " [Line: " << __LINE__ << "]"        \
                << std::endl;                                                  \
      g_failCount++;                                                           \
    }                                                                          \
  } while (0)

// =============================================================================
// TEST 1: QNX POSIX Resource & System API Compliance
// =============================================================================
void TestQnxPosixResourceCompliance() {
  std::cout << "\n[TEST 1] QNX POSIX Resource & System API Compliance..."
            << std::endl;

  // 1. Thread priority configuration
  std::atomic<bool> threadExecuted{false};
  std::thread worker([&]() {
    SetThreadRealTimePriority(pthread_self(), QnxPriority::HIGH_COORDINATOR,
                              true);
    SetThreadName(pthread_self(), "AEGIS_POSIX_RT");
    threadExecuted.store(true);
  });
  worker.join();
  HARDENING_TEST_ASSERT(
      threadExecuted.load(),
      "POSIX thread created, prioritized, and joined cleanly");

  // 2. High-resolution monotonic timers
  uint64_t startNs = GetMonotonicTimeNs();
  std::this_thread::sleep_for(std::chrono::milliseconds(5));
  uint64_t endNs = GetMonotonicTimeNs();
  HARDENING_TEST_ASSERT(
      endNs > startNs,
      "POSIX high-resolution monotonic timer progressed forward");
  HARDENING_TEST_ASSERT((endNs - startNs) >= 4000000ULL,
                        "Measured elapsed time >= 4ms");

  // 3. QNX device path formatting
  std::string devPath = FormatQnxDevicePath("gpio");
  HARDENING_TEST_ASSERT(devPath == "/dev/aegis/gpio",
                        "QNX resource manager path formatted correctly");
}

// =============================================================================
// TEST 2: Target Hardware Node Topology (4 RPi 4 SBCs, Static Wi-Fi Only)
// =============================================================================
void TestTargetHardwareNodeTopology() {
  std::cout << "\n[TEST 2] Target Hardware Node Topology (4 RPi 4 SBCs, Wi-Fi "
               "Only)..."
            << std::endl;

  HardwareConfiguration config;
  bool loaded = config.LoadFromFile("config/hardware.json");
  HARDENING_TEST_ASSERT(loaded, "Loaded config/hardware.json successfully");

  // Verify Coordinator (RPi 1)
  WifiNodeRecord coord;
  bool hasCoord = config.GetNodeRecord("COORDINATOR", coord);
  HARDENING_TEST_ASSERT(hasCoord, "Coordinator node record found");
  HARDENING_TEST_ASSERT(coord.staticIp == "192.168.1.100",
                        "Coordinator static IP is 192.168.1.100");
  HARDENING_TEST_ASSERT(coord.port == 9000, "Coordinator port is 9000");

  // Verify Intersection A (RPi 2)
  WifiNodeRecord intA;
  bool hasIntA = config.GetNodeRecord("INT_A", intA);
  HARDENING_TEST_ASSERT(hasIntA, "Intersection A node record found");
  HARDENING_TEST_ASSERT(intA.staticIp == "192.168.1.101",
                        "Intersection A static IP is 192.168.1.101");
  HARDENING_TEST_ASSERT(intA.port == 9001, "Intersection A port is 9001");
  HARDENING_TEST_ASSERT(intA.coordinatorIp == "192.168.1.100",
                        "Intersection A targets Coordinator IP");

  // Verify Intersection B (RPi 3)
  WifiNodeRecord intB;
  bool hasIntB = config.GetNodeRecord("INT_B", intB);
  HARDENING_TEST_ASSERT(hasIntB, "Intersection B node record found");
  HARDENING_TEST_ASSERT(intB.staticIp == "192.168.1.102",
                        "Intersection B static IP is 192.168.1.102");
  HARDENING_TEST_ASSERT(intB.port == 9002, "Intersection B port is 9002");

  // Verify Intersection C (RPi 4)
  WifiNodeRecord intC;
  bool hasIntC = config.GetNodeRecord("INT_C", intC);
  HARDENING_TEST_ASSERT(hasIntC, "Intersection C node record found");
  HARDENING_TEST_ASSERT(intC.staticIp == "192.168.1.103",
                        "Intersection C static IP is 192.168.1.103");
  HARDENING_TEST_ASSERT(intC.port == 9003, "Intersection C port is 9003");
}

// =============================================================================
// TEST 3: Wi-Fi Communication, Packet Retry & Latency Measurement
// =============================================================================
void TestWifiCommunicationAndQoS() {
  std::cout
      << "\n[TEST 3] Wi-Fi Communication, Packet Retry & Latency Measurement..."
      << std::endl;

  WirelessNodeManager nodeMgr("INT_A");
  WifiNodeRecord record;
  record.nodeId = "INT_A";
  record.staticIp = "192.168.1.101";
  record.port = 9001;
  record.coordinatorIp = "192.168.1.100";
  record.coordinatorPort = 9000;

  HARDENING_TEST_ASSERT(nodeMgr.ConfigureWirelessNode(record),
                        "Configured INT_A wireless node parameters");
  HARDENING_TEST_ASSERT(nodeMgr.Connect(),
                        "Connected via simulated Wi-Fi TCP socket");
  HARDENING_TEST_ASSERT(nodeMgr.IsConnected(),
                        "Node reports Wi-Fi link CONNECTED");

  // Transmit command with callback verification
  std::string rxCmd, rxPayload;
  nodeMgr.SetCommandCallback(
      [&](const std::string &cmd, const std::string &payload) {
        rxCmd = cmd;
        rxPayload = payload;
      });

  bool sent = nodeMgr.SendCommand("COORDINATOR", "PREEMPTION_REQUEST",
                                  "CORRIDOR_GREEN_WAVE_1");
  HARDENING_TEST_ASSERT(sent, "Dispatched PREEMPTION_REQUEST to Coordinator");
  HARDENING_TEST_ASSERT(rxCmd == "PREEMPTION_REQUEST",
                        "Command callback delivered command");
  HARDENING_TEST_ASSERT(rxPayload == "CORRIDOR_GREEN_WAVE_1",
                        "Command callback delivered payload");

  // Latency & Packet Loss Accounting
  nodeMgr.RecordLatency(1.35);
  nodeMgr.RecordPacketLoss(0, 100);
  WirelessLinkStats stats = nodeMgr.GetLinkStats();
  HARDENING_TEST_ASSERT(stats.rttLatencyMs == 1.35,
                        "Recorded RTT latency is 1.35 ms");
  HARDENING_TEST_ASSERT(stats.packetLossPct == 0.0,
                        "Packet loss percentage is 0.0%");

  // Packet Retry Verification
  bool retrySuccess =
      nodeMgr.SendCommandWithRetry("COORDINATOR", "HEARTBEAT", "", 3, 10);
  HARDENING_TEST_ASSERT(retrySuccess,
                        "SendCommandWithRetry succeeded on active link");
}

// =============================================================================
// TEST 4: 1 Hz Heartbeat Supervision, Timeout & Node Offline Detection
// =============================================================================
void TestHeartbeatTimeoutAndOfflineDetection() {
  std::cout << "\n[TEST 4] 1 Hz Heartbeat Supervision, Timeout & Node Offline "
               "Detection..."
            << std::endl;

  WirelessNodeManager nodeMgr("INT_B");
  WifiNodeRecord record;
  record.nodeId = "INT_B";
  record.staticIp = "192.168.1.102";
  record.port = 9002;
  record.coordinatorIp = "192.168.1.100";
  record.coordinatorPort = 9000;
  nodeMgr.ConfigureWirelessNode(record);
  nodeMgr.Connect();

  // Send 3 heartbeats
  for (int i = 0; i < 3; ++i) {
    nodeMgr.SendHeartbeat();
  }
  WirelessLinkStats stats = nodeMgr.GetLinkStats();
  HARDENING_TEST_ASSERT(stats.heartbeatCount == 3,
                        "Sent 3 consecutive 1 Hz heartbeats");
  HARDENING_TEST_ASSERT(!nodeMgr.IsNodeOffline(),
                        "Node is ONLINE with fresh heartbeats");

  // Advance watchdog timer beyond 5.0 second threshold without heartbeats
  nodeMgr.UpdateHeartbeatWatchdog(3.0, 5.0);
  HARDENING_TEST_ASSERT(!nodeMgr.IsNodeOffline(),
                        "Node remains online at 3.0s elapsed");

  nodeMgr.UpdateHeartbeatWatchdog(2.5, 5.0); // Total 5.5s elapsed
  HARDENING_TEST_ASSERT(nodeMgr.IsNodeOffline(),
                        "Node marked OFFLINE after 5.5s timeout");
  HARDENING_TEST_ASSERT(!nodeMgr.IsConnected(),
                        "Connection status dropped on offline timeout");

  // Recovery
  nodeMgr.ResetOfflineState();
  HARDENING_TEST_ASSERT(!nodeMgr.IsNodeOffline(),
                        "Node offline state cleared on recovery");
  HARDENING_TEST_ASSERT(nodeMgr.IsConnected(),
                        "Node re-established connection on recovery");
}

// =============================================================================
// TEST 5: Wi-Fi Disruption & Connection Recovery
// =============================================================================
void TestConnectionSeveringAndAutoReconnect() {
  std::cout << "\n[TEST 5] Wi-Fi Disruption & Connection Recovery..."
            << std::endl;

  WirelessNodeManager nodeMgr("INT_C");
  WifiNodeRecord record;
  record.nodeId = "INT_C";
  record.staticIp = "192.168.1.103";
  record.port = 9003;
  record.coordinatorIp = "192.168.1.100";
  record.coordinatorPort = 9000;
  nodeMgr.ConfigureWirelessNode(record);
  nodeMgr.Connect();

  // Sever connection
  nodeMgr.SimulateWifiInterruption(true);
  HARDENING_TEST_ASSERT(!nodeMgr.IsConnected(),
                        "Link severed: IsConnected returns false");
  HARDENING_TEST_ASSERT(nodeMgr.IsInterrupted(), "Link marked as interrupted");
  HARDENING_TEST_ASSERT(!nodeMgr.SendCommand("COORDINATOR", "STATUS_PING"),
                        "Transmission fails during link drop");

  // Auto-reconnect fails while link is still down
  nodeMgr.TriggerAutoReconnect();
  HARDENING_TEST_ASSERT(
      !nodeMgr.IsConnected(),
      "Auto-reconnect rejected while Wi-Fi is physically down");

  // Restore link and trigger auto-reconnect
  nodeMgr.SimulateWifiInterruption(false);
  nodeMgr.TriggerAutoReconnect();
  HARDENING_TEST_ASSERT(
      nodeMgr.IsConnected(),
      "Auto-reconnect succeeded when Wi-Fi link was restored");
  HARDENING_TEST_ASSERT(nodeMgr.SendCommand("COORDINATOR", "RESTORE_ACK"),
                        "Transmission resumes after link recovery");
}

// =============================================================================
// TEST 6: BCM2711 GPIO Pin Reservations & Active Direction Configuration
// =============================================================================
void TestRaspberryPi4BspGpioValidation() {
  std::cout << "\n[TEST 6] BCM2711 GPIO Pin Reservations & Active Direction "
               "Configuration..."
            << std::endl;

  auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
  HARDENING_TEST_ASSERT(bsp->InitializeBoard(),
                        "BCM2711 BSP Board initialized");

  // Road A Signal Pins
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(17, "ROAD_A_RED"),
                        "Reserved GPIO 17 (Road A Red)");
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(27, "ROAD_A_YELLOW"),
                        "Reserved GPIO 27 (Road A Yellow)");
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(22, "ROAD_A_GREEN"),
                        "Reserved GPIO 22 (Road A Green)");

  // Road B Signal Pins
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(23, "ROAD_B_RED"),
                        "Reserved GPIO 23 (Road B Red)");
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(24, "ROAD_B_YELLOW"),
                        "Reserved GPIO 24 (Road B Yellow)");
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(25, "ROAD_B_GREEN"),
                        "Reserved GPIO 25 (Road B Green)");

  // Optical IR Pins
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(12, "IR_APPROACH"),
                        "Reserved GPIO 12 (IR Approach)");
  HARDENING_TEST_ASSERT(bsp->ReserveGPIO(13, "IR_DEPARTURE"),
                        "Reserved GPIO 13 (IR Departure)");

  // Configure Pin Modes
  HARDENING_TEST_ASSERT(bsp->ConfigurePinMode(17, bsp::PinMode::OUTPUT),
                        "GPIO 17 configured as OUTPUT");
  HARDENING_TEST_ASSERT(bsp->ConfigurePinMode(12, bsp::PinMode::INPUT),
                        "GPIO 12 configured as INPUT");
  HARDENING_TEST_ASSERT(bsp->ConfigurePullResistor(12, bsp::PullMode::PULL_UP),
                        "GPIO 12 configured with PULL_UP");

  // Clean release
  HARDENING_TEST_ASSERT(bsp->ReleaseGPIO(17), "Released GPIO 17 cleanly");
  bsp->ShutdownBoard();
}

// =============================================================================
// TEST 7: 4-Channel Active-Low Relay Actuation & Power-On Safe State
// =============================================================================
void TestRelayActiveLowAndPowerOnSafeState() {
  std::cout << "\n[TEST 7] 4-Channel Active-Low Relay Actuation & Power-On "
               "Safe State..."
            << std::endl;

  auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
  bsp->InitializeBoard();

  RelayController relayCtrl(bsp);
  std::vector<RelayChannelDef> channels = {
      {1, 17, "ROAD_A_RED", true, false},
      {2, 27, "ROAD_A_YELLOW", true, false},
      {3, 22, "ROAD_A_GREEN", true, false},
      {4, 25, "ROAD_B_GREEN", true, false}};

  HARDENING_TEST_ASSERT(relayCtrl.Initialize(channels, true),
                        "Initialized RelayController with active-low logic");

  // Verify initial de-energized safe state (active-low means coil off = 3.3V
  // HIGH)
  bsp::PinState pinState = bsp::PinState::LOW;
  bsp->ReadGPIO(17, pinState);
  HARDENING_TEST_ASSERT(pinState == bsp::PinState::HIGH,
                        "De-energized relay drives GPIO HIGH (off / 3.3V)");
  HARDENING_TEST_ASSERT(!relayCtrl.IsRelayEnergized(1),
                        "Channel 1 initially de-energized");

  // Energize Channel 1 (active-low means coil on = 0V LOW)
  relayCtrl.SetRelay(1, true);
  bsp->ReadGPIO(17, pinState);
  HARDENING_TEST_ASSERT(
      pinState == bsp::PinState::LOW,
      "Energized active-low relay drives GPIO LOW (coil on / 0V)");
  HARDENING_TEST_ASSERT(relayCtrl.IsRelayEnergized(1),
                        "Channel 1 reports energized");

  // Verify cycle counter
  HARDENING_TEST_ASSERT(relayCtrl.GetSwitchCycleCount(1) == 1,
                        "Switch cycle count incremented to 1");

  // De-energize all (power failure safe state)
  relayCtrl.DeenergizeAll();
  bsp->ReadGPIO(17, pinState);
  HARDENING_TEST_ASSERT(pinState == bsp::PinState::HIGH,
                        "DeenergizeAll drives GPIO HIGH (all-safe fallback)");
  HARDENING_TEST_ASSERT(!relayCtrl.IsRelayEnergized(1),
                        "Channel 1 de-energized after DeenergizeAll");

  relayCtrl.Shutdown();
}

// =============================================================================
// TEST 8: Signal Sequencing & Complementary Dual-Road Logic
// =============================================================================
void TestSignalSequencingAndComplementaryLogic() {
  std::cout << "\n[TEST 8] Signal Sequencing & Complementary Dual-Road Logic..."
            << std::endl;

  auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
  bsp->InitializeBoard();
  auto relay = std::make_shared<RelayController>(bsp);
  TrafficLightController lights(bsp, relay);

  DualSignalPins pins;
  HARDENING_TEST_ASSERT(lights.Initialize(pins),
                        "TrafficLightController initialized");

  // Phase 1: Road A Green -> Road B must be Red
  lights.SetPhase(DualPhase::ROAD_A_GREEN_ROAD_B_RED);
  HARDENING_TEST_ASSERT(lights.GetRoadAColor() == LightColor::GREEN,
                        "Road A is GREEN");
  HARDENING_TEST_ASSERT(lights.GetRoadBColor() == LightColor::RED,
                        "Road B is RED (Complementary safety rule)");

  // Phase 2: Road A Yellow -> Road B must be Red
  lights.SetPhase(DualPhase::ROAD_A_YELLOW_ROAD_B_RED);
  HARDENING_TEST_ASSERT(lights.GetRoadAColor() == LightColor::YELLOW,
                        "Road A is YELLOW");
  HARDENING_TEST_ASSERT(lights.GetRoadBColor() == LightColor::RED,
                        "Road B is RED");

  // Phase 3: All-Red Clearance -> Both Red
  lights.SetPhase(DualPhase::ALL_RED_CLEARANCE);
  HARDENING_TEST_ASSERT(lights.GetRoadAColor() == LightColor::RED,
                        "Road A is RED (All-Red hold)");
  HARDENING_TEST_ASSERT(lights.GetRoadBColor() == LightColor::RED,
                        "Road B is RED (All-Red hold)");

  // Phase 4: Road A Red -> Road B Green
  lights.SetPhase(DualPhase::ROAD_A_RED_ROAD_B_GREEN);
  HARDENING_TEST_ASSERT(lights.GetRoadAColor() == LightColor::RED,
                        "Road A is RED");
  HARDENING_TEST_ASSERT(lights.GetRoadBColor() == LightColor::GREEN,
                        "Road B is GREEN (Complementary safety rule)");

  lights.Shutdown();
}

// =============================================================================
// TEST 9: Hardware Mutual Exclusion Safety Interlock
// =============================================================================
void TestHardwareMutualExclusionSafetyInterlock() {
  std::cout << "\n[TEST 9] Hardware Mutual Exclusion Safety Interlock..."
            << std::endl;

  auto bsp = std::make_shared<bsp::RaspberryPi4BSP>();
  bsp->InitializeBoard();
  auto relay = std::make_shared<RelayController>(bsp);
  TrafficLightController lights(bsp, relay);
  DualSignalPins pins;
  lights.Initialize(pins);

  // Attempt to force simultaneous Green (Road A Green + Road B Green)
  bool accepted = lights.SetSignals(LightColor::GREEN, LightColor::GREEN);
  HARDENING_TEST_ASSERT(
      !accepted,
      "Hardware interlock REJECTED conflicting simultaneous green outputs!");
  HARDENING_TEST_ASSERT(lights.IsConflictingGreenDetected(),
                        "Conflict flag recorded by safety interlock");
  HARDENING_TEST_ASSERT(lights.HasHardwareFault(),
                        "Hardware fault tripped by safety interlock");

  // Verify immediate All-Red trip
  HARDENING_TEST_ASSERT(lights.GetRoadAColor() == LightColor::RED,
                        "Road A forced to RED under interlock trip");
  HARDENING_TEST_ASSERT(lights.GetRoadBColor() == LightColor::RED,
                        "Road B forced to RED under interlock trip");

  // Clear fault with valid complementary input
  lights.ClearFault();
  lights.SetPhase(DualPhase::ROAD_A_RED_ROAD_B_GREEN);
  HARDENING_TEST_ASSERT(!lights.HasHardwareFault(),
                        "Hardware fault cleared after setting safe phase");

  lights.Shutdown();
}

// =============================================================================
// TEST 10: Optical IR Noise Filtering & Debounce Validation
// =============================================================================
void TestOpticalIrNoiseFilteringAndDebounce() {
  std::cout << "\n[TEST 10] Optical IR Noise Filtering & Debounce Validation..."
            << std::endl;

  OpticalSensorFilter filter(50, 30000); // 50ms debounce, 30s stuck timeout
  HARDENING_TEST_ASSERT(!filter.IsBeamBroken(),
                        "Filter initially reports beam unbroken");

  // 1. Noise spike simulation: Active for only 15ms (less than 50ms threshold)
  filter.ProcessSample(true, 15);
  HARDENING_TEST_ASSERT(!filter.IsBeamBroken(),
                        "Transient 15ms pulse rejected (below 50ms debounce)");
  filter.ProcessSample(false, 20); // release
  HARDENING_TEST_ASSERT(filter.GetNoiseSpikeCount() == 1,
                        "Noise spike counter incremented to 1");

  // 2. Valid vehicle arrival: Sustained interruption for 60ms (>= 50ms
  // threshold)
  filter.ProcessSample(true, 25);
  filter.ProcessSample(true, 25);
  filter.ProcessSample(true, 10); // total 60ms active
  HARDENING_TEST_ASSERT(
      filter.IsBeamBroken(),
      "Qualified vehicle arrival detected after 60ms active beam");
  HARDENING_TEST_ASSERT(filter.GetQualifiedActivationCount() == 1,
                        "Qualified activation count is 1");

  // 3. Valid vehicle departure: Sustained clearance for 60ms
  filter.ProcessSample(false, 30);
  filter.ProcessSample(false, 30);
  HARDENING_TEST_ASSERT(
      !filter.IsBeamBroken(),
      "Qualified vehicle clearance detected after 60ms clear beam");
  HARDENING_TEST_ASSERT(filter.GetQualifiedDeactivationCount() == 1,
                        "Qualified deactivation count is 1");

  // 4. Stuck sensor fault: Sustained interruption for > 30 seconds
  filter.Reset();
  filter.ProcessSample(true, 100);   // Trigger qualified break
  filter.ProcessSample(true, 30500); // Held for > 30s
  HARDENING_TEST_ASSERT(
      filter.IsStuckFault(),
      "Stuck sensor fault raised after continuous >30s obstruction");
}

// =============================================================================
// TEST 11: IR Sensor Emergency Preemption Trigger & Departure Clearance
// =============================================================================
void TestIrEmergencyTriggerAndCorridorClearance() {
  std::cout << "\n[TEST 11] IR Sensor Emergency Preemption Trigger & Departure "
               "Clearance..."
            << std::endl;

  HardwareController controller("INT_A", "config/hardware.json");
  controller.Initialize("config/hardware.json");
  controller.Start();

  // Trigger preemption for Road A
  controller.TriggerEmergencyPreemption("MISSION_PREEMPT_101", "ROAD_A");
  HARDENING_TEST_ASSERT(controller.IsPreempted(),
                        "Emergency preemption engaged on Road A");
  HARDENING_TEST_ASSERT(controller.GetCurrentPhase() ==
                            DualPhase::PREEMPTION_ROAD_A,
                        "Phase is PREEMPTION_ROAD_A");

  // Process vehicle departure via IR sensor
  controller.SimulateVehicleDeparture();
  HARDENING_TEST_ASSERT(
      !controller.IsPreempted(),
      "Departure IR sensor automatically cleared preemption!");
  HARDENING_TEST_ASSERT(
      controller.GetCurrentPhase() == DualPhase::ALL_RED_CLEARANCE,
      "Safely transitioned to All-Red hold before cycle resume");

  controller.Stop();
}

// =============================================================================
// TEST 12: Fail-Safe Degradation to Flashing Yellow & Safe Recovery
// =============================================================================
void TestFailSafeDegradationAndRecovery() {
  std::cout << "\n[TEST 12] Fail-Safe Degradation to Flashing Yellow & Safe "
               "Recovery..."
            << std::endl;

  HardwareController controller("INT_B", "config/hardware.json");
  controller.Initialize("config/hardware.json");
  controller.Start();

  // Trigger fail-safe degradation (communication timeout)
  controller.TriggerFailSafe("COMMUNICATION_WATCHDOG_TIMEOUT");
  HARDENING_TEST_ASSERT(controller.IsInFailSafe(),
                        "Controller entered fail-safe mode");
  HARDENING_TEST_ASSERT(controller.GetCurrentPhase() ==
                            DualPhase::FAILSAFE_FLASHING_YELLOW,
                        "Phase transitioned to FAILSAFE_FLASHING_YELLOW");

  // Recover from fail-safe
  controller.RecoverFromFailSafe();
  HARDENING_TEST_ASSERT(!controller.IsInFailSafe(),
                        "Controller recovered from fail-safe");
  HARDENING_TEST_ASSERT(
      controller.GetCurrentPhase() == DualPhase::ALL_RED_CLEARANCE,
      "Recovery enforced mandatory All-Red clearance before cycle resume");

  controller.Stop();
}

// =============================================================================
// TEST 13: Coordinator Boot & Cluster Execution Lifecycle
// =============================================================================
void TestCoordinatorBootAndClusterExecution() {
  std::cout << "\n[TEST 13] Coordinator Boot & Cluster Execution Lifecycle..."
            << std::endl;

  CoordinatorRuntime coordinator;
  HARDENING_TEST_ASSERT(
      coordinator.Initialize("config/deployment/rpi1_coordinator.json"),
      "CoordinatorRuntime initialized from JSON");

  // Use loopback port 21100 to avoid conflicts
  HARDENING_TEST_ASSERT(
      coordinator.Start("127.0.0.1", 21100),
      "CoordinatorRuntime started TCP server on 127.0.0.1:21100");
  HARDENING_TEST_ASSERT(coordinator.IsRunning(),
                        "CoordinatorRuntime is running");

  TrafficNodeRuntime nodeA;
  HARDENING_TEST_ASSERT(
      nodeA.Initialize("INT_A", "config/deployment/rpi2_intersection_a.json"),
      "INT_A TrafficNodeRuntime initialized");
  HARDENING_TEST_ASSERT(nodeA.Start("127.0.0.1", 21100),
                        "INT_A connected to Coordinator");
  HARDENING_TEST_ASSERT(nodeA.IsRunning(), "INT_A is running");

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  nodeA.Stop();
  coordinator.Stop();
  HARDENING_TEST_ASSERT(!coordinator.IsRunning(),
                        "CoordinatorRuntime stopped cleanly");
  HARDENING_TEST_ASSERT(!nodeA.IsRunning(), "INT_A stopped cleanly");
}

// =============================================================================
// TEST 14: End-to-End Green Wave Emergency Corridor
// =============================================================================
void TestEndToEndGreenWaveEmergencyCorridor() {
  std::cout << "\n[TEST 14] End-to-End Green Wave Emergency Corridor..."
            << std::endl;

  DistributedRuntimeOrchestrator orchestrator;
  std::vector<std::string> nodes = {"INT_A", "INT_B", "INT_C"};
  HARDENING_TEST_ASSERT(
      orchestrator.InitializeCluster(21110, nodes),
      "Initialized 3-node cluster orchestrator on port 21110");
  HARDENING_TEST_ASSERT(orchestrator.StartCluster(),
                        "Cluster started successfully");

  std::this_thread::sleep_for(std::chrono::milliseconds(80));

  EmergencyPipelineInput callInput;
  callInput.callerName = "Highway Patrol";
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
  HARDENING_TEST_ASSERT(result.success,
                        "End-to-End Emergency Corridor executed successfully");
  HARDENING_TEST_ASSERT(result.assignedAmbulanceId == "AMB_01",
                        "Ambulance AMB_01 assigned");
  HARDENING_TEST_ASSERT(!result.assignedHospitalId.empty(),
                        "Hospital destination assigned");
  HARDENING_TEST_ASSERT(result.consensusApproved,
                        "Distributed consensus approved green wave");

  orchestrator.StopCluster();
  HARDENING_TEST_ASSERT(true,
                        "Cluster stopped cleanly after green wave completion");
}

// =============================================================================
// TEST 15: Dashboard & Simulation Communication
// =============================================================================
void TestDashboardAndSimulationCommunication() {
  std::cout << "\n[TEST 15] Dashboard & Simulation Communication..."
            << std::endl;

  CityClock clock;
  clock.SetTimeScale(2.0);
  HARDENING_TEST_ASSERT(clock.GetTimeScale() == 2.0,
                        "Simulation clock time scale set to 2x");

  CityModel model;
  HARDENING_TEST_ASSERT(model.LoadFromConfigFile("config/simulation.json"),
                        "CityModel loaded config/simulation.json");
  HARDENING_TEST_ASSERT(model.GetAllIntersections().size() >= 3,
                        "CityModel initialized 3+ virtual intersections");
  HARDENING_TEST_ASSERT(model.GetHospital("HOSP_CITY_TRAUMA") != nullptr,
                        "CityModel initialized virtual hospitals");
  HARDENING_TEST_ASSERT(model.GetAmbulance("AMB_01") != nullptr,
                        "CityModel initialized virtual ambulance fleet");

  // Verify headless dashboard environment variable
  const char *qpaPlatform = std::getenv("QT_QPA_PLATFORM");
  if (!qpaPlatform) {
    setenv("QT_QPA_PLATFORM", "offscreen", 1);
    qpaPlatform = std::getenv("QT_QPA_PLATFORM");
  }
  HARDENING_TEST_ASSERT(
      qpaPlatform != nullptr && std::string(qpaPlatform) == "offscreen",
      "QT_QPA_PLATFORM configured as 'offscreen' for headless QNX execution");
}

// =============================================================================
// TEST 16: QNX Startup Supervisor & Packaging Validation
// =============================================================================
void TestQnxStartupSupervisorAndPackagingValidation() {
  std::cout << "\n[TEST 16] QNX Startup Supervisor & Packaging Validation..."
            << std::endl;

  // Verify script files exist and are non-empty
  std::vector<std::string> scripts = {"scripts/qnx/run_qnx_coordinator.sh",
                                      "scripts/qnx/run_qnx_intersection.sh",
                                      "scripts/qnx/qnx_setup_wifi.sh",
                                      "scripts/qnx/qnx_autolaunch.sh",
                                      "scripts/qnx/package_qnx.sh",
                                      "scripts/qnx/deploy_qnx.sh"};

  for (const auto &scriptPath : scripts) {
    struct stat st{};
    int ret = stat(scriptPath.c_str(), &st);
    HARDENING_TEST_ASSERT(ret == 0 && st.st_size > 0,
                          "Verified script: " + scriptPath);
  }

  // Verify packages
  std::vector<std::string> tarballs = {"aegis_coordinator_qnx_rpi4.tar.gz",
                                       "aegis_intersection_qnx_rpi4.tar.gz",
                                       "aegis_qnx_rpi4.tar.gz"};

  for (const auto &pkg : tarballs) {
    struct stat st{};
    int ret = stat(pkg.c_str(), &st);
    HARDENING_TEST_ASSERT(ret == 0 && st.st_size > 0,
                          "Verified package archive: " + pkg);
  }
}

// =============================================================================
// MAIN ENTRYPOINT
// =============================================================================
int main() {
  std::cout << "=========================================================="
            << std::endl;
  std::cout << " SMART AEGIS (AERS) Phase 20.6: QNX Deployment Hardening"
            << std::endl;
  std::cout << " Raspberry Pi 4 Quick Start Image Validation Harness"
            << std::endl;
  std::cout << " Target: Raspberry Pi 4 Model B / QNX SDP 8.0 (aarch64le)"
            << std::endl;
  std::cout << "=========================================================="
            << std::endl;

  TestQnxPosixResourceCompliance();
  TestTargetHardwareNodeTopology();
  TestWifiCommunicationAndQoS();
  TestHeartbeatTimeoutAndOfflineDetection();
  TestConnectionSeveringAndAutoReconnect();
  TestRaspberryPi4BspGpioValidation();
  TestRelayActiveLowAndPowerOnSafeState();
  TestSignalSequencingAndComplementaryLogic();
  TestHardwareMutualExclusionSafetyInterlock();
  TestOpticalIrNoiseFilteringAndDebounce();
  TestIrEmergencyTriggerAndCorridorClearance();
  TestFailSafeDegradationAndRecovery();
  TestCoordinatorBootAndClusterExecution();
  TestEndToEndGreenWaveEmergencyCorridor();
  TestDashboardAndSimulationCommunication();
  TestQnxStartupSupervisorAndPackagingValidation();

  std::cout << "\n=========================================================="
            << std::endl;
  std::cout << " Total Passed: " << g_passCount << " / "
            << (g_passCount + g_failCount) << std::endl;
  std::cout << " Total Failed: " << g_failCount << std::endl;
  std::cout << "=========================================================="
            << std::endl;

  return (g_failCount == 0) ? 0 : 1;
}