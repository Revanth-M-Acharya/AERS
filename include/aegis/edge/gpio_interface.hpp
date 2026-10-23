#ifndef AEGIS_EDGE_GPIO_INTERFACE_HPP
#define AEGIS_EDGE_GPIO_INTERFACE_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Hardware GPIO / Relay driver abstraction.
 * TODO: Implement physical register writes or QNX /dev/traffic_controller resmgr.
 */
class GpioInterface {
public:
    GpioInterface();
    ~GpioInterface();

    bool Initialize();
    void SetLampState(common::SignalPhase northSouth, common::SignalPhase eastWest);
    bool VerifyHardwareInterlock() const;
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_GPIO_INTERFACE_HPP
