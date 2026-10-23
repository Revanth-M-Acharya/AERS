#ifndef AEGIS_EDGE_GPS_TRACKER_HPP
#define AEGIS_EDGE_GPS_TRACKER_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct VehicleKinematics {
    common::GeoCoordinate position;
    float speedKmH{0.0f};
    float bearingDeg{0.0f};
    float hdop{1.0f};
    uint8_t fixQuality{1};
    uint64_t timestampMs{0};
};

/**
 * @brief Serial NMEA-0183 parser with Extended Kalman Filter dead-reckoning fallback.
 * TODO: Implement hardware serial reader and IMU covariance fusion.
 */
class GpsTracker {
public:
    GpsTracker();
    ~GpsTracker();

    bool Initialize(const std::string& serialPortPath, uint32_t baudRate);
    VehicleKinematics GetLatestKinematics() const;

private:
    VehicleKinematics m_latestKinematics;
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_GPS_TRACKER_HPP
