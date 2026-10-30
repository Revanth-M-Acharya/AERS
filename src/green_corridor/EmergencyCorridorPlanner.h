/**
 * @file EmergencyCorridorPlanner.h
 * @brief Planning engine computing optimal emergency corridors for AERS (Phase 7A).
 *
 * Implements deterministic Dijkstra shortest-path planning across road networks,
 * timeline arrival estimation (ETA) per intersection, thread-safe concurrent
 * querying, and structured route generation audit logging.
 */

#ifndef AEGIS_GREEN_CORRIDOR_EMERGENCY_CORRIDOR_PLANNER_H
#define AEGIS_GREEN_CORRIDOR_EMERGENCY_CORRIDOR_PLANNER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>

namespace aegis {
namespace green_corridor {

/**
 * @brief Represents geographical and identification info for a road intersection.
 */
struct IntersectionInfo {
    std::string intersectionId{""};
    double latitude{0.0};
    double longitude{0.0};

    IntersectionInfo() = default;
    IntersectionInfo(std::string id, double lat, double lon)
        : intersectionId(std::move(id)), latitude(lat), longitude(lon) {}

    bool operator==(const IntersectionInfo& other) const noexcept {
        return intersectionId == other.intersectionId;
    }
};

/**
 * @brief Represents a directed road segment connecting two intersections.
 */
struct RoadSegment {
    std::string fromIntersection{""};
    std::string toIntersection{""};
    double distanceMeters{0.0};
    double estimatedTravelTimeSeconds{0.0};

    RoadSegment() = default;
    RoadSegment(std::string from, std::string to, double distMeters, double travelTimeSec)
        : fromIntersection(std::move(from))
        , toIntersection(std::move(to))
        , distanceMeters(distMeters)
        , estimatedTravelTimeSeconds(travelTimeSec) {}
};

/**
 * @brief Estimated arrival time and progress at a specific corridor intersection.
 */
struct IntersectionEta {
    std::string intersectionId{""};
    double etaSeconds{0.0};
    double cumulativeDistanceMeters{0.0};

    IntersectionEta() = default;
    IntersectionEta(std::string id, double eta, double dist = 0.0)
        : intersectionId(std::move(id)), etaSeconds(eta), cumulativeDistanceMeters(dist) {}
};

/**
 * @brief Complete computed emergency corridor route plan.
 */
struct EmergencyRoute {
    std::vector<std::string> intersections{};
    double totalDistance{0.0};
    double estimatedArrivalTime{0.0};
    std::vector<IntersectionEta> timeline{};
    bool isValid{false};
    std::string ambulanceId{"AMBULANCE"};
    std::string hospitalId{"HOSPITAL"};
};

/**
 * @brief Cost optimization objective for shortest path planning.
 */
enum class CostMetric : uint8_t {
    TRAVEL_TIME = 0, ///< Minimize total estimated travel time (seconds)
    DISTANCE    = 1  ///< Minimize total physical route distance (meters)
};

/**
 * @class EmergencyCorridorPlanner
 * @brief Thread-safe route computation engine for emergency green corridors.
 */
class EmergencyCorridorPlanner {
public:
    EmergencyCorridorPlanner();
    explicit EmergencyCorridorPlanner(CostMetric metric);
    ~EmergencyCorridorPlanner() = default;

    // Non-copyable for robust concurrency ownership
    EmergencyCorridorPlanner(const EmergencyCorridorPlanner&) = delete;
    EmergencyCorridorPlanner& operator=(const EmergencyCorridorPlanner&) = delete;
    EmergencyCorridorPlanner(EmergencyCorridorPlanner&&) noexcept;
    EmergencyCorridorPlanner& operator=(EmergencyCorridorPlanner&&) noexcept;

    // =========================================================================
    // Graph Configuration
    // =========================================================================
    void AddIntersection(const IntersectionInfo& info);
    void AddIntersection(const std::string& id, double lat, double lon);
    void SetIntersections(const std::vector<IntersectionInfo>& intersections);

    void AddRoadSegment(const RoadSegment& segment, bool bidirectional = false);
    void AddRoadSegment(const std::string& from, const std::string& to,
                        double distanceMeters, double estimatedTravelTimeSeconds,
                        bool bidirectional = false);
    void SetRoadSegments(const std::vector<RoadSegment>& segments);

    void ClearGraph();

