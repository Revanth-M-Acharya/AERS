#include "aegis/network/mqtt_client.hpp"
#include <mutex>

namespace aegis {
namespace network {

class MqttClient::Impl {
public:
    std::string clientId{"AEGIS_CLIENT"};
    std::string host{"127.0.0.1"};
    int port{1883};
    bool useTls{false};

    LastWill will;
    RetryPolicy retryPolicy;
    ConnectionManager connManager;
    MessageRouter router;

    common::CircularBuffer<MqttMessage, 1024> offlineBuffer;
    mutable std::mutex bufferMutex;

    void FlushOfflineBuffer() {
        std::lock_guard<std::mutex> lock(bufferMutex);
        MqttMessage msg;
        while (offlineBuffer.Pop(msg)) {
            // Dispatched once restored
            router.RouteMessage(msg.topic, msg.payload);
        }
    }
};

MqttClient::MqttClient() : m_impl(std::make_unique<Impl>()) {}
MqttClient::~MqttClient() = default;

void MqttClient::SetClientId(const std::string& clientId) {
    m_impl->clientId = clientId;
}

void MqttClient::SetLastWill(const LastWill& will) {
    m_impl->will = will;
}

void MqttClient::SetRetryPolicy(const RetryPolicy& policy) {
    m_impl->retryPolicy = policy;
}

bool MqttClient::Connect(const std::string& host, int port, bool useTls) {
    m_impl->host = host;
    m_impl->port = port;
    m_impl->useTls = useTls;
    m_impl->connManager.SetState(ConnectionState::CONNECTED);
    m_impl->retryPolicy.Reset();
    m_impl->FlushOfflineBuffer();
    return true;
}

void MqttClient::Disconnect() {
    m_impl->connManager.SetState(ConnectionState::DISCONNECTED);
}

bool MqttClient::Reconnect() {
    if (!m_impl->retryPolicy.CanRetry()) {
        m_impl->connManager.SetState(ConnectionState::FAILED);
        return false;
    }
    m_impl->connManager.SetState(ConnectionState::RECONNECTING);
    m_impl->retryPolicy.RecordAttempt();
    m_impl->connManager.SetState(ConnectionState::CONNECTED);
    m_impl->FlushOfflineBuffer();
    return true;
}

bool MqttClient::Publish(const std::string& topic, const std::string& payload, int qos) {
    if (!m_impl->connManager.IsConnected()) {
        std::lock_guard<std::mutex> lock(m_impl->bufferMutex);
        m_impl->offlineBuffer.PushOverwrite({topic, payload, qos, false});
        return false;
    }

    // Direct loopback dispatch to local subscribers for simulation/embedded bus
    m_impl->router.RouteMessage(topic, payload);
    return true;
}

bool MqttClient::Subscribe(const std::string& topicFilter, TopicHandler handler, int /*qos*/) {
    m_impl->router.Subscribe(topicFilter, std::move(handler));
    return true;
}

bool MqttClient::IsConnected() const noexcept {
    return m_impl->connManager.IsConnected();
}

ConnectionState MqttClient::GetConnectionState() const noexcept {
    return m_impl->connManager.GetState();
}

size_t MqttClient::GetBufferedMessageCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_impl->bufferMutex);
    return m_impl->offlineBuffer.Size();
}

void MqttClient::InjectIncomingMessage(const std::string& topic, const std::string& payload) {
    m_impl->router.RouteMessage(topic, payload);
}

} // namespace network
} // namespace aegis
