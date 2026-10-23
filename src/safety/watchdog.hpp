#ifndef AEGIS_SAFETY_WATCHDOG_HPP
#define AEGIS_SAFETY_WATCHDOG_HPP

#include <cstdint>

namespace aegis {
namespace safety {

class Watchdog {
public:
    Watchdog() = default;
    ~Watchdog() = default;

    bool Initialize(uint32_t timeoutMs);
    void Kick();

private:
    int m_wdtFd{-1};
};

} // namespace safety
} // namespace aegis

#endif // AEGIS_SAFETY_WATCHDOG_HPP
