#ifndef AEGIS_NETWORK_MQTT_CLIENT_HPP
#define AEGIS_NETWORK_MQTT_CLIENT_HPP

#include <string>
#include <functional>
#include <memory>

namespace aegis {
namespace network {

using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

/**
 * @brief Thread-safe MQTT client wrapper with automatic TLS and reconnect logic.
 * TODO: Integrate libmosquitto or Paho MQTT C++ client library.
 */
class MqttClient {
public:
    MqttClient();
    ~MqttClient();

    bool Connect(const std::string& host, int port, bool useTls = false);
    void Disconnect();
    bool Publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool Subscribe(const std::string& topic, MessageCallback callback, int qos = 1);
    bool IsConnected() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MQTT_CLIENT_HPP
