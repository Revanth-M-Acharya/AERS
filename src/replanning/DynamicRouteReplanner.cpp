/**
 * @file DynamicRouteReplanner.cpp
 * @brief Implementation of Dynamic Route Replanning & Live Traffic Adaptation Engine.
 */

#include "replanning/DynamicRouteReplanner.h"

namespace aegis {
namespace replanning {

DynamicRouteReplanner::DynamicRouteReplanner()
    : m_planner(std::make_shared<green_corridor::EmergencyCorridorPlanner>())
    , m_greenWaveCoordinator(std::make_shared<green_wave::GreenWaveCoordinator>())
    , m_coordinatorManager(nullptr) {
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
    m_history.reserve(64);
    SyncBaseRoadSegments();
}

DynamicRouteReplanner::DynamicRouteReplanner(
    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner,
    std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator,
    std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager)
    : m_planner(planner ? planner : std::make_shared<green_corridor::EmergencyCorridorPlanner>())
    , m_greenWaveCoordinator(coordinator ? coordinator : std::make_shared<green_wave::GreenWaveCoordinator>())
    , m_coordinatorManager(coordinatorManager) {
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
    m_history.reserve(64);
    SyncBaseRoadSegments();
}

uint64_t DynamicRouteReplanner::CurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string DynamicRouteReplanner::MakeRoadKey(const std::string& from, const std::string& to) {
    return from + "->" + to;
}

void DynamicRouteReplanner::SyncBaseRoadSegments() {
    if (!m_planner) return;
    auto segments = m_planner->GetAllRoadSegments();
    for (const auto& seg : segments) {
        std::string key = MakeRoadKey(seg.fromIntersection, seg.toIntersection);
        if (m_baseRoadSegments.find(key) == m_baseRoadSegments.end()) {
            m_baseRoadSegments[key] = seg;
        }
    }
}

bool DynamicRouteReplanner::IsRoadOnCurrentCorridor(const std::string& from, const std::string& to) const {
    if (m_currentRoute.intersections.size() < 2) {
        return false;
    }

    size_t startIdx = 0;
    if (!m_ambulanceState.currentIntersection.empty()) {
        for (size_t i = 0; i < m_currentRoute.intersections.size(); ++i) {
            if (m_currentRoute.intersections[i] == m_ambulanceState.currentIntersection) {
                startIdx = i;
                break;
            }
        }
    }

    for (size_t i = startIdx; i + 1 < m_currentRoute.intersections.size(); ++i) {
        if (m_currentRoute.intersections[i] == from && m_currentRoute.intersections[i + 1] == to) {
            return true;
        }
    }
    return false;
}

bool DynamicRouteReplanner::UpdateTrafficCondition(const TrafficCondition& condition) {
    if (condition.fromIntersection.empty() && condition.roadId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    SyncBaseRoadSegments();

    std::string key;
    if (!condition.fromIntersection.empty() && !condition.toIntersection.empty()) {
        key = MakeRoadKey(condition.fromIntersection, condition.toIntersection);
    } else if (!condition.roadId.empty()) {
        auto it = m_roadIdToKey.find(condition.roadId);
        if (it != m_roadIdToKey.end()) {
            key = it->second;
        } else {
            key = condition.roadId;
        }
    }

    if (!condition.roadId.empty() && !key.empty()) {
        m_roadIdToKey[condition.roadId] = key;
    }

    m_trafficConditions[key] = condition;
    m_trafficChanged = true;

    // Apply modified road segment weights to planner
    if (m_planner) {
        std::vector<RoadSegment> updatedSegments;
        updatedSegments.reserve(m_baseRoadSegments.size());

        for (const auto& pair : m_baseRoadSegments) {
            RoadSegment seg = pair.second;
            std::string segKey = MakeRoadKey(seg.fromIntersection, seg.toIntersection);

            auto it = m_trafficConditions.find(segKey);
            if (it != m_trafficConditions.end()) {
                const auto& tc = it->second;
                if (tc.blocked) {
                    // Omit blocked segment so Dijkstra pathfinder routes around it
                    continue;
                }
                if (tc.averageSpeedKmph > 0.0) {
                    double speedMps = tc.averageSpeedKmph / 3.6;
                    double travelTime = (speedMps > 0.0) ? (seg.distanceMeters / speedMps) : seg.estimatedTravelTimeSeconds;
                    seg.estimatedTravelTimeSeconds = travelTime + tc.delaySeconds;
                } else if (tc.delaySeconds > 0.0) {
                    seg.estimatedTravelTimeSeconds += tc.delaySeconds;
                }
            }
            updatedSegments.push_back(seg);
        }

        m_planner->SetRoadSegments(updatedSegments);
    }

    return true;
}

bool DynamicRouteReplanner::UpdateAmbulanceState(const LiveAmbulanceState& state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulanceState = state;
    if (m_baselineEtaSeconds <= 0.0 && state.currentEtaSeconds > 0.0) {
        m_baselineEtaSeconds = state.currentEtaSeconds;
    }
    m_trafficChanged = true;
    return true;
}

bool DynamicRouteReplanner::ShouldReplan() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Rule 5: Coordinator explicitly requests rerouting
    if (m_coordinatorRerouteRequested) {
        m_replanReason = m_coordinatorReason.empty() ? "Coordinator Manual Reroute Request" : m_coordinatorReason;
        return true;
    }

    // Rule 1: Road becomes blocked
    for (const auto& pair : m_trafficConditions) {
        if (pair.second.blocked) {
            if (m_currentRoute.intersections.empty() ||
                IsRoadOnCurrentCorridor(pair.second.fromIntersection, pair.second.toIntersection)) {
                m_replanReason = "Road Blocked";
                return true;
            }
        }
    }

    // Rule 3: Average road speed drops below 30 km/h
    for (const auto& pair : m_trafficConditions) {
        if (pair.second.averageSpeedKmph > 0.0 && pair.second.averageSpeedKmph < m_speedThresholdKmph) {
            if (m_currentRoute.intersections.empty() ||
                IsRoadOnCurrentCorridor(pair.second.fromIntersection, pair.second.toIntersection)) {
                m_replanReason = "Speed Below 30 km/h";
                return true;
            }
        }
    }

    // Rule 2: ETA increases by more than 20 seconds
    if (m_baselineEtaSeconds > 0.0 &&
        (m_ambulanceState.currentEtaSeconds > m_baselineEtaSeconds + m_etaThresholdSeconds)) {
        m_replanReason = "ETA Increase Exceeded Threshold (20s)";
        return true;
    }

    // Also check accumulated delays on active corridor
    double accumulatedCorridorDelay = 0.0;
    if (m_currentRoute.intersections.size() >= 2) {
        size_t startIdx = 0;
        if (!m_ambulanceState.currentIntersection.empty()) {
            for (size_t i = 0; i < m_currentRoute.intersections.size(); ++i) {
                if (m_currentRoute.intersections[i] == m_ambulanceState.currentIntersection) {
                    startIdx = i;
                    break;
                }
            }
        }
        for (size_t i = startIdx; i + 1 < m_currentRoute.intersections.size(); ++i) {
            std::string segKey = MakeRoadKey(m_currentRoute.intersections[i], m_currentRoute.intersections[i + 1]);
            auto it = m_trafficConditions.find(segKey);
            if (it != m_trafficConditions.end()) {
                accumulatedCorridorDelay += it->second.delaySeconds;
            }
        }
    }
    if (accumulatedCorridorDelay > m_etaThresholdSeconds) {
        m_replanReason = "ETA Increase Exceeded Threshold (20s)";
        return true;
    }

    // If nothing has changed, no replan required (Rule 1-5 false)
    if (!m_trafficChanged) {
        return false;
    }

    // Rule 4: A newly computed path reduces travel time
    if (m_planner && !m_ambulanceState.currentIntersection.empty() &&
        !m_ambulanceState.destinationHospital.empty()) {
        EmergencyRoute candidate = m_planner->ComputeEmergencyRoute(
            m_ambulanceState.ambulanceId,
            m_ambulanceState.currentIntersection,
            m_ambulanceState.destinationHospital);

        if (candidate.isValid && !candidate.intersections.empty()) {
            double currentRemainingTime = (m_currentRoute.isValid && m_currentRoute.estimatedArrivalTime > 0.0)
                                          ? m_currentRoute.estimatedArrivalTime
                                          : m_baselineEtaSeconds;
            if (currentRemainingTime <= 0.0) {
                currentRemainingTime = m_ambulanceState.currentEtaSeconds;
            }

            if (candidate.estimatedArrivalTime < currentRemainingTime - 0.5 ||
                (candidate.intersections != m_currentRoute.intersections && candidate.estimatedArrivalTime < currentRemainingTime)) {
                m_pendingRoute = candidate;
                m_replanReason = "Faster Path Available";
                return true;
            }
        }
    }

    return false;
}

EmergencyRoute DynamicRouteReplanner::GenerateUpdatedRoute() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_pendingRoute.isValid && !m_pendingRoute.intersections.empty()) {
        return m_pendingRoute;
    }
    if (m_planner && !m_ambulanceState.currentIntersection.empty() && !m_ambulanceState.destinationHospital.empty()) {
        m_pendingRoute = m_planner->ComputeEmergencyRoute(
            m_ambulanceState.ambulanceId,
            m_ambulanceState.currentIntersection,
            m_ambulanceState.destinationHospital);
        return m_pendingRoute;
    }
    return EmergencyRoute{};
}

