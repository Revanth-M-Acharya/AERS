/**
 * @file DistributedSignalSynchronizer.cpp
 * @brief Implementation of Distributed Traffic Signal Synchronization Engine (Phase 10A).
 */

#include "DistributedSignalSynchronizer.h"

namespace aegis {
namespace distributed_signal {

DistributedSignalSynchronizer::DistributedSignalSynchronizer(
    std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager)
    : m_coordinatorManager(std::move(coordinatorManager)) {}

DistributedSignalSynchronizer::DistributedSignalSynchronizer(
    DistributedSignalSynchronizer&& other) noexcept {
    std::unique_lock<std::mutex> lock(other.m_mutex);
    m_intersections = std::move(other.m_intersections);
    m_executionQueue = std::move(other.m_executionQueue);
    m_executionHistory = std::move(other.m_executionHistory);
    m_coordinatorManager = std::move(other.m_coordinatorManager);
    m_customDispatcher = std::move(other.m_customDispatcher);
    m_commandCounter = other.m_commandCounter;
    m_simulateFailure = other.m_simulateFailure;
}

DistributedSignalSynchronizer& DistributedSignalSynchronizer::operator=(
    DistributedSignalSynchronizer&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_intersections = std::move(other.m_intersections);
        m_executionQueue = std::move(other.m_executionQueue);
        m_executionHistory = std::move(other.m_executionHistory);
        m_coordinatorManager = std::move(other.m_coordinatorManager);
        m_customDispatcher = std::move(other.m_customDispatcher);
        m_commandCounter = other.m_commandCounter;
        m_simulateFailure = other.m_simulateFailure;
    }
    return *this;
}

uint64_t DistributedSignalSynchronizer::CurrentTimeMs() const {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

// =============================================================================
// Core Registration & Topology APIs
// =============================================================================

bool DistributedSignalSynchronizer::RegisterIntersection(
    const std::string& intersectionId,
    const std::string& initialState) {
    if (intersectionId.empty()) {
        return false;
    }

    const uint64_t now = CurrentTimeMs();
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        it->second.currentSignalState = initialState;
        it->second.lastHeartbeat = now;
        it->second.isOnline = true;
        it->second.timestamp = now;
        return true;
    }

    IntersectionExecutionState state;
    state.intersectionId = intersectionId;
    state.currentSignalState = initialState;
    state.greenStartTime = 0.0;
    state.greenEndTime = 0.0;
    state.scheduleVersion = 1;
    state.lastHeartbeat = now;
    state.executionStatus = "IDLE";
    state.ackReceived = false;
    state.timestamp = now;
    state.isOnline = true;
    state.clockOffsetMs = 0;

    m_intersections[intersectionId] = std::move(state);
    return true;
}

bool DistributedSignalSynchronizer::RemoveIntersection(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.erase(intersectionId) > 0;
}

bool DistributedSignalSynchronizer::HasIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.find(intersectionId) != m_intersections.end();
}

std::vector<std::string> DistributedSignalSynchronizer::GetRegisteredIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_intersections.size());
    for (const auto& [id, _] : m_intersections) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

size_t DistributedSignalSynchronizer::GetRegisteredIntersectionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.size();
}

// =============================================================================
// Plan Ingestion & Execution Scheduling
// =============================================================================

