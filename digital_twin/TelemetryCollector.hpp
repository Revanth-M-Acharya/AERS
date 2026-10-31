/**
 * @file TelemetryCollector.hpp
 * @brief Aggregator of hardware, network, packet, and operational telemetry from across the cluster.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct NodeTelemetryStatus {
    std::string nodeId;
    std::string ipAddress;
    uint64_t lastHeartbeatMonoMs{0};
    uint32_t missedHeartbeats{0};
    bool isConnected{true};
    bool esp32Connected{true};
    double pingLatencyMs{0.5};
};

struct SystemTelemetrySnapshot {
    uint64_t timestampMonoMs{0};
    // Network / Packet counts
    uint64_t totalPacketsTx{0};
    uint64_t totalPacketsRx{0};
    uint64_t totalBytesTx{0};
    uint64_t totalBytesRx{0};
    std::unordered_map<std::string, uint64_t> packetsByType;

    // Node Health
    std::vector<NodeTelemetryStatus> nodes;

    // Performance & Hardware
    double cpuUsagePercent{0.0};
    uint64_t memoryUsageKb{0};
    uint32_t activeThreadCount{0};
    double eventThroughputPerSec{0.0};
    uint64_t totalEventsProcessed{0};
    uint32_t deadlineMisses{0};
    double avgLatencyMs{0.5};
    double maxLatencyMs{1.2};

    // Domain Metrics
    uint32_t totalRouteReplans{0};
    uint32_t totalSignalSyncs{0};
    uint32_t totalCameraEvents{0};
    uint32_t totalHospitalNotifications{0};
    uint32_t totalCompletedMissions{0};
    double averageEtaReductionSeconds{0.0};

    std::string ToJson() const;
};

class TelemetryCollector {
public:
    TelemetryCollector();
    ~TelemetryCollector() = default;

    void RecordPacketTx(const std::string& type, uint64_t bytes);
    void RecordPacketRx(const std::string& type, uint64_t bytes);

    void RecordNodeHeartbeat(const std::string& nodeId, const std::string& ip = "127.0.0.1", double pingMs = 0.5, bool esp32 = true);
    void RecordNodeMissedHeartbeat(const std::string& nodeId);
    void RecordNodeDisconnect(const std::string& nodeId);

    void RecordEventDispatched();
    void RecordDeadlineMiss();
    void RecordLatencySample(double latencyMs);

    void UpdateHardwareTelemetry(double cpuPercent, uint64_t memKb, uint32_t threadCount);

    void IncrementRouteReplans(uint32_t count = 1);
    void IncrementSignalSyncs(uint32_t count = 1);
    void IncrementCameraEvents(uint32_t count = 1);
    void IncrementHospitalNotifications(uint32_t count = 1);
    void RecordCompletedMission(double etaReductionSeconds);

    SystemTelemetrySnapshot GetSnapshot() const;
    void Reset();

private:
    mutable std::mutex mutex_;
    uint64_t totalPacketsTx_{0};
    uint64_t totalPacketsRx_{0};
    uint64_t totalBytesTx_{0};
    uint64_t totalBytesRx_{0};
    std::unordered_map<std::string, uint64_t> packetsByType_;

    std::unordered_map<std::string, NodeTelemetryStatus> nodes_;

    double cpuUsagePercent_{3.5};
    uint64_t memoryUsageKb_{12800};
    uint32_t activeThreadCount_{16};
    uint64_t totalEventsProcessed_{0};
    uint64_t lastEventCalculationTimeMonoMs_{0};
    uint64_t lastEventsCountSnapshot_{0};
    double eventThroughputPerSec_{0.0};

    uint32_t deadlineMisses_{0};
    std::vector<double> latencySamples_;

    uint32_t totalRouteReplans_{0};
    uint32_t totalSignalSyncs_{0};
    uint32_t totalCameraEvents_{0};
    uint32_t totalHospitalNotifications_{0};
    uint32_t totalCompletedMissions_{0};
    double totalEtaReductionSec_{0.0};
};

} // namespace aegis::digital_twin

