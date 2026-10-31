/**
 * @file IntersectionService.cpp
 * @brief Implementation of Intersection TCP Server daemon.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "intersection_service/IntersectionService.hpp"
#include "gpio/GpioFactory.hpp"
#include <iostream>
#include <cstring>
#include <chrono>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

namespace aegis::intersection {

IntersectionService::IntersectionService(int intersectionId,
                                         uint16_t port,
                                         std::shared_ptr<gpio::IGpioController> gpio)
    : intersectionId_(intersectionId),
      port_(port > 0 ? port : static_cast<uint16_t>(5000 + intersectionId)),
      gpio_(std::move(gpio)) {
    if (!gpio_) {
        gpio_ = gpio::GpioFactory::Create(intersectionId_);
    }
}

IntersectionService::~IntersectionService() {
    Stop();
}

bool IntersectionService::Start() {
    if (running_.load()) {
        return true;
    }

#if defined(SIGPIPE)
    signal(SIGPIPE, SIG_IGN);
#endif

    if (gpio_) {
        gpio_->initialize();
    }

    listenSock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock_ < 0) {
        std::cerr << "[INT " << intersectionId_ << " ERROR] Failed to create socket: "
                  << strerror(errno) << std::endl;
        return false;
    }

    int opt = 1;
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#ifdef SO_REUSEPORT
    setsockopt(listenSock_, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
#endif

    // Set non-blocking for clean shutdown with select()
    int flags = fcntl(listenSock_, F_GETFL, 0);
    fcntl(listenSock_, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    if (bind(listenSock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[INT " << intersectionId_ << " ERROR] Bind failed on port "
                  << port_ << ": " << strerror(errno) << std::endl;
        close(listenSock_);
        listenSock_ = -1;
        return false;
    }

    if (listen(listenSock_, 5) < 0) {
        std::cerr << "[INT " << intersectionId_ << " ERROR] Listen failed: "
                  << strerror(errno) << std::endl;
        close(listenSock_);
        listenSock_ = -1;
        return false;
    }

    running_.store(true);
    serverThread_ = std::thread(&IntersectionService::ServerWorker, this);

    std::cout << "[INT " << intersectionId_ << " READY] TCP Server listening on port "
              << port_ << " (Backend: " << (gpio_ ? gpio_->getBackendType() : "NONE") << ")" << std::endl;
    return true;
}

void IntersectionService::Stop() {
    if (!running_.exchange(false)) {
        return;
    }

    int cSock = currentClientSock_.exchange(-1);
    if (cSock >= 0) {
        shutdown(cSock, SHUT_RDWR);
        close(cSock);
    }

    if (listenSock_ >= 0) {
        shutdown(listenSock_, SHUT_RDWR);
        close(listenSock_);
        listenSock_ = -1;
    }

    if (serverThread_.joinable()) {
        serverThread_.join();
    }

    if (gpio_) {
        gpio_->reset();
        gpio_->shutdown();
    }

    clientConnected_.store(false);
    std::cout << "[INT " << intersectionId_ << " SHUTDOWN] Server stopped." << std::endl;
}

void IntersectionService::SetCommandCallback(MessageCallback cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    commandCallback_ = std::move(cb);
}

bool IntersectionService::SendMessage(int clientFd, const protocol::AegisMessage& msg) {
    if (clientFd < 0) return false;
    std::string json = msg.ToJson();
#if defined(MSG_NOSIGNAL)
    int sendFlags = MSG_NOSIGNAL;
#else
    int sendFlags = 0;
#endif
    ssize_t sent = send(clientFd, json.data(), json.size(), sendFlags);
    return (sent == static_cast<ssize_t>(json.size()));
}

void IntersectionService::ServerWorker() {
    while (running_.load()) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(listenSock_, &readFds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms select timeout

        int activity = select(listenSock_ + 1, &readFds, nullptr, nullptr, &tv);
        if (activity < 0 && errno != EINTR) {
            if (!running_.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (activity > 0 && FD_ISSET(listenSock_, &readFds)) {
            sockaddr_in clientAddr{};
            socklen_t clientLen = sizeof(clientAddr);
            int clientFd = accept(listenSock_, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
            if (clientFd >= 0) {
#ifdef SO_NOSIGPIPE
                int optNoSigpipe = 1;
                setsockopt(clientFd, SOL_SOCKET, SO_NOSIGPIPE, &optNoSigpipe, sizeof(optNoSigpipe));
#endif
                char clientIp[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
                std::cout << "[INT " << intersectionId_ << "] Coordinator connected from "
                          << clientIp << ":" << ntohs(clientAddr.sin_port) << std::endl;

                clientConnected_.store(true);
                currentClientSock_.store(clientFd);

                // Send immediate INTERSECTION_READY greeting
                protocol::AegisMessage readyMsg;
                readyMsg.timestamp = protocol::AegisMessage::CurrentIsoTimestamp();
                readyMsg.intersection = intersectionId_;
                readyMsg.command = std::string(protocol::Commands::INTERSECTION_READY);
                readyMsg.sequence = 0;
                readyMsg.status = "OK";
                readyMsg.details = "Intersection " + std::to_string(intersectionId_) + " Ready";
                SendMessage(clientFd, readyMsg);

                HandleClientSession(clientFd);

                close(clientFd);
                currentClientSock_.store(-1);
                clientConnected_.store(false);
                std::cout << "[INT " << intersectionId_ << "] Coordinator disconnected." << std::endl;
            }
        }
    }
}

void IntersectionService::HandleClientSession(int clientFd) {
    std::string rxBuffer;
    char buffer[1024];

    // Set non-blocking on client socket
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    while (running_.load()) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(clientFd, &readFds);

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int res = select(clientFd + 1, &readFds, nullptr, nullptr, &tv);
        if (res < 0 && errno != EINTR) {
            break;
        }

        if (res > 0 && FD_ISSET(clientFd, &readFds)) {
            ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) {
                if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    continue;
                }
                // Client closed connection or error
                break;
            }

            buffer[bytesRead] = '\0';
            rxBuffer.append(buffer, static_cast<size_t>(bytesRead));

            // Process full lines
            size_t newlinePos;
            while ((newlinePos = rxBuffer.find('\n')) != std::string::npos) {
                std::string line = rxBuffer.substr(0, newlinePos);
                rxBuffer.erase(0, newlinePos + 1);
                if (!line.empty()) {
                    ProcessIncomingLine(clientFd, line);
                }
            }
        }
    }
}

void IntersectionService::ProcessIncomingLine(int clientFd, const std::string& line) {
    auto parsed = protocol::AegisMessage::FromJson(line);
    if (!parsed) {
        std::cerr << "[INT " << intersectionId_ << " WARN] Malformed JSON received: " << line << std::endl;
        protocol::AegisMessage err;
        err.intersection = intersectionId_;
        err.command = std::string(protocol::Commands::ERROR);
        err.status = "ERROR";
        err.details = "Malformed JSON message";
        SendMessage(clientFd, err);
        return;
    }

    const auto& req = *parsed;
    protocol::AegisMessage ack;
    ack.timestamp = protocol::AegisMessage::CurrentIsoTimestamp();
    ack.intersection = intersectionId_;
    ack.command = std::string(protocol::Commands::ACK);
    ack.sequence = req.sequence;
    ack.status = "OK";

    if (req.command == protocol::Commands::INITIALIZE) {
        if (gpio_) {
            gpio_->initialize();
            gpio_->reset();
        }
        ack.details = "INITIALIZED";
    } else if (req.command == protocol::Commands::PING) {
        ack.details = "PONG";
    } else if (req.command == protocol::Commands::HEALTH_CHECK || req.command == protocol::Commands::STATUS) {
        bool blue = gpio_ ? gpio_->getBlue() : false;
        bool red = gpio_ ? gpio_->getRed() : false;
        ack.details = "BLUE=" + std::string(blue ? "ON" : "OFF") +
                      ",RED=" + std::string(red ? "ON" : "OFF");
    } else if (req.command == protocol::Commands::BLUE_ON) {
        if (gpio_) gpio_->setBlue(true);
        ack.details = "BLUE_ON_OK";
    } else if (req.command == protocol::Commands::BLUE_OFF) {
        if (gpio_) gpio_->setBlue(false);
        ack.details = "BLUE_OFF_OK";
    } else if (req.command == protocol::Commands::RED_ON) {
        if (gpio_) gpio_->setRed(true);
        ack.details = "RED_ON_OK";
    } else if (req.command == protocol::Commands::RED_OFF) {
        if (gpio_) gpio_->setRed(false);
        ack.details = "RED_OFF_OK";
    } else if (req.command == protocol::Commands::RESET) {
        if (gpio_) gpio_->reset();
        ack.details = "RESET_OK";
    } else if (req.command == protocol::Commands::SHUTDOWN) {
        if (gpio_) gpio_->shutdown();
        ack.details = "SHUTDOWN_OK";
    } else {
        ack.command = std::string(protocol::Commands::ERROR);
        ack.status = "ERROR";
        ack.details = "UNKNOWN_COMMAND: " + req.command;
    }

    SendMessage(clientFd, ack);
    commandCount_.fetch_add(1);

    // Invoke callback hook
    MessageCallback cbCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        cbCopy = commandCallback_;
    }
    if (cbCopy) {
        cbCopy(req);
    }
}

} // namespace aegis::intersection
