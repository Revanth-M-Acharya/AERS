/**
 * @file TrafficDashboardDataModel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Dashboard Central Data Model.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_DATA_MODEL_H
#define AEGIS_DASHBOARD_TRAFFIC_DATA_MODEL_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Detailed real-time telemetry for a single roadside intersection node.
 */
struct IntersectionTelemetry {
    std::string nodeId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};

    // Signal State
    std::string signalState{"RED"}; // RED, YELLOW, GREEN, FLASHING_YELLOW
    std::string currentPhase{"ALL-RED HOLD"};
    int countdownSec{0};

    // Preemption & Override
    bool isPreempted{false};
    std::string activeCorridorId;
    bool manualOverrideActive{false};
    std::string manualOverrideState{"NONE"}; // NONE, HOLD_RED, FORCE_GREEN

    // Hardware & Connectivity
    bool isOnline{true};
    bool isReady{true};
    double lastHeartbeatSec{0.1};
    double latencyMs{0.4};
    double packetLossPct{0.0};
    double cpuPercent{15.0};
    double memPercent{32.0};
    double tempCelsius{42.0};
    std::string gpioSignalState{"RED"};
    std::vector<bool> activeGpioPins{false, false, false, false, false, false, false, false};
};

/**
 * @brief Queue length, density, and throughput telemetry for an intersection.
 */
struct QueueTelemetry {
    std::string nodeId;
    uint32_t queueLengthVehicles{3};
    double estimatedWaitTimeSec{18.0};
    double trafficDensityPct{35.0};
    double laneOccupancyPct{30.0};
    uint32_t currentThroughputVpm{24};
    bool isPeakLoad{false};
};

/**
 * @brief Approach and departure optical IR vehicle detection telemetry (Phase 20 ready).
 */
struct IrSensorTelemetry {
    std::string nodeId;
    bool approachDetected{false};
    bool departureDetected{false};
    std::string approachHealth{"HEALTHY"};     // HEALTHY, DEGRADED, OFFLINE
    std::string departureHealth{"HEALTHY"};    // HEALTHY, DEGRADED, OFFLINE
    uint32_t approachCount{0};
    uint32_t departureCount{0};
    std::string lastDetectionIso;
};

/**
 * @brief Green wave corridor progression and preemption schedule.
 */
struct GreenCorridorSchedule {
    std::string activeCorridorId;
    std::vector<std::string> upcomingCorridors;
    double preemptionWindowStartSec{0.0};
    double preemptionWindowDurationSec{60.0};
    std::string signalTransitionStage{"STANDBY"}; // STANDBY, ALL_RED_CLEARANCE, PREEMPTION_GREEN, RESTORE_CYCLE
    std::string corridorDirection{"NORTH_BOUND"};
    std::vector<std::string> intersectionSequence;
    size_t currentPreemptionIndex{0};
    double estimatedArrivalTimeSec{0.0};
    std::string emergencyVehiclePriority{"P1_CRITICAL"};
    std::string assignedAmbulanceId{"AMB_01"};
};

/**
 * @brief Traffic operational event log entry.
 */
struct TrafficEventLogEntry {
    uint64_t id{0};
    std::string timestampIso;
    std::string nodeId;
    std::string severity{"INFO"}; // INFO, WARN, ERROR, CRITICAL
    std::string category{"SIGNAL"}; // SIGNAL, HEARTBEAT, REGISTRATION, FAILURE, RECOVERY, PREEMPTION, OVERRIDE, DIAGNOSTIC
    std::string message;
};

/**
 * @brief Pop-up banner notification item.
 */
struct TrafficNotification {
    std::string id;
    std::string timestampIso;
    std::string severity{"INFO"}; // INFO, WARN, CRITICAL, SUCCESS
    std::string title;
    std::string message;
    bool isAcknowledged{false};
};

/**
 * @brief Thread-safe central state repository for the Traffic Operations Dashboard.
 */
class TrafficDashboardDataModel : public QObject {
    Q_OBJECT

public:
    explicit TrafficDashboardDataModel(QObject* parent = nullptr);
    ~TrafficDashboardDataModel() override = default;

    // Initialization / Reset
    void ResetToDefaults();

    // Intersection Telemetry
    void UpdateIntersection(const IntersectionTelemetry& telemetry);
    bool GetIntersection(const std::string& nodeId, IntersectionTelemetry& out) const;
    std::vector<IntersectionTelemetry> GetAllIntersections() const;

    // Queue Telemetry
    void UpdateQueue(const QueueTelemetry& queue);
    bool GetQueue(const std::string& nodeId, QueueTelemetry& out) const;
    std::vector<QueueTelemetry> GetAllQueues() const;

    // IR Sensors
    void UpdateIrSensors(const IrSensorTelemetry& ir);
    bool GetIrSensors(const std::string& nodeId, IrSensorTelemetry& out) const;
    std::vector<IrSensorTelemetry> GetAllIrSensors() const;

    // Green Corridor
    void SetCorridorSchedule(const GreenCorridorSchedule& sched);
    GreenCorridorSchedule GetCorridorSchedule() const;

    // Event Logs
    void AddEventLog(const std::string& nodeId,
                     const std::string& severity,
                     const std::string& category,
                     const std::string& message);
    std::vector<TrafficEventLogEntry> GetLogs(size_t limit = 200) const;
    void ClearLogs();

    // Notifications
    void TriggerNotification(const std::string& title,
                             const std::string& message,
                             const std::string& severity = "INFO");
    std::vector<TrafficNotification> GetNotifications() const;
    void ClearNotifications();

    // Selection
    std::string GetSelectedIntersectionId() const;
    void SetSelectedIntersectionId(const std::string& nodeId);

signals:
    void dataModelUpdated();
    void intersectionSelected(const QString& nodeId);
    void notificationAdded(const QString& title, const QString& message, const QString& severity);
    void corridorStateChanged(const QString& corridorId);

private:
    mutable std::mutex m_mutex;

    std::unordered_map<std::string, IntersectionTelemetry> m_intersections;
    std::unordered_map<std::string, QueueTelemetry> m_queues;
    std::unordered_map<std::string, IrSensorTelemetry> m_irSensors;
    GreenCorridorSchedule m_corridorSchedule;
    std::vector<TrafficEventLogEntry> m_logs;
    std::vector<TrafficNotification> m_notifications;
    std::string m_selectedNodeId{"INT_A"};

    uint64_t m_logCounter{0};
    uint64_t m_notificationCounter{0};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_DATA_MODEL_H

