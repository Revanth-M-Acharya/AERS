#ifndef AEGIS_NETWORK_NETWORK_MANAGER_HPP
#define AEGIS_NETWORK_NETWORK_MANAGER_HPP

#include "mqtt_client.hpp"

namespace aegis {
namespace network {

class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize();
    MqttClient& GetMqttClient();

private:
    MqttClient m_mqttClient;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_NETWORK_MANAGER_HPP
