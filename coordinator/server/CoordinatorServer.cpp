/**
 * @file CoordinatorServer.cpp
 * @brief Implementation of production-grade Coordinator TCP Server.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/server/CoordinatorServer.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/network/SocketUtils.hpp"
#include "common/utils/ThreadUtils.hpp"
#include "common/utils/TimeUtils.hpp"
#include <unistd.h>
#include <poll.h>
#include <algorithm>
#include <sstream>

namespace aegis::coordinator {

CoordinatorServer::CoordinatorServer()
    : CoordinatorServer(config::ServerConfig{}) {}

CoordinatorServer::CoordinatorServer(const config::ServerConfig& config)
    : config_(config) {}

CoordinatorServer::~CoordinatorServer() {
    Stop();
}

void CoordinatorServer::SetConfig(const config::ServerConfig& config) {
    if (!isRunning_.load()) {
        config_ = config;
    }
}

bool CoordinatorServer::Start() {
    if (isRunning_.load()) {
        return true;
    }

    CoordinatorLogger::Instance().Initialize(config_.logFilePath, config_.logToConsole);
    CoordinatorLogger::Instance().Info("COORD_SRV", "Starting Coordinator TCP Server...");
    CoordinatorLogger::Instance().Info("COORD_SRV", config_.ToString());

    if (!listeningSocket_.Create()) {
        CoordinatorLogger::Instance().Error("COORD_SRV", "Failed to create listening socket.");
        return false;
    }

    if (!listeningSocket_.SetReuseAddress(true)) {
        CoordinatorLogger::Instance().Warn("COORD_SRV", "Failed to set SO_REUSEADDR on listening socket.");
    }

    if (!listeningSocket_.SetNonBlocking(true)) {
        CoordinatorLogger::Instance().Error("COORD_SRV", "Failed to set non-blocking mode on listening socket.");
        listeningSocket_.Close();
        return false;
    }

    if (!listeningSocket_.Bind(config_.bindAddress, config_.port)) {
        std::ostringstream oss;
        oss << "Failed to bind to " << config_.bindAddress << ":" << config_.port;
        CoordinatorLogger::Instance().Error("COORD_SRV", oss.str());
        listeningSocket_.Close();
        return false;
    }

    if (!listeningSocket_.Listen(config_.listenBacklog)) {
        CoordinatorLogger::Instance().Error("COORD_SRV", "Failed to listen on socket.");
        listeningSocket_.Close();
        return false;
    }

    isRunning_.store(true);

    // Launch worker threads
    acceptThread_ = std::thread(&CoordinatorServer::AcceptLoop, this);
    ioWorkerThread_ = std::thread(&CoordinatorServer::IoWorkerLoop, this);

    std::ostringstream oss;
    oss << "Coordinator TCP Server successfully listening on "
        << config_.bindAddress << ":" << config_.port;
    CoordinatorLogger::Instance().Info("COORD_SRV", oss.str());

    return true;
}

void CoordinatorServer::Stop() {
    if (!isRunning_.exchange(false)) {
        return;
    }

    CoordinatorLogger::Instance().Info("COORD_SRV", "Initiating graceful shutdown...");

    // Close listening socket to unblock accept poll
    listeningSocket_.Close();

    // Close all client sockets
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (auto& pair : sessionsByFd_) {
            pair.second->Close();
        }
    }

    // Join threads
    if (acceptThread_.joinable()) {
        acceptThread_.join();
    }
    if (ioWorkerThread_.joinable()) {
        ioWorkerThread_.join();
    }

    // Clear session containers
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        sessionsByFd_.clear();
        registeredNodes_.clear();
    }

    CoordinatorLogger::Instance().Info("COORD_SRV", "Coordinator TCP Server stopped successfully.");
}

bool CoordinatorServer::SendToNode(const std::string& nodeId, const protocol::Packet& packet) {
    std::shared_ptr<ClientSession> session;
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto it = registeredNodes_.find(nodeId);
        if (it != registeredNodes_.end()) {
            session = it->second;
        }
    }

    if (!session) {
        std::ostringstream oss;
        oss << "Cannot send packet to node " << nodeId << ": Node not found or unregistered.";
        CoordinatorLogger::Instance().Warn("COORD_SRV", oss.str());
        return false;
    }

    return session->SendPacket(packet);
}

size_t CoordinatorServer::Broadcast(const protocol::Packet& packet) {
    std::vector<std::shared_ptr<ClientSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        sessions.reserve(registeredNodes_.size());
        for (const auto& pair : registeredNodes_) {
            sessions.push_back(pair.second);
        }
    }

    size_t successfulSends = 0;
    for (const auto& session : sessions) {
        if (session->SendPacket(packet)) {
            ++successfulSends;
        }
    }
    return successfulSends;
}

void CoordinatorServer::DisconnectNode(const std::string& nodeId, const std::string& reason) {
    int fdToClose = -1;
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto it = registeredNodes_.find(nodeId);
        if (it != registeredNodes_.end()) {
            fdToClose = it->second->GetFd();
        }
    }

    if (fdToClose >= 0) {
        RemoveSession(fdToClose, reason);
    }
}

std::vector<std::string> CoordinatorServer::GetRegisteredNodeIds() const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    std::vector<std::string> nodeIds;
    nodeIds.reserve(registeredNodes_.size());
    for (const auto& pair : registeredNodes_) {
        nodeIds.push_back(pair.first);
    }
    return nodeIds;
}

size_t CoordinatorServer::GetRegisteredNodeCount() const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return registeredNodes_.size();
}

bool CoordinatorServer::IsNodeRegistered(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(sessionMutex_);
    return (registeredNodes_.find(nodeId) != registeredNodes_.end());
}

void CoordinatorServer::AcceptLoop() {
    common::ThreadUtils::SetCurrentThreadName("CoordAcceptor");
    common::ThreadUtils::SetRealtimePriority(pthread_self(), SCHED_FIFO, config_.workerThreadPriority);

    while (isRunning_.load()) {
        int listenFd = listeningSocket_.GetFd();
        if (listenFd < 0) {
            break;
        }

        struct pollfd pfd{};
        pfd.fd = listenFd;
        pfd.events = POLLIN;

        int ret = ::poll(&pfd, 1, 100); // 100ms timeout
        if (ret < 0) {
            if (errno == EINTR) continue;
            break;
        } else if (ret == 0) {
            continue; // Poll timeout
        }

        if (pfd.revents & POLLIN) {
            std::string clientIp;
            uint16_t clientPort = 0;
            int clientFd = listeningSocket_.Accept(clientIp, clientPort);
            if (clientFd >= 0) {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                if (sessionsByFd_.size() >= config_.maxClients) {
                    CoordinatorLogger::Instance().Warn("COORD_NET", "Max client limit reached. Rejecting connection.");
                    ::close(clientFd);
                    continue;
                }

                auto session = std::make_shared<ClientSession>(clientFd, clientIp, clientPort);
                sessionsByFd_[clientFd] = session;
                CoordinatorLogger::Instance().LogConnect(clientFd, clientIp, clientPort);
            }
        }
    }
}

void CoordinatorServer::IoWorkerLoop() {
    common::ThreadUtils::SetCurrentThreadName("CoordIoWorker");
    common::ThreadUtils::SetRealtimePriority(pthread_self(), SCHED_FIFO, config_.workerThreadPriority);

    uint64_t lastTimeoutCheckMs = common::TimeUtils::GetMonotonicTimeMs();

    while (isRunning_.load()) {
        std::vector<struct pollfd> pollFds;
        std::vector<std::shared_ptr<ClientSession>> currentSessions;

        {
            std::lock_guard<std::mutex> lock(sessionMutex_);
            pollFds.reserve(sessionsByFd_.size());
            currentSessions.reserve(sessionsByFd_.size());

            for (const auto& pair : sessionsByFd_) {
                struct pollfd pfd{};
                pfd.fd = pair.first;
                pfd.events = POLLIN;
                pollFds.push_back(pfd);
                currentSessions.push_back(pair.second);
            }
        }

        if (pollFds.empty()) {
            common::TimeUtils::SleepMs(config_.pollTimeoutMs);
            continue;
        }

        int pollRet = ::poll(pollFds.data(), static_cast<nfds_t>(pollFds.size()), static_cast<int>(config_.pollTimeoutMs));
        if (pollRet < 0) {
            if (errno == EINTR) continue;
            break;
        }

        std::vector<std::pair<int, std::string>> sessionsToDisconnect;

        for (size_t i = 0; i < pollFds.size(); ++i) {
            int fd = pollFds[i].fd;
            short revents = pollFds[i].revents;
            const auto& session = currentSessions[i];

            if (revents & (POLLHUP | POLLERR | POLLNVAL)) {
                sessionsToDisconnect.emplace_back(fd, "Socket hangup or error");
                continue;
            }

            if (revents & POLLIN) {
                ssize_t bytesRead = session->ReadAvailable();
                if (bytesRead == 0) {
                    sessionsToDisconnect.emplace_back(fd, "Remote peer disconnected");
                } else if (bytesRead > 0) {
                    std::vector<protocol::Packet> packets;
                    if (session->ExtractPackets(packets)) {
                        for (const auto& pkt : packets) {
                            HandleIncomingPacket(session, pkt);
                        }
                    }
                } else if (bytesRead == -1) {
                    sessionsToDisconnect.emplace_back(fd, "Read error: " + network::SocketUtils::GetLastSocketErrorString());
                }
            }
        }

        // Process disconnections
        for (const auto& item : sessionsToDisconnect) {
            RemoveSession(item.first, item.second);
        }

        // Periodic heartbeat timeout check
        uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
        if (nowMono - lastTimeoutCheckMs >= 1000) {
            lastTimeoutCheckMs = nowMono;
            CheckHeartbeatTimeouts();
        }
    }
}

void CoordinatorServer::HandleIncomingPacket(const std::shared_ptr<ClientSession>& session,
                                            const protocol::Packet& packet) {
    session->UpdateHeartbeat();
    std::string nodeId = packet.GetNodeId();

    // Check if session needs registration
    if (!session->IsRegistered() || session->GetNodeId().empty()) {
        if (!nodeId.empty()) {
            session->SetNodeId(nodeId);
            session->SetRegistered(true);

            {
                std::lock_guard<std::mutex> lock(sessionMutex_);
                registeredNodes_[nodeId] = session;
            }

            CoordinatorLogger::Instance().LogRegister(nodeId, session->GetIp(), session->GetPort(), session->GetFd());

            // Automatic ACK response confirming registration to Traffic Node
            protocol::Packet ackPkt(protocol::MessageType::ACK, "COORDINATOR", "REGISTERED: " + nodeId);
            session->SendPacket(ackPkt);

            if (connectCallback_) {
                connectCallback_(nodeId, session->GetIp(), session->GetPort());
            }
        }
    }

    std::string activeNodeId = session->GetNodeId();
    if (activeNodeId.empty()) {
        activeNodeId = session->GetIp();
    }

    CoordinatorLogger::Instance().LogPacketRx(activeNodeId, packet);

    // If client sent HEARTBEAT, send immediate ACK
    if (packet.GetType() == protocol::MessageType::HEARTBEAT) {
        protocol::Packet ackPkt(protocol::MessageType::ACK, "COORDINATOR", "HEARTBEAT_ACK");
        session->SendPacket(ackPkt);
    }

    // Forward to general protocol callback
    if (packetCallback_) {
        packetCallback_(activeNodeId, packet);
    }
}

void CoordinatorServer::RemoveSession(int fd, const std::string& reason) {
    std::string nodeId;
    std::shared_ptr<ClientSession> session;

    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        auto it = sessionsByFd_.find(fd);
        if (it != sessionsByFd_.end()) {
            session = it->second;
            nodeId = session->GetNodeId();
            sessionsByFd_.erase(it);
        }
        if (!nodeId.empty()) {
            registeredNodes_.erase(nodeId);
        }
    }

    if (session) {
        session->Close();
        CoordinatorLogger::Instance().LogDisconnect(nodeId, fd, reason);

        if (!nodeId.empty() && disconnectCallback_) {
            disconnectCallback_(nodeId, reason);
        }
    }
}

void CoordinatorServer::CheckHeartbeatTimeouts() {
    std::vector<std::pair<int, std::string>> timedOutSessions;

    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        for (const auto& pair : sessionsByFd_) {
            const auto& session = pair.second;
            if (session->IsRegistered() && session->IsTimedOut(config_.heartbeatTimeoutMs)) {
                uint64_t elapsed = common::TimeUtils::GetMonotonicTimeMs() - session->GetLastHeartbeatMs();
                CoordinatorLogger::Instance().LogTimeout(session->GetNodeId(), elapsed, config_.heartbeatTimeoutMs);
                timedOutSessions.emplace_back(pair.first, "Heartbeat timeout expired");
            }
        }
    }

    for (const auto& item : timedOutSessions) {
        RemoveSession(item.first, item.second);
    }
}

} // namespace aegis::coordinator
