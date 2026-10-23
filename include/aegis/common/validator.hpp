/**
 * @file validator.hpp
 * @brief Input sanitization, numerical boundary checks, and CRC32 checksums.
 */

#ifndef AEGIS_COMMON_VALIDATOR_HPP
#define AEGIS_COMMON_VALIDATOR_HPP

#include <string_view>
#include <cstdint>
#include <cstddef>

namespace aegis {
namespace common {

class Validator {
public:
    template <typename T>
    static constexpr bool IsInRange(T value, T minVal, T maxVal) noexcept {
        return (value >= minVal && value <= maxVal);
    }

    static bool IsValidString(std::string_view str, size_t minLength, size_t maxLength) noexcept;
    static uint32_t ComputeCrc32(const uint8_t* data, size_t length) noexcept;
    static uint32_t ComputeCrc32(std::string_view text) noexcept;
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_VALIDATOR_HPP
