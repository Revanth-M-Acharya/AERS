/**
 * @file SocketServer.cpp
 * @brief Implementation of SocketServer non-blocking multi-client TCP server.
 */

#include "SocketServer.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <iostream>

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
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const void*>(&opt), sizeof(opt));
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const void*>(&opt), sizeof(opt));
#if defined(SO_NOSIGPIPE)
    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<const void*>(&opt), sizeof(opt));
#endif
}

} // anonymous namespace

SocketServer::SocketServer() {
    m_wakeupPipe[0] = -1;
    m_wakeupPipe[1] = -1;
}

SocketServer::~SocketServer() {
    Stop();
}

SocketServer::SocketServer(SocketServer&& other) noexcept {
    Stop();
    other.Stop();

    m_serverFd = other.m_serverFd;
    other.m_serverFd = -1;
    m_port = other.m_port;
    other.m_port = 0;
    m_host = std::move(other.m_host);
}

SocketServer& SocketServer::operator=(SocketServer&& other) noexcept {
    if (this != &other) {
        Stop();
        other.Stop();

        m_serverFd = other.m_serverFd;
        other.m_serverFd = -1;
        m_port = other.m_port;
        other.m_port = 0;
        m_host = std::move(other.m_host);
    }
    return *this;
}

bool SocketServer::Start(uint16_t port, const std::string& host) {
    if (m_running.load()) {
        return true;
    }

    m_port = port;
    m_host = host;

    // 1. Create listening TCP socket
    m_serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0) {
        return false;
    }

    ConfigureSocketOptions(m_serverFd);
    if (!SetSocketNonBlocking(m_serverFd)) {
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    // 2. Bind to host and port
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (host.empty() || host == "0.0.0.0") {
        serverAddr.sin_addr.s_addr = INADDR_ANY;
    } else {
        if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
            close(m_serverFd);
            m_serverFd = -1;
            return false;
        }
    }

    if (bind(m_serverFd, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    // 3. Listen for connections
    if (listen(m_serverFd, SOMAXCONN) < 0) {
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    // 4. Create wakeup pipe for thread cancellation
    if (pipe(m_wakeupPipe) < 0) {
        close(m_serverFd);
        m_serverFd = -1;
        return false;
    }
    SetSocketNonBlocking(m_wakeupPipe[0]);
    SetSocketNonBlocking(m_wakeupPipe[1]);

    // 5. Start event loop thread
    m_running.store(true);
    m_workerThread = std::thread(&SocketServer::EventLoop, this);

    return true;
}

void SocketServer::Stop() {
    if (!m_running.exchange(false)) {
        if (m_serverFd >= 0) {
            close(m_serverFd);
            m_serverFd = -1;
        }
        return;
    }

    // Wakeup event loop
    WakeupEventLoop();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    // Close listening socket
    if (m_serverFd >= 0) {
        close(m_serverFd);
        m_serverFd = -1;
    }

    // Close wakeup pipe
    if (m_wakeupPipe[0] >= 0) {
        close(m_wakeupPipe[0]);
        m_wakeupPipe[0] = -1;
    }
    if (m_wakeupPipe[1] >= 0) {
        close(m_wakeupPipe[1]);
        m_wakeupPipe[1] = -1;
    }

    // Disconnect and clean all clients
    std::unordered_map<int, std::shared_ptr<ClientConnection>> clientsToClose;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        clientsToClose = std::move(m_clients);
        m_clients.clear();
    }

    for (auto& pair : clientsToClose) {
        if (pair.second && pair.second->socketFd >= 0) {
            close(pair.second->socketFd);
            pair.second->socketFd = -1;
        }
    }
}

bool SocketServer::IsRunning() const noexcept {
    return m_running.load();
}

uint16_t SocketServer::GetPort() const noexcept {
    return m_port;
}

std::string SocketServer::GetHost() const {
    return m_host;
}

void SocketServer::WakeupEventLoop() {
    if (m_wakeupPipe[1] >= 0) {
        char sig = 'q';
        ssize_t written = write(m_wakeupPipe[1], &sig, 1);
        (void)written;
    }
}

void SocketServer::EventLoop() {
    while (m_running.load()) {
        std::vector<pollfd> pollFds;
        std::vector<int> clientIds;

        // Slot 0: Wakeup pipe
        pollfd pfdWakeup{};
        pfdWakeup.fd = m_wakeupPipe[0];
        pfdWakeup.events = POLLIN;
        pollFds.push_back(pfdWakeup);

        // Slot 1: Listening server socket
        pollfd pfdServer{};
        pfdServer.fd = m_serverFd;
        pfdServer.events = POLLIN;
        pollFds.push_back(pfdServer);

        // Slots 2+: Active client sockets
        {
            std::lock_guard<std::mutex> lock(m_clientsMutex);
            pollFds.reserve(2 + m_clients.size());
            clientIds.reserve(m_clients.size());
            for (const auto& pair : m_clients) {
                if (pair.second && pair.second->socketFd >= 0) {
                    pollfd pfdClient{};
                    pfdClient.fd = pair.second->socketFd;
                    pfdClient.events = POLLIN | POLLERR | POLLHUP;
                    pollFds.push_back(pfdClient);
                    clientIds.push_back(pair.first);
                }
            }
        }

        int ret = poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), 50);
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (!m_running.load()) break;
        if (ret == 0) continue; // Timeout

        // 1. Check wakeup pipe
        if ((pollFds[0].revents & POLLIN) != 0) {
            char drain[32];
            while (read(m_wakeupPipe[0], drain, sizeof(drain)) > 0) {}
            if (!m_running.load()) break;
        }

        // 2. Check server socket for incoming client connections
        if ((pollFds[1].revents & POLLIN) != 0) {
            HandleNewConnection();
        }

        // 3. Check client sockets for data or disconnection
        std::vector<int> deadClients;
        for (size_t i = 2; i < pollFds.size(); ++i) {
            int clientId = clientIds[i - 2];
            short revents = pollFds[i].revents;

            if ((revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                deadClients.push_back(clientId);
            } else if ((revents & POLLIN) != 0) {
                HandleClientData(clientId);
            }
        }

        for (int clientId : deadClients) {
            CloseClientInternal(clientId);
        }
    }
}

