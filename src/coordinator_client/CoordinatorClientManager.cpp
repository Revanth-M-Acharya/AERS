/**
 * @file CoordinatorClientManager.cpp
 * @brief Implementation of SMART AEGIS Central Coordinator TCP Client Engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator_client/CoordinatorClientManager.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <fcntl.h>
#include <sys/select.h>

namespace aegis::coordinator {

CoordinatorClientManager::CoordinatorClientManager(std::shared_ptr<config::IntersectionsConfig> config)
    : config_(std::move(config)) {
    if (!config_) {
        config_ = config::IntersectionsConfig::LoadFromFile("config/intersections.json");
        if (!config_ || !config_->IsValid()) {
            config_ = config::IntersectionsConfig::CreateDefaultCluster();
        }
    }

    const auto& settings = config_->GetSystemSettings();
    reconnectIntervalMs_ = settings.reconnectIntervalMs;
    pingIntervalMs_ = settings.pingIntervalMs;
    maxRetries_ = settings.maxRetries;

    for (const auto& ic : config_->GetIntersections()) {
        auto session = std::make_unique<NodeSession>();
        session->id = ic.id;
        session->ip = ic.ip;
        session->port = ic.port;
        session->hasRedLed = ic.gpio.hasRed;
        session->hasBlueLed = ic.gpio.hasBlue;
        nodes_[ic.id] = std::move(session);
    }
}

CoordinatorClientManager::~CoordinatorClientManager() {
    Stop();
}

bool CoordinatorClientManager::Start() {
    if (running_.load()) {
        return true;
    }

#if defined(SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
#endif

    running_.store(true);
    std::cout << "[COORDINATOR] Starting Coordinator Client Manager (Cluster Nodes: "
              << nodes_.size() << ", Reconnect: " << reconnectIntervalMs_
              << "ms, Ping: " << pingIntervalMs_ << "ms, Retries: " << maxRetries_ << ")" << std::endl;

    connectionThread_ = std::thread(&CoordinatorClientManager::ConnectionWorker, this);
    healthThread_ = std::thread(&CoordinatorClientManager::HealthMonitorWorker, this);

    return true;
}

void CoordinatorClientManager::Stop() {
    if (!running_.exchange(false)) {
        return;
    }

    std::cout << "[COORDINATOR] Stopping Coordinator Client Manager..." << std::endl;

    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (auto& [id, session] : nodes_) {
            std::lock_guard<std::mutex> slock(session->sessionMutex);
            DisconnectNode(*session);
        }
    }

    if (connectionThread_.joinable()) {
        connectionThread_.join();
    }
    if (healthThread_.joinable()) {
        healthThread_.join();
    }

    std::cout << "[COORDINATOR] Coordinator Client Manager stopped." << std::endl;
}

void CoordinatorClientManager::SetNodeEndpoint(int intersectionId, const std::string& ip, uint16_t port) {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = nodes_.find(intersectionId);
    if (it != nodes_.end()) {
        std::lock_guard<std::mutex> slock(it->second->sessionMutex);
        if (it->second->ip != ip || it->second->port != port) {
            DisconnectNode(*it->second);
            it->second->ip = ip;
            it->second->port = port;
            it->second->status = NodeHealthStatus::OFFLINE;
        }
    } else {
        auto session = std::make_unique<NodeSession>();
        session->id = intersectionId;
        session->ip = ip;
        session->port = port;
        session->status = NodeHealthStatus::OFFLINE;
        nodes_[intersectionId] = std::move(session);
    }
}

NodeHealthStatus CoordinatorClientManager::GetNodeStatus(int intersectionId) const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    auto it = nodes_.find(intersectionId);
    if (it != nodes_.end()) {
        return it->second->status.load();
    }
    return NodeHealthStatus::OFFLINE;
}

std::map<int, NodeHealthStatus> CoordinatorClientManager::GetAllNodeStatuses() const {
    std::lock_guard<std::mutex> lock(nodesMutex_);
    std::map<int, NodeHealthStatus> statuses;
    for (const auto& [id, session] : nodes_) {
        statuses[id] = session->status.load();
    }
    return statuses;
}

bool CoordinatorClientManager::ConnectNode(NodeSession& session) {
    if (session.socketFd >= 0) {
        DisconnectNode(session);
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set 1-second socket timeouts so connect / send / recv won't hang
    timeval tv{};
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

#ifdef SO_NOSIGPIPE
    int optNoSigpipe = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &optNoSigpipe, sizeof(optNoSigpipe));
#endif

    sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(session.port);
    if (inet_pton(AF_INET, session.ip.c_str(), &saddr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    // Use non-blocking connect with a strict 500ms timeout to avoid kernel hangs on unreachable IPs
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    int connRes = connect(sock, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    if (connRes < 0) {
        if (errno == EINPROGRESS) {
            fd_set writeFds;
            FD_ZERO(&writeFds);
            FD_SET(sock, &writeFds);

            timeval ctv{};
            ctv.tv_sec = 0;
            ctv.tv_usec = 500000; // 500ms maximum connection attempt timeout

            int selRes = select(sock + 1, nullptr, &writeFds, nullptr, &ctv);
            if (selRes > 0) {
                int sockErr = 0;
                socklen_t errLen = sizeof(sockErr);
                if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &errLen) < 0 || sockErr != 0) {
                    close(sock);
                    return false;
                }
            } else {
                // Timeout or error
                close(sock);
                return false;
            }
        } else {
            close(sock);
            return false;
        }
    }

    // Restore original blocking mode with 1-second timeout
    if (flags >= 0) {
        fcntl(sock, F_SETFL, flags);
    }

    session.socketFd = sock;
    session.rxBuffer.clear();

    // Await INTERSECTION_READY greeting
    auto readyMsg = ReceiveMessageWithTimeout(session, 1500);
    if (!readyMsg || readyMsg->command != protocol::Commands::INTERSECTION_READY) {
        DisconnectNode(session);
        return false;
    }

    // Send INITIALIZE command
    protocol::AegisMessage initMsg;
    initMsg.timestamp = protocol::AegisMessage::CurrentIsoTimestamp();
    initMsg.intersection = session.id;
    initMsg.command = std::string(protocol::Commands::INITIALIZE);
    initMsg.sequence = session.sequenceCounter.fetch_add(1);
    initMsg.status = "OK";

    if (!TransmitMessage(session, initMsg)) {
        DisconnectNode(session);
        return false;
    }

    auto ackMsg = ReceiveMessageWithTimeout(session, 1500);
    if (!ackMsg || ackMsg->command != protocol::Commands::ACK) {
        DisconnectNode(session);
        return false;
    }

    session.status = NodeHealthStatus::ONLINE;
    session.lastAck = std::chrono::steady_clock::now();
    std::cout << "[COORDINATOR] Node " << session.id << " (" << session.ip << ":"
              << session.port << ") connected & initialized -> ONLINE" << std::endl;
    return true;
}

void CoordinatorClientManager::DisconnectNode(NodeSession& session) {
    if (session.socketFd >= 0) {
        shutdown(session.socketFd, SHUT_RDWR);
        close(session.socketFd);
        session.socketFd = -1;
    }
    session.rxBuffer.clear();
}

bool CoordinatorClientManager::TransmitMessage(NodeSession& session, const protocol::AegisMessage& msg) {
    if (session.socketFd < 0) return false;
    std::string json = msg.ToJson();
#if defined(MSG_NOSIGNAL)
    int sendFlags = MSG_NOSIGNAL;
#else
    int sendFlags = 0;
#endif
    ssize_t sent = send(session.socketFd, json.data(), json.size(), sendFlags);
    return (sent == static_cast<ssize_t>(json.size()));
}

std::optional<protocol::AegisMessage> CoordinatorClientManager::ReceiveMessageWithTimeout(NodeSession& session, uint32_t timeoutMs) {
    if (session.socketFd < 0) return std::nullopt;

    auto start = std::chrono::steady_clock::now();
    char buffer[1024];

    while (running_.load()) {
        // Check if full line already in rxBuffer
        size_t newlinePos = session.rxBuffer.find('\n');
        if (newlinePos != std::string::npos) {
            std::string line = session.rxBuffer.substr(0, newlinePos);
            session.rxBuffer.erase(0, newlinePos + 1);
            if (!line.empty()) {
                return protocol::AegisMessage::FromJson(line);
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= timeoutMs) {
            return std::nullopt;
        }

        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(session.socketFd, &readFds);

        timeval tv{};
        long remaining = static_cast<long>(timeoutMs - elapsed);
        tv.tv_sec = remaining / 1000;
        tv.tv_usec = (remaining % 1000) * 1000;

        int res = select(session.socketFd + 1, &readFds, nullptr, nullptr, &tv);
        if (res <= 0) {
            return std::nullopt;
        }

        ssize_t bytes = recv(session.socketFd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            return std::nullopt;
        }

        buffer[bytes] = '\0';
        session.rxBuffer.append(buffer, static_cast<size_t>(bytes));
    }
    return std::nullopt;
}

void CoordinatorClientManager::ConnectionWorker() {
    while (running_.load()) {
        {
            std::lock_guard<std::mutex> lock(nodesMutex_);
            for (auto& [id, session] : nodes_) {
                if (session->status.load() != NodeHealthStatus::ONLINE) {
                    session->status = NodeHealthStatus::RECONNECTING;
                    std::lock_guard<std::mutex> slock(session->sessionMutex);
                    ConnectNode(*session);
                }
            }
        }

        // Sleep in small increments up to reconnectIntervalMs_
        auto sleepTime = std::chrono::milliseconds(reconnectIntervalMs_);
        auto slice = std::chrono::milliseconds(50);
        auto elapsed = std::chrono::milliseconds(0);
        while (running_.load() && elapsed < sleepTime) {
            std::this_thread::sleep_for(slice);
            elapsed += slice;
        }
    }
}

void CoordinatorClientManager::HealthMonitorWorker() {
    while (running_.load()) {
        auto sleepTime = std::chrono::milliseconds(pingIntervalMs_);
        auto slice = std::chrono::milliseconds(100);
        auto elapsed = std::chrono::milliseconds(0);
        while (running_.load() && elapsed < sleepTime) {
            std::this_thread::sleep_for(slice);
            elapsed += slice;
        }
        if (!running_.load()) break;

        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (auto& [id, session] : nodes_) {
            if (session->status.load() == NodeHealthStatus::ONLINE) {
                std::lock_guard<std::mutex> slock(session->sessionMutex);
                protocol::AegisMessage ping;
                ping.timestamp = protocol::AegisMessage::CurrentIsoTimestamp();
                ping.intersection = session->id;
                ping.command = std::string(protocol::Commands::PING);
                ping.sequence = session->sequenceCounter.fetch_add(1);
                ping.status = "OK";

                if (TransmitMessage(*session, ping)) {
                    session->lastPing = std::chrono::steady_clock::now();
                    auto ack = ReceiveMessageWithTimeout(*session, 1500);
                    if (ack && ack->command == protocol::Commands::ACK) {
                        session->lastAck = std::chrono::steady_clock::now();
                    } else {
                        std::cout << "[COORDINATOR WARN] Node " << session->id
                                  << " missed PING ACK. Marking RECONNECTING." << std::endl;
                        DisconnectNode(*session);
                        session->status = NodeHealthStatus::RECONNECTING;
                    }
                } else {
                    std::cout << "[COORDINATOR WARN] Node " << session->id
                              << " write failed on PING. Marking RECONNECTING." << std::endl;
                    DisconnectNode(*session);
                    session->status = NodeHealthStatus::RECONNECTING;
                }
            }
        }
    }
}

bool CoordinatorClientManager::SendCommandWithAck(int intersectionId,
                                                  const std::string& command,
                                                  const std::string& details,
                                                  uint32_t timeoutMs) {
    NodeSession* sessionPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        auto it = nodes_.find(intersectionId);
        if (it != nodes_.end()) {
            sessionPtr = it->second.get();
        }
    }

    if (!sessionPtr) {
        std::cerr << "[COORDINATOR ERROR] Node ID " << intersectionId << " unknown." << std::endl;
        return false;
    }

    if (sessionPtr->status.load() != NodeHealthStatus::ONLINE) {
        std::cerr << "[COORDINATOR WARN] Cannot dispatch " << command
                  << " to Node " << intersectionId << ": Node is "
                  << NodeHealthStatusToString(sessionPtr->status.load()) << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> slock(sessionPtr->sessionMutex);

    for (uint32_t attempt = 1; attempt <= maxRetries_; ++attempt) {
        protocol::AegisMessage req;
        req.timestamp = protocol::AegisMessage::CurrentIsoTimestamp();
        req.intersection = intersectionId;
        req.command = command;
        req.sequence = sessionPtr->sequenceCounter.fetch_add(1);
        req.status = "OK";
        req.details = details;

        if (!TransmitMessage(*sessionPtr, req)) {
            std::cerr << "[COORDINATOR WARN] Send failed for " << command
                      << " to Node " << intersectionId << " (attempt "
                      << attempt << "/" << maxRetries_ << ")" << std::endl;
            DisconnectNode(*sessionPtr);
            sessionPtr->status = NodeHealthStatus::RECONNECTING;
            break;
        }

        auto ack = ReceiveMessageWithTimeout(*sessionPtr, timeoutMs);
        if (ack && ack->command == protocol::Commands::ACK && ack->sequence == req.sequence) {
            sessionPtr->lastAck = std::chrono::steady_clock::now();
            return true;
        }

        std::cerr << "[COORDINATOR WARN] Command " << command << " to Node "
                  << intersectionId << " ACK timeout/mismatch (attempt "
                  << attempt << "/" << maxRetries_ << ")" << std::endl;
    }

    // All retries failed
    std::cerr << "[COORDINATOR ERROR] Node " << intersectionId << " failed "
              << maxRetries_ << " attempts for command " << command
              << ". Marking ERROR." << std::endl;
    sessionPtr->status = NodeHealthStatus::ERROR;
    return false;
}

bool CoordinatorClientManager::GrantGreenCorridor(const std::vector<int>& intersectionIds) {
    std::cout << "[COORDINATOR] >>> Granting Dynamic Green Corridor Preemption (Blue ON, Red OFF) <<<" << std::endl;
    bool allSuccess = true;
    for (int id : intersectionIds) {
        bool rOff = SendCommandWithAck(id, std::string(protocol::Commands::RED_OFF));
        bool bOn = SendCommandWithAck(id, std::string(protocol::Commands::BLUE_ON));
        if (!rOff || !bOn) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool CoordinatorClientManager::ReleaseGreenCorridor(const std::vector<int>& intersectionIds) {
    std::cout << "[COORDINATOR] >>> Releasing Green Corridor (Blue OFF, Red OFF / Safe Reset) <<<" << std::endl;
    bool allSuccess = true;
    for (int id : intersectionIds) {
        bool bOff = SendCommandWithAck(id, std::string(protocol::Commands::BLUE_OFF));
        bool rOff = SendCommandWithAck(id, std::string(protocol::Commands::RED_OFF));
        if (!bOff || !rOff) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool CoordinatorClientManager::SetAllRed() {
    std::cout << "[COORDINATOR] >>> Setting All-Red Hold Phase <<<" << std::endl;
    bool allSuccess = true;
    std::vector<int> ids;
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (const auto& [id, s] : nodes_) ids.push_back(id);
    }
    for (int id : ids) {
        SendCommandWithAck(id, std::string(protocol::Commands::BLUE_OFF));
        SendCommandWithAck(id, std::string(protocol::Commands::RED_ON));
    }
    return allSuccess;
}

bool CoordinatorClientManager::ResetAll() {
    std::cout << "[COORDINATOR] >>> Resetting All Intersections to Normal <<<" << std::endl;
    bool allSuccess = true;
    std::vector<int> ids;
    {
        std::lock_guard<std::mutex> lock(nodesMutex_);
        for (const auto& [id, s] : nodes_) ids.push_back(id);
    }
    for (int id : ids) {
        if (!SendCommandWithAck(id, std::string(protocol::Commands::RESET))) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

} // namespace aegis::coordinator
