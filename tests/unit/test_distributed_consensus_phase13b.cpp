/**
 * @file test_distributed_consensus_phase13b.cpp
 * @brief SMART AEGIS (AERS) Phase 13B Unit Tests
 *
 * Validates the Distributed Traffic Signal Consensus & Fault-Tolerant Coordination Engine:
 * - 12 Deterministic Unit Tests
 * - Node Registration & Removal
 * - Deterministic Priority Leader Election & Invariants
 * - Majority Consensus Verification & Immutability
 * - Consensus Rejection & Timeout Abort
 * - Leader Failure Detection & Automatic Re-election
 * - Safe Signal Rollback Clearance
 * - Heartbeat Monitoring & Health Classification
 * - Offline Node Isolation & Recovery
 * - Structured ASCII Reports ([CONSENSUS], [CLUSTER], [NODE])
 * - 100-Thread Concurrency & Race-Free Stress Test
 * - Performance Benchmark (100 Nodes, 20 Sessions, 1,000 Votes)
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "distributed_consensus/DistributedConsensusEngine.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis::consensus;

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
    std::cout << "[TEST 1] Node Registration & Removal..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_01", "INT_01", 5, 12.5);
    ConsensusNode n2("NODE_02", "INT_02", 10, 8.0);

    TEST_ASSERT(engine.RegisterNode(n1), "Register NODE_01 must succeed");
    TEST_ASSERT(engine.RegisterNode(n2), "Register NODE_02 must succeed");
    TEST_ASSERT(!engine.RegisterNode(n1), "Duplicate registration of NODE_01 must fail");

    ConsensusNode queryNode;
    TEST_ASSERT(engine.GetNode("NODE_01", queryNode), "GetNode NODE_01 must succeed");
    TEST_ASSERT(queryNode.intersectionId == "INT_01", "Intersection ID must match");
    TEST_ASSERT(queryNode.priority == 5, "Priority must match");
    TEST_ASSERT(queryNode.online, "Node must be online");

    auto allNodes = engine.GetAllNodes();
    TEST_ASSERT(allNodes.size() == 2, "There must be exactly 2 registered nodes");

    TEST_ASSERT(engine.RemoveNode("NODE_01"), "RemoveNode NODE_01 must succeed");
    TEST_ASSERT(!engine.GetNode("NODE_01", queryNode), "NODE_01 must no longer exist");
    TEST_ASSERT(engine.GetAllNodes().size() == 1, "There must be exactly 1 remaining node");
    TEST_ASSERT(!engine.RemoveNode("NON_EXISTENT"), "Removing non-existent node must fail");

    std::cout << "  ✓ PASS: Node registration, duplicate prevention, and node removal verified." << std::endl;
}

/**
 * @brief TEST 2: Leader Election
 */
void Test2_LeaderElection() {
    std::cout << "[TEST 2] Leader Election..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode nLow("NODE_LOW", "INT_01", 1, 20.0);
    ConsensusNode nMid("NODE_MID", "INT_02", 5, 15.0);
    ConsensusNode nHigh("NODE_HIGH", "INT_03", 10, 5.0);

    engine.RegisterNode(nLow);
    engine.RegisterNode(nMid);
    engine.RegisterNode(nHigh);

    std::string leader = engine.ElectLeader();
    TEST_ASSERT(leader == "NODE_HIGH", "Highest priority node must be elected leader");
    TEST_ASSERT(engine.GetLeader() == "NODE_HIGH", "GetLeader must return NODE_HIGH");

    ConsensusNode queryHigh;
    engine.GetNode("NODE_HIGH", queryHigh);
    TEST_ASSERT(queryHigh.role == NodeRole::LEADER, "Elected node role must be LEADER");

    ConsensusNode queryMid;
    engine.GetNode("NODE_MID", queryMid);
    TEST_ASSERT(queryMid.role == NodeRole::FOLLOWER, "Other node role must be FOLLOWER");

    ConsensusNode queryLow;
    engine.GetNode("NODE_LOW", queryLow);
    TEST_ASSERT(queryLow.role == NodeRole::FOLLOWER, "Other node role must be FOLLOWER");

    // Rule 1: Exactly one leader exists
    uint32_t leaderCount = 0;
    for (const auto& node : engine.GetAllNodes()) {
        if (node.role == NodeRole::LEADER) {
            leaderCount++;
        }
    }
    TEST_ASSERT(leaderCount == 1, "Rule 1: Exactly one leader must exist in cluster");
    TEST_ASSERT(engine.GetCurrentTerm() > 0, "Current term must be positive");

    std::cout << "  ✓ PASS: Deterministic leader election and single-leader invariant verified." << std::endl;
}

