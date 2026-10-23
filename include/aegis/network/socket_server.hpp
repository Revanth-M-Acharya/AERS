/**
 * @file socket_server.hpp
 * @brief Non-blocking POSIX UDP receiver for local V2V/V2I beacon listening.
 */

#ifndef AEGIS_NETWORK_SOCKET_SERVER_HPP
#define AEGIS_NETWORK_SOCKET_SERVER_HPP

#include <string>
#include <cstdint>
#include <functional>
#include "aegis/common/error_codes.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace network {

using SocketPacketHandler = std::function<void(const std::string& senderIp, uint16_t port, const std::string& payload)>;

class SocketServer {
public:
    SocketServer();
    ~SocketServer();

    common::Result<void> BindUdp(uint16_t port);
    bool PollOnce(uint32_t timeoutMs, SocketPacketHandler handler);
    void Close();

    bool IsBound() const noexcept;

private:
    int m_sockfd{-1};
    uint16_t m_port{0};
    bool m_useLoopback{false};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_SOCKET_SERVER_HPP
