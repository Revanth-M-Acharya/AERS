/**
 * @file EmergencyOrchestrator.h
 * @brief Autonomous Emergency Orchestration Engine connecting PriorityScheduler,
 *        EmergencyCorridorPlanner, GreenWaveCoordinator, and CoordinatorManager.
 */

#ifndef AEGIS_ORCHESTRATOR_EMERGENCY_ORCHESTRATOR_H
#define AEGIS_ORCHESTRATOR_EMERGENCY_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <functional>
#include <chrono>
#include <iostream>
#include <sstream>

#include "scheduler/PriorityScheduler.h"
#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "coordinator/CoordinatorManager.h"

namespace aegis {
namespace orchestrator {

// Type aliases from upstream Phase modules
using EmergencyRequest = aegis::scheduler::EmergencyRequest;
using PatientSeverity = aegis::scheduler::PatientSeverity;
using EmergencyRoute = aegis::green_corridor::EmergencyRoute;
using GreenWavePlan = aegis::green_wave::GreenWavePlan;
using GreenWindow = aegis::green_wave::GreenWindow;

/**
 * @brief Orchestrator deterministic execution states and failure transitions.
 */
enum class OrchestratorState {
    IDLE,
    REQUEST_RECEIVED,
    PRIORITY_SELECTED,
    ROUTE_GENERATED,
    GREEN_WAVE_GENERATED,
    COMMANDS_DISPATCHED,
    COMPLETED,
    FAILED_ROUTE,
    FAILED_SCHEDULER,
    FAILED_SIGNAL
};

using ExecutionStatus = OrchestratorState;

inline std::string OrchestratorStateToString(OrchestratorState state) {
    switch (state) {
        case OrchestratorState::IDLE:                 return "IDLE";
        case OrchestratorState::REQUEST_RECEIVED:     return "REQUEST_RECEIVED";
        case OrchestratorState::PRIORITY_SELECTED:    return "PRIORITY_SELECTED";
        case OrchestratorState::ROUTE_GENERATED:       return "ROUTE_GENERATED";
        case OrchestratorState::GREEN_WAVE_GENERATED: return "GREEN_WAVE_GENERATED";
        case OrchestratorState::COMMANDS_DISPATCHED:  return "COMMANDS_DISPATCHED";
        case OrchestratorState::COMPLETED:            return "COMPLETED";
        case OrchestratorState::FAILED_ROUTE:         return "FAILED_ROUTE";
        case OrchestratorState::FAILED_SCHEDULER:     return "FAILED_SCHEDULER";
        case OrchestratorState::FAILED_SIGNAL:        return "FAILED_SIGNAL";
        default:                                      return "UNKNOWN";
    }
}

inline std::string ExecutionStatusToString(ExecutionStatus status) {
    return OrchestratorStateToString(status);
}

/**
 * @brief Execution record for an orchestrated emergency corridor pipeline.
 */
struct ExecutionRecord {
    EmergencyRequest request{};
    EmergencyRoute route{};
    GreenWavePlan greenWavePlan{};
    ExecutionStatus status{ExecutionStatus::IDLE};
    uint64_t startTimestampMs{0};
    uint64_t completionTimestampMs{0};
    std::string failureReason{""};

    bool IsSuccess() const noexcept {
        return status == ExecutionStatus::COMPLETED;
    }

    uint64_t GetDurationMs() const noexcept {
        if (completionTimestampMs >= startTimestampMs) {
            return completionTimestampMs - startTimestampMs;
        }
        return 0;
    }
};

/**
 * @brief Formats an EmergencyRoute into an arrow-separated string (e.g. "INT_A -> INT_B -> INT_C").
 */
inline std::string FormatRouteString(const EmergencyRoute& route) {
    if (!route.isValid || route.intersections.empty()) {
        return "NONE";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < route.intersections.size(); ++i) {
        oss << route.intersections[i];
        if (i + 1 < route.intersections.size()) {
            oss << " -> ";
        }
    }
    return oss.str();
}

/**
 * @brief Formats an ExecutionRecord matching the exact Phase 8B log specification.
 */
inline std::string FormatExecutionRecord(const ExecutionRecord& record) {
    std::ostringstream oss;
    oss << "[ORCHESTRATOR]\n"
        << "Emergency:\n"
        << (record.request.ambulanceId.empty() ? "NONE" : record.request.ambulanceId) << "\n"
        << "Priority:\n"
        << aegis::scheduler::PatientSeverityToString(record.request.GetSeverity()) << "\n"
        << "Route:\n";

    if (record.status == ExecutionStatus::FAILED_ROUTE) {
        oss << "FAILED\n";
    } else {
        oss << FormatRouteString(record.route) << "\n";
    }

    oss << "Green Wave:\n";
    if (record.status == ExecutionStatus::FAILED_ROUTE || record.status == ExecutionStatus::FAILED_SCHEDULER) {
        oss << "NOT_GENERATED\n";
    } else if (record.status == ExecutionStatus::FAILED_SIGNAL && !record.greenWavePlan.isValid) {
        oss << "FAILED\n";
    } else {
        oss << "GENERATED\n";
    }

    oss << "Traffic Commands:\n";
    if (record.status == ExecutionStatus::COMPLETED || record.status == ExecutionStatus::COMMANDS_DISPATCHED) {
        oss << "DISPATCHED\n";
    } else if (record.status == ExecutionStatus::FAILED_SIGNAL && record.greenWavePlan.isValid) {
        oss << "FAILED\n";
    } else {
        oss << "NOT_DISPATCHED\n";
    }

    oss << "Status:\n"
        << OrchestratorStateToString(record.status) << "\n";

    return oss.str();
}

/**
 * @class EmergencyOrchestrator
 * @brief Autonomous Emergency Orchestration Engine connecting all emergency response subsystems.
 */
class EmergencyOrchestrator {
public:
    using CommandDispatcher = std::function<bool(const std::string& intersectionId,
                                                  const std::string& command,
                                                  const std::string& reason)>;

