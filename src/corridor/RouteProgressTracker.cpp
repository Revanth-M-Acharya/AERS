/**
 * @file RouteProgressTracker.cpp
 * @brief Implementation of Phase 3 Route Progress Tracker for SMART AEGIS.
 */

#include "aegis/corridor/RouteProgressTracker.hpp"
#include "aegis/logger/logger.hpp"

#include <algorithm>
#include <iostream>

namespace aegis {
namespace corridor {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "ROUTE_TRACK", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "ROUTE_TRACK", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "ROUTE_TRACK", msg);
}

RouteProgressTracker::RouteProgressTracker() {
    // Default fallback replanner: skips failed node if possible
    m_replanner = [](const std::string& currentPos,
                     const std::string& destination,
                     const std::string& failedNode) -> std::vector<std::string> {
        (void)failedNode;
        // Simple fallback bypass path
        std::vector<std::string> bypass;
        bypass.push_back(currentPos);
        bypass.push_back("BYPASS_ALT_1");
        bypass.push_back(destination);
        return bypass;
    };
}

uint64_t RouteProgressTracker::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

void RouteProgressTracker::RecomputeMetrics(RouteProgressState& state, uint64_t nowUs) {
    state.lastUpdateTimestampUs = nowUs;

    size_t totalNodes = state.routeIntersections.size();
    if (totalNodes == 0) {
        state.percentageCompletion = 0.0;
        return;
    }

    size_t passedCount = state.passedIntersections.size();
    if (state.isCompleted) {
        state.percentageCompletion = 100.0;
        state.currentEstimatedEtaSeconds = 0.0;
        state.remainingIntersections.clear();
        state.nextIntersection = "";
        return;
    }

    // Determine current, previous, next, and remaining
    state.remainingIntersections.clear();
    bool foundCurrent = false;

    for (size_t i = 0; i < totalNodes; ++i) {
        const std::string& node = state.routeIntersections[i];
        if (!foundCurrent) {
            if (node == state.currentIntersection) {
                foundCurrent = true;
                if (i > 0) {
                    state.previousIntersection = state.routeIntersections[i - 1];
                }
                if (i + 1 < totalNodes) {
                    state.nextIntersection = state.routeIntersections[i + 1];
                } else {
                    state.nextIntersection = "";
                }
            }
        } else {
            state.remainingIntersections.push_back(node);
        }
    }

    // Percentage completion based on passed waypoints and distance
    if (totalNodes > 1) {
        double hopRatio = static_cast<double>(passedCount) / static_cast<double>(totalNodes - 1);
        if (hopRatio > 1.0) hopRatio = 1.0;
        state.percentageCompletion = hopRatio * 100.0;
    } else {
        state.percentageCompletion = passedCount > 0 ? 100.0 : 0.0;
    }

    // Dynamic ETA update
    if (state.currentSpeedMps > 0.0 && state.totalDistanceMeters > 0.0) {
        double remainingDist = state.totalDistanceMeters * (1.0 - (state.percentageCompletion / 100.0));
        state.currentEstimatedEtaSeconds = remainingDist / state.currentSpeedMps;
    }
}

bool RouteProgressTracker::InitializeRoute(const std::string& ambulanceId,
                                          const std::string& corridorId,
                                          const std::vector<std::string>& route,
                                          double totalDistanceMeters,
                                          double etaSeconds,
                                          uint64_t timestampUs) {
    if (ambulanceId.empty() || corridorId.empty() || route.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();

    RouteProgressState state;
    state.ambulanceId = ambulanceId;
    state.corridorId = corridorId;
    state.destinationId = route.back();
    state.routeIntersections = route;
    state.totalDistanceMeters = (totalDistanceMeters > 0.0) ? totalDistanceMeters : 1000.0;
    state.passedDistanceMeters = 0.0;
    state.currentIntersection = route.front();
    state.previousIntersection = "";
    state.nextIntersection = (route.size() > 1) ? route[1] : "";
    state.initialEtaSeconds = (etaSeconds > 0.0) ? etaSeconds : 60.0;
    state.currentEstimatedEtaSeconds = state.initialEtaSeconds;
    state.currentSpeedMps = state.totalDistanceMeters / state.initialEtaSeconds;
    state.lastUpdateTimestampUs = nowUs;
    state.isCompleted = false;
    state.isRerouted = false;
    state.recalculationCount = 0;

    double segDist = state.totalDistanceMeters / static_cast<double>(route.size());
    double segTime = state.initialEtaSeconds / static_cast<double>(route.size());

    for (size_t i = 0; i < route.size(); ++i) {
        WaypointNode wp;
        wp.intersectionId = route[i];
        wp.distanceFromStartMeters = static_cast<double>(i) * segDist;
        wp.estimatedSegmentDurationSec = segTime;
        wp.isPassed = false;
        wp.arrivalTimestampUs = 0;
        wp.departureTimestampUs = 0;
        state.waypoints.push_back(wp);

        if (i > 0) {
            state.remainingIntersections.push_back(route[i]);
        }
    }

    m_activeRoutes[ambulanceId] = std::move(state);
    LogInfo("Initialized route for " + ambulanceId + " (Nodes: " +
            std::to_string(route.size()) + ", Dist: " +
            std::to_string(totalDistanceMeters) + "m, ETA: " +
            std::to_string(etaSeconds) + "s)");
    return true;
}

bool RouteProgressTracker::ReportArrival(const std::string& ambulanceId,
                                        const std::string& intersectionId,
                                        uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it == m_activeRoutes.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    auto& state = it->second;
    state.currentIntersection = intersectionId;

    for (auto& wp : state.waypoints) {
        if (wp.intersectionId == intersectionId) {
            wp.arrivalTimestampUs = nowUs;
            break;
        }
    }

    RecomputeMetrics(state, nowUs);
    LogInfo(ambulanceId + " ARRIVED at " + intersectionId +
            " (Progress: " + std::to_string(state.percentageCompletion) +
            "%, Next: " + state.nextIntersection + ")");
    return true;
}

bool RouteProgressTracker::ReportDeparture(const std::string& ambulanceId,
                                          const std::string& intersectionId,
                                          uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it == m_activeRoutes.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    auto& state = it->second;

    // Mark waypoint passed
    for (auto& wp : state.waypoints) {
        if (wp.intersectionId == intersectionId) {
            wp.isPassed = true;
            wp.departureTimestampUs = nowUs;
            break;
        }
    }

    // Add to passed intersections if not already present
    if (std::find(state.passedIntersections.begin(), state.passedIntersections.end(), intersectionId) ==
        state.passedIntersections.end()) {
        state.passedIntersections.push_back(intersectionId);
    }

    // If departing destination, complete the route
    if (intersectionId == state.destinationId) {
        state.isCompleted = true;
    }

    RecomputeMetrics(state, nowUs);
    LogInfo(ambulanceId + " DEPARTED " + intersectionId +
            " (Progress: " + std::to_string(state.percentageCompletion) +
            "%, Completed: " + (state.isCompleted ? "YES" : "NO") + ")");
    return true;
}

bool RouteProgressTracker::UpdateVelocity(const std::string& ambulanceId, double speedMps) {
    if (speedMps <= 0.0) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it == m_activeRoutes.end()) {
        return false;
    }

    it->second.currentSpeedMps = speedMps;
    RecomputeMetrics(it->second, GetCurrentTimeUs());
    return true;
}

