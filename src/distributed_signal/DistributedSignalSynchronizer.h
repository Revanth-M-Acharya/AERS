/**
 * @file DistributedSignalSynchronizer.h
 * @brief Distributed Traffic Signal Synchronization Engine for SMART AEGIS (Phase 10A).
 *
 * Implements the distributed execution layer synchronizing Raspberry Pi 4 Intersection
 * Nodes with the Coordinator Node in real time. Ingests Green Wave schedules from
 * GreenWaveCoordinator, generates intersection-specific execution commands, broadcasts
 * them across distributed nodes, monitors execution queues, clocks, heartbeats, and
 * acknowledgements, automatically handles transmission retries and offline node detection,
 * and maintains complete execution history and audit logs.
 */

#ifndef AEGIS_DISTRIBUTED_SIGNAL_SYNCHRONIZER_H
#define AEGIS_DISTRIBUTED_SIGNAL_SYNCHRONIZER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <iostream>

#include "green_wave/GreenWaveCoordinator.h"
#include "coordinator/CoordinatorManager.h"

namespace aegis {
namespace distributed_signal {

// Upstream type aliases
using GreenWavePlan = aegis::green_wave::GreenWavePlan;
using GreenWindow = aegis::green_wave::GreenWindow;

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Runtime execution state of an intersection node.
 */
struct IntersectionExecutionState {
    std::string intersectionId{""};
    std::string currentSignalState{"RED"};
    double greenStartTime{0.0};
    double greenEndTime{0.0};
    uint32_t scheduleVersion{1};
    uint64_t lastHeartbeat{0};
    std::string executionStatus{"IDLE"};
    bool ackReceived{false};
    uint64_t timestamp{0};

    // Operational metadata
    bool isOnline{true};
    int64_t clockOffsetMs{0};
    std::string activeCommandId{""};

    IntersectionExecutionState() = default;
    IntersectionExecutionState(std::string id,
                               std::string state = "RED",
                               double start = 0.0,
                               double end = 0.0,
                               uint32_t version = 1,
                               uint64_t hb = 0,
                               std::string status = "IDLE",
                               bool ack = false,
                               uint64_t ts = 0,
                               bool online = true,
                               int64_t offset = 0,
                               std::string activeCmd = "")
        : intersectionId(std::move(id))
        , currentSignalState(std::move(state))
        , greenStartTime(start)
        , greenEndTime(end)
        , scheduleVersion(version)
        , lastHeartbeat(hb)
        , executionStatus(std::move(status))
        , ackReceived(ack)
        , timestamp(ts)
        , isOnline(online)
        , clockOffsetMs(offset)
        , activeCommandId(std::move(activeCmd)) {}

    bool operator==(const IntersectionExecutionState& other) const noexcept {
        return intersectionId == other.intersectionId &&
               currentSignalState == other.currentSignalState &&
               greenStartTime == other.greenStartTime &&
               greenEndTime == other.greenEndTime &&
               scheduleVersion == other.scheduleVersion &&
               lastHeartbeat == other.lastHeartbeat &&
               executionStatus == other.executionStatus &&
               ackReceived == other.ackReceived &&
               isOnline == other.isOnline;
    }
};

/**
 * @brief Discrete traffic signal execution command targeting a specific intersection.
 */
struct SignalExecutionCommand {
    std::string commandId{""};
    std::string intersectionId{""};
    std::string signalPhase{"GREEN"};
    double executionTime{0.0};
    double duration{0.0};
    std::string priority{"HIGH"};
    uint32_t routeVersion{1};
    uint64_t timestamp{0};

    // Tracking and retry metadata
    std::string ambulanceId{""};
    std::string corridorId{""};
    uint32_t retryCount{0};
    uint32_t maxRetries{3};
    bool acknowledged{false};
    bool executed{false};
    uint64_t dispatchedTimeMs{0};

