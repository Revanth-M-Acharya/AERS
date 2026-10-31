/**
 * @file ConnectionManager.hpp
 * @brief Connection state supervisor and transition notifier for Traffic Signal Node.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <mutex>
#include <functional>
#include <vector>

namespace aegis::traffic {

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    RECONNECTING
};

inline std::string ConnectionStateToString(ConnectionState state) {
    switch (state) {
        case ConnectionState::DISCONNECTED: return "DISCONNECTED";
        case ConnectionState::CONNECTING:   return "CONNECTING";
        case ConnectionState::CONNECTED:    return "CONNECTED";
        case ConnectionState::RECONNECTING: return "RECONNECTING";
        default:                            return "UNKNOWN";
    }
}

using ConnectionStateObserver = std::function<void(ConnectionState newState, ConnectionState oldState)>;

class ConnectionManager {
public:
    ConnectionManager();
    ~ConnectionManager() = default;

    ConnectionState GetState() const;
    bool IsConnected() const;

    /**
     * @brief Transition to a new connection state and broadcast notification to observers.
     */
    void SetState(ConnectionState newState);

    /**
     * @brief Register observer callback for connection state changes.
     */
    void RegisterObserver(ConnectionStateObserver observer);

private:
    mutable std::mutex mutex_;
    ConnectionState currentState_{ConnectionState::DISCONNECTED};
    std::vector<ConnectionStateObserver> observers_;
};

} // namespace aegis::traffic

