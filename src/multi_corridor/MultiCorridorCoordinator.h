/**
 * @file MultiCorridorCoordinator.h
 * @brief Multi-Ambulance Corridor Coordination & Conflict Resolution Engine for SMART AEGIS (Phase 9B).
 *
 * Coordinates simultaneous en-route ambulances, detects overlapping emergency corridors,
 * resolves intersection contention deterministically using 5-tier arbitration rules,
 * adjusts Green Wave schedules for safe waiting, and triggers DynamicRouteReplanner
 * when rerouting is required.
 */

#ifndef AEGIS_MULTI_CORRIDOR_COORDINATOR_H
#define AEGIS_MULTI_CORRIDOR_COORDINATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "scheduler/PriorityScheduler.h"
#include "replanning/DynamicRouteReplanner.h"
#include "coordinator/CoordinatorManager.h"

namespace aegis {
namespace multi_corridor {

// Convenient upstream type aliases
using EmergencyRoute = aegis::green_corridor::EmergencyRoute;
using IntersectionEta = aegis::green_corridor::IntersectionEta;
using GreenWavePlan = aegis::green_wave::GreenWavePlan;
using GreenWindow = aegis::green_wave::GreenWindow;
using PatientSeverity = aegis::scheduler::PatientSeverity;

// =============================================================================
// Enums
// =============================================================================

/**
 * @brief Categorization of corridor contention types.
 */
enum class ConflictType {
    INTERSECTION_CONFLICT,  ///< Single intersection shared by multiple ambulances
    TIME_CONFLICT,          ///< Multiple ambulances arrive at junction within 3 seconds
    OPPOSITE_DIRECTION,     ///< Ambulances traverse the same junction in opposite directions
    SHARED_CORRIDOR,        ///< Corridors share 2 or more consecutive intersections
    FULL_CORRIDOR_OVERLAP   ///< Complete route overlap between two ambulances
};

inline std::string ConflictTypeToString(ConflictType type) {
    switch (type) {
        case ConflictType::INTERSECTION_CONFLICT: return "INTERSECTION_CONFLICT";
        case ConflictType::TIME_CONFLICT:         return "TIME_CONFLICT";
        case ConflictType::OPPOSITE_DIRECTION:    return "OPPOSITE_DIRECTION";
        case ConflictType::SHARED_CORRIDOR:       return "SHARED_CORRIDOR";
        case ConflictType::FULL_CORRIDOR_OVERLAP: return "FULL_CORRIDOR_OVERLAP";
        default:                                  return "UNKNOWN";
    }
}

/**
 * @brief Severity level of a detected corridor conflict.
 */
enum class ConflictSeverity {
    LOW,
    MEDIUM,
    HIGH,
    CRITICAL
};

inline std::string ConflictSeverityToString(ConflictSeverity severity) {
    switch (severity) {
        case ConflictSeverity::LOW:      return "LOW";
        case ConflictSeverity::MEDIUM:   return "MEDIUM";
        case ConflictSeverity::HIGH:     return "HIGH";
        case ConflictSeverity::CRITICAL: return "CRITICAL";
        default:                         return "UNKNOWN";
    }
}

/**
 * @brief Ergonomic priority level supporting PatientSeverity enum and string literals.
 */
struct Priority {
    PatientSeverity value{PatientSeverity::LOW};

    Priority() = default;
    Priority(PatientSeverity p) : value(p) {}
    Priority(const std::string& str) : value(aegis::scheduler::StringToPatientSeverity(str)) {}
    Priority(const char* str) : value(aegis::scheduler::StringToPatientSeverity(str ? str : "")) {}

    operator PatientSeverity() const noexcept { return value; }
    operator std::string() const { return aegis::scheduler::PatientSeverityToString(value); }

