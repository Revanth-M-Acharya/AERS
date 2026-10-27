/**
 * @file mosquitto_transport.hpp
 * @brief Production transport skeleton preparing SMART AEGIS for libmosquitto / POSIX broker deployment.
 */

#ifndef AEGIS_NETWORK_MOSQUITTO_TRANSPORT_HPP
#define AEGIS_NETWORK_MOSQUITTO_TRANSPORT_HPP

#include "aegis/network/mqtt_transport.hpp"
#include "aegis/common/circular_buffer.hpp"
#include <mutex>
#include <vector>

namespace aegis {
namespace network {

/**
 * @brief Clean transport skeleton interfacing with libmosquitto C client library.
 * Fully implements the IMqttTransport contract with complete documentation and extension hooks.
 * Does not require external libmosquitto headers or linking during Phase 5B / Pre-5C.
 */
class MosquittoTransport : public IMqttTransport {
public:
    MosquittoTransport();
    ~MosquittoTransport() override;

    bool Connect(const std::string& host, int port, bool useTls = false) override;
    void Disconnect() override;
    bool Reconnect() override;
    bool IsConnected() const noexcept override;

    bool Publish(const std::string& topic, const std::string& payload, int qos = 1) override;
    bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1) override;
    void Unsubscribe(const std::string& topicFilter) override;

    void SetClientId(const std::string& clientId) override;
    void SetLastWill(const LastWill& will) override;
    void SetRetryPolicy(const RetryPolicy& policy) override;

    ConnectionState GetConnectionState() const noexcept override;
    size_t GetBufferedMessageCount() const noexcept override;

    void InjectIncomingMessage(const std::string& topic, const std::string& payload) override;

    TransportType GetType() const noexcept override { return TransportType::MOSQUITTO; }
    std::string_view GetName() const noexcept override { return "MosquittoTransport"; }

    // Configuration inspection
    const std::string& GetHost() const noexcept { return m_host; }
    int GetPort() const noexcept { return m_port; }
    bool IsTlsEnabled() const noexcept { return m_useTls; }
    const std::string& GetClientId() const noexcept { return m_clientId; }

private:
    void FlushOfflineBuffer();

    struct SubscriptionRecord {
        std::string filter;
        TopicHandler handler;
        int qos{1};
    };

    std::string m_clientId{"AEGIS_MOSQ_CLIENT"};
    std::string m_host{"127.0.0.1"};
    int m_port{1883};
    bool m_useTls{false};

    LastWill m_will;
    RetryPolicy m_retryPolicy;
    ConnectionManager m_connManager;

    std::vector<SubscriptionRecord> m_subscriptions;
    common::CircularBuffer<MqttMessage, 1024> m_offlineBuffer;
    mutable std::mutex m_mutex;

    // Placeholder pointer for future struct mosquitto* context
    [[maybe_unused]] void* m_mosqContext{nullptr};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_MOSQUITTO_TRANSPORT_HPP
