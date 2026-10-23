#ifndef AEGIS_COMMON_GEO_UTILS_HPP
#define AEGIS_COMMON_GEO_UTILS_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace common {

/**
 * @brief Computes Haversine distance in meters between two coordinates.
 * @param a Origin coordinate
 * @param b Destination coordinate
 * @return Distance in meters
 */
double CalculateHaversineDistance(const GeoCoordinate& a, const GeoCoordinate& b);

/**
 * @brief Computes initial bearing in degrees [0, 360) from origin to destination.
 * @param origin Origin coordinate
 * @param destination Destination coordinate
 * @return Bearing in degrees
 */
double CalculateBearing(const GeoCoordinate& origin, const GeoCoordinate& destination);

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_GEO_UTILS_HPP