    bool operator==(PatientSeverity other) const noexcept { return value == other; }
    bool operator!=(PatientSeverity other) const noexcept { return value != other; }
    bool operator==(const Priority& other) const noexcept { return value == other.value; }
    bool operator!=(const Priority& other) const noexcept { return value != other.value; }
    bool operator==(const std::string& str) const { return ToString() == str; }
    bool operator==(const char* str) const { return ToString() == (str ? str : ""); }

    int Rank() const noexcept { return aegis::scheduler::SeverityToRank(value); }
    std::string ToString() const { return aegis::scheduler::PatientSeverityToString(value); }
};

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Active en-route ambulance corridor registration.
 */
struct ActiveCorridor {
    std::string ambulanceId{""};
    std::string hospitalId{""};
    Priority priority{PatientSeverity::LOW};
    EmergencyRoute route{};
    GreenWavePlan greenWave{};
    std::string currentIntersection{""};
    double ETA{0.0};
    uint32_t routeVersion{1};
    bool active{true};

    // Metadata for tie-breaking and tracking
    double remainingDistanceMeters{0.0};
    uint64_t timestamp{0};
    uint64_t timestampMs{0};

    ActiveCorridor() = default;
    ActiveCorridor(std::string ambId, std::string hospId, Priority prio,
                   EmergencyRoute r, GreenWavePlan gw = {}, std::string currInt = "",
                   double etaVal = 0.0, uint32_t version = 1, bool isActive = true,
                   double remainingDist = 0.0, uint64_t ts = 0)
        : ambulanceId(std::move(ambId))
        , hospitalId(std::move(hospId))
        , priority(prio)
        , route(std::move(r))
        , greenWave(std::move(gw))
        , currentIntersection(std::move(currInt))
        , ETA(etaVal)
        , routeVersion(version)
        , active(isActive)
        , remainingDistanceMeters(remainingDist)
        , timestamp(ts)
        , timestampMs(ts)
    {
        if (currentIntersection.empty() && !route.intersections.empty()) {
            currentIntersection = route.intersections.front();
        }
        if (ETA == 0.0 && route.estimatedArrivalTime > 0.0) {
            ETA = route.estimatedArrivalTime;
        }
        if (remainingDistanceMeters == 0.0 && route.totalDistance > 0.0) {
            remainingDistanceMeters = route.totalDistance;
        }
    }
};

/**
 * @brief Detected corridor conflict between two active ambulances.
 */
struct CorridorConflict {
    std::string ambulanceA{""};
    std::string ambulanceB{""};
    std::string intersectionId{""};
    double ETA_A{0.0};
    double ETA_B{0.0};
    ConflictType conflictType{ConflictType::INTERSECTION_CONFLICT};
    ConflictType type{ConflictType::INTERSECTION_CONFLICT}; // Alias
    ConflictSeverity severity{ConflictSeverity::LOW};
    std::string resolution{""};
    uint64_t timestamp{0};
    uint64_t timestampMs{0};