GreenWavePlan DynamicRouteReplanner::GenerateUpdatedGreenWave() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_pendingRoute.isValid || m_pendingRoute.intersections.empty()) {
        if (m_planner && !m_ambulanceState.currentIntersection.empty() && !m_ambulanceState.destinationHospital.empty()) {
            m_pendingRoute = m_planner->ComputeEmergencyRoute(
                m_ambulanceState.ambulanceId,
                m_ambulanceState.currentIntersection,
                m_ambulanceState.destinationHospital);
        }
    }

    const auto& targetRoute = (m_pendingRoute.isValid && !m_pendingRoute.intersections.empty())
                              ? m_pendingRoute : m_currentRoute;

    if (m_greenWaveCoordinator && targetRoute.isValid && !targetRoute.intersections.empty()) {
        m_greenWaveCoordinator->GenerateGreenWaveSchedule(targetRoute, m_pendingGreenWave);
    }
    return m_pendingGreenWave;
}

bool DynamicRouteReplanner::ApplyNewPlan() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_pendingRoute.isValid || m_pendingRoute.intersections.empty()) {
        if (m_planner && !m_ambulanceState.currentIntersection.empty() && !m_ambulanceState.destinationHospital.empty()) {
            m_pendingRoute = m_planner->ComputeEmergencyRoute(
                m_ambulanceState.ambulanceId,
                m_ambulanceState.currentIntersection,
                m_ambulanceState.destinationHospital);
        }
    }

    if (!m_pendingRoute.isValid || m_pendingRoute.intersections.empty()) {
        return false;
    }

    if (!m_pendingGreenWave.isValid || m_pendingGreenWave.windows.empty()) {
        if (m_greenWaveCoordinator) {
            m_greenWaveCoordinator->GenerateGreenWaveSchedule(m_pendingRoute, m_pendingGreenWave);
        }
    }

    // Dispatch updated traffic signal commands
    bool dispatchedAny = false;
    for (const auto& window : m_pendingGreenWave.windows) {
        std::string cmd = (window.command.rfind("SET_", 0) == 0) ? window.command : ("SET_" + window.command);
        std::string reason = "Dynamic Route Replanning: " + m_ambulanceState.ambulanceId;
        if (m_customDispatcher) {
            m_customDispatcher(window.intersectionId, cmd, reason);
            dispatchedAny = true;
        } else if (m_coordinatorManager) {
            m_coordinatorManager->SendTrafficSignalCommand(window.intersectionId, cmd, reason);
            dispatchedAny = true;
        } else {
            dispatchedAny = true;
        }
    }

    // Version management
    m_ambulanceState.routeVersion++;

    // Record execution
    ReplanResult result;
    result.oldRoute = m_currentRoute;
    result.newRoute = m_pendingRoute;
    result.oldEtaSeconds = (m_currentRoute.isValid && m_currentRoute.estimatedArrivalTime > 0.0)
                          ? m_currentRoute.estimatedArrivalTime
                          : (m_baselineEtaSeconds > 0.0 ? m_baselineEtaSeconds : m_ambulanceState.currentEtaSeconds);
    result.newEtaSeconds = m_pendingRoute.estimatedArrivalTime;
    result.timeSavedSeconds = std::max(0.0, result.oldEtaSeconds - result.newEtaSeconds);
    result.greenWaveUpdated = m_pendingGreenWave.isValid;
    result.commandsRedispatched = dispatchedAny;
    result.success = true;
    result.reason = m_replanReason.empty() ? "Road Blocked" : m_replanReason;
    result.timestampMs = CurrentTimeMs();

    // Commit state
    m_currentRoute = m_pendingRoute;
    m_currentGreenWave = m_pendingGreenWave;
    m_baselineEtaSeconds = m_pendingRoute.estimatedArrivalTime;
    m_ambulanceState.currentEtaSeconds = m_pendingRoute.estimatedArrivalTime;
    m_lastReplan = result;
    m_history.push_back(result);

    // Reset pending triggers
    m_coordinatorRerouteRequested = false;
    m_coordinatorReason.clear();
    m_trafficChanged = false;
    m_replanReason.clear();
    m_pendingRoute = EmergencyRoute{};
    m_pendingGreenWave = GreenWavePlan{};

    if (m_enableConsoleLog) {
        std::cout << FormatReplanLog(result, m_ambulanceState.ambulanceId) << std::flush;
    }

    return true;
}

