/**
 * @file MultiCorridorCoordinator.cpp
 * @brief Implementation of Multi-Ambulance Corridor Coordination & Conflict Resolution Engine (Phase 9B).
 */

#include "multi_corridor/MultiCorridorCoordinator.h"

namespace aegis {
namespace multi_corridor {

MultiCorridorCoordinator::MultiCorridorCoordinator()
    : m_replanner(nullptr)
    , m_coordinatorManager(nullptr) {}

MultiCorridorCoordinator::MultiCorridorCoordinator(
    std::shared_ptr<replanning::DynamicRouteReplanner> replanner,
    std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager)
    : m_replanner(std::move(replanner))
    , m_coordinatorManager(std::move(coordinatorManager)) {}

uint64_t MultiCorridorCoordinator::CurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

double MultiCorridorCoordinator::GetIntersectionEta(const ActiveCorridor& corridor,
                                                    const std::string& intersectionId,
                                                    size_t nodeIdx) {
    // 0. Fast-path O(1) index check if nodeIdx provided
    if (nodeIdx < corridor.route.timeline.size() &&
        corridor.route.timeline[nodeIdx].intersectionId == intersectionId) {
        return corridor.route.timeline[nodeIdx].etaSeconds;
    }
    if (nodeIdx < corridor.greenWave.windows.size() &&
        corridor.greenWave.windows[nodeIdx].intersectionId == intersectionId) {
        if (corridor.greenWave.windows[nodeIdx].etaSeconds > 0.0) {
            return corridor.greenWave.windows[nodeIdx].etaSeconds;
        }
        return corridor.greenWave.windows[nodeIdx].startTimeSeconds + corridor.greenWave.windows[nodeIdx].safetyBufferSeconds;
    }

    // 1. Check corridor route timeline
    for (const auto& item : corridor.route.timeline) {
        if (item.intersectionId == intersectionId) {
            return item.etaSeconds;
        }
    }

    // 2. Check Green Wave windows
    for (const auto& win : corridor.greenWave.windows) {
        if (win.intersectionId == intersectionId) {
            if (win.etaSeconds > 0.0) {
                return win.etaSeconds;
            }
            return win.startTimeSeconds + win.safetyBufferSeconds;
        }
    }

    // 3. Estimate along route sequence
    const auto& nodes = corridor.route.intersections;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i] == intersectionId) {
            double ratio = static_cast<double>(i + 1) / static_cast<double>(std::max<size_t>(1, nodes.size()));
            return corridor.ETA * ratio;
        }
    }

    return corridor.ETA;
}

bool MultiCorridorCoordinator::IsWinner(const ActiveCorridor& a, const ActiveCorridor& b) {
    // Rule 1: Higher medical priority wins (CRITICAL > HIGH > MEDIUM > LOW)
    int rankA = a.priority.Rank();
    int rankB = b.priority.Rank();
    if (rankA != rankB) {
        return rankA < rankB; // Smaller rank number is higher priority
    }

    // Rule 2: Lower ETA wins
    if (std::abs(a.ETA - b.ETA) > 1e-4) {
        return a.ETA < b.ETA;
    }

    // Rule 3: Shorter remaining distance wins
    double distA = (a.remainingDistanceMeters > 0.0) ? a.remainingDistanceMeters : a.route.totalDistance;
    double distB = (b.remainingDistanceMeters > 0.0) ? b.remainingDistanceMeters : b.route.totalDistance;
    if (std::abs(distA - distB) > 1e-4) {
        return distA < distB;
    }

    // Rule 4: Earlier timestamp wins
    uint64_t tsA = (a.timestampMs > 0) ? a.timestampMs : a.timestamp;
    uint64_t tsB = (b.timestampMs > 0) ? b.timestampMs : b.timestamp;
    if (tsA != tsB) {
        return tsA < tsB;
    }

    // Rule 5: Lexicographical Ambulance ID wins
    return a.ambulanceId < b.ambulanceId;
}

