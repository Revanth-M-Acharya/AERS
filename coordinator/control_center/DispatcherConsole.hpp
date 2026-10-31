/**
 * @file DispatcherConsole.hpp
 * @brief Dispatcher operator console, shift session management, and manual override interface.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/control_center/IncidentManager.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <memory>

namespace aegis::coordinator::control_center {

struct DispatcherSession {
    std::string dispatcherId;
    std::string name;
    bool isLoggedIn{false};
    uint64_t loginTimeMonoMs{0};
    std::vector<std::string> assignedIncidentIds;
};

class DispatcherConsole {
public:
    explicit DispatcherConsole(std::shared_ptr<IncidentManager> incidentManager = nullptr);
    ~DispatcherConsole() = default;

    /**
     * @brief Operator login.
     */
    bool Login(const std::string& dispatcherId, const std::string& name, uint64_t monoMs = 0);

    /**
     * @brief Operator logout.
     */
    bool Logout(const std::string& dispatcherId);

    /**
     * @brief Assign incident to an active dispatcher.
     */
    bool AssignIncident(const std::string& dispatcherId, const std::string& incidentId);

    /**
     * @brief Remove incident assignment from dispatcher upon completion.
     */
    bool UnassignIncident(const std::string& dispatcherId, const std::string& incidentId);

    /**
     * @brief Get next available dispatcher with minimum active workload.
     */
    std::string GetNextAvailableDispatcher() const;

    /**
     * @brief Manual operator override: Escalate priority.
     */
    bool OverridePriority(const std::string& incidentId, PriorityLevel newPriority, const std::string& reason);

    /**
     * @brief Manual operator override: Cancel incident.
     */
    bool OverrideCancelIncident(const std::string& incidentId, const std::string& reason);

    std::optional<DispatcherSession> GetSession(const std::string& dispatcherId) const;
    std::vector<DispatcherSession> GetActiveSessions() const;

    void Reset();

private:
    std::shared_ptr<IncidentManager> incidentManager_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, DispatcherSession> sessions_;
};

} // namespace aegis::coordinator::control_center

