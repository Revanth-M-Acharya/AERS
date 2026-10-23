/**
 * @file mqtt_client.hpp
 * @brief Thread-safe MQTT client supporting QoS 0/1/2, LWT, reconnect, and offline ring buffering.
 */

#ifndef AEGIS_NETWORK_MQTT_CLIENT_HPP
#define AEGIS_NETWORK_MQTT_CLIENT_HPP

#include <string>
#include <functional>
#include <memory>
#include "aegis/common/circular_buffer.hpp"
#include "aegis/network/retry_policy.hpp"
#include "aegis/network/connection_manager.hpp"
#include "aegis/network/message_router.hpp"

namespace aegis {
namespace network {

struct MqttMessage {
    std::string topic;
    std::string payload;
    int qos{1};
    bool retained{false};
};

struct LastWill {
    std::string topic;
    std::string message;
    int qos{1};
    bool retained{false};
};

class MqttClient {
public:
    MqttClient();
    ~MqttClient();

    void SetClientId(const std::string& clientId);
    void SetLastWill(const LastWill& will);
    void SetRetryPolicy(const RetryPolicy& policy);

    bool Connect(const std::string& host, int port, bool useTls = false);
    void Disconnect();
    bool Reconnect();

    bool Publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1);

    bool IsConnected() const noexcept;
    ConnectionState GetConnectionState() const noexcept;
    size_t GetBufferedMessageCount() const noexcept;

    // Direct loopback for simulation & deterministic testing
    void InjectIncomingMessage(const std::string& topic, const std::string& payload);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MQTT_CLIENT_HPP
