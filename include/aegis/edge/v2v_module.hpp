/**
 * @file v2v_module.hpp
 * @brief Autonomous ad-hoc wireless transceiver for direct V2V BSMs and V2I fallback beacons.
 */

#ifndef AEGIS_EDGE_V2V_MODULE_HPP
#define AEGIS_EDGE_V2V_MODULE_HPP

#include <string>
#include <string_view>
#include <cstdint>
#include <memory>
#include <functional>
#include <mutex>
#include <atomic>
#include "aegis/common/types.hpp"
#include "aegis/common/result.hpp"
#include "aegis/edge/gps_tracker.hpp"
#include "aegis/network/network_manager.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct BasicSafetyMessage {
    std::string vehicleId;
    common::GeoCoordinate position;
    float speedKmH{0.0f};
    float bearingDeg{0.0f};
    common::AmbulanceState state{common::AmbulanceState::AVAILABLE};
    uint64_t timestampMs{0};
};

struct DirectPreemptionBeacon {
    std::string unitId;
    std::string intersectionId;
    uint16_t estimatedArrivalSec{10};
    uint64_t timestampMs{0};
};

/**
 * @brief Ad-hoc V2V / V2I communications module broadcasting 10 Hz BSMs and emergency preemption beacons.
 */
class V2vModule {
public:
    using BsmCallback = std::function<void(const BasicSafetyMessage&)>;

    V2vModule();
    ~V2vModule();

    // Lifecycle
    bool Initialize(uint16_t udpPort = 8999, std::shared_ptr<network::NetworkManager> netManager = nullptr);
    bool Start();
    void Stop();
    bool IsRunning() const noexcept;

    // Transmission API
    void BroadcastSafetyBeacon();
    bool BroadcastSafetyBeacon(const std::string& vehicleId, const VehicleKinematics& kinematics, common::AmbulanceState state);
    void SendDirectPreemptionBeacon(const std::string& intersectionId);
    bool SendDirectPreemptionBeacon(const std::string& intersectionId, const std::string& unitId, uint16_t etaSec = 10);

    // Reception & Callbacks
    void RegisterBsmCallback(BsmCallback callback);
    void HandleIncomingBsm(std::string_view payload);

    // Serialization
    static std::string SerializeBsm(const BasicSafetyMessage& bsm);
    static common::Result<BasicSafetyMessage> DeserializeBsm(std::string_view json);

    // Metrics
    size_t GetTransmittedBsmCount() const noexcept;
    size_t GetTransmittedPreemptionCount() const noexcept;

private:
    uint16_t m_port{8999};
    std::shared_ptr<network::NetworkManager> m_network;
    std::unique_ptr<network::SocketClient> m_localSocketClient;

    mutable std::mutex m_mutex;
    std::atomic<bool> m_isRunning{false};
    BsmCallback m_bsmCallback{nullptr};

    BasicSafetyMessage m_latestCachedBsm;
    std::atomic<size_t> m_txBsmCount{0};
    std::atomic<size_t> m_txPreemptionCount{0};
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_V2V_MODULE_HPP