std::vector<SignalExecutionCommand> DistributedSignalSynchronizer::SendExecutionPlan(
    const GreenWavePlan& plan,
    const std::string& priority,
    uint32_t routeVersion,
    const std::string& ambulanceId) {
    std::vector<SignalExecutionCommand> generated;
    generated.reserve(plan.windows.size());

    const uint64_t now = CurrentTimeMs();
    const std::string effectiveAmbId = !ambulanceId.empty() ? ambulanceId : plan.ambulanceId;

    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& window : plan.windows) {
        if (window.intersectionId.empty()) {
            continue;
        }

        const uint64_t cmdNum = ++m_commandCounter;
        std::string cmdId = "CMD_" + window.intersectionId + "_" + std::to_string(cmdNum);
        double duration = std::max(0.0, window.endTimeSeconds - window.startTimeSeconds);
        std::string phase = window.command.empty() ? "GREEN" : window.command;

        SignalExecutionCommand cmd(
            cmdId,
            window.intersectionId,
            phase,
            window.startTimeSeconds,
            duration,
            priority,
            routeVersion,
            now,
            effectiveAmbId,
            plan.routeId);

        // Auto-register node if unknown
        auto it = m_intersections.find(window.intersectionId);
        if (it == m_intersections.end()) {
            IntersectionExecutionState state(
                window.intersectionId, "RED", window.startTimeSeconds, window.endTimeSeconds,
                routeVersion, now, "PENDING", false, now, true, 0, cmdId);
            m_intersections[window.intersectionId] = std::move(state);
        } else {
            it->second.executionStatus = "PENDING";
            it->second.greenStartTime = window.startTimeSeconds;
            it->second.greenEndTime = window.endTimeSeconds;
            it->second.scheduleVersion = routeVersion;
            it->second.ackReceived = false;
            it->second.timestamp = now;
            it->second.activeCommandId = cmdId;
        }

        m_executionQueue.push_back(cmd);
        generated.push_back(std::move(cmd));
    }

    return generated;
}

std::vector<ExecutionResult> DistributedSignalSynchronizer::BroadcastExecution() {
    std::vector<ExecutionResult> results;
    const uint64_t now = CurrentTimeMs();

    std::lock_guard<std::mutex> lock(m_mutex);
    results.reserve(m_executionQueue.size());

    for (auto& cmd : m_executionQueue) {
        if (cmd.executed) {
            continue;
        }

        bool success = false;
        std::string errorMsg = "";

        if (m_simulateFailure) {
            success = false;
            errorMsg = "Simulated transmission failure";
        } else if (m_customDispatcher) {
            success = m_customDispatcher(cmd);
            if (!success) {
                errorMsg = "Custom dispatcher dispatch rejected";
            }
        } else if (m_coordinatorManager) {
            std::string signalCmd = (cmd.signalPhase.rfind("SET_", 0) == 0)
                                        ? cmd.signalPhase
                                        : ("SET_" + cmd.signalPhase);
            std::string reason = "GreenWave Distributed Sync: " + cmd.commandId;
            success = m_coordinatorManager->SendTrafficSignalCommand(
                cmd.intersectionId, signalCmd, reason);
            if (!success) {
                errorMsg = "CoordinatorManager dispatch failed";
            }
        } else {
            // Software simulation default success
            success = true;
        }

        cmd.dispatchedTimeMs = now;
        cmd.executed = success;

        auto it = m_intersections.find(cmd.intersectionId);
        if (it != m_intersections.end()) {
            if (success) {
                it->second.executionStatus = "EXECUTING";
                it->second.currentSignalState = cmd.signalPhase;
            } else {
                it->second.executionStatus = "FAILED";
            }
            it->second.timestamp = now;
        }

        ExecutionResult res(
            cmd.intersectionId,
            success,
            cmd.executionTime,
            0.0,
            false,
            errorMsg,
            cmd.commandId,
            now);

        m_executionHistory.push_back(res);
        results.push_back(std::move(res));
    }

    return results;
}

