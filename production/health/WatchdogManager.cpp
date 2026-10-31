/**
 * @file WatchdogManager.cpp
 * @brief Implementation of software watchdog manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/health/WatchdogManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <vector>

namespace aegis::production {

WatchdogManager::WatchdogManager() = default;

void WatchdogManager::RegisterTask(const std::string& taskId, uint32_t timeoutMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    WatchdogTask task;
    task.taskId = taskId;
    task.timeoutMs = (timeoutMs == 0) ? 1000 : timeoutMs;
    task.lastPetMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    task.missedCount = 0;
    task.isExpired = false;
    tasks_[taskId] = task;
}

void WatchdogManager::UnregisterTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.erase(taskId);
}

void WatchdogManager::Pet(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second.lastPetMonoMs = common::TimeUtils::GetMonotonicTimeMs();
        it->second.isExpired = false;
    }
}

uint32_t WatchdogManager::CheckDeadlines() {
    uint64_t nowMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    uint32_t expiredCount = 0;
    std::vector<std::tuple<std::string, uint32_t, uint32_t>> timeouts;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, task] : tasks_) {
            uint64_t elapsed = (nowMonoMs >= task.lastPetMonoMs) ? (nowMonoMs - task.lastPetMonoMs) : 0;
            if (elapsed > task.timeoutMs) {
                if (!task.isExpired) {
                    task.isExpired = true;
                    task.missedCount++;
                    totalViolations_++;
                    timeouts.emplace_back(id, static_cast<uint32_t>(elapsed), task.timeoutMs);
                }
                expiredCount++;
            }
        }
    }

    for (const auto& [id, elapsed, timeout] : timeouts) {
        if (timeoutCallback_) {
            timeoutCallback_(id, elapsed, timeout);
        }
    }

    return expiredCount;
}

void WatchdogManager::SetTimeoutCallback(WatchdogTimeoutCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeoutCallback_ = std::move(callback);
}

bool WatchdogManager::IsTaskExpired(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second.isExpired;
    }
    return false;
}

uint32_t WatchdogManager::GetMissedCount(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second.missedCount;
    }
    return 0;
}

size_t WatchdogManager::GetTotalTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

uint32_t WatchdogManager::GetTotalViolations() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalViolations_;
}

void WatchdogManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    for (auto& [id, task] : tasks_) {
        task.lastPetMonoMs = now;
        task.isExpired = false;
        task.missedCount = 0;
    }
    totalViolations_ = 0;
}

std::string WatchdogManager::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"total_tasks\": " << tasks_.size() << ",\n"
       << "  \"total_violations\": " << totalViolations_ << ",\n"
       << "  \"tasks\": [\n";

    bool first = true;
    for (const auto& [id, t] : tasks_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"task_id\": \"" << id << "\",\n"
           << "      \"timeout_ms\": " << t.timeoutMs << ",\n"
           << "      \"expired\": " << (t.isExpired ? "true" : "false") << ",\n"
           << "      \"missed_count\": " << t.missedCount << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

} // namespace aegis::production

