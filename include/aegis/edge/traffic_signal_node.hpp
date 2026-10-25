/**
 * @file traffic_signal_node.hpp
 * @brief High-level roadside traffic signal node with MQTT V2I communication and Coordinator integration.
 */

#ifndef AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP
#define AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP

#include <memory>
#include <atomic>
#include <cstdint>
#include <string>
#include <mutex>

#include "aegis/edge/traffic_controller.hpp"
#include "aegis/network/network_manager.hpp"
#include "aegis/network/serializer.hpp"

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Roadside traffic signal node encapsulating controller lifecycle, deterministic stepping,
 * and MQTT V2I communication with the Central Coordinator.
 */
class TrafficSignalNode {
public:
    TrafficSignalNode();
    explicit TrafficSignalNode(std::unique_ptr<TrafficController> controller);
    explicit TrafficSignalNode(const TrafficTimingConfig& config);
    explicit TrafficSignalNode(const std::string& intersectionId,
                               const TrafficTimingConfig& config = TrafficTimingConfig{});
    TrafficSignalNode(const std::string& intersectionId,
                      std::unique_ptr<TrafficController> controller,
                      std::shared_ptr<network::NetworkManager> netManager = nullptr);
    TrafficSignalNode(const std::string& intersectionId,
                      std::shared_ptr<network::NetworkManager> netManager);
    ~TrafficSignalNode();

    // Non-copyable, non-movable daemon node
    TrafficSignalNode(const TrafficSignalNode&) = delete;
    TrafficSignalNode& operator=(const TrafficSignalNode&) = delete;
    TrafficSignalNode(TrafficSignalNode&&) = delete;
    TrafficSignalNode& operator=(TrafficSignalNode&&) = delete;

    /**
     * @brief Starts the underlying traffic controller and arms execution state.
     * @return true if started successfully.
     */
    bool Start();

    /**
     * @brief Stops the traffic controller and execution loop.
     */
    void Stop();

    /**
     * @brief Executes the signal node cycle.
     * @param iterations Number of iterations to execute (0 = run continuously until Stop() is called).
     * @param stepMs Delta time in milliseconds advanced per tick step.
     */
    void Run(uint32_t iterations = 0, uint32_t stepMs = 100);

    /**
     * @brief Advances the traffic controller and network timers by a single time increment.
     * @param deltaMs Elapsed duration in milliseconds.
     */
    void Tick(uint32_t deltaMs = 100);

    // MQTT Integration
    bool Connect(const std::string& host = "127.0.0.1", int port = 1883, bool useTls = false);
    void Disconnect();
    bool Reconnect();
    bool Subscribe();
    bool PublishStatus(bool force = false);
    bool ProcessCommand(const std::string& commandPayload);
    bool SendHeartbeat();

    // Diagnostics & State
    bool IsRunning() const noexcept;
    bool IsConnected() const noexcept;
    SignalState GetCurrentState() const;
    std::string GetHealthStatus() const;

    TrafficController& GetController() noexcept;
    const TrafficController& GetController() const noexcept;

    std::shared_ptr<network::NetworkManager> GetNetworkManager() const noexcept { return m_network; }

    const std::string& GetIntersectionId() const noexcept { return m_intersectionId; }
    void SetIntersectionId(const std::string& id);

    uint64_t GetHeartbeatCounter() const noexcept { return m_heartbeatCounter.load(); }
    uint64_t GetUptimeMs() const noexcept { return m_uptimeMs.load(); }
    const std::string& GetCommandTopic() const noexcept { return m_commandTopic; }
    const std::string& GetStatusTopic() const noexcept { return m_statusTopic; }

private:
    void SetupTopics();

    mutable std::mutex m_nodeMutex;
    std::string m_intersectionId{"INT-01"};
    std::string m_commandTopic{"aegis/intersection/INT-01/commands"};
    std::string m_statusTopic{"aegis/intersection/INT-01/status"};

    std::unique_ptr<TrafficController> m_controller;
    std::shared_ptr<network::NetworkManager> m_network;
    bool m_ownsNetwork{true};

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isSubscribed{false};
    std::atomic<bool> m_isHeartbeatTimedOut{false};

    std::atomic<uint64_t> m_heartbeatCounter{0};
    std::atomic<uint64_t> m_uptimeMs{0};

    uint32_t m_timeSinceLastHeartbeatPublishMs{0};
    uint32_t m_timeSinceLastCoordinatorHeartbeatMs{0};

    std::string m_lastPublishedPayload;
    std::string m_lastPublishedState;
    bool m_lastPublishedEmergency{false};
    bool m_lastPublishedRecovery{false};
    std::string m_lastPublishedHealth;
    uint64_t m_lastPublishedHbCounter{0};
    bool m_hasPublished{false};

    std::string m_brokerHost{"127.0.0.1"};
    int m_brokerPort{1883};
    bool m_brokerTls{false};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP
