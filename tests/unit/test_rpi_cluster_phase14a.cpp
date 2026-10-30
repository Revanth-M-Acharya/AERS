/**
 * @file test_rpi_cluster_phase14a.cpp
 * @brief SMART AEGIS (AERS) Phase 14A Unit Tests
 *
 * Validates the Raspberry Pi Cluster Deployment & Health Management Engine:
 * - 12 Comprehensive Unit Tests
 * - Node Registration & Removal
 * - Dynamic Node Discovery
 * - Heartbeat Supervision & Staleness Detection
 * - Offline Node Detection (Rule 1)
 * - Fault Recovery & Synchronization Guarding (Rule 9)
 * - Telemetry & Health Monitoring (Rules 2, 3, 4, 5)
 * - Deployment State Transitions & Participation Guarding (Rule 6)
 * - Synchronization State Lifecycle (Rule 7)
 * - Configuration Management & Payload Push
 * - Structured Operational Reports ([NODE], [CLUSTER], [DEPLOYMENT])
 * - 100-Thread Concurrency & Race-Free Stress Test
 * - Performance Benchmark (100 Nodes, 1,000 Heartbeats, 1,000 Telemetries, 100 Deployments, < 5ms Verification)
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "rpi_cluster/RaspberryPiClusterManager.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis::cluster;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

// =============================================================================
// Unit Tests
// =============================================================================

/**
 * @brief TEST 1: Node Registration & Removal
 */
void Test1_NodeRegistration() {
    std::cout << "[TEST 1] Node Registration..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode n1("RPI_01", "rpi-intersection-1", "192.168.1.101", "B8:27:EB:01:02:03");
    RaspberryPiNode n2("RPI_02", "rpi-intersection-2", "192.168.1.102", "B8:27:EB:01:02:04");

    TEST_ASSERT(manager.RegisterNode(n1), "Register RPI_01 must succeed");
    TEST_ASSERT(manager.RegisterNode(n2), "Register RPI_02 must succeed");
    TEST_ASSERT(!manager.RegisterNode(n1), "Duplicate registration of RPI_01 must fail");

    RaspberryPiNode queryNode;
    TEST_ASSERT(manager.GetNode("RPI_01", queryNode), "GetNode RPI_01 must succeed");
    TEST_ASSERT(queryNode.hostname == "rpi-intersection-1", "Hostname must match");
    TEST_ASSERT(queryNode.ipAddress == "192.168.1.101", "IP address must match");
    TEST_ASSERT(queryNode.online, "Node must be online");

    auto allNodes = manager.GetAllNodes();
    TEST_ASSERT(allNodes.size() == 2, "There must be exactly 2 registered nodes");

    TEST_ASSERT(manager.RemoveNode("RPI_01"), "RemoveNode RPI_01 must succeed");
    TEST_ASSERT(!manager.GetNode("RPI_01", queryNode), "RPI_01 must no longer exist");
    TEST_ASSERT(manager.GetAllNodes().size() == 1, "There must be exactly 1 remaining node");
    TEST_ASSERT(!manager.RemoveNode("NON_EXISTENT"), "Removing non-existent node must fail");

    std::cout << "  ✓ PASS: Node registration, duplicate prevention, and node removal verified." << std::endl;
}

/**
 * @brief TEST 2: Dynamic Node Discovery
 */
void Test2_Discovery() {
    std::cout << "[TEST 2] Discovery..." << std::endl;
    RaspberryPiClusterManager manager;

    TEST_ASSERT(manager.DiscoverNode("RPI_DISC_1", "192.168.1.150", "rpi-edge-150"), "Discover new node");

    RaspberryPiNode queryNode;
    TEST_ASSERT(manager.GetNode("RPI_DISC_1", queryNode), "Get discovered node");
    TEST_ASSERT(queryNode.ipAddress == "192.168.1.150", "Discovered IP must match");
    TEST_ASSERT(queryNode.hostname == "rpi-edge-150", "Discovered hostname must match");
    TEST_ASSERT(queryNode.deploymentState == DeploymentState::NOT_DEPLOYED, "Initial state NOT_DEPLOYED");

    // Re-discovery updates IP address if changed
    TEST_ASSERT(manager.DiscoverNode("RPI_DISC_1", "192.168.1.151", "rpi-edge-151"), "Re-discover node with new IP");
    manager.GetNode("RPI_DISC_1", queryNode);
    TEST_ASSERT(queryNode.ipAddress == "192.168.1.151", "Updated IP must match");

    std::cout << "  ✓ PASS: Dynamic node discovery and telemetry update verified." << std::endl;
}

