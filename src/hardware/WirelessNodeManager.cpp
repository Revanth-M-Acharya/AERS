/**
 * @file WirelessNodeManager.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Wi-Fi TCP/IP Mesh Communication & Reconnection Implementation.
 */

#include "WirelessNodeManager.h"

namespace aegis {
namespace hardware {

WirelessNodeManager::WirelessNodeManager(std::string nodeId,
                                         std::shared_ptr<HardwareEventLogger> logger)
    : m_nodeId(std::move(nodeId))
    , m_logger(std::move(logger))
    , m_qualityMonitor(std::make_shared<WirelessQualityMonitor>())
{
    if (!m_logger) {
        m_logger = std::make_shared<HardwareEventLogger>();
    }
}

WirelessNodeManager::~WirelessNodeManager() {
    Disconnect();
}

bool WirelessNodeManager::ConfigureWirelessNode(const WifiNodeRecord& config) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_config = config;
    m_stats.localStaticIp = config.staticIp;
    m_stats.localPort = config.port;
    m_stats.remoteCoordinatorIp = config.coordinatorIp;
    m_stats.remoteCoordinatorPort = config.coordinatorPort;
    m_stats.rssiDbm = -55;
    m_stats.rttLatencyMs = 1.2;
    m_stats.packetLossPct = 0.0;
    return true;
}

bool WirelessNodeManager::Connect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_interrupted.load()) {
        m_logger->Warn("WIRELESS", m_nodeId + " cannot connect: Wi-Fi link interrupted.");
        return false;
    }

    m_stats.isConnected = true;
    m_stats.txPackets++;
    if (m_qualityMonitor) {
        m_qualityMonitor->SetConnected(true);
    }
    m_logger->Info("WIRELESS", m_nodeId + " connected via Wi-Fi TCP/IP to Coordinator at "
                               + m_config.coordinatorIp + ":" + std::to_string(m_config.coordinatorPort));
    return true;
}

void WirelessNodeManager::Disconnect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_stats.isConnected) {
        m_stats.isConnected = false;
        if (m_qualityMonitor) {
            m_qualityMonitor->SetConnected(false);
        }
        m_logger->Info("WIRELESS", m_nodeId + " disconnected from wireless network.");
    }
}

bool WirelessNodeManager::IsConnected() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_stats.isConnected && !m_interrupted.load();
}

void WirelessNodeManager::SendHeartbeat() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_stats.isConnected || m_interrupted.load()) {
        return;
    }
    m_stats.heartbeatCount++;
    m_stats.txPackets++;
    m_stats.lastHeartbeatAgeSec = 0.0;
    m_stats.nodeOffline = false;
}

void WirelessNodeManager::SendStatusUpdate(const std::string& statusJson) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_stats.isConnected || m_interrupted.load()) {
        return;
    }
    m_stats.txPackets++;
    m_logger->Info("WIRELESS", m_nodeId + " sent hardware status update: " + statusJson);
}

bool WirelessNodeManager::SendCommand(const std::string& targetNodeId,
                                      const std::string& command,
                                      const std::string& payload) {
    HardwareCommandCallback cb;
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (!m_stats.isConnected || m_interrupted.load()) {
            return false;
        }

        m_stats.txPackets++;
        m_logger->Info("WIRELESS", "Command to " + targetNodeId + ": " + command + " (" + payload + ")");
        cb = m_commandCallback;
    }

    if (cb) {
        cb(command, payload);
    }
    return true;
}

bool WirelessNodeManager::SendCommandWithRetry(const std::string& targetNodeId,
                                              const std::string& command,
                                              const std::string& payload,
                                              uint32_t maxRetries,
                                              uint32_t /*backoffMs*/) {
    for (uint32_t attempt = 0; attempt <= maxRetries; ++attempt) {
        if (SendCommand(targetNodeId, command, payload)) {
            return true;
        }
        {
            std::lock_guard<std::recursive_mutex> lock(m_mutex);
            m_stats.retryCount++;
            m_stats.droppedPackets++;
            m_logger->Warn("WIRELESS", m_nodeId + " command retry " + std::to_string(attempt + 1)
                                       + "/" + std::to_string(maxRetries) + " for " + command);
        }
    }
    return false;
}

