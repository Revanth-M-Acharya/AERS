/**
 * @file RouteProgressTracker.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Real-Time Intersection-by-Intersection Route Progress Tracker.
 *
 * Tracks emergency vehicle progression through scheduled corridors:
 * - Current position, previous intersection, next intersection, and remaining path
 * - Real-time percentage completion calculation (0.0% - 100.0%)
 * - Dynamic ETA recalculation based on actual traversing velocity
 * - Support for seamless in-flight route replanning around failed/blocked intersections
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CORRIDOR_ROUTE_PROGRESS_TRACKER_HPP
#define AEGIS_CORRIDOR_ROUTE_PROGRESS_TRACKER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iomanip>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"

namespace aegis {
namespace corridor {

/**
 * @brief Representation of an individual intersection waypoint along a tracked corridor route.
 */
struct WaypointNode {
    std::string intersectionId{""};
    double distanceFromStartMeters{0.0};
    double estimatedSegmentDurationSec{15.0};
    bool isPassed{false};
    uint64_t arrivalTimestampUs{0};
    uint64_t departureTimestampUs{0};
};

/**
 * @brief Complete spatial and temporal progress state for an emergency vehicle.
 */
struct RouteProgressState {
    std::string ambulanceId{""};
    std::string corridorId{""};
    std::string destinationId{""};

    std::vector<std::string> routeIntersections{};
    std::vector<WaypointNode> waypoints{};

    double totalDistanceMeters{1000.0};
    double passedDistanceMeters{0.0};

    std::string currentIntersection{""};
    std::string previousIntersection{""};
    std::string nextIntersection{""};
    std::vector<std::string> remainingIntersections{};
    std::vector<std::string> passedIntersections{};

    double percentageCompletion{0.0};      ///< Dynamic completion percentage: 0.0% - 100.0%
    double initialEtaSeconds{60.0};
    double currentEstimatedEtaSeconds{60.0};
    double currentSpeedMps{13.88};          ///< Current estimated speed (~50 km/h)
    uint64_t lastUpdateTimestampUs{0};

    bool isCompleted{false};
    bool isRerouted{false};
    uint32_t recalculationCount{0};
};

/**
 * @class RouteProgressTracker
 * @brief Deterministic, thread-safe tracker supervising route progression and in-flight replanning.
 */
class RouteProgressTracker {
public:
    using RouteReplannerCallback = std::function<std::vector<std::string>(
        const std::string& currentPos,
        const std::string& destination,
        const std::string& failedNode)>;

    RouteProgressTracker();
    ~RouteProgressTracker() = default;

    RouteProgressTracker(const RouteProgressTracker&) = delete;
    RouteProgressTracker& operator=(const RouteProgressTracker&) = delete;
    RouteProgressTracker(RouteProgressTracker&&) = delete;
    RouteProgressTracker& operator=(RouteProgressTracker&&) = delete;

    /**
     * @brief Initializes route tracking for an ambulance.
     */
    bool InitializeRoute(const std::string& ambulanceId,
                         const std::string& corridorId,
                         const std::vector<std::string>& route,
                         double totalDistanceMeters = 1000.0,
                         double etaSeconds = 60.0,
                         uint64_t timestampUs = 0);

    /**
     * @brief Reports arrival at an intersection (e.g. Approach IR sensor triggered).
     */
    bool ReportArrival(const std::string& ambulanceId,
                       const std::string& intersectionId,
                       uint64_t timestampUs = 0);

    /**
     * @brief Reports departure from an intersection (e.g. Departure IR sensor triggered).
     */
    bool ReportDeparture(const std::string& ambulanceId,
                         const std::string& intersectionId,
                         uint64_t timestampUs = 0);

    /**
     * @brief Updates estimated speed and adjusts downstream ETAs.
     */
    bool UpdateVelocity(const std::string& ambulanceId, double speedMps);

    /**
     * @brief In-flight route recalculation around a failed or congested intersection.
     * Preserves already traversed nodes, replaces remaining path, updates ETA.
     */
    bool RecalculateRouteOnFailure(const std::string& ambulanceId,
                                  const std::string& failedIntersectionId,
                                  uint64_t timestampUs = 0);

    /**
     * @brief Registers an external shortest-path replanner function.
     */
    void SetRouteReplanner(RouteReplannerCallback replanner);

    /**
     * @brief Retrieves active progress state for an ambulance.
     */
    bool GetProgress(const std::string& ambulanceId, RouteProgressState& outState) const;

    /**
     * @brief Queries current percentage completion (0.0 - 100.0%).
     */
    double GetPercentageCompletion(const std::string& ambulanceId) const;

    /**
     * @brief Returns current intersection position.
     */
    std::string GetCurrentIntersection(const std::string& ambulanceId) const;

    /**
     * @brief Returns immediate next downstream intersection.
     */
    std::string GetNextIntersection(const std::string& ambulanceId) const;

    /**
     * @brief Returns list of remaining downstream intersections.
     */
    std::vector<std::string> GetRemainingIntersections(const std::string& ambulanceId) const;

    /**
     * @brief Returns dynamically updated ETA in seconds.
     */
    double GetUpdatedEtaSeconds(const std::string& ambulanceId) const;

    /**
     * @brief Checks if route has been completed.
     */
    bool IsRouteCompleted(const std::string& ambulanceId) const;

    /**
     * @brief Generates ASCII diagnostic progress report.
     */
    std::string GenerateProgressReport(const std::string& ambulanceId) const;

    /**
     * @brief Clears all internal tracking states.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;
    void RecomputeMetrics(RouteProgressState& state, uint64_t nowUs);

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, RouteProgressState> m_activeRoutes;
    RouteReplannerCallback m_replanner{nullptr};
};

} // namespace corridor
} // namespace aegis

#endif // AEGIS_CORRIDOR_ROUTE_PROGRESS_TRACKER_HPP
