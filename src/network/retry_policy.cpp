#include "aegis/network/retry_policy.hpp"
#include <algorithm>

namespace aegis {
namespace network {

RetryPolicy::RetryPolicy(uint32_t baseDelayMs,
                         uint32_t maxDelayMs,
                         double backoffMultiplier,
                         uint32_t maxAttempts)
    : m_baseDelayMs(baseDelayMs),
      m_maxDelayMs(maxDelayMs),
      m_backoffMultiplier(backoffMultiplier),
      m_maxAttempts(maxAttempts),
      m_attemptCount(0),
      m_currentDelayMs(baseDelayMs) {}

uint32_t RetryPolicy::GetNextDelayMs() noexcept {
    return m_currentDelayMs;
}

void RetryPolicy::RecordAttempt() noexcept {
    ++m_attemptCount;
    auto next = static_cast<uint32_t>(static_cast<double>(m_currentDelayMs) * m_backoffMultiplier);
    m_currentDelayMs = std::min(next, m_maxDelayMs);
}

void RetryPolicy::Reset() noexcept {
    m_attemptCount = 0;
    m_currentDelayMs = m_baseDelayMs;
}

bool RetryPolicy::CanRetry() const noexcept {
    return (m_maxAttempts == 0) || (m_attemptCount < m_maxAttempts);
}

uint32_t RetryPolicy::GetAttemptCount() const noexcept {
    return m_attemptCount;
}

uint32_t RetryPolicy::GetMaxAttempts() const noexcept {
    return m_maxAttempts;
}

} // namespace network
} // namespace aegis
