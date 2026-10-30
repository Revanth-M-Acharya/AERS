/**
 * @file NetworkDiscovery.h
 * @brief SMART AEGIS (AERS) Phase 16D - Network Discovery & Coordinator Resolution.
 *
 * Implements deterministic coordinator discovery, socket availability probing,
 * and endpoint resolution for edge traffic nodes.
 */

#ifndef AEGIS_DEPLOYMENT_NETWORK_DISCOVERY_H
#define AEGIS_DEPLOYMENT_NETWORK_DISCOVERY_H

#include "NodeConfiguration.h"
#include <string>
#include <cstdint>

namespace aegis {
namespace deployment {

struct DiscoveredCoordinator {
    std::string ip{"127.0.0.1"};
    uint16_t port{9000};
    bool verified{false};
    uint64_t responseTimeMs{0};
    std::string discoveryMethod{"STATIC_CONFIG"};
};

class NetworkDiscovery {
public:
    NetworkDiscovery() = default;
    ~NetworkDiscovery() = default;

    /**
     * @brief Resolve the Central Coordinator endpoint from configuration and probe availability.
     * @param config The local node configuration.
     * @param outResult Populated DiscoveredCoordinator information.
     * @param probeTimeoutMs Maximum wait time in milliseconds for TCP connection probe.
     * @return true if coordinator endpoint is resolved (and reached if reachable); false if endpoint is malformed.
     */
    static bool ResolveCoordinator(const NodeConfiguration& config,
                                   DiscoveredCoordinator& outResult,
                                   uint32_t probeTimeoutMs = 1500);

    /**
     * @brief Probe whether a coordinator TCP service is actively accepting connections.
     * @param ip IPv4 host address.
     * @param port TCP port.
     * @param timeoutMs Timeout in milliseconds.
     * @param outResponseTimeMs Optional destination for measured round-trip probe latency.
     * @return true if port accepted TCP handshake; false on connection refusal/timeout.
     */
    static bool ProbeCoordinator(const std::string& ip,
                                 uint16_t port,
                                 uint32_t timeoutMs = 1500,
                                 uint64_t* outResponseTimeMs = nullptr);

    /**
     * @brief Broadcast a discovery inquiry over UDP broadcast.
     * @param broadcastPort Port to broadcast on (default 9876).
     * @param timeoutMs Discovery timeout in ms.
     * @param outResult Populated DiscoveredCoordinator info if announced.
     * @return true if a coordinator replied to the discovery broadcast; false otherwise.
     */
    static bool DiscoverViaBroadcast(uint16_t broadcastPort,
                                     uint32_t timeoutMs,
                                     DiscoveredCoordinator& outResult);
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_NETWORK_DISCOVERY_H
