/**
 * @file TrafficDashboardDataModel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Dashboard Central Data Model Implementation.
 */

#include "TrafficDashboardDataModel.h"
#include <QtCore/QDateTime>
#include <algorithm>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficDashboardDataModel::TrafficDashboardDataModel(QObject* parent)
    : QObject(parent)
{
    ResetToDefaults();
}

void TrafficDashboardDataModel::ResetToDefaults() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_intersections.clear();
        m_queues.clear();
        m_irSensors.clear();
        m_logs.clear();
        m_notifications.clear();
        m_selectedNodeId = "INT_A";

        // Initial Intersections
        const std::vector<std::tuple<std::string, std::string, double, double>> nodes = {
            {"INT_A", "Brigade Road / MG Intersection", 12.9716, 77.5946},
            {"INT_B", "MG Road / Residency Intersection", 12.9725, 77.5958},
            {"INT_C", "Hospital Approach Intersection", 12.9738, 77.5972}
        };

        for (const auto& [nodeId, name, lat, lon] : nodes) {
            IntersectionTelemetry inter;
            inter.nodeId = nodeId;
            inter.name = name;
            inter.latitude = lat;
            inter.longitude = lon;
            inter.signalState = (nodeId == "INT_A") ? "GREEN" : (nodeId == "INT_B" ? "YELLOW" : "RED");
            inter.currentPhase = (nodeId == "INT_A") ? "NORTH_SOUTH_GREEN" : (nodeId == "INT_B" ? "CLEARANCE_YELLOW" : "ALL_RED");
            inter.countdownSec = (nodeId == "INT_A") ? 18 : (nodeId == "INT_B" ? 3 : 22);
            inter.isPreempted = false;
            inter.manualOverrideActive = false;
            inter.manualOverrideState = "NONE";
            inter.isOnline = true;
            inter.isReady = true;
            inter.lastHeartbeatSec = 0.1;
            inter.latencyMs = 0.4;
            inter.packetLossPct = 0.0;
            inter.cpuPercent = 14.5;
            inter.memPercent = 31.0;
            inter.tempCelsius = 41.5;
            inter.gpioSignalState = inter.signalState;
            inter.activeGpioPins = {true, true, false, false, false, false, false, false};
            m_intersections[nodeId] = inter;

            QueueTelemetry q;
            q.nodeId = nodeId;
            q.queueLengthVehicles = (nodeId == "INT_A") ? 5 : (nodeId == "INT_B" ? 8 : 3);
            q.estimatedWaitTimeSec = static_cast<double>(q.queueLengthVehicles) * 4.2;
            q.trafficDensityPct = (nodeId == "INT_A") ? 40.0 : (nodeId == "INT_B" ? 65.0 : 25.0);
            q.laneOccupancyPct = q.trafficDensityPct * 0.85;
            q.currentThroughputVpm = 28;
            q.isPeakLoad = (q.queueLengthVehicles >= 10);
            m_queues[nodeId] = q;

            IrSensorTelemetry ir;
            ir.nodeId = nodeId;
            ir.approachDetected = false;
            ir.departureDetected = false;
            ir.approachHealth = "HEALTHY";
            ir.departureHealth = "HEALTHY";
            ir.approachCount = 142;
            ir.departureCount = 138;
            ir.lastDetectionIso = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();
            m_irSensors[nodeId] = ir;
        }

        // Default Corridor Schedule
        m_corridorSchedule.activeCorridorId = "";
        m_corridorSchedule.upcomingCorridors = {};
        m_corridorSchedule.preemptionWindowStartSec = 0.0;
        m_corridorSchedule.preemptionWindowDurationSec = 60.0;
        m_corridorSchedule.signalTransitionStage = "STANDBY";
        m_corridorSchedule.corridorDirection = "NORTH_BOUND";
        m_corridorSchedule.intersectionSequence = {"INT_A", "INT_B", "INT_C"};
        m_corridorSchedule.currentPreemptionIndex = 0;
        m_corridorSchedule.estimatedArrivalTimeSec = 0.0;
        m_corridorSchedule.emergencyVehiclePriority = "P1_CRITICAL";
        m_corridorSchedule.assignedAmbulanceId = "AMB_01";

        // Initial logs
        TrafficEventLogEntry l1;
        l1.id = ++m_logCounter;
        l1.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        l1.nodeId = "INT_A";
        l1.severity = "INFO";
        l1.category = "REGISTRATION";
        l1.message = "Intersection INT_A registered to Traffic Operations Console";
        m_logs.push_back(l1);

        TrafficEventLogEntry l2;
        l2.id = ++m_logCounter;
        l2.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        l2.nodeId = "INT_B";
        l2.severity = "INFO";
        l2.category = "REGISTRATION";
        l2.message = "Intersection INT_B registered to Traffic Operations Console";
        m_logs.push_back(l2);

        TrafficEventLogEntry l3;
        l3.id = ++m_logCounter;
        l3.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        l3.nodeId = "INT_C";
        l3.severity = "INFO";
        l3.category = "REGISTRATION";
        l3.message = "Intersection INT_C registered to Traffic Operations Console";
        m_logs.push_back(l3);
    }

    emit dataModelUpdated();
}

