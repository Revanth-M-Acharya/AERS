/**
 * @file tcp_server.cpp
 * @brief QNX Network Communication Layer - Multi-Client TCP Server Implementation.
 */

#include "tcp_server.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
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

uint64_t TcpServer::GetCurrentTimeMs() noexcept {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

void TcpServer::SetThreadName(const std::string& name) {
#if defined(__QNXNTO__) || defined(__linux__)
    pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#elif defined(__APPLE__)
    pthread_setname_np(name.substr(0, 15).c_str());
#endif
}

TcpServer::TcpServer() {
    std::signal(SIGPIPE, SIG_IGN);
}

TcpServer::~TcpServer() {
    Stop();
}

bool TcpServer::Bind(uint16_t port, const std::string& host) {
    if (isRunning_.load()) {
        std::cerr << "[NET_SERVER] [ERROR] Cannot bind while server is running.\n";
        return false;
    }

    port_ = port;
    host_ = host;

    serverSocketFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketFd_ < 0) {
        std::cerr << "[NET_SERVER] [ERROR] Failed to create socket: " << strerror(errno) << "\n";
        return false;
    }

    // Set SO_REUSEADDR and SO_REUSEPORT to enable immediate reuse across restarts
    int opt = 1;
    if (::setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[NET_SERVER] [WARN] setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << "\n";
    }
#ifdef SO_REUSEPORT
    if (::setsockopt(serverSocketFd_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "[NET_SERVER] [WARN] setsockopt(SO_REUSEPORT) failed: " << strerror(errno) << "\n";
    }
#endif

    struct sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);

    if (host_ == "0.0.0.0" || host_.empty()) {
        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (::inet_pton(AF_INET, host_.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "[NET_SERVER] [ERROR] Invalid bind address: " << host_ << "\n";
            ::close(serverSocketFd_);
            serverSocketFd_ = -1;
            return false;
        }
    }

    if (::bind(serverSocketFd_, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "[NET_SERVER] [ERROR] bind() failed on " << host_ << ":" << port_
                  << ": " << strerror(errno) << "\n";
        ::close(serverSocketFd_);
        serverSocketFd_ = -1;
        return false;
    }

    std::cout << "[NET_SERVER] Bound to " << host_ << ":" << port_ << "\n";
    return true;
}

bool TcpServer::Listen(int backlog) {
    if (serverSocketFd_ < 0) {
        std::cerr << "[NET_SERVER] [ERROR] Cannot listen: socket not bound.\n";
        return false;
    }

    if (::listen(serverSocketFd_, backlog) < 0) {
        std::cerr << "[NET_SERVER] [ERROR] listen() failed: " << strerror(errno) << "\n";
        ::close(serverSocketFd_);
        serverSocketFd_ = -1;
        return false;
    }

    std::cout << "[NET_SERVER] Listening on " << host_ << ":" << port_
              << " with backlog " << backlog << "\n";
    return true;
}

bool TcpServer::Start(uint16_t port, const std::string& host, int backlog) {
    if (isRunning_.load()) {
        std::cerr << "[NET_SERVER] [WARN] Server already running.\n";
        return true;
    }

    if (!Bind(port, host)) {
        return false;
    }

    if (!Listen(backlog)) {
        return false;
    }

    isRunning_.store(true);

    // Spawn native accept and supervisor threads
    acceptThread_ = std::thread(&TcpServer::AcceptLoop, this);
    supervisorThread_ = std::thread(&TcpServer::SupervisorLoop, this);

    std::cout << "[NET_SERVER] Server started successfully on " << host_ << ":" << port_ << "\n";
    return true;
}

void TcpServer::Stop() {
    if (!isRunning_.exchange(false)) {
        return;
    }

    std::cout << "[NET_SERVER] Stopping server on " << host_ << ":" << port_ << "...\n";

    // Close listening socket to unblock accept()
    if (serverSocketFd_ >= 0) {
        ::shutdown(serverSocketFd_, SHUT_RDWR);
        ::close(serverSocketFd_);
        serverSocketFd_ = -1;
    }

    // Wake and disconnect all clients
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        for (auto& pair : clientsByFd_) {
            if (pair.second) {
                pair.second->isAlive.store(false);
                if (pair.second->socketFd >= 0) {
                    ::shutdown(pair.second->socketFd, SHUT_RDWR);
                    ::close(pair.second->socketFd);
                    pair.second->socketFd = -1;
                }
            }
        }
        clientsByFd_.clear();
        nodeToFdMap_.clear();
    }

    // Join accept and supervisor threads
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    if (supervisorThread_.joinable()) {
        supervisorThread_.join();
    }

    // Join all client worker threads
    {
        std::lock_guard<std::mutex> lock(threadsMutex_);
        for (auto& t : clientWorkerThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }
        clientWorkerThreads_.clear();
    }

    std::cout << "[NET_SERVER] Server stopped.\n";
}

std::string TcpServer::GetHost() const {
    return host_;
}

void TcpServer::AcceptLoop() {
    SetThreadName("aegis_tcp_acc");

    while (isRunning_.load()) {
        struct pollfd pfd{};
        pfd.fd = serverSocketFd_;
        pfd.events = POLLIN;

        int pollRes = ::poll(&pfd, 1, 200); // 200ms timeout
        if (pollRes < 0) {
            if (errno == EINTR) continue;
            if (!isRunning_.load()) break;
            std::cerr << "[NET_SERVER] [ERROR] poll() on server socket failed: " << strerror(errno) << "\n";
            break;
        }

        if (pollRes == 0) {
            // Periodic idle timeout, re-check isRunning_
            continue;
        }

        if (pfd.revents & POLLIN) {
            struct sockaddr_in clientAddr{};
            socklen_t addrLen = sizeof(clientAddr);

            int clientFd = ::accept(serverSocketFd_, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrLen);
            if (clientFd < 0) {
                if (!isRunning_.load()) break;
                if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
                std::cerr << "[NET_SERVER] [ERROR] accept() failed: " << strerror(errno) << "\n";
                continue;
            }

            // Extract remote client IP & Port
            char ipStr[INET_ADDRSTRLEN];
            ::inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, sizeof(ipStr));
            const uint16_t clientPort = ntohs(clientAddr.sin_port);

            // Configure socket timeout (500 ms) to avoid blocking threads indefinitely
            struct timeval tv{};
            tv.tv_sec = 0;
            tv.tv_usec = 500000;
            ::setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            ::setsockopt(clientFd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

            // Enable TCP_NODELAY for emergency preemption low-latency communication
            int nodelay = 1;
            ::setsockopt(clientFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

#ifdef SO_NOSIGPIPE
            int noSigpipe = 1;
            ::setsockopt(clientFd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(noSigpipe));
#endif

            auto session = std::make_shared<ClientSession>();
            session->socketFd = clientFd;
            session->ipAddress = ipStr;
            session->port = clientPort;
            session->connectedAtMs = GetCurrentTimeMs();
            session->lastHeartbeatMs = session->connectedAtMs;
            session->isAlive.store(true);

            {
                std::lock_guard<std::mutex> lock(clientsMutex_);
                clientsByFd_[clientFd] = session;
            }

            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                stats_.totalConnectionsAccepted++;
                stats_.currentConnections = clientsByFd_.size();
            }

            // Log connection per requirement
            std::cout << "[NET_SERVER] [CONNECT] Client connected on fd: " << clientFd
                      << " from " << ipStr << ":" << clientPort << "\n";

            // Spawn dedicated QNX native thread for this client session
            std::lock_guard<std::mutex> tLock(threadsMutex_);
            CleanupInactiveClients();
            clientWorkerThreads_.emplace_back(&TcpServer::HandleClient, this, session);
        }
    }
}

void TcpServer::CleanupInactiveClients() {
    // Collect joinable threads from closed client connections
    auto it = std::remove_if(clientWorkerThreads_.begin(), clientWorkerThreads_.end(),
                             [](std::thread& t) {
                                 if (!t.joinable()) return true;
                                 return false;
                             });
    clientWorkerThreads_.erase(it, clientWorkerThreads_.end());
}

void TcpServer::HandleClient(std::shared_ptr<ClientSession> client) {
    if (!client) return;
    SetThreadName("aegis_cli_" + std::to_string(client->socketFd));

    uint8_t readBuffer[4096];

    while (isRunning_.load() && client->isAlive.load()) {
        ssize_t bytesRead = ::recv(client->socketFd, readBuffer, sizeof(readBuffer), 0);

        if (bytesRead > 0) {
            client->rxBuffer.insert(client->rxBuffer.end(), readBuffer, readBuffer + bytesRead);

            {
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                stats_.bytesReceived += static_cast<uint64_t>(bytesRead);
            }

            // Parse framed messages from TCP stream buffer
            size_t bytesConsumed = 0;
            Message msg;
            while (Message::Deserialize(client->rxBuffer, msg, bytesConsumed)) {
                // Erase parsed packet from stream buffer
                client->rxBuffer.erase(client->rxBuffer.begin(),
                                       client->rxBuffer.begin() + static_cast<ptrdiff_t>(bytesConsumed));

                const uint64_t nowMs = GetCurrentTimeMs();
                client->lastHeartbeatMs = nowMs;

                {
                    std::lock_guard<std::mutex> lock(statisticsMutex_);
                    stats_.messagesReceived++;
                }

                const std::string msgNodeId = msg.GetNodeId();

                // Node identification & Reconnection handling
                bool wasReconnected = false;
                bool isNewRegistration = false;
                {
                    std::lock_guard<std::mutex> lock(clientsMutex_);
                    if (!msgNodeId.empty() && client->nodeId.empty()) {
                        client->nodeId = msgNodeId;

                        auto existingIt = nodeToFdMap_.find(msgNodeId);
                        if (existingIt != nodeToFdMap_.end() && existingIt->second != client->socketFd) {
                            // Node reconnected! Graceful replacement of dead/old socket
                            wasReconnected = true;
                            const int oldFd = existingIt->second;
                            auto oldSessionIt = clientsByFd_.find(oldFd);
                            if (oldSessionIt != clientsByFd_.end()) {
                                oldSessionIt->second->isAlive.store(false);
                                ::shutdown(oldFd, SHUT_RDWR);
                                ::close(oldFd);
                                clientsByFd_.erase(oldSessionIt);
                            }
                            nodeToFdMap_[msgNodeId] = client->socketFd;
                        } else {
                            nodeToFdMap_[msgNodeId] = client->socketFd;
                            isNewRegistration = true;
                        }
                    }
                }

                // Log reconnect or registration
                if (wasReconnected) {
                    std::cout << "[NET_SERVER] [RECONNECT] Node reconnected: " << client->nodeId
                              << " on fd " << client->socketFd << "\n";
                } else if (isNewRegistration) {
                    std::cout << "[NET_SERVER] [REGISTER] Node registered: " << client->nodeId
                              << " from " << client->ipAddress << ":" << client->port << "\n";

                    std::lock_guard<std::mutex> cbLock(callbackMutex_);
                    if (onConnect_) {
                        onConnect_(client->nodeId, client->ipAddress, client->port);
                    }
                }

                // Log every message per requirement
                if (msg.GetType() == MessageType::HEARTBEAT) {
                    {
                        std::lock_guard<std::mutex> lock(statisticsMutex_);
                        stats_.heartbeatsReceived++;
                    }
                    std::cout << "[NET_SERVER] [RX] [HEARTBEAT] Node: " << client->nodeId << "\n";
                } else {
                    std::cout << "[NET_SERVER] [RX] " << msg.GetFormattedLog() << "\n";
                }

                // Dispatch message callback
                {
                    std::lock_guard<std::mutex> cbLock(callbackMutex_);
                    if (onMessage_) {
                        onMessage_(client->nodeId, msg);
                    }
                }
            }

            if (bytesConsumed > 0 && !client->rxBuffer.empty()) {
                // If bytes were consumed due to corruption (magic byte alignment recovery)
                client->rxBuffer.erase(client->rxBuffer.begin(),
                                       client->rxBuffer.begin() + static_cast<ptrdiff_t>(bytesConsumed));
                std::lock_guard<std::mutex> lock(statisticsMutex_);
                stats_.invalidPackets++;
            }
        } else if (bytesRead == 0) {
            // Client gracefully closed socket
            std::cout << "[NET_SERVER] [DISCONNECT] Client closed socket fd: " << client->socketFd
                      << ", Node: " << (client->nodeId.empty() ? "UNREGISTERED" : client->nodeId) << "\n";
            client->isAlive.store(false);
            break;
        } else {
            // bytesRead < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                // Normal read timeout, keep checking isRunning_ & isAlive
                continue;
            }

            if (!isRunning_.load() || !client->isAlive.load()) {
                break;
            }

            // Socket broken / reset by peer
            std::cout << "[NET_SERVER] [DISCONNECT] Socket error on fd: " << client->socketFd
                      << " (" << strerror(errno) << "), Node: "
                      << (client->nodeId.empty() ? "UNREGISTERED" : client->nodeId) << "\n";
            client->isAlive.store(false);
            break;
        }
    }

    // Cleanup client on thread exit
    std::string disconnectedNodeId;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        disconnectedNodeId = client->nodeId;
        const int fd = client->socketFd;
        if (fd >= 0) {
            ::close(fd);
            client->socketFd = -1;
            clientsByFd_.erase(fd);
        }
        if (!disconnectedNodeId.empty()) {
            auto it = nodeToFdMap_.find(disconnectedNodeId);
            if (it != nodeToFdMap_.end() && it->second == fd) {
                nodeToFdMap_.erase(it);
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats_.currentConnections = clientsByFd_.size();
    }

    if (!disconnectedNodeId.empty()) {
        std::lock_guard<std::mutex> cbLock(callbackMutex_);
        if (onDisconnect_) {
            onDisconnect_(disconnectedNodeId);
        }
    }
}

void TcpServer::SupervisorLoop() {
    SetThreadName("aegis_tcp_sup");

    while (isRunning_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        if (!isRunning_.load()) break;

        const uint64_t nowMs = GetCurrentTimeMs();
        std::vector<std::string> timedOutNodes;

        {
            std::lock_guard<std::mutex> lock(clientsMutex_);
            for (auto it = clientsByFd_.begin(); it != clientsByFd_.end();) {
                const auto& session = it->second;
                if (!session) {
                    it = clientsByFd_.erase(it);
                    continue;
                }

                if (session->isAlive.load()) {
                    const uint64_t elapsed = nowMs - session->lastHeartbeatMs;
                    if (elapsed > heartbeatTimeoutMs_) {
                        // Node heartbeat timed out (> 5 seconds)!
                        std::cout << "[NET_SERVER] [TIMEOUT] Node: "
                                  << (session->nodeId.empty() ? "UNKNOWN" : session->nodeId)
                                  << " heartbeat timeout exceeded 5 seconds (elapsed: "
                                  << elapsed << "ms). Removing node.\n";

                        session->isAlive.store(false);
                        if (session->socketFd >= 0) {
                            ::shutdown(session->socketFd, SHUT_RDWR);
                            ::close(session->socketFd);
                            session->socketFd = -1;
                        }

                        if (!session->nodeId.empty()) {
                            timedOutNodes.push_back(session->nodeId);
                            nodeToFdMap_.erase(session->nodeId);
                        }

                        it = clientsByFd_.erase(it);

                        std::lock_guard<std::mutex> sLock(statisticsMutex_);
                        stats_.timeoutDisconnections++;
                        continue;
                    }
                }
                ++it;
            }
        }

        // Notify disconnect callbacks outside mutex
        for (const auto& nodeId : timedOutNodes) {
            std::lock_guard<std::mutex> cbLock(callbackMutex_);
            if (onDisconnect_) {
                onDisconnect_(nodeId);
            }
        }
    }
}

bool TcpServer::Send(int socketFd, const Message& msg) {
    if (socketFd < 0 || !isRunning_.load()) {
        return false;
    }

    const std::vector<uint8_t> packet = msg.Serialize();

    ssize_t totalSent = 0;
    const ssize_t packetSize = static_cast<ssize_t>(packet.size());

    while (totalSent < packetSize && isRunning_.load()) {
        ssize_t sent = ::send(socketFd, packet.data() + totalSent,
                              static_cast<size_t>(packetSize - totalSent), MSG_NOSIGNAL);
        if (sent <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                continue;
            }
            std::cerr << "[NET_SERVER] [ERROR] send() failed on socket fd: " << socketFd
                      << ": " << strerror(errno) << "\n";
            return false;
        }
        totalSent += sent;
    }

    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        stats_.messagesSent++;
        stats_.bytesSent += static_cast<uint64_t>(totalSent);
    }

    // Log message TX per requirement
    std::cout << "[NET_SERVER] [TX] " << msg.GetFormattedLog() << "\n";
    return true;
}

