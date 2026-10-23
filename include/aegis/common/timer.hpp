/**
 * @file timer.hpp
 * @brief Non-blocking software timer utility.
 */

#ifndef AEGIS_COMMON_TIMER_HPP
#define AEGIS_COMMON_TIMER_HPP

#include <cstdint>

namespace aegis {
namespace common {

class SoftwareTimer {
public:
    SoftwareTimer();
    explicit SoftwareTimer(uint64_t durationMs);

    void Start() noexcept;
    void Start(uint64_t durationMs) noexcept;
    void Stop() noexcept;
    void Reset() noexcept;

    bool IsExpired() const noexcept;
    bool IsActive() const noexcept;
    uint64_t RemainingMs() const noexcept;
    uint64_t ElapsedMs() const noexcept;

private:
    uint64_t m_startMonotonicMs{0};
    uint64_t m_durationMs{0};
    bool m_isActive{false};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_TIMER_HPP