bool MultiCorridorCoordinator::RegisterCorridor(const ActiveCorridor& corridor) {
    if (corridor.ambulanceId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    ActiveCorridor entry = corridor;
    entry.active = true;
    if (entry.timestamp == 0) {
        entry.timestamp = CurrentTimeMs();
        entry.timestampMs = entry.timestamp;
    }
    m_activeCorridors[entry.ambulanceId] = entry;
    return true;
}

bool MultiCorridorCoordinator::UpdateCorridor(const ActiveCorridor& corridor) {
    if (corridor.ambulanceId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    ActiveCorridor entry = corridor;
    if (entry.timestamp == 0) {
        auto it = m_activeCorridors.find(corridor.ambulanceId);
        if (it != m_activeCorridors.end()) {
            entry.timestamp = it->second.timestamp;
            entry.timestampMs = it->second.timestampMs;
        } else {
            entry.timestamp = CurrentTimeMs();
            entry.timestampMs = entry.timestamp;
        }
    }
    m_activeCorridors[entry.ambulanceId] = entry;
    return true;
}

bool MultiCorridorCoordinator::RemoveCorridor(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeCorridors.erase(ambulanceId) > 0;
}

std::vector<CorridorConflict> MultiCorridorCoordinator::DetectConflicts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<CorridorConflict> detected;

    // Collect all active corridors in deterministic order
    std::vector<ActiveCorridor> corridors;
    corridors.reserve(m_activeCorridors.size());
    for (const auto& kv : m_activeCorridors) {
        if (kv.second.active) {
            corridors.push_back(kv.second);
        }
    }
    std::sort(corridors.begin(), corridors.end(), [](const ActiveCorridor& a, const ActiveCorridor& b) {
        return a.ambulanceId < b.ambulanceId;
    });

    const size_t n = corridors.size();
    if (n < 2) {
        return detected;
    }

    uint64_t now = CurrentTimeMs();

    // Precompute node index maps and adjacent edges for all corridors once
    std::vector<std::unordered_map<std::string, size_t>> allNodeIndices(n);
    std::vector<std::vector<std::pair<std::string, std::string>>> allEdges(n);
    for (size_t k = 0; k < n; ++k) {
        const auto& p = corridors[k].route.intersections;
        allNodeIndices[k].reserve(p.size());
        for (size_t idx = 0; idx < p.size(); ++idx) {
            allNodeIndices[k].emplace(p[idx], idx);
        }
        if (p.size() >= 2) {
            allEdges[k].reserve(p.size() - 1);
            for (size_t idx = 0; idx + 1 < p.size(); ++idx) {
                allEdges[k].emplace_back(p[idx], p[idx + 1]);
            }
        }
    }

    static const std::string emptyStr{""};
    detected.reserve(2048);

    struct NodeMatch {
        size_t idxA;
        size_t idxB;
    };

    // Compare all pairs O(N^2)
    for (size_t i = 0; i < n; ++i) {
        const auto& A = corridors[i];
        const auto& pathA = A.route.intersections;
        if (pathA.empty()) continue;
        const auto& nodeMapA = allNodeIndices[i];
        const auto& edgesA = allEdges[i];

        for (size_t j = i + 1; j < n; ++j) {
            const auto& B = corridors[j];
            const auto& pathB = B.route.intersections;
            if (pathB.empty()) continue;

            // Step 1: Collect shared intersections directly on stack
            NodeMatch matches[16];
            size_t matchCount = 0;

            for (size_t idxB = 0; idxB < pathB.size(); ++idxB) {
                auto itA = nodeMapA.find(pathB[idxB]);
                if (itA != nodeMapA.end()) {
                    if (matchCount < 16) {
                        matches[matchCount++] = {itA->second, idxB};
                    }
                }
            }

            // Early exit if corridors have zero shared intersections
            if (matchCount == 0) {
                continue;
            }

            const auto& edgesB = allEdges[j];

            // Check Condition 5: Entire corridor overlap (zero-copy)
            bool isFullOverlap = false;
            if (matchCount == pathA.size() && pathA.size() == pathB.size()) {
                if (pathA == pathB) {
                    isFullOverlap = true;
                } else {
                    bool matchRev = true;
                    for (size_t k = 0; k < pathA.size(); ++k) {
                        if (pathA[k] != pathB[pathB.size() - 1 - k]) {
                            matchRev = false;
                            break;
                        }
                    }
                    isFullOverlap = matchRev;
                }
            }

            // Check Condition 4: Shared corridor with >= 2 consecutive intersections
            bool hasConsecutiveShared = false;
            if (matchCount >= 2 && !edgesA.empty() && !edgesB.empty()) {
                for (const auto& eA : edgesA) {
                    for (const auto& eB : edgesB) {
                        if ((eA.first == eB.first && eA.second == eB.second) ||
                            (eA.first == eB.second && eA.second == eB.first)) {
                            hasConsecutiveShared = true;
                            break;
                        }
                    }
                    if (hasConsecutiveShared) break;
                }
            }

            // Evaluate conflict details for each shared intersection using collected matches
            for (size_t m = 0; m < matchCount; ++m) {
                size_t idxA = matches[m].idxA;
                size_t idxB = matches[m].idxB;
                const std::string& intId = pathB[idxB];

                double etaA = GetIntersectionEta(A, intId, idxA);
                double etaB = GetIntersectionEta(B, intId, idxB);
                double deltaEta = std::abs(etaA - etaB);

                // Check Condition 3: Opposite directions through same junction (zero-copy references)
                const std::string& prevA = (idxA > 0) ? pathA[idxA - 1] : emptyStr;
                const std::string& nextA = (idxA + 1 < pathA.size()) ? pathA[idxA + 1] : emptyStr;
                const std::string& prevB = (idxB > 0) ? pathB[idxB - 1] : emptyStr;
                const std::string& nextB = (idxB + 1 < pathB.size()) ? pathB[idxB + 1] : emptyStr;

                bool isOpposite = (!prevA.empty() && !nextB.empty() && prevA == nextB) ||
                                  (!nextA.empty() && !prevB.empty() && nextA == prevB);

                ConflictType type = ConflictType::INTERSECTION_CONFLICT;
                ConflictSeverity severity = ConflictSeverity::LOW;

                if (isOpposite) {
                    type = ConflictType::OPPOSITE_DIRECTION;
                    severity = ConflictSeverity::CRITICAL;
                } else if (isFullOverlap) {
                    type = ConflictType::FULL_CORRIDOR_OVERLAP;
                    severity = ConflictSeverity::CRITICAL;
                } else if (deltaEta <= 3.0) {
                    type = ConflictType::TIME_CONFLICT;
                    severity = ConflictSeverity::HIGH;
                } else if (hasConsecutiveShared) {
                    type = ConflictType::SHARED_CORRIDOR;
                    severity = ConflictSeverity::MEDIUM;
                } else {
                    type = ConflictType::INTERSECTION_CONFLICT;
                    severity = ConflictSeverity::LOW;
                }

                detected.emplace_back(A.ambulanceId, B.ambulanceId, intId,
                                      etaA, etaB, type, severity, "", now);
            }
        }
    }

    // Append to conflict history
    m_conflictHistory.reserve(m_conflictHistory.size() + detected.size());
    m_conflictHistory.insert(m_conflictHistory.end(), detected.begin(), detected.end());
    return detected;
}

CoordinationDecision MultiCorridorCoordinator::ResolveConflict(const CorridorConflict& conflict) {
    std::lock_guard<std::mutex> lock(m_mutex);
    CoordinationDecision decision;
    decision.conflictIntersection = conflict.intersectionId;
    decision.timestamp = CurrentTimeMs();

    auto itA = m_activeCorridors.find(conflict.ambulanceA);
    auto itB = m_activeCorridors.find(conflict.ambulanceB);

    if (itA == m_activeCorridors.end() || itB == m_activeCorridors.end()) {
        decision.successful = false;
        decision.reason = "One or more conflicting corridors not found in registry";
        return decision;
    }

    // Determine Winner and Loser via 5-tier rules
    bool aWins = IsWinner(itA->second, itB->second);
    ActiveCorridor& winner = aWins ? itA->second : itB->second;
    ActiveCorridor& loser  = aWins ? itB->second : itA->second;

    decision.winner = winner.ambulanceId;
    decision.loser  = loser.ambulanceId;

    // Determine whether loser can safely wait
    bool canSafelyWait = true;

    if (conflict.conflictType == ConflictType::OPPOSITE_DIRECTION) {
        canSafelyWait = false;
    } else if (conflict.conflictType == ConflictType::FULL_CORRIDOR_OVERLAP) {
        canSafelyWait = false;
    } else if (m_forceReroute) {
        canSafelyWait = false;
    } else if (m_defaultDelaySeconds > m_maxSafeWaitSeconds) {
        canSafelyWait = false;
    }

    if (canSafelyWait) {
        // Adjust Green Wave timing for loser
        double delay = m_defaultDelaySeconds;

        for (auto& win : loser.greenWave.windows) {
            win.startTimeSeconds += delay;
            win.endTimeSeconds += delay;
            win.etaSeconds += delay;
        }
        loser.greenWave.totalDurationSeconds += delay;
        loser.ETA += delay;

        // Redispatch commands via CoordinatorManager or custom dispatcher
        if (m_customDispatcher) {
            for (const auto& win : loser.greenWave.windows) {
                m_customDispatcher(win.intersectionId, win.command, "MultiCorridor Coordination Delay");
            }
        } else if (m_coordinatorManager) {
            for (const auto& win : loser.greenWave.windows) {
                m_coordinatorManager->SendTrafficSignalCommand(
                    win.intersectionId, win.command, "MultiCorridor Coordination Delay");
            }
        }

        decision.greenWaveAdjusted = true;
        decision.rerouteTriggered = false;
        decision.delaySeconds = delay;
        decision.successful = true;
        decision.reason = "Green Wave schedule delayed for " + loser.ambulanceId;
    } else {
        // Trigger DynamicRouteReplanner
        decision.greenWaveAdjusted = false;
        decision.rerouteTriggered = true;
        decision.delaySeconds = 0.0;

        if (m_replanner) {
            m_replanner->RequestCoordinatorReroute(
                "Multi-corridor conflict at " + conflict.intersectionId + " with " + winner.ambulanceId);
            EmergencyRoute updatedRoute = m_replanner->GenerateUpdatedRoute();
            GreenWavePlan updatedGw = m_replanner->GenerateUpdatedGreenWave();
            m_replanner->ApplyNewPlan();

            if (updatedRoute.isValid) {
                loser.route = updatedRoute;
                loser.greenWave = updatedGw;
                loser.routeVersion = m_replanner->GetRouteVersion();
                loser.ETA = updatedRoute.estimatedArrivalTime;
            }
        }

        if (m_rerouteCallback) {
            m_rerouteCallback(loser.ambulanceId, conflict.intersectionId);
        }

        decision.successful = true;
        decision.reason = "Dynamic route replanning triggered for " + loser.ambulanceId;
    }

    m_resolutionHistory.push_back(decision);

    if (m_enableConsoleLog) {
        std::cout << FormatCoordinatorLog(conflict, decision) << std::endl;
    }

    return decision;
}

std::vector<CoordinationDecision> MultiCorridorCoordinator::ResolveAllConflicts() {
    std::vector<CorridorConflict> conflicts = DetectConflicts();
    std::vector<CoordinationDecision> decisions;
    decisions.reserve(conflicts.size());

    for (const auto& c : conflicts) {
        decisions.push_back(ResolveConflict(c));
    }
    return decisions;
}

std::vector<ActiveCorridor> MultiCorridorCoordinator::GetActiveCorridors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ActiveCorridor> list;
    list.reserve(m_activeCorridors.size());
    for (const auto& kv : m_activeCorridors) {
        list.push_back(kv.second);
    }
    std::sort(list.begin(), list.end(), [](const ActiveCorridor& a, const ActiveCorridor& b) {
        return a.ambulanceId < b.ambulanceId;
    });
    return list;
}