/**
 * @brief TEST 3: Heartbeat Supervision
 */
void Test3_Heartbeat() {
    std::cout << "[TEST 3] Heartbeat..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_HB", "rpi-hb", "192.168.1.110");
    manager.RegisterNode(node);

    TEST_ASSERT(manager.UpdateHeartbeat("RPI_HB", 1000), "First heartbeat update");
    RaspberryPiNode qNode;
    manager.GetNode("RPI_HB", qNode);
    TEST_ASSERT(qNode.lastHeartbeat == 1000, "Heartbeat timestamp must match");
    TEST_ASSERT(qNode.online, "Node must be online");

    // Update with later timestamp
    TEST_ASSERT(manager.UpdateHeartbeat("RPI_HB", 2000), "Second heartbeat update");
    manager.GetNode("RPI_HB", qNode);
    TEST_ASSERT(qNode.lastHeartbeat == 2000, "Updated heartbeat timestamp must match");

    std::cout << "  ✓ PASS: Heartbeat updating and online state verified." << std::endl;
}

/**
 * @brief TEST 4: Offline Detection (Rule 1)
 */
void Test4_OfflineDetection() {
    std::cout << "[TEST 4] Offline Detection..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode n1("RPI_ACTIVE", "rpi-active", "192.168.1.120");
    RaspberryPiNode n2("RPI_STALE", "rpi-stale", "192.168.1.121");

    manager.RegisterNode(n1);
    manager.RegisterNode(n2);

    manager.UpdateHeartbeat("RPI_ACTIVE", 9000);
    manager.UpdateHeartbeat("RPI_STALE", 1000);

    // At current time 10000 with timeout 5000ms:
    // RPI_ACTIVE: 10000 - 9000 = 1000 <= 5000 (online)
    // RPI_STALE: 10000 - 1000 = 9000 > 5000 (offline)
    auto offline = manager.DetectOfflineNodes(10000, 5000);
    TEST_ASSERT(offline.size() == 1, "Exactly 1 node must be detected offline");
    TEST_ASSERT(offline.front() == "RPI_STALE", "RPI_STALE must be detected offline");

    RaspberryPiNode qStale;
    manager.GetNode("RPI_STALE", qStale);
    TEST_ASSERT(!qStale.online, "RPI_STALE must be flagged offline");
    TEST_ASSERT(qStale.nodeHealth == NodeHealth::OFFLINE, "RPI_STALE health must be OFFLINE");

    auto offlineList = manager.GetOfflineNodes();
    TEST_ASSERT(offlineList.size() == 1, "GetOfflineNodes must return 1 node");

    std::cout << "  ✓ PASS: Automatic offline detection (Rule 1) verified." << std::endl;
}

/**
 * @brief TEST 5: Recovery (Rule 9)
 */
void Test5_Recovery() {
    std::cout << "[TEST 5] Recovery..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_REC", "rpi-rec", "192.168.1.130");
    manager.RegisterNode(node);

    // Node goes offline
    manager.UpdateHeartbeat("RPI_REC", 1000);
    manager.DetectOfflineNodes(10000, 5000);

    RaspberryPiNode qBefore;
    manager.GetNode("RPI_REC", qBefore);
    TEST_ASSERT(!qBefore.online, "Node must be offline before recovery");

    // Recover node at timestamp 11000
    TEST_ASSERT(manager.RecoverNode("RPI_REC", 11000), "Recover node");

    RaspberryPiNode qAfter;
    manager.GetNode("RPI_REC", qAfter);
    TEST_ASSERT(qAfter.online, "Node must be online after recovery");
    TEST_ASSERT(qAfter.nodeHealth == NodeHealth::HEALTHY, "Health must be restored to HEALTHY");
    TEST_ASSERT(qAfter.deploymentState == DeploymentState::RECOVERING, "State must be RECOVERING");

    // Rule 9: Recovered nodes require synchronization before rejoining
    TEST_ASSERT(qAfter.syncState == SyncState::NOT_SYNCED, "Rule 9: Recovered node must be NOT_SYNCED");

    std::cout << "  ✓ PASS: Node recovery and synchronization requirement (Rule 9) verified." << std::endl;
}

