#ifndef AEGIS_AMBULANCE_GPS_TRACKER_HPP
#define AEGIS_AMBULANCE_GPS_TRACKER_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace ambulance {

struct VehicleKinematics {
    common::GeoCoordinate position;
    float speedKmH{0.0f};
    float bearingDeg{0.0f};
    uint64_t timestampMs{0};
};

class GpsTracker {
public:
    GpsTracker() = default;
    ~GpsTracker() = default;

    bool Initialize(const std::string& devicePort);
    VehicleKinematics GetLatestKinematics();
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_GPS_TRACKER_HPP
