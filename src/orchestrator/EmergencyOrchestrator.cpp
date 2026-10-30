/**
 * @file EmergencyOrchestrator.cpp
 * @brief Implementation of the Autonomous Emergency Orchestration Engine.
 */

#include "orchestrator/EmergencyOrchestrator.h"

namespace aegis {
namespace orchestrator {

EmergencyOrchestrator::EmergencyOrchestrator()
    : m_scheduler(std::make_shared<scheduler::PriorityScheduler>())
    , m_planner(std::make_shared<green_corridor::EmergencyCorridorPlanner>())
    , m_greenWaveCoordinator(std::make_shared<green_wave::GreenWaveCoordinator>())
    , m_coordinatorManager(nullptr)
    , m_state(OrchestratorState::IDLE)
    , m_enableConsoleLog(true)
    , m_simulateDispatchFailure(false) {
    if (m_scheduler) {
        m_scheduler->SetConsoleLogging(false);
    }
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
    m_executionHistory.reserve(128);
}

EmergencyOrchestrator::EmergencyOrchestrator(
    std::shared_ptr<scheduler::PriorityScheduler> scheduler,
    std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner,
    std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator,
    std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager)
    : m_scheduler(scheduler ? scheduler : std::make_shared<scheduler::PriorityScheduler>())
    , m_planner(planner ? planner : std::make_shared<green_corridor::EmergencyCorridorPlanner>())
    , m_greenWaveCoordinator(coordinator ? coordinator : std::make_shared<green_wave::GreenWaveCoordinator>())
    , m_coordinatorManager(coordinatorManager)
    , m_state(OrchestratorState::IDLE)
    , m_enableConsoleLog(true)
    , m_simulateDispatchFailure(false) {
    if (m_scheduler) {
        m_scheduler->SetConsoleLogging(false);
    }
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
    m_executionHistory.reserve(128);
}

uint64_t EmergencyOrchestrator::CurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

bool EmergencyOrchestrator::SubmitEmergency(const EmergencyRequest& request) {
    if (request.requestId.empty() && request.ambulanceId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_scheduler) {
        return false;
    }

    bool submitted = m_scheduler->SubmitEmergencyRequest(request);
    if (submitted) {
        m_state = OrchestratorState::REQUEST_RECEIVED;
    }
    return submitted;
}

ExecutionRecord EmergencyOrchestrator::ProcessNextEmergency() {
    std::lock_guard<std::mutex> lock(m_mutex);

    ExecutionRecord record;
    record.startTimestampMs = CurrentTimeMs();

    // 1. Verify scheduler queue
    if (!m_scheduler || m_scheduler->GetQueueSize() == 0) {
        m_state = OrchestratorState::FAILED_SCHEDULER;
        record.status = ExecutionStatus::FAILED_SCHEDULER;
        record.failureReason = "Empty scheduler queue";
        record.completionTimestampMs = CurrentTimeMs();
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    auto nextOpt = m_scheduler->GetNextEmergency(true);
    if (!nextOpt.has_value()) {
        m_state = OrchestratorState::FAILED_SCHEDULER;
        record.status = ExecutionStatus::FAILED_SCHEDULER;
        record.failureReason = "Failed to retrieve next emergency from scheduler queue";
        record.completionTimestampMs = CurrentTimeMs();
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    EmergencyRequest emergency = *nextOpt;
    record.request = emergency;
    m_activeEmergency = emergency;
    m_state = OrchestratorState::PRIORITY_SELECTED;

    // 2. Generate Corridor Route via EmergencyCorridorPlanner
    if (!m_planner) {
        m_state = OrchestratorState::FAILED_ROUTE;
        record.status = ExecutionStatus::FAILED_ROUTE;
        record.failureReason = "EmergencyCorridorPlanner subsystem uninitialized";
        record.completionTimestampMs = CurrentTimeMs();
        m_activeEmergency = std::nullopt;
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    std::string startIntersection = emergency.currentLocation;
    std::string destinationHospital = emergency.destination;
    std::string routeKey = startIntersection + "->" + destinationHospital;

    EmergencyRoute route;
    auto cacheIt = m_routeCache.find(routeKey);
    if (cacheIt != m_routeCache.end()) {
        route = cacheIt->second;
        route.ambulanceId = emergency.ambulanceId;
    } else {
        route = m_planner->ComputeEmergencyRoute(emergency.ambulanceId,
                                                startIntersection,
                                                destinationHospital);
        if (route.isValid && !route.intersections.empty()) {
            m_routeCache[routeKey] = route;
        }
    }
    record.route = route;

    if (m_simulateRouteFailure || !route.isValid || route.intersections.empty()) {
        m_state = OrchestratorState::FAILED_ROUTE;
        record.status = ExecutionStatus::FAILED_ROUTE;
        record.failureReason = m_simulateRouteFailure
            ? "Route planning simulated failure"
            : ("Route generation failed for corridor: " + startIntersection + " -> " + destinationHospital);
        record.completionTimestampMs = CurrentTimeMs();
        m_activeEmergency = std::nullopt;
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    m_state = OrchestratorState::ROUTE_GENERATED;

    // 3. Generate Green Wave Schedule via GreenWaveCoordinator
    if (!m_greenWaveCoordinator) {
        m_state = OrchestratorState::FAILED_SIGNAL;
        record.status = ExecutionStatus::FAILED_SIGNAL;
        record.failureReason = "GreenWaveCoordinator subsystem uninitialized";
        record.completionTimestampMs = CurrentTimeMs();
        m_activeEmergency = std::nullopt;
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    GreenWavePlan plan;
    bool waveOk = false;
    if (m_simulateGreenWaveFailure) {
        waveOk = false;
    } else {
        auto waveIt = m_waveCache.find(routeKey);
        if (waveIt != m_waveCache.end()) {
            plan = waveIt->second;
            plan.ambulanceId = emergency.ambulanceId;
            plan.routeId = emergency.ambulanceId + "_" + plan.hospitalId;
            waveOk = plan.isValid;
        } else {
            waveOk = m_greenWaveCoordinator->GenerateGreenWaveSchedule(route, plan);
            if (waveOk && plan.isValid) {
                m_waveCache[routeKey] = plan;
            }
        }
    }
    record.greenWavePlan = plan;

    if (m_simulateGreenWaveFailure || !waveOk || !plan.isValid) {
        m_state = OrchestratorState::FAILED_SIGNAL;
        record.status = ExecutionStatus::FAILED_SIGNAL;
        record.failureReason = m_simulateGreenWaveFailure
            ? "Green wave schedule simulated failure"
            : "Green wave schedule generation failed for route";
        record.completionTimestampMs = CurrentTimeMs();
        m_activeEmergency = std::nullopt;
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    m_state = OrchestratorState::GREEN_WAVE_GENERATED;

    // 4. Issue Traffic Signal Commands through CoordinatorManager
    if (m_simulateDispatchFailure) {
        m_state = OrchestratorState::FAILED_SIGNAL;
        record.status = ExecutionStatus::FAILED_SIGNAL;
        record.failureReason = "Coordinator command dispatch simulated failure";
        record.completionTimestampMs = CurrentTimeMs();
        m_activeEmergency = std::nullopt;
        m_executionHistory.push_back(std::move(record));
        return m_executionHistory.back();
    }

    for (const auto& window : plan.windows) {
        bool dispatchOk = true;

        if (m_customDispatcher) {
            std::string cmd = (window.command.rfind("SET_", 0) == 0) ? window.command : ("SET_" + window.command);
            std::string reason = "Emergency Orchestration Preemption: " + emergency.ambulanceId;
            dispatchOk = m_customDispatcher(window.intersectionId, cmd, reason);
        } else if (m_coordinatorManager) {
            std::string cmd = (window.command.rfind("SET_", 0) == 0) ? window.command : ("SET_" + window.command);
            std::string reason = "Emergency Orchestration Preemption: " + emergency.ambulanceId;
            dispatchOk = m_coordinatorManager->SendTrafficSignalCommand(window.intersectionId, cmd, reason);
        } else {
            // Software simulation default success
            dispatchOk = true;
        }

        if (!dispatchOk) {
            m_state = OrchestratorState::FAILED_SIGNAL;
            record.status = ExecutionStatus::FAILED_SIGNAL;
            record.failureReason = "Coordinator command dispatch failed for intersection: " + window.intersectionId;
            record.completionTimestampMs = CurrentTimeMs();
            m_activeEmergency = std::nullopt;
            m_executionHistory.push_back(std::move(record));
            return m_executionHistory.back();
        }
    }

    m_state = OrchestratorState::COMMANDS_DISPATCHED;

    // 5. Complete Execution
    m_state = OrchestratorState::COMPLETED;
    record.status = ExecutionStatus::COMPLETED;
    record.completionTimestampMs = CurrentTimeMs();
    m_activeEmergency = std::nullopt;

    if (m_enableConsoleLog) {
        std::cout << FormatExecutionRecord(record) << std::flush;
    }

    m_executionHistory.push_back(std::move(record));
    return m_executionHistory.back();
}

bool EmergencyOrchestrator::ProcessNextEmergency(ExecutionRecord& outRecord) {
    outRecord = ProcessNextEmergency();
    return outRecord.IsSuccess();
}

std::vector<ExecutionRecord> EmergencyOrchestrator::ProcessAllEmergencies() {
    std::vector<ExecutionRecord> results;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_scheduler) {
            size_t count = m_scheduler->GetQueueSize();
            results.reserve(count);
            m_executionHistory.reserve(m_executionHistory.size() + count);
        }
    }
    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_scheduler || m_scheduler->GetQueueSize() == 0) {
                break;
            }
        }
        results.push_back(ProcessNextEmergency());
    }
    return results;
}

bool EmergencyOrchestrator::CancelEmergency(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeEmergency &&
        (m_activeEmergency->requestId == requestId || m_activeEmergency->ambulanceId == requestId)) {
        m_activeEmergency = std::nullopt;
        m_state = OrchestratorState::IDLE;
    }
    if (m_scheduler) {
        return m_scheduler->RemoveRequest(requestId);
    }
    return false;
}

std::optional<EmergencyRequest> EmergencyOrchestrator::GetActiveEmergency() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeEmergency;
}

std::vector<ExecutionRecord> EmergencyOrchestrator::GetExecutionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionHistory;
}