bool TcpServer::Send(const std::string& nodeId, const Message& msg) {
    int targetFd = -1;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = nodeToFdMap_.find(nodeId);
        if (it != nodeToFdMap_.end()) {
            targetFd = it->second;
        }
    }

    if (targetFd < 0) {
        std::cerr << "[NET_SERVER] [ERROR] Node not connected: " << nodeId << "\n";
        return false;
    }

    return Send(targetFd, msg);
}

size_t TcpServer::Broadcast(const Message& msg) {
    std::vector<int> targetFds;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        targetFds.reserve(clientsByFd_.size());
        for (const auto& pair : clientsByFd_) {
            if (pair.second && pair.second->isAlive.load() && pair.second->socketFd >= 0) {
                targetFds.push_back(pair.second->socketFd);
            }
        }
    }

    size_t successfulCount = 0;
    for (int fd : targetFds) {
        if (Send(fd, msg)) {
            successfulCount++;
        }
    }

    return successfulCount;
}

bool TcpServer::IsNodeConnected(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    auto it = nodeToFdMap_.find(nodeId);
    if (it == nodeToFdMap_.end()) {
        return false;
    }
    auto sessionIt = clientsByFd_.find(it->second);
    return (sessionIt != clientsByFd_.end() && sessionIt->second->isAlive.load());
}

