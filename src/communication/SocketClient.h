/**
 * @file SocketClient.h
 * @brief Resilient, non-blocking TCP client with automatic reconnect.
 * 
 * Implements connection lifecycle management, synchronous and asynchronous
 * reception of framed AERS messages, and thread-safe transmission.
 */

#ifndef AEGIS_COMMUNICATION_SOCKET_CLIENT_H
#define AEGIS_COMMUNICATION_SOCKET_CLIENT_H

#include "Message.h"
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

namespace aegis {
namespace communication {

/**
 * @brief Thread-safe TCP client with automated reconnection and stream framing.
 */
class SocketClient {
public:
    using MessageCallback    = std::function<void(const Message& message)>;
    using ConnectionCallback = std::function<void(bool connected)>;
    using ErrorCallback      = std::function<void(const std::string& error)>;

    /**
     * @brief Construct an uncommitted SocketClient.
     */
    SocketClient();

    /**
     * @brief Destructor guarantees disconnection and resource cleanup.
     */
    virtual ~SocketClient();

    // Non-copyable, movable
    SocketClient(const SocketClient&) = delete;
    SocketClient& operator=(const SocketClient&) = delete;
    SocketClient(SocketClient&&) noexcept;
    SocketClient& operator=(SocketClient&&) noexcept;

    // =========================================================================
    // Connection Management
    // =========================================================================

    /**
     * @brief Connect to target TCP server with timeout.
     * @param host Hostname or IPv4 string (e.g. "127.0.0.1", "192.168.1.100").
     * @param port Remote TCP port number.
     * @param timeoutMs Connection timeout in milliseconds.
     * @return true if connection established; false on failure or timeout.
     */
    bool Connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 3000);

    /**
     * @brief Explicitly disconnect from the server and stop background worker.
     */
    void Disconnect();

    /**
     * @brief Check if client is currently connected.
     */
    bool IsConnected() const noexcept;

    /**
     * @brief Configure automatic reconnection policy.
     * @param enable Whether to automatically reconnect upon dropped connection.
     * @param intervalMs Delay between reconnect attempts in milliseconds (default: 2000ms).
     * @param maxAttempts Maximum attempts before giving up (0 = retry indefinitely).
     */
    void SetAutoReconnect(bool enable, uint32_t intervalMs = 2000, uint32_t maxAttempts = 0);

    /**
     * @brief Check if automatic reconnection is enabled.
     */
    bool IsAutoReconnectEnabled() const noexcept;

    /**
     * @brief Get configured reconnect interval in milliseconds.
     */
    uint32_t GetReconnectIntervalMs() const noexcept;

    /**
     * @brief Get count of reconnect attempts executed so far.
     */
    uint32_t GetReconnectAttemptCount() const noexcept;

    // =========================================================================
    // Transmission (Thread-Safe)
    // =========================================================================

    /**
     * @brief Serialize and transmit a typed Message packet.
     * @param message Message to transmit.
     * @return true if written to socket successfully; false on disconnection or socket error.
     */
    bool Send(const Message& message);

    /**
     * @brief Transmit a raw binary byte vector over the TCP connection.
     * @param data Byte buffer to send.
     * @return true on success; false otherwise.
     */
    bool Send(const std::vector<uint8_t>& data);

    /**
     * @brief Transmit a raw string payload over the TCP connection.
     * @param rawPayload Raw string data to send.
     * @return true on success; false otherwise.
     */
    bool Send(const std::string& rawPayload);

    // =========================================================================
    // Synchronous Reception (Thread-Safe)
    // =========================================================================

    /**
     * @brief Wait for and pop the next parsed Message from the receive queue.
     * @param outMessage Populated with received Message.
     * @param timeoutMs Maximum milliseconds to wait (default: 1000ms).
     * @return true if a message was received within timeout; false on timeout or disconnect.
     */
    bool Receive(Message& outMessage, uint32_t timeoutMs = 1000);

    /**
     * @brief Wait for and receive the payload of the next incoming message as string.
     * @param outPayload String populated with payload.
     * @param timeoutMs Timeout in milliseconds.
     * @return true on receipt; false on timeout.
     */
    bool Receive(std::string& outPayload, uint32_t timeoutMs = 1000);

    /**
     * @brief Wait for and receive binary data of the next incoming message.
     * @param outData Populated with byte vector.
     * @param timeoutMs Timeout in milliseconds.
     * @return true on receipt; false on timeout.
     */
    bool Receive(std::vector<uint8_t>& outData, uint32_t timeoutMs = 1000);

    // =========================================================================
    // Asynchronous Callbacks
    // =========================================================================

    void SetMessageHandler(MessageCallback callback);
    void SetConnectionHandler(ConnectionCallback callback);
    void SetErrorHandler(ErrorCallback callback);

    // =========================================================================
    // Diagnostic Information
    // =========================================================================

    std::string GetServerHost() const;
    uint16_t GetServerPort() const noexcept;

private:
    void EventLoop();
    void WakeupEventLoop();
    bool AttemptConnectInternal(const std::string& host, uint16_t port, uint32_t timeoutMs);
    void CloseSocketInternal();
    bool InternalSendRaw(const uint8_t* data, size_t length);

    int m_socketFd{-1};
    std::string m_serverHost{""};
    uint16_t m_serverPort{0};
    uint32_t m_connectTimeoutMs{3000};

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_autoReconnect{true};
    std::atomic<uint32_t> m_reconnectIntervalMs{2000};
    std::atomic<uint32_t> m_maxReconnectAttempts{0};
    std::atomic<uint32_t> m_reconnectAttemptCount{0};

    int m_wakeupPipe[2]{-1, -1};
    std::thread m_workerThread{};

    mutable std::mutex m_socketMutex{};
    std::vector<uint8_t> m_rxBuffer{};

    mutable std::mutex m_rxQueueMutex{};
    std::condition_variable m_rxCv{};
    std::deque<Message> m_rxQueue{};
    static constexpr size_t MAX_QUEUE_SIZE = 1000;

    mutable std::mutex m_callbacksMutex{};
    MessageCallback    m_onMessage{nullptr};
    ConnectionCallback m_onConnectionChange{nullptr};
    ErrorCallback      m_onError{nullptr};
};

} // namespace communication
} // namespace aegis

#endif // AEGIS_COMMUNICATION_SOCKET_CLIENT_H
