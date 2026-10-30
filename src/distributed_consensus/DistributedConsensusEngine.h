/**
 * @file DistributedConsensusEngine.h
 * @brief SMART AEGIS (AERS) Phase 13B - Distributed Traffic Signal Consensus & Fault-Tolerant Coordination Engine
 *
 * Implements the distributed consensus layer responsible for guaranteeing that every
 * Raspberry Pi traffic controller executes exactly the same emergency traffic plan.
 * Sits between TrafficNetworkOrchestrator (Phase 13A) and Distributed Signal Controller (Phase 10A).
 *
 * MissionExecutionEngine decides WHAT.
 * IncidentCommandEngine decides WHY.
 * TrafficNetworkOrchestrator decides HOW.
 * DistributedConsensusEngine guarantees EVERY NODE agrees BEFORE execution.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 Model B & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_DISTRIBUTED_CONSENSUS_ENGINE_H
#define AEGIS_DISTRIBUTED_CONSENSUS_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace consensus {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Distributed consensus state for a multi-node session.
 */
enum class ConsensusState {
    UNKNOWN,
    PREPARING,
    VOTING,
    COMMITTED,
    REJECTED,
    FAILED,
    ROLLBACK
};

/**
 * @brief Individual node voting decision.
 */
enum class NodeVote {
    YES,
    NO,
    TIMEOUT,
    OFFLINE
};

/**
 * @brief Cluster role of a distributed consensus node.
 */
enum class NodeRole {
    LEADER,
    FOLLOWER,
    CANDIDATE
};

/**
 * @brief Synchronization and communication health classification.
 */
enum class SynchronizationHealth {
    HEALTHY,
    DEGRADED,
    UNSTABLE,
    FAILED
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(ConsensusState state);
std::string ToString(NodeVote vote);
std::string ToString(NodeRole role);
std::string ToString(SynchronizationHealth health);

ConsensusState ConsensusStateFromString(const std::string& str);
NodeVote NodeVoteFromString(const std::string& str);
NodeRole NodeRoleFromString(const std::string& str);
SynchronizationHealth SynchronizationHealthFromString(const std::string& str);

// =============================================================================
// Data Structures
// =============================================================================

/**
 * @brief Represents an individual edge consensus participant (Raspberry Pi node).
 */
struct ConsensusNode {
    std::string nodeId;
    std::string intersectionId;
    NodeRole role{NodeRole::FOLLOWER};
    bool online{true};
    uint64_t lastHeartbeat{0};
    uint64_t currentTerm{0};
    uint64_t lastCommitIndex{0};
    NodeVote vote{NodeVote::NO};
    SynchronizationHealth health{SynchronizationHealth::HEALTHY};
    double latencyMs{0.0};
    uint32_t priority{1};

    ConsensusNode() = default;

    ConsensusNode(const std::string& nId,
                  const std::string& intId = "",
                  uint32_t prio = 1,
                  double lat = 0.0)
        : nodeId(nId),
          intersectionId(intId.empty() ? nId : intId),
          role(NodeRole::FOLLOWER),
          online(true),
          lastHeartbeat(0),
          currentTerm(0),
          lastCommitIndex(0),
          vote(NodeVote::NO),
          health(SynchronizationHealth::HEALTHY),
          latencyMs(lat),
          priority(prio) {}
};

/**
 * @brief Represents a distributed consensus execution session.
 */
struct ConsensusSession {
    std::string sessionId;
    std::string missionId;
    std::string corridorId;
    std::string leaderNode;
    std::vector<std::string> participatingNodes;
    uint32_t requiredVotes{0};
    uint32_t receivedVotes{0};
    uint64_t startTimestamp{0};
    uint64_t commitTimestamp{0};
    uint64_t rollbackTimestamp{0};
    ConsensusState state{ConsensusState::UNKNOWN};

    // Internal vote tracking and audit records
    std::unordered_map<std::string, NodeVote> votes;
    uint32_t yesVotes{0};
    uint32_t noVotes{0};
    uint32_t timeoutVotes{0};
    uint64_t term{0};
    double latencyMs{0.0};

    ConsensusSession() = default;

    ConsensusSession(const std::string& sId,
                     const std::string& mId = "",
                     const std::string& cId = "",
                     const std::string& leader = "",
                     const std::vector<std::string>& nodes = {})
        : sessionId(sId),
          missionId(mId),
          corridorId(cId),
          leaderNode(leader),
          participatingNodes(nodes),
          requiredVotes(nodes.empty() ? 0 : static_cast<uint32_t>((nodes.size() / 2) + 1)),
          receivedVotes(0),
          startTimestamp(0),
          commitTimestamp(0),
          rollbackTimestamp(0),
          state(ConsensusState::PREPARING),
          votes(),
          yesVotes(0),
          noVotes(0),
          timeoutVotes(0),
          term(0),
          latencyMs(0.0) {}
};

/**
 * @brief Statistical counters and latency aggregates for distributed consensus.
 */
struct ConsensusStatistics {
    uint32_t totalSessions{0};
    uint32_t successfulSessions{0};
    uint32_t failedSessions{0};
    uint32_t rollbacks{0};
    uint32_t timeouts{0};
    uint32_t leaderChanges{0};
    double averageConsensusLatency{0.0};  ///< ms
    double averageHeartbeatLatency{0.0};  ///< ms
};

/**
 * @brief Permanent audit record for cast votes.
 */
struct VoteAuditRecord {
    std::string sessionId;
    std::string nodeId;
    NodeVote vote{NodeVote::NO};
    uint64_t timestamp{0};
    uint64_t term{0};
};

// =============================================================================
// DistributedConsensusEngine Class
// =============================================================================

/**
 * @brief Distributed Traffic Signal Consensus & Fault-Tolerant Coordination Engine.
 *
 * Guarantees that every Raspberry Pi traffic controller in an emergency corridor
 * agrees on the exact signal preemption plan before physical lamp transitions occur.
 */
class DistributedConsensusEngine {
public:
    DistributedConsensusEngine();
    ~DistributedConsensusEngine() = default;

