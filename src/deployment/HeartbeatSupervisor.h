/**
 * @file HeartbeatSupervisor.h
 * @brief SMART AEGIS (AERS) Phase 16D - Distributed Heartbeat Supervisor & Telemetry Engine.
 *
 * Supervises node keepalives, collects hardware telemetry (CPU, RAM, Temp, GPIO),
 * detects offline timeouts, triggers edge Flashing Yellow fail-safe, and generates
 * live Coordinator status dashboards.
 */

#ifndef AEGIS_DEPLOYMENT_HEARTBEAT_SUPERVISOR_H
#define AEGIS_DEPLOYMENT_HEARTBEAT_SUPERVISOR_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <cstdint>

namespace aegis {
namespace deployment {

struct NodeTelemetry {
    std::string nodeId{"NODE"};
    uint64_t timestampMs{0};
    double cpuUsagePercent{12.5};
    double memoryUsagePercent{24.0};
    double temperatureCelsius{46.2};
    std::string gpioStatus{"RED=ON YELLOW=OFF GREEN=OFF PED_WALK=OFF"};
    double nodeHealth{100.0};
    std::string signalState{"RED"};
    bool isOnline{true};
    uint32_t missedHeartbeats{0};
    uint64_t lastHeartbeatEpochMs{0};
};

class HeartbeatSupervisor {
public:
    using TimeoutCallback = std::function<void(const std::string& nodeId)>;
    using RecoveryCallback = std::function<void(const std::string& nodeId)>;

    HeartbeatSupervisor();
    ~HeartbeatSupervisor();

    // Non-copyable, non-movable
    HeartbeatSupervisor(const HeartbeatSupervisor&) = delete;
    HeartbeatSupervisor& operator=(const HeartbeatSupervisor&) = delete;

    /**
     * @brief Start background supervisor thread for coordinator heartbeat monitoring.
     * @param timeoutMs Timeout in milliseconds before a silent node is marked OFFLINE (default 3000 ms).
     * @param checkIntervalMs Polling frequency of watchdog in ms (default 200 ms).
     */
    void StartSupervision(uint32_t timeoutMs = 3000, uint32_t checkIntervalMs = 200);

    /**
     * @brief Stop background supervisor thread.
     */
    void StopSupervision();

    /**
     * @brief Record receipt of an inbound heartbeat packet from an edge node.
     * @param nodeId Edge node identifier.
     * @param telemetry Telemetry data parsed from heartbeat payload.
     */
    void RecordHeartbeat(const std::string& nodeId, const NodeTelemetry& telemetry);

    /**
     * @brief Query whether a specific node is currently ONLINE and healthy.
     */
    bool IsNodeOnline(const std::string& nodeId) const;

    /**
     * @brief Retrieve snapshot of telemetry for a specific node.
     */
    bool GetNodeTelemetry(const std::string& nodeId, NodeTelemetry& outTelemetry) const;

    /**
     * @brief Retrieve telemetry snapshot for all tracked nodes.
     */
    std::vector<NodeTelemetry> GetAllTelemetry() const;

    /**
     * @brief Register callback for offline node failure detection.
     */
    void SetTimeoutCallback(TimeoutCallback callback);

    /**
     * @brief Register callback for node recovery detection.
     */
    void SetRecoveryCallback(RecoveryCallback callback);

    /**
     * @brief Edge Node Helper: Gather local hardware telemetry for dispatch.
     * @param nodeId Unique node ID.
     * @param signalState Current signal state (e.g. "RED", "GREEN", "FLASHING_YELLOW").
     * @param gpioStatus Current GPIO pin summary string.
     * @return Fully populated NodeTelemetry struct.
     */
    static NodeTelemetry GatherLocalTelemetry(const std::string& nodeId,
                                              const std::string& signalState = "RED",
                                              const std::string& gpioStatus = "");

    /**
     * @brief Format live ASCII dashboard summary table of all nodes.
     */
    std::string GenerateDashboardView() const;

    /**
     * @brief Explicitly mark a node offline (e.g. on disconnect event).
     */
    void SetNodeOffline(const std::string& nodeId);

private:
    void SupervisorLoop();

    mutable std::mutex m_mutex;
    std::map<std::string, NodeTelemetry> m_nodes;
    TimeoutCallback m_timeoutCallback;
    RecoveryCallback m_recoveryCallback;

    uint32_t m_timeoutMs{3000};
    uint32_t m_checkIntervalMs{200};
    std::atomic<bool> m_running{false};
    std::thread m_supervisorThread;
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_HEARTBEAT_SUPERVISOR_H
