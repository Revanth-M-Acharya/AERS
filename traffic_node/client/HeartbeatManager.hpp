/**
 * @file HeartbeatManager.hpp
 * @brief Autonomous heartbeat engine for SMART AEGIS Traffic Signal Node.
 * Transmits enriched diagnostic telemetry (State, Relays, CPU, Memory, Temp, Uptime).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace aegis::traffic {

using HeartbeatSendCallback = std::function<bool(const protocol::Packet& packet)>;
using HeartbeatTimeoutCallback = std::function<void()>;
using HeartbeatPayloadGenerator = std::function<std::string()>;
using HeartbeatPetCallback = std::function<void()>;

class HeartbeatManager {
public:
    HeartbeatManager(std::string nodeId, uint64_t intervalMs, uint64_t timeoutMs,
                     HeartbeatSendCallback sendCb, HeartbeatTimeoutCallback timeoutCb,
                     HeartbeatPayloadGenerator payloadGen = nullptr);
    ~HeartbeatManager();

    // Prevent copying
    HeartbeatManager(const HeartbeatManager&) = delete;
    HeartbeatManager& operator=(const HeartbeatManager&) = delete;

    void Start();
    void Stop();
    void OnHeartbeatAckReceived();
    void Reset();

    void SetPayloadGenerator(HeartbeatPayloadGenerator gen) {
        std::lock_guard<std::mutex> lock(cvMutex_);
        payloadGenerator_ = std::move(gen);
    }

    void SetPetCallback(HeartbeatPetCallback petCb) {
        std::lock_guard<std::mutex> lock(cvMutex_);
        petCallback_ = std::move(petCb);
    }

    bool IsRunning() const { return isRunning_.load(); }
    uint64_t GetLastAckMonotonicMs() const { return lastAckMonotonicMs_.load(); }
    uint64_t GetHeartbeatCounter() const { return heartbeatCounter_.load(); }

    std::string GenerateDefaultTelemetry() const;

private:
    void HeartbeatLoop();

    std::string nodeId_;
    uint64_t intervalMs_{1000};
    uint64_t timeoutMs_{5000};
    HeartbeatSendCallback sendCallback_;
    HeartbeatTimeoutCallback timeoutCallback_;
    HeartbeatPayloadGenerator payloadGenerator_;
    HeartbeatPetCallback petCallback_;

    std::atomic<bool> isRunning_{false};
    std::atomic<uint64_t> lastAckMonotonicMs_{0};
    std::atomic<uint64_t> heartbeatCounter_{0};
    uint64_t bootTimeMonoMs_{0};

    std::thread workerThread_;
    std::mutex cvMutex_;
    std::condition_variable cv_;
};

} // namespace aegis::traffic
