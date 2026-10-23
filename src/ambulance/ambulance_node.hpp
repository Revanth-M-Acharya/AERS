#ifndef AEGIS_AMBULANCE_NODE_HPP
#define AEGIS_AMBULANCE_NODE_HPP

#include "gps_tracker.hpp"
#include "vitals_monitor.hpp"
#include "v2v_module.hpp"

namespace aegis {
namespace ambulance {

class AmbulanceNode {
public:
    AmbulanceNode();
    ~AmbulanceNode();

    bool Start();
    void Stop();

private:
    GpsTracker m_gps;
    VitalsMonitor m_vitals;
    V2vModule m_v2v;
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_NODE_HPP
