#include "watchdog.hpp"

namespace aegis {
namespace safety {

bool Watchdog::Initialize(uint32_t /*timeoutMs*/) {
    // Placeholder: Open /dev/watchdog on Linux or attach HAM pulse on QNX
    return true;
}

void Watchdog::Kick() {
    // Placeholder: Write keep-alive strobe to watchdog driver
}

} // namespace safety
} // namespace aegis
