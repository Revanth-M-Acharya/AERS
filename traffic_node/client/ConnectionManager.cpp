/**
 * @file ConnectionManager.cpp
 * @brief Implementation of ConnectionManager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/client/ConnectionManager.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"

namespace aegis::traffic {

ConnectionManager::ConnectionManager() = default;

ConnectionState ConnectionManager::GetState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentState_;
}

bool ConnectionManager::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return (currentState_ == ConnectionState::CONNECTED);
}

void ConnectionManager::SetState(ConnectionState newState) {
    ConnectionState oldState;
    std::vector<ConnectionStateObserver> observersCopy;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (currentState_ == newState) {
            return;
        }
        oldState = currentState_;
        currentState_ = newState;
        observersCopy = observers_;
    }

    TrafficLogger::Instance().Info("CONN_MGR", "Connection State Changed: " +
                                  ConnectionStateToString(oldState) + " -> " +
                                  ConnectionStateToString(newState));

    for (const auto& observer : observersCopy) {
        if (observer) {
            observer(newState, oldState);
        }
    }
}

void ConnectionManager::RegisterObserver(ConnectionStateObserver observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (observer) {
        observers_.push_back(std::move(observer));
    }
}

} // namespace aegis::traffic

