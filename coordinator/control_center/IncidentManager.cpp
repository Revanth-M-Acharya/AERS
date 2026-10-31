/**
 * @file IncidentManager.cpp
 * @brief Implementation of IncidentManager CRUD and historical auditing.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/IncidentManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::control_center {

bool IncidentManager::CreateIncident(const IncidentRecord& record) {
    if (record.incidentId.empty() || record.emergencyId.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (incidents_.find(record.incidentId) != incidents_.end()) {
        return false;
    }

    IncidentRecord rec = record;
    if (rec.createdAtMonoMs == 0) rec.createdAtMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    rec.updatedAtMonoMs = rec.createdAtMonoMs;
    rec.missionHistory.push_back("Incident created in state: " + IncidentStateToString(rec.state));

    incidents_[rec.incidentId] = rec;
    emergencyToIncidentMap_[rec.emergencyId] = rec.incidentId;
    return true;
}

bool IncidentManager::UpdateState(const std::string& incidentId, IncidentState newState, const std::string& note, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    it->second.state = newState;
    it->second.updatedAtMonoMs = monoMs;
    std::string entry = "Transitioned to " + IncidentStateToString(newState);
    if (!note.empty()) entry += " (" + note + ")";
    it->second.missionHistory.push_back(entry);
    return true;
}

bool IncidentManager::AssignResources(const std::string& incidentId,
                                     const std::string& ambulanceId,
                                     const std::string& hospitalId,
                                     const std::string& dispatcherId) {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    if (!ambulanceId.empty()) it->second.assignedAmbulanceId = ambulanceId;
    if (!hospitalId.empty()) it->second.assignedHospitalId = hospitalId;
    if (!dispatcherId.empty()) it->second.assignedDispatcherId = dispatcherId;
    it->second.updatedAtMonoMs = now;
    it->second.missionHistory.push_back("Resources assigned: Amb=" + ambulanceId + ", Hosp=" + hospitalId + ", Disp=" + dispatcherId);
    return true;
}

bool IncidentManager::UpdateRouteAndETA(const std::string& incidentId,
                                       const std::vector<std::string>& route,
                                       double etaSeconds,
                                       uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    it->second.currentRoute = route;
    it->second.currentEtaSeconds = etaSeconds;
    it->second.updatedAtMonoMs = monoMs;
    it->second.missionHistory.push_back("Route updated: " + std::to_string(route.size()) + " hops, ETA: " + std::to_string(etaSeconds) + "s");
    return true;
}

bool IncidentManager::CancelIncident(const std::string& incidentId, const std::string& reason, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    it->second.state = IncidentState::CANCELLED;
    it->second.updatedAtMonoMs = monoMs;
    it->second.closedAtMonoMs = monoMs;
    it->second.missionHistory.push_back("Incident cancelled: " + reason);
    return true;
}

bool IncidentManager::CompleteIncident(const std::string& incidentId, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    it->second.state = IncidentState::HANDOVER_COMPLETE;
    it->second.updatedAtMonoMs = monoMs;
    it->second.closedAtMonoMs = monoMs;
    it->second.missionHistory.push_back("Incident handover completed");
    return true;
}

bool IncidentManager::ArchiveIncident(const std::string& incidentId, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) return false;

    if (it->second.state != IncidentState::HANDOVER_COMPLETE &&
        it->second.state != IncidentState::CANCELLED) {
        return false;
    }

    it->second.state = IncidentState::CLOSED;
    it->second.updatedAtMonoMs = monoMs;
    it->second.closedAtMonoMs = monoMs;
    it->second.missionHistory.push_back("Incident archived & closed");
    return true;
}

std::optional<IncidentRecord> IncidentManager::GetIncident(const std::string& incidentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = incidents_.find(incidentId);
    if (it != incidents_.end()) return it->second;
    return std::nullopt;
}

std::optional<IncidentRecord> IncidentManager::GetIncidentByEmergencyId(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto mapIt = emergencyToIncidentMap_.find(emergencyId);
    if (mapIt == emergencyToIncidentMap_.end()) return std::nullopt;

    auto it = incidents_.find(mapIt->second);
    if (it != incidents_.end()) return it->second;
    return std::nullopt;
}

std::vector<IncidentRecord> IncidentManager::GetAllActiveIncidents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IncidentRecord> active;
    for (const auto& [_, inc] : incidents_) {
        if (inc.state != IncidentState::CLOSED && inc.state != IncidentState::CANCELLED) {
            active.push_back(inc);
        }
    }
    return active;
}

std::vector<IncidentRecord> IncidentManager::GetAllIncidents() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IncidentRecord> all;
    all.reserve(incidents_.size());
    for (const auto& [_, inc] : incidents_) {
        all.push_back(inc);
    }
    return all;
}

size_t IncidentManager::GetActiveIncidentCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, inc] : incidents_) {
        if (inc.state != IncidentState::CLOSED && inc.state != IncidentState::CANCELLED) {
            count++;
        }
    }
    return count;
}

void IncidentManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    incidents_.clear();
    emergencyToIncidentMap_.clear();
}

} // namespace aegis::coordinator::control_center

