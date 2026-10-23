#include "aegis/common/validator.hpp"

namespace aegis {
namespace common {

bool Validator::IsValidString(std::string_view str, size_t minLength, size_t maxLength) noexcept {
    return (str.size() >= minLength && str.size() <= maxLength);
}

uint32_t Validator::ComputeCrc32(const uint8_t* data, size_t length) noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

uint32_t Validator::ComputeCrc32(std::string_view text) noexcept {
    return ComputeCrc32(reinterpret_cast<const uint8_t*>(text.data()), text.size());
}

} // namespace common
} // namespace aegis
