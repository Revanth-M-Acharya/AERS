/**
 * @file ResourceLeakMonitor.hpp
 * @brief Memory growth slope, socket leak, and file descriptor retention monitor.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::production {

struct ResourceSample {
    uint64_t timestampMs{0};
    uint64_t memoryKb{0};
    uint32_t socketCount{0};
    uint32_t threadCount{0};
    uint32_t fdCount{0};
};

class ResourceLeakMonitor {
public:
    explicit ResourceLeakMonitor(size_t windowSize = 20);
    ~ResourceLeakMonitor() = default;

    /**
     * @brief Record resource usage sample.
     */
    void AddSample(uint64_t memoryKb, uint32_t socketCount, uint32_t threadCount, uint32_t fdCount);

    /**
     * @brief Detect whether memory or FDs exhibit monotonic growth (leak slope).
     * @return true if significant leak pattern detected.
     */
    bool DetectLeakPattern() const;

    /**
     * @brief Verify that active resources have been returned to baseline levels.
     */
    bool VerifyCleanup(uint32_t maxAllowedSockets = 0, uint32_t maxAllowedFds = 10) const;

    double GetMemoryGrowthSlopeKbPerSec() const;
    size_t GetSampleCount() const;
    void Reset();

private:
    mutable std::mutex mutex_;
    size_t windowSize_{20};
    std::vector<ResourceSample> samples_;
};

} // namespace aegis::production

