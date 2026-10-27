/**
 * @file SocketClient.cpp
 * @brief Implementation of SocketClient TCP client with auto-reconnect.
 */

#include "SocketClient.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

namespace aegis {
namespace communication {

namespace {

bool SetSocketNonBlocking(int fd) {
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

void ConfigureSocketOptions(int fd) {
    if (fd < 0) return;
    int opt = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const void*>(&opt), sizeof(opt));
#if defined(SO_NOSIGPIPE)
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const void*>(&opt), sizeof(opt));
#endif
}

} // anonymous namespace

SocketClient::SocketClient() {
    m_wakeupPipe[0] = -1;
    m_wakeupPipe[1] = -1;
}

SocketClient::~SocketClient() {
    Disconnect();
}

SocketClient::SocketClient(SocketClient&& other) noexcept {
    Disconnect();
    other.Disconnect();

    m_socketFd = other.m_socketFd;
    other.m_socketFd = -1;
    m_serverHost = std::move(other.m_serverHost);
    m_serverPort = other.m_serverPort;
    other.m_serverPort = 0;
    m_connected.store(other.m_connected.load());
}

SocketClient& SocketClient::operator=(SocketClient&& other) noexcept {
    if (this != &other) {
        Disconnect();
        other.Disconnect();

        m_socketFd = other.m_socketFd;
        other.m_socketFd = -1;
        m_serverHost = std::move(other.m_serverHost);
        m_serverPort = other.m_serverPort;
        other.m_serverPort = 0;
        m_connected.store(other.m_connected.load());
    }
    return *this;
}

bool SocketClient::AttemptConnectInternal(const std::string& host, uint16_t port, uint32_t timeoutMs) {
    CloseSocketInternal();

    addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* res = nullptr;
    std::string portStr = std::to_string(port);
    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0 || res == nullptr) {
        return false;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(res);
        return false;
    }

    ConfigureSocketOptions(fd);
    if (!SetSocketNonBlocking(fd)) {
        close(fd);
        freeaddrinfo(res);
        return false;
    }

    int connRes = connect(fd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);

    if (connRes < 0) {
        if (errno != EINPROGRESS) {
            close(fd);
            return false;
        }

        // Wait for connect completion via poll
        pollfd pfd{};
        pfd.fd = fd;
        pfd.events = POLLOUT;

        int pollRet = poll(&pfd, 1, static_cast<int>(timeoutMs));
        if (pollRet <= 0) {
            close(fd);
            return false; // Timeout or error
        }

        int soError = 0;
        socklen_t len = sizeof(soError);
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<void*>(&soError), &len) < 0 || soError != 0) {
            close(fd);
            return false;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        m_socketFd = fd;
        m_rxBuffer.clear();
    }

    return true;
}

bool SocketClient::Connect(const std::string& host, uint16_t port, uint32_t timeoutMs) {
    if (m_connected.load()) {
        if (m_serverHost == host && m_serverPort == port) {
            return true;
        }
        Disconnect();
    }

    m_serverHost = host;
    m_serverPort = port;
    m_connectTimeoutMs = timeoutMs;
    m_reconnectAttemptCount.store(0);

    // Create wakeup pipe if needed
    if (m_wakeupPipe[0] < 0) {
        if (pipe(m_wakeupPipe) < 0) {
            return false;
        }
        SetSocketNonBlocking(m_wakeupPipe[0]);
        SetSocketNonBlocking(m_wakeupPipe[1]);
    }

    bool success = AttemptConnectInternal(host, port, timeoutMs);
    m_connected.store(success);

    // Start background event loop worker
    if (!m_running.exchange(true)) {
        m_workerThread = std::thread(&SocketClient::EventLoop, this);
    }

    if (success) {
        ConnectionCallback callback = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_callbacksMutex);
            callback = m_onConnectionChange;
        }
        if (callback) {
            callback(true);
        }
    }

    return success;
}

void SocketClient::Disconnect() {
    m_autoReconnect.store(false);
    if (!m_running.exchange(false)) {
        CloseSocketInternal();
        m_connected.store(false);
        return;
    }

    WakeupEventLoop();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    CloseSocketInternal();
    m_connected.store(false);

    if (m_wakeupPipe[0] >= 0) {
        close(m_wakeupPipe[0]);
        m_wakeupPipe[0] = -1;
    }
    if (m_wakeupPipe[1] >= 0) {
        close(m_wakeupPipe[1]);
        m_wakeupPipe[1] = -1;
    }

    // Wakeup any blocking Receive calls
    m_rxCv.notify_all();
}

