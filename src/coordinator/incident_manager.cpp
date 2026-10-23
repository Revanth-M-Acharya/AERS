#include "aegis/core/incident_manager.hpp"
#include "aegis/common/time_utils.hpp"

namespace aegis {
namespace core {
namespace coordinator {

common::Result<std::string> IncidentManager::CreateIncident(const IncidentReport& report) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (report.incidentId.empty()) {
        return common::Result<std::string>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    IncidentRecord rec;
    rec.incidentId = report.incidentId;
    rec.createdAtMs = report.timestamp > 0 ? report.timestamp : common::GetUtcMilliseconds();
    rec.callerName = report.callerName;
    rec.callerContact = report.callerContact;
    rec.location = report.location;
    rec.triageCategory = report.triage;
    rec.incidentType = report.incidentType;
    rec.requiredCapabilities = report.requiredCapabilities;
    rec.description = report.description;
    rec.status = IncidentStatus::REPORTED;

    m_incidents[report.incidentId] = rec;
    return common::Result<std::string>::Ok(report.incidentId);
}

common::Result<IncidentRecord> IncidentManager::GetIncident(const std::string& incidentId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_incidents.find(incidentId);
    if (it != m_incidents.end()) {
        return common::Result<IncidentRecord>::Ok(it->second);
    }
    return common::Result<IncidentRecord>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
}

bool IncidentManager::UpdateStatus(const std::string& incidentId, IncidentStatus newStatus) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_incidents.find(incidentId);
    if (it == m_incidents.end()) {
        return false;
    }
    it->second.status = newStatus;
    if (newStatus == IncidentStatus::RESOLVED || newStatus == IncidentStatus::CANCELLED) {
        it->second.resolvedAtMs = common::GetUtcMilliseconds();
    }
    return true;
}

bool IncidentManager::AssignAmbulance(const std::string& incidentId, const std::string& unitId, const std::string& routeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_incidents.find(incidentId);
    if (it == m_incidents.end()) {
        return false;
    }
    it->second.assignedUnitId = unitId;
    it->second.activeRouteId = routeId;
    it->second.assignedAtMs = common::GetUtcMilliseconds();
    it->second.status = IncidentStatus::ASSIGNED;
    return true;
}

std::vector<IncidentRecord> IncidentManager::GetActiveIncidents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IncidentRecord> active;
    for (const auto& kv : m_incidents) {
        if (kv.second.status != IncidentStatus::RESOLVED && kv.second.status != IncidentStatus::CANCELLED) {
            active.push_back(kv.second);
        }
    }
    return active;
}

std::vector<IncidentRecord> IncidentManager::GetAllIncidents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IncidentRecord> all;
    all.reserve(m_incidents.size());
    for (const auto& kv : m_incidents) {
        all.push_back(kv.second);
    }
    return all;
}

size_t IncidentManager::GetIncidentCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_incidents.size();
}

void IncidentManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_incidents.clear();
}

} // namespace coordinator
} // namespace core
} // namespace aegis
