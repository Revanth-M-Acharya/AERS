/**
 * @file NodeWatchdog.cpp
 * @brief Implementation of Multi-Threaded Node Watchdog.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/watchdog/NodeWatchdog.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "common/utils/ThreadUtils.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::traffic {

NodeWatchdog::NodeWatchdog(uint64_t timeoutMs, WatchdogFailsafeCallback failsafeCb)
    : timeoutMs_(timeoutMs),
      failsafeCb_(std::move(failsafeCb)) {
    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    lastPetMonoMs_[WatchdogThreadId::MAIN] = nowMono;
    lastPetMonoMs_[WatchdogThreadId::NETWORK] = nowMono;
    lastPetMonoMs_[WatchdogThreadId::FSM] = nowMono;
    lastPetMonoMs_[WatchdogThreadId::HEARTBEAT] = nowMono;
}

NodeWatchdog::~NodeWatchdog() {
    Stop();
}

void NodeWatchdog::Start() {
    if (isRunning_.load()) {
        return;
    }
    isRunning_.store(true);
    hasExpired_.store(false);

    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : lastPetMonoMs_) {
            pair.second = nowMono;
        }
    }

    supervisorThread_ = std::thread(&NodeWatchdog::WatchdogLoop, this);
    TrafficLogger::Instance().Info("WATCHDOG", "Node Watchdog started (Timeout: " +
                                  std::to_string(timeoutMs_) + "ms).");
}

void NodeWatchdog::Stop() {
    if (!isRunning_.exchange(false)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        cv_.notify_all();
    }

    if (supervisorThread_.joinable()) {
        supervisorThread_.join();
    }
    TrafficLogger::Instance().Info("WATCHDOG", "Node Watchdog stopped.");
}

void NodeWatchdog::Pet(WatchdogThreadId threadId) {
    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    std::lock_guard<std::mutex> lock(mutex_);
    lastPetMonoMs_[threadId] = nowMono;
}

void NodeWatchdog::WatchdogLoop() {
    common::ThreadUtils::SetCurrentThreadName("TrafficWdog");
    // Assign higher real-time priority to watchdog supervisor
    common::ThreadUtils::SetRealtimePriority(pthread_self(), SCHED_FIFO, 28);

    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(500), [this]() {
            return !isRunning_.load();
        });

        if (!isRunning_.load()) {
            break;
        }

        uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
        WatchdogThreadId stalledThread = WatchdogThreadId::MAIN;
        bool expired = false;
        uint64_t maxElapsed = 0;

        for (const auto& pair : lastPetMonoMs_) {
            if (nowMono > pair.second) {
                uint64_t elapsed = nowMono - pair.second;
                if (elapsed > timeoutMs_) {
                    expired = true;
                    if (elapsed > maxElapsed) {
                        maxElapsed = elapsed;
                        stalledThread = pair.first;
                    }
                }
            }
        }

        if (expired && !hasExpired_.load()) {
            hasExpired_.store(true);
            std::string threadName = WatchdogThreadIdToString(stalledThread);
            TrafficLogger::Instance().Error("WATCHDOG", "[WATCHDOG_FAULT] Thread " + threadName +
                                            " stalled for " + std::to_string(maxElapsed) +
                                            "ms (> " + std::to_string(timeoutMs_) +
                                            "ms). Enforcing ALL_RED safety clearance!");

            WatchdogFailsafeCallback cb = failsafeCb_;
            lock.unlock(); // Release lock before invoking failsafe callback

            if (cb) {
                cb(stalledThread, maxElapsed);
            }
            break;
        }
    }
}

} // namespace aegis::traffic