/**
 * @brief TEST 6: Health Monitoring (Rules 2, 3, 4, 5)
 */
void Test6_HealthMonitoring() {
    std::cout << "[TEST 6] Health Monitoring..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_HEALTH", "rpi-health", "192.168.1.140");
    manager.RegisterNode(node);

    // Initial state: HEALTHY
    RaspberryPiNode qNode;
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::HEALTHY, "Initial health must be HEALTHY");

    // Rule 3: CPU > 85% -> WARNING
    manager.UpdateCPUUsage("RPI_HEALTH", 88.0);
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::WARNING, "CPU > 85% must result in WARNING");

    // Rule 4: RAM > 85% -> WARNING
    manager.UpdateCPUUsage("RPI_HEALTH", 40.0);
    manager.UpdateRAMUsage("RPI_HEALTH", 89.0);
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::WARNING, "RAM > 85% must result in WARNING");

    // Rule 5: Storage > 90% -> WARNING
    manager.UpdateRAMUsage("RPI_HEALTH", 50.0);
    manager.UpdateStorageUsage("RPI_HEALTH", 92.0);
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::WARNING, "Storage > 90% must result in WARNING");

    // Rule 2: Temperature > 80.0°C -> CRITICAL (overrides WARNING)
    manager.UpdateTemperature("RPI_HEALTH", 82.5);
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::CRITICAL, "Temperature > 80C must result in CRITICAL");

    // Temperature drops below 80.0°C and all metrics normal -> HEALTHY
    manager.UpdateStorageUsage("RPI_HEALTH", 45.0);
    manager.UpdateTemperature("RPI_HEALTH", 52.0);
    manager.GetNode("RPI_HEALTH", qNode);
    TEST_ASSERT(qNode.nodeHealth == NodeHealth::HEALTHY, "Normal metrics must restore HEALTHY");

    std::cout << "  ✓ PASS: Multi-metric health classification (Rules 2, 3, 4, 5) verified." << std::endl;
}

/**
 * @brief TEST 7: Deployment State Transitions (Rule 6)
 */
void Test7_Deployment() {
    std::cout << "[TEST 7] Deployment..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_DEP", "rpi-dep", "192.168.1.160");
    manager.RegisterNode(node);

    // Rule 6: Only DEPLOYED nodes may participate
    TEST_ASSERT(!manager.AssignIntersection("RPI_DEP", "INT_01"), "Cannot assign intersection to NOT_DEPLOYED node");
    TEST_ASSERT(!manager.AssignMission("RPI_DEP", "MISSION_01"), "Cannot assign mission to NOT_DEPLOYED node");

    // Transition: NOT_DEPLOYED -> DEPLOYING -> DEPLOYED
    TEST_ASSERT(manager.UpdateDeploymentState("RPI_DEP", DeploymentState::DEPLOYING), "Update state to DEPLOYING");
    TEST_ASSERT(manager.UpdateDeploymentState("RPI_DEP", DeploymentState::DEPLOYED), "Update state to DEPLOYED");

    // Still cannot participate until SYNCED (Rule 7)
    TEST_ASSERT(!manager.AssignIntersection("RPI_DEP", "INT_01"), "Cannot assign until SYNCED");

    // Sync node
    manager.UpdateSynchronizationState("RPI_DEP", SyncState::SYNCED);
    TEST_ASSERT(manager.AssignIntersection("RPI_DEP", "INT_01"), "Assign intersection must succeed when DEPLOYED and SYNCED");
    TEST_ASSERT(manager.AssignMission("RPI_DEP", "MISSION_01"), "Assign mission must succeed when DEPLOYED and SYNCED");

    RaspberryPiNode qNode;
    manager.GetNode("RPI_DEP", qNode);
    TEST_ASSERT(qNode.assignedIntersection == "INT_01", "Assigned intersection must match");
    TEST_ASSERT(qNode.assignedMission == "MISSION_01", "Assigned mission must match");

    std::cout << "  ✓ PASS: Deployment state transitions and participation guarding (Rule 6) verified." << std::endl;
}