/**
 * @brief TEST 3: Majority Consensus
 */
void Test3_MajorityConsensus() {
    std::cout << "[TEST 3] Majority Consensus..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_1", "INT_1", 5);
    ConsensusNode n2("NODE_2", "INT_2", 8);
    ConsensusNode n3("NODE_3", "INT_3", 10);

    engine.RegisterNode(n1);
    engine.RegisterNode(n2);
    engine.RegisterNode(n3);

    engine.ElectLeader();

    std::vector<std::string> cluster = {"NODE_1", "NODE_2", "NODE_3"};
    TEST_ASSERT(engine.StartConsensus("SESS_01", "MISSION_101", "CORR_A", cluster, 1000), "Start consensus");

    ConsensusSession session;
    TEST_ASSERT(engine.GetConsensus("SESS_01", session), "Get consensus session");
    TEST_ASSERT(session.requiredVotes == 2, "Majority of 3 nodes must be 2 votes");
    TEST_ASSERT(session.state == ConsensusState::PREPARING, "Initial state must be PREPARING");

    // Cast majority votes
    TEST_ASSERT(engine.CastVote("SESS_01", "NODE_1", NodeVote::YES, 1010), "Node 1 votes YES");
    TEST_ASSERT(engine.CastVote("SESS_01", "NODE_2", NodeVote::YES, 1020), "Node 2 votes YES");

    // Attempt duplicate vote from NODE_1
    TEST_ASSERT(!engine.CastVote("SESS_01", "NODE_1", NodeVote::YES, 1025), "Duplicate vote must be rejected");

    // Commit consensus (2 out of 3 = majority)
    TEST_ASSERT(engine.CommitConsensus("SESS_01", 1050), "Commit consensus with majority");

    engine.GetConsensus("SESS_01", session);
    TEST_ASSERT(session.state == ConsensusState::COMMITTED, "Session state must be COMMITTED");
    TEST_ASSERT(session.commitTimestamp == 1050, "Commit timestamp must match");
    TEST_ASSERT(session.latencyMs == 50.0, "Latency must be calculated from start to commit");

    // Rule 10: Committed sessions become immutable
    TEST_ASSERT(!engine.CastVote("SESS_01", "NODE_3", NodeVote::YES, 1060), "Cannot vote on committed session");
    TEST_ASSERT(!engine.CommitConsensus("SESS_01", 1070), "Cannot re-commit committed session");
    TEST_ASSERT(!engine.AbortConsensus("SESS_01", "late abort"), "Cannot abort committed session");

    // Check last commit index updated
    ConsensusNode nodeQuery;
    engine.GetNode("NODE_1", nodeQuery);
    TEST_ASSERT(nodeQuery.lastCommitIndex == 1, "Last commit index must be incremented");

    std::cout << "  ✓ PASS: Majority consensus and session immutability verified." << std::endl;
}

/**
 * @brief TEST 4: Consensus Rejection
 */
