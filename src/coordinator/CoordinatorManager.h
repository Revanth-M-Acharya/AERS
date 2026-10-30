/**
 * @file CoordinatorManager.h
 * @brief Core foundation coordinator manager for SMART AEGIS.
 *
 * Coordinates system lifecycle, owns fundamental subsystems (IntersectionRegistry,
 * EventQueue, SocketServer, IntersectionState, GreenCorridor, TrafficCommandManager),
 * ingests messages from the communication module, and drives the corridor decision engine.
 */

#ifndef AEGIS_COORDINATOR_COORDINATOR_MANAGER_H
#define AEGIS_COORDINATOR_COORDINATOR_MANAGER_H

#include "EventQueue.h"
#include "IntersectionRegistry.h"
#include "IntersectionState.h"
#include "GreenCorridor.h"
#include "TrafficCommandManager.h"
#include "communication/SocketServer.h"
#include "communication/Message.h"
#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace aegis {
namespace coordinator {

/**
 * @brief Runtime telemetry and connection status record for a network node.
 */
struct ConnectedNodeInfo {
    std::string nodeId{""};                  ///< Unique node identifier (e.g., "INT_A", "AMB_01")
    std::string role{"Unknown"};             ///< Role (e.g., "Intersection", "Ambulance", "Hospital")
    std::string ipAddress{""};               ///< Remote IPv4 address of the node
    uint16_t port{0};                        ///< Remote port number
    std::string connectionStatus{"OFFLINE"}; ///< Current connection state: "ONLINE" or "OFFLINE"
    uint64_t lastHeartbeatTimestamp{0};      ///< Epoch timestamp (ms) of the most recent heartbeat
    int clientId{-1};                         ///< Connected socket client ID (-1 if disconnected)
    bool everRegistered{false};              ///< Latch indicating node has successfully registered
};

/**
 * @brief Record representing an acknowledgement returned by an intersection for a command (Phase 6B).
 */
struct SignalCommandAck {
    std::string intersectionId{""};
    std::string currentState{"RED"};
    std::string status{"OK"}; // "OK" or "ERROR"
    std::string command{""};
    std::string rawLog{""};
    uint64_t timestampMs{0};
};

/**
 * @brief Central manager governing coordinator foundation subsystems.
 */
class CoordinatorManager {
public:
    using MessageHandler = std::function<void(int clientId, const communication::Message& message)>;
    using EventHook      = std::function<void(const CoordinatorEvent& event)>;

    /**
     * @brief Construct an unstarted CoordinatorManager and instantiate core subsystems.
     */
    CoordinatorManager();

    /**
     * @brief Destructor guarantees clean, orderly shutdown of all owned subsystems.
     */
    virtual ~CoordinatorManager();

    // Non-copyable, movable
    CoordinatorManager(const CoordinatorManager&) = delete;
    CoordinatorManager& operator=(const CoordinatorManager&) = delete;
    CoordinatorManager(CoordinatorManager&&) noexcept;
    CoordinatorManager& operator=(CoordinatorManager&&) noexcept;

    // =========================================================================
    // Lifecycle Management
    // =========================================================================

    /**
     * @brief Start the coordinator network server and background event dispatch worker.
     * @param port TCP port to bind (default: 9000).
     * @param host Local interface IP to bind (default: "0.0.0.0").
     * @return true if network server bound and worker started; false on failure.
     */
    bool Start(uint16_t port = 9000, const std::string& host = "0.0.0.0");

    /**
     * @brief Stop the coordinator, disconnect clients, stop dispatch worker, and release subsystems.
     */
    void Stop();

    /**
     * @brief Check if coordinator is currently executing and accepting messages.
     */
    bool IsRunning() const noexcept;

    // =========================================================================
    // Subsystem Ownership & Accessors
    // =========================================================================

    /**
     * @brief Access the owned IntersectionRegistry instance.
     */
    IntersectionRegistry& GetIntersectionRegistry() noexcept { return m_intersectionRegistry; }
    const IntersectionRegistry& GetIntersectionRegistry() const noexcept { return m_intersectionRegistry; }

    /**
     * @brief Access the owned EventQueue instance.
     */
    EventQueue& GetEventQueue() noexcept { return m_eventQueue; }
    const EventQueue& GetEventQueue() const noexcept { return m_eventQueue; }

    /**
     * @brief Access the owned SocketServer communication instance.
     */
    communication::SocketServer& GetSocketServer() noexcept { return m_socketServer; }
    const communication::SocketServer& GetSocketServer() const noexcept { return m_socketServer; }

