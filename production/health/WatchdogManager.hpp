/**
 * @file WatchdogManager.hpp
 * @brief Multi-task real-time software watchdog manager for QNX Neutrino RTOS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <cstdint>

namespace aegis::production {

struct WatchdogTask {
    std::string taskId;
    uint32_t timeoutMs{1000};
    uint64_t lastPetMonoMs{0};
    uint32_t missedCount{0};
    bool isExpired{false};
};

using WatchdogTimeoutCallback = std::function<void(const std::string& taskId, uint32_t elapsedMs, uint32_t timeoutMs)>;

class WatchdogManager {
public:
    WatchdogManager();
    ~WatchdogManager() = default;

    void RegisterTask(const std::string& taskId, uint32_t timeoutMs);
    void UnregisterTask(const std::string& taskId);

    /**
     * @brief Pet/kick the watchdog for a specific task.
     */
    void Pet(const std::string& taskId);

    /**
     * @brief Check all tasks against current monotonic time.
     * @return Number of expired/starved tasks detected.
     */
    uint32_t CheckDeadlines();

    void SetTimeoutCallback(WatchdogTimeoutCallback callback);

    bool IsTaskExpired(const std::string& taskId) const;
    uint32_t GetMissedCount(const std::string& taskId) const;
    size_t GetTotalTasks() const;
    uint32_t GetTotalViolations() const;

    void Reset();

    std::string ToJson() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, WatchdogTask> tasks_;
    WatchdogTimeoutCallback timeoutCallback_;
    uint32_t totalViolations_{0};
};

} // namespace aegis::production