void SocketClient::CloseSocketInternal() {
    std::lock_guard<std::mutex> lock(m_socketMutex);
    if (m_socketFd >= 0) {
        close(m_socketFd);
        m_socketFd = -1;
    }
}

bool SocketClient::IsConnected() const noexcept {
    return m_connected.load();
}

void SocketClient::SetAutoReconnect(bool enable, uint32_t intervalMs, uint32_t maxAttempts) {
    m_autoReconnect.store(enable);
    m_reconnectIntervalMs.store(intervalMs);
    m_maxReconnectAttempts.store(maxAttempts);
}

bool SocketClient::IsAutoReconnectEnabled() const noexcept {
    return m_autoReconnect.load();
}

uint32_t SocketClient::GetReconnectIntervalMs() const noexcept {
    return m_reconnectIntervalMs.load();
}

uint32_t SocketClient::GetReconnectAttemptCount() const noexcept {
    return m_reconnectAttemptCount.load();
}

void SocketClient::WakeupEventLoop() {
    if (m_wakeupPipe[1] >= 0) {
        char sig = 'q';
        ssize_t written = write(m_wakeupPipe[1], &sig, 1);
        (void)written;
    }
}

void SocketClient::EventLoop() {
    while (m_running.load()) {
        if (!m_connected.load()) {
            if (!m_autoReconnect.load()) {
                // Sleep waiting for wakeup
                pollfd pfdWake{};
                pfdWake.fd = m_wakeupPipe[0];
                pfdWake.events = POLLIN;
                poll(&pfdWake, 1, 200);
                continue;
            }

            uint32_t maxAttempts = m_maxReconnectAttempts.load();
            if (maxAttempts > 0 && m_reconnectAttemptCount.load() >= maxAttempts) {
                // Cease further attempts
                pollfd pfdWake{};
                pfdWake.fd = m_wakeupPipe[0];
                pfdWake.events = POLLIN;
                poll(&pfdWake, 1, 200);
                continue;
            }

            // Sleep interruptibly for reconnect interval
            pollfd pfdWake{};
            pfdWake.fd = m_wakeupPipe[0];
            pfdWake.events = POLLIN;
            int ret = poll(&pfdWake, 1, static_cast<int>(m_reconnectIntervalMs.load()));
            if (!m_running.load()) break;

            if (ret > 0 && (pfdWake.revents & POLLIN)) {
                char drain[32];
                while (read(m_wakeupPipe[0], drain, sizeof(drain)) > 0) {}
                if (!m_running.load()) break;
            }

            m_reconnectAttemptCount.fetch_add(1);
            if (AttemptConnectInternal(m_serverHost, m_serverPort, m_connectTimeoutMs)) {
                m_connected.store(true);
                m_reconnectAttemptCount.store(0);

                ConnectionCallback callback = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_callbacksMutex);
                    callback = m_onConnectionChange;
                }
                if (callback) {
                    callback(true);
                }
            }
            continue;
        }

        // Active connection polling
        pollfd pfds[2]{};
        pfds[0].fd = m_wakeupPipe[0];
        pfds[0].events = POLLIN;

        int activeFd = -1;
        {
            std::lock_guard<std::mutex> lock(m_socketMutex);
            activeFd = m_socketFd;
        }

        if (activeFd < 0) {
            m_connected.store(false);
            continue;
        }

        pfds[1].fd = activeFd;
        pfds[1].events = POLLIN | POLLERR | POLLHUP;

        int ret = poll(pfds, 2, 50);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (!m_running.load()) break;
        if (ret == 0) continue; // Timeout

        if ((pfds[0].revents & POLLIN) != 0) {
            char drain[32];
            while (read(m_wakeupPipe[0], drain, sizeof(drain)) > 0) {}
            if (!m_running.load()) break;
        }

        if ((pfds[1].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
            // Socket disconnected
            CloseSocketInternal();
            m_connected.store(false);

            ConnectionCallback callback = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_callbacksMutex);
                callback = m_onConnectionChange;
            }
            if (callback) {
                callback(false);
            }
            m_rxCv.notify_all();
            continue;
        }

        if ((pfds[1].revents & POLLIN) != 0) {
            uint8_t buffer[4096];
            bool disconnectDetected = false;

            while (true) {
                ssize_t bytesRead = recv(activeFd, buffer, sizeof(buffer), 0);
                if (bytesRead > 0) {
                    std::lock_guard<std::mutex> lock(m_socketMutex);
                    m_rxBuffer.insert(m_rxBuffer.end(), buffer, buffer + bytesRead);
                } else if (bytesRead == 0) {
                    disconnectDetected = true;
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        break;
                    }
                    disconnectDetected = true;
                    break;
                }
            }

            // Parse complete incoming messages
            std::vector<Message> newlyParsed;
            {
                std::lock_guard<std::mutex> lock(m_socketMutex);
                Message parsedMsg;
                while (Message::ParseNext(m_rxBuffer, parsedMsg)) {
                    newlyParsed.push_back(parsedMsg);
                }
            }

            if (!newlyParsed.empty()) {
                MessageCallback msgCallback = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_callbacksMutex);
                    msgCallback = m_onMessage;
                }

                {
                    std::lock_guard<std::mutex> lock(m_rxQueueMutex);
                    for (const auto& msg : newlyParsed) {
                        if (m_rxQueue.size() >= MAX_QUEUE_SIZE) {
                            m_rxQueue.pop_front();
                        }
                        m_rxQueue.push_back(msg);
                    }
                }
                m_rxCv.notify_all();

                if (msgCallback) {
                    for (const auto& msg : newlyParsed) {
                        msgCallback(msg);
                    }
                }
            }

            if (disconnectDetected) {
                CloseSocketInternal();
                m_connected.store(false);

                ConnectionCallback callback = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_callbacksMutex);
                    callback = m_onConnectionChange;
                }
                if (callback) {
                    callback(false);
                }
                m_rxCv.notify_all();
            }
        }
    }
}

