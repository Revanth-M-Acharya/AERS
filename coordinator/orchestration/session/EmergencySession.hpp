/**
 * @file EmergencySession.hpp
 * @brief Data structure representing an active emergency mission and green corridor lifecycle.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/intelligence/execution/IExecutionPlanGenerator.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace aegis::coordinator::orchestration {

enum class SessionState {
    CREATED,
    ACTIVE,
    PAUSED,
    CANCELLED,
    COMPLETED,
    ARCHIVED
};

inline std::string SessionStateToString(SessionState state) {
    switch (state) {
        case SessionState::CREATED:   return "CREATED";
        case SessionState::ACTIVE:    return "ACTIVE";
        case SessionState::PAUSED:    return "PAUSED";
        case SessionState::CANCELLED: return "CANCELLED";
        case SessionState::COMPLETED: return "COMPLETED";
        case SessionState::ARCHIVED:  return "ARCHIVED";
        default:                      return "UNKNOWN";
    }
}

enum class CorridorState {
    INACTIVE,
    STAGING,
    ACTIVE,
    CLEARED,
    RECOVERING
};

inline std::string CorridorStateToString(CorridorState state) {
    switch (state) {
        case CorridorState::INACTIVE:   return "INACTIVE";
        case CorridorState::STAGING:    return "STAGING";
        case CorridorState::ACTIVE:     return "ACTIVE";
        case CorridorState::CLEARED:    return "CLEARED";
        case CorridorState::RECOVERING: return "RECOVERING";
        default:                        return "UNKNOWN";
    }
}

struct EmergencySession {
    std::string emergencyId;
    std::string ambulanceId;
    std::string hospitalDestination;
    std::vector<std::string> route;
    double etaSeconds{0.0};
    intelligence::ExecutionPlan executionPlan;
    std::string currentIntersection;
    CorridorState corridorState{CorridorState::INACTIVE};
    SessionState sessionState{SessionState::CREATED};
    uint32_t priorityLevel{1};
    uint64_t createdAtMonoMs{0};
    uint64_t updatedAtMonoMs{0};
    uint64_t completedAtMonoMs{0};
    std::string notes;
};

} // namespace aegis::coordinator::orchestration

