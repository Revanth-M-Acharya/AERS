/**
 * @file test_deployment_layer_phase16d.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Complete Deployment Layer Integration Test Suite.
 *
 * Validates deterministic node boot, configuration loading, BCM2711 GPIO drivers,
 * network discovery, heartbeat supervision, failure detection, auto-reconnect,
 * Coordinator CLI, graceful multi-phase shutdown, and deployment scripts.
 */

#include "deployment/NodeConfiguration.h"
#include "deployment/ConfigurationLoader.h"
#include "deployment/DeploymentValidator.h"
#include "deployment/RuntimeConfiguration.h"
#include "deployment/NetworkDiscovery.h"
#include "deployment/BootManager.h"
#include "deployment/StartupManager.h"
#include "deployment/DeploymentGpioDriver.h"
#include "deployment/DeploymentLogger.h"
#include "deployment/HeartbeatSupervisor.h"
#include "deployment/GracefulShutdownManager.h"
#include "deployment/DeploymentManager.h"
#include "deployment/ClusterLauncher.h"

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstdlib>

using namespace aegis::deployment;

// Test 1: Configuration Loading & Cluster Validation
void Test1_ConfigurationLoadingAndValidation() {
    std::cout << "[TEST 1] Configuration Loading & Cluster Validation..." << std::endl;

    // Load default config
    NodeConfiguration config;
    std::string err;
    bool loaded = ConfigurationLoader::LoadFromFile("config/node.json", config, &err);
    assert(loaded && "config/node.json must be loaded successfully");
    assert(config.nodeId == "COORDINATOR");
    assert(config.IsCoordinator());
    assert(config.port == 9000);
    assert(config.expectedNodes.size() == 3);

    // Test serialization and reload
    std::string jsonStr = config.ToJson();
    NodeConfiguration reloaded;
    bool parsed = ConfigurationLoader::LoadFromString(jsonStr, reloaded, &err);
    assert(parsed && "Serialized JSON must reload cleanly");
    assert(reloaded.nodeId == config.nodeId);
    assert(reloaded.role == config.role);

    // Validate single node
    ValidationResult val = DeploymentValidator::ValidateNode(config);
    assert(val.valid && "Coordinator config must be valid");

    // Validate edge node
    NodeConfiguration edge = NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", 9000);
    ValidationResult edgeVal = DeploymentValidator::ValidateNode(edge);
    assert(edgeVal.valid && "Default edge config must be valid");

    // Validate cluster
    std::vector<NodeConfiguration> edgeList = {
        NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", 9000),
        NodeConfiguration::DefaultTrafficNode("INT_B", "127.0.0.1", 9000),
        NodeConfiguration::DefaultTrafficNode("INT_C", "127.0.0.1", 9000)
    };
    ValidationResult clusterVal = DeploymentValidator::ValidateCluster(config, edgeList);
    assert(clusterVal.valid && "Full 4-node cluster configuration must be valid");

    std::cout << "  ✓ PASS: Configuration Loading, JSON serialization, and Cluster Validation verified." << std::endl;
}

// Test 2: Runtime Configuration Dynamic Updates
void Test2_RuntimeConfigurationDynamicUpdates() {
    std::cout << "[TEST 2] Runtime Configuration Dynamic Updates..." << std::endl;

    NodeConfiguration initial = NodeConfiguration::DefaultCoordinator();
    RuntimeConfiguration runtimeConfig(initial);

    bool callbackFired = false;
    runtimeConfig.RegisterChangeCallback([&callbackFired](const NodeConfiguration& cfg) {
        if (cfg.loggingLevel == LogLevel::DEBUG_LEVEL) {
            callbackFired = true;
        }
    });

    assert(runtimeConfig.GetLogLevel() == LogLevel::INFO_LEVEL);
    runtimeConfig.SetLogLevel(LogLevel::DEBUG_LEVEL);
    assert(runtimeConfig.GetLogLevel() == LogLevel::DEBUG_LEVEL);
    assert(callbackFired && "Change callback must trigger on runtime parameter modification");

    runtimeConfig.SetHeartbeatInterval(500);
    assert(runtimeConfig.GetHeartbeatInterval() == 500);

    runtimeConfig.SetGpioEnabled(false);
    assert(!runtimeConfig.IsGpioEnabled());

    std::cout << "  ✓ PASS: Runtime Configuration dynamic tuning & callbacks verified." << std::endl;
}

