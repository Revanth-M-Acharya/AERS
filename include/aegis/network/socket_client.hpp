/**
 * @file socket_client.hpp
 * @brief Lightweight POSIX UDP client for direct V2V / V2I ad-hoc beacons.
 */

#ifndef AEGIS_NETWORK_SOCKET_CLIENT_HPP
#define AEGIS_NETWORK_SOCKET_CLIENT_HPP

#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include "aegis/common/error_codes.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace network {

class SocketClient {
public:
    SocketClient();
    ~SocketClient();

    common::Result<void> OpenUdp(bool enableBroadcast = true);
    common::Result<void> SendTo(const std::string& destinationIp, uint16_t port, std::string_view data);
    void Close();

    bool IsOpen() const noexcept;

private:
    int m_sockfd{-1};
    bool m_useLoopback{false};
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_SOCKET_CLIENT_HPP
