/**
 * @file ResourceLeakMonitor.cpp
 * @brief Implementation of resource leak monitoring.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/reliability/ResourceLeakMonitor.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::production {

ResourceLeakMonitor::ResourceLeakMonitor(size_t windowSize)
    : windowSize_(windowSize) {}

void ResourceLeakMonitor::AddSample(uint64_t memoryKb, uint32_t socketCount, uint32_t threadCount, uint32_t fdCount) {
    std::lock_guard<std::mutex> lock(mutex_);
    ResourceSample s;
    s.timestampMs = common::TimeUtils::GetCurrentTimestampMs();
    s.memoryKb = memoryKb;
    s.socketCount = socketCount;
    s.threadCount = threadCount;
    s.fdCount = fdCount;

    samples_.push_back(s);
    if (samples_.size() > windowSize_) {
        samples_.erase(samples_.begin());
    }
}

bool ResourceLeakMonitor::DetectLeakPattern() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.size() < 5) {
        return false;
    }

    // Check if the last 4 consecutive transitions (last 5 samples) show monotonic memory increase
    size_t n = samples_.size();
    for (size_t i = n - 4; i < n; ++i) {
        if (samples_[i].memoryKb <= samples_[i - 1].memoryKb) {
            return false;
        }
    }

    return true;
}

bool ResourceLeakMonitor::VerifyCleanup(uint32_t maxAllowedSockets, uint32_t maxAllowedFds) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.empty()) {
        return true;
    }
    const auto& last = samples_.back();
    return (last.socketCount <= maxAllowedSockets && last.fdCount <= maxAllowedFds);
}

double ResourceLeakMonitor::GetMemoryGrowthSlopeKbPerSec() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (samples_.size() < 2) {
        return 0.0;
    }

    const auto& first = samples_.front();
    const auto& last = samples_.back();

    double dtSec = (last.timestampMs > first.timestampMs)
        ? (static_cast<double>(last.timestampMs - first.timestampMs) / 1000.0)
        : 0.001; // Minimum 1 ms quantum

    double dMem = static_cast<double>(last.memoryKb) - static_cast<double>(first.memoryKb);
    return dMem / dtSec;
}

size_t ResourceLeakMonitor::GetSampleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return samples_.size();
}

void ResourceLeakMonitor::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    samples_.clear();
}

} // namespace aegis::production