void SocketServer::HandleNewConnection() {
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    int clientFd = accept(m_serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientFd < 0) {
        return;
    }

    ConfigureSocketOptions(clientFd);
    if (!SetSocketNonBlocking(clientFd)) {
        close(clientFd);
        return;
    }

    char ipStr[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
    uint16_t clientPort = ntohs(clientAddr.sin_port);

    int assignedId = m_nextClientId.fetch_add(1);

    auto session = std::make_shared<ClientConnection>();
    session->id = assignedId;
    session->socketFd = clientFd;
    session->ipAddress = std::string(ipStr);
    session->port = clientPort;
    session->connectedAt = std::chrono::steady_clock::now();
    session->lastActivity = session->connectedAt;

    ConnectHandler connectCallback = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        m_clients[assignedId] = session;
    }
    {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        connectCallback = m_onConnect;
    }

    if (connectCallback) {
        connectCallback(assignedId, session->ipAddress, session->port);
    }
}

void SocketServer::HandleClientData(int clientId) {
    std::shared_ptr<ClientConnection> session = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            session = it->second;
        }
    }

    if (!session || session->socketFd < 0) return;

    uint8_t buffer[4096];
    bool shouldDisconnect = false;

    while (true) {
        ssize_t bytesRead = recv(session->socketFd, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            session->rxBuffer.insert(session->rxBuffer.end(), buffer, buffer + bytesRead);
            session->lastActivity = std::chrono::steady_clock::now();
        } else if (bytesRead == 0) {
            // Orderly disconnect
            shouldDisconnect = true;
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Done reading available chunk
                break;
            }
            shouldDisconnect = true;
            break;
        }
    }

    // Extract all complete messages from accumulated buffer
    MessageHandler msgCallback = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        msgCallback = m_onMessage;
    }

    Message parsedMsg;
    while (Message::ParseNext(session->rxBuffer, parsedMsg)) {
        if (msgCallback) {
            msgCallback(clientId, parsedMsg);
        }
    }

    if (shouldDisconnect) {
        CloseClientInternal(clientId);
    }
}

