#ifndef AEGIS_CORE_COORDINATOR_HPP
#define AEGIS_CORE_COORDINATOR_HPP

#include <memory>
#include "aegis/core/emergency_call_handler.hpp"
#include "aegis/core/dynamic_green_corridor.hpp"
#include "aegis/core/route_planner.hpp"
#include "aegis/core/priority_scheduler.hpp"

namespace aegis {
namespace core {
namespace coordinator {

/**
 * @brief Central dispatch supervisor coordinating call intake, routing, and preemption.
 * TODO: Implement central event processing loop and asset state locking.
 */
class CentralCoordinator {
public:
    CentralCoordinator();
    ~CentralCoordinator();

    bool Start();
    void Stop();

private:
    EmergencyCallHandler m_callHandler;
    DynamicGreenCorridor m_corridorEngine;
    routing::RoutePlanner m_routePlanner;
    scheduler::PriorityScheduler m_priorityScheduler;
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_COORDINATOR_HPP
