/**
 * @file DashboardDataModel.h
 * @brief SMART AEGIS (AERS) Phase 17 - Central Real-Time Dashboard Data Model.
 */

#ifndef AEGIS_DASHBOARD_DATA_MODEL_H
#define AEGIS_DASHBOARD_DATA_MODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace dashboard {

/**
 * @brief 10-stage sequential Emergency Pipeline Progression.
 */
enum class PipelineStage : uint8_t {
    CALL_RECEIVED = 0,
    PRIORITY_EVALUATED,
    FLEET_DISPATCHED,
    HOSPITAL_RESERVED,
    ROUTE_PLANNED,
    DYNAMIC_REPLANNED,
    CORRIDOR_ACTIVE,
    CONSENSUS_COMMITTED,
    SIGNALS_PREEMPTED,
    HOSPITAL_ARRIVED
};

std::string PipelineStageToString(PipelineStage stage);

/**
 * @brief Comprehensive Active Emergency Incident Record.
 */
struct EmergencyRecord {
    std::string incidentId;
    std::string callId;
    std::string callerName;
    std::string location;
    std::string condition;
    std::string complaint;
    std::string vitalsStr;
    double triageScore{0.0};
    double clinicalScore{0.0};
    double latitude{0.0};
    double longitude{0.0};
    std::string priorityLevel{"P1_CRITICAL"};
    std::string assignedAmbulanceId;
    std::string selectedHospitalId;
    std::string assignedHospitalId;
    uint32_t etaSeconds{0};
    PipelineStage currentStage{PipelineStage::CALL_RECEIVED};
    double stageProgress{0.0}; // 0.0 to 1.0
    std::vector<std::string> routeWaypoints;
    std::vector<std::string> preemptedIntersections;
    std::string greenCorridorPlanId;
    bool isActive{true};
    bool isResolved{false};
    uint64_t epochTimestampMs{0};
};

/**
 * @brief Cluster Hardware & Process Node Telemetry.
 */
struct ClusterNodeRecord {
    std::string nodeId;
    std::string roleStr{"TRAFFIC_NODE"};
    std::string role{"TRAFFIC_NODE"};
    std::string ipAddress{"127.0.0.1"};
    bool isOnline{false};
    bool isReady{false};
    bool isLeader{false};
    uint64_t lastHeartbeatEpochMs{0};
    uint32_t missedHeartbeats{0};
    double cpuPercent{0.0};
    double cpuUsagePct{0.0};
    double memPercent{0.0};
    double memoryUsagePct{0.0};
    double tempCelsius{0.0};
    double temperatureCelsius{0.0};
    double latencyMs{0.0};
    double lastHeartbeatSec{0.0};
    std::string gpioSignalState{"RED"};
    std::string gpioStatusStr;
    std::vector<bool> activeGpioPins{false, false, false, false, false, false, false, false};
};

/**
 * @brief Roadside Intersection Signal State.
 */
struct IntersectionRecord {
    std::string intersectionId;
    std::string nodeId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    std::string signalState{"RED"}; // RED, YELLOW, GREEN, FLASHING_YELLOW
    std::string currentPhaseStr{"ALL_RED"};
    std::string currentPhase{"ALL-RED HOLD"};
    uint32_t remainingSeconds{0};
    int countdownSec{0};
    bool isPreempted{false};
    std::string corridorId;
    std::string activeCorridorId;
    bool irSensorActive{false};
    bool overrideActive{false};
    bool manualOverrideActive{false};
};

/**
 * @brief Fleet Vehicle Telemetry Record.
 */
struct FleetVehicleRecord {
    std::string vehicleId;
    std::string driverName{"EMS Unit"};
    std::string statusStr{"AVAILABLE"};
    std::string status{"AVAILABLE"};
    double speedKmh{0.0};
    double headingDeg{0.0};
    double latitude{0.0};
    double longitude{0.0};
    std::string destination;
    uint32_t etaMinutes{0};
    double transitEtaSec{0.0};
    std::string priorityLevel{"P1_CRITICAL"};
    std::string assignedIncidentId;
    std::string assignedHospitalId;
    std::string currentIntersectionId;
    std::string vitalsSummary;
    bool hasPatientOnboard{false};

    struct PatientVitals {
        uint16_t heartRateBpm{0};
        uint16_t spo2Pct{0};
        uint16_t bloodPressureSystolic{0};
        uint16_t bloodPressureDiastolic{0};
        uint16_t gcsScore{15};
    };
    PatientVitals patientVitals;
};

/**
 * @brief Hospital Capacity & Real-Time Readiness.
 */
struct HospitalCapacityRecord {
    std::string hospitalId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    uint32_t icuBedsAvailable{0};
    uint32_t icuBedsTotal{0};
    uint32_t availableIcuBeds{0};
    uint32_t totalIcuBeds{0};
    uint32_t edBedsAvailable{0};
    uint32_t edBedsTotal{0};
    uint32_t availableEdBeds{0};
    uint32_t totalEdBeds{0};
    std::string erStatusStr{"NORMAL"};
    bool isDiverting{false};
    double distanceKm{0.0};
    uint32_t etaMinutes{0};
    std::string assignedDoctor{"Dr. Roberts, MD"};
};