std::vector<CorridorConflict> MultiCorridorCoordinator::GetConflictHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_conflictHistory;
}

std::vector<CoordinationDecision> MultiCorridorCoordinator::GetResolutionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_resolutionHistory;
}

void MultiCorridorCoordinator::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_conflictHistory.clear();
    m_resolutionHistory.clear();
}

void MultiCorridorCoordinator::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeCorridors.clear();
    m_conflictHistory.clear();
    m_resolutionHistory.clear();
}

void MultiCorridorCoordinator::ClearCorridors() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeCorridors.clear();
}

bool MultiCorridorCoordinator::GetCorridor(const std::string& ambulanceId, ActiveCorridor& outCorridor) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeCorridors.find(ambulanceId);
    if (it != m_activeCorridors.end()) {
        outCorridor = it->second;
        return true;
    }
    return false;
}

bool MultiCorridorCoordinator::HasCorridor(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeCorridors.find(ambulanceId) != m_activeCorridors.end();
}

size_t MultiCorridorCoordinator::GetActiveCorridorCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeCorridors.size();
}

void MultiCorridorCoordinator::SetDynamicRouteReplanner(
    std::shared_ptr<replanning::DynamicRouteReplanner> replanner) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_replanner = std::move(replanner);
}

std::shared_ptr<replanning::DynamicRouteReplanner> MultiCorridorCoordinator::GetDynamicRouteReplanner() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_replanner;
}

