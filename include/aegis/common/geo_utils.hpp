/**
 * @file geo_utils.hpp
 * @brief Geospatial calculations: Haversine distance, initial bearing, ETA, coordinate validation.
 */

#ifndef AEGIS_COMMON_GEO_UTILS_HPP
#define AEGIS_COMMON_GEO_UTILS_HPP

#include "aegis/common/types.hpp"

namespace aegis {
namespace common {

/**
 * @brief Validates if latitude and longitude conform to WGS84 physical boundaries.
 */
bool IsValidCoordinate(const GeoCoordinate& coord) noexcept;
bool IsValidCoordinate(double lat, double lon) noexcept;

/**
 * @brief Computes Great-Circle Haversine distance in meters between two coordinates.
 */
double CalculateHaversineDistance(const GeoCoordinate& a, const GeoCoordinate& b) noexcept;

/**
 * @brief Computes forward compass azimuth bearing in degrees [0.0, 360.0).
 */
double CalculateBearing(const GeoCoordinate& origin, const GeoCoordinate& destination) noexcept;

/**
 * @brief Estimates travel time in seconds given distance in meters and ground speed in km/h.
 * Returns 0.0 if speed <= 0.0 or distance <= 0.0.
 */
double CalculateEtaSeconds(double distanceMeters, double speedKmH) noexcept;

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_GEO_UTILS_HPP