    CorridorConflict() = default;
    CorridorConflict(std::string ambA, std::string ambB, std::string intId,
                     double etaA, double etaB, ConflictType cType,
                     ConflictSeverity sev, std::string res = "", uint64_t ts = 0)
        : ambulanceA(std::move(ambA))
        , ambulanceB(std::move(ambB))
        , intersectionId(std::move(intId))
        , ETA_A(etaA)
        , ETA_B(etaB)
        , conflictType(cType)
        , type(cType)
        , severity(sev)
        , resolution(std::move(res))
        , timestamp(ts)
        , timestampMs(ts) {}
};

/**
 * @brief Outcome of a conflict arbitration decision between two ambulances.
 */
struct CoordinationDecision {
    std::string winner{""};
    std::string loser{""};
    double delaySeconds{0.0};
    bool greenWaveAdjusted{false};
    bool rerouteTriggered{false};
    bool successful{false};
    std::string conflictIntersection{""};
    std::string reason{""};
    uint64_t timestamp{0};
};

// =============================================================================
// Formatting Helpers
// =============================================================================

inline std::string FormatConflict(const CorridorConflict& conflict) {
    std::ostringstream oss;
    oss << "Conflict: " << conflict.ambulanceA << " vs " << conflict.ambulanceB
        << " at " << conflict.intersectionId
        << " [Type: " << ConflictTypeToString(conflict.conflictType)
        << ", Severity: " << ConflictSeverityToString(conflict.severity) << "]";
    return oss.str();
}

inline std::string FormatResolution(const CoordinationDecision& decision) {
    std::ostringstream oss;
    oss << "Winner: " << decision.winner << ", Loser: " << decision.loser;
    if (decision.rerouteTriggered) {
        oss << ", Action: REROUTE";
    } else if (decision.greenWaveAdjusted) {
        oss << ", Action: DELAY " << std::llround(decision.delaySeconds) << "s";
    }
    return oss.str();
}

inline std::string FormatCoordinatorLog(const CorridorConflict& conflict, const CoordinationDecision& decision) {
    std::ostringstream oss;
    oss << "[MULTI CORRIDOR]\n\n"
        << "Conflict\n"
        << conflict.ambulanceA << "\n"
        << conflict.ambulanceB << "\n\n"
        << "Intersection\n"
        << conflict.intersectionId << "\n\n"
        << "Winner\n"
        << decision.winner << "\n\n"
        << "Resolution\n";
    if (decision.rerouteTriggered) {
        oss << "Reroute " << decision.loser << "\n\n"
            << "Delay\n"
            << "0 sec\n";
    } else {
        long long delay = static_cast<long long>(std::llround(decision.delaySeconds));
        oss << "Delay " << decision.loser << "\n\n"
            << "Delay\n"
            << delay << " sec\n";
    }
    return oss.str();
}

// =============================================================================
// MultiCorridorCoordinator Class Definition
// =============================================================================

/**
 * @class MultiCorridorCoordinator
 * @brief Thread-safe Multi-Ambulance Corridor Coordination & Conflict Resolution Engine.
 */
class MultiCorridorCoordinator {
public:
    using CommandDispatcher = std::function<bool(const std::string& intersectionId,
                                                  const std::string& command,
                                                  const std::string& reason)>;
    using RerouteCallback = std::function<bool(const std::string& ambulanceId,
                                               const std::string& intersectionId)>;

    MultiCorridorCoordinator();
    explicit MultiCorridorCoordinator(
        std::shared_ptr<replanning::DynamicRouteReplanner> replanner,
        std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager = nullptr);
    ~MultiCorridorCoordinator() = default;

    // Non-copyable for robust concurrent ownership
    MultiCorridorCoordinator(const MultiCorridorCoordinator&) = delete;
    MultiCorridorCoordinator& operator=(const MultiCorridorCoordinator&) = delete;

    // =========================================================================
    // Core Phase 9B Public APIs
    // =========================================================================

    /**
     * @brief Registers a new active emergency corridor in the coordination engine.
     */
    bool RegisterCorridor(const ActiveCorridor& corridor);

    /**
     * @brief Updates an existing active emergency corridor with latest telemetry.
     */
    bool UpdateCorridor(const ActiveCorridor& corridor);

    /**
     * @brief Removes a corridor from active coordination (e.g. ambulance arrived).
     */
    bool RemoveCorridor(const std::string& ambulanceId);

    /**
     * @brief Compares all active corridors O(N^2) and identifies all points of contention.
     * Detects:
     *  1. Same intersection used by multiple ambulances.
     *  2. Arrival times within 3 seconds.
     *  3. Opposite directions through same junction.
     *  4. Shared corridor containing multiple consecutive intersections.
     *  5. Entire corridor overlap.
     */
    std::vector<CorridorConflict> DetectConflicts();

