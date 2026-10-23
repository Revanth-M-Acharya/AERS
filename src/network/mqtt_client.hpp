#ifndef AEGIS_NETWORK_MQTT_CLIENT_HPP
#define AEGIS_NETWORK_MQTT_CLIENT_HPP

#include <string>
#include <functional>

namespace aegis {
namespace network {

using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

class MqttClient {
public:
    MqttClient();
    ~MqttClient();

    bool Connect(const std::string& host, int port, bool useTls = false);
    void Disconnect();
    bool Publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool Subscribe(const std::string& topic, MessageCallback callback, int qos = 1);

private:
    class Impl;
    Impl* m_impl{nullptr};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MQTT_CLIENT_HPP