/**
 * @brief System Event Audit Log Entry.
 */
struct DashboardLogEntry {
    uint64_t id{0};
    std::string timestampStr;
    std::string timestampIso;
    std::string severity{"INFO"}; // INFO, WARN, ERROR, CRITICAL
    std::string component;
    std::string message;
};

/**
 * @brief Notification / Pop-up Alert Banner Item.
 */
struct NotificationAlert {
    std::string alertId;
    std::string title;
    std::string message;
    std::string severity{"INFO"};
    std::string timestampStr;
    bool isAcknowledged{false};
};

/**
 * @brief City Operational Metrics Summary.
 */
struct CitySummaryMetrics {
    std::string cityConditionStr{"NORMAL"};
    uint32_t activeCorridorsCount{0};
    uint32_t preemptedIntersectionsCount{0};
    double avgTransitEtaSec{0.0};
    double fleetAvailabilityPct{100.0};
    uint32_t totalEmergenciesHandled{0};
};

/**
 * @brief Thread-safe central data model powering all dashboard widgets.
 */
class DashboardDataModel : public QObject {
    Q_OBJECT

public:
    explicit DashboardDataModel(QObject* parent = nullptr);
    ~DashboardDataModel() override = default;

    // Emergency Incidents
    void AddOrUpdateEmergency(const EmergencyRecord& record);
    bool GetEmergency(const std::string& incidentId, EmergencyRecord& outRecord) const;
    std::vector<EmergencyRecord> GetAllActiveEmergencies() const;
    void RemoveEmergency(const std::string& incidentId);
    size_t GetActiveEmergencyCount() const;

    // Cluster Nodes
    void UpdateClusterNode(const ClusterNodeRecord& node);
    bool GetClusterNode(const std::string& nodeId, ClusterNodeRecord& outNode) const;
    std::vector<ClusterNodeRecord> GetAllClusterNodes() const;

    // Intersections
    void UpdateIntersection(const IntersectionRecord& intersection);
    bool GetIntersection(const std::string& intersectionId, IntersectionRecord& outInter) const;
    std::vector<IntersectionRecord> GetAllIntersections() const;

    // Fleet Vehicles
    void UpdateFleetVehicle(const FleetVehicleRecord& vehicle);
    bool GetFleetVehicle(const std::string& vehicleId, FleetVehicleRecord& outVeh) const;
    std::vector<FleetVehicleRecord> GetAllFleetVehicles() const;

    // Hospitals
    void UpdateHospital(const HospitalCapacityRecord& hospital);
    bool GetHospital(const std::string& hospitalId, HospitalCapacityRecord& outHosp) const;
    std::vector<HospitalCapacityRecord> GetAllHospitals() const;

    // Event Logs
    void AppendLogEntry(const std::string& severity, const std::string& component, const std::string& message);
    void AddLog(const DashboardLogEntry& entry);
    std::vector<DashboardLogEntry> GetRecentLogs(size_t limit = 200) const;
    std::vector<DashboardLogEntry> GetLogs(size_t limit = 200) const { return GetRecentLogs(limit); }
    void ClearLogs();

    // Notifications
    void TriggerAlert(const std::string& title, const std::string& message, const std::string& severity = "INFO");
    std::vector<NotificationAlert> GetPendingAlerts() const;
    void AcknowledgeAlert(const std::string& alertId);

    // City Summary Metrics
    CitySummaryMetrics GetCityMetrics() const;
    void UpdateCityMetrics(const CitySummaryMetrics& metrics);
    void SetCityMetrics(const CitySummaryMetrics& metrics) { UpdateCityMetrics(metrics); }

    // Corridor and Routing helpers
    void SetCorridor(const std::string& incidentId, const std::vector<std::string>& path);

    // Aliases for convenience
    std::vector<ClusterNodeRecord> GetClusterNodes() const { return GetAllClusterNodes(); }
    std::vector<IntersectionRecord> GetIntersections() const { return GetAllIntersections(); }
    std::vector<FleetVehicleRecord> GetFleet() const { return GetAllFleetVehicles(); }
    std::vector<HospitalCapacityRecord> GetHospitals() const { return GetAllHospitals(); }
    void ResetToDefaults() { ResetAll(); }

    // Bulk reset
    void ResetAll();

signals:
    void dataModelUpdated();
    void emergencyAdded(const QString& incidentId);
    void emergencyUpdated(const QString& incidentId);
    void alertTriggered(const QString& alertId, const QString& title, const QString& message, const QString& severity);

private:
    mutable std::mutex m_mutex;

    std::unordered_map<std::string, EmergencyRecord> m_emergencies;
    std::unordered_map<std::string, ClusterNodeRecord> m_clusterNodes;
    std::unordered_map<std::string, IntersectionRecord> m_intersections;
    std::unordered_map<std::string, FleetVehicleRecord> m_fleetVehicles;
    std::unordered_map<std::string, HospitalCapacityRecord> m_hospitals;
    std::vector<DashboardLogEntry> m_logs;
    std::vector<NotificationAlert> m_alerts;
    CitySummaryMetrics m_cityMetrics;
    uint64_t m_logCounter{0};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_DATA_MODEL_H
