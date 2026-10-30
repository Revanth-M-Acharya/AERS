/**
 * @file WirelessNodeManager.h
 * @brief SMART AEGIS (AERS) Phase 20 - Wi-Fi TCP/IP Mesh Communication & Reconnection Manager.
 */

#ifndef AEGIS_HARDWARE_WIRELESS_NODE_MANAGER_H
#define AEGIS_HARDWARE_WIRELESS_NODE_MANAGER_H

#include "HardwareConfiguration.h"
#include "HardwareEventLogger.h"
#include "aegis/hardware/WirelessQualityMonitor.hpp"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <atomic>

namespace aegis {
namespace hardware {

struct WirelessLinkStats {
    bool isConnected{false};
    std::string localStaticIp{"192.168.1.101"};
    uint16_t localPort{9001};
    std::string remoteCoordinatorIp{"192.168.1.100"};
    uint16_t remoteCoordinatorPort{9000};
    int8_t rssiDbm{-58};
    double rttLatencyMs{1.4};
    double packetLossPct{0.0};
    uint64_t txPackets{0};
    uint64_t rxPackets{0};
    uint64_t heartbeatCount{0};
    uint64_t reconnectAttempts{0};
    uint64_t retryCount{0};
    uint64_t droppedPackets{0};
    bool nodeOffline{false};
    double lastHeartbeatAgeSec{0.0};
};

using HardwareCommandCallback = std::function<void(const std::string& command, const std::string& payload)>;

class WirelessNodeManager {
public:
    explicit WirelessNodeManager(std::string nodeId = "INT_A",
                                 std::shared_ptr<HardwareEventLogger> logger = nullptr);
    ~WirelessNodeManager();

    // Prevent copy/move
    WirelessNodeManager(const WirelessNodeManager&) = delete;
    WirelessNodeManager& operator=(const WirelessNodeManager&) = delete;

    // Initialization & Lifecycle
    bool ConfigureWirelessNode(const WifiNodeRecord& config);
    bool Connect();
    void Disconnect();
    bool IsConnected() const;

    // Heartbeat & Telemetry
    void SendHeartbeat();
    void SendStatusUpdate(const std::string& statusJson);

    // Command Dispatch & Handling
    bool SendCommand(const std::string& targetNodeId, const std::string& command, const std::string& payload = "");
    bool SendCommandWithRetry(const std::string& targetNodeId, const std::string& command,
                              const std::string& payload = "", uint32_t maxRetries = 3, uint32_t backoffMs = 50);
    bool SendAck(const std::string& command, bool success);
    void SetCommandCallback(HardwareCommandCallback callback);

    // Wi-Fi Fault Injection & Automatic Reconnection
    void SimulateWifiInterruption(bool interrupted);
    bool IsInterrupted() const;
    void TriggerAutoReconnect();

    // Phase 20.6 Network Hardening & QoS Supervision
    void RecordLatency(double rttMs);
    void RecordPacketLoss(uint64_t dropped, uint64_t total);
    void UpdateHeartbeatWatchdog(double dtSec, double timeoutSec = 5.0);
    bool IsNodeOffline() const;
    void ResetOfflineState();

    // Telemetry & Metrics
    WirelessLinkStats GetLinkStats() const;
    std::shared_ptr<WirelessQualityMonitor> GetQualityMonitor() const { return m_qualityMonitor; }
    std::string GetNodeId() const { return m_nodeId; }

private:
    mutable std::recursive_mutex m_mutex;
    std::string m_nodeId;
    std::shared_ptr<HardwareEventLogger> m_logger;
    std::shared_ptr<WirelessQualityMonitor> m_qualityMonitor;

    WifiNodeRecord m_config;
    WirelessLinkStats m_stats;

    std::atomic<bool> m_interrupted{false};
    HardwareCommandCallback m_commandCallback;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_WIRELESS_NODE_MANAGER_H
