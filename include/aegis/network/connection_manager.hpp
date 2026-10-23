/**
 * @file connection_manager.hpp
 * @brief State machine and lifecycle manager for network link connectivity.
 */

#ifndef AEGIS_NETWORK_CONNECTION_MANAGER_HPP
#define AEGIS_NETWORK_CONNECTION_MANAGER_HPP

#include <string>
#include <functional>
#include <cstdint>
#include <mutex>

namespace aegis {
namespace network {

enum class ConnectionState : uint8_t {
    DISCONNECTED = 0,
    CONNECTING,
    CONNECTED,
    RECONNECTING,
    FAILED
};

using ConnectionStateCallback = std::function<void(ConnectionState newState)>;

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager() = default;

    void SetState(ConnectionState newState);
    ConnectionState GetState() const noexcept;
    bool IsConnected() const noexcept;

    void RecordKeepAlivePing() noexcept;
    uint64_t GetLastPingTimeMs() const noexcept;

    void SetStateCallback(ConnectionStateCallback callback);

private:
    ConnectionState m_state{ConnectionState::DISCONNECTED};
    uint64_t m_lastPingMs{0};
    ConnectionStateCallback m_callback;
    mutable std::mutex m_mutex;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_CONNECTION_MANAGER_HPP