    // =========================================================================
    // Context / Planner Inputs
    // =========================================================================
    void SetAmbulancePosition(double lat, double lon, const std::string& ambulanceId = "AMBULANCE");
    void SetAmbulancePosition(const std::string& nearestIntersectionId, const std::string& ambulanceId = "AMBULANCE");
    void SetDestinationHospital(const std::string& hospitalId, double lat = 0.0, double lon = 0.0);
    void SetCostMetric(CostMetric metric);

    // =========================================================================
    // Graph Introspection & Nearest Neighbor
    // =========================================================================
    bool HasIntersection(const std::string& id) const;
    IntersectionInfo GetIntersection(const std::string& id) const;
    std::vector<IntersectionInfo> GetAllIntersections() const;
    std::vector<RoadSegment> GetAllRoadSegments() const;
    size_t GetIntersectionCount() const;
    size_t GetRoadSegmentCount() const;
    std::string FindNearestIntersection(double lat, double lon) const;

    // =========================================================================
    // Requirement 5: Primary Route Planning APIs
    // =========================================================================
    EmergencyRoute ComputeEmergencyRoute();
    EmergencyRoute ComputeEmergencyRoute(const std::string& startIntersectionId,
                                        const std::string& destinationHospitalId);
    EmergencyRoute ComputeEmergencyRoute(const std::string& ambulanceId,
                                        const std::string& startIntersectionId,
                                        const std::string& destinationHospitalId,
                                        double initialEtaOffset = 0.0,
                                        double initialDistOffset = 0.0);
    EmergencyRoute ComputeEmergencyRoute(const std::string& ambulanceId,
                                        double ambLat, double ambLon,
                                        const std::string& destinationHospitalId);
    EmergencyRoute ComputeEmergencyRoute(const std::string& ambulanceId,
                                        double ambLat, double ambLon,
                                        const std::string& destinationHospitalId,
                                        double hospLat, double hospLon);

    std::vector<std::string> ComputeGreenCorridor();
    std::vector<std::string> ComputeGreenCorridor(const std::string& startIntersectionId,
                                                  const std::string& destinationHospitalId);
    std::vector<std::string> ComputeGreenCorridor(const std::string& ambulanceId,
                                                  const std::string& startIntersectionId,
                                                  const std::string& destinationHospitalId);

    std::vector<IntersectionEta> EstimateArrivalTimes();
    std::vector<IntersectionEta> EstimateArrivalTimes(const std::vector<std::string>& corridor) const;
    std::vector<IntersectionEta> EstimateArrivalTimes(const EmergencyRoute& route) const;

    EmergencyRoute GetCurrentPlan() const;
    void ClearPlan();

    // =========================================================================
    // Requirement 4, 7, 8: Formatting & Structured Logging
    // =========================================================================
    std::string FormatGreenCorridor(const std::vector<std::string>& corridor) const;
    std::string FormatGreenCorridor() const;
    std::string FormatTimeline(const std::vector<IntersectionEta>& timeline) const;
    std::string FormatTimeline() const;
    static std::string FormatRouteLog(const EmergencyRoute& route);
    std::string GetLastRouteLog() const;
    void PrintRouteLog(const EmergencyRoute& route) const;
    void EnableAutoLogging(bool enable);

private:
    // Internal Dijkstra routing engine (pure functional over graph snapshots)
    EmergencyRoute RunDijkstra(
        const std::unordered_map<std::string, IntersectionInfo>& nodes,
        const std::unordered_map<std::string, std::vector<RoadSegment>>& adj,
        const std::string& startId,
        const std::string& goalId,
        const std::string& ambId,
        double initialEtaOffset,
        double initialDistOffset,
        CostMetric metric
    ) const;

    mutable std::mutex m_mutex;
    CostMetric m_costMetric{CostMetric::TRAVEL_TIME};
    bool m_autoLogging{true};

    // Graph storage
    std::unordered_map<std::string, IntersectionInfo> m_intersections;
    std::unordered_map<std::string, std::vector<RoadSegment>> m_adjacency;
    std::vector<RoadSegment> m_segments;

    // Planner Input State
    std::string m_ambulanceId{"AMBULANCE"};
    std::string m_ambulanceStartNode{""};
    double m_ambulanceLat{0.0};
    double m_ambulanceLon{0.0};
    bool m_ambulancePosSet{false};

    std::string m_hospitalId{"HOSPITAL"};
    double m_hospitalLat{0.0};
    double m_hospitalLon{0.0};
    bool m_hospitalSet{false};

    // Cached current plan
    EmergencyRoute m_currentPlan{};
    std::string m_lastRouteLog{""};
};

} // namespace green_corridor
} // namespace aegis

#endif // AEGIS_GREEN_CORRIDOR_EMERGENCY_CORRIDOR_PLANNER_H
