/**
 * @file simulation_transport.hpp
 * @brief In-memory deterministic simulation transport implementation of IMqttTransport.
 */

#ifndef AEGIS_NETWORK_SIMULATION_TRANSPORT_HPP
#define AEGIS_NETWORK_SIMULATION_TRANSPORT_HPP

#include "aegis/network/mqtt_transport.hpp"
#include "aegis/common/circular_buffer.hpp"
#include <mutex>

namespace aegis {
namespace network {

/**
 * @brief In-memory loopback transport for deterministic testing and simulation.
 * Uses MessageRouter for topic matching and pattern dispatch.
 */
class SimulationTransport : public IMqttTransport {
public:
    SimulationTransport();
    ~SimulationTransport() override = default;

    bool Connect(const std::string& host, int port, bool useTls = false) override;
    void Disconnect() override;
    bool Reconnect() override;
    bool IsConnected() const noexcept override;

    bool Publish(const std::string& topic, const std::string& payload, int qos = 1) override;
    bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1) override;
    void Unsubscribe(const std::string& topicFilter) override;

    void SetClientId(const std::string& clientId) override;
    void SetLastWill(const LastWill& will) override;
    void SetRetryPolicy(const RetryPolicy& policy) override;

    ConnectionState GetConnectionState() const noexcept override;
    size_t GetBufferedMessageCount() const noexcept override;

    void InjectIncomingMessage(const std::string& topic, const std::string& payload) override;

    TransportType GetType() const noexcept override { return TransportType::SIMULATION; }
    std::string_view GetName() const noexcept override { return "SimulationTransport"; }

    MessageRouter& GetRouter() noexcept { return m_router; }
    const MessageRouter& GetRouter() const noexcept { return m_router; }

private:
    void FlushOfflineBuffer();

    std::string m_clientId{"AEGIS_SIM_CLIENT"};
    std::string m_host{"127.0.0.1"};
    int m_port{1883};
    bool m_useTls{false};

    LastWill m_will;
    RetryPolicy m_retryPolicy;
    ConnectionManager m_connManager;
    MessageRouter m_router;

    common::CircularBuffer<MqttMessage, 1024> m_offlineBuffer;
    mutable std::mutex m_bufferMutex;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_SIMULATION_TRANSPORT_HPP
