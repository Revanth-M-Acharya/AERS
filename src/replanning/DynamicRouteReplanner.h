/**
 * @file DynamicRouteReplanner.h
 * @brief Dynamic Route Replanning & Live Traffic Adaptation Engine for SMART AEGIS (Phase 9A).
 *
 * Continuously monitors live traffic conditions, ambulance state, and road status.
 * Recomputes optimal emergency corridors, regenerates Green Wave signal schedules,
 * dispatches updated traffic signal commands via CoordinatorManager, and tracks
 * complete route version history.
 */

#ifndef AEGIS_REPLANNING_DYNAMIC_ROUTE_REPLANNER_H
#define AEGIS_REPLANNING_DYNAMIC_ROUTE_REPLANNER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iostream>

#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "coordinator/CoordinatorManager.h"

namespace aegis {
namespace replanning {

// Upstream type aliases
using EmergencyRoute = aegis::green_corridor::EmergencyRoute;
using RoadSegment = aegis::green_corridor::RoadSegment;
using GreenWavePlan = aegis::green_wave::GreenWavePlan;
using GreenWindow = aegis::green_wave::GreenWindow;

/**
 * @brief Live traffic report on a directed road segment.
 */
struct TrafficCondition {
    std::string roadId{""};
    std::string fromIntersection{""};
    std::string toIntersection{""};
    double averageSpeedKmph{0.0};
    double delaySeconds{0.0};
    bool blocked{false};
    uint64_t timestampMs{0};

    TrafficCondition() = default;
    TrafficCondition(std::string rId, std::string from, std::string to,
                     double avgSpeed, double delay, bool isBlocked, uint64_t ts = 0)
        : roadId(std::move(rId))
        , fromIntersection(std::move(from))
        , toIntersection(std::move(to))
        , averageSpeedKmph(avgSpeed)
        , delaySeconds(delay)
        , blocked(isBlocked)
        , timestampMs(ts) {}
};

/**
 * @brief Live dynamic telemetry state of an en-route emergency vehicle.
 */
struct LiveAmbulanceState {
    std::string ambulanceId{""};
    std::string currentIntersection{""};
    std::string destinationHospital{""};
    double currentLatitude{0.0};
    double currentLongitude{0.0};
    double remainingDistanceMeters{0.0};
    double currentEtaSeconds{0.0};
    uint32_t routeVersion{1};

    LiveAmbulanceState() = default;
    LiveAmbulanceState(std::string ambId, std::string currInt, std::string destHosp,
                       double lat = 0.0, double lon = 0.0,
                       double distRemaining = 0.0, double etaSec = 0.0, uint32_t version = 1)
        : ambulanceId(std::move(ambId))
        , currentIntersection(std::move(currInt))
        , destinationHospital(std::move(destHosp))
        , currentLatitude(lat)
        , currentLongitude(lon)
        , remainingDistanceMeters(distRemaining)
        , currentEtaSeconds(etaSec)
        , routeVersion(version) {}
};

/**
 * @brief Detailed outcome of an automated route replanning event.
 */
struct ReplanResult {
    EmergencyRoute oldRoute{};
    EmergencyRoute newRoute{};
    double oldEtaSeconds{0.0};
    double newEtaSeconds{0.0};
    double timeSavedSeconds{0.0};
    bool greenWaveUpdated{false};
    bool commandsRedispatched{false};
    bool success{false};
    std::string reason{""};
    uint64_t timestampMs{0};
};

/**
 * @brief Formats a ReplanResult into the exact Phase 9A structured audit log representation.
 */
inline std::string FormatReplanLog(const ReplanResult& result, const std::string& ambulanceId = "") {
    std::ostringstream oss;
    std::string amb = !ambulanceId.empty() ? ambulanceId
                     : (!result.newRoute.ambulanceId.empty() ? result.newRoute.ambulanceId : "AMB_01");

    oss << "[ROUTE REPLANNED]\n\n"
        << "Ambulance:\n"
        << amb << "\n\n"
        << "Old ETA:\n"
        << std::llround(result.oldEtaSeconds) << " sec\n\n"
        << "New ETA:\n"
        << std::llround(result.newEtaSeconds) << " sec\n\n"
        << "Time Saved:\n"
        << std::llround(result.timeSavedSeconds) << " sec\n\n"
        << "Reason:\n"
        << (result.reason.empty() ? "Road Blocked" : result.reason) << "\n\n"
        << "New Corridor:\n";

    if (!result.newRoute.intersections.empty()) {
        for (size_t i = 0; i < result.newRoute.intersections.size(); ++i) {
            oss << result.newRoute.intersections[i];
            if (i + 1 < result.newRoute.intersections.size()) {
                oss << "\n\n↓\n\n";
            }
        }
        oss << "\n";
    } else {
        oss << "(empty)\n";
    }
    return oss.str();
}

/**
 * @class DynamicRouteReplanner
 * @brief Thread-safe dynamic route adaptation & green wave rescheduling engine.
 */
class DynamicRouteReplanner {
public:
    using CommandDispatcher = std::function<bool(const std::string& intersectionId,
                                                  const std::string& command,
                                                  const std::string& reason)>;

