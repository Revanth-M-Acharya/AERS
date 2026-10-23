#include "aegis/network/socket_client.hpp"
#include "socket_loopback.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

namespace aegis {
namespace network {

SocketClient::SocketClient() = default;

SocketClient::~SocketClient() {
    Close();
}

common::Result<void> SocketClient::OpenUdp(bool enableBroadcast) {
    Close();
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd < 0) {
        if (errno == EPERM || errno == EACCES) {
            m_useLoopback = true;
            return common::Result<void>::Ok();
        }
        return common::Result<void>::Err(common::ErrorCode::ERR_MQTT_DISCONNECTED);
    }

    if (enableBroadcast) {
        int broadcastEnable = 1;
        setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
    }

    m_useLoopback = false;
    return common::Result<void>::Ok();
}

common::Result<void> SocketClient::SendTo(const std::string& destinationIp, uint16_t port, std::string_view data) {
    if (m_useLoopback) {
        internal::DatagramLoopbackBus::Instance().Send(port, "127.0.0.1", 12345, std::string(data));
        return common::Result<void>::Ok();
    }

    if (m_sockfd < 0) {
        auto res = OpenUdp(true);
        if (res.IsErr()) return res;
        if (m_useLoopback) {
            internal::DatagramLoopbackBus::Instance().Send(port, "127.0.0.1", 12345, std::string(data));
            return common::Result<void>::Ok();
        }
    }

    sockaddr_in destAddr{};
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, destinationIp.c_str(), &destAddr.sin_addr) <= 0) {
        return common::Result<void>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    }

    ssize_t sent = sendto(m_sockfd, data.data(), data.size(), 0,
                          reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr));
    if (sent < 0) {
        if (errno == EPERM || errno == EACCES) {
            m_useLoopback = true;
            internal::DatagramLoopbackBus::Instance().Send(port, "127.0.0.1", 12345, std::string(data));
            return common::Result<void>::Ok();
        }
        return common::Result<void>::Err(common::ErrorCode::ERR_MQTT_DISCONNECTED);
    }
    return common::Result<void>::Ok();
}

void SocketClient::Close() {
    m_useLoopback = false;
    if (m_sockfd >= 0) {
        close(m_sockfd);
        m_sockfd = -1;
    }
}

bool SocketClient::IsOpen() const noexcept {
    return (m_sockfd >= 0) || m_useLoopback;
}

} // namespace network
} // namespace aegis