void EmergencyOrchestrator::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_executionHistory.clear();
    m_routeCache.clear();
    m_waveCache.clear();
}

OrchestratorState EmergencyOrchestrator::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

std::string EmergencyOrchestrator::FormatExecutionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    for (size_t i = 0; i < m_executionHistory.size(); ++i) {
        oss << FormatExecutionRecord(m_executionHistory[i]);
        if (i + 1 < m_executionHistory.size()) {
            oss << "\n";
        }
    }
    return oss.str();
}

std::shared_ptr<scheduler::PriorityScheduler> EmergencyOrchestrator::GetScheduler() const noexcept {
    return m_scheduler;
}

std::shared_ptr<green_corridor::EmergencyCorridorPlanner> EmergencyOrchestrator::GetPlanner() const noexcept {
    return m_planner;
}

std::shared_ptr<green_wave::GreenWaveCoordinator> EmergencyOrchestrator::GetGreenWaveCoordinator() const noexcept {
    return m_greenWaveCoordinator;
}

std::shared_ptr<coordinator::CoordinatorManager> EmergencyOrchestrator::GetCoordinatorManager() const noexcept {
    return m_coordinatorManager;
}

void EmergencyOrchestrator::SetScheduler(std::shared_ptr<scheduler::PriorityScheduler> scheduler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_scheduler = scheduler;
    if (m_scheduler) {
        m_scheduler->SetConsoleLogging(false);
    }
}

