/**
 * @file DeadlockDetector.hpp
 * @brief Lock acquisition timeout, thread starvation, and priority inversion monitor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <vector>

namespace aegis::production {

struct LockRecord {
    std::string lockName;
    std::string holdingThreadId;
    uint64_t acquiredMonoMs{0};
    uint32_t contentionCount{0};
    bool isHeld{false};
};

struct StarvationAlert {
    std::string threadId;
    std::string resourceName;
    uint32_t waitDurationMs{0};
    bool isPriorityInversionSuspected{false};
};

class DeadlockDetector {
public:
    explicit DeadlockDetector(uint32_t maxLockHoldMs = 2000);
    ~DeadlockDetector() = default;

    /**
     * @brief Record lock acquisition.
     */
    void RecordLockAcquired(const std::string& lockName, const std::string& threadId);

    /**
     * @brief Record lock release.
     */
    void RecordLockReleased(const std::string& lockName, const std::string& threadId);

    /**
     * @brief Record waiting thread for a lock.
     */
    void RecordLockContention(const std::string& lockName, const std::string& waitingThreadId, uint32_t priority = 10);

    /**
     * @brief Audit active locks for deadlock or starvation conditions.
     * @return List of starvation or priority inversion alerts.
     */
    std::vector<StarvationAlert> CheckForDeadlocks();

    void SetMaxLockHoldMs(uint32_t holdMs);
    uint32_t GetContentionCount(const std::string& lockName) const;
    uint32_t GetTotalDeadlockWarnings() const;

    void Reset();

private:
    mutable std::mutex mutex_;
    uint32_t maxLockHoldMs_{2000};
    std::unordered_map<std::string, LockRecord> locks_;
    std::unordered_map<std::string, std::pair<uint64_t, uint32_t>> waitStartTimes_; // threadId -> (startTimeMs, priority)
    uint32_t totalDeadlockWarnings_{0};
};

} // namespace aegis::production

