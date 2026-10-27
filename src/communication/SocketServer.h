/**
 * @file SocketServer.h
 * @brief Thread-safe, non-blocking TCP server for multi-client AERS communication.
 * 
 * Manages concurrent connections from multiple edge nodes (Ambulance OBUs,
 * Roadside Intersection Units) with automated message framing and dispatch.
 */

#ifndef AEGIS_COMMUNICATION_SOCKET_SERVER_H
#define AEGIS_COMMUNICATION_SOCKET_SERVER_H

#include "Message.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>
#include <chrono>

namespace aegis {
namespace communication {

/**
 * @brief Representation of an individual connected client session.
 */
struct ClientConnection {
    int id{0};                                                      ///< Unique client ID
    int socketFd{-1};                                               ///< Operating system socket file descriptor
    std::string ipAddress{};                                        ///< Remote IPv4 address
    uint16_t port{0};                                               ///< Remote ephemeral port
    std::vector<uint8_t> rxBuffer{};                                ///< TCP stream defragmentation buffer
    std::chrono::steady_clock::time_point connectedAt{};           ///< Connection establishment time
    std::chrono::steady_clock::time_point lastActivity{};          ///< Last read/write activity timestamp
};

/**
 * @brief Asynchronous, non-blocking, multi-client TCP server.
 */
class SocketServer {
public:
    using MessageHandler   = std::function<void(int clientId, const Message& message)>;
    using ConnectHandler   = std::function<void(int clientId, const std::string& ip, uint16_t port)>;
    using DisconnectHandler = std::function<void(int clientId)>;
    using ErrorHandler     = std::function<void(int clientId, const std::string& error)>;

    /**
     * @brief Construct an unstarted SocketServer.
     */
    SocketServer();

    /**
     * @brief Destructor ensures clean shutdown and resource release.
     */
    virtual ~SocketServer();

    // Non-copyable, movable
    SocketServer(const SocketServer&) = delete;
    SocketServer& operator=(const SocketServer&) = delete;
    SocketServer(SocketServer&&) noexcept;
    SocketServer& operator=(SocketServer&&) noexcept;

    // =========================================================================
    // Server Lifecycle
    // =========================================================================

    /**
     * @brief Start listening on the specified port and host interface.
     * @param port TCP port number (e.g., 8883, 9000).
     * @param host Interface IP to bind (default "0.0.0.0" for all interfaces).
     * @return true if socket was created, bound, and listening; false on failure.
     */
    bool Start(uint16_t port, const std::string& host = "0.0.0.0");

    /**
     * @brief Stop the server, disconnect all clients, and terminate background thread.
     */
    void Stop();

    /**
     * @brief Check if server is currently active and listening.
     */
    bool IsRunning() const noexcept;

    /**
     * @brief Get bound port number.
     */
    uint16_t GetPort() const noexcept;

    /**
     * @brief Get bound host interface.
     */
    std::string GetHost() const;

    // =========================================================================
    // Multi-Client Messaging (Thread-Safe)
    // =========================================================================

    /**
     * @brief Broadcast a typed Message to all currently connected clients.
     * @param message Message to serialize and transmit.
     * @return true if transmitted to all clients without immediate socket error.
     */
    bool Broadcast(const Message& message);

    /**
     * @brief Broadcast a raw binary buffer to all connected clients.
     * @param data Byte vector to transmit.
     * @return true if transmitted successfully.
     */
    bool Broadcast(const std::vector<uint8_t>& data);

    /**
     * @brief Broadcast a raw string payload to all connected clients.
     * @param rawPayload String data to transmit.
     * @return true if transmitted successfully.
     */
    bool Broadcast(const std::string& rawPayload);

    /**
     * @brief Send a typed Message to a specific client by ID.
     * @param clientId Target client ID.
     * @param message Message to serialize and send.
     * @return true on successful transmission; false if client does not exist or write fails.
     */
    bool SendToClient(int clientId, const Message& message);

    /**
     * @brief Send raw binary data to a specific client.
     * @param clientId Target client ID.
     * @param data Raw data bytes.
     * @return true on success; false otherwise.
     */
    bool SendToClient(int clientId, const std::vector<uint8_t>& data);

    /**
     * @brief Send raw string payload to a specific client.
     * @param clientId Target client ID.
     * @param rawPayload String to transmit.
     * @return true on success; false otherwise.
     */
    bool SendToClient(int clientId, const std::string& rawPayload);

    // =========================================================================
    // Client Management
    // =========================================================================

    /**
     * @brief Forcefully disconnect an individual client.
     * @param clientId Target client ID to disconnect.
     */
    void DisconnectClient(int clientId);

    /**
     * @brief Get the current number of connected clients.
     */
    size_t GetClientCount() const;

    /**
     * @brief Retrieve list of all currently connected client IDs.
     */
    std::vector<int> GetConnectedClientIds() const;

    /**
     * @brief Get remote IP and port for a connected client.
     * @param clientId Target client ID.
     * @param outIp Populated with remote IP address.
     * @param outPort Populated with remote port number.
     * @return true if client was found; false otherwise.
     */
    bool GetClientInfo(int clientId, std::string& outIp, uint16_t& outPort) const;

    // =========================================================================
    // Callbacks
    // =========================================================================

    void SetMessageHandler(MessageHandler handler);
    void SetConnectHandler(ConnectHandler handler);
    void SetDisconnectHandler(DisconnectHandler handler);
    void SetErrorHandler(ErrorHandler handler);

private:
    void EventLoop();
    void WakeupEventLoop();
    void HandleNewConnection();
    void HandleClientData(int clientId);
    void CloseClientInternal(int clientId);
    bool InternalSendRaw(int socketFd, const uint8_t* data, size_t length);

    int m_serverFd{-1};
    uint16_t m_port{0};
    std::string m_host{"0.0.0.0"};
    std::atomic<bool> m_running{false};

    std::thread m_workerThread{};
    int m_wakeupPipe[2]{-1, -1};

    mutable std::mutex m_clientsMutex{};
    std::unordered_map<int, std::shared_ptr<ClientConnection>> m_clients{};
    std::atomic<int> m_nextClientId{1};

    mutable std::mutex m_handlersMutex{};
    MessageHandler    m_onMessage{nullptr};
    ConnectHandler    m_onConnect{nullptr};
    DisconnectHandler m_onDisconnect{nullptr};
    ErrorHandler      m_onError{nullptr};
};

} // namespace communication
} // namespace aegis

#endif // AEGIS_COMMUNICATION_SOCKET_SERVER_H
