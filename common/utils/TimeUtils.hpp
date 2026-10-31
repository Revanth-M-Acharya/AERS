/**
 * @file TimeUtils.hpp
 * @brief High-resolution time utilities for SMART AEGIS (QNX Neutrino & POSIX).
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace aegis::common {

class TimeUtils {
public:
    /**
     * @brief Get current system epoch timestamp in milliseconds.
     */
    static uint64_t GetCurrentTimestampMs();

    /**
     * @brief Get current system epoch timestamp in microseconds.
     */
    static uint64_t GetCurrentTimestampUs();

    /**
     * @brief Get monotonic clock timestamp in milliseconds.
     * Guaranteed never to step backwards; ideal for timeouts and intervals.
     */
    static uint64_t GetMonotonicTimeMs();

    /**
     * @brief Get monotonic clock timestamp in microseconds.
     */
    static uint64_t GetMonotonicTimeUs();

    /**
     * @brief Format epoch timestamp into ISO 8601 string: YYYY-MM-DD HH:MM:SS.mmm
     */
    static std::string FormatTimestampIso(uint64_t epochMs);

    /**
     * @brief Sleep for specified milliseconds using high-resolution nanosleep.
     */
    static void SleepMs(uint32_t ms);
};

} // namespace aegis::common