    EmergencyOrchestrator();
    EmergencyOrchestrator(std::shared_ptr<scheduler::PriorityScheduler> scheduler,
                          std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner,
                          std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator,
                          std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager = nullptr);
    ~EmergencyOrchestrator() = default;

    // Non-copyable for concurrency safety
    EmergencyOrchestrator(const EmergencyOrchestrator&) = delete;
    EmergencyOrchestrator& operator=(const EmergencyOrchestrator&) = delete;

    // =========================================================================
    // Public Orchestration APIs
    // =========================================================================

    /**
     * @brief Submits a new emergency request to the orchestration pipeline.
     */
    bool SubmitEmergency(const EmergencyRequest& request);

    /**
     * @brief Processes the next highest priority emergency through the full pipeline.
     */
    ExecutionRecord ProcessNextEmergency();

    /**
     * @brief Processes the next highest priority emergency with out-parameter.
     */
    bool ProcessNextEmergency(ExecutionRecord& outRecord);

    /**
     * @brief Processes all pending emergencies in the scheduler until queue is empty.
     */
    std::vector<ExecutionRecord> ProcessAllEmergencies();

    /**
     * @brief Cancels an emergency request by Request ID or Ambulance ID.
     */
    bool CancelEmergency(const std::string& requestId);

    /**
     * @brief Retrieves the currently active emergency being processed, if any.
     */
    std::optional<EmergencyRequest> GetActiveEmergency() const;

    /**
     * @brief Returns complete history of all executed pipeline runs.
     */
    std::vector<ExecutionRecord> GetExecutionHistory() const;

    /**
     * @brief Clears the execution history records.
     */
    void ClearHistory();

    /**
     * @brief Returns current state machine operational state.
     */
    OrchestratorState GetCurrentState() const;

    /**
     * @brief Formats the entire execution history into the specified log representation.
     */
    std::string FormatExecutionHistory() const;

    // =========================================================================
    // Subsystem Management & Configuration
    // =========================================================================

    std::shared_ptr<scheduler::PriorityScheduler> GetScheduler() const noexcept;
    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> GetPlanner() const noexcept;
    std::shared_ptr<green_wave::GreenWaveCoordinator> GetGreenWaveCoordinator() const noexcept;
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinatorManager() const noexcept;

    void SetScheduler(std::shared_ptr<scheduler::PriorityScheduler> scheduler);
    void SetPlanner(std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner);
    void SetGreenWaveCoordinator(std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator);
    void SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager);

    void SetSimulateRouteFailure(bool fail) noexcept;
    bool GetSimulateRouteFailure() const noexcept;

    void SetSimulateGreenWaveFailure(bool fail) noexcept;
    bool GetSimulateGreenWaveFailure() const noexcept;

    void SetSimulateDispatchFailure(bool fail) noexcept;
    bool GetSimulateDispatchFailure() const noexcept;

    void SetCommandDispatcher(CommandDispatcher dispatcher);

    void SetConsoleLogging(bool enabled) noexcept;
    bool GetConsoleLogging() const noexcept;

private:
    static uint64_t CurrentTimeMs();

    mutable std::mutex m_mutex;
    std::shared_ptr<scheduler::PriorityScheduler> m_scheduler;
    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> m_planner;
    std::shared_ptr<green_wave::GreenWaveCoordinator> m_greenWaveCoordinator;
    std::shared_ptr<coordinator::CoordinatorManager> m_coordinatorManager;

    OrchestratorState m_state{OrchestratorState::IDLE};
    std::optional<EmergencyRequest> m_activeEmergency{std::nullopt};
    std::vector<ExecutionRecord> m_executionHistory{};

    bool m_enableConsoleLog{true};
    bool m_simulateRouteFailure{false};
    bool m_simulateGreenWaveFailure{false};
    bool m_simulateDispatchFailure{false};
    CommandDispatcher m_customDispatcher{nullptr};
    std::unordered_map<std::string, EmergencyRoute> m_routeCache{};
    std::unordered_map<std::string, GreenWavePlan> m_waveCache{};
};

} // namespace orchestrator
} // namespace aegis

#endif // AEGIS_ORCHESTRATOR_EMERGENCY_ORCHESTRATOR_H
