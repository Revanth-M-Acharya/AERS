/**
 * @file EmergencySessionManager.cpp
 * @brief Implementation of EmergencySessionManager lifecycle operations.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::orchestration {

EmergencySessionManager::EmergencySessionManager(std::shared_ptr<CoordinatorStateManager> stateManager,
                                                 std::shared_ptr<IEventBus> eventBus)
    : stateManager_(std::move(stateManager)), eventBus_(std::move(eventBus)) {}

bool EmergencySessionManager::CreateSession(const std::string& emergencyId,
                                            const std::string& ambulanceId,
                                            const std::string& hospitalId,
                                            uint32_t priority,
                                            uint64_t currentMonoMs) {
    if (emergencyId.empty() || ambulanceId.empty()) return false;
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(sessionMutex_);
    if (stateManager_->GetEmergencySession(emergencyId).has_value()) {
        return false; // Already exists
    }

    EmergencySession session;
    session.emergencyId = emergencyId;
    session.ambulanceId = ambulanceId;
    session.hospitalDestination = hospitalId;
    session.priorityLevel = priority;
    session.sessionState = SessionState::CREATED;
    session.corridorState = CorridorState::INACTIVE;
    session.createdAtMonoMs = currentMonoMs;
    session.updatedAtMonoMs = currentMonoMs;

    stateManager_->SaveEmergencySession(session);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::EmergencyCreated, emergencyId, ambulanceId,
                                 "SessionCreated: priority=" + std::to_string(priority),
                                 currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::ActivateSession(const std::string& emergencyId, uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    if (session.sessionState != SessionState::CREATED && session.sessionState != SessionState::PAUSED) {
        return false;
    }

    session.sessionState = SessionState::ACTIVE;
    session.corridorState = CorridorState::ACTIVE;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::ExecutionStarted, emergencyId, session.ambulanceId,
                                 "SessionActivated", currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::UpdateRoute(const std::string& emergencyId,
                                          const std::vector<std::string>& route,
                                          double etaSeconds,
                                          uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.route = route;
    session.etaSeconds = etaSeconds;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::RouteChanged, emergencyId, session.ambulanceId,
                                 "RouteUpdated: hops=" + std::to_string(route.size()),
                                 currentMonoMs));
        eventBus_->Publish(Event(EventType::ETAUpdated, emergencyId, session.ambulanceId,
                                 "ETAUpdated: eta=" + std::to_string(etaSeconds),
                                 currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::UpdateExecutionPlan(const std::string& emergencyId,
                                                  const intelligence::ExecutionPlan& plan,
                                                  uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.executionPlan = plan;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    stateManager_->SetActiveExecutionPlan(plan.planId, plan);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::ExecutionPlanApproved, emergencyId, plan.planId,
                                 "PlanUpdated: checksum=" + std::to_string(plan.checksum),
                                 currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::UpdateCurrentIntersection(const std::string& emergencyId,
                                                        const std::string& intersectionId,
                                                        uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.currentIntersection = intersectionId;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    return true;
}

bool EmergencySessionManager::UpdateCorridorState(const std::string& emergencyId,
                                                  CorridorState corridorState,
                                                  uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.corridorState = corridorState;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    return true;
}

bool EmergencySessionManager::PauseSession(const std::string& emergencyId,
                                           const std::string& reason,
                                           uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    if (session.sessionState != SessionState::ACTIVE) return false;

    session.sessionState = SessionState::PAUSED;
    session.corridorState = CorridorState::STAGING;
    session.notes = reason;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    return true;
}

bool EmergencySessionManager::ResumeSession(const std::string& emergencyId, uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    if (session.sessionState != SessionState::PAUSED) return false;

    session.sessionState = SessionState::ACTIVE;
    session.corridorState = CorridorState::ACTIVE;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    return true;
}

bool EmergencySessionManager::CancelSession(const std::string& emergencyId,
                                            const std::string& reason,
                                            uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.sessionState = SessionState::CANCELLED;
    session.corridorState = CorridorState::CLEARED;
    session.notes = reason;
    session.updatedAtMonoMs = currentMonoMs;
    session.completedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::EmergencyFinished, emergencyId, session.ambulanceId,
                                 "SessionCancelled: " + reason, currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::CompleteSession(const std::string& emergencyId, uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    session.sessionState = SessionState::COMPLETED;
    session.corridorState = CorridorState::CLEARED;
    session.updatedAtMonoMs = currentMonoMs;
    session.completedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);

    if (eventBus_) {
        eventBus_->Publish(Event(EventType::ExecutionCompleted, emergencyId, session.ambulanceId,
                                 "SessionCompleted", currentMonoMs));
        eventBus_->Publish(Event(EventType::EmergencyFinished, emergencyId, session.ambulanceId,
                                 "SessionFinished", currentMonoMs));
    }
    return true;
}

bool EmergencySessionManager::ArchiveSession(const std::string& emergencyId, uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(sessionMutex_);

    auto optSession = stateManager_->GetEmergencySession(emergencyId);
    if (!optSession.has_value()) return false;

    EmergencySession session = optSession.value();
    if (session.sessionState != SessionState::COMPLETED && session.sessionState != SessionState::CANCELLED) {
        return false; // Can only archive finished/cancelled sessions
    }

    session.sessionState = SessionState::ARCHIVED;
    session.updatedAtMonoMs = currentMonoMs;
    stateManager_->SaveEmergencySession(session);
    return true;
}

std::optional<EmergencySession> EmergencySessionManager::GetSession(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return stateManager_->GetEmergencySession(emergencyId);
}

std::vector<EmergencySession> EmergencySessionManager::GetAllSessions() const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return stateManager_->GetAllEmergencySessions();
}

std::vector<EmergencySession> EmergencySessionManager::GetActiveSessions() const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return stateManager_->GetActiveEmergencySessions();
}

} // namespace aegis::coordinator::orchestration

