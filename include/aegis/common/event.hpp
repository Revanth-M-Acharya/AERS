/**
 * @file event.hpp
 * @brief Base class for asynchronous system events.
 */

#ifndef AEGIS_COMMON_EVENT_HPP
#define AEGIS_COMMON_EVENT_HPP

#include <string>
#include <string_view>
#include <cstdint>

namespace aegis {
namespace common {

class Event {
public:
    explicit Event(std::string_view eventType);
    virtual ~Event() = default;

    std::string_view EventId() const noexcept;
    std::string_view EventType() const noexcept;
    uint64_t TimestampMs() const noexcept;

    virtual std::string ToJson() const;

private:
    std::string m_eventId;
    std::string m_eventType;
    uint64_t m_timestampMs{0};
};

} // namespace common
} // namespace aegis

#endif // AEGIS_COMMON_EVENT_HPP