std::vector<std::string> TcpServer::GetConnectedNodes() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    std::vector<std::string> nodes;
    nodes.reserve(nodeToFdMap_.size());
    for (const auto& pair : nodeToFdMap_) {
        nodes.push_back(pair.first);
    }
    std::sort(nodes.begin(), nodes.end());
    return nodes;
}

size_t TcpServer::GetClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clientsByFd_.size();
}

void TcpServer::RemoveNode(const std::string& nodeId) {
    int targetFd = -1;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        auto it = nodeToFdMap_.find(nodeId);
        if (it != nodeToFdMap_.end()) {
            targetFd = it->second;
            nodeToFdMap_.erase(it);
        }
        if (targetFd >= 0) {
            auto sessionIt = clientsByFd_.find(targetFd);
            if (sessionIt != clientsByFd_.end()) {
                sessionIt->second->isAlive.store(false);
                ::shutdown(targetFd, SHUT_RDWR);
                ::close(targetFd);
                clientsByFd_.erase(sessionIt);
            }
        }
    }

    if (targetFd >= 0) {
        std::cout << "[NET_SERVER] [DISCONNECT] Node manually removed: " << nodeId << "\n";
        std::lock_guard<std::mutex> cbLock(callbackMutex_);
        if (onDisconnect_) {
            onDisconnect_(nodeId);
        }
    }
}