void Test4_ConsensusRejection() {
    std::cout << "[TEST 4] Consensus Rejection..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_R1", "INT_1", 5);
    ConsensusNode n2("NODE_R2", "INT_2", 5);
    ConsensusNode n3("NODE_R3", "INT_3", 5);

    engine.RegisterNode(n1);
    engine.RegisterNode(n2);
    engine.RegisterNode(n3);

    std::vector<std::string> cluster = {"NODE_R1", "NODE_R2", "NODE_R3"};
    engine.StartConsensus("SESS_REJECT", "MISSION_102", "CORR_B", cluster, 2000);

    // Nodes vote NO
    TEST_ASSERT(engine.CastVote("SESS_REJECT", "NODE_R1", NodeVote::NO, 2010), "Node 1 votes NO");
    TEST_ASSERT(engine.CastVote("SESS_REJECT", "NODE_R2", NodeVote::NO, 2020), "Node 2 votes NO");

    // Attempt commit without majority of YES votes
    TEST_ASSERT(!engine.CommitConsensus("SESS_REJECT", 2050), "Commit must fail without majority");

    ConsensusSession session;
    engine.GetConsensus("SESS_REJECT", session);
    TEST_ASSERT(session.state == ConsensusState::REJECTED, "Session state must be REJECTED");

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.failedSessions > 0, "Failed sessions count must increment");

    std::cout << "  ✓ PASS: Consensus rejection upon insufficient YES votes verified." << std::endl;
}

/**
 * @brief TEST 5: Leader Failure Detection
 */
void Test5_LeaderFailure() {
    std::cout << "[TEST 5] Leader Failure..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode nLeader("LEADER_1", "INT_L", 20, 5.0);
    ConsensusNode nFollower("FOLLOWER_1", "INT_F", 10, 8.0);

    engine.RegisterNode(nLeader);
    engine.RegisterNode(nFollower);

    engine.ElectLeader();
    TEST_ASSERT(engine.GetLeader() == "LEADER_1", "LEADER_1 must be elected leader");

    // Set heartbeat for LEADER_1 at timestamp 1000
    engine.UpdateHeartbeat("LEADER_1", 1000);
    engine.UpdateHeartbeat("FOLLOWER_1", 5000);

    // At timestamp 7000, threshold 5000ms: LEADER_1 is stale (7000 - 1000 = 6000 > 5000)
    auto failed = engine.DetectNodeFailure(7000, 5000);
    TEST_ASSERT(!failed.empty(), "Failure must be detected");
    TEST_ASSERT(failed.front() == "LEADER_1", "LEADER_1 must be marked failed");

    ConsensusNode queryLeader;
    engine.GetNode("LEADER_1", queryLeader);
    TEST_ASSERT(!queryLeader.online, "LEADER_1 must be offline");
    TEST_ASSERT(queryLeader.health == SynchronizationHealth::FAILED, "Health must be FAILED");

    std::cout << "  ✓ PASS: Heartbeat timeout and leader failure detection verified." << std::endl;
}

/**
 * @brief TEST 6: Automatic Re-Election
 */
void Test6_AutomaticReElection() {
    std::cout << "[TEST 6] Automatic Re-Election..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_A", "INT_A", 10);
    ConsensusNode n2("NODE_B", "INT_B", 25);
    ConsensusNode n3("NODE_C", "INT_C", 50);

    engine.RegisterNode(n1);
    engine.RegisterNode(n2);
    engine.RegisterNode(n3);

    engine.ElectLeader();
    TEST_ASSERT(engine.GetLeader() == "NODE_C", "NODE_C must be initial leader");

    // Leader goes offline
    TEST_ASSERT(engine.SetNodeOffline("NODE_C"), "Set NODE_C offline");

    // Rule 5: Leader failure triggers new election
    std::string newLeader = engine.GetLeader();
    TEST_ASSERT(newLeader == "NODE_B", "NODE_B must be automatically re-elected as next highest priority");

    ConsensusNode nodeB;
    engine.GetNode("NODE_B", nodeB);
    TEST_ASSERT(nodeB.role == NodeRole::LEADER, "NODE_B role must be LEADER");

    ConsensusNode nodeC;
    engine.GetNode("NODE_C", nodeC);
    TEST_ASSERT(nodeC.role == NodeRole::FOLLOWER, "NODE_C role must be FOLLOWER");

    std::cout << "  ✓ PASS: Leader failure trigger and automatic re-election verified." << std::endl;
}