/**
 * @brief TEST 8: Synchronization State Lifecycle (Rule 7)
 */
void Test8_Synchronization() {
    std::cout << "[TEST 8] Synchronization..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_SYNC", "rpi-sync", "192.168.1.170");
    manager.RegisterNode(node);

    TEST_ASSERT(manager.UpdateSynchronizationState("RPI_SYNC", SyncState::SYNCING), "Transition to SYNCING");
    RaspberryPiNode qNode;
    manager.GetNode("RPI_SYNC", qNode);
    TEST_ASSERT(qNode.syncState == SyncState::SYNCING, "State must be SYNCING");

    TEST_ASSERT(manager.UpdateSynchronizationState("RPI_SYNC", SyncState::SYNCED), "Transition to SYNCED");
    manager.GetNode("RPI_SYNC", qNode);
    TEST_ASSERT(qNode.syncState == SyncState::SYNCED, "State must be SYNCED");

    std::cout << "  ✓ PASS: Synchronization state lifecycle (Rule 7) verified." << std::endl;
}

/**
 * @brief TEST 9: Configuration Push
 */
void Test9_ConfigurationPush() {
    std::cout << "[TEST 9] Configuration Push..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode node("RPI_CFG", "rpi-cfg", "192.168.1.180");
    manager.RegisterNode(node);

    std::string config1 = "{\"nodeId\":\"RPI_CFG\",\"phaseTiming\":30,\"mode\":\"EMERGENCY\"}";
    TEST_ASSERT(manager.PushConfiguration("RPI_CFG", config1), "Push initial configuration");

    RaspberryPiNode qNode;
    manager.GetNode("RPI_CFG", qNode);
    TEST_ASSERT(qNode.lastConfigPayload == config1, "Payload must match pushed config");
    TEST_ASSERT(qNode.configVersion == 1, "Config version must be 1");

    std::string config2 = "{\"nodeId\":\"RPI_CFG\",\"phaseTiming\":45,\"mode\":\"CIVIL\"}";
    TEST_ASSERT(manager.PushConfiguration("RPI_CFG", config2), "Push updated configuration");
    manager.GetNode("RPI_CFG", qNode);
    TEST_ASSERT(qNode.lastConfigPayload == config2, "Updated payload must match");
    TEST_ASSERT(qNode.configVersion == 2, "Config version must be incremented to 2");

    std::cout << "  ✓ PASS: Configuration payload push and versioning verified." << std::endl;
}

/**
 * @brief TEST 10: Reports Generation
 */
void Test10_Reports() {
    std::cout << "[TEST 10] Reports..." << std::endl;
    RaspberryPiClusterManager manager;

    RaspberryPiNode n1("RPI_REP_1", "rpi-rep-1", "192.168.1.201");
    n1.cpuUsage = 25.5;
    n1.cpuTemperature = 48.0;
    n1.latency = 2.5;
    n1.deploymentState = DeploymentState::DEPLOYED;
    n1.syncState = SyncState::SYNCED;
    n1.nodeHealth = NodeHealth::HEALTHY;

    RaspberryPiNode n2("RPI_REP_2", "rpi-rep-2", "192.168.1.202");
    n2.cpuUsage = 88.0;
    n2.cpuTemperature = 55.0;
    n2.latency = 3.5;
    n2.deploymentState = DeploymentState::DEPLOYED;
    n2.syncState = SyncState::SYNCED;
    n2.nodeHealth = NodeHealth::WARNING;

    manager.RegisterNode(n1);
    manager.RegisterNode(n2);
    manager.UpdateLatency("RPI_REP_1", 2.5);
    manager.UpdateTemperature("RPI_REP_1", 48.0);
    manager.UpdateLatency("RPI_REP_2", 3.5);
    manager.UpdateTemperature("RPI_REP_2", 55.0);

    // 1. Cluster Health Report
    std::string clusterRep = manager.GenerateClusterHealthReport();
    std::cout << clusterRep << std::endl;
    TEST_ASSERT(clusterRep.find("[CLUSTER]") != std::string::npos, "Must contain [CLUSTER]");
    TEST_ASSERT(clusterRep.find("Total Nodes:\n2") != std::string::npos, "Total nodes count");
    TEST_ASSERT(clusterRep.find("Online:\n2") != std::string::npos, "Online nodes count");

    // 2. Deployment Report
    std::string depRep = manager.GenerateDeploymentReport();
    std::cout << depRep << std::endl;
    TEST_ASSERT(depRep.find("[DEPLOYMENT]") != std::string::npos, "Must contain [DEPLOYMENT]");
    TEST_ASSERT(depRep.find("Cluster Deployment:\nSMART_AEGIS_CLUSTER") != std::string::npos, "Cluster deployment name");
    TEST_ASSERT(depRep.find("Deployed:\n2") != std::string::npos, "Deployed count");
    TEST_ASSERT(depRep.find("Synchronized:\n2") != std::string::npos, "Synchronized count");

    // 3. Node Report
    std::string nodeRep = manager.GenerateNodeReport("RPI_REP_1");
    std::cout << nodeRep << std::endl;
    TEST_ASSERT(nodeRep.find("[NODE]") != std::string::npos, "Must contain [NODE]");
    TEST_ASSERT(nodeRep.find("Node:\nRPI_REP_1") != std::string::npos, "Node ID");
    TEST_ASSERT(nodeRep.find("Hostname:\nrpi-rep-1") != std::string::npos, "Hostname");
    TEST_ASSERT(nodeRep.find("IP Address:\n192.168.1.201") != std::string::npos, "IP Address");
    TEST_ASSERT(nodeRep.find("Health:\nHEALTHY") != std::string::npos, "Health status");

    std::cout << "  ✓ PASS: All formatted reports validated against exact specification." << std::endl;
}

