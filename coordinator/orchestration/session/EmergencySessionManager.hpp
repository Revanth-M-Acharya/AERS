/**
 * @file EmergencySessionManager.hpp
 * @brief Complete lifecycle manager for active emergency response missions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/orchestration/session/EmergencySession.hpp"
#include "coordinator/orchestration/state/CoordinatorStateManager.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>

namespace aegis::coordinator::orchestration {

class EmergencySessionManager {
public:
    EmergencySessionManager(std::shared_ptr<CoordinatorStateManager> stateManager,
                            std::shared_ptr<IEventBus> eventBus);
    ~EmergencySessionManager() = default;

    /**
     * @brief Create a new emergency session in CREATED state.
     */
    bool CreateSession(const std::string& emergencyId,
                       const std::string& ambulanceId,
                       const std::string& hospitalId,
                       uint32_t priority = 1,
                       uint64_t currentMonoMs = 0);

    /**
     * @brief Activate session and begin green corridor staging.
     */
    bool ActivateSession(const std::string& emergencyId, uint64_t currentMonoMs = 0);

    /**
     * @brief Update route, ETA, or execution plan of an active session.
     */
    bool UpdateRoute(const std::string& emergencyId,
                     const std::vector<std::string>& route,
                     double etaSeconds,
                     uint64_t currentMonoMs = 0);

    bool UpdateExecutionPlan(const std::string& emergencyId,
                             const intelligence::ExecutionPlan& plan,
                             uint64_t currentMonoMs = 0);

    bool UpdateCurrentIntersection(const std::string& emergencyId,
                                   const std::string& intersectionId,
                                   uint64_t currentMonoMs = 0);

    bool UpdateCorridorState(const std::string& emergencyId,
                             CorridorState corridorState,
                             uint64_t currentMonoMs = 0);

    /**
     * @brief Pause active corridor (e.g. temporary vehicle halt or manual intervention).
     */
    bool PauseSession(const std::string& emergencyId,
                      const std::string& reason,
                      uint64_t currentMonoMs = 0);

    /**
     * @brief Resume a paused emergency session.
     */
    bool ResumeSession(const std::string& emergencyId, uint64_t currentMonoMs = 0);

    /**
     * @brief Cancel an emergency mission (e.g. false alarm or patient divert).
     */
    bool CancelSession(const std::string& emergencyId,
                       const std::string& reason,
                       uint64_t currentMonoMs = 0);

    /**
     * @brief Mark emergency mission successfully completed upon hospital arrival.
     */
    bool CompleteSession(const std::string& emergencyId, uint64_t currentMonoMs = 0);

    /**
     * @brief Archive a finished/cancelled session and remove from active tracking.
     */
    bool ArchiveSession(const std::string& emergencyId, uint64_t currentMonoMs = 0);

    /**
     * @brief Retrieve session copy by ID.
     */
    std::optional<EmergencySession> GetSession(const std::string& emergencyId) const;

    /**
     * @brief Get list of all sessions currently tracked.
     */
    std::vector<EmergencySession> GetAllSessions() const;

    /**
     * @brief Get all active/executing sessions.
     */
    std::vector<EmergencySession> GetActiveSessions() const;

private:
    std::shared_ptr<CoordinatorStateManager> stateManager_;
    std::shared_ptr<IEventBus> eventBus_;
    mutable std::mutex sessionMutex_;
};

} // namespace aegis::coordinator::orchestration