    SignalExecutionCommand() = default;
    SignalExecutionCommand(std::string cmdId,
                           std::string intId,
                           std::string phase = "GREEN",
                           double execTime = 0.0,
                           double dur = 0.0,
                           std::string prio = "HIGH",
                           uint32_t version = 1,
                           uint64_t ts = 0,
                           std::string ambId = "",
                           std::string corrId = "")
        : commandId(std::move(cmdId))
        , intersectionId(std::move(intId))
        , signalPhase(std::move(phase))
        , executionTime(execTime)
        , duration(dur)
        , priority(std::move(prio))
        , routeVersion(version)
        , timestamp(ts)
        , ambulanceId(std::move(ambId))
        , corridorId(std::move(corrId)) {}

    bool operator==(const SignalExecutionCommand& other) const noexcept {
        return commandId == other.commandId &&
               intersectionId == other.intersectionId &&
               signalPhase == other.signalPhase &&
               executionTime == other.executionTime &&
               duration == other.duration &&
               priority == other.priority &&
               routeVersion == other.routeVersion;
    }
};

/**
 * @brief Outcome and performance measurement of an execution command dispatch.
 */
struct ExecutionResult {
    std::string intersectionId{""};
    bool success{false};
    double actualExecutionTime{0.0};
    double latency{0.0}; // latency in milliseconds
    bool acknowledged{false};
    std::string errorMessage{""};

    // Associated command metadata
    std::string commandId{""};
    uint64_t timestamp{0};

    ExecutionResult() = default;
    ExecutionResult(std::string intId,
                    bool ok,
                    double actualTime = 0.0,
                    double lat = 0.0,
                    bool ack = false,
                    std::string err = "",
                    std::string cmdId = "",
                    uint64_t ts = 0)
        : intersectionId(std::move(intId))
        , success(ok)
        , actualExecutionTime(actualTime)
        , latency(lat)
        , acknowledged(ack)
        , errorMessage(std::move(err))
        , commandId(std::move(cmdId))
        , timestamp(ts) {}

    bool operator==(const ExecutionResult& other) const noexcept {
        return intersectionId == other.intersectionId &&
               success == other.success &&
               actualExecutionTime == other.actualExecutionTime &&
               latency == other.latency &&
               acknowledged == other.acknowledged &&
               errorMessage == other.errorMessage &&
               commandId == other.commandId;
    }
};

// =============================================================================
// Synchronization Engine Class
// =============================================================================

/**
 * @class DistributedSignalSynchronizer
 * @brief Production-grade thread-safe distributed traffic signal synchronization engine.
 */
class DistributedSignalSynchronizer {
public:
    using CustomDispatcher = std::function<bool(const SignalExecutionCommand&)>;

    /**
     * @brief Construct an instance of DistributedSignalSynchronizer.
     * @param coordinatorManager Optional shared pointer to CoordinatorManager for networking.
     */
    explicit DistributedSignalSynchronizer(
        std::shared_ptr<coordinator::CoordinatorManager> coordinatorManager = nullptr);

    ~DistributedSignalSynchronizer() = default;

    // Non-copyable for robust concurrent thread safety
    DistributedSignalSynchronizer(const DistributedSignalSynchronizer&) = delete;
    DistributedSignalSynchronizer& operator=(const DistributedSignalSynchronizer&) = delete;
    DistributedSignalSynchronizer(DistributedSignalSynchronizer&&) noexcept;
    DistributedSignalSynchronizer& operator=(DistributedSignalSynchronizer&&) noexcept;

    // =========================================================================
    // Core Registration & Topology APIs
    // =========================================================================

    /**
     * @brief Register a new intersection node into the synchronizer.
     * @param intersectionId Unique intersection identifier (e.g., "INT_A", "INT_001").
     * @param initialState Initial signal state (default: "RED").
     * @return true if successfully registered or updated.
     */
    bool RegisterIntersection(const std::string& intersectionId,
                              const std::string& initialState = "RED");

    /**
     * @brief Remove an intersection from synchronization tracking.
     * @param intersectionId Intersection identifier.
     * @return true if removed; false if not found.
     */
    bool RemoveIntersection(const std::string& intersectionId);

    /**
     * @brief Check whether an intersection is registered.
     */
    bool HasIntersection(const std::string& intersectionId) const;

    /**
     * @brief Get list of all registered intersection IDs.
     */
    std::vector<std::string> GetRegisteredIntersections() const;

