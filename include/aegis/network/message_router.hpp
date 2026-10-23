/**
 * @file message_router.hpp
 * @brief Topic-based MQTT subscription router with single/multi-level wildcard matching.
 */

#ifndef AEGIS_NETWORK_MESSAGE_ROUTER_HPP
#define AEGIS_NETWORK_MESSAGE_ROUTER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <functional>
#include <mutex>

namespace aegis {
namespace network {

using TopicHandler = std::function<void(const std::string& topic, const std::string& payload)>;

class MessageRouter {
public:
    MessageRouter() = default;
    ~MessageRouter() = default;

    void Subscribe(const std::string& topicFilter, TopicHandler handler);
    void RouteMessage(const std::string& topic, const std::string& payload);
    void ClearSubscriptions();

    static bool MatchTopic(std::string_view topicFilter, std::string_view actualTopic) noexcept;

private:
    struct RouteEntry {
        std::string filter;
        TopicHandler handler;
    };

    std::vector<RouteEntry> m_routes;
    mutable std::mutex m_mutex;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MESSAGE_ROUTER_HPP