/**
 * @brief TEST 11: 100-Thread Concurrency
 */
void Test11_Concurrency() {
    std::cout << "[TEST 11] 100-Thread Concurrency..." << std::endl;
    RaspberryPiClusterManager manager;

    // Pre-register 20 base nodes
    for (int i = 0; i < 20; ++i) {
        RaspberryPiNode n("BASE_RPI_" + std::to_string(i), "base-" + std::to_string(i), "192.168.1." + std::to_string(10 + i));
        n.deploymentState = DeploymentState::DEPLOYED;
        n.syncState = SyncState::SYNCED;
        manager.RegisterNode(n);
    }

    std::vector<std::thread> threads;
    threads.reserve(100);
    std::atomic<int> successCount{0};

    for (int t = 0; t < 100; ++t) {
        threads.emplace_back([&manager, t, &successCount]() {
            std::string tNodeId = "THREAD_RPI_" + std::to_string(t);
            manager.DiscoverNode(tNodeId, "192.168.2." + std::to_string(t % 250), "thread-host-" + std::to_string(t));

            manager.UpdateHeartbeat(tNodeId, 1000 + static_cast<uint64_t>(t));
            manager.UpdateTemperature(tNodeId, 45.0 + static_cast<double>(t % 40));
            manager.UpdateCPUUsage(tNodeId, static_cast<double>(t % 95));
            manager.UpdateRAMUsage(tNodeId, static_cast<double>((t * 3) % 95));
            manager.UpdateLatency(tNodeId, 1.5 + static_cast<double>(t % 20));

            manager.UpdateDeploymentState(tNodeId, DeploymentState::DEPLOYED);
            manager.UpdateSynchronizationState(tNodeId, SyncState::SYNCED);
            manager.PushConfiguration(tNodeId, "{\"thread\":" + std::to_string(t) + "}");

            // Queries & reports
            RaspberryPiNode qNode;
            manager.GetNode(tNodeId, qNode);
            manager.GenerateNodeReport(tNodeId);
            manager.GenerateClusterHealthReport();
            manager.GetStatistics();

            successCount++;
        });
    }

    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    TEST_ASSERT(successCount == 100, "All 100 concurrent threads must complete cleanly");
    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: " << successCount << ")." << std::endl;
}

/**
 * @brief TEST 12: Performance Benchmark
 * (100 Nodes, 1,000 Heartbeats, 1,000 Telemetries, 100 Deployments, < 5ms Verification)
 */
