/**
 * @file tcp_client.cpp
 * @brief QNX Network Communication Layer - Thread-Safe TCP Client Implementation.
 */

#include "tcp_client.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <pthread.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace aegis {
namespace network {

uint64_t TcpClient::GetCurrentTimeMs() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

void TcpClient::SetThreadName(const std::string& name) {
#if defined(__QNXNTO__) || defined(__linux__)
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name.substr(0, 15).c_str());
#endif
}

TcpClient::TcpClient(std::string nodeId)
    : nodeId_(std::move(nodeId)) {
    std::signal(SIGPIPE, SIG_IGN);
}

TcpClient::~TcpClient() {
    Disconnect();
}

bool TcpClient::Connect(const std::string& host, uint16_t port, uint32_t timeoutMs) {
    if (isConnected_.load()) {
        std::cerr << "[NET_CLIENT] [WARN] Already connected to " << host_ << ":" << port_ << "\n";
        return true;
    }

    host_ = host;
    port_ = port;

    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[NET_CLIENT] [ERROR] socket() creation failed: " << strerror(errno) << "\n";
        return false;
    }

    // Set non-blocking mode to support connection timeout
    int flags = ::fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        ::fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in servAddr{};
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port_);

    if (::inet_pton(AF_INET, host_.c_str(), &servAddr.sin_addr) <= 0) {
        std::cerr << "[NET_CLIENT] [ERROR] Invalid host address: " << host_ << "\n";
        ::close(sock);
        return false;
    }

    int res = ::connect(sock, reinterpret_cast<struct sockaddr*>(&servAddr), sizeof(servAddr));
    if (res < 0) {
        if (errno != EINPROGRESS) {
            std::cerr << "[NET_CLIENT] [ERROR] connect() failed: " << strerror(errno) << "\n";
            ::close(sock);
            return false;
        }

        // Wait for connect completion using poll
        struct pollfd pfd{};
        pfd.fd = sock;
        pfd.events = POLLOUT;

        int pollRes = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
        if (pollRes <= 0) {
            std::cerr << "[NET_CLIENT] [ERROR] connect() timed out after " << timeoutMs << "ms\n";
            ::close(sock);
            return false;
        }

        int soError = 0;
        socklen_t len = sizeof(soError);
        if (::getsockopt(sock, SOL_SOCKET, SO_ERROR, &soError, &len) < 0 || soError != 0) {
            std::cerr << "[NET_CLIENT] [ERROR] connect() socket error: "
                      << strerror(soError != 0 ? soError : errno) << "\n";
            ::close(sock);
            return false;
        }
    }

    // Restore blocking mode
    if (flags >= 0) {
        ::fcntl(sock, F_SETFL, flags);
    }

    // Set socket receive & send timeouts (500 ms)
    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    ::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Enable TCP_NODELAY
    int nodelay = 1;
    ::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    // Enable SO_LINGER to gracefully flush buffers on close
    struct linger sl{};
    sl.l_onoff = 1;
    sl.l_linger = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_LINGER, &sl, sizeof(sl));

#ifdef SO_NOSIGPIPE
    int noSigpipe = 1;
    ::setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
#endif

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        socketFd_ = sock;
        isConnected_.store(true);
        isRunning_.store(true);
    }

    // Log connection per requirement
    std::cout << "[NET_CLIENT] [CONNECT] Connected to " << host_ << ":" << port_
              << " as Node: " << nodeId_ << "\n";

    // Spawn native background receiver thread
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    receiveThread_ = std::thread(&TcpClient::ReceiveLoop, this);

    // Automatically send initial REGISTER message
    Message regMsg = Message::CreateRegister(nodeId_, "REGISTER_NODE");
    Send(regMsg);

    // Start 1-second automated heartbeat thread per requirement
    StartHeartbeat(heartbeatIntervalMs_);

    // Notify callback
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (onConnectionChange_) {
            onConnectionChange_(true);
        }
    }

    return true;
}

void TcpClient::Disconnect() {
    bool wasConnected = isConnected_.exchange(false);
    isRunning_.store(false);

    StopHeartbeat();

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        if (socketFd_ >= 0) {
            ::shutdown(socketFd_, SHUT_RDWR);
            ::close(socketFd_);
            socketFd_ = -1;
        }
    }

    // Wake any synchronous receive waiters
    syncRxCv_.notify_all();

    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }

    if (reconnectThread_.joinable()) {
        reconnectThread_.join();
    }

    if (wasConnected) {
        std::cout << "[NET_CLIENT] [DISCONNECT] Disconnected from server " << host_ << ":" << port_ << "\n";
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (onConnectionChange_) {
            onConnectionChange_(false);
        }
    }
}

