/**
 * @file WirelessQualityMonitor.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Wi-Fi Link Quality Monitor Implementation.
 */

#include "aegis/hardware/WirelessQualityMonitor.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <numeric>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace hardware {

WirelessQualityMonitor::WirelessQualityMonitor(size_t windowSize)
    : m_windowSize(windowSize > 0 ? windowSize : 20)
    , m_connected(false)
    , m_connectTimestampUs(0)
{
}

void WirelessQualityMonitor::SetConnected(bool connected) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected != connected) {
        m_connected = connected;
        if (connected) {
            m_connectTimestampUs = common::GetMonotonicMicroseconds();
        } else {
            m_connectTimestampUs = 0;
        }
        UpdateQualityAssessment();
    }
}

void WirelessQualityMonitor::RecordSample(int32_t rssiDb, double latencyMs, bool packetDropped) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected) {
        m_connected = true;
        m_connectTimestampUs = common::GetMonotonicMicroseconds();
    }

    m_rssiHistory.push_back(rssiDb);
    if (m_rssiHistory.size() > m_windowSize) {
        m_rssiHistory.pop_front();
    }

    m_latencyHistory.push_back(latencyMs);
    if (m_latencyHistory.size() > m_windowSize) {
        m_latencyHistory.pop_front();
    }

    m_packetDropHistory.push_back(packetDropped);
    if (m_packetDropHistory.size() > m_windowSize) {
        m_packetDropHistory.pop_front();
    }

    m_totalPackets++;
    if (packetDropped) {
        m_droppedPackets++;
    }

    UpdateQualityAssessment();
}

void WirelessQualityMonitor::RecordLatency(double latencyMs) {
    RecordSample(GetCurrentRssi(), latencyMs, false);
}

void WirelessQualityMonitor::RecordRssi(int32_t rssiDb) {
    RecordSample(rssiDb, GetCurrentLatencyMs(), false);
}

void WirelessQualityMonitor::RecordPacketDrop(bool dropped) {
    RecordSample(GetCurrentRssi(), GetCurrentLatencyMs(), dropped);
}

void WirelessQualityMonitor::RecordReconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_reconnectCount++;
    m_connected = true;
    m_connectTimestampUs = common::GetMonotonicMicroseconds();
    logger::Logger::Instance().Log(
        logger::LogLevel::WARN, "WIFI_MON",
        "Wi-Fi Reconnect recorded. Total reconnect count: " + std::to_string(m_reconnectCount));
    UpdateQualityAssessment();
}

int32_t WirelessQualityMonitor::GetCurrentRssi() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_rssiHistory.empty() ? -55 : m_rssiHistory.back();
}

double WirelessQualityMonitor::GetAverageRssi() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_rssiHistory.empty()) return -55.0;
    int64_t sum = std::accumulate(m_rssiHistory.begin(), m_rssiHistory.end(), 0LL);
    return static_cast<double>(sum) / static_cast<double>(m_rssiHistory.size());
}

double WirelessQualityMonitor::GetCurrentLatencyMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latencyHistory.empty() ? 2.0 : m_latencyHistory.back();
}

double WirelessQualityMonitor::GetAverageLatencyMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_latencyHistory.empty()) return 2.0;
    double sum = std::accumulate(m_latencyHistory.begin(), m_latencyHistory.end(), 0.0);
    return sum / static_cast<double>(m_latencyHistory.size());
}

double WirelessQualityMonitor::GetPacketLossPercent() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_packetDropHistory.empty()) return 0.0;
    size_t drops = 0;
    for (bool d : m_packetDropHistory) {
        if (d) drops++;
    }
    return (static_cast<double>(drops) / static_cast<double>(m_packetDropHistory.size())) * 100.0;
}

uint32_t WirelessQualityMonitor::GetReconnectCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_reconnectCount;
}

uint64_t WirelessQualityMonitor::GetUptimeSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected || m_connectTimestampUs == 0) return 0;
    uint64_t nowUs = common::GetMonotonicMicroseconds();
    return (nowUs > m_connectTimestampUs) ? (nowUs - m_connectTimestampUs) / 1000000ULL : 0;
}

SignalTrend WirelessQualityMonitor::GetSignalTrend() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_trend;
}

LinkQualityLevel WirelessQualityMonitor::GetQualityLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_qualityLevel;
}

std::string WirelessQualityMonitor::GetWarningMessage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_warningMessage;
}

bool WirelessQualityMonitor::HasWarning() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_qualityLevel == LinkQualityLevel::FAIR ||
           m_qualityLevel == LinkQualityLevel::POOR ||
           m_qualityLevel == LinkQualityLevel::CRITICAL ||
           m_qualityLevel == LinkQualityLevel::DISCONNECTED;
}

