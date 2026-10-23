#include "aegis/network/heartbeat_manager.hpp"
#include "aegis/common/time_utils.hpp"

namespace aegis {
namespace network {

HeartbeatManager::HeartbeatManager(std::string nodeId, uint32_t timeoutThresholdMs)
    : m_nodeId(std::move(nodeId)),
      m_timeoutMs(timeoutThresholdMs) {}

HeartbeatMsg HeartbeatManager::GenerateHeartbeat(float cpuUsagePct, float ramUsageMb, const std::string& status) {
    HeartbeatMsg msg;
    msg.nodeId = m_nodeId;
    msg.timestamp = common::GetUtcMilliseconds();
    msg.cpuUsagePct = cpuUsagePct;
    msg.ramUsageMb = ramUsageMb;
    msg.processStatus = status;
    msg.sequenceNumber = ++m_sequenceNumber;
    return msg;
}

void HeartbeatManager::RecordPeerHeartbeat(const HeartbeatMsg& msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_peerRegistry[msg.nodeId] = {msg.timestamp, msg.sequenceNumber, msg.processStatus};
}

bool HeartbeatManager::IsPeerAlive(const std::string& peerNodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_peerRegistry.find(peerNodeId);
    if (it == m_peerRegistry.end()) return false;

    uint64_t now = common::GetUtcMilliseconds();
    return (now >= it->second.lastTimestampMs) && ((now - it->second.lastTimestampMs) <= m_timeoutMs);
}

std::vector<std::string> HeartbeatManager::GetStalePeers() const {
    std::vector<std::string> stale;
    uint64_t now = common::GetUtcMilliseconds();

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& [nodeId, state] : m_peerRegistry) {
        if ((now - state.lastTimestampMs) > m_timeoutMs) {
            stale.push_back(nodeId);
        }
    }
    return stale;
}

} // namespace network
} // namespace aegis