bool TcpClient::Reconnect() {
    Disconnect();
    return Connect(host_, port_);
}

std::string TcpClient::GetHost() const {
    return host_;
}

bool TcpClient::Send(const Message& msg) {
    if (!isConnected_.load()) {
        return false;
    }

    const std::vector<uint8_t> packet = msg.Serialize();
    const ssize_t packetSize = static_cast<ssize_t>(packet.size());
    ssize_t totalSent = 0;

    std::lock_guard<std::mutex> lock(socketMutex_);
    if (socketFd_ < 0) return false;

    while (totalSent < packetSize && isConnected_.load()) {
        ssize_t sent = ::send(socketFd_, packet.data() + totalSent,
                              static_cast<size_t>(packetSize - totalSent), MSG_NOSIGNAL);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            if (!isRunning_.load() || !isConnected_.load()) {
                return false;
            }
            std::cerr << "[NET_CLIENT] [ERROR] send() failed: " << strerror(errno) << "\n";
            return false;
        }
        totalSent += sent;
    }

    {
        std::lock_guard<std::mutex> sLock(statisticsMutex_);
        stats_.messagesSent++;
        stats_.bytesSent += static_cast<uint64_t>(totalSent);
    }

    // Log message TX per requirement
    if (msg.GetType() == MessageType::HEARTBEAT) {
        std::cout << "[NET_CLIENT] [TX] [HEARTBEAT] Node: " << nodeId_ << "\n";
    } else {
        std::cout << "[NET_CLIENT] [TX] " << msg.GetFormattedLog() << "\n";
    }

    return true;
}

bool TcpClient::Send(MessageType type, const std::string& payload) {
    Message msg(type, nodeId_, payload);
    return Send(msg);
}

bool TcpClient::Receive(Message& outMsg, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(syncRxMutex_);
    if (!syncRxQueue_.empty()) {
        outMsg = syncRxQueue_.front();
        syncRxQueue_.pop_front();
        return true;
    }

    if (!isConnected_.load()) {
        return false;
    }

    bool success = syncRxCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
        return !syncRxQueue_.empty() || !isConnected_.load();
    });

    if (success && !syncRxQueue_.empty()) {
        outMsg = syncRxQueue_.front();
        syncRxQueue_.pop_front();
        return true;
    }

    return false;
}

bool TcpClient::SendHeartbeat() {
    Message hb = Message::CreateHeartbeat(nodeId_);
    bool sent = Send(hb);
    if (sent) {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats_.heartbeatsSent++;
    }
    return sent;
}

void TcpClient::StartHeartbeat(uint32_t intervalMs) {
    heartbeatIntervalMs_ = intervalMs;
    if (heartbeatRunning_.exchange(true)) {
        return; // Already running
    }

    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
    heartbeatThread_ = std::thread(&TcpClient::HeartbeatLoop, this);
}

void TcpClient::StopHeartbeat() {
    heartbeatRunning_.store(false);
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
}

void TcpClient::HeartbeatLoop() {
    SetThreadName("aegis_hb_" + nodeId_);

    while (heartbeatRunning_.load() && isConnected_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatIntervalMs_));

        if (!heartbeatRunning_.load() || !isConnected_.load()) {
            break;
        }

        SendHeartbeat();
    }
}

void TcpClient::ReceiveLoop() {
    SetThreadName("aegis_rx_" + nodeId_);

    uint8_t readBuffer[4096];

    while (isRunning_.load() && isConnected_.load()) {
        int currentSock = -1;
        {
            std::lock_guard<std::mutex> lock(socketMutex_);
            currentSock = socketFd_;
        }
        if (currentSock < 0) break;

        ssize_t bytesRead = ::recv(currentSock, readBuffer, sizeof(readBuffer), 0);

        if (bytesRead > 0) {
            rxBuffer_.insert(rxBuffer_.end(), readBuffer, readBuffer + bytesRead);

            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                stats_.bytesReceived += static_cast<uint64_t>(bytesRead);
            }

            size_t bytesConsumed = 0;
            Message msg;
            while (Message::Deserialize(rxBuffer_, msg, bytesConsumed)) {
                rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + static_cast<ptrdiff_t>(bytesConsumed));

                {
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    stats_.messagesReceived++;
                }

                // Log every message RX per requirement
                std::cout << "[NET_CLIENT] [RX] " << msg.GetFormattedLog() << "\n";

                // Push to sync queue
                {
                    std::lock_guard<std::mutex> sLock(syncRxMutex_);
                    syncRxQueue_.push_back(msg);
                }
                syncRxCv_.notify_one();

                // Dispatch callback
                {
                    std::lock_guard<std::mutex> cbLock(callbackMutex_);
                    if (onMessage_) {
                        onMessage_(msg);
                    }
                }
            }

            if (bytesConsumed > 0 && !rxBuffer_.empty()) {
                rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + static_cast<ptrdiff_t>(bytesConsumed));
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                stats_.checksumFailures++;
            }
        } else if (bytesRead == 0) {
            // Coordinator closed connection
            std::cout << "[NET_CLIENT] [DISCONNECT] Coordinator closed connection\n";
            TriggerConnectionDrop();
            break;
        } else {
            // bytesRead < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }

            if (!isRunning_.load() || !isConnected_.load()) {
                // Connection was intentionally closed
                break;
            }

            std::cout << "[NET_CLIENT] [DISCONNECT] Socket read error: " << strerror(errno) << "\n";
            TriggerConnectionDrop();
            break;
        }
    }
}