    /**
     * @brief Resolves a specific corridor conflict using deterministic 5-tier rules.
     * Rule 1: Higher medical priority wins (CRITICAL > HIGH > MEDIUM > LOW).
     * Rule 2: Lower ETA wins.
     * Rule 3: Shorter remaining distance wins.
     * Rule 4: Earlier timestamp wins.
     * Rule 5: Lexicographical Ambulance ID wins.
     *
     * If loser can safely wait: Adjust Green Wave timing.
     * Otherwise: Trigger DynamicRouteReplanner.
     */
    CoordinationDecision ResolveConflict(const CorridorConflict& conflict);

    /**
     * @brief Automatically detects and resolves all current corridor conflicts.
     */
    std::vector<CoordinationDecision> ResolveAllConflicts();

    /**
     * @brief Returns snapshot of all currently registered active corridors.
     */
    std::vector<ActiveCorridor> GetActiveCorridors() const;

    /**
     * @brief Returns complete history of detected conflicts.
     */
    std::vector<CorridorConflict> GetConflictHistory() const;

    /**
     * @brief Returns complete history of coordination decisions.
     */
    std::vector<CoordinationDecision> GetResolutionHistory() const;

    /**
     * @brief Clears conflict and resolution histories.
     */
    void ClearHistory();

    // Formatting API wrappers
    std::string FormatConflict(const CorridorConflict& conflict) const {
        return aegis::multi_corridor::FormatConflict(conflict);
    }
    std::string FormatResolution(const CoordinationDecision& decision) const {
        return aegis::multi_corridor::FormatResolution(decision);
    }
    std::string FormatCoordinatorLog(const CorridorConflict& conflict, const CoordinationDecision& decision) const {
        return aegis::multi_corridor::FormatCoordinatorLog(conflict, decision);
    }

    // =========================================================================
    // Subsystem Integration & Configuration
    // =========================================================================

    void SetDynamicRouteReplanner(std::shared_ptr<replanning::DynamicRouteReplanner> replanner);
    std::shared_ptr<replanning::DynamicRouteReplanner> GetDynamicRouteReplanner() const;

    void SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> manager);
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinatorManager() const;

    void SetCommandDispatcher(CommandDispatcher dispatcher);
    void SetRerouteCallback(RerouteCallback callback);

    void SetMaxSafeWaitSeconds(double seconds) noexcept;
    double GetMaxSafeWaitSeconds() const noexcept;

    void SetDefaultDelaySeconds(double seconds) noexcept;
    double GetDefaultDelaySeconds() const noexcept;

    void SetForceReroute(bool force) noexcept;
    bool GetForceReroute() const noexcept;

    void SetConsoleLogging(bool enabled) noexcept;
    bool GetConsoleLogging() const noexcept;

    void Clear();
    void ClearCorridors();

    bool GetCorridor(const std::string& ambulanceId, ActiveCorridor& outCorridor) const;
    bool HasCorridor(const std::string& ambulanceId) const;
    size_t GetActiveCorridorCount() const;

private:
    static uint64_t CurrentTimeMs();
    static double GetIntersectionEta(const ActiveCorridor& corridor, const std::string& intersectionId, size_t nodeIdx = static_cast<size_t>(-1));
    static bool IsWinner(const ActiveCorridor& a, const ActiveCorridor& b);

    mutable std::mutex m_mutex;

    std::shared_ptr<replanning::DynamicRouteReplanner> m_replanner;
    std::shared_ptr<coordinator::CoordinatorManager> m_coordinatorManager;
    CommandDispatcher m_customDispatcher{nullptr};
    RerouteCallback m_rerouteCallback{nullptr};

    std::unordered_map<std::string, ActiveCorridor> m_activeCorridors{};

    std::vector<CorridorConflict> m_conflictHistory{};
    std::vector<CoordinationDecision> m_resolutionHistory{};

    double m_maxSafeWaitSeconds{10.0};
    double m_defaultDelaySeconds{5.0};
    bool m_forceReroute{false};
    bool m_enableConsoleLog{true};
};

} // namespace multi_corridor
} // namespace aegis

#endif // AEGIS_MULTI_CORRIDOR_COORDINATOR_H