void WirelessQualityMonitor::UpdateQualityAssessment() {
    if (!m_connected) {
        m_qualityLevel = LinkQualityLevel::DISCONNECTED;
        m_warningMessage = "Wi-Fi link is DISCONNECTED";
        m_trend = SignalTrend::STABLE;
        return;
    }

    // Compute metrics
    double avgRssi = -55.0;
    if (!m_rssiHistory.empty()) {
        int64_t sum = std::accumulate(m_rssiHistory.begin(), m_rssiHistory.end(), 0LL);
        avgRssi = static_cast<double>(sum) / static_cast<double>(m_rssiHistory.size());
    }

    double avgLatency = 2.0;
    if (!m_latencyHistory.empty()) {
        double sum = std::accumulate(m_latencyHistory.begin(), m_latencyHistory.end(), 0.0);
        avgLatency = sum / static_cast<double>(m_latencyHistory.size());
    }

    double lossPct = 0.0;
    if (!m_packetDropHistory.empty()) {
        size_t drops = 0;
        for (bool d : m_packetDropHistory) {
            if (d) drops++;
        }
        lossPct = (static_cast<double>(drops) / static_cast<double>(m_packetDropHistory.size())) * 100.0;
    }

    // Calculate Trend
    if (m_rssiHistory.size() >= 4) {
        size_t mid = m_rssiHistory.size() / 2;
        int64_t oldSum = 0;
        int64_t newSum = 0;
        for (size_t i = 0; i < mid; ++i) oldSum += m_rssiHistory[i];
        for (size_t i = mid; i < m_rssiHistory.size(); ++i) newSum += m_rssiHistory[i];
        double oldAvg = static_cast<double>(oldSum) / static_cast<double>(mid);
        double newAvg = static_cast<double>(newSum) / static_cast<double>(m_rssiHistory.size() - mid);

        if (newAvg > oldAvg + 2.0) {
            m_trend = SignalTrend::IMPROVING;
        } else if (newAvg < oldAvg - 2.0) {
            m_trend = SignalTrend::DEGRADING;
        } else {
            m_trend = SignalTrend::STABLE;
        }
    } else {
        m_trend = SignalTrend::STABLE;
    }

    // Determine Quality Level (5 Tiers: EXCELLENT, GOOD, FAIR, POOR, CRITICAL)
    LinkQualityLevel prevLevel = m_qualityLevel;
    if (avgRssi < -85.0 || lossPct >= 20.0 || avgLatency >= 300.0) {
        m_qualityLevel = LinkQualityLevel::CRITICAL;
        m_warningMessage = "CRITICAL: Severe link degradation (RSSI " + std::to_string(static_cast<int>(avgRssi)) +
                           " dBm, Loss " + std::to_string(static_cast<int>(lossPct)) + "%, RTT " +
                           std::to_string(static_cast<int>(avgLatency)) + " ms)";
    } else if (avgRssi < -75.0 || lossPct >= 10.0 || avgLatency >= 150.0) {
        m_qualityLevel = LinkQualityLevel::POOR;
        m_warningMessage = "POOR: Suboptimal link quality (RSSI " + std::to_string(static_cast<int>(avgRssi)) +
                           " dBm, Loss " + std::to_string(static_cast<int>(lossPct)) + "%, RTT " +
                           std::to_string(static_cast<int>(avgLatency)) + " ms)";
    } else if (avgRssi < -68.0 || lossPct >= 3.0 || avgLatency >= 60.0) {
        m_qualityLevel = LinkQualityLevel::FAIR;
        m_warningMessage = "FAIR: Moderate link degradation (RSSI " + std::to_string(static_cast<int>(avgRssi)) +
                           " dBm, Loss " + std::to_string(static_cast<int>(lossPct)) + "%, RTT " +
                           std::to_string(static_cast<int>(avgLatency)) + " ms)";
    } else if (avgRssi < -60.0 || lossPct >= 1.0 || avgLatency >= 20.0) {
        m_qualityLevel = LinkQualityLevel::GOOD;
        m_warningMessage.clear();
    } else {
        m_qualityLevel = LinkQualityLevel::EXCELLENT;
        m_warningMessage.clear();
    }

    if (m_qualityLevel != prevLevel) {
        if (m_qualityLevel > prevLevel) {
            logger::Logger::Instance().Log(
                logger::LogLevel::WARN, "WIFI_MON",
                "Link degradation event: " + LinkQualityLevelToString(prevLevel) + " -> " +
                LinkQualityLevelToString(m_qualityLevel) + " (RSSI: " + std::to_string(avgRssi) +
                " dBm, Loss: " + std::to_string(lossPct) + "%, RTT: " + std::to_string(avgLatency) + " ms)");
        }
        if (m_alertCallback) {
            m_alertCallback(m_qualityLevel, m_warningMessage);
        }
    }
}

