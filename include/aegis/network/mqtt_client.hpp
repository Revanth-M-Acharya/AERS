/**
 * @file mqtt_client.hpp
 * @brief Thread-safe MQTT client supporting interchangeable transport backends (Simulation / Mosquitto).
 */

#ifndef AEGIS_NETWORK_MQTT_CLIENT_HPP
#define AEGIS_NETWORK_MQTT_CLIENT_HPP

#include <string>
#include <functional>
#include <memory>

#include "aegis/network/mqtt_transport.hpp"
#include "aegis/network/simulation_transport.hpp"
#include "aegis/network/mosquitto_transport.hpp"

namespace aegis {
namespace network {

/**
 * @brief High-level MQTT client facade delegating network operations to an IMqttTransport.
 * Defaults to SimulationTransport for host simulation, while enabling seamless runtime
 * switching to MosquittoTransport or custom hardware backends.
 */
class MqttClient {
public:
    MqttClient();
    explicit MqttClient(std::unique_ptr<IMqttTransport> transport);
    explicit MqttClient(TransportType transportType);
    ~MqttClient();

    // Non-copyable, movable
    MqttClient(const MqttClient&) = delete;
    MqttClient& operator=(const MqttClient&) = delete;
    MqttClient(MqttClient&&) noexcept = default;
    MqttClient& operator=(MqttClient&&) noexcept = default;

    void SetClientId(const std::string& clientId);
    void SetLastWill(const LastWill& will);
    void SetRetryPolicy(const RetryPolicy& policy);

    bool Connect(const std::string& host, int port, bool useTls = false);
    void Disconnect();
    bool Reconnect();

    bool Publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1);
    void Unsubscribe(const std::string& topicFilter);

    bool IsConnected() const noexcept;
    ConnectionState GetConnectionState() const noexcept;
    size_t GetBufferedMessageCount() const noexcept;

    // Direct loopback for simulation & deterministic testing
    void InjectIncomingMessage(const std::string& topic, const std::string& payload);

    // Transport management
    void SetTransport(std::unique_ptr<IMqttTransport> transport);
    IMqttTransport* GetTransport() noexcept;
    const IMqttTransport* GetTransport() const noexcept;
    TransportType GetTransportType() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MQTT_CLIENT_HPP
