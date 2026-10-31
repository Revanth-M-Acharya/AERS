/**
 * @file TimeUtils.cpp
 * @brief Implementation of high-resolution time utilities.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "common/utils/TimeUtils.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>

namespace aegis::common {

uint64_t TimeUtils::GetCurrentTimestampMs() {
    auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
}

uint64_t TimeUtils::GetCurrentTimestampUs() {
    auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()
    );
}

uint64_t TimeUtils::GetMonotonicTimeMs() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );
}

uint64_t TimeUtils::GetMonotonicTimeUs() {
    auto now = std::chrono::steady_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count()
    );
}

std::string TimeUtils::FormatTimestampIso(uint64_t epochMs) {
    auto secs = static_cast<time_t>(epochMs / 1000ULL);
    auto msecs = static_cast<uint32_t>(epochMs % 1000ULL);

    std::tm tmBuffer{};
#if defined(_WIN32)
    localtime_s(&tmBuffer, &secs);
#else
    localtime_r(&secs, &tmBuffer);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << msecs;
    return oss.str();
}

void TimeUtils::SleepMs(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

} // namespace aegis::common

