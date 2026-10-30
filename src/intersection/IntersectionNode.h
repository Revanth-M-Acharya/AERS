/**
 * @file IntersectionNode.h
 * @brief Roadside intersection Raspberry Pi edge node controller.
 *
 * Owns the TCP client, autonomous heartbeat manager, and software signal
 * controller. Manages coordinator registration, keepalive beacons, command
 * execution, and automatic failover reconnection.
 */

#ifndef AEGIS_INTERSECTION_INTERSECTION_NODE_H
#define AEGIS_INTERSECTION_INTERSECTION_NODE_H

#include "SignalController.h"
#include "HeartbeatManager.h"
#include "traffic_signal/TrafficSignalNode.h"
#include "communication/SocketClient.h"
#include "communication/Message.h"
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>

namespace aegis {
namespace intersection {

/**
 * @brief Represents one roadside intersection node (Raspberry Pi 2/3/4).
 */
class IntersectionNode {
public:
    using CommandCallback = std::function<void(const std::string& command)>;

    /**
     * @brief Construct an IntersectionNode with a unique intersection ID.
     * @param intersectionId Identifying string (e.g., "INT_A", "INT_B", "INT_C").
     * @param heartbeatIntervalMs Heartbeat beacon period in milliseconds (default: 1000).
     */
    explicit IntersectionNode(std::string intersectionId, uint32_t heartbeatIntervalMs = 1000);

    /**
     * @brief Destructor guarantees disconnection and clean shutdown of owned workers.
     */
    virtual ~IntersectionNode();

    // Non-copyable, movable
    IntersectionNode(const IntersectionNode&) = delete;
    IntersectionNode& operator=(const IntersectionNode&) = delete;
    IntersectionNode(IntersectionNode&&) noexcept;
    IntersectionNode& operator=(IntersectionNode&&) noexcept;

    // =========================================================================
    // Node Lifecycle & Network Operations
    // =========================================================================

    /**
     * @brief Start node: connects to coordinator, registers, and initiates heartbeat upon ACK.
     * @param coordinatorIp IP address of the Central Coordinator (Raspberry Pi 1).
     * @param coordinatorPort TCP port of Coordinator (e.g., 9000).
     * @param connectTimeoutMs Connection establishment timeout in ms.
     * @param ackTimeoutMs Timeout waiting for registration ACK.
     * @return true if connected and registered; false on failure.
     */
    bool Start(const std::string& coordinatorIp,
               uint16_t coordinatorPort,
               uint32_t connectTimeoutMs = 3000,
               uint32_t ackTimeoutMs = 3000);

    /**
     * @brief Stop node: stops heartbeat, disconnects from coordinator, and releases resources.
     */
    void Stop();

    /**
     * @brief Disconnect client socket and halt heartbeat beaconing.
     */
    void Disconnect();

    /**
     * @brief Connect TCP client to the Central Coordinator.
     * @param coordinatorIp Coordinator IP.
     * @param coordinatorPort Coordinator port.
     * @param timeoutMs Timeout in milliseconds.
     * @return true if TCP handshake succeeded; false otherwise.
     */
    bool ConnectToCoordinator(const std::string& coordinatorIp,
                              uint16_t coordinatorPort,
                              uint32_t timeoutMs = 3000);

    /**
     * @brief Transmit REGISTER message to Coordinator and block until ACK is received.
     * @param ackTimeoutMs Maximum duration to await registration ACK.
     * @return true if ACK received within timeout; false otherwise.
     */
    bool RegisterToCoordinator(uint32_t ackTimeoutMs = 3000);

    /**
     * @brief Force explicit reconnection to the configured Coordinator endpoint.
     * @param timeoutMs Connection timeout in milliseconds.
     * @return true if reconnected and re-registered; false otherwise.
     */
    bool Reconnect(uint32_t timeoutMs = 3000);

    // =========================================================================
    // Inbound Message & Command Processing
    // =========================================================================

    /**
     * @brief Ingest and process an incoming message received over the TCP socket.
     * @param message Inbound communication::Message packet.
     */
    void HandleIncomingMessage(const communication::Message& message);

    /**
     * @brief Process and apply a coordinator control command.
     *
     * Supported Commands:
     * - SET_RED
     * - SET_YELLOW
     * - SET_GREEN
     * - SET_ALL_RED
     * - SET_NORMAL, EMERGENCY_MODE_ON, EMERGENCY_MODE_OFF
     *
     * @param commandMessage Message containing the command payload.
     * @return true if command recognized and executed; false otherwise.
     */
    bool ProcessCoordinatorCommand(const communication::Message& commandMessage);

    /**
     * @brief Process and apply a coordinator control command string directly.
     * @param commandName Command name (e.g. "SET_GREEN", "SET_RED").
     * @param reason Optional operational rationale for the command.
     * @return true if executed; false if invalid or rejected.
     */
    bool ProcessCoordinatorCommand(const std::string& commandName, const std::string& reason = "");

    // =========================================================================
    // Inspection & Subsystem Accessors
    // =========================================================================

    const std::string& GetIntersectionId() const noexcept { return m_intersectionId; }
    std::string GetCoordinatorIp() const;
    uint16_t GetCoordinatorPort() const noexcept;

    bool IsConnected() const noexcept;
    bool IsRegistered() const noexcept;
    bool IsRunning() const noexcept;

    communication::SocketClient& GetSocketClient() noexcept { return m_socketClient; }
    const communication::SocketClient& GetSocketClient() const noexcept { return m_socketClient; }

    HeartbeatManager& GetHeartbeatManager() noexcept { return m_heartbeatManager; }
    const HeartbeatManager& GetHeartbeatManager() const noexcept { return m_heartbeatManager; }

    SignalController& GetSignalController() noexcept { return m_signalController; }
    const SignalController& GetSignalController() const noexcept { return m_signalController; }

    traffic_signal::TrafficSignalNode& GetTrafficSignalNode() noexcept { return m_trafficSignalNode; }
    const traffic_signal::TrafficSignalNode& GetTrafficSignalNode() const noexcept { return m_trafficSignalNode; }

    void SetCommandCallback(CommandCallback callback);

    using AckCallback = std::function<void(const communication::Message& ackMsg)>;
    void SetAckCallback(AckCallback callback);

    // Phase 6B Inspection APIs
    size_t GetCommandsReceivedCount() const;
    std::vector<std::string> GetReceivedCommands() const;
    std::string GetLastReceivedCommand() const;
    void ClearReceivedCommands();

private:
    void OnConnectionStateChanged(bool connected);
    void AutoRegisterWorker();

    std::string m_intersectionId{""};
    std::string m_coordinatorIp{""};
    uint16_t m_coordinatorPort{0};

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_registered{false};
    std::atomic<bool> m_running{false};

    // Owned subsystems
    communication::SocketClient m_socketClient{};
    HeartbeatManager m_heartbeatManager{};
    SignalController m_signalController{};
    traffic_signal::TrafficSignalNode m_trafficSignalNode;

    mutable std::mutex m_ackMutex{};
    std::condition_variable m_ackCv{};

    mutable std::mutex m_configMutex{};
    CommandCallback m_commandCallback{nullptr};

    mutable std::mutex m_ackCbMutex{};
    AckCallback m_ackCallback{nullptr};

    mutable std::mutex m_receivedCommandsMutex{};
    size_t m_commandsReceivedCount{0};
    std::string m_lastReceivedCommand{""};
    std::vector<std::string> m_receivedCommands{};
};

} // namespace intersection
} // namespace aegis

#endif // AEGIS_INTERSECTION_INTERSECTION_NODE_H