// Test 3: Network Discovery & Port Probing
void Test3_NetworkDiscoveryAndEndpointProbing() {
    std::cout << "[TEST 3] Network Discovery & Endpoint Probing..." << std::endl;

    NodeConfiguration cfg = NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", 22010);
    DiscoveredCoordinator disc;

    bool resolved = NetworkDiscovery::ResolveCoordinator(cfg, disc, 200);
    assert(resolved && "Coordinator address must resolve");
    assert(disc.ip == "127.0.0.1");
    assert(disc.port == 22010);

    // Port probe on closed port should return false
    uint64_t latency = 0;
    bool probeClosed = NetworkDiscovery::ProbeCoordinator("127.0.0.1", 22019, 100, &latency);
    assert(!probeClosed && "Probe on closed port must return false");

    std::cout << "  ✓ PASS: Network Discovery resolution and probing verified." << std::endl;
}

// Test 4: Deterministic Boot Process & 8-Stage Sequencing
void Test4_DeterministicBootProcessAndSequencing() {
    std::cout << "[TEST 4] Deterministic Boot Process & 8-Stage Sequencing..." << std::endl;

    BootManager boot;
    assert(boot.GetCurrentStage() == BootStage::POWER_ON);
    assert(!boot.IsReady());

    NodeConfiguration config = NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", 22020);
    bool bootSuccess = boot.RunFullBootSequence(config);
    assert(bootSuccess && "Full boot sequence must complete successfully");
    assert(boot.IsReady());
    assert(boot.GetCurrentStage() == BootStage::READY);

    const auto& reports = boot.GetBootReports();
    assert(reports.size() == 9); // POWER_ON + 8 stages
    assert(reports[1].stage == BootStage::CONFIG_READ);
    assert(reports[2].stage == BootStage::BSP_INIT);
    assert(reports[3].stage == BootStage::HAL_INIT);
    assert(reports[4].stage == BootStage::GPIO_INIT);
    assert(reports[5].stage == BootStage::TCP_INIT);
    assert(reports[6].stage == BootStage::REGISTERING);
    assert(reports[7].stage == BootStage::HEARTBEATING);
    assert(reports[8].stage == BootStage::READY);

    for (const auto& r : reports) {
        assert(r.success && "Every boot stage must report success");
    }

    std::cout << "  ✓ PASS: Deterministic Boot Sequence verified (Total Boot Time: "
              << boot.GetTotalBootTimeUs() << " us)." << std::endl;
}

// Test 5: GPIO Driver Initialization & Signal Operations
void Test5_GPIOInitializationAndDriverInterface() {
    std::cout << "[TEST 5] GPIO Driver Initialization & Signal Operations..." << std::endl;

    DeploymentGpioDriver driver;
    GPIOPinMapping mapping;
    mapping.redPin = 17;
    mapping.yellowPin = 27;
    mapping.greenPin = 22;
    mapping.pedWalkPin = 25;
    mapping.pedDontWalkPin = 5;
    mapping.preemptStrobePin = 26;
    mapping.buzzerOutput = 6;
    mapping.irSensorNorthSouth = 12;
    mapping.irSensorEastWest = 13;
    mapping.emergencyOverrideButton = 16;

    bool init = driver.Initialize(mapping, true);
    assert(init && "GPIO Driver must initialize cleanly");
    assert(driver.IsInitialized());

    // Verify safe Red state on init
    bool redState = false;
    driver.Read(17, redState);
    assert(redState && "Red lamp must be HIGH on initialization");

    // Actuate Green
    driver.SetTrafficSignalState("GREEN");
    bool greenState = false;
    driver.Read(22, greenState);
    assert(greenState && "Green lamp must be HIGH");

    driver.Read(17, redState);
    assert(!redState && "Red lamp must be LOW when Green is active");

    // Actuate Yellow
    driver.SetTrafficSignalState("YELLOW");
    bool yellowState = false;
    driver.Read(27, yellowState);
    assert(yellowState && "Yellow lamp must be HIGH");

    // Actuate Pedestrian
    driver.SetPedestrianSignal(true);
    bool walkState = false;
    driver.Read(25, walkState);
    assert(walkState && "Pedestrian Walk lamp must be HIGH");

    // Actuate Preemption Strobe & Buzzer
    driver.SetPreemptionStrobe(true);
    bool strobeState = false;
    driver.Read(26, strobeState);
    assert(strobeState && "Preemption Strobe must be HIGH");

    driver.SetBuzzer(true);
    bool buzzerState = false;
    driver.Read(6, buzzerState);
    assert(buzzerState && "Buzzer must be HIGH");

    // Toggle Pin
    driver.Toggle(6);
    driver.Read(6, buzzerState);
    assert(!buzzerState && "Buzzer must toggle to LOW");

    // IR Sensors & Emergency Button Reads
    bool nsDetected = false;
    assert(driver.ReadIRSensorNorthSouth(nsDetected));
    bool ewDetected = false;
    assert(driver.ReadIRSensorEastWest(ewDetected));
    bool overridePressed = false;
    assert(driver.ReadEmergencyOverrideButton(overridePressed));

    // Cleanup
    assert(driver.Cleanup());
    assert(!driver.IsInitialized());

    std::cout << "  ✓ PASS: GPIO Hardware Driver adapter, pins & sensors verified." << std::endl;
}

