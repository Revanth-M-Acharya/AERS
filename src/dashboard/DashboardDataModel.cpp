/**
 * @file DashboardDataModel.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Central Real-Time Dashboard Data Model Implementation.
 */

#include "DashboardDataModel.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace aegis {
namespace dashboard {

namespace {
std::string GetCurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer;
#if defined(_WIN32)
    localtime_s(&tmBuffer, &t);
#else
    localtime_r(&t, &tmBuffer);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuffer, "%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
} // anonymous namespace

std::string PipelineStageToString(PipelineStage stage) {
    switch (stage) {
        case PipelineStage::CALL_RECEIVED:        return "CALL_RECEIVED";
        case PipelineStage::PRIORITY_EVALUATED:   return "PRIORITY_EVALUATED";
        case PipelineStage::FLEET_DISPATCHED:     return "FLEET_DISPATCHED";
        case PipelineStage::HOSPITAL_RESERVED:    return "HOSPITAL_RESERVED";
        case PipelineStage::ROUTE_PLANNED:        return "ROUTE_PLANNED";
        case PipelineStage::DYNAMIC_REPLANNED:    return "DYNAMIC_REPLANNED";
        case PipelineStage::CORRIDOR_ACTIVE:      return "CORRIDOR_ACTIVE";
        case PipelineStage::CONSENSUS_COMMITTED:  return "CONSENSUS_COMMITTED";
        case PipelineStage::SIGNALS_PREEMPTED:    return "SIGNALS_PREEMPTED";
        case PipelineStage::HOSPITAL_ARRIVED:     return "HOSPITAL_ARRIVED";
        default:                                  return "UNKNOWN";
    }
}

DashboardDataModel::DashboardDataModel(QObject* parent)
    : QObject(parent)
{
}

void DashboardDataModel::AddOrUpdateEmergency(const EmergencyRecord& record) {
    bool isNew = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_emergencies.find(record.incidentId) == m_emergencies.end()) {
            isNew = true;
        }
        m_emergencies[record.incidentId] = record;
    }

    if (isNew) {
        emit emergencyAdded(QString::fromStdString(record.incidentId));
    } else {
        emit emergencyUpdated(QString::fromStdString(record.incidentId));
    }
    emit dataModelUpdated();
}

bool DashboardDataModel::GetEmergency(const std::string& incidentId, EmergencyRecord& outRecord) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_emergencies.find(incidentId);
    if (it == m_emergencies.end()) {
        return false;
    }
    outRecord = it->second;
    return true;
}

std::vector<EmergencyRecord> DashboardDataModel::GetAllActiveEmergencies() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<EmergencyRecord> active;
    for (const auto& kv : m_emergencies) {
        if (kv.second.isActive) {
            active.push_back(kv.second);
        }
    }
    return active;
}

void DashboardDataModel::RemoveEmergency(const std::string& incidentId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_emergencies.erase(incidentId);
    }
    emit dataModelUpdated();
}

size_t DashboardDataModel::GetActiveEmergencyCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& kv : m_emergencies) {
        if (kv.second.isActive) count++;
    }
    return count;
}

void DashboardDataModel::UpdateClusterNode(const ClusterNodeRecord& node) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_clusterNodes[node.nodeId] = node;
    }
    emit dataModelUpdated();
}

bool DashboardDataModel::GetClusterNode(const std::string& nodeId, ClusterNodeRecord& outNode) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_clusterNodes.find(nodeId);
    if (it == m_clusterNodes.end()) {
        return false;
    }
    outNode = it->second;
    return true;
}

std::vector<ClusterNodeRecord> DashboardDataModel::GetAllClusterNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ClusterNodeRecord> nodes;
    nodes.reserve(m_clusterNodes.size());
    for (const auto& kv : m_clusterNodes) {
        nodes.push_back(kv.second);
    }
    return nodes;
}

void DashboardDataModel::UpdateIntersection(const IntersectionRecord& intersection) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_intersections[intersection.intersectionId] = intersection;
    }
    emit dataModelUpdated();
}

bool DashboardDataModel::GetIntersection(const std::string& intersectionId, IntersectionRecord& outInter) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it == m_intersections.end()) {
        return false;
    }
    outInter = it->second;
    return true;
}

std::vector<IntersectionRecord> DashboardDataModel::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionRecord> list;
    list.reserve(m_intersections.size());
    for (const auto& kv : m_intersections) {
        list.push_back(kv.second);
    }
    return list;
}

