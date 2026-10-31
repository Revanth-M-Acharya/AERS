/**
 * @file TelemetryCollector.cpp
 * @brief Implementation of TelemetryCollector.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/TelemetryCollector.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>
#include <numeric>
#include <algorithm>

namespace aegis::digital_twin {

TelemetryCollector::TelemetryCollector() {
    Reset();
}

void TelemetryCollector::RecordPacketTx(const std::string& type, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalPacketsTx_++;
    totalBytesTx_ += bytes;
    packetsByType_[type]++;
}

void TelemetryCollector::RecordPacketRx(const std::string& type, uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalPacketsRx_++;
    totalBytesRx_ += bytes;
    packetsByType_[type]++;
}

void TelemetryCollector::RecordNodeHeartbeat(const std::string& nodeId, const std::string& ip, double pingMs, bool esp32) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& n = nodes_[nodeId];
    n.nodeId = nodeId;
    n.ipAddress = ip;
    n.lastHeartbeatMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    n.missedHeartbeats = 0;
    n.isConnected = true;
    n.esp32Connected = esp32;
    n.pingLatencyMs = pingMs;
}

void TelemetryCollector::RecordNodeMissedHeartbeat(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.missedHeartbeats++;
        if (it->second.missedHeartbeats >= 3) {
            it->second.isConnected = false;
        }
    }
}

void TelemetryCollector::RecordNodeDisconnect(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        it->second.isConnected = false;
    }
}

void TelemetryCollector::RecordEventDispatched() {
    std::lock_guard<std::mutex> lock(mutex_);
    totalEventsProcessed_++;
    uint64_t now = common::TimeUtils::GetMonotonicTimeMs();
    if (lastEventCalculationTimeMonoMs_ == 0) {
        lastEventCalculationTimeMonoMs_ = now;
        lastEventsCountSnapshot_ = totalEventsProcessed_;
    } else if (now > lastEventCalculationTimeMonoMs_ + 1000) {
        uint64_t dt = now - lastEventCalculationTimeMonoMs_;
        uint64_t dEv = totalEventsProcessed_ - lastEventsCountSnapshot_;
        eventThroughputPerSec_ = (static_cast<double>(dEv) * 1000.0) / static_cast<double>(dt);
        lastEventCalculationTimeMonoMs_ = now;
        lastEventsCountSnapshot_ = totalEventsProcessed_;
    }
}

void TelemetryCollector::RecordDeadlineMiss() {
    std::lock_guard<std::mutex> lock(mutex_);
    deadlineMisses_++;
}

void TelemetryCollector::RecordLatencySample(double latencyMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (latencySamples_.size() >= 200) {
        latencySamples_.erase(latencySamples_.begin());
    }
    latencySamples_.push_back(latencyMs);
}

void TelemetryCollector::UpdateHardwareTelemetry(double cpuPercent, uint64_t memKb, uint32_t threadCount) {
    std::lock_guard<std::mutex> lock(mutex_);
    cpuUsagePercent_ = cpuPercent;
    memoryUsageKb_ = memKb;
    activeThreadCount_ = threadCount;
}

void TelemetryCollector::IncrementRouteReplans(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalRouteReplans_ += count;
}

void TelemetryCollector::IncrementSignalSyncs(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalSignalSyncs_ += count;
}

void TelemetryCollector::IncrementCameraEvents(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalCameraEvents_ += count;
}

void TelemetryCollector::IncrementHospitalNotifications(uint32_t count) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalHospitalNotifications_ += count;
}

void TelemetryCollector::RecordCompletedMission(double etaReductionSeconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalCompletedMissions_++;
    totalEtaReductionSec_ += etaReductionSeconds;
}

SystemTelemetrySnapshot TelemetryCollector::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    SystemTelemetrySnapshot s;
    s.timestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    s.totalPacketsTx = totalPacketsTx_;
    s.totalPacketsRx = totalPacketsRx_;
    s.totalBytesTx = totalBytesTx_;
    s.totalBytesRx = totalBytesRx_;
    s.packetsByType = packetsByType_;

    for (const auto& [_, node] : nodes_) {
        s.nodes.push_back(node);
    }

    s.cpuUsagePercent = cpuUsagePercent_;
    s.memoryUsageKb = memoryUsageKb_;
    s.activeThreadCount = activeThreadCount_;
    s.eventThroughputPerSec = eventThroughputPerSec_;
    s.totalEventsProcessed = totalEventsProcessed_;
    s.deadlineMisses = deadlineMisses_;

    if (!latencySamples_.empty()) {
        double sum = std::accumulate(latencySamples_.begin(), latencySamples_.end(), 0.0);
        s.avgLatencyMs = sum / static_cast<double>(latencySamples_.size());
        s.maxLatencyMs = *std::max_element(latencySamples_.begin(), latencySamples_.end());
    }

    s.totalRouteReplans = totalRouteReplans_;
    s.totalSignalSyncs = totalSignalSyncs_;
    s.totalCameraEvents = totalCameraEvents_;
    s.totalHospitalNotifications = totalHospitalNotifications_;
    s.totalCompletedMissions = totalCompletedMissions_;
    s.averageEtaReductionSeconds = (totalCompletedMissions_ > 0) ? (totalEtaReductionSec_ / totalCompletedMissions_) : 0.0;

    return s;
}

void TelemetryCollector::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    totalPacketsTx_ = 0;
    totalPacketsRx_ = 0;
    totalBytesTx_ = 0;
    totalBytesRx_ = 0;
    packetsByType_.clear();
    nodes_.clear();

    cpuUsagePercent_ = 3.5;
    memoryUsageKb_ = 12800;
    activeThreadCount_ = 16;
    totalEventsProcessed_ = 0;
    lastEventCalculationTimeMonoMs_ = common::TimeUtils::GetMonotonicTimeMs();
    lastEventsCountSnapshot_ = 0;
    eventThroughputPerSec_ = 0.0;
    deadlineMisses_ = 0;
    latencySamples_.clear();

    totalRouteReplans_ = 0;
    totalSignalSyncs_ = 0;
    totalCameraEvents_ = 0;
    totalHospitalNotifications_ = 0;
    totalCompletedMissions_ = 0;
    totalEtaReductionSec_ = 0.0;
}

std::string SystemTelemetrySnapshot::ToJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"timestampMonoMs\":" << timestampMonoMs << ","
       << "\"packetsTx\":" << totalPacketsTx << ","
       << "\"packetsRx\":" << totalPacketsRx << ","
       << "\"bytesTx\":" << totalBytesTx << ","
       << "\"bytesRx\":" << totalBytesRx << ","
       << "\"cpuUsagePercent\":" << std::fixed << std::setprecision(1) << cpuUsagePercent << ","
       << "\"memoryUsageKb\":" << memoryUsageKb << ","
       << "\"activeThreads\":" << activeThreadCount << ","
       << "\"eventThroughput\":" << std::fixed << std::setprecision(1) << eventThroughputPerSec << ","
       << "\"deadlineMisses\":" << deadlineMisses << ","
       << "\"avgLatencyMs\":" << std::fixed << std::setprecision(2) << avgLatencyMs << ","
       << "\"replans\":" << totalRouteReplans << ","
       << "\"signalSyncs\":" << totalSignalSyncs << ","
       << "\"cameraEvents\":" << totalCameraEvents << ","
       << "\"hospitalNotifs\":" << totalHospitalNotifications << ","
       << "\"completedMissions\":" << totalCompletedMissions << ","
       << "\"avgEtaReduction\":" << std::fixed << std::setprecision(1) << averageEtaReductionSeconds
       << "}";
    return ss.str();
}

} // namespace aegis::digital_twin

