/**
 * @file CoordinatorClientManager.hpp
 * @brief TCP Client Engine and Health Supervisor for SMART AEGIS Central Coordinator.
 *
 * Implements the Central Coordinator network layer on Raspberry Pi 1.
 * Acts as a resilient TCP Client connecting to Intersection Nodes (Pis 2, 3, 4).
 * Enforces 2-second automatic reconnects, 5-second periodic PING health checks,
 * 3-attempt command retries with ACK verification, and dynamic Green Corridor preemption.
 *
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "aegis/config/IntersectionsConfig.hpp"
#include "protocol/AegisProtocol.hpp"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

namespace aegis::coordinator {

enum class NodeHealthStatus {
    OFFLINE,
    RECONNECTING,
    ONLINE,
    ERROR
};

inline const char* NodeHealthStatusToString(NodeHealthStatus s) {
    switch (s) {
        case NodeHealthStatus::ONLINE: return "ONLINE";
        case NodeHealthStatus::OFFLINE: return "OFFLINE";
        case NodeHealthStatus::RECONNECTING: return "RECONNECTING";
        case NodeHealthStatus::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

struct NodeSession {
    int id{0};
    std::string ip;
    uint16_t port{0};
    int socketFd{-1};
    std::atomic<NodeHealthStatus> status{NodeHealthStatus::OFFLINE};
    std::atomic<uint64_t> sequenceCounter{1};
    std::chrono::steady_clock::time_point lastPing{};
    std::chrono::steady_clock::time_point lastAck{};
    std::string rxBuffer;
    std::mutex sessionMutex;
    bool hasRedLed{true};
    bool hasBlueLed{true};
};

class CoordinatorClientManager {
public:
    /**
     * @brief Construct Coordinator Client Manager using parsed cluster config.
     * @param config Cluster configuration loaded from config/intersections.json.
     */
    explicit CoordinatorClientManager(std::shared_ptr<config::IntersectionsConfig> config = nullptr);
    ~CoordinatorClientManager();

    // Non-copyable
    CoordinatorClientManager(const CoordinatorClientManager&) = delete;
    CoordinatorClientManager& operator=(const CoordinatorClientManager&) = delete;

    /**
     * @brief Initialize sessions and start background reconnect and health check threads.
     */
    bool Start();

    /**
     * @brief Gracefully terminate client sockets and workers.
     */
    void Stop();

    /**
     * @brief Check whether the manager is actively running.
     */
    bool IsRunning() const noexcept { return running_.load(); }

    /**
     * @brief Send a command to a specific intersection and wait for ACK with auto-retry.
     * @param intersectionId Numerical ID (1, 2, or 3).
     * @param command Command verb (e.g. BLUE_ON, RED_OFF).
     * @param details Optional diagnostic details.
     * @param timeoutMs Timeout in milliseconds for each attempt (default 1000 ms).
     * @return true if ACK received within max retries; false if failed.
     */
    bool SendCommandWithAck(int intersectionId,
                            const std::string& command,
                            const std::string& details = "",
                            uint32_t timeoutMs = 1000);

    /**
     * @brief Preempt green corridor across specified intersections (Turn Blue ON, Red OFF).
     */
    bool GrantGreenCorridor(const std::vector<int>& intersectionIds);

    /**
     * @brief Release green corridor across specified intersections (Turn Blue OFF, Red OFF).
     */
    bool ReleaseGreenCorridor(const std::vector<int>& intersectionIds);

    /**
     * @brief Set all intersections to All-Red hold state.
     */
    bool SetAllRed();

    /**
     * @brief Reset all intersections to safe default OFF state.
     */
    bool ResetAll();

    /**
     * @brief Query current health status of an intersection node.
     */
    NodeHealthStatus GetNodeStatus(int intersectionId) const;

    /**
     * @brief Retrieve health statuses of all managed nodes.
     */
    std::map<int, NodeHealthStatus> GetAllNodeStatuses() const;

    /**
     * @brief Override target IP / Port (useful for local testing or simulated topologies).
     */
    void SetNodeEndpoint(int intersectionId, const std::string& ip, uint16_t port);

    /**
     * @brief Adjust timing intervals (milliseconds).
     */
    void SetReconnectIntervalMs(uint32_t ms) noexcept { reconnectIntervalMs_ = ms; }
    void SetPingIntervalMs(uint32_t ms) noexcept { pingIntervalMs_ = ms; }
    void SetMaxRetries(uint32_t retries) noexcept { maxRetries_ = retries; }

    uint32_t GetReconnectIntervalMs() const noexcept { return reconnectIntervalMs_; }
    uint32_t GetPingIntervalMs() const noexcept { return pingIntervalMs_; }
    uint32_t GetMaxRetries() const noexcept { return maxRetries_; }

private:
    void ConnectionWorker();
    void HealthMonitorWorker();

    bool ConnectNode(NodeSession& session);
    void DisconnectNode(NodeSession& session);
    bool TransmitMessage(NodeSession& session, const protocol::AegisMessage& msg);
    std::optional<protocol::AegisMessage> ReceiveMessageWithTimeout(NodeSession& session, uint32_t timeoutMs);

    std::shared_ptr<config::IntersectionsConfig> config_;
    std::atomic<bool> running_{false};
    uint32_t reconnectIntervalMs_{2000};
    uint32_t pingIntervalMs_{5000};
    uint32_t maxRetries_{3};

    mutable std::mutex nodesMutex_;
    std::map<int, std::unique_ptr<NodeSession>> nodes_;

    std::thread connectionThread_;
    std::thread healthThread_;
};

} // namespace aegis::coordinator
