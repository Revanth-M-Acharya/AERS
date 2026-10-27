/**
 * @file mosquitto_transport.cpp
 * @brief Implementation and documentation hooks for MosquittoTransport skeleton.
 */

#include "aegis/network/mosquitto_transport.hpp"
#include "aegis/network/simulation_transport.hpp"
#include "aegis/network/message_router.hpp"

namespace aegis {
namespace network {

MosquittoTransport::MosquittoTransport() = default;

MosquittoTransport::~MosquittoTransport() {
    Disconnect();
}

void MosquittoTransport::SetClientId(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clientId = clientId;
}

void MosquittoTransport::SetLastWill(const LastWill& will) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_will = will;
}

void MosquittoTransport::SetRetryPolicy(const RetryPolicy& policy) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_retryPolicy = policy;
}

bool MosquittoTransport::Connect(const std::string& host, int port, bool useTls) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_host = host;
    m_port = port;
    m_useTls = useTls;

    // ==============================================================================
    // [HARDWARE HOOK - Phase 5C Deployment]
    // 1. mosquitto_lib_init();
    // 2. m_mosqContext = mosquitto_new(m_clientId.c_str(), true, this);
    // 3. if (m_useTls) mosquitto_tls_set(m_mosqContext, ...);
    // 4. if (!m_will.topic.empty()) mosquitto_will_set(m_mosqContext, m_will.topic.c_str(), ...);
    // 5. Set callbacks: mosquitto_connect_callback_set, mosquitto_message_callback_set
    // 6. mosquitto_connect_async(m_mosqContext, host.c_str(), port, 60);
    // 7. mosquitto_loop_start(m_mosqContext);
    // ==============================================================================

    m_connManager.SetState(ConnectionState::CONNECTED);
    m_retryPolicy.Reset();
    FlushOfflineBuffer();
    return true;
}

void MosquittoTransport::Disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connManager.IsConnected()) {
        // [HARDWARE HOOK - Phase 5C Deployment]
        // if (m_mosqContext) {
        //     mosquitto_disconnect(static_cast<struct mosquitto*>(m_mosqContext));
        //     mosquitto_loop_stop(static_cast<struct mosquitto*>(m_mosqContext), false);
        //     mosquitto_destroy(static_cast<struct mosquitto*>(m_mosqContext));
        //     m_mosqContext = nullptr;
        //     mosquitto_lib_cleanup();
        // }
        m_connManager.SetState(ConnectionState::DISCONNECTED);
    }
}

bool MosquittoTransport::Reconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_retryPolicy.CanRetry()) {
        m_connManager.SetState(ConnectionState::FAILED);
        return false;
    }

    m_connManager.SetState(ConnectionState::RECONNECTING);
    m_retryPolicy.RecordAttempt();

    // [HARDWARE HOOK - Phase 5C Deployment]
    // if (m_mosqContext) mosquitto_reconnect_async(static_cast<struct mosquitto*>(m_mosqContext));

    m_connManager.SetState(ConnectionState::CONNECTED);
    FlushOfflineBuffer();
    return true;
}

bool MosquittoTransport::IsConnected() const noexcept {
    return m_connManager.IsConnected();
}

ConnectionState MosquittoTransport::GetConnectionState() const noexcept {
    return m_connManager.GetState();
}

size_t MosquittoTransport::GetBufferedMessageCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_offlineBuffer.Size();
}

bool MosquittoTransport::Publish(const std::string& topic, const std::string& payload, int qos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connManager.IsConnected()) {
        m_offlineBuffer.PushOverwrite({topic, payload, qos, false});
        return false;
    }

    // ==============================================================================
    // [HARDWARE HOOK - Phase 5C Deployment]
    // int mid = 0;
    // int rc = mosquitto_publish(static_cast<struct mosquitto*>(m_mosqContext),
    //                            &mid, topic.c_str(), static_cast<int>(payload.size()),
    //                            payload.data(), qos, false);
    // return (rc == MOSQ_ERR_SUCCESS);
    // ==============================================================================

    // Dispatch to any local handlers matched (supporting in-tree unit testing)
    for (const auto& sub : m_subscriptions) {
        if (MessageRouter::MatchTopic(sub.filter, topic)) {
            if (sub.handler) {
                sub.handler(topic, payload);
            }
        }
    }
    return true;
}

bool MosquittoTransport::Subscribe(const std::string& topicFilter, TopicHandler handler, int qos) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscriptions.push_back({topicFilter, std::move(handler), qos});

    // ==============================================================================
    // [HARDWARE HOOK - Phase 5C Deployment]
    // if (m_mosqContext && m_connManager.IsConnected()) {
    //     int mid = 0;
    //     mosquitto_subscribe(static_cast<struct mosquitto*>(m_mosqContext), &mid, topicFilter.c_str(), qos);
    // }
    // ==============================================================================

    return true;
}

void MosquittoTransport::Unsubscribe(const std::string& topicFilter) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_subscriptions.begin(); it != m_subscriptions.end();) {
        if (it->filter == topicFilter) {
            it = m_subscriptions.erase(it);
        } else {
            ++it;
        }
    }
}

void MosquittoTransport::InjectIncomingMessage(const std::string& topic, const std::string& payload) {
    std::vector<TopicHandler> matched;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& sub : m_subscriptions) {
            if (MessageRouter::MatchTopic(sub.filter, topic)) {
                matched.push_back(sub.handler);
            }
        }
    }
    for (const auto& handler : matched) {
        if (handler) {
            handler(topic, payload);
        }
    }
}

void MosquittoTransport::FlushOfflineBuffer() {
    MqttMessage msg;
    while (m_offlineBuffer.Pop(msg)) {
        for (const auto& sub : m_subscriptions) {
            if (MessageRouter::MatchTopic(sub.filter, msg.topic)) {
                if (sub.handler) {
                    sub.handler(msg.topic, msg.payload);
                }
            }
        }
    }
}

std::unique_ptr<IMqttTransport> CreateMqttTransport(TransportType type) {
    switch (type) {
        case TransportType::SIMULATION:
            return std::make_unique<SimulationTransport>();
        case TransportType::MOSQUITTO:
            return std::make_unique<MosquittoTransport>();
        default:
            return std::make_unique<SimulationTransport>();
    }
}

} // namespace network
} // namespace aegis