std::vector<ExecutionResult> DistributedSignalSynchronizer::BroadcastExecution(
    const std::vector<SignalExecutionCommand>& commands) {
    std::vector<ExecutionResult> results;
    const uint64_t now = CurrentTimeMs();

    std::lock_guard<std::mutex> lock(m_mutex);
    results.reserve(commands.size());

    for (const auto& inCmd : commands) {
        SignalExecutionCommand cmd = inCmd;
        bool success = false;
        std::string errorMsg = "";

        if (m_simulateFailure) {
            success = false;
            errorMsg = "Simulated transmission failure";
        } else if (m_customDispatcher) {
            success = m_customDispatcher(cmd);
            if (!success) {
                errorMsg = "Custom dispatcher dispatch rejected";
            }
        } else if (m_coordinatorManager) {
            std::string signalCmd = (cmd.signalPhase.rfind("SET_", 0) == 0)
                                        ? cmd.signalPhase
                                        : ("SET_" + cmd.signalPhase);
            std::string reason = "GreenWave Distributed Sync: " + cmd.commandId;
            success = m_coordinatorManager->SendTrafficSignalCommand(
                cmd.intersectionId, signalCmd, reason);
            if (!success) {
                errorMsg = "CoordinatorManager dispatch failed";
            }
        } else {
            success = true;
        }

        cmd.dispatchedTimeMs = now;
        cmd.executed = success;

        auto it = m_intersections.find(cmd.intersectionId);
        if (it != m_intersections.end()) {
            if (success) {
                it->second.executionStatus = "EXECUTING";
                it->second.currentSignalState = cmd.signalPhase;
            } else {
                it->second.executionStatus = "FAILED";
            }
            it->second.timestamp = now;
        }

        // Add or update queue entry
        auto qIt = std::find_if(m_executionQueue.begin(), m_executionQueue.end(),
            [&](const SignalExecutionCommand& c) { return c.commandId == cmd.commandId; });
        if (qIt != m_executionQueue.end()) {
            *qIt = cmd;
        } else {
            m_executionQueue.push_back(cmd);
        }

        ExecutionResult res(
            cmd.intersectionId,
            success,
            cmd.executionTime,
            0.0,
            false,
            errorMsg,
            cmd.commandId,
            now);

        m_executionHistory.push_back(res);
        results.push_back(std::move(res));
    }

    return results;
}

// =============================================================================
// Acknowledgement & Heartbeat Ingestion
// =============================================================================

bool DistributedSignalSynchronizer::ReceiveAcknowledgement(
    const std::string& intersectionId,
    const std::string& commandId,
    bool success,
    const std::string& errorMessage) {
    const uint64_t now = CurrentTimeMs();
    std::lock_guard<std::mutex> lock(m_mutex);

    double actualExecutionTime = 0.0;
    double latency = 1.0;

    // Locate command in queue
    auto cmdIt = std::find_if(m_executionQueue.begin(), m_executionQueue.end(),
        [&](const SignalExecutionCommand& c) {
            return (!commandId.empty() && c.commandId == commandId) ||
                   (commandId.empty() && c.intersectionId == intersectionId);
        });

    if (cmdIt != m_executionQueue.end()) {
        cmdIt->acknowledged = success;
        actualExecutionTime = cmdIt->executionTime;
        if (cmdIt->dispatchedTimeMs > 0 && now >= cmdIt->dispatchedTimeMs) {
            latency = static_cast<double>(now - cmdIt->dispatchedTimeMs);
        }
    }

    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        it->second.ackReceived = success;
        it->second.timestamp = now;
        it->second.executionStatus = success ? "COMPLETED" : "FAILED";
    }

    ExecutionResult res(
        intersectionId,
        success,
        actualExecutionTime,
        latency,
        success,
        errorMessage,
        commandId,
        now);

    m_executionHistory.push_back(res);
    return true;
}

bool DistributedSignalSynchronizer::ReceiveHeartbeat(
    const std::string& intersectionId,
    uint64_t timestamp) {
    if (intersectionId.empty()) {
        return false;
    }

    const uint64_t effectiveTimestamp = (timestamp == 0) ? CurrentTimeMs() : timestamp;
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_intersections.find(intersectionId);
    if (it == m_intersections.end()) {
        IntersectionExecutionState state;
        state.intersectionId = intersectionId;
        state.currentSignalState = "RED";
        state.greenStartTime = 0.0;
        state.greenEndTime = 0.0;
        state.scheduleVersion = 1;
        state.lastHeartbeat = effectiveTimestamp;
        state.executionStatus = "IDLE";
        state.ackReceived = false;
        state.timestamp = effectiveTimestamp;
        state.isOnline = true;
        m_intersections[intersectionId] = std::move(state);
    } else {
        it->second.lastHeartbeat = effectiveTimestamp;
        it->second.isOnline = true;
        it->second.timestamp = effectiveTimestamp;
        if (it->second.executionStatus == "OFFLINE") {
            it->second.executionStatus = "IDLE";
        }
    }

    return true;
}

// =============================================================================
// State Tracking & Health Monitoring
// =============================================================================

