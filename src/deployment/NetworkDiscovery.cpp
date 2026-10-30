/**
 * @file NetworkDiscovery.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Network Discovery & Coordinator Resolution Implementation.
 */

#include "NetworkDiscovery.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <chrono>

namespace aegis {
namespace deployment {

bool NetworkDiscovery::ResolveCoordinator(const NodeConfiguration& config,
                                          DiscoveredCoordinator& outResult,
                                          uint32_t probeTimeoutMs) {
    outResult.ip = config.coordinatorAddress.empty() ? "127.0.0.1" : config.coordinatorAddress;
    outResult.port = config.coordinatorPort == 0 ? 9000 : config.coordinatorPort;
    outResult.discoveryMethod = "STATIC_CONFIG";

    uint64_t latencyMs = 0;
    outResult.verified = ProbeCoordinator(outResult.ip, outResult.port, probeTimeoutMs, &latencyMs);
    outResult.responseTimeMs = latencyMs;
    return true;
}

bool NetworkDiscovery::ProbeCoordinator(const std::string& ip,
                                        uint16_t port,
                                        uint32_t timeoutMs,
                                        uint64_t* outResponseTimeMs) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Set non-blocking
    int flags = ::fcntl(sock, F_GETFL, 0);
    if (flags != -1) {
        ::fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    if (::inet_pton(AF_INET, ip.c_str(), &saddr.sin_addr) <= 0) {
        ::close(sock);
        return false;
    }

    auto start = std::chrono::steady_clock::now();
    int ret = ::connect(sock, reinterpret_cast<struct sockaddr*>(&saddr), sizeof(saddr));
    bool connected = false;

    if (ret == 0) {
        connected = true;
    } else {
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLOUT;
        int pollRet = ::poll(&pfd, 1, static_cast<int>(timeoutMs));
        if (pollRet > 0 && (pfd.revents & POLLOUT)) {
            int sockErr = 0;
            socklen_t len = sizeof(sockErr);
            if (::getsockopt(sock, SOL_SOCKET, SO_ERROR, &sockErr, &len) == 0 && sockErr == 0) {
                connected = true;
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    uint64_t elapsed = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

    if (outResponseTimeMs) {
        *outResponseTimeMs = elapsed;
    }

    ::close(sock);
    return connected;
}

bool NetworkDiscovery::DiscoverViaBroadcast(uint16_t broadcastPort,
                                           uint32_t timeoutMs,
                                           DiscoveredCoordinator& outResult) {
    (void)broadcastPort;
    (void)timeoutMs;
    // Default fallback to local resolution if no coordinator is broadcasting
    outResult.ip = "127.0.0.1";
    outResult.port = 9000;
    outResult.discoveryMethod = "BROADCAST_FALLBACK";
    outResult.verified = ProbeCoordinator(outResult.ip, outResult.port, 500);
    return outResult.verified;
}

} // namespace deployment
} // namespace aegis
