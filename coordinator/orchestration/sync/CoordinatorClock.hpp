/**
 * @file CoordinatorClock.hpp
 * @brief High-precision monotonic and synchronization clock provider for SMART AEGIS.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/utils/TimeUtils.hpp"
#include <cstdint>

namespace aegis::coordinator::orchestration {

struct ExecutionTimeTags {
    uint64_t creationTimeMonoMs{0};
    uint64_t dispatchTimeMonoMs{0};
    uint64_t executionTimeMonoMs{0};
    uint64_t expirationTimeMonoMs{0};
};

class CoordinatorClock {
public:
    static uint64_t NowMonoMs() {
        return common::TimeUtils::GetMonotonicTimeMs();
    }

    static uint64_t NowEpochMs() {
        return common::TimeUtils::GetCurrentTimestampMs();
    }

    static ExecutionTimeTags CreateTimeTags(uint64_t dispatchDelayMs,
                                            uint32_t leadTimeMs,
                                            uint32_t holdDurationMs,
                                            uint32_t safetyMarginMs = 5000) {
        uint64_t now = NowMonoMs();
        ExecutionTimeTags tags;
        tags.creationTimeMonoMs = now;
        tags.dispatchTimeMonoMs = now + dispatchDelayMs;
        tags.executionTimeMonoMs = tags.dispatchTimeMonoMs + leadTimeMs;
        tags.expirationTimeMonoMs = tags.executionTimeMonoMs + holdDurationMs + safetyMarginMs;
        return tags;
    }
};

} // namespace aegis::coordinator::orchestration

