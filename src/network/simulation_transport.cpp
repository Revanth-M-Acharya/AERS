/**
 * @file simulation_transport.cpp
 * @brief Implementation of in-memory loopback SimulationTransport.
 */

#include "aegis/network/simulation_transport.hpp"

namespace aegis {
namespace network {

SimulationTransport::SimulationTransport() = default;

void SimulationTransport::SetClientId(const std::string& clientId) {
    m_clientId = clientId;
}

void SimulationTransport::SetLastWill(const LastWill& will) {
    m_will = will;
}

void SimulationTransport::SetRetryPolicy(const RetryPolicy& policy) {
    m_retryPolicy = policy;
}

bool SimulationTransport::Connect(const std::string& host, int port, bool useTls) {
    m_host = host;
    m_port = port;
    m_useTls = useTls;
    m_connManager.SetState(ConnectionState::CONNECTED);
    m_retryPolicy.Reset();
    FlushOfflineBuffer();
    return true;
}

void SimulationTransport::Disconnect() {
    m_connManager.SetState(ConnectionState::DISCONNECTED);
}

bool SimulationTransport::Reconnect() {
    if (!m_retryPolicy.CanRetry()) {
        m_connManager.SetState(ConnectionState::FAILED);
        return false;
    }
    m_connManager.SetState(ConnectionState::RECONNECTING);
    m_retryPolicy.RecordAttempt();
    m_connManager.SetState(ConnectionState::CONNECTED);
    FlushOfflineBuffer();
    return true;
}

bool SimulationTransport::IsConnected() const noexcept {
    return m_connManager.IsConnected();
}

ConnectionState SimulationTransport::GetConnectionState() const noexcept {
    return m_connManager.GetState();
}

size_t SimulationTransport::GetBufferedMessageCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_offlineBuffer.Size();
}

bool SimulationTransport::Publish(const std::string& topic, const std::string& payload, int qos) {
    if (!m_connManager.IsConnected()) {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_offlineBuffer.PushOverwrite({topic, payload, qos, false});
        return false;
    }

    // Direct loopback dispatch to local subscribers for simulation/embedded bus
    m_router.RouteMessage(topic, payload);
    return true;
}

bool SimulationTransport::Subscribe(const std::string& topicFilter, TopicHandler handler, int /*qos*/) {
    m_router.Subscribe(topicFilter, std::move(handler));
    return true;
}

void SimulationTransport::Unsubscribe(const std::string& /*topicFilter*/) {
    // Currently MessageRouter clear is available; per-filter unsubscribe can be expanded
}

void SimulationTransport::InjectIncomingMessage(const std::string& topic, const std::string& payload) {
    m_router.RouteMessage(topic, payload);
}

void SimulationTransport::FlushOfflineBuffer() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    MqttMessage msg;
    while (m_offlineBuffer.Pop(msg)) {
        m_router.RouteMessage(msg.topic, msg.payload);
    }
}

} // namespace network
} // namespace aegis
