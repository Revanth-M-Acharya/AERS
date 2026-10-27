/**
 * @file mqtt_client.cpp
 * @brief Implementation of MqttClient delegating to interchangeable IMqttTransport backends.
 */

#include "aegis/network/mqtt_client.hpp"

namespace aegis {
namespace network {

class MqttClient::Impl {
public:
    explicit Impl(std::unique_ptr<IMqttTransport> t)
        : transport(t ? std::move(t) : std::make_unique<SimulationTransport>()) {}

    std::unique_ptr<IMqttTransport> transport;
};

MqttClient::MqttClient()
    : m_impl(std::make_unique<Impl>(std::make_unique<SimulationTransport>())) {}

MqttClient::MqttClient(std::unique_ptr<IMqttTransport> transport)
    : m_impl(std::make_unique<Impl>(std::move(transport))) {}

MqttClient::MqttClient(TransportType transportType)
    : m_impl(std::make_unique<Impl>(CreateMqttTransport(transportType))) {}

MqttClient::~MqttClient() = default;

void MqttClient::SetClientId(const std::string& clientId) {
    m_impl->transport->SetClientId(clientId);
}

void MqttClient::SetLastWill(const LastWill& will) {
    m_impl->transport->SetLastWill(will);
}

void MqttClient::SetRetryPolicy(const RetryPolicy& policy) {
    m_impl->transport->SetRetryPolicy(policy);
}

bool MqttClient::Connect(const std::string& host, int port, bool useTls) {
    return m_impl->transport->Connect(host, port, useTls);
}

void MqttClient::Disconnect() {
    m_impl->transport->Disconnect();
}

bool MqttClient::Reconnect() {
    return m_impl->transport->Reconnect();
}

bool MqttClient::Publish(const std::string& topic, const std::string& payload, int qos) {
    return m_impl->transport->Publish(topic, payload, qos);
}

bool MqttClient::Subscribe(const std::string& topicFilter, TopicHandler handler, int qos) {
    return m_impl->transport->Subscribe(topicFilter, std::move(handler), qos);
}

void MqttClient::Unsubscribe(const std::string& topicFilter) {
    m_impl->transport->Unsubscribe(topicFilter);
}

bool MqttClient::IsConnected() const noexcept {
    return m_impl->transport->IsConnected();
}

ConnectionState MqttClient::GetConnectionState() const noexcept {
    return m_impl->transport->GetConnectionState();
}

size_t MqttClient::GetBufferedMessageCount() const noexcept {
    return m_impl->transport->GetBufferedMessageCount();
}

void MqttClient::InjectIncomingMessage(const std::string& topic, const std::string& payload) {
    m_impl->transport->InjectIncomingMessage(topic, payload);
}

void MqttClient::SetTransport(std::unique_ptr<IMqttTransport> transport) {
    if (transport) {
        m_impl->transport = std::move(transport);
    }
}

IMqttTransport* MqttClient::GetTransport() noexcept {
    return m_impl->transport.get();
}

const IMqttTransport* MqttClient::GetTransport() const noexcept {
    return m_impl->transport.get();
}

TransportType MqttClient::GetTransportType() const noexcept {
    return m_impl->transport ? m_impl->transport->GetType() : TransportType::SIMULATION;
}

} // namespace network
} // namespace aegis
