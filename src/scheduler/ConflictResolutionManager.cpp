/**
 * @file ConflictResolutionManager.cpp
 * @brief Implementation of ConflictResolutionManager command generators and coordinator integration.
 */

#include "scheduler/ConflictResolutionManager.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

std::string ResolutionCommand::ToString() const {
    std::ostringstream oss;
    oss << "ResolutionCommand [" << commandId << "]\n"
        << "  Ambulance    : " << ambulanceId << "\n"
        << "  Intersection : " << intersectionId << "\n"
        << "  Action       : " << ResolutionActionToString(action) << "\n"
        << "  Delay        : " << delaySeconds << "s\n"
        << "  Reason       : " << reason;
    if (!alternateRoute.empty()) {
        oss << "\n  Alt Route    : ";
        for (size_t i = 0; i < alternateRoute.size(); ++i) {
            oss << alternateRoute[i];
            if (i + 1 < alternateRoute.size()) oss << " -> ";
        }
    }
    return oss.str();
}

std::string ResolutionCommand::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"command_id\":\"" << commandId << "\","
        << "\"ambulance_id\":\"" << ambulanceId << "\","
        << "\"intersection_id\":\"" << intersectionId << "\","
        << "\"action\":\"" << ResolutionActionToString(action) << "\","
        << "\"delay_seconds\":" << delaySeconds << ","
        << "\"reason\":\"" << reason << "\","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << ","
        << "\"alternate_route\":[";

    for (size_t i = 0; i < alternateRoute.size(); ++i) {
        oss << "\"" << alternateRoute[i] << "\"";
        if (i + 1 < alternateRoute.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

uint64_t ConflictResolutionManager::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

ConflictResolutionManager::ConflictResolutionManager(std::shared_ptr<coordinator::CoordinatorManager> coordinator)
    : m_coordinator(std::move(coordinator)) {}

void ConflictResolutionManager::SetCoordinator(std::shared_ptr<coordinator::CoordinatorManager> coordinator) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinator = std::move(coordinator);
}

std::shared_ptr<coordinator::CoordinatorManager> ConflictResolutionManager::GetCoordinator() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinator;
}

bool ConflictResolutionManager::HasCoordinator() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinator != nullptr;
}

uint32_t ConflictResolutionManager::GetAlternateRouteThresholdSeconds() const noexcept {
    return m_alternateRouteThresholdSeconds;
}

void ConflictResolutionManager::SetAlternateRouteThresholdSeconds(uint32_t seconds) noexcept {
    m_alternateRouteThresholdSeconds = seconds;
}

std::string ConflictResolutionManager::GenerateCommandId(const std::string& prefix) {
    ++m_commandSequence;
    std::ostringstream oss;
    oss << prefix << "-" << m_commandSequence;
    return oss.str();
}

ResolutionCommand ConflictResolutionManager::GenerateProceedCommand(
    const std::string& ambulanceId,
    const std::string& intersectionId,
    const std::string& reason) {

    ResolutionCommand cmd;
    cmd.ambulanceId = ambulanceId;
    cmd.intersectionId = intersectionId;
    cmd.action = ResolutionAction::PROCEED;
    cmd.delaySeconds = 0;
    cmd.reason = reason;
    cmd.timestampEpochMs = CurrentTimeMillis();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cmd.commandId = GenerateCommandId("CMD-PROCEED");
        m_commandHistory.push_back(cmd);

        // If coordinator connected, issue green preemption command
        if (m_coordinator) {
            auto greenMsg = m_coordinator->GetTrafficCommandManager().CreateGreenCommand(intersectionId, 30);
            (void)greenMsg;
        }
    }

    return cmd;
}

ResolutionCommand ConflictResolutionManager::GenerateHoldCommand(
    const std::string& ambulanceId,
    const std::string& intersectionId,
    const std::string& reason) {

    ResolutionCommand cmd;
    cmd.ambulanceId = ambulanceId;
    cmd.intersectionId = intersectionId;
    cmd.action = ResolutionAction::HOLD;
    cmd.delaySeconds = 0;
    cmd.reason = reason;
    cmd.timestampEpochMs = CurrentTimeMillis();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cmd.commandId = GenerateCommandId("CMD-HOLD");
        m_commandHistory.push_back(cmd);

        if (m_coordinator) {
            auto redMsg = m_coordinator->GetTrafficCommandManager().CreateRedCommand(intersectionId);
            (void)redMsg;
        }
    }

    return cmd;
}

ResolutionCommand ConflictResolutionManager::GenerateDelayCommand(
    const std::string& ambulanceId,
    const std::string& intersectionId,
    uint32_t delaySec,
    const std::string& reason) {

    ResolutionCommand cmd;
    cmd.ambulanceId = ambulanceId;
    cmd.intersectionId = intersectionId;
    cmd.action = ResolutionAction::DELAY;
    cmd.delaySeconds = delaySec;
    cmd.reason = reason;
    cmd.timestampEpochMs = CurrentTimeMillis();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cmd.commandId = GenerateCommandId("CMD-DELAY");
        m_commandHistory.push_back(cmd);
    }

    return cmd;
}