    /**
     * @brief Get total number of registered intersections.
     */
    size_t GetRegisteredIntersectionCount() const;

    // =========================================================================
    // Plan Ingestion & Execution Scheduling
    // =========================================================================

    /**
     * @brief Ingest a Green Wave schedule and split it into intersection execution commands.
     * @param plan GreenWavePlan generated by GreenWaveCoordinator or DynamicRouteReplanner.
     * @param priority Medical priority tag (default: "HIGH").
     * @param routeVersion Version number of the route corridor.
     * @param ambulanceId Ambulance identifier.
     * @return Vector of generated SignalExecutionCommands added to execution queue.
     */
    std::vector<SignalExecutionCommand> SendExecutionPlan(
        const GreenWavePlan& plan,
        const std::string& priority = "HIGH",
        uint32_t routeVersion = 1,
        const std::string& ambulanceId = "");

    /**
     * @brief Broadcast all pending execution commands in the execution queue to intersections.
     * @return Vector of ExecutionResults representing the dispatch outcome.
     */
    std::vector<ExecutionResult> BroadcastExecution();

    /**
     * @brief Broadcast a specific batch of commands to intersection nodes.
     * @param commands List of commands to transmit.
     * @return Vector of ExecutionResults representing the dispatch outcome.
     */
    std::vector<ExecutionResult> BroadcastExecution(
        const std::vector<SignalExecutionCommand>& commands);

    // =========================================================================
    // Acknowledgement & Heartbeat Ingestion
    // =========================================================================

    /**
     * @brief Ingest execution acknowledgement from an intersection node.
     * @param intersectionId Identifier of acknowledging node.
     * @param commandId Identifier of acknowledged command.
     * @param success Whether the signal phase switch succeeded.
     * @param errorMessage Optional error message if execution failed.
     * @return true if acknowledgement was successfully processed.
     */
    bool ReceiveAcknowledgement(const std::string& intersectionId,
                                const std::string& commandId,
                                bool success = true,
                                const std::string& errorMessage = "");

    /**
     * @brief Ingest a heartbeat signal from an intersection node.
     * @param intersectionId Node identifier.
     * @param timestamp Optional epoch timestamp ms (if 0, current system time is used).
     * @return true if heartbeat processed; false if node is unknown.
     */
    bool ReceiveHeartbeat(const std::string& intersectionId, uint64_t timestamp = 0);

    // =========================================================================
    // State Tracking & Health Monitoring
    // =========================================================================

    /**
     * @brief Update the execution state of an intersection node.
     * @param intersectionId Identifier of intersection.
     * @param state Updated IntersectionExecutionState.
     * @return true if updated.
     */
    bool UpdateExecutionState(const std::string& intersectionId,
                              const IntersectionExecutionState& state);

    /**
     * @brief Update execution state using embedded intersectionId.
     */
    bool UpdateExecutionState(const IntersectionExecutionState& state);

    /**
     * @brief Query current execution state of an intersection.
     * @param intersectionId Node identifier.
     * @return IntersectionExecutionState (default/empty if node not found).
     */
    IntersectionExecutionState GetIntersectionStatus(const std::string& intersectionId) const;

    /**
     * @brief Check and detect nodes that have exceeded heartbeat timeout, marking them OFFLINE.
     * @param timeoutMs Maximum allowable silence duration in milliseconds (default: 5000 ms).
     * @return Vector of node IDs marked OFFLINE during this check.
     */
    std::vector<std::string> CheckOfflineNodes(uint64_t timeoutMs = 5000);

    // =========================================================================
    // Retries, Timeouts & Late Execution Detection
    // =========================================================================

    /**
     * @brief Resend commands that failed dispatch or lack acknowledgements up to maxRetries.
     * @param maxRetries Maximum retry attempts allowed per command (default: 3).
     * @return Vector of commands that were retried.
     */
    std::vector<SignalExecutionCommand> RetryFailedCommands(uint32_t maxRetries = 3);