EmergencyRoute DynamicRouteReplanner::GetCurrentRoute() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentRoute;
}

GreenWavePlan DynamicRouteReplanner::GetCurrentGreenWave() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentGreenWave;
}

ReplanResult DynamicRouteReplanner::GetLastReplan() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastReplan;
}

void DynamicRouteReplanner::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trafficConditions.clear();
    m_roadIdToKey.clear();
    m_ambulanceState = LiveAmbulanceState{};
    m_currentRoute = EmergencyRoute{};
    m_currentGreenWave = GreenWavePlan{};
    m_pendingRoute = EmergencyRoute{};
    m_pendingGreenWave = GreenWavePlan{};
    m_history.clear();
    m_lastReplan = ReplanResult{};
    m_trafficChanged = false;
    m_coordinatorRerouteRequested = false;
    m_baselineEtaSeconds = 0.0;
    m_replanReason.clear();

    // Restore base road segments
    if (m_planner && !m_baseRoadSegments.empty()) {
        std::vector<RoadSegment> baseSegments;
        baseSegments.reserve(m_baseRoadSegments.size());
        for (const auto& pair : m_baseRoadSegments) {
            baseSegments.push_back(pair.second);
        }
        m_planner->SetRoadSegments(baseSegments);
    }
}

std::vector<ReplanResult> DynamicRouteReplanner::GetHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_history;
}

