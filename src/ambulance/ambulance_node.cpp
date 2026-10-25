/**
 * @file ambulance_node.cpp
 * @brief Implementation of AmbulanceNode On-Board Unit edge controller.
 */

#include "aegis/edge/ambulance_node.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/network/serializer.hpp"
#include "aegis/logger/logger.hpp"

#include <chrono>

namespace aegis {
namespace edge {
namespace ambulance {

AmbulanceNode::AmbulanceNode()
    : m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {}

AmbulanceNode::AmbulanceNode(std::shared_ptr<network::NetworkManager> netManager)
    : m_network(std::move(netManager)),
      m_ownsNetwork(false) {
    if (!m_network) {
        m_network = std::make_shared<network::NetworkManager>();
        m_ownsNetwork = true;
    }
}

AmbulanceNode::~AmbulanceNode() {
    Stop();
    if (m_ownsNetwork && m_network) {
        m_network->Shutdown();
    }
}

bool AmbulanceNode::Initialize(const std::string& unitId, const std::string& configPath) {
    return Initialize(unitId, configPath, nullptr);
}

bool AmbulanceNode::Initialize(const std::string& unitId,
                               const std::string& /*configPath*/,
                               std::shared_ptr<network::NetworkManager> netManager) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_unitId = unitId;
    m_state = common::AmbulanceState::AVAILABLE;

    if (netManager) {
        m_network = std::move(netManager);
        m_ownsNetwork = false;
    } else if (!m_network) {
        m_network = std::make_shared<network::NetworkManager>();
        m_ownsNetwork = true;
    }

    if (m_ownsNetwork) {
        m_network->Initialize("127.0.0.1", 1883, false, m_unitId);
    }

    // Initialize subsystems
    m_gps.Initialize();
    m_vitals.Initialize();
    m_v2v.Initialize(8999, m_network);

    SetupSubsystemCallbacks();
    return true;
}

void AmbulanceNode::SetupSubsystemCallbacks() {
    // Forward vital alarms to central hospital dashboard and fleet monitoring
    m_vitals.RegisterAlarmCallback([this](const VitalsAlert& alert) {
        network::HospitalAlertMsg alertMsg;
        alertMsg.alertId = "ALR-" + std::to_string(alert.timestampMs);
        alertMsg.unitId = m_unitId;
        alertMsg.patientId = m_patientId;
        alertMsg.severity = alert.severity;
        alertMsg.alarmType = alert.description;
        alertMsg.triggeredValue = alert.triggeredValue;
        alertMsg.timestamp = alert.timestampMs;

        std::string payload = network::Serializer::Serialize(alertMsg);
        PublishOrBuffer("aegis/fleet/" + m_unitId + "/alarms", payload, 2);
    });

    // Subscribe to physician ER directives
    m_network->Subscribe("aegis/hospital/" + m_unitId + "/commands",
        [](const std::string& /*topic*/, const std::string& /*payload*/) {
            logger::Logger::Instance().Log(
                logger::LogLevel::INFO, "OBU", "Received ER clinical command from hospital.");
        }, 2);
}

bool AmbulanceNode::Start() {
    if (m_isRunning.load()) {
        return true;
    }
    m_isRunning.store(true);

    m_gps.Start();
    m_vitals.Start();
    m_v2v.Start();

    m_telemetryThread = std::thread(&AmbulanceNode::RunTelemetryLoop, this);
    return true;
}

void AmbulanceNode::Stop() {
    if (m_isRunning.load()) {
        m_isRunning.store(false);
        m_gps.Stop();
        m_vitals.Stop();
        m_v2v.Stop();

        if (m_telemetryThread.joinable()) {
            m_telemetryThread.join();
        }
    }
}

bool AmbulanceNode::IsRunning() const noexcept {
    return m_isRunning.load();
}

bool AmbulanceNode::TransitionState(common::AmbulanceState newState) {
    {
        std::lock_guard<std::mutex> lock(m_stateMutex);
        if (m_state == newState) {
            return true;
        }
        m_state = newState;
    }
    return PublishStateOnce();
}

void AmbulanceNode::SetState(common::AmbulanceState state) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_state = state;
}

common::AmbulanceState AmbulanceNode::GetState() const noexcept {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_state;
}

std::string AmbulanceNode::GetStateString() const {
    return StateToString(GetState());
}

std::string AmbulanceNode::StateToString(common::AmbulanceState state) {
    switch (state) {
        case common::AmbulanceState::AVAILABLE: return "AVAILABLE";
        case common::AmbulanceState::DISPATCHED: return "DISPATCHED";
        case common::AmbulanceState::EN_ROUTE_PATIENT: return "EN_ROUTE_PATIENT";
        case common::AmbulanceState::PATIENT_PICKED: return "ON_SCENE";
        case common::AmbulanceState::EN_ROUTE_HOSPITAL: return "EN_ROUTE_HOSPITAL";
        case common::AmbulanceState::ARRIVED: return "ARRIVED";
        case common::AmbulanceState::OFF_DUTY: return "OFF_DUTY";
        default: return "UNKNOWN";
    }
}

common::AmbulanceState AmbulanceNode::StringToState(const std::string& str) {
    if (str == "AVAILABLE" || str == "NORMAL") return common::AmbulanceState::AVAILABLE;
    if (str == "DISPATCHED") return common::AmbulanceState::DISPATCHED;
    if (str == "EN_ROUTE_PATIENT") return common::AmbulanceState::EN_ROUTE_PATIENT;
    if (str == "ON_SCENE" || str == "PATIENT_PICKED") return common::AmbulanceState::PATIENT_PICKED;
    if (str == "EN_ROUTE_HOSPITAL") return common::AmbulanceState::EN_ROUTE_HOSPITAL;
    if (str == "ARRIVED") return common::AmbulanceState::ARRIVED;
    if (str == "OFF_DUTY") return common::AmbulanceState::OFF_DUTY;
    return common::AmbulanceState::AVAILABLE;
}

void AmbulanceNode::SetPatientId(const std::string& patientId) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_patientId = patientId;
}

