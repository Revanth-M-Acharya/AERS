#include "aegis/network/socket_server.hpp"
#include "socket_loopback.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <array>

namespace aegis {
namespace network {

SocketServer::SocketServer() = default;

SocketServer::~SocketServer() {
    Close();
}

common::Result<void> SocketServer::BindUdp(uint16_t port) {
    Close();
    m_port = port;
    m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockfd < 0) {
        if (errno == EPERM || errno == EACCES) {
            m_useLoopback = true;
            return common::Result<void>::Ok();
        }
        return common::Result<void>::Err(common::ErrorCode::ERR_MQTT_DISCONNECTED);
    }

    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_sockfd, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        if (errno == EPERM || errno == EACCES) {
            Close();
            m_port = port;
            m_useLoopback = true;
            return common::Result<void>::Ok();
        }
        Close();
        return common::Result<void>::Err(common::ErrorCode::ERR_MQTT_DISCONNECTED);
    }

    m_useLoopback = false;
    return common::Result<void>::Ok();
}

bool SocketServer::PollOnce(uint32_t timeoutMs, SocketPacketHandler handler) {
    internal::DatagramPacket packet;
    if (internal::DatagramLoopbackBus::Instance().Receive(m_port, packet)) {
        if (handler) {
            handler(packet.senderIp, packet.senderPort, packet.payload);
        }
        return true;
    }

    if (m_sockfd < 0) return false;

    pollfd pfd{};
    pfd.fd = m_sockfd;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, static_cast<int>(timeoutMs));
    if (ret > 0 && (pfd.revents & POLLIN)) {
        std::array<char, 2048> buf{};
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);

        ssize_t received = recvfrom(m_sockfd, buf.data(), buf.size() - 1, 0,
                                    reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (received > 0) {
            buf[static_cast<size_t>(received)] = '\0';
            std::string senderIp = inet_ntoa(clientAddr.sin_addr);
            uint16_t senderPort = ntohs(clientAddr.sin_port);
            if (handler) {
                handler(senderIp, senderPort, std::string(buf.data(), static_cast<size_t>(received)));
            }
            return true;
        }
    }
    return false;
}

void SocketServer::Close() {
    if (m_useLoopback) {
        internal::DatagramLoopbackBus::Instance().Clear(m_port);
        m_useLoopback = false;
        m_port = 0;
    }
    if (m_sockfd >= 0) {
        close(m_sockfd);
        m_sockfd = -1;
    }
}

bool SocketServer::IsBound() const noexcept {
    return (m_sockfd >= 0) || m_useLoopback;
}

} // namespace network
} // namespace aegis
