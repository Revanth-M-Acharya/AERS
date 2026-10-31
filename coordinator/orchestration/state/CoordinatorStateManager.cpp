/**
 * @file CoordinatorStateManager.cpp
 * @brief Thread-safe single source of truth implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"

namespace aegis::coordinator::orchestration {

CoordinatorStateManager::CoordinatorStateManager() = default;

void CoordinatorStateManager::SetSystemState(const std::string& state) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    systemState_ = state;
}

std::string CoordinatorStateManager::GetSystemState() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    return systemState_;
}

void CoordinatorStateManager::RegisterTrafficNode(const TrafficNodeRecord& node) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    trafficNodes_[node.nodeId] = node;
    if (nodeHealth_.find(node.nodeId) == nodeHealth_.end()) {
        NodeHealthRecord hr;
        hr.nodeId = node.nodeId;
        hr.healthState = NodeHealthState::ONLINE;
        hr.connectionStatus = ConnectionStatus::CONNECTED;
        hr.lastHeartbeatMonoMs = node.lastSeenMonoMs;
        nodeHealth_[node.nodeId] = hr;
    }
}

void CoordinatorStateManager::UnregisterTrafficNode(const std::string& nodeId) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    trafficNodes_.erase(nodeId);
    nodeHealth_.erase(nodeId);
    nodeSyncs_.erase(nodeId);
}

bool CoordinatorStateManager::HasTrafficNode(const std::string& nodeId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    return trafficNodes_.find(nodeId) != trafficNodes_.end();
}

std::optional<TrafficNodeRecord> CoordinatorStateManager::GetTrafficNode(const std::string& nodeId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = trafficNodes_.find(nodeId);
    if (it != trafficNodes_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<TrafficNodeRecord> CoordinatorStateManager::GetAllTrafficNodes() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<TrafficNodeRecord> result;
    result.reserve(trafficNodes_.size());
    for (const auto& [_, node] : trafficNodes_) {
        result.push_back(node);
    }
    return result;
}

size_t CoordinatorStateManager::GetTrafficNodeCount() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    return trafficNodes_.size();
}

void CoordinatorStateManager::UpdateNodeHealth(const NodeHealthRecord& health) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    nodeHealth_[health.nodeId] = health;
}

std::optional<NodeHealthRecord> CoordinatorStateManager::GetNodeHealth(const std::string& nodeId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = nodeHealth_.find(nodeId);
    if (it != nodeHealth_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<NodeHealthRecord> CoordinatorStateManager::GetAllNodeHealth() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<NodeHealthRecord> result;
    result.reserve(nodeHealth_.size());
    for (const auto& [_, health] : nodeHealth_) {
        result.push_back(health);
    }
    return result;
}

void CoordinatorStateManager::SetNodeConnectionStatus(const std::string& nodeId, ConnectionStatus status) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto it = nodeHealth_.find(nodeId);
    if (it != nodeHealth_.end()) {
        it->second.connectionStatus = status;
        if (status == ConnectionStatus::DISCONNECTED) {
            it->second.healthState = NodeHealthState::OFFLINE;
        }
    }
}

ConnectionStatus CoordinatorStateManager::GetNodeConnectionStatus(const std::string& nodeId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = nodeHealth_.find(nodeId);
    if (it != nodeHealth_.end()) {
        return it->second.connectionStatus;
    }
    return ConnectionStatus::DISCONNECTED;
}

void CoordinatorStateManager::RegisterAmbulance(const AmbulanceRecord& ambulance) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    ambulances_[ambulance.ambulanceId] = ambulance;
}

void CoordinatorStateManager::UnregisterAmbulance(const std::string& ambulanceId) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    ambulances_.erase(ambulanceId);
}

std::optional<AmbulanceRecord> CoordinatorStateManager::GetAmbulance(const std::string& ambulanceId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = ambulances_.find(ambulanceId);
    if (it != ambulances_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<AmbulanceRecord> CoordinatorStateManager::GetAllAmbulances() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<AmbulanceRecord> result;
    result.reserve(ambulances_.size());
    for (const auto& [_, amb] : ambulances_) {
        result.push_back(amb);
    }
    return result;
}

void CoordinatorStateManager::RegisterHospital(const HospitalRecord& hospital) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    hospitals_[hospital.hospitalId] = hospital;
}

std::optional<HospitalRecord> CoordinatorStateManager::GetHospital(const std::string& hospitalId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void CoordinatorStateManager::UpdateHospitalCapacity(const std::string& hospitalId, uint32_t availableBeds) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) {
        it->second.availableBeds = availableBeds;
    }
}

std::vector<HospitalRecord> CoordinatorStateManager::GetAllHospitals() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<HospitalRecord> result;
    result.reserve(hospitals_.size());
    for (const auto& [_, hosp] : hospitals_) {
        result.push_back(hosp);
    }
    return result;
}

void CoordinatorStateManager::SaveEmergencySession(const EmergencySession& session) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    emergencySessions_[session.emergencyId] = session;
}

std::optional<EmergencySession> CoordinatorStateManager::GetEmergencySession(const std::string& emergencyId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = emergencySessions_.find(emergencyId);
    if (it != emergencySessions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<EmergencySession> CoordinatorStateManager::GetAllEmergencySessions() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<EmergencySession> result;
    result.reserve(emergencySessions_.size());
    for (const auto& [_, sess] : emergencySessions_) {
        result.push_back(sess);
    }
    return result;
}

std::vector<EmergencySession> CoordinatorStateManager::GetActiveEmergencySessions() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<EmergencySession> result;
    for (const auto& [_, sess] : emergencySessions_) {
        if (sess.sessionState == SessionState::ACTIVE || sess.sessionState == SessionState::PAUSED) {
            result.push_back(sess);
        }
    }
    return result;
}

bool CoordinatorStateManager::RemoveEmergencySession(const std::string& emergencyId) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    return emergencySessions_.erase(emergencyId) > 0;
}

void CoordinatorStateManager::SetActiveExecutionPlan(const std::string& planId, const intelligence::ExecutionPlan& plan) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    activePlans_[planId] = plan;
}

std::optional<intelligence::ExecutionPlan> CoordinatorStateManager::GetActiveExecutionPlan(const std::string& planId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = activePlans_.find(planId);
    if (it != activePlans_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<intelligence::ExecutionPlan> CoordinatorStateManager::GetAllActiveExecutionPlans() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<intelligence::ExecutionPlan> result;
    result.reserve(activePlans_.size());
    for (const auto& [_, plan] : activePlans_) {
        result.push_back(plan);
    }
    return result;
}

void CoordinatorStateManager::RemoveActiveExecutionPlan(const std::string& planId) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    activePlans_.erase(planId);
}

void CoordinatorStateManager::UpdateNodeSync(const NodeSyncRecord& sync) {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    nodeSyncs_[sync.nodeId] = sync;
}

std::optional<NodeSyncRecord> CoordinatorStateManager::GetNodeSync(const std::string& nodeId) const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    auto it = nodeSyncs_.find(nodeId);
    if (it != nodeSyncs_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<NodeSyncRecord> CoordinatorStateManager::GetAllNodeSync() const {
    std::shared_lock<std::shared_mutex> lock(rwMutex_);
    std::vector<NodeSyncRecord> result;
    result.reserve(nodeSyncs_.size());
    for (const auto& [_, sync] : nodeSyncs_) {
        result.push_back(sync);
    }
    return result;
}

void CoordinatorStateManager::Reset() {
    std::unique_lock<std::shared_mutex> lock(rwMutex_);
    systemState_ = "IDLE";
    trafficNodes_.clear();
    nodeHealth_.clear();
    ambulances_.clear();
    hospitals_.clear();
    emergencySessions_.clear();
    activePlans_.clear();
    nodeSyncs_.clear();
}

} // namespace aegis::coordinator::orchestration

