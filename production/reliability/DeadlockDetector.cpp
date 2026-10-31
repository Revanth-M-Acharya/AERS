/**
 * @file DeadlockDetector.cpp
 * @brief Implementation of deadlock and thread starvation detector.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/reliability/DeadlockDetector.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::production {

DeadlockDetector::DeadlockDetector(uint32_t maxLockHoldMs)
    : maxLockHoldMs_(maxLockHoldMs) {}

void DeadlockDetector::RecordLockAcquired(const std::string& lockName, const std::string& threadId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& rec = locks_[lockName];
    rec.lockName = lockName;
    rec.holdingThreadId = threadId;
    rec.acquiredMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    rec.isHeld = true;

    // Clear wait time for this thread if it was waiting
    waitStartTimes_.erase(threadId);
}

void DeadlockDetector::RecordLockReleased(const std::string& lockName, const std::string& /*threadId*/) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = locks_.find(lockName);
    if (it != locks_.end()) {
        it->second.isHeld = false;
        it->second.holdingThreadId.clear();
    }
}

void DeadlockDetector::RecordLockContention(const std::string& lockName, const std::string& waitingThreadId, uint32_t priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    locks_[lockName].contentionCount++;
    if (waitStartTimes_.find(waitingThreadId) == waitStartTimes_.end()) {
        waitStartTimes_[waitingThreadId] = {common::TimeUtils::GetMonotonicTimeMs(), priority};
    }
}

std::vector<StarvationAlert> DeadlockDetector::CheckForDeadlocks() {
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    std::vector<StarvationAlert> alerts;

    std::lock_guard<std::mutex> lock(mutex_);

    // 1. Check prolonged lock holds
    for (const auto& [name, rec] : locks_) {
        if (rec.isHeld) {
            uint64_t holdDuration = (now >= rec.acquiredMonoMs) ? (now - rec.acquiredMonoMs) : 0;
            if (holdDuration > maxLockHoldMs_) {
                StarvationAlert alert;
                alert.threadId = rec.holdingThreadId;
                alert.resourceName = name;
                alert.waitDurationMs = static_cast<uint32_t>(holdDuration);
                alert.isPriorityInversionSuspected = false;
                alerts.push_back(alert);
                totalDeadlockWarnings_++;
            }
        }
    }

    // 2. Check waiting threads for starvation / priority inversion
    for (const auto& [tId, pInfo] : waitStartTimes_) {
        uint64_t waitDuration = (now >= pInfo.first) ? (now - pInfo.first) : 0;
        if (waitDuration > maxLockHoldMs_) {
            StarvationAlert alert;
            alert.threadId = tId;
            alert.resourceName = "BLOCKED_ON_LOCK";
            alert.waitDurationMs = static_cast<uint32_t>(waitDuration);
            // High priority thread waiting long is likely priority inversion
            alert.isPriorityInversionSuspected = (pInfo.second >= 20);
            alerts.push_back(alert);
            totalDeadlockWarnings_++;
        }
    }

    return alerts;
}

void DeadlockDetector::SetMaxLockHoldMs(uint32_t holdMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    maxLockHoldMs_ = holdMs;
}

uint32_t DeadlockDetector::GetContentionCount(const std::string& lockName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = locks_.find(lockName);
    return (it != locks_.end()) ? it->second.contentionCount : 0;
}

uint32_t DeadlockDetector::GetTotalDeadlockWarnings() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalDeadlockWarnings_;
}

void DeadlockDetector::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    locks_.clear();
    waitStartTimes_.clear();
    totalDeadlockWarnings_ = 0;
}

} // namespace aegis::production

