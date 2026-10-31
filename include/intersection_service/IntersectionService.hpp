/**
 * @file IntersectionService.hpp
 * @brief TCP Server daemon for SMART AEGIS Roadside Intersection Nodes.
 *
 * Runs on Intersection Raspberry Pis (Pis 2, 3, 4) or in simulated environments.
 * Listens on dedicated TCP port (5001, 5002, 5003), greets the Coordinator with
 * INTERSECTION_READY, processes protocol commands, drives GPIO LEDs via IGpioController,
 * and replies with JSON ACK messages.
 *
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "gpio/IGpioController.hpp"
#include "protocol/AegisProtocol.hpp"
#include <memory>
#include <thread>
#include <atomic>
#include <functional>
#include <string>
#include <mutex>

namespace aegis::intersection {

class IntersectionService {
public:
    using MessageCallback = std::function<void(const protocol::AegisMessage&)>;

    /**
     * @brief Construct an Intersection TCP Server service.
     * @param intersectionId Numerical ID (1, 2, or 3).
     * @param port TCP port to bind (default 5000 + intersectionId).
     * @param gpio Controller instance (if nullptr, instantiated via GpioFactory).
     */
    explicit IntersectionService(int intersectionId,
                                 uint16_t port = 0,
                                 std::shared_ptr<gpio::IGpioController> gpio = nullptr);
    ~IntersectionService();

    // Prevent copies
    IntersectionService(const IntersectionService&) = delete;
    IntersectionService& operator=(const IntersectionService&) = delete;

    /**
     * @brief Start TCP server listening thread.
     * @return true if socket bound and listening started successfully.
     */
    bool Start();

    /**
     * @brief Gracefully terminate TCP server, close client connections, and release GPIO.
     */
    void Stop();

    /**
     * @brief Check if the server thread is running.
     */
    bool IsRunning() const noexcept { return running_.load(); }

    /**
     * @brief Check if a Coordinator client is currently connected.
     */
    bool IsClientConnected() const noexcept { return clientConnected_.load(); }

    /**
     * @brief Return bound port.
     */
    uint16_t GetPort() const noexcept { return port_; }

    /**
     * @brief Return intersection numerical ID.
     */
    int GetIntersectionId() const noexcept { return intersectionId_; }

    /**
     * @brief Total number of processed commands.
     */
    size_t GetCommandCount() const noexcept { return commandCount_.load(); }

    /**
     * @brief Retrieve GPIO controller interface.
     */
    std::shared_ptr<gpio::IGpioController> GetGpioController() const noexcept { return gpio_; }

    /**
     * @brief Register callback hook for incoming executed commands.
     */
    void SetCommandCallback(MessageCallback cb);

private:
    void ServerWorker();
    void HandleClientSession(int clientFd);
    void ProcessIncomingLine(int clientFd, const std::string& line);
    bool SendMessage(int clientFd, const protocol::AegisMessage& msg);

    int intersectionId_{1};
    uint16_t port_{5001};
    std::shared_ptr<gpio::IGpioController> gpio_;
    std::atomic<bool> running_{false};
    std::atomic<bool> clientConnected_{false};
    std::atomic<size_t> commandCount_{0};

    int listenSock_{-1};
    std::atomic<int> currentClientSock_{-1};
    std::thread serverThread_;
    mutable std::mutex mutex_;
    MessageCallback commandCallback_;
};

} // namespace aegis::intersection
