#include "aegis/network/network_manager.hpp"

namespace aegis {
namespace network {

NetworkManager::NetworkManager()
    : m_mqtt(std::make_unique<MqttClient>()),
      m_auth(std::make_unique<AuthenticationManager>()),
      m_heartbeat(std::make_unique<HeartbeatManager>("AEGIS_NODE")),
      m_v2xClient(std::make_unique<SocketClient>()),
      m_v2xServer(std::make_unique<SocketServer>()) {}

NetworkManager::~NetworkManager() = default;

bool NetworkManager::Initialize(const std::string& host, int port, bool useTls, const std::string& nodeId) {
    m_heartbeat = std::make_unique<HeartbeatManager>(nodeId);
    m_mqtt->SetClientId(nodeId);
    return m_mqtt->Connect(host, port, useTls);
}

void NetworkManager::Shutdown() {
    m_mqtt->Disconnect();
    m_v2xClient->Close();
    m_v2xServer->Close();
}

bool NetworkManager::Publish(const std::string& topic, const std::string& payload, int qos) {
    return m_mqtt->Publish(topic, payload, qos);
}

bool NetworkManager::Subscribe(const std::string& topicFilter, TopicHandler handler, int qos) {
    return m_mqtt->Subscribe(topicFilter, std::move(handler), qos);
}

bool NetworkManager::SendV2xBeacon(uint16_t port, std::string_view payload) {
    return m_v2xClient->SendTo("255.255.255.255", port, payload).IsOk();
}

bool NetworkManager::StartV2xReceiver(uint16_t port, SocketPacketHandler handler) {
    auto res = m_v2xServer->BindUdp(port);
    if (res.IsErr()) return false;
    return m_v2xServer->PollOnce(10, std::move(handler));
}

AuthenticationManager& NetworkManager::Auth() {
    return *m_auth;
}

const AuthenticationManager& NetworkManager::Auth() const {
    return *m_auth;
}

HeartbeatManager& NetworkManager::Heartbeat() {
    return *m_heartbeat;
}

const HeartbeatManager& NetworkManager::Heartbeat() const {
    return *m_heartbeat;
}

bool NetworkManager::IsConnected() const noexcept {
    return m_mqtt->IsConnected();
}

MqttClient& NetworkManager::GetMqttClient() {
    return *m_mqtt;
}

} // namespace network
} // namespace aegis