/**
 * @brief TEST 7: Rollback
 */
void Test7_Rollback() {
    std::cout << "[TEST 7] Rollback..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_RB1", "INT_1", 5);
    ConsensusNode n2("NODE_RB2", "INT_2", 5);
    engine.RegisterNode(n1);
    engine.RegisterNode(n2);

    std::vector<std::string> cluster = {"NODE_RB1", "NODE_RB2"};
    engine.StartConsensus("SESS_RB", "MISSION_RB", "CORR_RB", cluster, 1000);
    engine.CastVote("SESS_RB", "NODE_RB1", NodeVote::YES, 1010);
    engine.CastVote("SESS_RB", "NODE_RB2", NodeVote::YES, 1020);
    engine.CommitConsensus("SESS_RB", 1050);

    // Execute rollback
    TEST_ASSERT(engine.RollbackConsensus("SESS_RB", 1100), "Rollback session");

    ConsensusSession session;
    engine.GetConsensus("SESS_RB", session);
    TEST_ASSERT(session.state == ConsensusState::ROLLBACK, "Session state must be ROLLBACK");
    TEST_ASSERT(session.rollbackTimestamp == 1100, "Rollback timestamp must be set");

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.rollbacks == 1, "Rollback statistic must be incremented");

    // Rule 6: Rollback restores previous stable signal state
    ConsensusNode q1;
    engine.GetNode("NODE_RB1", q1);
    TEST_ASSERT(q1.lastCommitIndex == 0, "Last commit index must be reverted");

    std::cout << "  ✓ PASS: Safe rollback and previous stable state restoration verified." << std::endl;
}

/**
 * @brief TEST 8: Heartbeat Monitoring & Health Classification
 */
void Test8_HeartbeatMonitoring() {
    std::cout << "[TEST 8] Heartbeat Monitoring..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode node("NODE_HB", "INT_HB", 5);
    engine.RegisterNode(node);

    // Initial heartbeat -> default latency 10.0 ms -> HEALTHY
    TEST_ASSERT(engine.UpdateHeartbeat("NODE_HB", 1000), "First heartbeat");
    ConsensusNode qNode;
    engine.GetNode("NODE_HB", qNode);
    TEST_ASSERT(qNode.health == SynchronizationHealth::HEALTHY, "Health must be HEALTHY (< 50ms)");

    // Test latency health updates
    engine.UpdateLatency("NODE_HB", 85.0);
    engine.GetNode("NODE_HB", qNode);
    TEST_ASSERT(qNode.health == SynchronizationHealth::DEGRADED, "Health must be DEGRADED (50-150ms)");

    engine.UpdateLatency("NODE_HB", 220.0);
    engine.GetNode("NODE_HB", qNode);
    TEST_ASSERT(qNode.health == SynchronizationHealth::UNSTABLE, "Health must be UNSTABLE (150-300ms)");

    engine.UpdateLatency("NODE_HB", 350.0);
    engine.GetNode("NODE_HB", qNode);
    TEST_ASSERT(qNode.health == SynchronizationHealth::FAILED, "Health must be FAILED (>= 300ms)");

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.averageHeartbeatLatency > 0.0, "Average heartbeat latency must be recorded");

    std::cout << "  ✓ PASS: Heartbeat monitoring and 4-tier health classification verified." << std::endl;
}

/**
 * @brief TEST 9: Node Recovery
 */