    DynamicRouteReplanner();
    explicit DynamicRouteReplanner(
        std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner,
        std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator = nullptr,
        std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager = nullptr);
    ~DynamicRouteReplanner() = default;

    // Non-copyable for robust concurrent ownership
    DynamicRouteReplanner(const DynamicRouteReplanner&) = delete;
    DynamicRouteReplanner& operator=(const DynamicRouteReplanner&) = delete;

    // =========================================================================
    // Core Phase 9A Public APIs
    // =========================================================================

    /**
     * @brief Ingests or updates a live traffic report on a directed road segment.
     */
    bool UpdateTrafficCondition(const TrafficCondition& condition);

    /**
     * @brief Ingests or updates live ambulance kinematics and position telemetry.
     */
    bool UpdateAmbulanceState(const LiveAmbulanceState& state);

    /**
     * @brief Evaluates whether any replanning rule is triggered.
     * Rules: 1) Road blocked, 2) ETA increase > 20s, 3) Speed < 30 km/h,
     *        4) Faster route available, 5) Coordinator manual request.
     */
    bool ShouldReplan();

    /**
     * @brief Computes the updated optimal emergency route using EmergencyCorridorPlanner.
     */
    EmergencyRoute GenerateUpdatedRoute();

    /**
     * @brief Regenerates the Green Wave schedule for the updated route.
     */
    GreenWavePlan GenerateUpdatedGreenWave();

    /**
     * @brief Commits the new plan, dispatches signal commands via CoordinatorManager,
     *        increments routeVersion, logs audit event, and appends to history.
     */
    bool ApplyNewPlan();

    /**
     * @brief Returns current active corridor route.
     */
    EmergencyRoute GetCurrentRoute() const;

    /**
     * @brief Returns current active Green Wave plan.
     */
    GreenWavePlan GetCurrentGreenWave() const;

    /**
     * @brief Returns the most recent ReplanResult event.
     */
    ReplanResult GetLastReplan() const;

    /**
     * @brief Clears active traffic conditions, restored base road weights, and state.
     */
    void Clear();

    // =========================================================================
    // Route History & Version Management
    // =========================================================================

    std::vector<ReplanResult> GetHistory() const;
    void ClearHistory();
    uint32_t GetRouteVersion() const;

    // =========================================================================
    // State & Context Configuration
    // =========================================================================

    void SetCurrentRoute(const EmergencyRoute& route);
    void SetCurrentGreenWave(const GreenWavePlan& plan);
    LiveAmbulanceState GetAmbulanceState() const;

    void RequestCoordinatorReroute(const std::string& reason = "Coordinator Manual Reroute Request");

    // =========================================================================
    // Subsystem Management & Configuration
    // =========================================================================

    void SetPlanner(std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner);
    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> GetPlanner() const;

    void SetGreenWaveCoordinator(std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator);
    std::shared_ptr<green_wave::GreenWaveCoordinator> GetGreenWaveCoordinator() const;

    void SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager);
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinatorManager() const;

    void SetCommandDispatcher(CommandDispatcher dispatcher);

    void SetConsoleLogging(bool enabled) noexcept;
    bool GetConsoleLogging() const noexcept;

    void SetEtaThresholdSeconds(double seconds) noexcept;
    double GetEtaThresholdSeconds() const noexcept;

    void SetSpeedThresholdKmph(double speedKmph) noexcept;
    double GetSpeedThresholdKmph() const noexcept;

    void SyncBaseRoadSegments();

private:
    static uint64_t CurrentTimeMs();
    static std::string MakeRoadKey(const std::string& from, const std::string& to);
    bool IsRoadOnCurrentCorridor(const std::string& from, const std::string& to) const;

    mutable std::mutex m_mutex;

    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> m_planner;
    std::shared_ptr<green_wave::GreenWaveCoordinator> m_greenWaveCoordinator;
    std::shared_ptr<coordinator::CoordinatorManager> m_coordinatorManager;
    CommandDispatcher m_customDispatcher{nullptr};

    // Base network graph state (uncongested baseline)
    std::unordered_map<std::string, RoadSegment> m_baseRoadSegments{};
    std::unordered_map<std::string, std::string> m_roadIdToKey{};

    // Live traffic database
    std::unordered_map<std::string, TrafficCondition> m_trafficConditions{};

    // Active state
    LiveAmbulanceState m_ambulanceState{};
    EmergencyRoute m_currentRoute{};
    GreenWavePlan m_currentGreenWave{};
    double m_baselineEtaSeconds{0.0};

    // Replanning working candidate state
    EmergencyRoute m_pendingRoute{};
    GreenWavePlan m_pendingGreenWave{};
    std::string m_replanReason{""};
    bool m_trafficChanged{false};
    bool m_coordinatorRerouteRequested{false};
    std::string m_coordinatorReason{""};

    // Configuration thresholds
    double m_etaThresholdSeconds{20.0};
    double m_speedThresholdKmph{30.0};
    bool m_enableConsoleLog{true};

    // Audit and version history
    std::vector<ReplanResult> m_history{};
    ReplanResult m_lastReplan{};
};

} // namespace replanning
} // namespace aegis

#endif // AEGIS_REPLANNING_DYNAMIC_ROUTE_REPLANNER_H
