/**
 * @file NodeWatchdog.hpp
 * @brief Multi-threaded watchdog supervisory monitor for Traffic Signal Node.
 * Enforces immediate failsafe ALL_RED transition upon thread starvation or stall.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

namespace aegis::traffic {

enum class WatchdogThreadId {
    MAIN,
    NETWORK,
    FSM,
    HEARTBEAT
};

inline std::string WatchdogThreadIdToString(WatchdogThreadId id) {
    switch (id) {
        case WatchdogThreadId::MAIN:      return "MAIN";
        case WatchdogThreadId::NETWORK:   return "NETWORK";
        case WatchdogThreadId::FSM:       return "FSM";
        case WatchdogThreadId::HEARTBEAT: return "HEARTBEAT";
        default:                          return "UNKNOWN";
    }
}

using WatchdogFailsafeCallback = std::function<void(WatchdogThreadId stalledThread, uint64_t elapsedMs)>;

class NodeWatchdog {
public:
    explicit NodeWatchdog(uint64_t timeoutMs = 5000, WatchdogFailsafeCallback failsafeCb = nullptr);
    ~NodeWatchdog();

    // Prevent copying
    NodeWatchdog(const NodeWatchdog&) = delete;
    NodeWatchdog& operator=(const NodeWatchdog&) = delete;

    void Start();
    void Stop();

    /**
     * @brief Pet / feed the watchdog for the calling or specified thread.
     */
    void Pet(WatchdogThreadId threadId);

    void SetFailsafeCallback(WatchdogFailsafeCallback cb) {
        std::lock_guard<std::mutex> lock(mutex_);
        failsafeCb_ = std::move(cb);
    }

    bool HasExpired() const { return hasExpired_.load(); }
    uint64_t GetTimeoutMs() const { return timeoutMs_; }

private:
    void WatchdogLoop();

    uint64_t timeoutMs_{5000};
    WatchdogFailsafeCallback failsafeCb_;

    std::atomic<bool> isRunning_{false};
    std::atomic<bool> hasExpired_{false};

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::thread supervisorThread_;

    std::unordered_map<WatchdogThreadId, uint64_t> lastPetMonoMs_;
};

} // namespace aegis::traffic

