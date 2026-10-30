/**
 * @file TrafficNetworkOrchestrator.cpp
 * @brief SMART AEGIS (AERS) Phase 13A - City-Wide Distributed Traffic Network Orchestration Engine
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "traffic_network_orchestrator/TrafficNetworkOrchestrator.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis {
namespace traffic_network {

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(IntersectionStatus status) {
    switch (status) {
        case IntersectionStatus::ONLINE:      return "ONLINE";
        case IntersectionStatus::OFFLINE:     return "OFFLINE";
        case IntersectionStatus::FAILED:      return "FAILED";
        case IntersectionStatus::RECOVERING:  return "RECOVERING";
        case IntersectionStatus::MAINTENANCE: return "MAINTENANCE";
        default:                              return "ONLINE";
    }
}

std::string ToString(SignalExecutionState state) {
    switch (state) {
        case SignalExecutionState::IDLE:       return "IDLE";
        case SignalExecutionState::WAITING:    return "WAITING";
        case SignalExecutionState::PREPARING:  return "PREPARING";
        case SignalExecutionState::GREEN:      return "GREEN";
        case SignalExecutionState::TRANSITION: return "TRANSITION";
        case SignalExecutionState::RESTORING:  return "RESTORING";
        case SignalExecutionState::FAILED:     return "FAILED";
        default:                               return "IDLE";
    }
}

std::string ToString(SynchronizationState state) {
    switch (state) {
        case SynchronizationState::READY:     return "READY";
        case SynchronizationState::EXECUTING: return "EXECUTING";
        case SynchronizationState::WAITING:   return "WAITING";
        case SynchronizationState::COMPLETED: return "COMPLETED";
        case SynchronizationState::FAILED:    return "FAILED";
        default:                              return "READY";
    }
}

IntersectionStatus IntersectionStatusFromString(const std::string& str) {
    if (str == "OFFLINE")     return IntersectionStatus::OFFLINE;
    if (str == "FAILED")      return IntersectionStatus::FAILED;
    if (str == "RECOVERING")  return IntersectionStatus::RECOVERING;
    if (str == "MAINTENANCE") return IntersectionStatus::MAINTENANCE;
    return IntersectionStatus::ONLINE;
}

SignalExecutionState SignalExecutionStateFromString(const std::string& str) {
    if (str == "WAITING")    return SignalExecutionState::WAITING;
    if (str == "PREPARING")  return SignalExecutionState::PREPARING;
    if (str == "GREEN")      return SignalExecutionState::GREEN;
    if (str == "TRANSITION") return SignalExecutionState::TRANSITION;
    if (str == "RESTORING")  return SignalExecutionState::RESTORING;
    if (str == "FAILED")     return SignalExecutionState::FAILED;
    return SignalExecutionState::IDLE;
}

SynchronizationState SynchronizationStateFromString(const std::string& str) {
    if (str == "EXECUTING") return SynchronizationState::EXECUTING;
    if (str == "WAITING")   return SynchronizationState::WAITING;
    if (str == "COMPLETED") return SynchronizationState::COMPLETED;
    if (str == "FAILED")    return SynchronizationState::FAILED;
    return SynchronizationState::READY;
}

// =============================================================================
// Time Utility
// =============================================================================

uint64_t TrafficNetworkOrchestrator::GetCurrentTimeMs() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

// =============================================================================
// Constructor & Destructor
// =============================================================================

TrafficNetworkOrchestrator::TrafficNetworkOrchestrator() {
    statistics_ = TrafficNetworkStatistics{};
}

// =============================================================================
// Intersection Lifecycle & Topology Management
// =============================================================================

bool TrafficNetworkOrchestrator::RegisterIntersection(const IntersectionNode& node) {
    if (node.intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (intersections_.find(node.intersectionId) != intersections_.end()) {
        return false;
    }

    IntersectionNode n = node;
    if (n.lastHeartbeat == 0) {
        n.lastHeartbeat = GetCurrentTimeMs();
    }
    intersections_[n.intersectionId] = n;
    return true;
}

bool TrafficNetworkOrchestrator::RemoveIntersection(const std::string& intersectionId) {
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = intersections_.find(intersectionId);
        if (it == intersections_.end()) {
            return false;
        }

        // Remove from topology neighbor lists
        for (auto& kv : intersections_) {
            auto& nbrs = kv.second.neighbors;
            nbrs.erase(std::remove(nbrs.begin(), nbrs.end(), intersectionId), nbrs.end());
        }

        intersections_.erase(it);
    }

    // Remove from corridors
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        for (auto& kv : corridors_) {
            auto& nodes = kv.second.intersections;
            nodes.erase(std::remove(nodes.begin(), nodes.end(), intersectionId), nodes.end());
        }
    }

    // Remove from groups
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        for (auto& kv : groups_) {
            auto& nodes = kv.second.intersections;
            nodes.erase(std::remove(nodes.begin(), nodes.end(), intersectionId), nodes.end());
            if (kv.second.leaderIntersection == intersectionId) {
                kv.second.leaderIntersection = nodes.empty() ? "" : nodes.front();
            }
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::UpdateHeartbeat(const std::string& intersectionId, uint64_t timestamp) {
    uint64_t now = (timestamp != 0) ? timestamp : GetCurrentTimeMs();
    bool wasOffline = false;
    uint64_t prevHb = 0;

    {
        std::lock_guard<std::mutex> hbLock(heartbeatsMutex_);
        std::lock_guard<std::mutex> regLock(registryMutex_);

        auto it = intersections_.find(intersectionId);
        if (it == intersections_.end()) {
            return false;
        }

        wasOffline = (!it->second.online || it->second.status == IntersectionStatus::OFFLINE);
        prevHb = it->second.lastHeartbeat;
        it->second.lastHeartbeat = now;

        // Automatic node recovery upon receiving heartbeat
        if (wasOffline) {
            it->second.online = true;
            it->second.status = IntersectionStatus::ONLINE;
        }
    }

    if (wasOffline && now > prevHb && prevHb != 0) {
        double recTime = static_cast<double>(now - prevHb);
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        totalRecoveryTimeMs_ += recTime;
        recoveryCount_++;
    }

    return true;
}

bool TrafficNetworkOrchestrator::SetIntersectionOnline(const std::string& intersectionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }

    it->second.online = true;
    it->second.status = IntersectionStatus::ONLINE;
    it->second.lastHeartbeat = GetCurrentTimeMs();
    return true;
}

bool TrafficNetworkOrchestrator::SetIntersectionOffline(const std::string& intersectionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }

    it->second.online = false;
    it->second.status = IntersectionStatus::OFFLINE;
    if (it->second.signalState == SignalExecutionState::GREEN ||
        it->second.signalState == SignalExecutionState::PREPARING) {
        it->second.signalState = SignalExecutionState::FAILED;
    }
    return true;
}

bool TrafficNetworkOrchestrator::AssignMission(const std::string& intersectionId, const std::string& missionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }

    it->second.currentMission = missionId;
    return true;
}

bool TrafficNetworkOrchestrator::AddNeighbor(const std::string& intersectionId, const std::string& neighborId) {
    if (intersectionId == neighborId) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it1 = intersections_.find(intersectionId);
    auto it2 = intersections_.find(neighborId);
    if (it1 == intersections_.end() || it2 == intersections_.end()) {
        return false;
    }

    auto& nbrs1 = it1->second.neighbors;
    if (std::find(nbrs1.begin(), nbrs1.end(), neighborId) == nbrs1.end()) {
        nbrs1.push_back(neighborId);
    }

    auto& nbrs2 = it2->second.neighbors;
    if (std::find(nbrs2.begin(), nbrs2.end(), intersectionId) == nbrs2.end()) {
        nbrs2.push_back(intersectionId);
    }

    return true;
}

std::vector<std::string> TrafficNetworkOrchestrator::GetNeighbors(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) {
        return it->second.neighbors;
    }
    return {};
}

// =============================================================================
// Corridor Lifecycle & Membership
// =============================================================================

bool TrafficNetworkOrchestrator::CreateCorridor(const TrafficCorridor& corridor) {
    if (corridor.corridorId.empty()) {
        return false;
    }

    TrafficCorridor corr = corridor;
    if (corr.creationTime == 0) {
        corr.creationTime = GetCurrentTimeMs();
    }
    if (corr.leaderIntersection.empty() && !corr.intersections.empty()) {
        corr.leaderIntersection = corr.intersections.front();
    }

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        if (corridors_.find(corr.corridorId) != corridors_.end()) {
            return false;
        }
        corridors_[corr.corridorId] = corr;
    }

    // Automatically instantiate matching synchronization group
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        SynchronizationGroup grp(corr.corridorId, corr.intersections, corr.leaderIntersection);
        groups_[corr.corridorId] = grp;
    }

    return true;
}

bool TrafficNetworkOrchestrator::ActivateCorridor(const std::string& corridorId) {
    std::vector<std::string> memberNodes;
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end()) {
            return false;
        }
        it->second.active = true;
        it->second.activationTime = now;
        it->second.state = SignalExecutionState::PREPARING;
        memberNodes = it->second.intersections;
    }

    // Update member intersections
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : memberNodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                nodeIt->second.currentCorridor = corridorId;
                nodeIt->second.signalState = SignalExecutionState::WAITING;
            }
        }
    }

    // Update group state
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            grpIt->second.state = SynchronizationState::EXECUTING;
            grpIt->second.lastSyncTimestamp = now;
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::DeactivateCorridor(const std::string& corridorId) {
    std::vector<std::string> memberNodes;
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end()) {
            return false;
        }
        it->second.active = false;
        it->second.completionTime = now;
        it->second.state = SignalExecutionState::IDLE;
        memberNodes = it->second.intersections;
    }

    // Restore member intersections
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : memberNodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                nodeIt->second.currentCorridor.clear();
                nodeIt->second.signalState = SignalExecutionState::IDLE;
            }
        }
    }

    // Update group state
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            grpIt->second.state = SynchronizationState::READY;
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::AssignIntersectionToCorridor(const std::string& corridorId, const std::string& intersectionId) {
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end()) {
            return false;
        }
        auto& nodes = it->second.intersections;
        if (std::find(nodes.begin(), nodes.end(), intersectionId) == nodes.end()) {
            nodes.push_back(intersectionId);
        }
    }

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto nodeIt = intersections_.find(intersectionId);
        if (nodeIt != intersections_.end()) {
            nodeIt->second.currentCorridor = corridorId;
        }
    }

    // Update sync group
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            auto& grpNodes = grpIt->second.intersections;
            if (std::find(grpNodes.begin(), grpNodes.end(), intersectionId) == grpNodes.end()) {
                grpNodes.push_back(intersectionId);
            }
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::RemoveIntersectionFromCorridor(const std::string& corridorId, const std::string& intersectionId) {
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end()) {
            return false;
        }
        auto& nodes = it->second.intersections;
        nodes.erase(std::remove(nodes.begin(), nodes.end(), intersectionId), nodes.end());
    }

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto nodeIt = intersections_.find(intersectionId);
        if (nodeIt != intersections_.end() && nodeIt->second.currentCorridor == corridorId) {
            nodeIt->second.currentCorridor.clear();
        }
    }

    return true;
}

// =============================================================================
// Synchronization Groups & Distributed Execution
// =============================================================================

bool TrafficNetworkOrchestrator::CreateSynchronizationGroup(const std::string& groupId,
                                                           const std::vector<std::string>& intersections,
                                                           const std::string& leader) {
    if (groupId.empty()) {
        return false;
    }

    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        if (groups_.find(groupId) != groups_.end()) {
            return false;
        }
        SynchronizationGroup grp(groupId, intersections, leader);
        groups_[groupId] = grp;
    }

    if (leader.empty() && !intersections.empty()) {
        ElectLeader(groupId);
    }

    return true;
}

bool TrafficNetworkOrchestrator::AddIntersectionToGroup(const std::string& groupId, const std::string& intersectionId) {
    std::lock_guard<std::mutex> grpLock(groupsMutex_);
    auto it = groups_.find(groupId);
    if (it == groups_.end()) {
        return false;
    }

    auto& nodes = it->second.intersections;
    if (std::find(nodes.begin(), nodes.end(), intersectionId) == nodes.end()) {
        nodes.push_back(intersectionId);
    }

    if (it->second.leaderIntersection.empty()) {
        it->second.leaderIntersection = intersectionId;
    }

    return true;
}

bool TrafficNetworkOrchestrator::RemoveIntersectionFromGroup(const std::string& groupId, const std::string& intersectionId) {
    bool needElection = false;
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto it = groups_.find(groupId);
        if (it == groups_.end()) {
            return false;
        }

        auto& nodes = it->second.intersections;
        nodes.erase(std::remove(nodes.begin(), nodes.end(), intersectionId), nodes.end());

        if (it->second.leaderIntersection == intersectionId) {
            needElection = true;
        }
    }

    if (needElection) {
        ElectLeader(groupId);
    }

    return true;
}

std::string TrafficNetworkOrchestrator::ElectLeader(const std::string& groupId) {
    std::vector<std::string> members;
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto it = groups_.find(groupId);
        if (it == groups_.end() || it->second.intersections.empty()) {
            return "";
        }
        members = it->second.intersections;
    }

    std::string bestLeader;
    uint32_t bestPriority = 0;
    double bestHealth = -1.0;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : members) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end() && nodeIt->second.online &&
                nodeIt->second.status == IntersectionStatus::ONLINE) {
                if (nodeIt->second.priority > bestPriority ||
                    (nodeIt->second.priority == bestPriority && nodeIt->second.healthScore > bestHealth) ||
                    (nodeIt->second.priority == bestPriority && nodeIt->second.healthScore == bestHealth && (bestLeader.empty() || nodeId < bestLeader))) {
                    bestLeader = nodeId;
                    bestPriority = nodeIt->second.priority;
                    bestHealth = nodeIt->second.healthScore;
                }
            }
        }
    }

    // Fallback if all offline: pick first member
    if (bestLeader.empty() && !members.empty()) {
        bestLeader = members.front();
    }

    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto it = groups_.find(groupId);
        if (it != groups_.end()) {
            it->second.leaderIntersection = bestLeader;
        }
    }

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto corrIt = corridors_.find(groupId);
        if (corrIt != corridors_.end()) {
            corrIt->second.leaderIntersection = bestLeader;
        }
    }

    return bestLeader;
}

// =============================================================================
// Green Wave Execution, Restoration & Rollback
// =============================================================================

bool TrafficNetworkOrchestrator::ExecuteGreenWave(const std::string& corridorId) {
    std::lock_guard<std::mutex> execLock(executionMutex_);
    std::vector<std::string> memberNodes;

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end() || it->second.intersections.empty()) {
            return false;
        }
        memberNodes = it->second.intersections;
    }

    // Check for offline or failed intersections
    bool allHealthy = true;
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : memberNodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt == intersections_.end() || !nodeIt->second.online ||
                nodeIt->second.status == IntersectionStatus::OFFLINE ||
                nodeIt->second.status == IntersectionStatus::FAILED) {
                allHealthy = false;
                break;
            }
        }
    }

    // If an intersection is down, trigger rollback and fail green wave
    if (!allHealthy) {
        RollbackExecution(corridorId);
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        statistics_.failedSynchronizations++;
        totalGreenWavesExecuted_++;
        statistics_.greenWaveSuccessRate = (totalGreenWavesExecuted_ > 0)
            ? (static_cast<double>(successfulGreenWaves_) / static_cast<double>(totalGreenWavesExecuted_) * 100.0)
            : 100.0;
        return false;
    }

    // Execute green preemption across all corridor junctions
    uint64_t now = GetCurrentTimeMs();
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : memberNodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                nodeIt->second.signalState = SignalExecutionState::GREEN;
                nodeIt->second.currentSignalPhase = "GREEN";
                nodeIt->second.currentCorridor = corridorId;
                nodeIt->second.executionLatency = 1.25; // Standard green wave dispatch latency in ms
            }
        }
    }

    // Update corridor state
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it != corridors_.end()) {
            it->second.state = SignalExecutionState::GREEN;
            it->second.active = true;
            it->second.activationTime = now;
        }
    }

    // Update group state
    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            grpIt->second.state = SynchronizationState::COMPLETED;
            grpIt->second.lastSyncTimestamp = now;
        }
    }

    // Update Statistics
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        totalGreenWavesExecuted_++;
        successfulGreenWaves_++;
        totalExecutionLatencyMs_ += (1.25 * static_cast<double>(memberNodes.size()));
        latencyMeasurementCount_ += memberNodes.size();
        statistics_.greenWaveSuccessRate = (totalGreenWavesExecuted_ > 0)
            ? (static_cast<double>(successfulGreenWaves_) / static_cast<double>(totalGreenWavesExecuted_) * 100.0)
            : 100.0;
    }

    return true;
}

bool TrafficNetworkOrchestrator::ExecuteIntersection(const std::string& intersectionId,
                                                    SignalExecutionState state,
                                                    double latencyMs) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }

    it->second.signalState = state;
    if (state == SignalExecutionState::GREEN) {
        it->second.currentSignalPhase = "GREEN";
    } else if (state == SignalExecutionState::IDLE) {
        it->second.currentSignalPhase = "RED";
    }

    if (latencyMs > 0.0) {
        it->second.executionLatency = latencyMs;
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        totalExecutionLatencyMs_ += latencyMs;
        latencyMeasurementCount_++;
    }

    return true;
}

bool TrafficNetworkOrchestrator::RestoreIntersection(const std::string& intersectionId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it == intersections_.end()) {
        return false;
    }

    it->second.signalState = SignalExecutionState::IDLE;
    it->second.currentSignalPhase = "RED";
    it->second.currentCorridor.clear();
    it->second.currentMission.clear();
    if (it->second.status == IntersectionStatus::RECOVERING) {
        it->second.status = IntersectionStatus::ONLINE;
    }
    return true;
}

bool TrafficNetworkOrchestrator::RestoreCorridor(const std::string& corridorId) {
    std::vector<std::string> nodes;
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it == corridors_.end()) {
            return false;
        }
        nodes = it->second.intersections;
        it->second.state = SignalExecutionState::IDLE;
        it->second.active = false;
        it->second.completionTime = GetCurrentTimeMs();
    }

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : nodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                nodeIt->second.signalState = SignalExecutionState::IDLE;
                nodeIt->second.currentSignalPhase = "RED";
                nodeIt->second.currentCorridor.clear();
            }
        }
    }

    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            grpIt->second.state = SynchronizationState::READY;
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::RollbackExecution(const std::string& corridorId) {
    std::vector<std::string> nodes;
    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        auto it = corridors_.find(corridorId);
        if (it != corridors_.end()) {
            nodes = it->second.intersections;
            it->second.state = SignalExecutionState::FAILED;
            it->second.active = false;
        }
    }

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : nodes) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                nodeIt->second.signalState = SignalExecutionState::RESTORING;
                nodeIt->second.currentSignalPhase = "YELLOW";
            }
        }
    }

    {
        std::lock_guard<std::mutex> grpLock(groupsMutex_);
        auto grpIt = groups_.find(corridorId);
        if (grpIt != groups_.end()) {
            grpIt->second.state = SynchronizationState::FAILED;
        }
    }

    return true;
}

bool TrafficNetworkOrchestrator::AcknowledgeExecution(const std::string& intersectionId,
                                                     const std::string& corridorId,
                                                     bool success) {
    if (!success) {
        RollbackExecution(corridorId);
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) {
        it->second.signalState = SignalExecutionState::GREEN;
        return true;
    }
    return false;
}

// =============================================================================
// Monitoring & Automatic Fault Recovery
// =============================================================================

uint32_t TrafficNetworkOrchestrator::CheckHeartbeats(uint64_t timeoutMs) {
    uint64_t now = GetCurrentTimeMs();
    uint32_t timedOutCount = 0;

    std::lock_guard<std::mutex> hbLock(heartbeatsMutex_);
    std::lock_guard<std::mutex> regLock(registryMutex_);

    for (auto& kv : intersections_) {
        if (kv.second.online && (now >= kv.second.lastHeartbeat) &&
            (now - kv.second.lastHeartbeat > timeoutMs)) {
            kv.second.online = false;
            kv.second.status = IntersectionStatus::OFFLINE;
            if (kv.second.signalState == SignalExecutionState::GREEN ||
                kv.second.signalState == SignalExecutionState::PREPARING) {
                kv.second.signalState = SignalExecutionState::FAILED;
            }
            timedOutCount++;
        }
    }

    return timedOutCount;
}

// =============================================================================
// Queries
// =============================================================================

bool TrafficNetworkOrchestrator::GetIntersection(const std::string& intersectionId, IntersectionNode& outNode) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) {
        outNode = it->second;
        return true;
    }
    return false;
}

std::vector<IntersectionNode> TrafficNetworkOrchestrator::GetAllIntersections() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<IntersectionNode> result;
    result.reserve(intersections_.size());
    for (const auto& kv : intersections_) {
        result.push_back(kv.second);
    }
    return result;
}

bool TrafficNetworkOrchestrator::GetCorridor(const std::string& corridorId, TrafficCorridor& outCorridor) const {
    std::lock_guard<std::mutex> corrLock(corridorsMutex_);
    auto it = corridors_.find(corridorId);
    if (it != corridors_.end()) {
        outCorridor = it->second;
        return true;
    }
    return false;
}

std::vector<TrafficCorridor> TrafficNetworkOrchestrator::GetAllCorridors() const {
    std::lock_guard<std::mutex> corrLock(corridorsMutex_);
    std::vector<TrafficCorridor> result;
    result.reserve(corridors_.size());
    for (const auto& kv : corridors_) {
        result.push_back(kv.second);
    }
    return result;
}

bool TrafficNetworkOrchestrator::GetSynchronizationGroup(const std::string& groupId, SynchronizationGroup& outGroup) const {
    std::lock_guard<std::mutex> grpLock(groupsMutex_);
    auto it = groups_.find(groupId);
    if (it != groups_.end()) {
        outGroup = it->second;
        return true;
    }
    return false;
}

std::vector<SynchronizationGroup> TrafficNetworkOrchestrator::GetAllSynchronizationGroups() const {
    std::lock_guard<std::mutex> grpLock(groupsMutex_);
    std::vector<SynchronizationGroup> result;
    result.reserve(groups_.size());
    for (const auto& kv : groups_) {
        result.push_back(kv.second);
    }
    return result;
}

// =============================================================================
// Reports & Statistics
// =============================================================================

TrafficNetworkStatistics TrafficNetworkOrchestrator::GenerateStatistics() const {
    return GetStatistics();
}

TrafficNetworkStatistics TrafficNetworkOrchestrator::GetStatistics() const {
    TrafficNetworkStatistics stats;
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        stats = statistics_;
    }

    uint64_t now = GetCurrentTimeMs();
    double totalHeartbeatDelta = 0.0;
    uint32_t hbCount = 0;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        stats.totalIntersections = static_cast<uint32_t>(intersections_.size());
        stats.onlineIntersections = 0;
        stats.offlineIntersections = 0;
        stats.executingSignals = 0;

        for (const auto& kv : intersections_) {
            if (kv.second.online && kv.second.status == IntersectionStatus::ONLINE) {
                stats.onlineIntersections++;
            } else {
                stats.offlineIntersections++;
            }

            if (kv.second.signalState == SignalExecutionState::GREEN ||
                kv.second.signalState == SignalExecutionState::PREPARING ||
                kv.second.signalState == SignalExecutionState::TRANSITION) {
                stats.executingSignals++;
            }

            if (kv.second.lastHeartbeat > 0 && now >= kv.second.lastHeartbeat) {
                totalHeartbeatDelta += static_cast<double>(now - kv.second.lastHeartbeat);
                hbCount++;
            }
        }
    }

    {
        std::lock_guard<std::mutex> corrLock(corridorsMutex_);
        stats.activeCorridors = 0;
        for (const auto& kv : corridors_) {
            if (kv.second.active) {
                stats.activeCorridors++;
            }
        }
    }

    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        if (latencyMeasurementCount_ > 0) {
            stats.averageExecutionLatency = totalExecutionLatencyMs_ / static_cast<double>(latencyMeasurementCount_);
        } else {
            stats.averageExecutionLatency = 0.0;
        }

        if (hbCount > 0) {
            stats.averageHeartbeat = totalHeartbeatDelta / static_cast<double>(hbCount);
        } else {
            stats.averageHeartbeat = 0.0;
        }

        if (recoveryCount_ > 0) {
            stats.averageRecoveryTime = totalRecoveryTimeMs_ / static_cast<double>(recoveryCount_);
        } else {
            stats.averageRecoveryTime = 0.0;
        }

        statistics_ = stats;
    }

    return stats;
}

std::string TrafficNetworkOrchestrator::GenerateNetworkReport() const {
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    TrafficNetworkStatistics stats = GetStatistics();

    std::ostringstream oss;
    oss << "[NETWORK]\n\n"
        << "Total Intersections:\n"
        << stats.totalIntersections << "\n\n"
        << "Online:\n"
        << stats.onlineIntersections << "\n\n"
        << "Offline:\n"
        << stats.offlineIntersections << "\n\n"
        << "Executing:\n"
        << stats.executingSignals << "\n\n"
        << "Green Corridors:\n"
        << stats.activeCorridors << "\n\n"
        << "Average Latency:\n"
        << std::fixed << std::setprecision(2) << stats.averageExecutionLatency << " ms\n\n"
        << "Average Heartbeat:\n"
        << std::fixed << std::setprecision(2) << stats.averageHeartbeat << " ms\n";

    return oss.str();
}

std::string TrafficNetworkOrchestrator::GenerateCorridorReport(const std::string& corridorId) const {
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    TrafficCorridor corr;
    if (!GetCorridor(corridorId, corr)) {
        return "[CORRIDOR]\n\nError: Corridor not found: " + corridorId + "\n";
    }

    std::ostringstream oss;
    oss << "[CORRIDOR]\n\n"
        << "Mission:\n"
        << corr.missionId << "\n\n"
        << "Corridor:\n"
        << corr.corridorId << "\n\n"
        << "Intersections:\n";

    for (size_t i = 0; i < corr.intersections.size(); ++i) {
        oss << corr.intersections[i];
        if (i + 1 < corr.intersections.size()) {
            oss << ", ";
        }
    }
    oss << "\n\n";

    double avgLat = 0.0;
    uint32_t count = 0;
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        for (const auto& nodeId : corr.intersections) {
            auto nodeIt = intersections_.find(nodeId);
            if (nodeIt != intersections_.end()) {
                avgLat += nodeIt->second.executionLatency;
                count++;
            }
        }
    }
    if (count > 0) {
        avgLat /= count;
    }

    oss << "Current State:\n"
        << ToString(corr.state) << "\n\n"
        << "Leader:\n"
        << corr.leaderIntersection << "\n\n"
        << "Latency:\n"
        << std::fixed << std::setprecision(2) << avgLat << " ms\n";

    return oss.str();
}

std::string TrafficNetworkOrchestrator::GenerateSynchronizationReport(const std::string& groupId) const {
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    SynchronizationGroup grp;
    if (!GetSynchronizationGroup(groupId, grp)) {
        return "[SYNC]\n\nError: Synchronization group not found: " + groupId + "\n";
    }

    std::ostringstream oss;
    oss << "[SYNC]\n\n"
        << "Synchronization Group:\n"
        << grp.groupId << "\n\n"
        << "Leader:\n"
        << grp.leaderIntersection << "\n\n"
        << "Members:\n";

    for (size_t i = 0; i < grp.intersections.size(); ++i) {
        oss << grp.intersections[i];
        if (i + 1 < grp.intersections.size()) {
            oss << ", ";
        }
    }
    oss << "\n\n";

    oss << "Execution State:\n"
        << ToString(grp.state) << "\n\n"
        << "Completion:\n"
        << (grp.state == SynchronizationState::COMPLETED ? "100%" : (grp.state == SynchronizationState::EXECUTING ? "50%" : "0%")) << "\n";

    return oss.str();
}

std::string TrafficNetworkOrchestrator::GenerateTopologyReport() const {
    std::lock_guard<std::mutex> repLock(reportsMutex_);
    std::lock_guard<std::mutex> regLock(registryMutex_);

    std::ostringstream oss;
    oss << "==========================================================\n"
        << " MUNICIPAL TRAFFIC TOPOLOGY REPORT\n"
        << "==========================================================\n"
        << " Total Nodes: " << intersections_.size() << "\n"
        << "----------------------------------------------------------\n"
        << " Adjacency Graph:\n";

    for (const auto& kv : intersections_) {
        oss << " " << kv.first << " [" << ToString(kv.second.status) << "] -> ";
        if (kv.second.neighbors.empty()) {
            oss << "NONE\n";
        } else {
            for (size_t i = 0; i < kv.second.neighbors.size(); ++i) {
                oss << kv.second.neighbors[i];
                if (i + 1 < kv.second.neighbors.size()) {
                    oss << ", ";
                }
            }
            oss << "\n";
        }
    }
    oss << "==========================================================\n";

    return oss.str();
}

// =============================================================================
// Lifecycle Reset & Cleanup
// =============================================================================

void TrafficNetworkOrchestrator::Reset() {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::lock_guard<std::mutex> corrLock(corridorsMutex_);
    std::lock_guard<std::mutex> grpLock(groupsMutex_);
    std::lock_guard<std::mutex> statLock(statisticsMutex_);

    intersections_.clear();
    corridors_.clear();
    groups_.clear();
    statistics_ = TrafficNetworkStatistics{};
    totalExecutionLatencyMs_ = 0.0;
    latencyMeasurementCount_ = 0;
    totalRecoveryTimeMs_ = 0.0;
    recoveryCount_ = 0;
    totalGreenWavesExecuted_ = 0;
    successfulGreenWaves_ = 0;
}

void TrafficNetworkOrchestrator::ClearHistory() {
    std::lock_guard<std::mutex> statLock(statisticsMutex_);
    totalExecutionLatencyMs_ = 0.0;
    latencyMeasurementCount_ = 0;
    totalRecoveryTimeMs_ = 0.0;
    recoveryCount_ = 0;
    totalGreenWavesExecuted_ = 0;
    successfulGreenWaves_ = 0;
    statistics_.failedSynchronizations = 0;
}

} // namespace traffic_network
} // namespace aegis
