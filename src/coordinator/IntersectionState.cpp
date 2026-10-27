/**
 * @file IntersectionState.cpp
 * @brief Implementation of IntersectionState.
 */

#include "IntersectionState.h"

namespace aegis {
namespace coordinator {

const char* SignalStateToString(SignalState state) noexcept {
    switch (state) {
        case SignalState::RED:    return "RED";
        case SignalState::YELLOW: return "YELLOW";
        case SignalState::GREEN:  return "GREEN";
        default:                  return "UNKNOWN";
    }
}

SignalState StringToSignalState(const std::string& str) noexcept {
    if (str == "RED")    return SignalState::RED;
    if (str == "YELLOW") return SignalState::YELLOW;
    if (str == "GREEN")  return SignalState::GREEN;
    return SignalState::RED;
}

uint64_t IntersectionState::CurrentTimestampMs() noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

IntersectionState::IntersectionState(IntersectionState&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_states = std::move(other.m_states);
}

IntersectionState& IntersectionState::operator=(IntersectionState&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_states = std::move(other.m_states);
    }
    return *this;
}

bool IntersectionState::RegisterIntersection(const std::string& intersectionId, uint32_t initialPriority) {
    if (intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it != m_states.end()) {
        // Already registered, update priority if specified
        if (initialPriority > 0) {
            it->second.currentPriority = initialPriority;
        }
        return true;
    }

    uint64_t now = CurrentTimestampMs();
    m_states[intersectionId] = IntersectionStatus(intersectionId, SignalState::RED, true, now, initialPriority);
    return true;
}

bool IntersectionState::UpdateState(const std::string& intersectionId,
                                    SignalState state,
                                    uint64_t heartbeat,
                                    uint32_t priority) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it == m_states.end()) {
        return false;
    }

    it->second.signalState = state;
    it->second.lastHeartbeat = (heartbeat == 0) ? CurrentTimestampMs() : heartbeat;
    if (priority > 0) {
        it->second.currentPriority = priority;
    }
    it->second.isOnline = true;
    return true;
}

bool IntersectionState::GetState(const std::string& intersectionId, IntersectionStatus& outStatus) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it != m_states.end()) {
        outStatus = it->second;
        return true;
    }
    return false;
}

std::optional<IntersectionStatus> IntersectionState::GetState(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it != m_states.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool IntersectionState::SetOnline(const std::string& intersectionId, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it == m_states.end()) {
        return false;
    }

    it->second.isOnline = true;
    it->second.lastHeartbeat = (timestamp == 0) ? CurrentTimestampMs() : timestamp;
    return true;
}

bool IntersectionState::SetOffline(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_states.find(intersectionId);
    if (it == m_states.end()) {
        return false;
    }

    it->second.isOnline = false;
    return true;
}

bool IntersectionState::HasIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_states.find(intersectionId) != m_states.end();
}

std::vector<IntersectionStatus> IntersectionState::GetAllStates() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionStatus> result;
    result.reserve(m_states.size());
    for (const auto& pair : m_states) {
        result.push_back(pair.second);
    }
    return result;
}

size_t IntersectionState::GetTotalCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_states.size();
}

size_t IntersectionState::GetOnlineCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_states) {
        if (pair.second.isOnline) {
            count++;
        }
    }
    return count;
}

void IntersectionState::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_states.clear();
}

} // namespace coordinator
} // namespace aegis
