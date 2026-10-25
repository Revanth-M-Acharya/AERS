/**
 * @file v2v_module.cpp
 * @brief Implementation of autonomous V2V Basic Safety Message transceiver and direct V2I preemption.
 */

#include "aegis/edge/v2v_module.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/json_helper.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

V2vModule::V2vModule() = default;

V2vModule::~V2vModule() {
    Stop();
}

bool V2vModule::Initialize(uint16_t udpPort, std::shared_ptr<network::NetworkManager> netManager) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_port = udpPort;
    m_network = std::move(netManager);

    if (!m_localSocketClient) {
        m_localSocketClient = std::make_unique<network::SocketClient>();
        m_localSocketClient->OpenUdp(true);
    }
    return true;
}

bool V2vModule::Start() {
    if (m_isRunning.load()) {
        return true;
    }
    m_isRunning.store(true);
    return true;
}

void V2vModule::Stop() {
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        if (m_localSocketClient) {
            m_localSocketClient->Close();
        }
    }
}

bool V2vModule::IsRunning() const noexcept {
    return m_isRunning.load();
}

void V2vModule::BroadcastSafetyBeacon() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_latestCachedBsm.vehicleId.empty()) {
        m_latestCachedBsm.timestampMs = common::GetUtcMilliseconds();
        std::string payload = SerializeBsm(m_latestCachedBsm);

        if (m_network) {
            m_network->SendV2xBeacon(m_port, payload);
        } else if (m_localSocketClient && m_localSocketClient->IsOpen()) {
            m_localSocketClient->SendTo("255.255.255.255", m_port, payload);
        }
        m_txBsmCount++;
    }
}

bool V2vModule::BroadcastSafetyBeacon(const std::string& vehicleId,
                                      const VehicleKinematics& kinematics,
                                      common::AmbulanceState state) {
    BasicSafetyMessage bsm;
    bsm.vehicleId = vehicleId;
    bsm.position = kinematics.position;
    bsm.speedKmH = kinematics.speedKmH;
    bsm.bearingDeg = kinematics.bearingDeg;
    bsm.state = state;
    bsm.timestampMs = common::GetUtcMilliseconds();

    std::string payload = SerializeBsm(bsm);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latestCachedBsm = bsm;
    }

    bool sent = false;
    if (m_network) {
        sent = m_network->SendV2xBeacon(m_port, payload);
    } else if (m_localSocketClient && m_localSocketClient->IsOpen()) {
        auto res = m_localSocketClient->SendTo("255.255.255.255", m_port, payload);
        sent = res.IsOk();
    }

    m_txBsmCount++;
    return sent;
}

void V2vModule::SendDirectPreemptionBeacon(const std::string& intersectionId) {
    SendDirectPreemptionBeacon(intersectionId, "AMB-01", 10);
}

bool V2vModule::SendDirectPreemptionBeacon(const std::string& intersectionId,
                                           const std::string& unitId,
                                           uint16_t etaSec) {
    uint64_t now = common::GetUtcMilliseconds();
    std::string payload = "{\"unitId\":\"" + unitId +
                          "\",\"intersectionId\":\"" + intersectionId +
                          "\",\"estimatedArrivalSec\":" + std::to_string(etaSec) +
                          ",\"timestamp\":" + std::to_string(now) + "}";

    bool sent = false;
    uint16_t preemptionPort = 8998;
    if (m_network) {
        sent = m_network->SendV2xBeacon(preemptionPort, payload);
    } else if (m_localSocketClient && m_localSocketClient->IsOpen()) {
        auto res = m_localSocketClient->SendTo("255.255.255.255", preemptionPort, payload);
        sent = res.IsOk();
    }

    m_txPreemptionCount++;
    return sent;
}

void V2vModule::RegisterBsmCallback(BsmCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bsmCallback = std::move(callback);
}

void V2vModule::HandleIncomingBsm(std::string_view payload) {
    auto bsmRes = DeserializeBsm(payload);
    if (bsmRes.IsOk()) {
        BsmCallback cbCopy{nullptr};
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            cbCopy = m_bsmCallback;
        }
        if (cbCopy) {
            cbCopy(bsmRes.Value());
        }
    }
}

std::string V2vModule::SerializeBsm(const BasicSafetyMessage& bsm) {
    std::string s;
    s.reserve(256);
    s += "{\"vehicleId\":\"";
    s += bsm.vehicleId;
    s += "\",\"timestamp\":";
    s += std::to_string(bsm.timestampMs);
    s += ",\"latitude\":";
    s += std::to_string(bsm.position.latitude);
    s += ",\"longitude\":";
    s += std::to_string(bsm.position.longitude);
    s += ",\"altitude\":";
    s += std::to_string(bsm.position.altitude);
    s += ",\"speed\":";
    s += std::to_string(bsm.speedKmH);
    s += ",\"heading\":";
    s += std::to_string(bsm.bearingDeg);
    s += ",\"state\":";
    s += std::to_string(static_cast<uint8_t>(bsm.state));
    s += "}";
    return s;
}

common::Result<BasicSafetyMessage> V2vModule::DeserializeBsm(std::string_view json) {
    BasicSafetyMessage bsm;
    auto vehicleId = common::JsonHelper::ExtractString(json, "vehicleId");
    if (!vehicleId) return common::Result<BasicSafetyMessage>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
    bsm.vehicleId = *vehicleId;

    auto ts = common::JsonHelper::ExtractInt64(json, "timestamp");
    if (ts) bsm.timestampMs = static_cast<uint64_t>(*ts);

    auto lat = common::JsonHelper::ExtractDouble(json, "latitude");
    if (lat) bsm.position.latitude = *lat;

    auto lon = common::JsonHelper::ExtractDouble(json, "longitude");
    if (lon) bsm.position.longitude = *lon;

    auto alt = common::JsonHelper::ExtractDouble(json, "altitude");
    if (alt) bsm.position.altitude = *alt;

    auto spd = common::JsonHelper::ExtractDouble(json, "speed");
    if (spd) bsm.speedKmH = static_cast<float>(*spd);

    auto hdg = common::JsonHelper::ExtractDouble(json, "heading");
    if (hdg) bsm.bearingDeg = static_cast<float>(*hdg);

    auto st = common::JsonHelper::ExtractInt64(json, "state");
    if (st) bsm.state = static_cast<common::AmbulanceState>(*st);

    return common::Result<BasicSafetyMessage>::Ok(bsm);
}

size_t V2vModule::GetTransmittedBsmCount() const noexcept {
    return m_txBsmCount.load();
}

size_t V2vModule::GetTransmittedPreemptionCount() const noexcept {
    return m_txPreemptionCount.load();
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