    /**
     * @brief Detect commands whose scheduled execution window has elapsed without acknowledgement.
     * @param currentTimeSeconds Current simulation/system time in seconds.
     * @param toleranceSeconds Grace period in seconds (default: 1.0 s).
     * @return Vector of late commands.
     */
    std::vector<SignalExecutionCommand> DetectLateExecution(
        double currentTimeSeconds,
        double toleranceSeconds = 1.0);

    /**
     * @brief Detect commands dispatched but lacking acknowledgement beyond timeoutMs.
     * @param timeoutMs Timeout in milliseconds (default: 3000 ms).
     * @return Vector of unacknowledged commands.
     */
    std::vector<SignalExecutionCommand> DetectMissingAcknowledgements(uint64_t timeoutMs = 3000);

    // =========================================================================
    // Clock Synchronization
    // =========================================================================

    /**
     * @brief Set calibrated clock offset for an intersection node.
     * @param intersectionId Node identifier.
     * @param offsetMs Clock offset in milliseconds (node_time - coordinator_time).
     */
    void SynchronizeClock(const std::string& intersectionId, int64_t offsetMs);

    /**
     * @brief Compute and set clock offset using coordinator/node timestamps and round trip time.
     * @param intersectionId Node identifier.
     * @param coordinatorSendTimeMs Timestamp when coordinator sent sync probe.
     * @param nodeReceiveTimeMs Node's timestamp upon receiving probe.
     * @param rttMs Round-trip time in milliseconds.
     */
    void SynchronizeClock(const std::string& intersectionId,
                          uint64_t coordinatorSendTimeMs,
                          uint64_t nodeReceiveTimeMs,
                          uint64_t rttMs);

    /**
     * @brief Get calibrated clock offset for an intersection.
     */
    int64_t GetClockOffset(const std::string& intersectionId) const;

    /**
     * @brief Get coordinator time adjusted for node clock.
     */
    uint64_t GetSynchronizedTime(const std::string& intersectionId) const;

    // =========================================================================
    // Execution Queue & History
    // =========================================================================

    /**
     * @brief Retrieve snapshot of the current execution queue.
     */
    std::vector<SignalExecutionCommand> GetExecutionQueue() const;

    /**
     * @brief Retrieve count of commands currently in execution queue.
     */
    size_t GetExecutionQueueSize() const;

    /**
     * @brief Retrieve complete history of execution results.
     */
    std::vector<ExecutionResult> GetExecutionHistory() const;

    /**
     * @brief Clear execution history and results.
     */
    void ClearHistory();

    /**
     * @brief Clear all internal state: registry, queue, and history.
     */
    void ClearAll();

    // =========================================================================
    // Logging & Formatting
    // =========================================================================

    /**
     * @brief Generate structured audit log entry for a signal execution.
     * @param cmd Dispatched execution command.
     * @param res Execution result outcome.
     * @return Formatted multi-line string matching SMART AEGIS specification.
     */
    static std::string FormatExecutionLog(const SignalExecutionCommand& cmd,
                                          const ExecutionResult& res);

    /**
     * @brief Generate structured audit log entry for node status.
     */
    static std::string FormatExecutionLog(const std::string& intersectionId,
                                          const std::string& status,
                                          const std::string& details);

    // =========================================================================
    // Integration & Simulation Configuration
    // =========================================================================

    void SetCoordinatorManager(std::shared_ptr<coordinator::CoordinatorManager> manager);
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinatorManager() const;

    void SetCustomDispatcher(CustomDispatcher dispatcher);

    void SetSimulateFailure(bool fail);
    bool GetSimulateFailure() const;

private:
    uint64_t CurrentTimeMs() const;

    mutable std::mutex m_mutex;

    std::unordered_map<std::string, IntersectionExecutionState> m_intersections;
    std::vector<SignalExecutionCommand> m_executionQueue;
    std::vector<ExecutionResult> m_executionHistory;

    std::shared_ptr<coordinator::CoordinatorManager> m_coordinatorManager{nullptr};
    CustomDispatcher m_customDispatcher{nullptr};

    uint64_t m_commandCounter{0};
    bool m_simulateFailure{false};
};

} // namespace distributed_signal
} // namespace aegis

#endif // AEGIS_DISTRIBUTED_SIGNAL_SYNCHRONIZER_H