void TcpClient::TriggerConnectionDrop() {
    bool wasConnected = isConnected_.exchange(false);
    if (!wasConnected) return;

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats_.connectionDrops++;
    }

    {
        std::lock_guard<std::mutex> lock(socketMutex_);
        if (socketFd_ >= 0) {
            ::close(socketFd_);
            socketFd_ = -1;
        }
    }

    syncRxCv_.notify_all();

    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (onConnectionChange_) {
            onConnectionChange_(false);
        }
    }

    // Trigger auto-reconnect if enabled
    if (autoReconnectEnabled_.load() && !inReconnectLoop_.exchange(true)) {
        if (reconnectThread_.joinable()) {
            reconnectThread_.join();
        }
        reconnectThread_ = std::thread(&TcpClient::ReconnectLoop, this);
    }
}

void TcpClient::ReconnectLoop() {
    SetThreadName("aegis_recon_" + nodeId_);

    uint32_t attempts = 0;

    while (autoReconnectEnabled_.load() && !isConnected_.load()) {
        attempts++;
        reconnectAttempts_.store(attempts);

        {
            std::lock_guard<std::mutex> lock(statisticsMutex_);
            stats_.reconnectAttempts++;
        }

        std::cout << "[NET_CLIENT] [RECONNECT] Attempting reconnect to " << host_ << ":" << port_
                  << " (Attempt " << attempts << ")...\n";

        if (Connect(host_, port_, 2000)) {
            std::cout << "[NET_CLIENT] [RECONNECT] Reconnected successfully to "
                      << host_ << ":" << port_ << " as Node: " << nodeId_ << "\n";

            std::lock_guard<std::mutex> lock(statisticsMutex_);
            stats_.successfulReconnects++;
            break;
        }

        if (maxReconnectRetries_ > 0 && attempts >= maxReconnectRetries_) {
            std::cerr << "[NET_CLIENT] [RECONNECT] Maximum reconnect retries ("
                      << maxReconnectRetries_ << ") reached. Giving up.\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(reconnectIntervalMs_));
    }

    inReconnectLoop_.store(false);
}

void TcpClient::SetAutoReconnect(bool enable, uint32_t intervalMs, uint32_t maxRetries) {
    autoReconnectEnabled_.store(enable);
    reconnectIntervalMs_ = intervalMs;
    maxReconnectRetries_ = maxRetries;
}

void TcpClient::SetOnMessage(MessageCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onMessage_ = std::move(cb);
}

void TcpClient::SetOnConnectionChange(ConnectionCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onConnectionChange_ = std::move(cb);
}

void TcpClient::SetOnError(ErrorCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onError_ = std::move(cb);
}

ClientStatistics TcpClient::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    return stats_;
}

void TcpClient::ResetStatistics() {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    stats_ = ClientStatistics();
}

std::string TcpClient::GenerateStatusReport() const {
    ClientStatistics stats = GetStatistics();

    std::ostringstream ss;
    ss << "==========================================================\n"
       << " QNX TCP Client Status Report                             \n"
       << "==========================================================\n"
       << " Node ID: " << nodeId_ << "\n"
       << " Connected: " << (IsConnected() ? "YES" : "NO") << "\n"
       << " Server: " << host_ << ":" << port_ << "\n"
       << " Heartbeat Active: " << (IsHeartbeatRunning() ? "YES (1 sec)" : "NO") << "\n"
       << " Messages Sent: " << stats.messagesSent << "\n"
       << " Messages Received: " << stats.messagesReceived << "\n"
       << " Heartbeats Sent: " << stats.heartbeatsSent << "\n"
       << " Reconnect Attempts: " << stats.reconnectAttempts << "\n"
       << " Successful Reconnects: " << stats.successfulReconnects << "\n"
       << " Connection Drops: " << stats.connectionDrops << "\n"
       << " Checksum Failures: " << stats.checksumFailures << "\n"
       << "==========================================================\n";
    return ss.str();
}

} // namespace network
} // namespace aegis
