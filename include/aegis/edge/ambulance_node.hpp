/**
 * @file ambulance_node.hpp
 * @brief Main In-Vehicle On-Board Unit (OBU) daemon supervisor and telemetry manager.
 */

#ifndef AEGIS_EDGE_AMBULANCE_NODE_HPP
#define AEGIS_EDGE_AMBULANCE_NODE_HPP

#include <string>
#include <string_view>
#include <memory>
#include <vector>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <cstdint>

#include "aegis/common/types.hpp"
#include "aegis/common/result.hpp"
#include "aegis/edge/gps_tracker.hpp"
#include "aegis/edge/vitals_monitor.hpp"
#include "aegis/edge/v2v_module.hpp"
#include "aegis/network/network_manager.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct BufferedTelemetryPacket {
    std::string topic;
    std::string payload;
    int qos{1};
    uint64_t timestampMs{0};
};

/**
 * @brief Main In-Vehicle On-Board Unit (OBU) daemon supervisor.
 * Coordinates GPS, vitals, V2V broadcasts, state machine, and store-and-forward telemetry.
 */
class AmbulanceNode {
public:
    AmbulanceNode();
    explicit AmbulanceNode(std::shared_ptr<network::NetworkManager> netManager);
    ~AmbulanceNode();

    // Lifecycle
    bool Initialize(const std::string& unitId, const std::string& configPath);
    bool Initialize(const std::string& unitId, const std::string& configPath, std::shared_ptr<network::NetworkManager> netManager);
    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    // State Machine
    bool TransitionState(common::AmbulanceState newState);
    void SetState(common::AmbulanceState state);
    common::AmbulanceState GetState() const noexcept;
    std::string GetStateString() const;
    static std::string StateToString(common::AmbulanceState state);
    static common::AmbulanceState StringToState(const std::string& str);

    // Subsystem Accessors
    const std::string& GetUnitId() const noexcept { return m_unitId; }
    GpsTracker& Gps() noexcept { return m_gps; }
    const GpsTracker& Gps() const noexcept { return m_gps; }
    VitalsMonitor& Vitals() noexcept { return m_vitals; }
    const VitalsMonitor& Vitals() const noexcept { return m_vitals; }
    V2vModule& V2v() noexcept { return m_v2v; }
    const V2vModule& V2v() const noexcept { return m_v2v; }
    network::NetworkManager& Network() noexcept { return *m_network; }
    const network::NetworkManager& Network() const noexcept { return *m_network; }

    // Patient Context
    void SetPatientId(const std::string& patientId);
    const std::string& GetPatientId() const noexcept { return m_patientId; }

    // Manual Telemetry Publish API (also executed periodically by event loops)
    bool PublishKinematicsOnce();
    bool PublishVitalsOnce();
    bool PublishHeartbeatOnce();
    bool PublishStateOnce();

    // Store-and-Forward Local Buffering
    size_t FlushBufferedTelemetry();
    size_t GetBufferedTelemetryCount() const;

private:
    void RunTelemetryLoop();
    void SetupSubsystemCallbacks();
    void PublishOrBuffer(const std::string& topic, const std::string& payload, int qos);

    std::string m_unitId{"AMB-01"};
    std::string m_patientId{"PATIENT-UNASSIGNED"};
    common::AmbulanceState m_state{common::AmbulanceState::AVAILABLE};

    std::shared_ptr<network::NetworkManager> m_network;
    bool m_ownsNetwork{false};

    GpsTracker m_gps;
    VitalsMonitor m_vitals;
    V2vModule m_v2v;

    mutable std::mutex m_stateMutex;
    mutable std::mutex m_bufferMutex;
    std::deque<BufferedTelemetryPacket> m_telemetryBuffer;
    static constexpr size_t MAX_BUFFER_CAPACITY = 2000;

    std::atomic<bool> m_isRunning{false};
    std::thread m_telemetryThread;

    uint64_t m_last10HzMs{0};
    uint64_t m_last1HzMs{0};
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_AMBULANCE_NODE_HPP