ResolutionCommand ConflictResolutionManager::GenerateResumeCommand(
    const std::string& ambulanceId,
    const std::string& intersectionId,
    const std::string& reason) {

    ResolutionCommand cmd;
    cmd.ambulanceId = ambulanceId;
    cmd.intersectionId = intersectionId;
    cmd.action = ResolutionAction::RESUME;
    cmd.delaySeconds = 0;
    cmd.reason = reason;
    cmd.timestampEpochMs = CurrentTimeMillis();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cmd.commandId = GenerateCommandId("CMD-RESUME");
        m_commandHistory.push_back(cmd);
    }

    return cmd;
}

ResolutionCommand ConflictResolutionManager::GenerateAlternateRouteCommand(
    const std::string& ambulanceId,
    const std::string& intersectionId,
    const std::vector<std::string>& alternateRoute,
    const std::string& reason) {

    ResolutionCommand cmd;
    cmd.ambulanceId = ambulanceId;
    cmd.intersectionId = intersectionId;
    cmd.action = ResolutionAction::ALTERNATE_ROUTE;
    cmd.alternateRoute = alternateRoute;
    cmd.delaySeconds = 0;
    cmd.reason = reason;
    cmd.timestampEpochMs = CurrentTimeMillis();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cmd.commandId = GenerateCommandId("CMD-ALTROUTE");
        m_commandHistory.push_back(cmd);
    }

    return cmd;
}

std::vector<ResolutionCommand> ConflictResolutionManager::GenerateResolutionCommands(
    const IntelligentScheduleDecision& decision,
    const std::unordered_map<std::string, std::vector<std::string>>& availableAlternateRoutes) {

    std::vector<ResolutionCommand> commands;

    // 1. Winner gets PROCEED
    if (!decision.winnerAmbulanceId.empty()) {
        std::ostringstream proceedReason;
        proceedReason << "Corridor green right-of-way granted: " << decision.decisionReason;
        commands.push_back(GenerateProceedCommand(
            decision.winnerAmbulanceId,
            decision.intersectionId,
            proceedReason.str()
        ));
    }

    // 2. Delayed units get HOLD, DELAY, or ALTERNATE_ROUTE
    for (const auto& delayedId : decision.delayedAmbulanceIds) {
        uint32_t delay = decision.GetAssignedDelay(delayedId);

        // Check if alternate route is recommended due to excessive delay
        auto altIt = availableAlternateRoutes.find(delayedId);
        if (delay >= m_alternateRouteThresholdSeconds && altIt != availableAlternateRoutes.end() && !altIt->second.empty()) {
            std::ostringstream altReason;
            altReason << "Delay (" << delay << "s) exceeds alternate routing threshold ("
                      << m_alternateRouteThresholdSeconds << "s). Diverting via alternate corridor.";
            commands.push_back(GenerateAlternateRouteCommand(
                delayedId,
                decision.intersectionId,
                altIt->second,
                altReason.str()
            ));
        } else if (delay <= 15) {
            // Immediate hold at junction stop line
            std::ostringstream holdReason;
            holdReason << "Hold at stop line to allow passage of " << decision.winnerAmbulanceId
                       << " (" << decision.decisionReason << ")";
            commands.push_back(GenerateHoldCommand(
                delayedId,
                decision.intersectionId,
                holdReason.str()
            ));
        } else {
            // Staggered speed deceleration / delay
            std::ostringstream delayReason;
            delayReason << "Assigned " << delay << "s delay window behind preceding emergency vehicle";
            commands.push_back(GenerateDelayCommand(
                delayedId,
                decision.intersectionId,
                delay,
                delayReason.str()
            ));
        }
    }

    return commands;
}

std::vector<ResolutionCommand> ConflictResolutionManager::GetCommandHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_commandHistory;
}

std::vector<ResolutionCommand> ConflictResolutionManager::GetCommandsForAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ResolutionCommand> result;
    for (const auto& cmd : m_commandHistory) {
        if (cmd.ambulanceId == ambulanceId) {
            result.push_back(cmd);
        }
    }
    return result;
}

std::vector<ResolutionCommand> ConflictResolutionManager::GetCommandsForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ResolutionCommand> result;
    for (const auto& cmd : m_commandHistory) {
        if (cmd.intersectionId == intersectionId) {
            result.push_back(cmd);
        }
    }
    return result;
}

size_t ConflictResolutionManager::GetTotalCommandCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_commandHistory.size();
}

void ConflictResolutionManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_commandHistory.clear();
}

} // namespace scheduler
} // namespace aegis
