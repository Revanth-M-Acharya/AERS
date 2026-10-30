/**
 * @file tcp_client.hpp
 * @brief QNX Network Communication Layer - Thread-Safe TCP Client.
 *
 * Implements a resilient, QNX-compatible TCP client for edge Traffic Nodes (A, B, C)
 * connecting to the Central Coordinator with automated 1-second heartbeats,
 * automatic reconnection, and framed message transmission.
 */

#ifndef AEGIS_NETWORK_TCP_CLIENT_HPP
#define AEGIS_NETWORK_TCP_CLIENT_HPP

#include "message.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>
#include <condition_variable>
#include <deque>

namespace aegis {
namespace network {

/**
 * @brief Statistics structure for monitoring client throughput and connectivity.
 */
struct ClientStatistics {
    uint64_t messagesSent{0};
    uint64_t messagesReceived{0};
    uint64_t bytesSent{0};
    uint64_t bytesReceived{0};
    uint64_t heartbeatsSent{0};
    uint64_t reconnectAttempts{0};
    uint64_t successfulReconnects{0};
    uint64_t checksumFailures{0};
    uint64_t connectionDrops{0};
};

/**
 * @brief QNX-compatible TCP client for Traffic Nodes.
 */
class TcpClient {
public:
    using MessageCallback    = std::function<void(const Message& message)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    using ErrorCallback      = std::function<void(const std::string& error)>;

    explicit TcpClient(std::string nodeId = "TRAFFIC_NODE_A");
    virtual ~TcpClient();

    // Non-copyable, non-movable
    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;
    TcpClient(TcpClient&&) = delete;
    TcpClient& operator=(TcpClient&&) = delete;

    // =========================================================================
    // Connection Lifecycle (CamelCase & lowercase)
    // =========================================================================

    bool Connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 3000);
    bool connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 3000) {
        return Connect(host, port, timeoutMs);
    }

    void Disconnect();
    void disconnect() { Disconnect(); }

    bool Reconnect();
    bool reconnect() { return Reconnect(); }

    bool IsConnected() const noexcept { return isConnected_.load(); }
    bool is_connected() const noexcept { return IsConnected(); }

    std::string GetHost() const;
    std::string get_host() const { return GetHost(); }

    uint16_t GetPort() const noexcept { return port_; }
    uint16_t get_port() const noexcept { return port_; }

    const std::string& GetNodeId() const noexcept { return nodeId_; }
    const std::string& get_node_id() const noexcept { return nodeId_; }

    void SetNodeId(const std::string& nodeId) { nodeId_ = nodeId; }
    void set_node_id(const std::string& nodeId) { SetNodeId(nodeId); }

    // =========================================================================
    // Transmission & Reception
    // =========================================================================

    bool Send(const Message& msg);
    bool send(const Message& msg) { return Send(msg); }

    bool Send(MessageType type, const std::string& payload = "");
    bool send(MessageType type, const std::string& payload = "") {
        return Send(type, payload);
    }

    bool Receive(Message& outMsg, uint32_t timeoutMs = 1000);
    bool receive(Message& outMsg, uint32_t timeoutMs = 1000) {
        return Receive(outMsg, timeoutMs);
    }

    // =========================================================================
    // Heartbeat Functionality (1-second periodic interval)
    // =========================================================================

    bool SendHeartbeat();
    bool send_heartbeat() { return SendHeartbeat(); }

    void StartHeartbeat(uint32_t intervalMs = 1000);
    void start_heartbeat(uint32_t intervalMs = 1000) { StartHeartbeat(intervalMs); }

    void StopHeartbeat();
    void stop_heartbeat() { StopHeartbeat(); }

    bool IsHeartbeatRunning() const noexcept { return heartbeatRunning_.load(); }
    bool is_heartbeat_running() const noexcept { return IsHeartbeatRunning(); }

    void SetHeartbeatIntervalMs(uint32_t intervalMs) noexcept { heartbeatIntervalMs_ = intervalMs; }
    uint32_t GetHeartbeatIntervalMs() const noexcept { return heartbeatIntervalMs_; }

    // =========================================================================
    // Auto-Reconnection Policy
    // =========================================================================

    void SetAutoReconnect(bool enable, uint32_t intervalMs = 1000, uint32_t maxRetries = 0);
    void set_auto_reconnect(bool enable, uint32_t intervalMs = 1000, uint32_t maxRetries = 0) {
        SetAutoReconnect(enable, intervalMs, maxRetries);
    }

    bool IsAutoReconnectEnabled() const noexcept { return autoReconnectEnabled_.load(); }
    bool is_auto_reconnect_enabled() const noexcept { return IsAutoReconnectEnabled(); }

    uint32_t GetReconnectIntervalMs() const noexcept { return reconnectIntervalMs_; }
    uint32_t GetReconnectAttemptCount() const noexcept { return reconnectAttempts_.load(); }

    // =========================================================================
    // Callbacks
    // =========================================================================

    void SetOnMessage(MessageCallback cb);
    void set_on_message(MessageCallback cb) { SetOnMessage(std::move(cb)); }

    void SetOnConnectionChange(ConnectionCallback cb);
    void set_on_connection_change(ConnectionCallback cb) { SetOnConnectionChange(std::move(cb)); }

    void SetOnError(ErrorCallback cb);
    void set_on_error(ErrorCallback cb) { SetOnError(std::move(cb)); }

    // =========================================================================
    // Statistics & Reporting
    // =========================================================================

    ClientStatistics GetStatistics() const;
    ClientStatistics get_statistics() const { return GetStatistics(); }

    void ResetStatistics();
    void reset_statistics() { ResetStatistics(); }

    std::string GenerateStatusReport() const;
    std::string generate_status_report() const { return GenerateStatusReport(); }

private:
    static uint64_t GetCurrentTimeMs() noexcept;
    static void SetThreadName(const std::string& name);

    void ReceiveLoop();
    void HeartbeatLoop();
    void ReconnectLoop();
    void TriggerConnectionDrop();

    std::string nodeId_{"TRAFFIC_NODE_A"};
    std::string host_{"127.0.0.1"};
    uint16_t port_{9000};
    int socketFd_{-1};

    std::atomic<bool> isConnected_{false};
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> heartbeatRunning_{false};
    std::atomic<bool> autoReconnectEnabled_{true};
    std::atomic<bool> inReconnectLoop_{false};
    std::atomic<uint32_t> reconnectAttempts_{0};

    uint32_t heartbeatIntervalMs_{1000};  // Default 1 second per requirement
    uint32_t reconnectIntervalMs_{1000};  // Reconnection check interval
    uint32_t maxReconnectRetries_{0};     // 0 = retry indefinitely

    // Threads
    std::thread receiveThread_;
    std::thread heartbeatThread_;
    std::thread reconnectThread_;

    // Mutexes & Condition Variables
    mutable std::mutex socketMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex callbackMutex_;
    mutable std::mutex syncRxMutex_;
    std::condition_variable syncRxCv_;
    std::deque<Message> syncRxQueue_;

    // Stream RX buffer
    std::vector<uint8_t> rxBuffer_;

    // Callbacks
    MessageCallback onMessage_{nullptr};
    ConnectionCallback onConnectionChange_{nullptr};
    ErrorCallback onError_{nullptr};

    // Telemetry
    ClientStatistics stats_{};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_TCP_CLIENT_HPP

