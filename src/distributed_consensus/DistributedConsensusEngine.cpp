/**
 * @file DistributedConsensusEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 13B - Distributed Traffic Signal Consensus & Fault-Tolerant Coordination Engine
 *
 * Implements the distributed consensus layer guaranteeing that every Raspberry Pi
 * traffic controller executes exactly the same emergency traffic plan.
 * Sits between TrafficNetworkOrchestrator (Phase 13A) and Distributed Signal Controller (Phase 10A).
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 Model B & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "DistributedConsensusEngine.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace consensus {

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(ConsensusState state) {
    switch (state) {
        case ConsensusState::PREPARING: return "PREPARING";
        case ConsensusState::VOTING:    return "VOTING";
        case ConsensusState::COMMITTED: return "COMMITTED";
        case ConsensusState::REJECTED:  return "REJECTED";
        case ConsensusState::FAILED:    return "FAILED";
        case ConsensusState::ROLLBACK:  return "ROLLBACK";
        case ConsensusState::UNKNOWN:
        default:                        return "UNKNOWN";
    }
}

std::string ToString(NodeVote vote) {
    switch (vote) {
        case NodeVote::YES:     return "YES";
        case NodeVote::NO:      return "NO";
        case NodeVote::TIMEOUT: return "TIMEOUT";
        case NodeVote::OFFLINE: return "OFFLINE";
        default:                return "NO";
    }
}

std::string ToString(NodeRole role) {
    switch (role) {
        case NodeRole::LEADER:    return "LEADER";
        case NodeRole::FOLLOWER:  return "FOLLOWER";
        case NodeRole::CANDIDATE: return "CANDIDATE";
        default:                  return "FOLLOWER";
    }
}

std::string ToString(SynchronizationHealth health) {
    switch (health) {
        case SynchronizationHealth::HEALTHY:  return "HEALTHY";
        case SynchronizationHealth::DEGRADED: return "DEGRADED";
        case SynchronizationHealth::UNSTABLE: return "UNSTABLE";
        case SynchronizationHealth::FAILED:   return "FAILED";
        default:                              return "HEALTHY";
    }
}

ConsensusState ConsensusStateFromString(const std::string& str) {
    if (str == "PREPARING") return ConsensusState::PREPARING;
    if (str == "VOTING")    return ConsensusState::VOTING;
    if (str == "COMMITTED") return ConsensusState::COMMITTED;
    if (str == "REJECTED")  return ConsensusState::REJECTED;
    if (str == "FAILED")    return ConsensusState::FAILED;
    if (str == "ROLLBACK")  return ConsensusState::ROLLBACK;
    return ConsensusState::UNKNOWN;
}

NodeVote NodeVoteFromString(const std::string& str) {
    if (str == "YES")     return NodeVote::YES;
    if (str == "NO")      return NodeVote::NO;
    if (str == "TIMEOUT") return NodeVote::TIMEOUT;
    if (str == "OFFLINE") return NodeVote::OFFLINE;
    return NodeVote::NO;
}

NodeRole NodeRoleFromString(const std::string& str) {
    if (str == "LEADER")    return NodeRole::LEADER;
    if (str == "FOLLOWER")  return NodeRole::FOLLOWER;
    if (str == "CANDIDATE") return NodeRole::CANDIDATE;
    return NodeRole::FOLLOWER;
}

SynchronizationHealth SynchronizationHealthFromString(const std::string& str) {
    if (str == "HEALTHY")  return SynchronizationHealth::HEALTHY;
    if (str == "DEGRADED") return SynchronizationHealth::DEGRADED;
    if (str == "UNSTABLE") return SynchronizationHealth::UNSTABLE;
    if (str == "FAILED")   return SynchronizationHealth::FAILED;
    return SynchronizationHealth::HEALTHY;
}

// =============================================================================
// Constructor
// =============================================================================

DistributedConsensusEngine::DistributedConsensusEngine() = default;

uint64_t DistributedConsensusEngine::GetCurrentTimeMs() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// =============================================================================
// Node Lifecycle & Heartbeat Management
// =============================================================================

bool DistributedConsensusEngine::RegisterNode(const ConsensusNode& node) {
    if (node.nodeId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        if (nodes_.find(node.nodeId) != nodes_.end()) {
            return false; // Prevent duplicate registration
        }
        nodes_[node.nodeId] = node;
    }

    // Rule 1: Exactly one leader may exist.
    // If no leader exists currently, elect one deterministically.
    {
        std::lock_guard<std::mutex> lock(leaderMutex_);
        if (currentLeader_.empty() && node.online) {
            // Trigger election outside of leaderMutex_ lock
        } else {
            return true;
        }
    }

    ElectLeader();
    return true;
}

bool DistributedConsensusEngine::RemoveNode(const std::string& nodeId) {
    bool wasLeader = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return false;
        }
        wasLeader = (it->second.role == NodeRole::LEADER);
        nodes_.erase(it);
    }

    if (wasLeader) {
        {
            std::lock_guard<std::mutex> lock(leaderMutex_);
            if (currentLeader_ == nodeId) {
                currentLeader_.clear();
            }
        }
        // Rule 5: Leader failure / removal triggers new election
        ElectLeader();
    }

    return true;
}

bool DistributedConsensusEngine::UpdateHeartbeat(const std::string& nodeId, uint64_t timestamp) {
    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    double latency = 10.0;
    {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        auto it = lastHeartbeats_.find(nodeId);
        if (it != lastHeartbeats_.end() && timestamp >= it->second && it->second > 0) {
            latency = static_cast<double>(timestamp - it->second);
        }
        lastHeartbeats_[nodeId] = timestamp;
        heartbeatLatencies_[nodeId] = latency;
    }

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return false;
        }
        it->second.lastHeartbeat = timestamp;
        it->second.online = true;
        it->second.latencyMs = latency;

        // Rule 7: Heartbeats determine node health
        if (latency < 50.0) {
            it->second.health = SynchronizationHealth::HEALTHY;
        } else if (latency < 150.0) {
            it->second.health = SynchronizationHealth::DEGRADED;
        } else if (latency < 300.0) {
            it->second.health = SynchronizationHealth::UNSTABLE;
        } else {
            it->second.health = SynchronizationHealth::FAILED;
        }
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        totalHeartbeatLatencySum_ += latency;
        totalHeartbeatSamples_++;
        statistics_.averageHeartbeatLatency = totalHeartbeatLatencySum_ / totalHeartbeatSamples_;
    }

    return true;
}

bool DistributedConsensusEngine::UpdateLatency(const std::string& nodeId, double latencyMs) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    it->second.latencyMs = latencyMs;
    // Rule 7: Update node health based on latency
    if (latencyMs < 50.0) {
        it->second.health = SynchronizationHealth::HEALTHY;
    } else if (latencyMs < 150.0) {
        it->second.health = SynchronizationHealth::DEGRADED;
    } else if (latencyMs < 300.0) {
        it->second.health = SynchronizationHealth::UNSTABLE;
    } else {
        it->second.health = SynchronizationHealth::FAILED;
    }
    return true;
}

bool DistributedConsensusEngine::SetNodeOffline(const std::string& nodeId) {
    bool wasLeader = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return false;
        }
        it->second.online = false;
        it->second.health = SynchronizationHealth::FAILED;
        wasLeader = (it->second.role == NodeRole::LEADER);
        it->second.role = NodeRole::FOLLOWER;
    }

    if (wasLeader) {
        {
            std::lock_guard<std::mutex> lock(leaderMutex_);
            if (currentLeader_ == nodeId) {
                currentLeader_.clear();
            }
        }
        // Rule 5: Leader failure triggers new election
        ElectLeader();
    }

    return true;
}

std::vector<std::string> DistributedConsensusEngine::DetectNodeFailure(uint64_t currentTime, uint64_t timeoutThresholdMs) {
    std::vector<std::string> failedNodes;
    bool leaderFailed = false;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        for (auto& pair : nodes_) {
            if (!pair.second.online) continue;
            if (pair.second.lastHeartbeat > 0 &&
                currentTime > pair.second.lastHeartbeat &&
                (currentTime - pair.second.lastHeartbeat) > timeoutThresholdMs) {
                pair.second.online = false;
                pair.second.health = SynchronizationHealth::FAILED;
                failedNodes.push_back(pair.first);
                if (pair.second.role == NodeRole::LEADER) {
                    pair.second.role = NodeRole::FOLLOWER;
                    leaderFailed = true;
                }
            }
        }
    }

    if (leaderFailed) {
        {
            std::lock_guard<std::mutex> lock(leaderMutex_);
            currentLeader_.clear();
        }
        // Rule 5: Leader failure immediately triggers new election
        ElectLeader();
    }

    return failedNodes;
}

bool DistributedConsensusEngine::RecoverNode(const std::string& nodeId, uint64_t timestamp) {
    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    bool recovered = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it != nodes_.end()) {
            it->second.online = true;
            it->second.health = SynchronizationHealth::HEALTHY;
            it->second.lastHeartbeat = timestamp;
            it->second.latencyMs = 10.0;
            it->second.vote = NodeVote::NO;
            recovered = true;
        }
    }

    if (recovered) {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        lastHeartbeats_[nodeId] = timestamp;
        heartbeatLatencies_[nodeId] = 10.0;
    }

    return recovered;
}

bool DistributedConsensusEngine::GetNode(const std::string& nodeId, ConsensusNode& outNode) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = nodes_.find(nodeId);
    if (it == nodes_.end()) {
        return false;
    }
    outNode = it->second;
    return true;
}

std::vector<ConsensusNode> DistributedConsensusEngine::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<ConsensusNode> result;
    result.reserve(nodes_.size());
    for (const auto& pair : nodes_) {
        result.push_back(pair.second);
    }
    return result;
}

// =============================================================================
// Leader Election
// =============================================================================

std::string DistributedConsensusEngine::ElectLeader(const std::string& preferredNodeId) {
    std::vector<ConsensusNode> candidates;

    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        for (const auto& pair : nodes_) {
            if (pair.second.online && pair.second.health != SynchronizationHealth::FAILED) {
                candidates.push_back(pair.second);
            }
        }
    }

    if (candidates.empty()) {
        std::lock_guard<std::mutex> lock(leaderMutex_);
        currentLeader_.clear();
        return "";
    }

    // Sort candidates deterministically:
    // 1. Preferred node ID if valid and online
    // 2. Highest priority score
    // 3. Health ranking (HEALTHY > DEGRADED > UNSTABLE > FAILED)
    // 4. Lowest latency
    // 5. Lexicographical tie-break on nodeId
    std::sort(candidates.begin(), candidates.end(),
              [&preferredNodeId](const ConsensusNode& a, const ConsensusNode& b) {
                  if (!preferredNodeId.empty()) {
                      if (a.nodeId == preferredNodeId && b.nodeId != preferredNodeId) return true;
                      if (b.nodeId == preferredNodeId && a.nodeId != preferredNodeId) return false;
                  }
                  if (a.priority != b.priority) {
                      return a.priority > b.priority;
                  }
                  auto healthScore = [](SynchronizationHealth h) -> int {
                      switch (h) {
                          case SynchronizationHealth::HEALTHY:  return 4;
                          case SynchronizationHealth::DEGRADED: return 3;
                          case SynchronizationHealth::UNSTABLE: return 2;
                          case SynchronizationHealth::FAILED:   return 1;
                          default:                              return 0;
                      }
                  };
                  if (healthScore(a.health) != healthScore(b.health)) {
                      return healthScore(a.health) > healthScore(b.health);
                  }
                  if (a.latencyMs != b.latencyMs) {
                      return a.latencyMs < b.latencyMs;
                  }
                  return a.nodeId < b.nodeId;
              });

    std::string electedLeader = candidates.front().nodeId;
    bool leaderChanged = false;
    uint64_t term = 1;

    {
        std::lock_guard<std::mutex> lock(leaderMutex_);
        leaderChanged = (currentLeader_ != electedLeader);
        if (leaderChanged) {
            currentTerm_++;
            currentLeader_ = electedLeader;
        }
        term = currentTerm_;
    }

    // Rule 1: Exactly one leader may exist. Update node roles in registry.
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        for (auto& pair : nodes_) {
            if (pair.first == electedLeader) {
                pair.second.role = NodeRole::LEADER;
                pair.second.currentTerm = term;
            } else {
                pair.second.role = NodeRole::FOLLOWER;
            }
        }
    }

    if (leaderChanged) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.leaderChanges++;
    }

    return electedLeader;
}

std::string DistributedConsensusEngine::GetLeader() const {
    std::lock_guard<std::mutex> lock(leaderMutex_);
    return currentLeader_;
}

uint64_t DistributedConsensusEngine::GetCurrentTerm() const {
    std::lock_guard<std::mutex> lock(leaderMutex_);
    return currentTerm_;
}

// =============================================================================
// Consensus Lifecycle
// =============================================================================

bool DistributedConsensusEngine::StartConsensus(const std::string& sessionId,
                                               const std::string& missionId,
                                               const std::string& corridorId,
                                               const std::vector<std::string>& participatingNodes,
                                               uint64_t timestamp) {
    if (sessionId.empty() || participatingNodes.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        if (sessions_.find(sessionId) != sessions_.end()) {
            return false; // Session ID must be unique
        }
    }

    std::string leader = GetLeader();
    uint64_t term = GetCurrentTerm();

    ConsensusSession session(sessionId, missionId, corridorId, leader, participatingNodes);
    session.startTimestamp = timestamp;
    session.state = ConsensusState::PREPARING;
    session.term = term;

    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        sessions_[sessionId] = session;
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.totalSessions++;
    }

    return true;
}

bool DistributedConsensusEngine::CastVote(const std::string& sessionId,
                                         const std::string& nodeId,
                                         NodeVote vote,
                                         uint64_t timestamp) {
    if (sessionId.empty() || nodeId.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    // Rule 3: Offline nodes cannot vote
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end() || !it->second.online) {
            return false;
        }
    }

    bool isTimeout = (vote == NodeVote::TIMEOUT);
    uint64_t term = 0;

    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        // Rule 10: Committed sessions become immutable
        if (it->second.state == ConsensusState::COMMITTED) {
            return false;
        }

        // If session was already rolled back or rejected/failed, reject new votes
        if (it->second.state == ConsensusState::ROLLBACK ||
            it->second.state == ConsensusState::FAILED ||
            it->second.state == ConsensusState::REJECTED) {
            return false;
        }

        // Check node participation
        bool isParticipant = false;
        for (const auto& p : it->second.participatingNodes) {
            if (p == nodeId) {
                isParticipant = true;
                break;
            }
        }
        if (!isParticipant) {
            return false;
        }

        // Prevent duplicate voting in the same session
        if (it->second.votes.find(nodeId) != it->second.votes.end()) {
            return false;
        }

        it->second.votes[nodeId] = vote;
        it->second.receivedVotes++;
        if (vote == NodeVote::YES) {
            it->second.yesVotes++;
        } else if (vote == NodeVote::NO) {
            it->second.noVotes++;
        } else if (vote == NodeVote::TIMEOUT) {
            it->second.timeoutVotes++;
        }

        if (it->second.state == ConsensusState::PREPARING) {
            it->second.state = ConsensusState::VOTING;
        }

        term = it->second.term;

        // Rule 9: Every vote is permanently logged
        VoteAuditRecord audit;
        audit.sessionId = sessionId;
        audit.nodeId = nodeId;
        audit.vote = vote;
        audit.timestamp = timestamp;
        audit.term = term;
        voteAuditLogs_.push_back(audit);

        // Rule 4: Timeout automatically aborts consensus
        if (isTimeout) {
            it->second.state = ConsensusState::FAILED;
        }
    }

    // Update node's latest vote in registry
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it != nodes_.end()) {
            it->second.vote = vote;
        }
    }

    if (isTimeout) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.timeouts++;
        statistics_.failedSessions++;
    }

    return true;
}

bool DistributedConsensusEngine::CommitConsensus(const std::string& sessionId, uint64_t timestamp) {
    if (sessionId.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    bool success = false;
    double latency = 0.0;
    std::vector<std::string> participants;

    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        // Rule 10: Committed sessions become immutable
        if (it->second.state == ConsensusState::COMMITTED) {
            return false;
        }

        if (it->second.state == ConsensusState::ROLLBACK ||
            it->second.state == ConsensusState::FAILED) {
            return false;
        }

        // Rule 2: Consensus requires majority vote
        if (it->second.yesVotes < it->second.requiredVotes) {
            it->second.state = ConsensusState::REJECTED;
            success = false;
        } else {
            it->second.state = ConsensusState::COMMITTED;
            it->second.commitTimestamp = timestamp;
            latency = (timestamp >= it->second.startTimestamp)
                          ? static_cast<double>(timestamp - it->second.startTimestamp)
                          : 0.0;
            it->second.latencyMs = latency;
            participants = it->second.participatingNodes;
            success = true;
        }
    }

    if (success) {
        {
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_.successfulSessions++;
            totalConsensusLatencySum_ += latency;
            totalLatencySamples_++;
            statistics_.averageConsensusLatency = totalConsensusLatencySum_ / totalLatencySamples_;
        }
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            for (const auto& nid : participants) {
                auto it = nodes_.find(nid);
                if (it != nodes_.end()) {
                    it->second.lastCommitIndex++;
                }
            }
        }
    } else {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.failedSessions++;
    }

    return success;
}

bool DistributedConsensusEngine::AbortConsensus(const std::string& sessionId, const std::string& reason) {
    if (sessionId.empty()) {
        return false;
    }

    bool aborted = false;
    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }

        // Rule 10: Committed sessions are immutable
        if (it->second.state == ConsensusState::COMMITTED) {
            return false;
        }

        it->second.state = ConsensusState::FAILED;
        aborted = true;
    }

    if (aborted) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_.failedSessions++;
        if (reason.find("timeout") != std::string::npos || reason.find("TIMEOUT") != std::string::npos) {
            statistics_.timeouts++;
        }
    }

    return aborted;
}

bool DistributedConsensusEngine::RollbackConsensus(const std::string& sessionId, uint64_t timestamp) {
    if (sessionId.empty()) {
        return false;
    }

    if (timestamp == 0) {
        timestamp = GetCurrentTimeMs();
    }

    bool rolledBack = false;
    std::vector<std::string> participants;

    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return false;
        }
        it->second.state = ConsensusState::ROLLBACK;
        it->second.rollbackTimestamp = timestamp;
        participants = it->second.participatingNodes;
        rolledBack = true;
    }

    if (rolledBack) {
        {
            std::lock_guard<std::mutex> lock(rollbackMutex_);
            rollbackHistory_[sessionId] = timestamp;
        }
        {
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            statistics_.rollbacks++;
        }
        // Rule 6: Rollback restores previous stable signal state
        {
            std::lock_guard<std::mutex> lock(registryMutex_);
            for (const auto& nid : participants) {
                auto it = nodes_.find(nid);
                if (it != nodes_.end()) {
                    if (it->second.lastCommitIndex > 0) {
                        it->second.lastCommitIndex--;
                    }
                    it->second.vote = NodeVote::NO;
                }
            }
        }
    }

    return rolledBack;
}

bool DistributedConsensusEngine::GetConsensus(const std::string& sessionId, ConsensusSession& outSession) const {
    std::lock_guard<std::mutex> lock(consensusMutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return false;
    }
    outSession = it->second;
    return true;
}

std::vector<ConsensusSession> DistributedConsensusEngine::GetAllSessions() const {
    std::lock_guard<std::mutex> lock(consensusMutex_);
    std::vector<ConsensusSession> result;
    result.reserve(sessions_.size());
    for (const auto& pair : sessions_) {
        result.push_back(pair.second);
    }
    return result;
}

// =============================================================================
// Audit & Statistics
// =============================================================================

ConsensusStatistics DistributedConsensusEngine::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    return statistics_;
}

std::vector<VoteAuditRecord> DistributedConsensusEngine::GetVoteAuditLogs() const {
    std::lock_guard<std::mutex> lock(consensusMutex_);
    return voteAuditLogs_;
}

// =============================================================================
// Reports
// =============================================================================

std::string DistributedConsensusEngine::GenerateConsensusReport(const std::string& sessionId) const {
    ConsensusSession session;
    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        auto it = sessions_.find(sessionId);
        if (it == sessions_.end()) {
            return "";
        }
        session = it->second;
    }

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[CONSENSUS]\n\n";
    oss << "Session:\n" << session.sessionId << "\n\n";
    oss << "Leader:\n" << (session.leaderNode.empty() ? "NONE" : session.leaderNode) << "\n\n";
    oss << "Mission:\n" << session.missionId << "\n\n";
    oss << "Corridor:\n" << session.corridorId << "\n\n";
    oss << "Votes:\n" << session.yesVotes << "/" << session.requiredVotes << "\n\n";
    oss << "State:\n" << ToString(session.state) << "\n\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Latency:\n" << session.latencyMs << " ms\n";
    return oss.str();
}

std::string DistributedConsensusEngine::GenerateClusterReport() const {
    std::string leader;
    {
        std::lock_guard<std::mutex> lock(leaderMutex_);
        leader = currentLeader_;
    }

    uint32_t followers = 0;
    uint32_t healthy = 0;
    uint32_t offline = 0;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        for (const auto& pair : nodes_) {
            if (pair.second.role == NodeRole::FOLLOWER) {
                followers++;
            }
            if (pair.second.health == SynchronizationHealth::HEALTHY) {
                healthy++;
            }
            if (!pair.second.online) {
                offline++;
            }
        }
    }

    ConsensusStatistics stats;
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats = statistics_;
    }

    uint32_t successRate = 100;
    if (stats.totalSessions > 0) {
        successRate = static_cast<uint32_t>((static_cast<double>(stats.successfulSessions) / stats.totalSessions) * 100.0);
    }

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[CLUSTER]\n\n";
    oss << "Cluster:\nSMART_AEGIS_CONSENSUS\n\n";
    oss << "Leader:\n" << (leader.empty() ? "NONE" : leader) << "\n\n";
    oss << "Followers:\n" << followers << "\n\n";
    oss << "Healthy Nodes:\n" << healthy << "\n\n";
    oss << "Offline Nodes:\n" << offline << "\n\n";
    oss << "Consensus Success:\n" << successRate << "%\n\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Average Latency:\n" << stats.averageConsensusLatency << " ms\n";
    return oss.str();
}

std::string DistributedConsensusEngine::GenerateNodeReport(const std::string& nodeId) const {
    ConsensusNode node;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = nodes_.find(nodeId);
        if (it == nodes_.end()) {
            return "";
        }
        node = it->second;
    }

    std::lock_guard<std::mutex> lock(reportMutex_);
    std::ostringstream oss;
    oss << "[NODE]\n\n";
    oss << "Node:\n" << node.nodeId << "\n\n";
    oss << "Intersection:\n" << node.intersectionId << "\n\n";
    oss << "Role:\n" << ToString(node.role) << "\n\n";
    oss << "Heartbeat:\n" << node.lastHeartbeat << "\n\n";
    oss << "Health:\n" << ToString(node.health) << "\n\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Latency:\n" << node.latencyMs << " ms\n\n";
    oss << "Vote:\n" << ToString(node.vote) << "\n";
    return oss.str();
}

// =============================================================================
// State Reset
// =============================================================================

void DistributedConsensusEngine::Reset() {
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        nodes_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(leaderMutex_);
        currentLeader_.clear();
        currentTerm_ = 1;
    }
    {
        std::lock_guard<std::mutex> lock(consensusMutex_);
        sessions_.clear();
        voteAuditLogs_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(heartbeatMutex_);
        lastHeartbeats_.clear();
        heartbeatLatencies_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        statistics_ = ConsensusStatistics{};
        totalConsensusLatencySum_ = 0.0;
        totalLatencySamples_ = 0;
        totalHeartbeatLatencySum_ = 0.0;
        totalHeartbeatSamples_ = 0;
    }
    {
        std::lock_guard<std::mutex> lock(rollbackMutex_);
        rollbackHistory_.clear();
    }
}

} // namespace consensus
} // namespace aegis

