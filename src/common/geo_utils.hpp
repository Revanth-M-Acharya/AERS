#ifndef AEGIS_COMMON_GEO_UTILS_HPP
#define AEGIS_COMMON_GEO_UTILS_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace common {

/**
 * @brief Computes Haversine distance in meters between two coordinates.
 */
double CalculateHaversineDistance(const GeoCoordinate& a, const GeoCoordinate& b);

/**
 * @brief Computes compass bearing in degrees [0, 360) from origin to destination.
 */
double CalculateBearing(const GeoCoordinate& origin, const GeoCoordinate& destination);

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_GEO_UTILS_HPP
