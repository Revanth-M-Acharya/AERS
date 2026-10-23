#include "aegis/common/timer.hpp"
#include "aegis/common/time_utils.hpp"

namespace aegis {
namespace common {

SoftwareTimer::SoftwareTimer() : m_durationMs(0), m_isActive(false) {}

SoftwareTimer::SoftwareTimer(uint64_t durationMs) : m_durationMs(durationMs), m_isActive(false) {}

void SoftwareTimer::Start() noexcept {
    m_startMonotonicMs = GetMonotonicMicroseconds() / 1000;
    m_isActive = true;
}

void SoftwareTimer::Start(uint64_t durationMs) noexcept {
    m_durationMs = durationMs;
    Start();
}

void SoftwareTimer::Stop() noexcept {
    m_isActive = false;
}

void SoftwareTimer::Reset() noexcept {
    Start();
}

bool SoftwareTimer::IsExpired() const noexcept {
    if (!m_isActive) return false;
    return ElapsedMs() >= m_durationMs;
}

bool SoftwareTimer::IsActive() const noexcept {
    return m_isActive;
}

uint64_t SoftwareTimer::ElapsedMs() const noexcept {
    if (!m_isActive) return 0;
    uint64_t nowMs = GetMonotonicMicroseconds() / 1000;
    if (nowMs < m_startMonotonicMs) return 0;
    return nowMs - m_startMonotonicMs;
}

uint64_t SoftwareTimer::RemainingMs() const noexcept {
    if (!m_isActive || IsExpired()) return 0;
    return m_durationMs - ElapsedMs();
}

} // namespace common
} // namespace aegis