void SocketServer::CloseClientInternal(int clientId) {
    std::shared_ptr<ClientConnection> session = nullptr;
    DisconnectHandler disconnectCallback = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end()) {
            session = it->second;
            m_clients.erase(it);
        }
    }

    if (session && session->socketFd >= 0) {
        close(session->socketFd);
        session->socketFd = -1;
    }

    {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        disconnectCallback = m_onDisconnect;
    }

    if (disconnectCallback) {
        disconnectCallback(clientId);
    }
}

bool SocketServer::InternalSendRaw(int socketFd, const uint8_t* data, size_t length) {
    if (socketFd < 0 || data == nullptr || length == 0) return false;

    size_t totalSent = 0;
    while (totalSent < length) {
        int flags = 0;
#if defined(MSG_NOSIGNAL)
        flags |= MSG_NOSIGNAL;
#endif
        ssize_t sent = send(socketFd, data + totalSent, length - totalSent, flags);
        if (sent > 0) {
            totalSent += static_cast<size_t>(sent);
        } else if (sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                pollfd pfd{};
                pfd.fd = socketFd;
                pfd.events = POLLOUT;
                int ret = poll(&pfd, 1, 200);
                if (ret <= 0) return false; // Write timeout or error
                continue;
            }
            return false;
        } else {
            return false;
        }
    }
    return true;
}

bool SocketServer::SendToClient(int clientId, const Message& message) {
    std::vector<uint8_t> payload = message.Serialize();
    return SendToClient(clientId, payload);
}

bool SocketServer::SendToClient(int clientId, const std::vector<uint8_t>& data) {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end() && it->second) {
            fd = it->second->socketFd;
        }
    }
    if (fd < 0) return false;
    return InternalSendRaw(fd, data.data(), data.size());
}

bool SocketServer::SendToClient(int clientId, const std::string& rawPayload) {
    int fd = -1;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        auto it = m_clients.find(clientId);
        if (it != m_clients.end() && it->second) {
            fd = it->second->socketFd;
        }
    }
    if (fd < 0) return false;
    return InternalSendRaw(fd, reinterpret_cast<const uint8_t*>(rawPayload.data()), rawPayload.size());
}

bool SocketServer::Broadcast(const Message& message) {
    std::vector<uint8_t> payload = message.Serialize();
    return Broadcast(payload);
}

bool SocketServer::Broadcast(const std::vector<uint8_t>& data) {
    std::vector<std::shared_ptr<ClientConnection>> clientList;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        clientList.reserve(m_clients.size());
        for (const auto& pair : m_clients) {
            clientList.push_back(pair.second);
        }
    }

    bool allSuccess = true;
    for (const auto& client : clientList) {
        if (client && client->socketFd >= 0) {
            if (!InternalSendRaw(client->socketFd, data.data(), data.size())) {
                allSuccess = false;
            }
        }
    }
    return allSuccess;
}

bool SocketServer::Broadcast(const std::string& rawPayload) {
    return Broadcast(std::vector<uint8_t>(rawPayload.begin(), rawPayload.end()));
}

void SocketServer::DisconnectClient(int clientId) {
    CloseClientInternal(clientId);
    WakeupEventLoop();
}

size_t SocketServer::GetClientCount() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    return m_clients.size();
}

std::vector<int> SocketServer::GetConnectedClientIds() const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    std::vector<int> ids;
    ids.reserve(m_clients.size());
    for (const auto& pair : m_clients) {
        ids.push_back(pair.first);
    }
    return ids;
}

bool SocketServer::GetClientInfo(int clientId, std::string& outIp, uint16_t& outPort) const {
    std::lock_guard<std::mutex> lock(m_clientsMutex);
    auto it = m_clients.find(clientId);
    if (it != m_clients.end() && it->second) {
        outIp = it->second->ipAddress;
        outPort = it->second->port;
        return true;
    }
    return false;
}

void SocketServer::SetMessageHandler(MessageHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_onMessage = std::move(handler);
}

void SocketServer::SetConnectHandler(ConnectHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_onConnect = std::move(handler);
}

void SocketServer::SetDisconnectHandler(DisconnectHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_onDisconnect = std::move(handler);
}

void SocketServer::SetErrorHandler(ErrorHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_onError = std::move(handler);
}

} // namespace communication
} // namespace aegis
