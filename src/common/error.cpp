#include "aegis/common/error.hpp"
#include <chrono>
#include <sstream>

namespace aegis {
namespace common {

AegisError::AegisError() : m_timestampMs(0) {}

AegisError::AegisError(ErrorCode code, ErrorSeverity severity, std::string_view originModule, std::string_view message)
    : m_code(code),
      m_severity(severity),
      m_originModule(originModule),
      m_message(message) {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    m_timestampMs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

ErrorCode AegisError::Code() const noexcept { return m_code; }
ErrorSeverity AegisError::Severity() const noexcept { return m_severity; }
std::string_view AegisError::OriginModule() const noexcept { return m_originModule; }
std::string_view AegisError::Message() const noexcept { return m_message; }
uint64_t AegisError::TimestampMs() const noexcept { return m_timestampMs; }

std::string AegisError::ToString() const {
    std::ostringstream oss;
    oss << "[" << ::aegis::common::ToString(m_code) << "] [" << m_originModule << "] " << m_message;
    return oss.str();
}

} // namespace common
} // namespace aegis