void DynamicRouteReplanner::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_history.clear();
    m_lastReplan = ReplanResult{};
}

uint32_t DynamicRouteReplanner::GetRouteVersion() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ambulanceState.routeVersion;
}

void DynamicRouteReplanner::SetCurrentRoute(const EmergencyRoute& route) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentRoute = route;
    if (m_baselineEtaSeconds <= 0.0 && route.estimatedArrivalTime > 0.0) {
        m_baselineEtaSeconds = route.estimatedArrivalTime;
    }
}

void DynamicRouteReplanner::SetCurrentGreenWave(const GreenWavePlan& plan) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentGreenWave = plan;
}

LiveAmbulanceState DynamicRouteReplanner::GetAmbulanceState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ambulanceState;
}

void DynamicRouteReplanner::RequestCoordinatorReroute(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorRerouteRequested = true;
    m_coordinatorReason = reason;
    m_trafficChanged = true;
}

void DynamicRouteReplanner::SetPlanner(std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_planner = planner;
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
    SyncBaseRoadSegments();
}

std::shared_ptr<green_corridor::EmergencyCorridorPlanner> DynamicRouteReplanner::GetPlanner() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_planner;
}

void DynamicRouteReplanner::SetGreenWaveCoordinator(std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_greenWaveCoordinator = coordinator;
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
}

std::shared_ptr<green_wave::GreenWaveCoordinator> DynamicRouteReplanner::GetGreenWaveCoordinator() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_greenWaveCoordinator;
}

void DynamicRouteReplanner::SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorManager = coordinatorManager;
}

std::shared_ptr<coordinator::CoordinatorManager> DynamicRouteReplanner::GetCoordinatorManager() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinatorManager;
}

void DynamicRouteReplanner::SetCommandDispatcher(CommandDispatcher dispatcher) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_customDispatcher = std::move(dispatcher);
}

void DynamicRouteReplanner::SetConsoleLogging(bool enabled) noexcept {
    m_enableConsoleLog = enabled;
}

bool DynamicRouteReplanner::GetConsoleLogging() const noexcept {
    return m_enableConsoleLog;
}

void DynamicRouteReplanner::SetEtaThresholdSeconds(double seconds) noexcept {
    m_etaThresholdSeconds = seconds;
}

double DynamicRouteReplanner::GetEtaThresholdSeconds() const noexcept {
    return m_etaThresholdSeconds;
}

void DynamicRouteReplanner::SetSpeedThresholdKmph(double speedKmph) noexcept {
    m_speedThresholdKmph = speedKmph;
}

double DynamicRouteReplanner::GetSpeedThresholdKmph() const noexcept {
    return m_speedThresholdKmph;
}

} // namespace replanning
} // namespace aegis
