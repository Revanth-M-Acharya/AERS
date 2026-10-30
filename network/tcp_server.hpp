/**
 * @file tcp_server.hpp
 * @brief QNX Network Communication Layer - Thread-Safe Multi-Client TCP Server.
 *
 * Implements a robust, QNX-compatible TCP server supporting simultaneous connections
 * from multiple Traffic Nodes (Node A, Node B, Node C), client disconnect detection,
 * graceful reconnection, message framing, and 5-second heartbeat timeout supervision.
 */

#ifndef AEGIS_NETWORK_TCP_SERVER_HPP
#define AEGIS_NETWORK_TCP_SERVER_HPP

#include "message.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <memory>

namespace aegis {
namespace network {

/**
 * @brief Representation of an individual connected client session.
 */
struct ClientSession {
    int socketFd{-1};
    std::string nodeId{};
    std::string ipAddress{};
    uint16_t port{0};
    uint64_t connectedAtMs{0};
    uint64_t lastHeartbeatMs{0};
    std::vector<uint8_t> rxBuffer{};
    std::atomic<bool> isAlive{true};
};

/**
 * @brief Statistics structure for monitoring server throughput and health.
 */
struct ServerStatistics {
    uint64_t totalConnectionsAccepted{0};
    uint64_t currentConnections{0};
    uint64_t messagesReceived{0};
    uint64_t messagesSent{0};
    uint64_t bytesReceived{0};
    uint64_t bytesSent{0};
    uint64_t heartbeatsReceived{0};
    uint64_t timeoutDisconnections{0};
    uint64_t checksumFailures{0};
    uint64_t invalidPackets{0};
};

/**
 * @brief QNX-compatible multi-client TCP server for Coordinator.
 */
class TcpServer {
public:
    using MessageCallback    = std::function<void(const std::string& nodeId, const Message& message)>;
    using ConnectCallback    = std::function<void(const std::string& nodeId, const std::string& ip, uint16_t port)>;
    using DisconnectCallback = std::function<void(const std::string& nodeId)>;
    using ErrorCallback      = std::function<void(const std::string& nodeId, const std::string& error)>;

    TcpServer();
    virtual ~TcpServer();

    // Non-copyable, non-movable
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    // =========================================================================
    // Server Lifecycle (CamelCase & lowercase)
    // =========================================================================

    bool Bind(uint16_t port, const std::string& host = "0.0.0.0");
    bool bind(uint16_t port, const std::string& host = "0.0.0.0") { return Bind(port, host); }

    bool Listen(int backlog = 128);
    bool listen(int backlog = 128) { return Listen(backlog); }

    bool Start(uint16_t port, const std::string& host = "0.0.0.0", int backlog = 128);
    bool start(uint16_t port, const std::string& host = "0.0.0.0", int backlog = 128) {
        return Start(port, host, backlog);
    }

    void Stop();
    void stop() { Stop(); }

    bool IsRunning() const noexcept { return isRunning_.load(); }
    bool is_running() const noexcept { return IsRunning(); }

    uint16_t GetPort() const noexcept { return port_; }
    uint16_t get_port() const noexcept { return port_; }

    std::string GetHost() const;
    std::string get_host() const { return GetHost(); }

    // =========================================================================
    // Transmission (Thread-Safe)
    // =========================================================================

    bool Send(const std::string& nodeId, const Message& msg);
    bool send(const std::string& nodeId, const Message& msg) { return Send(nodeId, msg); }

    bool Send(int socketFd, const Message& msg);
    bool send(int socketFd, const Message& msg) { return Send(socketFd, msg); }

    size_t Broadcast(const Message& msg);
    size_t broadcast(const Message& msg) { return Broadcast(msg); }

    // =========================================================================
    // Client & Node Management
    // =========================================================================

    bool IsNodeConnected(const std::string& nodeId) const;
    bool is_node_connected(const std::string& nodeId) const { return IsNodeConnected(nodeId); }

    std::vector<std::string> GetConnectedNodes() const;
    std::vector<std::string> get_connected_nodes() const { return GetConnectedNodes(); }

    size_t GetClientCount() const;
    size_t client_count() const { return GetClientCount(); }

    void RemoveNode(const std::string& nodeId);
    void remove_node(const std::string& nodeId) { RemoveNode(nodeId); }

    // =========================================================================
    // Event Callbacks
    // =========================================================================

    void SetOnMessage(MessageCallback cb);
    void set_on_message(MessageCallback cb) { SetOnMessage(std::move(cb)); }

    void SetOnConnect(ConnectCallback cb);
    void set_on_connect(ConnectCallback cb) { SetOnConnect(std::move(cb)); }

    void SetOnDisconnect(DisconnectCallback cb);
    void set_on_disconnect(DisconnectCallback cb) { SetOnDisconnect(std::move(cb)); }

    void SetOnError(ErrorCallback cb);
    void set_on_error(ErrorCallback cb) { SetOnError(std::move(cb)); }

    // =========================================================================
    // Configuration & Supervision
    // =========================================================================

    void SetHeartbeatTimeoutMs(uint64_t timeoutMs) noexcept;
    void set_heartbeat_timeout_ms(uint64_t timeoutMs) noexcept { SetHeartbeatTimeoutMs(timeoutMs); }

    uint64_t GetHeartbeatTimeoutMs() const noexcept { return heartbeatTimeoutMs_; }
    uint64_t get_heartbeat_timeout_ms() const noexcept { return heartbeatTimeoutMs_; }

    ServerStatistics GetStatistics() const;
    ServerStatistics get_statistics() const { return GetStatistics(); }

    void ResetStatistics();
    void reset_statistics() { ResetStatistics(); }

    std::string GenerateStatusReport() const;
    std::string generate_status_report() const { return GenerateStatusReport(); }

private:
    static uint64_t GetCurrentTimeMs() noexcept;
    static void SetThreadName(const std::string& name);

    void AcceptLoop();
    void HandleClient(std::shared_ptr<ClientSession> client);
    void SupervisorLoop();
    void CleanupInactiveClients();

    int serverSocketFd_{-1};
    uint16_t port_{0};
    std::string host_{"0.0.0.0"};
    std::atomic<bool> isRunning_{false};
    uint64_t heartbeatTimeoutMs_{5000}; // Default 5 seconds per requirement

    // Threads
    std::thread acceptThread_;
    std::thread supervisorThread_;
    std::vector<std::thread> clientWorkerThreads_;

    // Mutexes
    mutable std::mutex clientsMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex callbackMutex_;
    mutable std::mutex threadsMutex_;

    // State tables
    std::unordered_map<int, std::shared_ptr<ClientSession>> clientsByFd_;
    std::unordered_map<std::string, int> nodeToFdMap_;

    // Callbacks
    MessageCallback onMessage_{nullptr};
    ConnectCallback onConnect_{nullptr};
    DisconnectCallback onDisconnect_{nullptr};
    ErrorCallback onError_{nullptr};

    // Telemetry
    ServerStatistics stats_{};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_TCP_SERVER_HPP
