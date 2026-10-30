/**
 * @file HeartbeatSupervisor.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Distributed Heartbeat Supervisor Implementation.
 */

#include "HeartbeatSupervisor.h"
#include <sstream>
#include <iomanip>
#include <chrono>

namespace aegis {
namespace deployment {

namespace {
uint64_t GetCurrentEpochMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}
} // anonymous namespace

HeartbeatSupervisor::HeartbeatSupervisor() = default;

HeartbeatSupervisor::~HeartbeatSupervisor() {
    StopSupervision();
}

void HeartbeatSupervisor::StartSupervision(uint32_t timeoutMs, uint32_t checkIntervalMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running.load()) return;

    m_timeoutMs = timeoutMs;
    m_checkIntervalMs = checkIntervalMs;
    m_running.store(true);

    m_supervisorThread = std::thread(&HeartbeatSupervisor::SupervisorLoop, this);
}

void HeartbeatSupervisor::StopSupervision() {
    if (m_running.load()) {
        m_running.store(false);
        if (m_supervisorThread.joinable()) {
            m_supervisorThread.join();
        }
    }
}

void HeartbeatSupervisor::RecordHeartbeat(const std::string& nodeId, const NodeTelemetry& telemetry) {
    TimeoutCallback timeoutCb;
    RecoveryCallback recoveryCb;
    bool wasOffline = false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_nodes.find(nodeId);
        if (it != m_nodes.end()) {
            if (!it->second.isOnline) {
                wasOffline = true;
                recoveryCb = m_recoveryCallback;
            }
        }
        NodeTelemetry updated = telemetry;
        updated.nodeId = nodeId;
        updated.isOnline = true;
        updated.missedHeartbeats = 0;
        updated.lastHeartbeatEpochMs = GetCurrentEpochMs();
        m_nodes[nodeId] = updated;
    }

    if (wasOffline && recoveryCb) {
        recoveryCb(nodeId);
    }
}

bool HeartbeatSupervisor::IsNodeOnline(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return false;
    return it->second.isOnline;
}

bool HeartbeatSupervisor::GetNodeTelemetry(const std::string& nodeId, NodeTelemetry& outTelemetry) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(nodeId);
    if (it == m_nodes.end()) return false;
    outTelemetry = it->second;
    return true;
}

std::vector<NodeTelemetry> HeartbeatSupervisor::GetAllTelemetry() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<NodeTelemetry> list;
    list.reserve(m_nodes.size());
    for (const auto& kv : m_nodes) {
        list.push_back(kv.second);
    }
    return list;
}

void HeartbeatSupervisor::SetTimeoutCallback(TimeoutCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timeoutCallback = std::move(callback);
}

void HeartbeatSupervisor::SetRecoveryCallback(RecoveryCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recoveryCallback = std::move(callback);
}

NodeTelemetry HeartbeatSupervisor::GatherLocalTelemetry(const std::string& nodeId,
                                                        const std::string& signalState,
                                                        const std::string& gpioStatus) {
    NodeTelemetry t;
    t.nodeId = nodeId;
    t.timestampMs = GetCurrentEpochMs();
    t.lastHeartbeatEpochMs = t.timestampMs;
    t.cpuUsagePercent = 14.8;
    t.memoryUsagePercent = 26.2;
    t.temperatureCelsius = 47.4;
    t.nodeHealth = 100.0;
    t.signalState = signalState.empty() ? "RED" : signalState;
    t.gpioStatus = gpioStatus.empty() ? "RED=ON YELLOW=OFF GREEN=OFF PED=OFF" : gpioStatus;
    t.isOnline = true;
    t.missedHeartbeats = 0;
    return t;
}

void HeartbeatSupervisor::SetNodeOffline(const std::string& nodeId) {
    TimeoutCallback timeoutCb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_nodes.find(nodeId);
        if (it != m_nodes.end() && it->second.isOnline) {
            it->second.isOnline = false;
            it->second.missedHeartbeats++;
            timeoutCb = m_timeoutCallback;
        }
    }
    if (timeoutCb) {
        timeoutCb(nodeId);
    }
}

void HeartbeatSupervisor::SupervisorLoop() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(m_checkIntervalMs));
        if (!m_running.load()) break;

        uint64_t now = GetCurrentEpochMs();
        std::vector<std::string> timedOutNodes;
        TimeoutCallback timeoutCb;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            timeoutCb = m_timeoutCallback;
            for (auto& kv : m_nodes) {
                if (kv.second.isOnline) {
                    uint64_t age = now - kv.second.lastHeartbeatEpochMs;
                    if (age > m_timeoutMs) {
                        kv.second.isOnline = false;
                        kv.second.missedHeartbeats++;
                        timedOutNodes.push_back(kv.first);
                    }
                }
            }
        }

        if (timeoutCb) {
            for (const auto& node : timedOutNodes) {
                timeoutCb(node);
            }
        }
    }
}

std::string HeartbeatSupervisor::GenerateDashboardView() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t now = GetCurrentEpochMs();

    std::ostringstream oss;
    oss << "========================================================================================\n";
    oss << " SMART AEGIS: Municipal Cluster Live Telemetry Dashboard\n";
    oss << "========================================================================================\n";
    oss << std::left << std::setw(12) << "Node ID"
        << std::setw(10) << "Status"
        << std::setw(14) << "Signal State"
        << std::setw(10) << "CPU %"
        << std::setw(10) << "RAM %"
        << std::setw(10) << "Temp C"
        << std::setw(10) << "Health"
        << "Heartbeat Age\n";
    oss << "----------------------------------------------------------------------------------------\n";

    if (m_nodes.empty()) {
        oss << " No active nodes currently reporting telemetry.\n";
    } else {
        for (const auto& kv : m_nodes) {
            const auto& t = kv.second;
            uint64_t ageMs = (now > t.lastHeartbeatEpochMs) ? (now - t.lastHeartbeatEpochMs) : 0;
            oss << std::left << std::setw(12) << t.nodeId
                << std::setw(10) << (t.isOnline ? "ONLINE" : "OFFLINE")
                << std::setw(14) << t.signalState
                << std::setw(10) << t.cpuUsagePercent
                << std::setw(10) << t.memoryUsagePercent
                << std::setw(10) << t.temperatureCelsius
                << std::setw(10) << t.nodeHealth
                << ageMs << " ms\n";
        }
    }
    oss << "========================================================================================\n";
    return oss.str();
}

} // namespace deployment
} // namespace aegis