bool DistributedSignalSynchronizer::UpdateExecutionState(
    const std::string& intersectionId,
    const IntersectionExecutionState& state) {
    if (intersectionId.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections[intersectionId] = state;
    m_intersections[intersectionId].intersectionId = intersectionId;
    return true;
}

bool DistributedSignalSynchronizer::UpdateExecutionState(
    const IntersectionExecutionState& state) {
    return UpdateExecutionState(state.intersectionId, state);
}

IntersectionExecutionState DistributedSignalSynchronizer::GetIntersectionStatus(
    const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        return it->second;
    }
    return IntersectionExecutionState{intersectionId};
}

std::vector<std::string> DistributedSignalSynchronizer::CheckOfflineNodes(uint64_t timeoutMs) {
    const uint64_t now = CurrentTimeMs();
    std::vector<std::string> offlineNodes;

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, state] : m_intersections) {
        if (now > state.lastHeartbeat && (now - state.lastHeartbeat) > timeoutMs) {
            state.isOnline = false;
            state.executionStatus = "OFFLINE";
            offlineNodes.push_back(id);
        }
    }
    std::sort(offlineNodes.begin(), offlineNodes.end());
    return offlineNodes;
}

// =============================================================================
// Retries, Timeouts & Late Execution Detection
// =============================================================================

std::vector<SignalExecutionCommand> DistributedSignalSynchronizer::RetryFailedCommands(
    uint32_t maxRetries) {
    std::vector<SignalExecutionCommand> retried;
    const uint64_t now = CurrentTimeMs();

    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& cmd : m_executionQueue) {
        if ((!cmd.acknowledged || !cmd.executed) && cmd.retryCount < maxRetries) {
            cmd.retryCount++;
            cmd.dispatchedTimeMs = now;

            bool success = false;
            std::string errorMsg = "";

            if (m_simulateFailure) {
                success = false;
                errorMsg = "Simulated retry transmission failure";
            } else if (m_customDispatcher) {
                success = m_customDispatcher(cmd);
                if (!success) {
                    errorMsg = "Custom dispatcher retry rejected";
                }
            } else if (m_coordinatorManager) {
                std::string signalCmd = (cmd.signalPhase.rfind("SET_", 0) == 0)
                                            ? cmd.signalPhase
                                            : ("SET_" + cmd.signalPhase);
                std::string reason = "GreenWave Retry: " + cmd.commandId;
                success = m_coordinatorManager->SendTrafficSignalCommand(
                    cmd.intersectionId, signalCmd, reason);
                if (!success) {
                    errorMsg = "CoordinatorManager retry dispatch failed";
                }
            } else {
                success = true;
            }

            cmd.executed = success;

            auto it = m_intersections.find(cmd.intersectionId);
            if (it != m_intersections.end()) {
                it->second.executionStatus = success ? "EXECUTING" : "FAILED";
                it->second.timestamp = now;
            }

            ExecutionResult res(
                cmd.intersectionId,
                success,
                cmd.executionTime,
                0.0,
                false,
                errorMsg,
                cmd.commandId,
                now);

            m_executionHistory.push_back(res);
            retried.push_back(cmd);
        }
    }

    return retried;
}

std::vector<SignalExecutionCommand> DistributedSignalSynchronizer::DetectLateExecution(
    double currentTimeSeconds,
    double toleranceSeconds) {
    std::vector<SignalExecutionCommand> late;
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& cmd : m_executionQueue) {
        if (!cmd.acknowledged) {
            double windowEnd = cmd.executionTime + cmd.duration;
            if (currentTimeSeconds > (windowEnd + toleranceSeconds)) {
                late.push_back(cmd);
            }
        }
    }

    return late;
}

std::vector<SignalExecutionCommand> DistributedSignalSynchronizer::DetectMissingAcknowledgements(
    uint64_t timeoutMs) {
    std::vector<SignalExecutionCommand> missing;
    const uint64_t now = CurrentTimeMs();

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& cmd : m_executionQueue) {
        if (cmd.executed && !cmd.acknowledged) {
            if (now > cmd.dispatchedTimeMs && (now - cmd.dispatchedTimeMs) > timeoutMs) {
                missing.push_back(cmd);
            }
        }
    }

    return missing;
}

