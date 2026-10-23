#include "aegis/network/network_manager.hpp"

namespace aegis {
namespace network {

NetworkManager::NetworkManager() : m_mqttClient(std::make_unique<MqttClient>()) {}
NetworkManager::~NetworkManager() = default;

bool NetworkManager::Initialize(const std::string& host, int port, bool useTls) {
    return m_mqttClient->Connect(host, port, useTls);
}

MqttClient& NetworkManager::GetMqttClient() {
    return *m_mqttClient;
}

} // namespace network
} // namespace aegis
