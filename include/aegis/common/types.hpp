/**
 * @file types.hpp
 * @brief Common data structures, enumerations, and type aliases for SMART AEGIS.
 * @author SMART AEGIS Engineering Team
 */

#ifndef AEGIS_COMMON_TYPES_HPP
#define AEGIS_COMMON_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace aegis {
namespace common {

/**
 * @brief Geographic coordinate point in WGS84 format.
 */
struct GeoCoordinate {
    double latitude{0.0};   ///< Latitude in degrees [-90.0, +90.0]
    double longitude{0.0};  ///< Longitude in degrees [-180.0, +180.0]
    double altitude{0.0};   ///< Altitude in meters
};

/**
 * @brief Operational lifecycle states for an ambulance unit.
 */
enum class AmbulanceState : uint8_t {
    AVAILABLE = 0,
    DISPATCHED,
    EN_ROUTE_PATIENT,
    PATIENT_PICKED,
    EN_ROUTE_HOSPITAL,
    ARRIVED,
    OFF_DUTY
};

/**
 * @brief Signal phase states for a traffic intersection movement.
 */
enum class SignalPhase : uint8_t {
    RED = 0,
    YELLOW,
    GREEN,
    FLASHING_AMBER
};

/**
 * @brief Medical triage priority categorization.
 */
enum class TriageCategory : uint8_t {
    CODE_GREEN = 0, ///< Non-urgent / minor
    CODE_AMBER,     ///< Urgent / delayed
    CODE_RED        ///< Immediate life threat / critical
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_TYPES_HPP