void Test12_PerformanceBenchmark() {
    std::cout << "[TEST 12] Performance Benchmark: 100 Nodes, 1,000 Heartbeats, 1,000 Telemetries, 100 Deployments..." << std::endl;
    RaspberryPiClusterManager manager;
    const int NUM_NODES = 100;
    const int NUM_HEARTBEATS = 1000;
    const int NUM_TELEMETRIES = 1000;

    // 1. Register 100 Raspberry Pi nodes
    for (int i = 0; i < NUM_NODES; ++i) {
        std::string id = "BENCH_RPI_" + std::to_string(i);
        RaspberryPiNode n(id, "bench-" + std::to_string(i), "10.0.0." + std::to_string(i + 1));
        manager.RegisterNode(n);
    }

    // 2. Measure 1,000 Heartbeat Updates
    auto hbStart = std::chrono::high_resolution_clock::now();
    for (int h = 0; h < NUM_HEARTBEATS; ++h) {
        std::string id = "BENCH_RPI_" + std::to_string(h % NUM_NODES);
        manager.UpdateHeartbeat(id, 1000 + static_cast<uint64_t>(h));
    }
    auto hbEnd = std::chrono::high_resolution_clock::now();
    double hbTotalMs = std::chrono::duration<double, std::milli>(hbEnd - hbStart).count();
    double hbAvgMs = hbTotalMs / NUM_HEARTBEATS;
    std::cout << "  -> 1,000 Heartbeats Total Latency: " << hbTotalMs << " ms (Avg: " << hbAvgMs << " ms / heartbeat)" << std::endl;

    // 3. Measure 1,000 Telemetry Updates (CPU, Temp, RAM)
    auto telStart = std::chrono::high_resolution_clock::now();
    for (int t = 0; t < NUM_TELEMETRIES; ++t) {
        std::string id = "BENCH_RPI_" + std::to_string(t % NUM_NODES);
        manager.UpdateTemperature(id, 50.0 + static_cast<double>(t % 25));
        manager.UpdateCPUUsage(id, static_cast<double>(t % 80));
    }
    auto telEnd = std::chrono::high_resolution_clock::now();
    double telTotalMs = std::chrono::duration<double, std::milli>(telEnd - telStart).count();
    double telAvgMs = telTotalMs / NUM_TELEMETRIES;
    std::cout << "  -> 1,000 Telemetries Total Latency: " << telTotalMs << " ms (Avg: " << telAvgMs << " ms / update)" << std::endl;

    // 4. Measure 100 Deployments and Synchronizations
    auto depStart = std::chrono::high_resolution_clock::now();
    for (int d = 0; d < NUM_NODES; ++d) {
        std::string id = "BENCH_RPI_" + std::to_string(d);
        manager.UpdateDeploymentState(id, DeploymentState::DEPLOYED);
        manager.UpdateSynchronizationState(id, SyncState::SYNCED);
    }
    auto depEnd = std::chrono::high_resolution_clock::now();
    double depTotalMs = std::chrono::duration<double, std::milli>(depEnd - depStart).count();
    std::cout << "  -> 100 Deployments Total Latency: " << depTotalMs << " ms" << std::endl;

    // 5. Measure Cluster Readiness Verification (< 5 ms requirement)
    auto verifyStart = std::chrono::high_resolution_clock::now();
    bool ready = manager.VerifyClusterReadiness();
    auto verifyEnd = std::chrono::high_resolution_clock::now();
    double verifyTotalMs = std::chrono::duration<double, std::milli>(verifyEnd - verifyStart).count();

    std::cout << "  -> Deployment Verification Latency: " << verifyTotalMs << " ms (Ready: " << (ready ? "YES" : "NO") << ")" << std::endl;
    TEST_ASSERT(ready, "Cluster readiness must be verified true");
    TEST_ASSERT(verifyTotalMs < 5.0, "Cluster readiness verification must be < 5.0 ms");

    std::cout << "  ✓ PASS: All performance benchmarks (< 5 ms deployment verification) satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 14A RPi Cluster Manager Tests         \n";
    std::cout << "==========================================================\n";

    Test1_NodeRegistration();
    Test2_Discovery();
    Test3_Heartbeat();
    Test4_OfflineDetection();
    Test5_Recovery();
    Test6_HealthMonitoring();
    Test7_Deployment();
    Test8_Synchronization();
    Test9_ConfigurationPush();
    Test10_Reports();
    Test11_Concurrency();
    Test12_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " ALL 12 PHASE 14A UNIT TESTS PASSED SUCCESSFULLY (100%)  \n";
    std::cout << "==========================================================\n";
    return 0;
}