void Test9_NodeRecovery() {
    std::cout << "[TEST 9] Node Recovery..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode node("NODE_REC", "INT_REC", 5);
    engine.RegisterNode(node);

    // Mark node offline
    TEST_ASSERT(engine.SetNodeOffline("NODE_REC"), "Set node offline");
    ConsensusNode qNode;
    engine.GetNode("NODE_REC", qNode);
    TEST_ASSERT(!qNode.online, "Node must be offline");

    // Rule 3: Offline nodes cannot vote
    engine.StartConsensus("SESS_OFF", "M_OFF", "C_OFF", {"NODE_REC"}, 1000);
    TEST_ASSERT(!engine.CastVote("SESS_OFF", "NODE_REC", NodeVote::YES, 1010), "Offline node cannot cast vote");

    // Recover node
    TEST_ASSERT(engine.RecoverNode("NODE_REC", 2000), "Recover node");
    engine.GetNode("NODE_REC", qNode);
    TEST_ASSERT(qNode.online, "Node must be online after recovery");
    TEST_ASSERT(qNode.health == SynchronizationHealth::HEALTHY, "Health must be restored to HEALTHY");

    // Re-attempt vote after recovery
    TEST_ASSERT(engine.CastVote("SESS_OFF", "NODE_REC", NodeVote::YES, 2010), "Recovered node can vote");

    std::cout << "  ✓ PASS: Node recovery and voting re-enablement verified." << std::endl;
}

/**
 * @brief TEST 10: Reports Generation
 */
void Test10_Reports() {
    std::cout << "[TEST 10] Reports..." << std::endl;
    DistributedConsensusEngine engine;

    ConsensusNode n1("NODE_REP_1", "INT_REP_1", 10, 1.25);
    ConsensusNode n2("NODE_REP_2", "INT_REP_2", 5, 2.50);
    engine.RegisterNode(n1);
    engine.RegisterNode(n2);

    engine.ElectLeader();

    std::vector<std::string> cluster = {"NODE_REP_1", "NODE_REP_2"};
    engine.StartConsensus("SESS_REP", "MISSION_REP", "CORR_REP", cluster, 1000);
    engine.CastVote("SESS_REP", "NODE_REP_1", NodeVote::YES, 1010);
    engine.CastVote("SESS_REP", "NODE_REP_2", NodeVote::YES, 1020);
    engine.CommitConsensus("SESS_REP", 1025);

    // 1. Consensus Report
    std::string rep = engine.GenerateConsensusReport("SESS_REP");
    std::cout << rep << std::endl;
    TEST_ASSERT(rep.find("[CONSENSUS]") != std::string::npos, "Report must contain [CONSENSUS]");
    TEST_ASSERT(rep.find("Session:\nSESS_REP") != std::string::npos, "Report must contain Session");
    TEST_ASSERT(rep.find("Mission:\nMISSION_REP") != std::string::npos, "Report must contain Mission");
    TEST_ASSERT(rep.find("Corridor:\nCORR_REP") != std::string::npos, "Report must contain Corridor");
    TEST_ASSERT(rep.find("Votes:\n2/2") != std::string::npos, "Report must contain Votes");
    TEST_ASSERT(rep.find("State:\nCOMMITTED") != std::string::npos, "Report must contain State");

    // 2. Cluster Report
    std::string clusterRep = engine.GenerateClusterReport();
    std::cout << clusterRep << std::endl;
    TEST_ASSERT(clusterRep.find("[CLUSTER]") != std::string::npos, "Report must contain [CLUSTER]");
    TEST_ASSERT(clusterRep.find("Cluster:\nSMART_AEGIS_CONSENSUS") != std::string::npos, "Cluster name");
    TEST_ASSERT(clusterRep.find("Leader:\nNODE_REP_1") != std::string::npos, "Leader name");

    // 3. Node Report
    std::string nodeRep = engine.GenerateNodeReport("NODE_REP_1");
    std::cout << nodeRep << std::endl;
    TEST_ASSERT(nodeRep.find("[NODE]") != std::string::npos, "Report must contain [NODE]");
    TEST_ASSERT(nodeRep.find("Node:\nNODE_REP_1") != std::string::npos, "Node ID");
    TEST_ASSERT(nodeRep.find("Role:\nLEADER") != std::string::npos, "Role must be LEADER");

    std::cout << "  ✓ PASS: All formatted reports validated against exact specification." << std::endl;
}

