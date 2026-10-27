/**
 * @file mqtt_transport.hpp
 * @brief Abstract interface and common definitions for interchangeable MQTT transports.
 */

#ifndef AEGIS_NETWORK_MQTT_TRANSPORT_HPP
#define AEGIS_NETWORK_MQTT_TRANSPORT_HPP

#include <string>
#include <string_view>
#include <memory>
#include <functional>
#include <cstdint>

#include "aegis/network/connection_manager.hpp"
#include "aegis/network/retry_policy.hpp"
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

enum class TransportType : uint8_t {
    SIMULATION = 0,
    MOSQUITTO
};

/**
 * @brief Abstract interface for MQTT transport backends.
 * Enables zero-overhead switching between in-memory simulation and real physical broker clients (e.g. libmosquitto).
 */
class IMqttTransport {
public:
    virtual ~IMqttTransport() = default;

    virtual bool Connect(const std::string& host, int port, bool useTls = false) = 0;
    virtual void Disconnect() = 0;
    virtual bool Reconnect() = 0;
    virtual bool IsConnected() const noexcept = 0;

    virtual bool Publish(const std::string& topic, const std::string& payload, int qos = 1) = 0;
    virtual bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1) = 0;
    virtual void Unsubscribe(const std::string& topicFilter) = 0;

    virtual void SetClientId(const std::string& clientId) = 0;
    virtual void SetLastWill(const LastWill& will) = 0;
    virtual void SetRetryPolicy(const RetryPolicy& policy) = 0;

    virtual ConnectionState GetConnectionState() const noexcept = 0;
    virtual size_t GetBufferedMessageCount() const noexcept = 0;

    // Simulation / testing direct injection
    virtual void InjectIncomingMessage(const std::string& topic, const std::string& payload) = 0;

    virtual TransportType GetType() const noexcept = 0;
    virtual std::string_view GetName() const noexcept = 0;
};

/**
 * @brief Factory function creating an MQTT transport instance based on type.
 */
std::unique_ptr<IMqttTransport> CreateMqttTransport(TransportType type);

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MQTT_TRANSPORT_HPP