void MultiCorridorCoordinator::SetCoordinatorManager(
    std::shared_ptr<coordinator::CoordinatorManager> manager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorManager = std::move(manager);
}

std::shared_ptr<coordinator::CoordinatorManager> MultiCorridorCoordinator::GetCoordinatorManager() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinatorManager;
}

void MultiCorridorCoordinator::SetCommandDispatcher(CommandDispatcher dispatcher) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_customDispatcher = std::move(dispatcher);
}

void MultiCorridorCoordinator::SetRerouteCallback(RerouteCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rerouteCallback = std::move(callback);
}

void MultiCorridorCoordinator::SetMaxSafeWaitSeconds(double seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxSafeWaitSeconds = seconds;
}

double MultiCorridorCoordinator::GetMaxSafeWaitSeconds() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_maxSafeWaitSeconds;
}

void MultiCorridorCoordinator::SetDefaultDelaySeconds(double seconds) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultDelaySeconds = seconds;
}

double MultiCorridorCoordinator::GetDefaultDelaySeconds() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultDelaySeconds;
}

void MultiCorridorCoordinator::SetForceReroute(bool force) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_forceReroute = force;
}

bool MultiCorridorCoordinator::GetForceReroute() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_forceReroute;
}

void MultiCorridorCoordinator::SetConsoleLogging(bool enabled) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enableConsoleLog = enabled;
}

bool MultiCorridorCoordinator::GetConsoleLogging() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enableConsoleLog;
}

} // namespace multi_corridor
} // namespace aegis
