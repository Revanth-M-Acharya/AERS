/**
 * @file serialization.hpp
 * @brief Abstract serialization contract for messages and telemetry packets.
 */

#ifndef AEGIS_COMMON_SERIALIZATION_HPP
#define AEGIS_COMMON_SERIALIZATION_HPP

#include <string>
#include <string_view>
#include "aegis/common/error_codes.hpp"

namespace aegis {
namespace common {

class ISerializable {
public:
    virtual ~ISerializable() = default;

    virtual std::string SerializeToJson() const = 0;
    virtual ErrorCode DeserializeFromJson(std::string_view json) = 0;
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_SERIALIZATION_HPP