void TcpServer::SetOnMessage(MessageCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onMessage_ = std::move(cb);
}

void TcpServer::SetOnConnect(ConnectCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onConnect_ = std::move(cb);
}

void TcpServer::SetOnDisconnect(DisconnectCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onDisconnect_ = std::move(cb);
}

void TcpServer::SetOnError(ErrorCallback cb) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    onError_ = std::move(cb);
}

void TcpServer::SetHeartbeatTimeoutMs(uint64_t timeoutMs) noexcept {
    heartbeatTimeoutMs_ = timeoutMs;
}

ServerStatistics TcpServer::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    ServerStatistics copy = stats_;
    std::lock_guard<std::mutex> cLock(clientsMutex_);
    copy.currentConnections = clientsByFd_.size();
    return copy;
}

void TcpServer::ResetStatistics() {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    stats_ = ServerStatistics();
}

std::string TcpServer::GenerateStatusReport() const {
    ServerStatistics stats = GetStatistics();
    auto nodes = GetConnectedNodes();

    std::ostringstream ss;
    ss << "==========================================================\n"
       << " QNX TCP Server Status Report                             \n"
       << "==========================================================\n"
       << " Host: " << host_ << ":" << port_ << "\n"
       << " Running: " << (IsRunning() ? "YES" : "NO") << "\n"
       << " Connected Nodes (" << nodes.size() << "):\n";
    for (const auto& n : nodes) {
        ss << "   - " << n << "\n";
    }
    ss << " Total Connections Accepted: " << stats.totalConnectionsAccepted << "\n"
       << " Messages Received: " << stats.messagesReceived << "\n"
       << " Messages Sent: " << stats.messagesSent << "\n"
       << " Heartbeats Received: " << stats.heartbeatsReceived << "\n"
       << " Timeout Disconnections: " << stats.timeoutDisconnections << "\n"
       << " Checksum Failures: " << stats.checksumFailures << "\n"
       << " Invalid Packets: " << stats.invalidPackets << "\n"
       << "==========================================================\n";
    return ss.str();
}

} // namespace network
} // namespace aegis
