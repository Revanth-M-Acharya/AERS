#ifndef AEGIS_COORDINATOR_COORDINATOR_HPP
#define AEGIS_COORDINATOR_COORDINATOR_HPP

#include "emergency_call_handler.hpp"
#include "dynamic_green_corridor.hpp"

namespace aegis {
namespace coordinator {

class CentralCoordinator {
public:
    CentralCoordinator();
    ~CentralCoordinator();

    bool Start();
    void Stop();

private:
    EmergencyCallHandler m_callHandler;
    DynamicGreenCorridor m_corridorEngine;
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_COORDINATOR_HPP
