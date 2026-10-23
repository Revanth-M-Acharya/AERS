/**
 * @file error.hpp
 * @brief Standardized structured error descriptor.
 */

#ifndef AEGIS_COMMON_ERROR_HPP
#define AEGIS_COMMON_ERROR_HPP

#include <string>
#include <string_view>
#include <cstdint>
#include "aegis/common/error_codes.hpp"

namespace aegis {
namespace common {

enum class ErrorSeverity : uint8_t {
    INFO = 0,
    WARNING,
    ERROR,
    CRITICAL,
    FATAL
};

class AegisError {
public:
    AegisError();
    AegisError(ErrorCode code, ErrorSeverity severity, std::string_view originModule, std::string_view message);

    ErrorCode Code() const noexcept;
    ErrorSeverity Severity() const noexcept;
    std::string_view OriginModule() const noexcept;
    std::string_view Message() const noexcept;
    uint64_t TimestampMs() const noexcept;

    std::string ToString() const;

private:
    ErrorCode m_code{ErrorCode::SUCCESS};
    ErrorSeverity m_severity{ErrorSeverity::INFO};
    std::string m_originModule;
    std::string m_message;
    uint64_t m_timestampMs{0};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_ERROR_HPP
