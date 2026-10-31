/**
 * @file DispatcherConsole.cpp
 * @brief Implementation of DispatcherConsole session and override controls.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/DispatcherConsole.hpp"
#include "common/utils/TimeUtils.hpp"
#include <algorithm>

namespace aegis::coordinator::control_center {

DispatcherConsole::DispatcherConsole(std::shared_ptr<IncidentManager> incidentManager)
    : incidentManager_(std::move(incidentManager)) {}

bool DispatcherConsole::Login(const std::string& dispatcherId, const std::string& name, uint64_t monoMs) {
    if (dispatcherId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(dispatcherId);
    if (it != sessions_.end()) {
        it->second.isLoggedIn = true;
        it->second.loginTimeMonoMs = monoMs;
        return true;
    }

    DispatcherSession session;
    session.dispatcherId = dispatcherId;
    session.name = name.empty() ? dispatcherId : name;
    session.isLoggedIn = true;
    session.loginTimeMonoMs = monoMs;
    sessions_[dispatcherId] = session;
    return true;
}

bool DispatcherConsole::Logout(const std::string& dispatcherId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(dispatcherId);
    if (it == sessions_.end()) return false;

    it->second.isLoggedIn = false;
    return true;
}

bool DispatcherConsole::AssignIncident(const std::string& dispatcherId, const std::string& incidentId) {
    if (dispatcherId.empty() || incidentId.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(dispatcherId);
    if (it == sessions_.end() || !it->second.isLoggedIn) return false;

    auto& list = it->second.assignedIncidentIds;
    if (std::find(list.begin(), list.end(), incidentId) == list.end()) {
        list.push_back(incidentId);
    }
    return true;
}

bool DispatcherConsole::UnassignIncident(const std::string& dispatcherId, const std::string& incidentId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(dispatcherId);
    if (it == sessions_.end()) return false;

    auto& list = it->second.assignedIncidentIds;
    list.erase(std::remove(list.begin(), list.end(), incidentId), list.end());
    return true;
}

std::string DispatcherConsole::GetNextAvailableDispatcher() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string bestId;
    size_t minLoad = 10000;

    for (const auto& [id, sess] : sessions_) {
        if (sess.isLoggedIn) {
            if (sess.assignedIncidentIds.size() < minLoad) {
                minLoad = sess.assignedIncidentIds.size();
                bestId = id;
            }
        }
    }
    return bestId;
}

bool DispatcherConsole::OverridePriority(const std::string& incidentId, PriorityLevel newPriority, const std::string& reason) {
    if (!incidentManager_) return false;
    auto optInc = incidentManager_->GetIncident(incidentId);
    if (!optInc.has_value()) return false;

    IncidentRecord rec = optInc.value();
    rec.priority = newPriority;
    rec.missionHistory.push_back("Priority manually overridden to " + PriorityLevelToString(newPriority) + " (" + reason + ")");
    incidentManager_->CreateIncident(rec); // Updates or creates
    return true;
}

bool DispatcherConsole::OverrideCancelIncident(const std::string& incidentId, const std::string& reason) {
    if (!incidentManager_) return false;
    return incidentManager_->CancelIncident(incidentId, "Dispatcher override: " + reason);
}

std::optional<DispatcherSession> DispatcherConsole::GetSession(const std::string& dispatcherId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(dispatcherId);
    if (it != sessions_.end()) return it->second;
    return std::nullopt;
}

std::vector<DispatcherSession> DispatcherConsole::GetActiveSessions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<DispatcherSession> active;
    for (const auto& [_, sess] : sessions_) {
        if (sess.isLoggedIn) active.push_back(sess);
    }
    return active;
}

void DispatcherConsole::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.clear();
}

} // namespace aegis::coordinator::control_center