    /**
     * @brief Access the owned IntersectionState runtime status instance.
     */
    IntersectionState& GetIntersectionState() noexcept { return m_intersectionState; }
    const IntersectionState& GetIntersectionState() const noexcept { return m_intersectionState; }

    /**
     * @brief Access the owned GreenCorridor instance.
     */
    GreenCorridor& GetGreenCorridor() noexcept { return m_corridor; }
    const GreenCorridor& GetGreenCorridor() const noexcept { return m_corridor; }

    /**
     * @brief Access the owned TrafficCommandManager factory instance.
     */
    TrafficCommandManager& GetTrafficCommandManager() noexcept { return m_trafficCommandManager; }
    const TrafficCommandManager& GetTrafficCommandManager() const noexcept { return m_trafficCommandManager; }

    // =========================================================================
    // Corridor Decision Engine Operations
    // =========================================================================

    /**
     * @brief Initialize a new green corridor route.
     * @param corridorId Unique identifier for the corridor.
     * @param intersectionIds Ordered sequence of intersections on the route.
     * @return true if initialized successfully.
     */
    bool CreateCorridor(const std::string& corridorId, const std::vector<std::string>& intersectionIds);

    /**
     * @brief Activate preemption along the configured corridor.
     *
     * Identifies current active intersection, generates SET_GREEN command,
     * generates SET_RED commands for others on the corridor, and stores them.
     * Commands are stored internally and NOT transmitted automatically.
     *
     * @return true if activated successfully; false otherwise.
     */
    bool ActivateCorridor();

    /**
     * @brief Progress active corridor preemption to the next down-path intersection.
     * @return true if advanced to a valid next intersection; false if end of route or inactive.
     */
    bool AdvanceCorridor();

    /**
     * @brief Complete active corridor and generate SET_NORMAL restoring cyclical phasing.
     * @return true on success; false if corridor is not ACTIVE.
     */
    bool CompleteCorridor();

    /**
     * @brief Abort active corridor and generate SET_NORMAL for all route intersections.
     * @return true on success; false if already concluded.
     */
    bool CancelCorridor();

    // =========================================================================
    // Generated Commands Management
    // =========================================================================

    /**
     * @brief Retrieve snapshot of all generated commands currently buffered.
     */
    std::vector<communication::Message> GetGeneratedCommands() const;

    /**
     * @brief Retrieve and clear all generated commands currently buffered.
     */
    std::vector<communication::Message> PopGeneratedCommands();

    /**
     * @brief Clear all buffered generated commands.
     */
    void ClearGeneratedCommands();

    /**
     * @brief Get count of buffered generated commands.
     */
    size_t GetGeneratedCommandsCount() const;

    // =========================================================================
    // Message Dispatching & Handler Registration
    // =========================================================================

    /**
     * @brief Register a callback for a specific incoming message type.
     * @param type Message type to handle.
     * @param handler User-defined callback invoked when message is dispatched.
     */
    void SetHandler(communication::MessageType type, MessageHandler handler);

    /**
     * @brief Register a general hook invoked on every processed event.
     */
    void SetEventHook(EventHook hook);

    // =========================================================================
    // Outbound Inter-Node Messaging
    // =========================================================================

    /**
     * @brief Send a typed Message packet to an intersection by its registered ID.
     * @param intersectionId Target intersection identifier.
     * @param message Message to serialize and send.
     * @return true if intersection exists, is online, and transmission succeeded.
     */
    bool SendMessageToIntersection(const std::string& intersectionId, const communication::Message& message);

    /**
     * @brief Broadcast a typed Message packet to all currently connected nodes.
     * @param message Message to transmit.
     * @return true if broadcast succeeded.
     */
    bool BroadcastMessage(const communication::Message& message);

    /**
     * @brief Synchronously process pending events from the queue (useful in test harnesses).
     * @param maxEvents Maximum events to process (0 = all pending).
     * @return Number of events successfully dispatched.
     */
    size_t ProcessPendingEvents(size_t maxEvents = 0);

    // =========================================================================
    // Node Monitoring & Diagnostics (Phase 5A)
    // =========================================================================

    /**
     * @brief Retrieve snapshot list of all tracked nodes in the in-memory table.
     * @return Vector containing all connected node telemetry records in deterministic order.
     */
    std::vector<ConnectedNodeInfo> GetConnectedNodes() const;