bool RouteProgressTracker::RecalculateRouteOnFailure(const std::string& ambulanceId,
                                                    const std::string& failedIntersectionId,
                                                    uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it == m_activeRoutes.end() || !m_replanner) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    auto& state = it->second;

    std::vector<std::string> bypassPath = m_replanner(state.currentIntersection, state.destinationId, failedIntersectionId);
    if (bypassPath.empty()) {
        return false;
    }

    // Build new full route: passed nodes + bypass path
    std::vector<std::string> newRoute = state.passedIntersections;
    for (const auto& node : bypassPath) {
        if (newRoute.empty() || newRoute.back() != node) {
            newRoute.push_back(node);
        }
    }

    state.routeIntersections = newRoute;
    state.isRerouted = true;
    state.recalculationCount++;

    RecomputeMetrics(state, nowUs);

    LogWarn("Route RECALCULATED for " + ambulanceId +
            " around failed intersection " + failedIntersectionId +
            " (New Route Hops: " + std::to_string(state.routeIntersections.size()) +
            ", Next: " + state.nextIntersection + ")");
    return true;
}

void RouteProgressTracker::SetRouteReplanner(RouteReplannerCallback replanner) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_replanner = std::move(replanner);
}

bool RouteProgressTracker::GetProgress(const std::string& ambulanceId, RouteProgressState& outState) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        outState = it->second;
        return true;
    }
    return false;
}

double RouteProgressTracker::GetPercentageCompletion(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.percentageCompletion;
    }
    return 0.0;
}

std::string RouteProgressTracker::GetCurrentIntersection(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.currentIntersection;
    }
    return "";
}

std::string RouteProgressTracker::GetNextIntersection(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.nextIntersection;
    }
    return "";
}

std::vector<std::string> RouteProgressTracker::GetRemainingIntersections(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.remainingIntersections;
    }
    return {};
}

double RouteProgressTracker::GetUpdatedEtaSeconds(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.currentEstimatedEtaSeconds;
    }
    return 0.0;
}

bool RouteProgressTracker::IsRouteCompleted(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it != m_activeRoutes.end()) {
        return it->second.isCompleted;
    }
    return false;
}

std::string RouteProgressTracker::GenerateProgressReport(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeRoutes.find(ambulanceId);
    if (it == m_activeRoutes.end()) {
        return "Ambulance " + ambulanceId + " not found.";
    }

    const auto& s = it->second;
    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - ROUTE PROGRESS REPORT: " << s.ambulanceId << "\n"
        << "=========================================================================\n"
        << " Corridor ID     : " << s.corridorId << "\n"
        << " Current Position: " << s.currentIntersection << "\n"
        << " Previous Node   : " << (s.previousIntersection.empty() ? "START" : s.previousIntersection) << "\n"
        << " Next Node       : " << (s.nextIntersection.empty() ? "DESTINATION" : s.nextIntersection) << "\n"
        << " Progress        : " << std::fixed << std::setprecision(1) << s.percentageCompletion << "%\n"
        << " Estimated ETA   : " << s.currentEstimatedEtaSeconds << "s\n"
        << " Speed           : " << s.currentSpeedMps << " m/s (" << (s.currentSpeedMps * 3.6) << " km/h)\n"
        << " Rerouted        : " << (s.isRerouted ? "YES" : "NO") << " (Count: " << s.recalculationCount << ")\n"
        << " Remaining Hops  : " << s.remainingIntersections.size() << "\n"
        << "=========================================================================\n";
    return oss.str();
}

void RouteProgressTracker::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeRoutes.clear();
}

} // namespace corridor
} // namespace aegis
