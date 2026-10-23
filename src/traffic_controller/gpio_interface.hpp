#ifndef AEGIS_TRAFFIC_GPIO_INTERFACE_HPP
#define AEGIS_TRAFFIC_GPIO_INTERFACE_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace traffic {

class GpioInterface {
public:
    GpioInterface() = default;
    ~GpioInterface() = default;

    bool Initialize();
    void SetLampState(common::SignalPhase northSouth, common::SignalPhase eastWest);
};

} // namespace traffic
} // namespace aegis

#endif // AEGIS_TRAFFIC_GPIO_INTERFACE_HPP
