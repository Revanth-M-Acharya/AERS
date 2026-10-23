#include "aegis/common/geo_utils.hpp"
#include <cmath>

namespace aegis {
namespace common {

namespace {
constexpr double EARTH_RADIUS_METERS = 6371000.0;
constexpr double PI = 3.14159265358979323846;

inline double DegreesToRadians(double deg) noexcept {
    return deg * (PI / 180.0);
}

inline double RadiansToDegrees(double rad) noexcept {
    return rad * (180.0 / PI);
}
} // namespace

bool IsValidCoordinate(double lat, double lon) noexcept {
    return (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0);
}

bool IsValidCoordinate(const GeoCoordinate& coord) noexcept {
    return IsValidCoordinate(coord.latitude, coord.longitude);
}

double CalculateHaversineDistance(const GeoCoordinate& a, const GeoCoordinate& b) noexcept {
    double lat1 = DegreesToRadians(a.latitude);
    double lon1 = DegreesToRadians(a.longitude);
    double lat2 = DegreesToRadians(b.latitude);
    double lon2 = DegreesToRadians(b.longitude);

    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;

    double sinDLat2 = std::sin(dLat / 2.0);
    double sinDLon2 = std::sin(dLon / 2.0);

    double h = (sinDLat2 * sinDLat2) + std::cos(lat1) * std::cos(lat2) * (sinDLon2 * sinDLon2);
    double c = 2.0 * std::atan2(std::sqrt(h), std::sqrt(1.0 - h));

    return EARTH_RADIUS_METERS * c;
}

double CalculateBearing(const GeoCoordinate& origin, const GeoCoordinate& destination) noexcept {
    double lat1 = DegreesToRadians(origin.latitude);
    double lon1 = DegreesToRadians(origin.longitude);
    double lat2 = DegreesToRadians(destination.latitude);
    double lon2 = DegreesToRadians(destination.longitude);

    double dLon = lon2 - lon1;
    double y = std::sin(dLon) * std::cos(lat2);
    double x = std::cos(lat1) * std::sin(lat2) - std::sin(lat1) * std::cos(lat2) * std::cos(dLon);

    double bearingRad = std::atan2(y, x);
    double bearingDeg = RadiansToDegrees(bearingRad);

    // Normalize to [0.0, 360.0)
    double normalized = std::fmod(bearingDeg + 360.0, 360.0);
    return normalized;
}

double CalculateEtaSeconds(double distanceMeters, double speedKmH) noexcept {
    if (speedKmH <= 0.0 || distanceMeters <= 0.0) {
        return 0.0;
    }
    double speedMetersPerSec = speedKmH * (1000.0 / 3600.0);
    return distanceMeters / speedMetersPerSec;
}

} // namespace common
} // namespace aegis
