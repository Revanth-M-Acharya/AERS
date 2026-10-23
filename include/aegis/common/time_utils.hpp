/**
 * @file time_utils.hpp
 * @brief High-resolution clock, monotonic timestamp, and Stopwatch utilities.
 */

#ifndef AEGIS_COMMON_TIME_UTILS_HPP
#define AEGIS_COMMON_TIME_UTILS_HPP

#include <cstdint>
#include <string>
#include <chrono>

namespace aegis {
namespace common {

/**
 * @brief Returns current monotonic time in microseconds (for deterministic timing loops).
 */
uint64_t GetMonotonicMicroseconds() noexcept;

/**
 * @brief Returns current UTC time in milliseconds since Unix Epoch.
 */
uint64_t GetUtcMilliseconds() noexcept;

/**
 * @brief Formats given Unix milliseconds into ISO 8601 UTC string (e.g., 2026-10-24T00:45:00.123Z).
 */
std::string FormatUtcIso8601(uint64_t epochMs);

/**
 * @brief Precision Stopwatch for profiling and timeout monitoring.
 */
class Stopwatch {
public:
    Stopwatch();
    void Start() noexcept;
    void Stop() noexcept;
    void Reset() noexcept;

    uint64_t ElapsedMilliseconds() const noexcept;
    uint64_t ElapsedMicroseconds() const noexcept;
    bool IsRunning() const noexcept;

private:
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_stopTime;
    bool m_isRunning{false};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_TIME_UTILS_HPP
