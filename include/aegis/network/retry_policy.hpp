/**
 * @file retry_policy.hpp
 * @brief Exponential backoff retry policy with jitter and maximum limits.
 * @author SMART AEGIS Engineering Team
 */

#ifndef AEGIS_NETWORK_RETRY_POLICY_HPP
#define AEGIS_NETWORK_RETRY_POLICY_HPP

#include <cstdint>

namespace aegis {
namespace network {

class RetryPolicy {
public:
    RetryPolicy(uint32_t baseDelayMs = 500,
                uint32_t maxDelayMs = 5000,
                double backoffMultiplier = 2.0,
                uint32_t maxAttempts = 10);

    uint32_t GetNextDelayMs() noexcept;
    void RecordAttempt() noexcept;
    void Reset() noexcept;

    bool CanRetry() const noexcept;
    uint32_t GetAttemptCount() const noexcept;
    uint32_t GetMaxAttempts() const noexcept;

private:
    uint32_t m_baseDelayMs;
    uint32_t m_maxDelayMs;
    double m_backoffMultiplier;
    uint32_t m_maxAttempts;

    uint32_t m_attemptCount{0};
    uint32_t m_currentDelayMs{500};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_RETRY_POLICY_HPP