bool SocketClient::InternalSendRaw(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) return false;

    int activeFd = -1;
    {
        std::lock_guard<std::mutex> lock(m_socketMutex);
        activeFd = m_socketFd;
    }

    if (activeFd < 0 || !m_connected.load()) return false;

    size_t totalSent = 0;
    while (totalSent < length) {
        int flags = 0;
#if defined(MSG_NOSIGNAL)
        flags |= MSG_NOSIGNAL;
#endif
        ssize_t sent = send(activeFd, data + totalSent, length - totalSent, flags);
        if (sent > 0) {
            totalSent += static_cast<size_t>(sent);
        } else if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                pollfd pfd{};
                pfd.fd = activeFd;
                pfd.events = POLLOUT;
                int ret = poll(&pfd, 1, 200);
                if (ret <= 0) return false;
                continue;
            }
            // Broken connection
            CloseSocketInternal();
            m_connected.store(false);
            WakeupEventLoop();
            return false;
        } else {
            return false;
        }
    }
    return true;
}

bool SocketClient::Send(const Message& message) {
    std::vector<uint8_t> payload = message.Serialize();
    return Send(payload);
}

bool SocketClient::Send(const std::vector<uint8_t>& data) {
    return InternalSendRaw(data.data(), data.size());
}

bool SocketClient::Send(const std::string& rawPayload) {
    return InternalSendRaw(reinterpret_cast<const uint8_t*>(rawPayload.data()), rawPayload.size());
}

bool SocketClient::Receive(Message& outMessage, uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(m_rxQueueMutex);
    m_rxCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
        return !m_rxQueue.empty() || !m_connected.load() || !m_running.load();
    });

    if (!m_rxQueue.empty()) {
        outMessage = std::move(m_rxQueue.front());
        m_rxQueue.pop_front();
        return true;
    }

    return false;
}

bool SocketClient::Receive(std::string& outPayload, uint32_t timeoutMs) {
    Message msg;
    if (Receive(msg, timeoutMs)) {
        outPayload = msg.GetPayload();
        return true;
    }
    return false;
}

bool SocketClient::Receive(std::vector<uint8_t>& outData, uint32_t timeoutMs) {
    Message msg;
    if (Receive(msg, timeoutMs)) {
        outData = msg.Serialize();
        return true;
    }
    return false;
}

void SocketClient::SetMessageHandler(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    m_onMessage = std::move(callback);
}

void SocketClient::SetConnectionHandler(ConnectionCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    m_onConnectionChange = std::move(callback);
}

void SocketClient::SetErrorHandler(ErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbacksMutex);
    m_onError = std::move(callback);
}

std::string SocketClient::GetServerHost() const {
    return m_serverHost;
}

uint16_t SocketClient::GetServerPort() const noexcept {
    return m_serverPort;
}

} // namespace communication
} // namespace aegis