bool WirelessNodeManager::SendAck(const std::string& command, bool success) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_stats.isConnected || m_interrupted.load()) {
        return false;
    }
    m_stats.txPackets++;
    m_logger->Info("WIRELESS", m_nodeId + " sent ACK for " + command + ": " + (success ? "SUCCESS" : "FAILURE"));
    return true;
}

void WirelessNodeManager::SetCommandCallback(HardwareCommandCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_commandCallback = std::move(callback);
}

void WirelessNodeManager::SimulateWifiInterruption(bool interrupted) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_interrupted.store(interrupted);
    if (interrupted) {
        m_stats.isConnected = false;
        m_stats.packetLossPct = 100.0;
        m_stats.rssiDbm = -99;
        m_logger->Error("WIRELESS", m_nodeId + " Wi-Fi connection severed (Link Down).");
    } else {
        m_logger->Info("WIRELESS", m_nodeId + " Wi-Fi radio restored (Link Up).");
    }
}

bool WirelessNodeManager::IsInterrupted() const {
    return m_interrupted.load();
}

void WirelessNodeManager::TriggerAutoReconnect() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stats.reconnectAttempts++;
    if (m_qualityMonitor) {
        m_qualityMonitor->RecordReconnect();
    }
    if (!m_interrupted.load()) {
        m_stats.isConnected = true;
        m_stats.packetLossPct = 0.0;
        m_stats.rssiDbm = -56;
        m_stats.nodeOffline = false;
        m_stats.lastHeartbeatAgeSec = 0.0;
        if (m_qualityMonitor) {
            m_qualityMonitor->SetConnected(true);
        }
        m_logger->Info("WIRELESS", m_nodeId + " auto-reconnected to Coordinator over Wi-Fi!");
    } else {
        m_logger->Warn("WIRELESS", m_nodeId + " auto-reconnect failed: Wi-Fi still down.");
    }
}

void WirelessNodeManager::RecordLatency(double rttMs) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stats.rttLatencyMs = rttMs;
    if (m_qualityMonitor) {
        m_qualityMonitor->RecordLatency(rttMs);
    }
}

void WirelessNodeManager::RecordPacketLoss(uint64_t dropped, uint64_t total) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stats.droppedPackets += dropped;
    if (total > 0) {
        m_stats.packetLossPct = (static_cast<double>(dropped) / static_cast<double>(total)) * 100.0;
    } else {
        m_stats.packetLossPct = 0.0;
    }
    if (m_qualityMonitor) {
        m_qualityMonitor->RecordSample(m_stats.rssiDbm, m_stats.rttLatencyMs, dropped > 0);
    }
}

void WirelessNodeManager::UpdateHeartbeatWatchdog(double dtSec, double timeoutSec) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stats.lastHeartbeatAgeSec += dtSec;
    if (m_stats.lastHeartbeatAgeSec >= timeoutSec && !m_stats.nodeOffline) {
        m_stats.nodeOffline = true;
        m_stats.isConnected = false;
        m_logger->Warn("WIRELESS", m_nodeId + " marked OFFLINE: No heartbeat for "
                                   + std::to_string(m_stats.lastHeartbeatAgeSec) + " sec.");
    }
}

bool WirelessNodeManager::IsNodeOffline() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_stats.nodeOffline;
}

void WirelessNodeManager::ResetOfflineState() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stats.nodeOffline = false;
    m_stats.lastHeartbeatAgeSec = 0.0;
    m_stats.isConnected = true;
}

WirelessLinkStats WirelessNodeManager::GetLinkStats() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_stats;
}

} // namespace hardware
} // namespace aegis
