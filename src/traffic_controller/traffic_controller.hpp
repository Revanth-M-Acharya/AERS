#ifndef AEGIS_TRAFFIC_CONTROLLER_HPP
#define AEGIS_TRAFFIC_CONTROLLER_HPP

#include <string>
#include "gpio_interface.hpp"

namespace aegis {
namespace traffic {

enum class ControllerMode : uint8_t {
    NORMAL_CYCLE = 0,
    PREEMPTION_PREPARE,
    GREEN_CORRIDOR,
    RESTORE,
    FAILSAFE_FLASHING_AMBER
};

class TrafficController {
public:
    TrafficController();
    ~TrafficController();

    bool Start();
    void Stop();
    void HandlePreemptionRequest(const std::string& unitId, uint32_t holdSeconds);
    void FallbackToFailSafe();

private:
    GpioInterface m_gpio;
    ControllerMode m_mode{ControllerMode::NORMAL_CYCLE};
};

} // namespace traffic
} // namespace aegis

#endif // AEGIS_TRAFFIC_CONTROLLER_HPP