    /**
     * @brief Retrieve telemetry record for a specific node by ID.
     * @param nodeId Unique identifier of the node.
     * @param outInfo Populated with node telemetry if found.
     * @return true if node exists in table; false otherwise.
     */
    bool GetConnectedNodeInfo(const std::string& nodeId, ConnectedNodeInfo& outInfo) const;

    /**
     * @brief Get formatted string representation of the current network state.
     */
    std::string GetNetworkStatusString() const;

    /**
     * @brief Print current network state to standard output.
     */
    void PrintNetworkStatus() const;

    // =========================================================================
    // Traffic Signal Command & Control Pipeline (Phase 6B)
    // =========================================================================

    /**
     * @brief Issue a traffic signal command to a specific intersection node (Phase 6B).
     *
     * Supported commands:
     * - "SET_RED"
     * - "SET_YELLOW"
     * - "SET_GREEN"
     * - "SET_ALL_RED"
     *
     * Transmits command to intersection, logs [COMMAND] in the specified 5-line format,
     * and returns true if message was sent.
     *
     * @param intersectionId Target intersection identifier (e.g. "INT_A").
     * @param command Command string to issue.
     * @param reason Operational rationale or trigger for the command.
     * @return true if transmitted successfully; false otherwise.
     */
    bool SendTrafficSignalCommand(const std::string& intersectionId,
                                  const std::string& command,
                                  const std::string& reason = "Coordinator Manual Command");

    static std::string FormatCommandLog(const std::string& coordinatorId,
                                        const std::string& destination,
                                        const std::string& command,
                                        const std::string& reason);

    static std::string FormatAckLog(const std::string& intersectionId,
                                    const std::string& currentState);

    std::vector<std::string> GetCommandLogs() const;
    std::string GetLastCommandLog() const;
    void ClearCommandLogs();

    std::vector<SignalCommandAck> GetReceivedAcks() const;
    size_t GetReceivedAcksCount() const;
    std::string GetLastAckLog() const;
    void ClearReceivedAcks();

    bool WaitForAck(const std::string& intersectionId,
                    uint32_t timeoutMs = 2000,
                    SignalCommandAck* outAck = nullptr);

private:
    void DispatchWorkerLoop();
    void DispatchEvent(const CoordinatorEvent& event);
    void HandleRegister(int clientId, const communication::Message& msg);
    void HandleHeartbeat(int clientId, const communication::Message& msg);
    void HandleSignalStatus(int clientId, const communication::Message& msg);
    void HandleAck(int clientId, const communication::Message& msg);

    static std::string DetermineRoleFromNodeId(const std::string& nodeId);
    static std::string NormalizeRole(const std::string& rawRole, const std::string& nodeId);
    static std::string ExtractNodeRole(const std::string& nodeId, const std::string& payload);

    // Owned subsystems
    IntersectionRegistry m_intersectionRegistry{};
    EventQueue m_eventQueue{};
    communication::SocketServer m_socketServer{};
    IntersectionState m_intersectionState{};
    GreenCorridor m_corridor{};
    TrafficCommandManager m_trafficCommandManager{};

    mutable std::mutex m_commandsMutex{};
    std::vector<communication::Message> m_generatedCommands{};

    std::atomic<bool> m_running{false};
    std::thread m_dispatchWorker{};

    mutable std::mutex m_handlersMutex{};
    std::unordered_map<communication::MessageType, MessageHandler> m_customHandlers{};
    EventHook m_eventHook{nullptr};

    // Phase 5A In-Memory Node Table & Diagnostics
    mutable std::mutex m_nodeTableMutex{};
    std::unordered_map<std::string, ConnectedNodeInfo> m_connectedNodes{};
    std::vector<std::string> m_nodeDisplayOrder{};
    std::unordered_map<int, std::string> m_clientIdToNodeId{};

    // Phase 6B Command Logging and ACK Tracking
    mutable std::mutex m_commandLogsMutex{};
    std::vector<std::string> m_commandLogs{};

    mutable std::mutex m_acksMutex{};
    std::condition_variable m_acksCv{};
    std::vector<SignalCommandAck> m_receivedAcks{};
    std::string m_lastAckLog{""};
};

/**
 * @brief Helper function that prints the current network state to standard output.
 * @param coordinator CoordinatorManager instance to inspect.
 */
void PrintNetworkStatus(const CoordinatorManager& coordinator);

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_COORDINATOR_MANAGER_H