WirelessMetricsSnapshot WirelessQualityMonitor::GetSnapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    WirelessMetricsSnapshot snap;
    snap.currentRssiDb = m_rssiHistory.empty() ? -55 : m_rssiHistory.back();

    if (!m_rssiHistory.empty()) {
        int64_t sum = std::accumulate(m_rssiHistory.begin(), m_rssiHistory.end(), 0LL);
        snap.averageRssiDb = static_cast<double>(sum) / static_cast<double>(m_rssiHistory.size());
    } else {
        snap.averageRssiDb = -55.0;
    }

    snap.currentLatencyMs = m_latencyHistory.empty() ? 2.0 : m_latencyHistory.back();
    if (!m_latencyHistory.empty()) {
        double sum = std::accumulate(m_latencyHistory.begin(), m_latencyHistory.end(), 0.0);
        snap.averageLatencyMs = sum / static_cast<double>(m_latencyHistory.size());
    } else {
        snap.averageLatencyMs = 2.0;
    }

    if (!m_packetDropHistory.empty()) {
        size_t drops = 0;
        for (bool d : m_packetDropHistory) {
            if (d) drops++;
        }
        snap.packetLossPercent = (static_cast<double>(drops) / static_cast<double>(m_packetDropHistory.size())) * 100.0;
    } else {
        snap.packetLossPercent = 0.0;
    }

    snap.reconnectCount = m_reconnectCount;
    uint64_t nowUs = common::GetMonotonicMicroseconds();
    snap.uptimeSeconds = (m_connected && m_connectTimestampUs > 0 && nowUs > m_connectTimestampUs)
                         ? (nowUs - m_connectTimestampUs) / 1000000ULL : 0;
    snap.trend = m_trend;
    snap.qualityLevel = m_qualityLevel;
    snap.warningMessage = m_warningMessage;
    return snap;
}

std::string WirelessQualityMonitor::ToJson() const {
    auto s = GetSnapshot();
    std::ostringstream oss;
    oss << "{"
        << "\"current_rssi_db\":" << s.currentRssiDb << ","
        << "\"average_rssi_db\":" << std::fixed << std::setprecision(1) << s.averageRssiDb << ","
        << "\"current_latency_ms\":" << std::fixed << std::setprecision(2) << s.currentLatencyMs << ","
        << "\"average_latency_ms\":" << std::fixed << std::setprecision(2) << s.averageLatencyMs << ","
        << "\"packet_loss_percent\":" << std::fixed << std::setprecision(2) << s.packetLossPercent << ","
        << "\"reconnect_count\":" << s.reconnectCount << ","
        << "\"uptime_seconds\":" << s.uptimeSeconds << ","
        << "\"trend\":\"" << SignalTrendToString(s.trend) << "\","
        << "\"quality_level\":\"" << LinkQualityLevelToString(s.qualityLevel) << "\","
        << "\"warning\":\"" << s.warningMessage << "\""
        << "}";
    return oss.str();
}

std::string WirelessQualityMonitor::GenerateReport() const {
    auto s = GetSnapshot();
    std::ostringstream oss;
    oss << "=== WI-FI LINK QUALITY REPORT ===\n"
        << "Quality Level  : " << LinkQualityLevelToString(s.qualityLevel) << "\n"
        << "RSSI           : " << s.currentRssiDb << " dBm (Avg: " << std::fixed << std::setprecision(1) << s.averageRssiDb << " dBm)\n"
        << "Latency (RTT)  : " << std::fixed << std::setprecision(2) << s.currentLatencyMs << " ms (Avg: " << s.averageLatencyMs << " ms)\n"
        << "Packet Loss    : " << std::fixed << std::setprecision(2) << s.packetLossPercent << " %\n"
        << "Trend          : " << SignalTrendToString(s.trend) << "\n"
        << "Reconnects     : " << s.reconnectCount << "\n"
        << "Link Uptime    : " << s.uptimeSeconds << " s\n";
    if (!s.warningMessage.empty()) {
        oss << "Active Warning : " << s.warningMessage << "\n";
    }
    oss << "=================================\n";
    return oss.str();
}

void WirelessQualityMonitor::SetAlertCallback(QualityAlertCallback cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alertCallback = std::move(cb);
}

void WirelessQualityMonitor::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rssiHistory.clear();
    m_latencyHistory.clear();
    m_packetDropHistory.clear();
    m_reconnectCount = 0;
    m_totalPackets = 0;
    m_droppedPackets = 0;
    m_qualityLevel = LinkQualityLevel::DISCONNECTED;
    m_trend = SignalTrend::STABLE;
    m_warningMessage.clear();
    m_connected = false;
    m_connectTimestampUs = 0;
}

} // namespace hardware
} // namespace aegis
