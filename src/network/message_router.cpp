#include "aegis/network/message_router.hpp"
#include <sstream>

namespace aegis {
namespace network {

namespace {
std::vector<std::string> SplitTopic(std::string_view topic) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : topic) {
        if (c == '/') {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}
} // namespace

void MessageRouter::Subscribe(const std::string& topicFilter, TopicHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routes.push_back({topicFilter, std::move(handler)});
}

void MessageRouter::RouteMessage(const std::string& topic, const std::string& payload) {
    std::vector<TopicHandler> matchedHandlers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& entry : m_routes) {
            if (MatchTopic(entry.filter, topic)) {
                matchedHandlers.push_back(entry.handler);
            }
        }
    }

    for (const auto& handler : matchedHandlers) {
        if (handler) {
            handler(topic, payload);
        }
    }
}

void MessageRouter::ClearSubscriptions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routes.clear();
}

bool MessageRouter::MatchTopic(std::string_view topicFilter, std::string_view actualTopic) noexcept {
    if (topicFilter == actualTopic) {
        return true;
    }

    auto filterTokens = SplitTopic(topicFilter);
    auto topicTokens = SplitTopic(actualTopic);

    size_t i = 0;
    size_t j = 0;

    while (i < filterTokens.size() && j < topicTokens.size()) {
        if (filterTokens[i] == "#") {
            return true; // Multi-level wildcard matches everything remaining
        }
        if (filterTokens[i] != "+" && filterTokens[i] != topicTokens[j]) {
            return false;
        }
        ++i;
        ++j;
    }

    if (i < filterTokens.size() && filterTokens[i] == "#") {
        return true;
    }

    return (i == filterTokens.size() && j == topicTokens.size());
}

} // namespace network
} // namespace aegis
