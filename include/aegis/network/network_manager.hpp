/**
 * @file network_manager.hpp
 * @brief Unified network facade coordinating MQTT, Sockets, Auth, and Heartbeat.
 */

#ifndef AEGIS_NETWORK_NETWORK_MANAGER_HPP
#define AEGIS_NETWORK_NETWORK_MANAGER_HPP

#include <string>
#include <memory>
#include "aegis/network/mqtt_client.hpp"
#include "aegis/network/authentication_manager.hpp"
#include "aegis/network/heartbeat_manager.hpp"
#include "aegis/network/socket_client.hpp"
#include "aegis/network/socket_server.hpp"

namespace aegis {
namespace network {

class NetworkManager {
public:
    NetworkManager();
    explicit NetworkManager(std::unique_ptr<IMqttTransport> transport);
    ~NetworkManager();

    bool Initialize(const std::string& host, int port, bool useTls = false, const std::string& nodeId = "NODE-01");
    bool InitializeWithTransport(std::unique_ptr<IMqttTransport> transport, const std::string& host, int port, bool useTls = false, const std::string& nodeId = "NODE-01");
    void Shutdown();

    // Central Telemetry & Commands (MQTT)
    bool Publish(const std::string& topic, const std::string& payload, int qos = 1);
    bool Subscribe(const std::string& topicFilter, TopicHandler handler, int qos = 1);

    // Direct V2X Ad-hoc Communications (UDP)
    bool SendV2xBeacon(uint16_t port, std::string_view payload);
    bool StartV2xReceiver(uint16_t port, SocketPacketHandler handler);

    // Authentication & Security
    AuthenticationManager& Auth();
    const AuthenticationManager& Auth() const;

    // Heartbeat & Node Status
    HeartbeatManager& Heartbeat();
    const HeartbeatManager& Heartbeat() const;

    // Diagnostics
    bool IsConnected() const noexcept;
    MqttClient& GetMqttClient();
    const MqttClient& GetMqttClient() const;

private:
    std::unique_ptr<MqttClient> m_mqtt;
    std::unique_ptr<AuthenticationManager> m_auth;
    std::unique_ptr<HeartbeatManager> m_heartbeat;
    std::unique_ptr<SocketClient> m_v2xClient;
    std::unique_ptr<SocketServer> m_v2xServer;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_NETWORK_MANAGER_HPP
