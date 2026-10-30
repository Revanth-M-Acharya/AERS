/**
 * @file WirelessQualityMonitor.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Wi-Fi Link Quality Monitor.
 *
 * Continuously monitors RSSI, packet loss, round-trip latency, reconnect count,
 * link uptime, and calculates signal strength trends with multi-tier warning generation.
 */

#ifndef AEGIS_HARDWARE_WIRELESS_QUALITY_MONITOR_HPP
#define AEGIS_HARDWARE_WIRELESS_QUALITY_MONITOR_HPP

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class LinkQualityLevel : uint8_t {
    EXCELLENT = 0,
    GOOD,
    FAIR,
    POOR,
    CRITICAL,
    DISCONNECTED
};

inline std::string LinkQualityLevelToString(LinkQualityLevel level) {
    switch (level) {
        case LinkQualityLevel::EXCELLENT:    return "EXCELLENT";
        case LinkQualityLevel::GOOD:         return "GOOD";
        case LinkQualityLevel::FAIR:         return "FAIR";
        case LinkQualityLevel::POOR:         return "POOR";
        case LinkQualityLevel::CRITICAL:     return "CRITICAL";
        case LinkQualityLevel::DISCONNECTED: return "DISCONNECTED";
        default:                             return "UNKNOWN";
    }
}

enum class SignalTrend : uint8_t {
    IMPROVING = 0,
    STABLE,
    DEGRADING
};

inline std::string SignalTrendToString(SignalTrend trend) {
    switch (trend) {
        case SignalTrend::IMPROVING: return "IMPROVING";
        case SignalTrend::STABLE:    return "STABLE";
        case SignalTrend::DEGRADING: return "DEGRADING";
        default:                     return "UNKNOWN";
    }
}

struct WirelessMetricsSnapshot {
    int32_t currentRssiDb{ -55 };
    double averageRssiDb{ -55.0 };
    double currentLatencyMs{ 2.0 };
    double averageLatencyMs{ 2.0 };
    double packetLossPercent{ 0.0 };
    uint32_t reconnectCount{ 0 };
    uint64_t uptimeSeconds{ 0 };
    SignalTrend trend{ SignalTrend::STABLE };
    LinkQualityLevel qualityLevel{ LinkQualityLevel::EXCELLENT };
    std::string warningMessage;
};

/**
 * @brief Production Wi-Fi quality and telemetry monitor tracking link health,
 * rolling averages, packet loss, and warning levels for the dashboard.
 */
class WirelessQualityMonitor {
public:
    using QualityAlertCallback = std::function<void(LinkQualityLevel level, const std::string& warning)>;

    explicit WirelessQualityMonitor(size_t windowSize = 20);
    ~WirelessQualityMonitor() = default;

    // Measurement Ingestion
    void RecordSample(int32_t rssiDb, double latencyMs, bool packetDropped = false);
    void RecordLatency(double latencyMs);
    void RecordRssi(int32_t rssiDb);
    void RecordPacketDrop(bool dropped = true);
    void RecordReconnect();
    void SetConnected(bool connected);

    // Queries & Telemetry
    int32_t GetCurrentRssi() const;
    double GetAverageRssi() const;
    double GetCurrentLatencyMs() const;
    double GetAverageLatencyMs() const;
    double GetPacketLossPercent() const;
    uint32_t GetReconnectCount() const;
    uint64_t GetUptimeSeconds() const;
    SignalTrend GetSignalTrend() const;
    LinkQualityLevel GetQualityLevel() const;
    std::string GetWarningMessage() const;
    bool HasWarning() const;

    WirelessMetricsSnapshot GetSnapshot() const;
    std::string ToJson() const;
    std::string GenerateReport() const;

    void SetAlertCallback(QualityAlertCallback cb);
    void Reset();

private:
    void UpdateQualityAssessment();

    mutable std::mutex m_mutex;
    size_t m_windowSize{20};
    bool m_connected{false};
    uint64_t m_connectTimestampUs{0};

    std::deque<int32_t> m_rssiHistory;
    std::deque<double> m_latencyHistory;
    std::deque<bool> m_packetDropHistory;

    uint32_t m_reconnectCount{0};
    uint64_t m_totalPackets{0};
    uint64_t m_droppedPackets{0};

    LinkQualityLevel m_qualityLevel{LinkQualityLevel::DISCONNECTED};
    SignalTrend m_trend{SignalTrend::STABLE};
    std::string m_warningMessage;
    QualityAlertCallback m_alertCallback{nullptr};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_WIRELESS_QUALITY_MONITOR_HPP