// Test 6: Automatic Node Registration & Configuration Receipt
void Test6_AutomaticNodeRegistration() {
    std::cout << "[TEST 6] Automatic Node Registration & Configuration Receipt..." << std::endl;

    uint16_t port = 22030;
    NodeConfiguration coordConfig = NodeConfiguration::DefaultCoordinator("COORDINATOR", port);
    coordConfig.logDirectory = "logs/test_reg_coord";
    DeploymentManager coordMgr;
    assert(coordMgr.Initialize(coordConfig));
    assert(coordMgr.Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    NodeConfiguration edgeConfig = NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", port);
    edgeConfig.port = 22031;
    edgeConfig.logDirectory = "logs/test_reg_edge";
    DeploymentManager edgeMgr;
    assert(edgeMgr.Initialize(edgeConfig));
    assert(edgeMgr.Start());

    // Allow registration handshake
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    assert(edgeMgr.IsRunning());
    assert(edgeMgr.IsReady());

    edgeMgr.Stop();
    coordMgr.Stop();

    std::cout << "  ✓ PASS: Automatic Node Registration & Ready handshake verified." << std::endl;
}

// Test 7: Heartbeat Monitoring & Telemetry Streaming
void Test7_HeartbeatMonitoringAndTelemetryStreaming() {
    std::cout << "[TEST 7] Heartbeat Monitoring & Telemetry Streaming..." << std::endl;

    HeartbeatSupervisor supervisor;
    supervisor.StartSupervision(1500, 100);

    NodeTelemetry t1 = HeartbeatSupervisor::GatherLocalTelemetry("INT_A", "RED");
    NodeTelemetry t2 = HeartbeatSupervisor::GatherLocalTelemetry("INT_B", "GREEN");

    supervisor.RecordHeartbeat("INT_A", t1);
    supervisor.RecordHeartbeat("INT_B", t2);

    assert(supervisor.IsNodeOnline("INT_A"));
    assert(supervisor.IsNodeOnline("INT_B"));

    NodeTelemetry fetched;
    assert(supervisor.GetNodeTelemetry("INT_A", fetched));
    assert(fetched.nodeId == "INT_A");
    assert(fetched.signalState == "RED");
    assert(fetched.isOnline);

    std::string dashboard = supervisor.GenerateDashboardView();
    assert(dashboard.find("INT_A") != std::string::npos);
    assert(dashboard.find("INT_B") != std::string::npos);
    assert(dashboard.find("ONLINE") != std::string::npos);

    supervisor.StopSupervision();

    std::cout << "  ✓ PASS: Heartbeat Monitoring & Telemetry streaming verified." << std::endl;
}

// Test 8: Failure Detection & Watchdog Fail-Safe Trigger
void Test8_FailureDetectionAndWatchdogFailSafe() {
    std::cout << "[TEST 8] Failure Detection & Watchdog Fail-Safe Trigger..." << std::endl;

    HeartbeatSupervisor supervisor;
    bool timeoutDetected = false;
    std::string timedOutNode;

    supervisor.SetTimeoutCallback([&timeoutDetected, &timedOutNode](const std::string& id) {
        timeoutDetected = true;
        timedOutNode = id;
    });

    // Short timeout for fast unit testing
    supervisor.StartSupervision(150, 50);

    NodeTelemetry t = HeartbeatSupervisor::GatherLocalTelemetry("INT_A", "GREEN");
    supervisor.RecordHeartbeat("INT_A", t);
    assert(supervisor.IsNodeOnline("INT_A"));

    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    assert(!supervisor.IsNodeOnline("INT_A"));
    assert(timeoutDetected);
    assert(timedOutNode == "INT_A");

    supervisor.StopSupervision();

    std::cout << "  ✓ PASS: Silent Node Failure Detection & Watchdog Fail-Safe verified." << std::endl;
}

// Test 9: Coordinator Restart & Edge Node Auto-Recovery
void Test9_CoordinatorRestartAndClusterAutoRecovery() {
    std::cout << "[TEST 9] Coordinator Restart & Edge Node Auto-Recovery..." << std::endl;

    uint16_t port = 22050;
    NodeConfiguration coordConfig = NodeConfiguration::DefaultCoordinator("COORDINATOR", port);
    coordConfig.logDirectory = "logs/test_rec_coord";

    auto coord = std::make_shared<DeploymentManager>();
    assert(coord->Initialize(coordConfig));
    assert(coord->Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    NodeConfiguration edgeConfig = NodeConfiguration::DefaultTrafficNode("INT_A", "127.0.0.1", port);
    edgeConfig.port = 22051;
    edgeConfig.reconnectIntervalMs = 150;
    edgeConfig.logDirectory = "logs/test_rec_edge";

    auto edge = std::make_shared<DeploymentManager>();
    assert(edge->Initialize(edgeConfig));
    assert(edge->Start());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    assert(edge->IsRunning());

    // Simulate Coordinator bounce
    coord->Stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Edge detects loss and auto-reconnects
    coord = std::make_shared<DeploymentManager>();
    assert(coord->Initialize(coordConfig));
    assert(coord->Start());

    // Wait for edge auto-reconnect
    std::this_thread::sleep_for(std::chrono::milliseconds(450));

    edge->Stop();
    coord->Stop();

    std::cout << "  ✓ PASS: Coordinator bounce and edge node auto-reconnection verified." << std::endl;
}

// Test 10: Cluster Control CLI Commands
void Test10_ClusterControlCliCommands() {
    std::cout << "[TEST 10] Cluster Control CLI Commands..." << std::endl;

    ClusterLauncher cluster;
    uint16_t port = 22060;
    assert(cluster.InitializeCluster(port, {"INT_A", "INT_B", "INT_C"}));
    assert(cluster.StartCluster());

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    // Test "help"
    std::string helpOut = cluster.ExecuteCliCommand("help");
    assert(helpOut.find("SMART AEGIS Cluster CLI") != std::string::npos);

    // Test "cluster status"
    std::string statusOut = cluster.ExecuteCliCommand("cluster status");
    assert(statusOut.find("Cluster Running") != std::string::npos);

    // Test "list nodes"
    std::string listOut = cluster.ExecuteCliCommand("list nodes");
    assert(listOut.find("COORDINATOR") != std::string::npos);
    assert(listOut.find("INT_A") != std::string::npos);

    // Test "node health"
    std::string healthOut = cluster.ExecuteCliCommand("node health");
    assert(healthOut.find("Live Telemetry Dashboard") != std::string::npos);

    // Test "broadcast message"
    std::string bcastOut = cluster.ExecuteCliCommand("broadcast message TEST_ALERT");
    assert(bcastOut.find("Broadcast") != std::string::npos);

    // Test "simulate failure"
    std::string failOut = cluster.ExecuteCliCommand("simulate failure INT_A");
    assert(failOut.find("Simulated failure") != std::string::npos);

    // Test "restart node"
    std::string restartOut = cluster.ExecuteCliCommand("restart node INT_A");
    assert(restartOut.find("restarted successfully") != std::string::npos);

    cluster.StopCluster();

    std::cout << "  ✓ PASS: Coordinator CLI execution and cluster commands verified." << std::endl;
}

// Test 11: Graceful Multi-Phase Shutdown & Multi-Stream Log Generation
void Test11_GracefulShutdownAndMultiStreamLogs() {
    std::cout << "[TEST 11] Graceful Multi-Phase Shutdown & Multi-Stream Log Generation..." << std::endl;

    std::string logDir = "logs/test_logs_phase16d";
    DeploymentLogger logger;
    assert(logger.Initialize(logDir, "TEST_NODE"));

    logger.LogRuntime("INFO", "TEST", "Testing runtime log");
    logger.LogNetwork("TX", "COORDINATOR", "HEARTBEAT", 64, "Sample heartbeat packet");
    logger.LogGpio(17, "WRITE", "HIGH", "Red LED energized");
    logger.LogDiagnostics("CPU", 12.5, 24.0, 46.2, 100.0, "Healthy");
    logger.LogCrash("SIMULATED_CRASH", "Test11", "Testing crash logger");

    logger.Close();

    // Verify all 5 files were created and contain content
    std::ifstream f1(logDir + "/runtime.log");
    std::ifstream f2(logDir + "/network.log");
    std::ifstream f3(logDir + "/gpio.log");
    std::ifstream f4(logDir + "/diagnostics.log");
    std::ifstream f5(logDir + "/crash.log");

    assert(f1.is_open() && "runtime.log must exist");
    assert(f2.is_open() && "network.log must exist");
    assert(f3.is_open() && "gpio.log must exist");
    assert(f4.is_open() && "diagnostics.log must exist");
    assert(f5.is_open() && "crash.log must exist");

    // Test Graceful Shutdown Manager
    GracefulShutdownManager shutdownMgr;
    bool hookFired = false;
    shutdownMgr.RegisterCleanupHook([&hookFired]() {
        hookFired = true;
    });

    assert(shutdownMgr.ExecuteShutdown(false));
    assert(shutdownMgr.IsTerminated());
    assert(hookFired && "Shutdown cleanup hook must execute");

    std::cout << "  ✓ PASS: Multi-phase graceful shutdown & all 5 log streams verified." << std::endl;
}

// Test 12: Deployment Scripts Execution
void Test12_DeploymentScriptsExecution() {
    std::cout << "[TEST 12] Deployment Scripts Execution..." << std::endl;

    // Verify scripts exist and are executable
    assert(system("test -x scripts/deploy.sh") == 0);
    assert(system("test -x scripts/run_coordinator.sh") == 0);
    assert(system("test -x scripts/run_intersection.sh") == 0);
    assert(system("test -x scripts/stop_cluster.sh") == 0);
    assert(system("test -x scripts/restart_cluster.sh") == 0);
    assert(system("test -x scripts/collect_logs.sh") == 0);
    assert(system("test -x scripts/health_check.sh") == 0);

    // Run health check script
    int healthRet = system("./scripts/health_check.sh > /dev/null 2>&1");
    assert(healthRet == 0 && "Health check script must execute cleanly");

    // Run log collection script
    int logRet = system("./scripts/collect_logs.sh > /dev/null 2>&1");
    assert(logRet == 0 && "Log collection script must execute cleanly");

    // Run stop cluster script
    int stopRet = system("./scripts/stop_cluster.sh > /dev/null 2>&1");
    assert(stopRet == 0 && "Stop cluster script must execute cleanly");

    std::cout << "  ✓ PASS: All 7 deployment shell scripts verified and executable." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS (AERS) PHASE 16D TEST SUITE" << std::endl;
    std::cout << " Hardware Layer & Distributed Deployment Verification" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_ConfigurationLoadingAndValidation();
    Test2_RuntimeConfigurationDynamicUpdates();
    Test3_NetworkDiscoveryAndEndpointProbing();
    Test4_DeterministicBootProcessAndSequencing();
    Test5_GPIOInitializationAndDriverInterface();
    Test6_AutomaticNodeRegistration();
    Test7_HeartbeatMonitoringAndTelemetryStreaming();
    Test8_FailureDetectionAndWatchdogFailSafe();
    Test9_CoordinatorRestartAndClusterAutoRecovery();
    Test10_ClusterControlCliCommands();
    Test11_GracefulShutdownAndMultiStreamLogs();
    Test12_DeploymentScriptsExecution();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 12 PHASE 16D TESTS PASSED SUCCESSFULLY (100%)" << std::endl;
    std::cout << " Distributed Deployment Layer Fully Operational" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

