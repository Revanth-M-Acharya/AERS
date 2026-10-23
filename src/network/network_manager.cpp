#include "network_manager.hpp"

namespace aegis {
namespace network {

NetworkManager::NetworkManager() = default;
NetworkManager::~NetworkManager() = default;

bool NetworkManager::Initialize() {
    return true;
}

MqttClient& NetworkManager::GetMqttClient() {
    return m_mqttClient;
}

} // namespace network
} // namespace aegis
