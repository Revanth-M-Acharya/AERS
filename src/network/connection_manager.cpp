#include "aegis/network/connection_manager.hpp"
#include "aegis/common/time_utils.hpp"

namespace aegis {
namespace network {

ConnectionManager::ConnectionManager() = default;

void ConnectionManager::SetState(ConnectionState newState) {
    ConnectionStateCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state == newState) return;
        m_state = newState;
        cb = m_callback;
    }
    if (cb) {
        cb(newState);
    }
}

ConnectionState ConnectionManager::GetState() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

bool ConnectionManager::IsConnected() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_state == ConnectionState::CONNECTED);
}

void ConnectionManager::RecordKeepAlivePing() noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastPingMs = common::GetUtcMilliseconds();
}

uint64_t ConnectionManager::GetLastPingTimeMs() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastPingMs;
}

void ConnectionManager::SetStateCallback(ConnectionStateCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

} // namespace network
} // namespace aegis