    // Non-copyable, non-movable to guarantee thread safety
    DistributedConsensusEngine(const DistributedConsensusEngine&) = delete;
    DistributedConsensusEngine& operator=(const DistributedConsensusEngine&) = delete;
    DistributedConsensusEngine(DistributedConsensusEngine&&) = delete;
    DistributedConsensusEngine& operator=(DistributedConsensusEngine&&) = delete;

    // -------------------------------------------------------------------------
    // Node Lifecycle & Heartbeat Management
    // -------------------------------------------------------------------------
    bool RegisterNode(const ConsensusNode& node);
    bool RemoveNode(const std::string& nodeId);
    bool UpdateHeartbeat(const std::string& nodeId, uint64_t timestamp = 0);
    bool UpdateLatency(const std::string& nodeId, double latencyMs);
    bool SetNodeOffline(const std::string& nodeId);
    std::vector<std::string> DetectNodeFailure(uint64_t currentTime, uint64_t timeoutThresholdMs = 5000);
    bool RecoverNode(const std::string& nodeId, uint64_t timestamp = 0);
    bool GetNode(const std::string& nodeId, ConsensusNode& outNode) const;
    std::vector<ConsensusNode> GetAllNodes() const;

    // -------------------------------------------------------------------------
    // Leader Election
    // -------------------------------------------------------------------------
    std::string ElectLeader(const std::string& preferredNodeId = "");
    std::string GetLeader() const;
    uint64_t GetCurrentTerm() const;

    // -------------------------------------------------------------------------
    // Consensus Lifecycle
    // -------------------------------------------------------------------------
    bool StartConsensus(const std::string& sessionId,
                        const std::string& missionId,
                        const std::string& corridorId,
                        const std::vector<std::string>& participatingNodes,
                        uint64_t timestamp = 0);
    bool CastVote(const std::string& sessionId,
                  const std::string& nodeId,
                  NodeVote vote,
                  uint64_t timestamp = 0);
    bool CommitConsensus(const std::string& sessionId, uint64_t timestamp = 0);
    bool AbortConsensus(const std::string& sessionId, const std::string& reason = "");
    bool RollbackConsensus(const std::string& sessionId, uint64_t timestamp = 0);
    bool GetConsensus(const std::string& sessionId, ConsensusSession& outSession) const;
    std::vector<ConsensusSession> GetAllSessions() const;

    // -------------------------------------------------------------------------
    // Audit & Statistics
    // -------------------------------------------------------------------------
    ConsensusStatistics GetStatistics() const;
    std::vector<VoteAuditRecord> GetVoteAuditLogs() const;

    // -------------------------------------------------------------------------
    // Structured ASCII Reports
    // -------------------------------------------------------------------------
    std::string GenerateConsensusReport(const std::string& sessionId) const;
    std::string GenerateClusterReport() const;
    std::string GenerateNodeReport(const std::string& nodeId) const;

    // -------------------------------------------------------------------------
    // State Reset
    // -------------------------------------------------------------------------
    void Reset();

private:
    static uint64_t GetCurrentTimeMs();

    // Seven dedicated mutexes as strictly specified (no nested locking):
    mutable std::mutex registryMutex_;
    mutable std::mutex leaderMutex_;
    mutable std::mutex consensusMutex_;
    mutable std::mutex heartbeatMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex rollbackMutex_;

    // Data protected by registryMutex_:
    std::unordered_map<std::string, ConsensusNode> nodes_;

    // Data protected by leaderMutex_:
    std::string currentLeader_;
    uint64_t currentTerm_{1};

    // Data protected by consensusMutex_:
    std::unordered_map<std::string, ConsensusSession> sessions_;
    std::vector<VoteAuditRecord> voteAuditLogs_;

    // Data protected by heartbeatMutex_:
    std::unordered_map<std::string, uint64_t> lastHeartbeats_;
    std::unordered_map<std::string, double> heartbeatLatencies_;

    // Data protected by statisticsMutex_:
    ConsensusStatistics statistics_;
    double totalConsensusLatencySum_{0.0};
    uint32_t totalLatencySamples_{0};
    double totalHeartbeatLatencySum_{0.0};
    uint32_t totalHeartbeatSamples_{0};

    // Data protected by rollbackMutex_:
    std::unordered_map<std::string, uint64_t> rollbackHistory_;
};

} // namespace consensus

// Namespace alias for distributed_consensus
namespace distributed_consensus = consensus;

} // namespace aegis

#endif // AEGIS_DISTRIBUTED_CONSENSUS_ENGINE_H

