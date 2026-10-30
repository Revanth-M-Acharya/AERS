/**
 * @file NodeRegistrationManager.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Central Coordinator Node Registration Service Implementation.
 */

#include "aegis/coordinator/NodeRegistrationManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <sstream>
#include <iomanip>

namespace aegis {
namespace coordinator {

NodeRegistrationManager::NodeRegistrationManager() = default;

bool NodeRegistrationManager::RegisterNode(const NodeRegistrationRecord& record, std::string& outError) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (record.nodeId.empty()) {
        outError = "Node ID cannot be empty.";
        return false;
    }

    auto it = m_nodes.find(record.nodeId);
    if (it != m_nodes.end()) {
        // If the node is currently marked ONLINE from a DIFFERENT IP, reject duplicate!
        if (it->second.onlineState == NodeOnlineState::ONLINE && it->second.ipAddress != record.ipAddress) {
            outError = "Duplicate Node ID collision: " + record.nodeId +
                       " is already active from " + it->second.ipAddress +
                       "; rejected registration from " + record.ipAddress;
            logger::Logger::Instance().Log(logger::LogLevel::ERR, "NODE_REG", outError);
            return false;
        }
    }

    NodeRegistrationRecord r = record;
    uint64_t nowUs = common::GetMonotonicMicroseconds();
    if (r.bootTimestampUs == 0) {
        r.bootTimestampUs = nowUs;
    }
    r.registeredTimestampUs = nowUs;
    r.lastHeartbeatTimestampUs = nowUs;
    r.onlineState = NodeOnlineState::ONLINE;
    r.inRecovery = false;
    if (r.currentStatus.empty()) {
        r.currentStatus = "READY";
    }

    if (r.capabilities.empty()) {
        r.capabilities = {"RELAY_4CH", "OPTICAL_IR_DUAL", "GREEN_WAVE"};
    }

    m_nodes[record.nodeId] = r;
    outError.clear();

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "NODE_REG",
        "Registered node: " + r.nodeId + " [Hostname: " + r.hostname +
        ", MAC: " + r.macAddress + ", IP: " + r.ipAddress +
        ", SW: " + r.softwareVersion + ", HW: " + r.hardwareVersion +
        ", Status: " + r.currentStatus +
        ", Relays: " + r.relayStatus + ", GPIO: " + r.gpioStatus + "]");

    if (m_callback) {
        m_callback(r.nodeId, NodeOnlineState::ONLINE, "Node registered successfully");
    }

    return true;
}

bool NodeRegistrationManager::UnregisterNode(const std::string& nodeId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }

    it->second.onlineState = NodeOnlineState::OFFLINE;
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "NODE_REG",
        "Unregistered node: " + nodeId + " (" + reason + ")");

    if (m_callback) {
        m_callback(nodeId, NodeOnlineState::OFFLINE, reason);
    }

    return true;
}

bool NodeRegistrationManager::RecordHeartbeat(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }

    it->second.lastHeartbeatTimestampUs = common::GetMonotonicMicroseconds();
    if (it->second.onlineState != NodeOnlineState::ONLINE) {
        it->second.onlineState = NodeOnlineState::ONLINE;
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "NODE_REG",
            "Node " + nodeId + " recovered back to ONLINE state via heartbeat.");
        if (m_callback) {
            m_callback(nodeId, NodeOnlineState::ONLINE, "Heartbeat restored");
        }
    }
    return true;
}

size_t NodeRegistrationManager::PruneStaleNodes(double timeoutSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    uint64_t nowUs = common::GetMonotonicMicroseconds();
    uint64_t timeoutUs = static_cast<uint64_t>(timeoutSeconds * 1000000.0);
    size_t staleCount = 0;

    for (auto& [nodeId, record] : m_nodes) {
        if (record.onlineState == NodeOnlineState::ONLINE) {
            if (nowUs > record.lastHeartbeatTimestampUs &&
                (nowUs - record.lastHeartbeatTimestampUs) > timeoutUs) {
                record.onlineState = NodeOnlineState::OFFLINE;
                staleCount++;

                double ageSec = static_cast<double>(nowUs - record.lastHeartbeatTimestampUs) / 1000000.0;
                std::string msg = "Heartbeat timeout (" + std::to_string(ageSec) +
                                  "s > " + std::to_string(timeoutSeconds) + "s)";
                logger::Logger::Instance().Log(
                    logger::LogLevel::WARN, "NODE_REG",
                    "Node " + nodeId + " marked OFFLINE: " + msg);

                if (m_callback) {
                    m_callback(nodeId, NodeOnlineState::OFFLINE, msg);
                }
            }
        }
    }

    return staleCount;
}

bool NodeRegistrationManager::SetNodeRecoveryState(const std::string& nodeId, bool inRecovery) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }
    it->second.inRecovery = inRecovery;
    return true;
}

bool NodeRegistrationManager::IsNodeRegistered(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nodes.find(nodeId) != m_nodes.end();
}

bool NodeRegistrationManager::IsNodeOnline(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }
    return it->second.onlineState == NodeOnlineState::ONLINE;
}

bool NodeRegistrationManager::GetNodeRecord(const std::string& nodeId, NodeRegistrationRecord& outRecord) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) {
        return false;
    }
    outRecord = it->second;
    return true;
}

std::vector<NodeRegistrationRecord> NodeRegistrationManager::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<NodeRegistrationRecord> result;
    result.reserve(m_nodes.size());
    for (const auto& kv : m_nodes) {
        result.push_back(kv.second);
    }
    return result;
}

size_t NodeRegistrationManager::GetOnlineNodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& kv : m_nodes) {
        if (kv.second.onlineState == NodeOnlineState::ONLINE) count++;
    }
    return count;
}

size_t NodeRegistrationManager::GetTotalNodeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_nodes.size();
}

void NodeRegistrationManager::SetAlertCallback(NodeAlertCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(cb);
}

std::string NodeRegistrationManager::GenerateReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: REGISTERED INTERSECTION NODES\n";
    oss << " Total Nodes: " << m_nodes.size() << "\n";
    oss << "==========================================================\n";
    oss << std::left << std::setw(10) << "NODE_ID"
        << std::setw(14) << "HOSTNAME"
        << std::setw(16) << "IP_ADDRESS"
        << std::setw(18) << "MAC_ADDRESS"
        << std::setw(10) << "STATE"
        << std::setw(10) << "STATUS"
        << std::setw(16) << "RELAY_STATUS"
        << "GPIO_STATUS\n";
    oss << "------------------------------------------------------------------------------------------------\n";

    for (const auto& [nodeId, r] : m_nodes) {
        oss << std::left << std::setw(10) << r.nodeId
            << std::setw(14) << r.hostname
            << std::setw(16) << r.ipAddress
            << std::setw(18) << r.macAddress
            << std::setw(10) << NodeOnlineStateToString(r.onlineState)
            << std::setw(10) << r.currentStatus
            << std::setw(16) << r.relayStatus
            << r.gpioStatus << "\n";
    }
    oss << "==========================================================\n";

    return oss.str();
}

void NodeRegistrationManager::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodes.clear();
}

} // namespace coordinator
} // namespace aegis
