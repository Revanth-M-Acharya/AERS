/**
 * @file uuid_utils.hpp
 * @brief RFC 4122 compliant UUIDv4 and identifier generation utilities.
 */

#ifndef AEGIS_COMMON_UUID_UTILS_HPP
#define AEGIS_COMMON_UUID_UTILS_HPP

#include <string>
#include <string_view>
#include <cstdint>

namespace aegis {
namespace common {

/**
 * @brief Generates an RFC 4122 compliant UUID version 4 string.
 */
std::string GenerateUuidV4();

/**
 * @brief Generates an Event Identifier with an optional prefix (e.g. evt_<uuid>).
 */
std::string GenerateEventId(std::string_view prefix = "evt");

/**
 * @brief Generates a structured Incident Identifier (e.g. inc_<uuid>).
 */
std::string GenerateIncidentId();

/**
 * @brief Formats an integer unit number into a standardized asset string (e.g. AMB-04).
 */
std::string FormatAmbulanceId(uint32_t unitNumber);

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_UUID_UTILS_HPP
