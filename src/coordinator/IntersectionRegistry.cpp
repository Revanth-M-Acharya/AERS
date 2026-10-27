/**
 * @file IntersectionRegistry.cpp
 * @brief Implementation of IntersectionRegistry.
 */

#include "IntersectionRegistry.h"

namespace aegis {
namespace coordinator {

uint64_t IntersectionRegistry::CurrentTimestampMs() noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

IntersectionRegistry::IntersectionRegistry(IntersectionRegistry&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_intersections = std::move(other.m_intersections);
    m_clientIdToIntersectionId = std::move(other.m_clientIdToIntersectionId);
}

IntersectionRegistry& IntersectionRegistry::operator=(IntersectionRegistry&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_intersections = std::move(other.m_intersections);
        m_clientIdToIntersectionId = std::move(other.m_clientIdToIntersectionId);
    }
    return *this;
}

bool IntersectionRegistry::RegisterIntersection(const std::string& intersectionId,
                                                const std::string& ipAddress,
                                                uint16_t port,
                                                int clientId) {
    if (intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    uint64_t now = CurrentTimestampMs();

    if (it != m_intersections.end()) {
        // Update existing record
        it->second.ipAddress = ipAddress;
        it->second.port = port;
        it->second.isOnline = true;
        it->second.lastHeartbeatTimestamp = now;
        if (clientId >= 0) {
            it->second.clientId = clientId;
            m_clientIdToIntersectionId[clientId] = intersectionId;
        }
    } else {
        // Insert new record
        IntersectionEntry entry(intersectionId, ipAddress, port, true, now, clientId);
        m_intersections[intersectionId] = entry;
        if (clientId >= 0) {
            m_clientIdToIntersectionId[clientId] = intersectionId;
        }
    }

    return true;
}

bool IntersectionRegistry::RegisterIntersection(const IntersectionEntry& entry) {
    if (entry.intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections[entry.intersectionId] = entry;
    if (entry.clientId >= 0) {
        m_clientIdToIntersectionId[entry.clientId] = entry.intersectionId;
    }
    return true;
}

bool IntersectionRegistry::RemoveIntersection(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it == m_intersections.end()) {
        return false;
    }

    if (it->second.clientId >= 0) {
        m_clientIdToIntersectionId.erase(it->second.clientId);
    }
    m_intersections.erase(it);
    return true;
}

bool IntersectionRegistry::UpdateHeartbeat(const std::string& intersectionId, uint64_t timestamp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it == m_intersections.end()) {
        return false;
    }

    it->second.lastHeartbeatTimestamp = (timestamp == 0) ? CurrentTimestampMs() : timestamp;
    it->second.isOnline = true;
    return true;
}

bool IntersectionRegistry::GetIntersection(const std::string& intersectionId, IntersectionEntry& outEntry) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        outEntry = it->second;
        return true;
    }
    return false;
}

std::optional<IntersectionEntry> IntersectionRegistry::GetIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<IntersectionEntry> IntersectionRegistry::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<IntersectionEntry> entries;
    entries.reserve(m_intersections.size());
    for (const auto& pair : m_intersections) {
        entries.push_back(pair.second);
    }
    return entries;
}

bool IntersectionRegistry::HasIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.find(intersectionId) != m_intersections.end();
}

bool IntersectionRegistry::SetOnlineStatus(const std::string& intersectionId, bool isOnline) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        it->second.isOnline = isOnline;
        return true;
    }
    return false;
}

std::vector<std::string> IntersectionRegistry::CheckHeartbeatTimeouts(uint64_t timeoutIntervalMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> timedOutIds;
    uint64_t now = CurrentTimestampMs();

    for (auto& pair : m_intersections) {
        if (pair.second.isOnline) {
            uint64_t elapsed = (now >= pair.second.lastHeartbeatTimestamp)
                               ? (now - pair.second.lastHeartbeatTimestamp)
                               : 0;
            if (elapsed > timeoutIntervalMs) {
                pair.second.isOnline = false;
                timedOutIds.push_back(pair.first);
            }
        }
    }

    return timedOutIds;
}

bool IntersectionRegistry::AssociateClientId(const std::string& intersectionId, int clientId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_intersections.find(intersectionId);
    if (it != m_intersections.end()) {
        it->second.clientId = clientId;
        if (clientId >= 0) {
            m_clientIdToIntersectionId[clientId] = intersectionId;
        }
        return true;
    }
    return false;
}

bool IntersectionRegistry::FindByClientId(int clientId, std::string& outIntersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_clientIdToIntersectionId.find(clientId);
    if (it != m_clientIdToIntersectionId.end()) {
        outIntersectionId = it->second;
        return true;
    }
    return false;
}

size_t IntersectionRegistry::GetTotalCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.size();
}

size_t IntersectionRegistry::GetOnlineCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& pair : m_intersections) {
        if (pair.second.isOnline) {
            count++;
        }
    }
    return count;
}

void IntersectionRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersections.clear();
    m_clientIdToIntersectionId.clear();
}

} // namespace coordinator
} // namespace aegis
