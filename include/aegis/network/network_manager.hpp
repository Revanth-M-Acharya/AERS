#ifndef AEGIS_NETWORK_NETWORK_MANAGER_HPP
#define AEGIS_NETWORK_NETWORK_MANAGER_HPP

#include <memory>
#include "aegis/network/mqtt_client.hpp"

namespace aegis {
namespace network {

/**
 * @brief Unified network manager with local store-and-forward buffering.
 * TODO: Implement circular ring-buffer queue for cellular dead-zone caching.
 */
class NetworkManager {
public:
    NetworkManager();
    ~NetworkManager();

    bool Initialize(const std::string& host, int port, bool useTls = false);
    MqttClient& GetMqttClient();

private:
    std::unique_ptr<MqttClient> m_mqttClient;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_NETWORK_MANAGER_HPP