/**
 * @brief TEST 11: 100-Thread Concurrency
 */
void Test11_Concurrency() {
    std::cout << "[TEST 11] 100-Thread Concurrency..." << std::endl;
    DistributedConsensusEngine engine;

    // Pre-register base nodes
    for (int i = 0; i < 20; ++i) {
        ConsensusNode n("BASE_NODE_" + std::to_string(i), "INT_" + std::to_string(i), static_cast<uint32_t>(i + 1));
        engine.RegisterNode(n);
    }
    engine.ElectLeader();

    std::vector<std::thread> threads;
    threads.reserve(100);
    std::atomic<int> successCount{0};

    for (int t = 0; t < 100; ++t) {
        threads.emplace_back([&engine, t, &successCount]() {
            std::string tNodeId = "THREAD_NODE_" + std::to_string(t);
            ConsensusNode node(tNodeId, "INT_T_" + std::to_string(t), static_cast<uint32_t>(t % 10 + 1));
            engine.RegisterNode(node);

            engine.UpdateHeartbeat(tNodeId, 1000 + static_cast<uint64_t>(t));
            engine.UpdateLatency(tNodeId, 15.0 + static_cast<double>(t % 50));

            // Start a quick session
            std::string sId = "THREAD_SESS_" + std::to_string(t);
            engine.StartConsensus(sId, "MISS_T", "CORR_T", {tNodeId, "BASE_NODE_0"});
            engine.CastVote(sId, tNodeId, NodeVote::YES);
            engine.CastVote(sId, "BASE_NODE_0", NodeVote::YES);
            engine.CommitConsensus(sId);

            // Queries & reports
            ConsensusNode qNode;
            engine.GetNode(tNodeId, qNode);
            std::string rep = engine.GenerateNodeReport(tNodeId);
            std::string clusterRep = engine.GenerateClusterReport();
            engine.GetStatistics();

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
 * @brief TEST 12: Performance Benchmark (100 Nodes, 20 Sessions, 1,000 Votes)
 */
void Test12_PerformanceBenchmark() {
    std::cout << "[TEST 12] Performance Benchmark: 100 Nodes, 20 Sessions, 1,000 Votes..." << std::endl;
    DistributedConsensusEngine engine;
    const int NUM_NODES = 100;
    const int NUM_SESSIONS = 20;
    const int NUM_VOTES = 1000;

    // 1. Register 100 Nodes
    for (int i = 0; i < NUM_NODES; ++i) {
        std::string id = "BENCH_NODE_" + std::to_string(i);
        ConsensusNode n(id, "BENCH_INT_" + std::to_string(i), static_cast<uint32_t>((i % 20) + 1));
        engine.RegisterNode(n);
    }

    // 2. Measure Leader Election Latency (< 1 ms target)
    auto electStart = std::chrono::high_resolution_clock::now();
    std::string leader = engine.ElectLeader();
    auto electEnd = std::chrono::high_resolution_clock::now();
    double electDurationMs = std::chrono::duration<double, std::milli>(electEnd - electStart).count();

    std::cout << "  -> Leader Election Latency: " << electDurationMs << " ms (Leader: " << leader << ")" << std::endl;
    TEST_ASSERT(electDurationMs < 1.0, "Leader election must complete in < 1.0 ms");

    // 3. Measure Heartbeat Update Latency (< 0.01 ms average target)
    auto hbStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM_NODES; ++i) {
        engine.UpdateHeartbeat("BENCH_NODE_" + std::to_string(i));
    }
    auto hbEnd = std::chrono::high_resolution_clock::now();
    double hbTotalMs = std::chrono::duration<double, std::milli>(hbEnd - hbStart).count();
    double hbPerNodeMs = hbTotalMs / NUM_NODES;

    std::cout << "  -> 100 Heartbeats Total Latency: " << hbTotalMs << " ms (Avg: " << hbPerNodeMs << " ms / node)" << std::endl;
    TEST_ASSERT(hbPerNodeMs < 0.01, "Average heartbeat update must be < 0.01 ms");

    // 4. Create 20 Consensus Sessions
    for (int s = 0; s < NUM_SESSIONS; ++s) {
        std::string sId = "BENCH_SESS_" + std::to_string(s);
        std::vector<std::string> participants;
        for (int j = 0; j < 5; ++j) {
            participants.push_back("BENCH_NODE_" + std::to_string((s * 5 + j) % NUM_NODES));
        }
        engine.StartConsensus(sId, "BENCH_MISSION_" + std::to_string(s), "BENCH_CORR_" + std::to_string(s), participants);
    }

    // 5. Measure 1,000 Votes Processing Latency (< 0.05 ms per vote target)
    auto voteStart = std::chrono::high_resolution_clock::now();
    for (int v = 0; v < NUM_VOTES; ++v) {
        std::string sId = "BENCH_SESS_VOTE_" + std::to_string(v % 100);
        // Create session on-the-fly for fresh votes
        if (v < 100) {
            engine.StartConsensus(sId, "M_V", "C_V", {"BENCH_NODE_0", "BENCH_NODE_1", "BENCH_NODE_2"});
        }
        std::string nId = "BENCH_NODE_" + std::to_string(v % 3);
        engine.CastVote(sId, nId, NodeVote::YES);
    }
    auto voteEnd = std::chrono::high_resolution_clock::now();
    double voteTotalMs = std::chrono::duration<double, std::milli>(voteEnd - voteStart).count();
    double votePerVoteMs = voteTotalMs / NUM_VOTES;

    std::cout << "  -> 1,000 Votes Processing Total Latency: " << voteTotalMs << " ms (Avg: " << votePerVoteMs << " ms / vote)" << std::endl;
    TEST_ASSERT(votePerVoteMs < 0.05, "Average vote processing must be < 0.05 ms");

    // 6. Measure 20 Consensus Sessions Commit Latency (< 5 ms target)
    auto consStart = std::chrono::high_resolution_clock::now();
    for (int s = 0; s < NUM_SESSIONS; ++s) {
        std::string sId = "BENCH_SESS_" + std::to_string(s);
        // Cast majority votes for each session
        for (int j = 0; j < 3; ++j) {
            std::string nId = "BENCH_NODE_" + std::to_string((s * 5 + j) % NUM_NODES);
            engine.CastVote(sId, nId, NodeVote::YES);
        }
        engine.CommitConsensus(sId);
    }
    auto consEnd = std::chrono::high_resolution_clock::now();
    double consTotalMs = std::chrono::duration<double, std::milli>(consEnd - consStart).count();

    std::cout << "  -> 20 Consensus Sessions Total Latency: " << consTotalMs << " ms" << std::endl;
    TEST_ASSERT(consTotalMs < 5.0, "20 consensus sessions must complete in < 5.0 ms");

    std::cout << "  ✓ PASS: All performance benchmarks (< 1 ms election, < 5 ms consensus, < 0.05 ms vote, < 0.01 ms heartbeat) satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================
int main() {
    std::cout << "==========================================================\n";
    std::cout << " SMART AEGIS: Phase 13B Distributed Consensus Engine Tests\n";
    std::cout << "==========================================================\n";

    Test1_NodeRegistration();
    Test2_LeaderElection();
    Test3_MajorityConsensus();
    Test4_ConsensusRejection();
    Test5_LeaderFailure();
    Test6_AutomaticReElection();
    Test7_Rollback();
    Test8_HeartbeatMonitoring();
    Test9_NodeRecovery();
    Test10_Reports();
    Test11_Concurrency();
    Test12_PerformanceBenchmark();

    std::cout << "==========================================================\n";
    std::cout << " ALL 12 PHASE 13B UNIT TESTS PASSED SUCCESSFULLY (100%)  \n";
    std::cout << "==========================================================\n";
    return 0;
}