void EmergencyOrchestrator::SetPlanner(std::shared_ptr<green_corridor::EmergencyCorridorPlanner> planner) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_planner = planner;
    m_routeCache.clear();
    m_waveCache.clear();
    if (m_planner) {
        m_planner->EnableAutoLogging(false);
    }
}

void EmergencyOrchestrator::SetGreenWaveCoordinator(std::shared_ptr<green_wave::GreenWaveCoordinator> coordinator) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_greenWaveCoordinator = coordinator;
    m_waveCache.clear();
    if (m_greenWaveCoordinator) {
        m_greenWaveCoordinator->EnableAutoLogging(false);
    }
}

void EmergencyOrchestrator::SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorManager = coordinatorManager;
}

void EmergencyOrchestrator::SetConsoleLogging(bool enabled) noexcept {
    m_enableConsoleLog = enabled;
}

bool EmergencyOrchestrator::GetConsoleLogging() const noexcept {
    return m_enableConsoleLog;
}

void EmergencyOrchestrator::SetSimulateRouteFailure(bool fail) noexcept {
    m_simulateRouteFailure = fail;
}

bool EmergencyOrchestrator::GetSimulateRouteFailure() const noexcept {
    return m_simulateRouteFailure;
}

void EmergencyOrchestrator::SetSimulateGreenWaveFailure(bool fail) noexcept {
    m_simulateGreenWaveFailure = fail;
}

bool EmergencyOrchestrator::GetSimulateGreenWaveFailure() const noexcept {
    return m_simulateGreenWaveFailure;
}

void EmergencyOrchestrator::SetSimulateDispatchFailure(bool fail) noexcept {
    m_simulateDispatchFailure = fail;
}

bool EmergencyOrchestrator::GetSimulateDispatchFailure() const noexcept {
    return m_simulateDispatchFailure;
}

void EmergencyOrchestrator::SetCommandDispatcher(CommandDispatcher dispatcher) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_customDispatcher = std::move(dispatcher);
}

} // namespace orchestrator
} // namespace aegis
