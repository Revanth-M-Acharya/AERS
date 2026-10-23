/**
 * @file heartbeat_manager.hpp
 * @brief Node heartbeat generation, peer tracking, and staleness detection.
 */

#ifndef AEGIS_NETWORK_HEARTBEAT_MANAGER_HPP
#define AEGIS_NETWORK_HEARTBEAT_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "aegis/network/serializer.hpp"

namespace aegis {
namespace network {

class HeartbeatManager {
public:
    explicit HeartbeatManager(std::string nodeId, uint32_t timeoutThresholdMs = 1000);
    ~HeartbeatManager() = default;

    HeartbeatMsg GenerateHeartbeat(float cpuUsagePct = 0.0f, float ramUsageMb = 0.0f, const std::string& status = "HEALTHY");
    void RecordPeerHeartbeat(const HeartbeatMsg& msg);
    bool IsPeerAlive(const std::string& peerNodeId) const;
    std::vector<std::string> GetStalePeers() const;

private:
    std::string m_nodeId;
    uint32_t m_timeoutMs;
    uint64_t m_sequenceNumber{0};

    struct PeerState {
        uint64_t lastTimestampMs;
        uint64_t lastSequence;
        std::string status;
    };

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, PeerState> m_peerRegistry;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_HEARTBEAT_MANAGER_HPP
