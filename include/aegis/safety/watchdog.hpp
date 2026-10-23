#ifndef AEGIS_SAFETY_WATCHDOG_HPP
#define AEGIS_SAFETY_WATCHDOG_HPP

#include <cstdint>

namespace aegis {
namespace safety {

/**
 * @brief Hardware Watchdog driver interfacing with /dev/watchdog or QNX HAM.
 * TODO: Implement hardware kick strobe and timeout arming.
 */
class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    bool Initialize(uint32_t timeoutMs);
    void Kick();

private:
    [[maybe_unused]] int m_wdtFd{-1};
    [[maybe_unused]] uint32_t m_timeoutMs{500};
};

} // namespace safety
} // namespace aegis

#endif // AEGIS_SAFETY_WATCHDOG_HPP