// =============================================================================
// Clock Synchronization
// =============================================================================

void DistributedSignalSynchronizer::SynchronizeClock(
    const std::string& intersectionId,
    int64_t offsetMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        it->second.clockOffsetMs = offsetMs;
    }
}

void DistributedSignalSynchronizer::SynchronizeClock(
    const std::string& intersectionId,
    uint64_t coordinatorSendTimeMs,
    uint64_t nodeReceiveTimeMs,
    uint64_t rttMs) {
    int64_t offset = static_cast<int64_t>(nodeReceiveTimeMs) -
                     static_cast<int64_t>(coordinatorSendTimeMs + (rttMs / 2));
    SynchronizeClock(intersectionId, offset);
}

int64_t DistributedSignalSynchronizer::GetClockOffset(
    const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        return it->second.clockOffsetMs;
    }
    return 0;
}

uint64_t DistributedSignalSynchronizer::GetSynchronizedTime(
    const std::string& intersectionId) const {
    const uint64_t now = CurrentTimeMs();
    int64_t offset = GetClockOffset(intersectionId);
    int64_t syncTime = static_cast<int64_t>(now) + offset;
    return syncTime > 0 ? static_cast<uint64_t>(syncTime) : now;
}

// =============================================================================
// Execution Queue & History
// =============================================================================

std::vector<SignalExecutionCommand> DistributedSignalSynchronizer::GetExecutionQueue() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionQueue;
}

size_t DistributedSignalSynchronizer::GetExecutionQueueSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionQueue.size();
}

std::vector<ExecutionResult> DistributedSignalSynchronizer::GetExecutionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_executionHistory;
}

void DistributedSignalSynchronizer::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_executionHistory.clear();
}

void DistributedSignalSynchronizer::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections.clear();
    m_executionQueue.clear();
    m_executionHistory.clear();
    m_commandCounter = 0;
}

// =============================================================================
// Logging & Formatting
// =============================================================================

std::string DistributedSignalSynchronizer::FormatExecutionLog(
    const SignalExecutionCommand& cmd,
    const ExecutionResult& res) {
    std::ostringstream oss;
    oss << "[DISTRIBUTED SIGNAL]\n";
    oss << "Command: " << cmd.commandId << "\n";
    oss << "Intersection: " << cmd.intersectionId << "\n";
    oss << "Phase: " << cmd.signalPhase << "\n";
    oss << std::fixed << std::setprecision(2);
    oss << "Execution: " << cmd.executionTime << "s\n";
    oss << "Duration: " << cmd.duration << "s\n";
    oss << "Status: " << (res.success ? (res.acknowledged ? "SUCCESS / ACKNOWLEDGED" : "SUCCESS") : "FAILED") << "\n";
    oss << "Latency: " << res.latency << " ms";
    return oss.str();
}

std::string DistributedSignalSynchronizer::FormatExecutionLog(
    const std::string& intersectionId,
    const std::string& status,
    const std::string& details) {
    std::ostringstream oss;
    oss << "[DISTRIBUTED SIGNAL]\n";
    oss << "Intersection: " << intersectionId << "\n";
    oss << "Status: " << status << "\n";
    if (!details.empty()) {
        oss << "Details: " << details << "\n";
    }
    return oss.str();
}

// =============================================================================
// Integration & Simulation Configuration
// =============================================================================

void DistributedSignalSynchronizer::SetCoordinatorManager(
    std::shared_ptr<coordinator::CoordinatorManager> manager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorManager = std::move(manager);
}

std::shared_ptr<coordinator::CoordinatorManager>
DistributedSignalSynchronizer::GetCoordinatorManager() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_coordinatorManager;
}

void DistributedSignalSynchronizer::SetCustomDispatcher(CustomDispatcher dispatcher) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_customDispatcher = std::move(dispatcher);
}

void DistributedSignalSynchronizer::SetSimulateFailure(bool fail) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_simulateFailure = fail;
}

bool DistributedSignalSynchronizer::GetSimulateFailure() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_simulateFailure;
}

} // namespace distributed_signal
} // namespace aegis
