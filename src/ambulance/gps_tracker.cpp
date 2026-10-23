#include "aegis/edge/gps_tracker.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

GpsTracker::GpsTracker() = default;
GpsTracker::~GpsTracker() = default;

bool GpsTracker::Initialize(const std::string& /*serialPortPath*/, uint32_t /*baudRate*/) {
    // TODO: Open UART device /dev/ttyS0 or QNX /dev/gps_nmea resmgr
    return true;
}

VehicleKinematics GpsTracker::GetLatestKinematics() const {
    // TODO: Return Extended Kalman Filter filtered telemetry
    return m_latestKinematics;
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
