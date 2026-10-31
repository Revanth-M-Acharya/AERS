/**
 * @file TrafficClient.hpp
 * @brief High-reliability TCP client with automatic reconnection and HELLO registration.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/network/TcpSocket.hpp"
#include "common/protocol/Packet.hpp"
#include "traffic_node/config/TrafficNodeConfig.hpp"
#include "traffic_node/client/ConnectionManager.hpp"
#include "traffic_node/client/HeartbeatManager.hpp"
#include "traffic_node/protocol/TrafficCommandDispatcher.hpp"
#include "traffic_node/signal_controller/SignalController.hpp"
#include "traffic_node/signal_controller/TrafficSignalFSM.hpp"
#include "traffic_node/watchdog/NodeWatchdog.hpp"
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace aegis::traffic {

class TrafficClient {
public:
    TrafficClient(TrafficNodeConfig config,
                  std::shared_ptr<SignalController> signalController,
                  std::shared_ptr<ConnectionManager> connectionManager = nullptr,
                  std::shared_ptr<NodeWatchdog> watchdog = nullptr);

    // Backward-compatible constructor accepting TrafficSignalFSM directly
    TrafficClient(TrafficNodeConfig config,
                  std::shared_ptr<TrafficSignalFSM> fsm,
                  std::shared_ptr<ConnectionManager> connectionManager = nullptr,
                  std::shared_ptr<NodeWatchdog> watchdog = nullptr);
    ~TrafficClient();

    // Prevent copying
    TrafficClient(const TrafficClient&) = delete;
    TrafficClient& operator=(const TrafficClient&) = delete;

    /**
     * @brief Start client connection supervisor, receiver loop, and heartbeat engine.
     */
    bool Start();

    /**
     * @brief Stop client gracefully, terminate threads, and close socket.
     */
    void Stop();

    /**
     * @brief Thread-safe packet transmission to Coordinator.
     */
    bool SendPacket(const protocol::Packet& packet);

    /**
     * @brief Manually trigger disconnect (for testing automatic reconnect).
     */
    void Disconnect();

    bool IsConnected() const;
    bool IsRegistered() const { return isRegistered_.load(); }
    bool IsRunning() const { return isRunning_.load(); }

    std::shared_ptr<ConnectionManager> GetConnectionManager() const { return connectionManager_; }
    std::shared_ptr<TrafficCommandDispatcher> GetDispatcher() const { return dispatcher_; }
    std::shared_ptr<SignalController> GetSignalController() const { return signalController_; }
    std::shared_ptr<NodeWatchdog> GetWatchdog() const { return watchdog_; }

    void SetWatchdog(std::shared_ptr<NodeWatchdog> watchdog) { watchdog_ = std::move(watchdog); }

    std::string GenerateEnrichedHeartbeatPayload() const;

private:
    void SupervisorLoop();
    void ReceiveLoop();
    bool PerformHelloHandshake();

    TrafficNodeConfig config_;
    std::shared_ptr<SignalController> signalController_;
    std::shared_ptr<TrafficSignalFSM> fsm_;
    std::shared_ptr<ConnectionManager> connectionManager_;
    std::shared_ptr<TrafficCommandDispatcher> dispatcher_;
    std::unique_ptr<HeartbeatManager> heartbeatManager_;
    std::shared_ptr<NodeWatchdog> watchdog_;

    network::TcpSocket socket_;
    std::atomic<bool> isRunning_{false};
    std::atomic<bool> isRegistered_{false};

    std::mutex sendMutex_;
    std::mutex rxMutex_;
    std::vector<uint8_t> rxBuffer_;

    std::thread supervisorThread_;
    std::thread receiveThread_;

    std::mutex cvMutex_;
    std::condition_variable cv_;
};

} // namespace aegis::traffic

