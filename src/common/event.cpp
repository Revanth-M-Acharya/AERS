#include "aegis/common/event.hpp"
#include "aegis/common/uuid_utils.hpp"
#include "aegis/common/time_utils.hpp"
#include <sstream>

namespace aegis {
namespace common {

Event::Event(std::string_view eventType)
    : m_eventId(GenerateEventId("evt")),
      m_eventType(eventType),
      m_timestampMs(GetUtcMilliseconds()) {}

std::string_view Event::EventId() const noexcept { return m_eventId; }
std::string_view Event::EventType() const noexcept { return m_eventType; }
uint64_t Event::TimestampMs() const noexcept { return m_timestampMs; }

std::string Event::ToJson() const {
    std::ostringstream oss;
    oss << "{\"eventId\":\"" << m_eventId << "\","
        << "\"eventType\":\"" << m_eventType << "\","
        << "\"timestampMs\":" << m_timestampMs << "}";
    return oss.str();
}

} // namespace common
} // namespace aegis