void TrafficDashboardDataModel::UpdateIntersection(const IntersectionTelemetry& telemetry) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_intersections[telemetry.nodeId] = telemetry;
    }
    emit dataModelUpdated();
}

bool TrafficDashboardDataModel::GetIntersection(const std::string& nodeId, IntersectionTelemetry& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(nodeId);
    if (it != m_intersections.end()) {
        out = it->second;
        return true;
    }
    return false;
}

std::vector<IntersectionTelemetry> TrafficDashboardDataModel::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionTelemetry> list;
    list.reserve(m_intersections.size());
    for (const auto& [id, inter] : m_intersections) {
        (void)id;
        list.push_back(inter);
    }
    // Sort predictably by nodeId
    std::sort(list.begin(), list.end(), [](const IntersectionTelemetry& a, const IntersectionTelemetry& b) {
        return a.nodeId < b.nodeId;
    });
    return list;
}

void TrafficDashboardDataModel::UpdateQueue(const QueueTelemetry& queue) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queues[queue.nodeId] = queue;
    }
    emit dataModelUpdated();
}

bool TrafficDashboardDataModel::GetQueue(const std::string& nodeId, QueueTelemetry& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(nodeId);
    if (it != m_queues.end()) {
        out = it->second;
        return true;
    }
    return false;
}

std::vector<QueueTelemetry> TrafficDashboardDataModel::GetAllQueues() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<QueueTelemetry> list;
    list.reserve(m_queues.size());
    for (const auto& [id, q] : m_queues) {
        (void)id;
        list.push_back(q);
    }
    std::sort(list.begin(), list.end(), [](const QueueTelemetry& a, const QueueTelemetry& b) {
        return a.nodeId < b.nodeId;
    });
    return list;
}

void TrafficDashboardDataModel::UpdateIrSensors(const IrSensorTelemetry& ir) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_irSensors[ir.nodeId] = ir;
    }
    emit dataModelUpdated();
}

bool TrafficDashboardDataModel::GetIrSensors(const std::string& nodeId, IrSensorTelemetry& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_irSensors.find(nodeId);
    if (it != m_irSensors.end()) {
        out = it->second;
        return true;
    }
    return false;
}

std::vector<IrSensorTelemetry> TrafficDashboardDataModel::GetAllIrSensors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IrSensorTelemetry> list;
    list.reserve(m_irSensors.size());
    for (const auto& [id, s] : m_irSensors) {
        (void)id;
        list.push_back(s);
    }
    std::sort(list.begin(), list.end(), [](const IrSensorTelemetry& a, const IrSensorTelemetry& b) {
        return a.nodeId < b.nodeId;
    });
    return list;
}

void TrafficDashboardDataModel::SetCorridorSchedule(const GreenCorridorSchedule& sched) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_corridorSchedule = sched;
    }
    emit corridorStateChanged(QString::fromStdString(sched.activeCorridorId));
    emit dataModelUpdated();
}

GreenCorridorSchedule TrafficDashboardDataModel::GetCorridorSchedule() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_corridorSchedule;
}

void TrafficDashboardDataModel::AddEventLog(const std::string& nodeId,
                                           const std::string& severity,
                                           const std::string& category,
                                           const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        TrafficEventLogEntry entry;
        entry.id = ++m_logCounter;
        entry.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss.zzz").toStdString();
        entry.nodeId = nodeId;
        entry.severity = severity;
        entry.category = category;
        entry.message = message;
        m_logs.push_back(entry);

        if (m_logs.size() > 1000) {
            m_logs.erase(m_logs.begin(), m_logs.begin() + 100);
        }
    }
    emit dataModelUpdated();
}

std::vector<TrafficEventLogEntry> TrafficDashboardDataModel::GetLogs(size_t limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logs.size() <= limit) {
        return m_logs;
    }
    return std::vector<TrafficEventLogEntry>(m_logs.end() - static_cast<ptrdiff_t>(limit), m_logs.end());
}

void TrafficDashboardDataModel::ClearLogs() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.clear();
    }
    emit dataModelUpdated();
}

void TrafficDashboardDataModel::TriggerNotification(const std::string& title,
                                                   const std::string& message,
                                                   const std::string& severity) {
    std::string notifId = "NOTIF_" + std::to_string(++m_notificationCounter);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        TrafficNotification n;
        n.id = notifId;
        n.timestampIso = QDateTime::currentDateTime().toString("hh:mm:ss").toStdString();
        n.severity = severity;
        n.title = title;
        n.message = message;
        n.isAcknowledged = false;
        m_notifications.push_back(n);
    }

    emit notificationAdded(QString::fromStdString(title),
                           QString::fromStdString(message),
                           QString::fromStdString(severity));
    emit dataModelUpdated();
}

std::vector<TrafficNotification> TrafficDashboardDataModel::GetNotifications() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_notifications;
}

void TrafficDashboardDataModel::ClearNotifications() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_notifications.clear();
    }
    emit dataModelUpdated();
}

std::string TrafficDashboardDataModel::GetSelectedIntersectionId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_selectedNodeId;
}

void TrafficDashboardDataModel::SetSelectedIntersectionId(const std::string& nodeId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_selectedNodeId = nodeId;
    }
    emit intersectionSelected(QString::fromStdString(nodeId));
    emit dataModelUpdated();
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