void DashboardDataModel::UpdateFleetVehicle(const FleetVehicleRecord& vehicle) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_fleetVehicles[vehicle.vehicleId] = vehicle;
    }
    emit dataModelUpdated();
}

bool DashboardDataModel::GetFleetVehicle(const std::string& vehicleId, FleetVehicleRecord& outVeh) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleetVehicles.find(vehicleId);
    if (it == m_fleetVehicles.end()) {
        return false;
    }
    outVeh = it->second;
    return true;
}

std::vector<FleetVehicleRecord> DashboardDataModel::GetAllFleetVehicles() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<FleetVehicleRecord> list;
    list.reserve(m_fleetVehicles.size());
    for (const auto& kv : m_fleetVehicles) {
        list.push_back(kv.second);
    }
    return list;
}

void DashboardDataModel::UpdateHospital(const HospitalCapacityRecord& hospital) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_hospitals[hospital.hospitalId] = hospital;
    }
    emit dataModelUpdated();
}

bool DashboardDataModel::GetHospital(const std::string& hospitalId, HospitalCapacityRecord& outHosp) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_hospitals.find(hospitalId);
    if (it == m_hospitals.end()) {
        return false;
    }
    outHosp = it->second;
    return true;
}

std::vector<HospitalCapacityRecord> DashboardDataModel::GetAllHospitals() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<HospitalCapacityRecord> list;
    list.reserve(m_hospitals.size());
    for (const auto& kv : m_hospitals) {
        list.push_back(kv.second);
    }
    return list;
}

void DashboardDataModel::AppendLogEntry(const std::string& severity, const std::string& component, const std::string& message) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        DashboardLogEntry entry;
        entry.id = ++m_logCounter;
        entry.timestampStr = GetCurrentTimestampString();
        entry.severity = severity;
        entry.component = component;
        entry.message = message;

        m_logs.push_back(entry);
        if (m_logs.size() > 1000) {
            m_logs.erase(m_logs.begin(), m_logs.begin() + 100);
        }
    }
    emit dataModelUpdated();
}

std::vector<DashboardLogEntry> DashboardDataModel::GetRecentLogs(size_t limit) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logs.size() <= limit) {
        return m_logs;
    }
    return std::vector<DashboardLogEntry>(m_logs.end() - static_cast<ptrdiff_t>(limit), m_logs.end());
}

void DashboardDataModel::TriggerAlert(const std::string& title, const std::string& message, const std::string& severity) {
    std::string alertId = "ALT_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        NotificationAlert alert;
        alert.alertId = alertId;
        alert.title = title;
        alert.message = message;
        alert.severity = severity;
        alert.timestampStr = GetCurrentTimestampString();
        alert.isAcknowledged = false;
        m_alerts.push_back(alert);
    }

    emit alertTriggered(QString::fromStdString(alertId),
                        QString::fromStdString(title),
                        QString::fromStdString(message),
                        QString::fromStdString(severity));
    emit dataModelUpdated();
}

std::vector<NotificationAlert> DashboardDataModel::GetPendingAlerts() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<NotificationAlert> pending;
    for (const auto& a : m_alerts) {
        if (!a.isAcknowledged) {
            pending.push_back(a);
        }
    }
    return pending;
}

void DashboardDataModel::AcknowledgeAlert(const std::string& alertId) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& a : m_alerts) {
            if (a.alertId == alertId) {
                a.isAcknowledged = true;
                break;
            }
        }
    }
    emit dataModelUpdated();
}

CitySummaryMetrics DashboardDataModel::GetCityMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cityMetrics;
}

void DashboardDataModel::UpdateCityMetrics(const CitySummaryMetrics& metrics) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cityMetrics = metrics;
    }
    emit dataModelUpdated();
}

void DashboardDataModel::AddLog(const DashboardLogEntry& entry) {
    AppendLogEntry(entry.severity, entry.component, entry.message);
}

void DashboardDataModel::ClearLogs() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logs.clear();
    }
    emit dataModelUpdated();
}

void DashboardDataModel::SetCorridor(const std::string& incidentId, const std::vector<std::string>& path) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_emergencies.find(incidentId);
        if (it != m_emergencies.end()) {
            it->second.routeWaypoints = path;
        }
    }
    emit dataModelUpdated();
}

void DashboardDataModel::ResetAll() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_emergencies.clear();
        m_clusterNodes.clear();
        m_intersections.clear();
        m_fleetVehicles.clear();
        m_hospitals.clear();
        m_logs.clear();
        m_alerts.clear();
        m_cityMetrics = CitySummaryMetrics{};
    }
    emit dataModelUpdated();
}

} // namespace dashboard
} // namespace aegis
