#ifndef AEGIS_EDGE_AMBULANCE_NODE_HPP
#define AEGIS_EDGE_AMBULANCE_NODE_HPP

#include <string>
#include "aegis/common/types.hpp"
#include "aegis/edge/gps_tracker.hpp"
#include "aegis/edge/vitals_monitor.hpp"
#include "aegis/edge/v2v_module.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

/**
 * @brief Main In-Vehicle On-Board Unit (OBU) daemon supervisor.
 * TODO: Implement state machine transitions and 10 Hz telematics transmission.
 */
class AmbulanceNode {
public:
    AmbulanceNode();
    ~AmbulanceNode();

    bool Initialize(const std::string& unitId, const std::string& configPath);
    bool Start();
    void Stop();

private:
    std::string m_unitId{"AMB-01"};
    [[maybe_unused]] common::AmbulanceState m_state{common::AmbulanceState::AVAILABLE};
    [[maybe_unused]] GpsTracker m_gps;
    [[maybe_unused]] VitalsMonitor m_vitals;
    [[maybe_unused]] V2vModule m_v2v;
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_AMBULANCE_NODE_HPP