bool AmbulanceNode::PublishKinematicsOnce() {
    VehicleKinematics k = m_gps.GetLatestKinematics();

    network::GPSUpdateMsg msg;
    msg.unitId = m_unitId;
    msg.timestamp = k.timestampMs;
    msg.latitude = k.position.latitude;
    msg.longitude = k.position.longitude;
    msg.speed = k.speedKmH;
    msg.bearing = k.bearingDeg;
    msg.hdop = k.hdop;
    msg.fixQuality = k.fixQuality;

    std::string payload = network::Serializer::Serialize(msg);

    // 1. Primary fleet location topic (ICD specification)
    PublishOrBuffer("aegis/fleet/" + m_unitId + "/location", payload, 0);

    // 2. Backward-compatible ambulance topic (for unmodified Coordinator Core)
    PublishOrBuffer("aegis/ambulance/" + m_unitId + "/gps", payload, 0);

    // 3. Ad-hoc V2V BSM broadcast
    m_v2v.BroadcastSafetyBeacon(m_unitId, k, GetState());

    return true;
}

bool AmbulanceNode::PublishVitalsOnce() {
    PatientVitals v = m_vitals.SampleVitals();

    network::VitalsUpdateMsg msg;
    msg.unitId = m_unitId;
    msg.patientId = m_patientId;
    msg.timestamp = v.timestampMs;
    msg.heartRate = v.heartRate;
    msg.spO2 = v.spO2;
    msg.systolicBP = v.systolicBP;
    msg.diastolicBP = v.diastolicBP;
    msg.respirationRate = v.respirationRate;
    msg.gcsScore = v.gcsScore;

    std::string payload = network::Serializer::Serialize(msg);
    PublishOrBuffer("aegis/fleet/" + m_unitId + "/vitals", payload, 1);
    return true;
}

bool AmbulanceNode::PublishHeartbeatOnce() {
    network::HeartbeatMsg hb;
    hb.nodeId = m_unitId;
    hb.timestamp = common::GetUtcMilliseconds();
    hb.cpuUsagePct = 14.2f;
    hb.ramUsageMb = 42.5f;
    hb.processStatus = GetStateString();

    std::string payload = network::Serializer::Serialize(hb);
    PublishOrBuffer("aegis/fleet/" + m_unitId + "/heartbeat", payload, 0);
    return true;
}

bool AmbulanceNode::PublishStateOnce() {
    uint64_t now = common::GetUtcMilliseconds();
    std::string stateStr = GetStateString();
    std::string payload = "{\"unitId\":\"" + m_unitId +
                          "\",\"state\":\"" + stateStr +
                          "\",\"timestamp\":" + std::to_string(now) + "}";

    PublishOrBuffer("aegis/fleet/" + m_unitId + "/state", payload, 2);
    return true;
}

void AmbulanceNode::PublishOrBuffer(const std::string& topic, const std::string& payload, int qos) {
    if (m_network && m_network->IsConnected()) {
        FlushBufferedTelemetry();
        m_network->Publish(topic, payload, qos);
    } else {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        if (m_telemetryBuffer.size() >= MAX_BUFFER_CAPACITY) {
            m_telemetryBuffer.pop_front();
        }
        BufferedTelemetryPacket pkt;
        pkt.topic = topic;
        pkt.payload = payload;
        pkt.qos = qos;
        pkt.timestampMs = common::GetUtcMilliseconds();
        m_telemetryBuffer.push_back(std::move(pkt));
    }
}

size_t AmbulanceNode::FlushBufferedTelemetry() {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    if (!m_network || !m_network->IsConnected() || m_telemetryBuffer.empty()) {
        return 0;
    }

    size_t flushed = 0;
    while (!m_telemetryBuffer.empty()) {
        const auto& pkt = m_telemetryBuffer.front();
        m_network->Publish(pkt.topic, pkt.payload, pkt.qos);
        m_telemetryBuffer.pop_front();
        flushed++;
    }
    return flushed;
}

size_t AmbulanceNode::GetBufferedTelemetryCount() const {
    std::lock_guard<std::mutex> lock(m_bufferMutex);
    return m_telemetryBuffer.size();
}

void AmbulanceNode::RunTelemetryLoop() {
    while (m_isRunning.load()) {
        auto startTime = std::chrono::steady_clock::now();
        uint64_t now = common::GetUtcMilliseconds();

        // 10 Hz Kinematics loop (every 100 ms)
        if (now - m_last10HzMs >= 100) {
            PublishKinematicsOnce();
            m_last10HzMs = now;
        }

        // 1 Hz Vitals & Heartbeat loop (every 1000 ms)
        if (now - m_last1HzMs >= 1000) {
            PublishVitalsOnce();
            PublishHeartbeatOnce();
            FlushBufferedTelemetry();
            m_last1HzMs = now;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed < 50) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50 - elapsed));
        }
    }
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
