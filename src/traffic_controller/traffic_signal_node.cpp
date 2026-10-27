/**
 * @file traffic_signal_node.cpp
 * @brief Implementation of TrafficSignalNode with MQTT V2I communication and Coordinator integration.
 */

#include "aegis/edge/traffic_signal_node.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/json_helper.hpp"
#include "aegis/logger/logger.hpp"
#include "aegis/network/topics.hpp"

#include <thread>
#include <chrono>

namespace aegis {
namespace edge {
namespace traffic {

TrafficSignalNode::TrafficSignalNode()
    : m_controller(std::make_unique<TrafficController>()),
      m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {
    SetupTopics();
}

TrafficSignalNode::TrafficSignalNode(std::unique_ptr<TrafficController> controller)
    : m_controller(controller ? std::move(controller) : std::make_unique<TrafficController>()),
      m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {
    SetupTopics();
}

TrafficSignalNode::TrafficSignalNode(const TrafficTimingConfig& config)
    : m_controller(std::make_unique<TrafficController>(CreateSimulationGpio(), config)),
      m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {
    SetupTopics();
}

TrafficSignalNode::TrafficSignalNode(const std::string& intersectionId,
                                     const TrafficTimingConfig& config)
    : m_intersectionId(intersectionId),
      m_controller(std::make_unique<TrafficController>(CreateSimulationGpio(), config)),
      m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {
    SetupTopics();
}

TrafficSignalNode::TrafficSignalNode(const std::string& intersectionId,
                                     std::unique_ptr<TrafficController> controller,
                                     std::shared_ptr<network::NetworkManager> netManager)
    : m_intersectionId(intersectionId),
      m_controller(controller ? std::move(controller) : std::make_unique<TrafficController>()),
      m_network(netManager ? netManager : std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(netManager == nullptr) {
    SetupTopics();
}

TrafficSignalNode::TrafficSignalNode(const std::string& intersectionId,
                                     std::shared_ptr<network::NetworkManager> netManager)
    : m_intersectionId(intersectionId),
      m_controller(std::make_unique<TrafficController>()),
      m_network(netManager ? netManager : std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(netManager == nullptr) {
    SetupTopics();
}

TrafficSignalNode::~TrafficSignalNode() {
    Stop();
    Disconnect();
}

void TrafficSignalNode::SetupTopics() {
    m_commandTopic = network::topics::IntersectionCommands(m_intersectionId);
    m_statusTopic = network::topics::IntersectionStatus(m_intersectionId);
}

void TrafficSignalNode::SetIntersectionId(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    m_intersectionId = id;
    SetupTopics();
}

bool TrafficSignalNode::Start() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    if (!m_controller) {
        m_controller = std::make_unique<TrafficController>();
    }

    if (!m_controller->Initialize()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "SIGNAL_NODE", "Failed to initialize TrafficController.");
        return false;
    }

    if (!m_controller->Start()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "SIGNAL_NODE", "Failed to start TrafficController.");
        return false;
    }

    m_running.store(true);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "SIGNAL_NODE", "TrafficSignalNode started.");
    return true;
}

void TrafficSignalNode::Stop() {
    if (m_running.exchange(false)) {
        if (m_controller) {
            m_controller->Stop();
        }
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "TrafficSignalNode stopped.");
    }
}

void TrafficSignalNode::Run(uint32_t iterations, uint32_t stepMs) {
    if (!m_running.load()) {
        if (!Start()) {
            return;
        }
    }

    if (iterations > 0) {
        for (uint32_t i = 0; i < iterations && m_running.load(); ++i) {
            Tick(stepMs);
        }
    } else {
        while (m_running.load()) {
            Tick(stepMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        }
    }
}

void TrafficSignalNode::Tick(uint32_t deltaMs) {
    if (!m_running.load()) {
        return;
    }

    m_uptimeMs.fetch_add(deltaMs);

    // 1. Advance traffic controller deterministic cycle
    if (m_controller) {
        m_controller->Tick(deltaMs);
    }

    // 2. Publish periodic heartbeat every 1 second (1000ms)
    m_timeSinceLastHeartbeatPublishMs += deltaMs;
    if (m_timeSinceLastHeartbeatPublishMs >= 1000) {
        m_timeSinceLastHeartbeatPublishMs = 0;
        SendHeartbeat();
    }

    // 3. Coordinator Heartbeat Watchdog (5000ms timeout)
    if (IsConnected()) {
        m_timeSinceLastCoordinatorHeartbeatMs += deltaMs;
        if (m_timeSinceLastCoordinatorHeartbeatMs >= 5000 && !m_isHeartbeatTimedOut.load()) {
            m_isHeartbeatTimedOut.store(true);
            logger::Logger::Instance().Log(
                logger::LogLevel::WARN, "SIGNAL_NODE", "Heartbeat Timeout: entering FLASHING_YELLOW");
            if (m_controller) {
                m_controller->EnterFailSafe(SignalState::FLASHING_YELLOW);
            }
            PublishStatus(true);
        }
    }
}

bool TrafficSignalNode::Connect(const std::string& host, int port, bool useTls) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    m_brokerHost = host;
    m_brokerPort = port;
    m_brokerTls = useTls;

    if (!m_network) {
        m_network = std::make_shared<network::NetworkManager>();
        m_ownsNetwork = true;
    }

    bool ok = m_network->Initialize(host, port, useTls, "NODE-" + m_intersectionId);
    if (ok) {
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "MQTT Connected");
        Subscribe();
        m_timeSinceLastCoordinatorHeartbeatMs = 0;
        m_isHeartbeatTimedOut.store(false);
        PublishStatus(true);
    }
    return ok;
}

void TrafficSignalNode::Disconnect() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    if (m_network && m_network->IsConnected()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "MQTT Disconnected");
        m_network->Shutdown();
        m_isSubscribed.store(false);
    }
}

bool TrafficSignalNode::Reconnect() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    if (!m_network) {
        return false;
    }

    bool ok = m_network->Initialize(m_brokerHost, m_brokerPort, m_brokerTls, "NODE-" + m_intersectionId);
    if (ok) {
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "MQTT Connected");
        Subscribe();
        m_timeSinceLastCoordinatorHeartbeatMs = 0;
        m_isHeartbeatTimedOut.store(false);
        PublishStatus(true);
    }
    return ok;
}

bool TrafficSignalNode::Subscribe() {
    if (!m_network || !m_network->IsConnected()) {
        return false;
    }

    bool ok = m_network->Subscribe(m_commandTopic, [this](const std::string&, const std::string& payload) {
        ProcessCommand(payload);
    }, 1);

    // Also subscribe to legacy preemption topic for backward compatibility
    m_network->Subscribe("aegis/traffic/" + m_intersectionId + "/preempt", [this](const std::string&, const std::string& payload) {
        ProcessCommand(payload);
    }, 1);

    if (ok) {
        m_isSubscribed.store(true);
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Subscribed to " + m_commandTopic);
    }
    return ok;
}

bool TrafficSignalNode::PublishStatus(bool force) {
    if (!m_network || !m_network->IsConnected()) {
        return false;
    }

    network::IntersectionStatusMsg msg;
    msg.intersectionId = m_intersectionId;
    msg.currentState = m_controller ? m_controller->GetCurrentStateString() : "UNKNOWN";
    msg.emergencyActive = m_controller ? m_controller->IsPreemptionActive() : false;
    msg.recoveryActive = m_controller ? m_controller->IsRecoveryActive() : false;
    msg.heartbeatCounter = m_heartbeatCounter.load();
    msg.uptimeMs = m_uptimeMs.load();
    msg.timestamp = common::GetUtcMilliseconds();
    msg.health = GetHealthStatus();

    bool stateChanged = (!m_hasPublished ||
                         m_lastPublishedState != msg.currentState ||
                         m_lastPublishedEmergency != msg.emergencyActive ||
                         m_lastPublishedRecovery != msg.recoveryActive ||
                         m_lastPublishedHealth != msg.health ||
                         m_lastPublishedHbCounter != msg.heartbeatCounter);

    if (!force && !stateChanged) {
        return false; // Suppress duplicate publication
    }

    std::string payload = network::Serializer::Serialize(msg);
    m_lastPublishedPayload = payload;
    m_lastPublishedState = msg.currentState;
    m_lastPublishedEmergency = msg.emergencyActive;
    m_lastPublishedRecovery = msg.recoveryActive;
    m_lastPublishedHealth = msg.health;
    m_lastPublishedHbCounter = msg.heartbeatCounter;
    m_hasPublished = true;

    bool ok = m_network->Publish(m_statusTopic, payload, 1);
    if (ok) {
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Published status to " + m_statusTopic);
    }
    return ok;
}

bool TrafficSignalNode::SendHeartbeat() {
    m_heartbeatCounter.fetch_add(1);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "SIGNAL_NODE", "Heartbeat Sent (count=" + std::to_string(m_heartbeatCounter.load()) + ")");
    return PublishStatus(true);
}

bool TrafficSignalNode::ProcessCommand(const std::string& commandPayload) {
    std::string cmd;
    auto jsonCmd = network::Serializer::DeserializeIntersectionCommand(commandPayload);
    if (jsonCmd.IsOk()) {
        cmd = jsonCmd.Value().command;
    } else {
        auto legacyPreempt = network::Serializer::DeserializeTrafficPreemptionRequest(commandPayload);
        if (legacyPreempt.IsOk()) {
            cmd = "PREEMPT";
        } else {
            auto extracted = common::JsonHelper::ExtractString(commandPayload, "command");
            if (extracted) {
                cmd = *extracted;
            } else {
                cmd = commandPayload;
                while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n' || cmd.back() == ' ')) {
                    cmd.pop_back();
                }
                while (!cmd.empty() && (cmd.front() == ' ' || cmd.front() == '\t')) {
                    cmd.erase(cmd.begin());
                }
            }
        }
    }

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "SIGNAL_NODE", "Command Received: " + cmd);

    // Any valid message received from coordinator resets the heartbeat watchdog
    m_timeSinceLastCoordinatorHeartbeatMs = 0;
    if (m_isHeartbeatTimedOut.load()) {
        m_isHeartbeatTimedOut.store(false);
        if (m_controller) {
            m_controller->RestoreNormalCycle();
        }
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Coordinator communication restored. Normal cycle resumed.");
    }

    if (cmd == "PREEMPT") {
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Emergency Started");
        if (m_controller) {
            m_controller->RequestPreemption();
        }
        PublishStatus(true);
        return true;
    } else if (cmd == "CLEAR") {
        if (m_controller) {
            m_controller->ClearPreemption();
        }
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Emergency Cleared");
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "Recovery Started");
        PublishStatus(true);
        return true;
    } else if (cmd == "RESTORE") {
        if (m_controller) {
            m_controller->RestoreNormalCycle();
        }
        PublishStatus(true);
        return true;
    } else if (cmd == "HEARTBEAT") {
        // Coordinator heartbeat timer already reset above
        return true;
    } else if (cmd == "PING") {
        PublishStatus(true);
        return true;
    } else if (cmd == "STATUS_REQUEST") {
        PublishStatus(true);
        return true;
    } else {
        logger::Logger::Instance().Log(
            logger::LogLevel::WARN, "SIGNAL_NODE", "Unknown command: " + cmd);
        return false;
    }
}

bool TrafficSignalNode::IsRunning() const noexcept {
    return m_running.load();
}

bool TrafficSignalNode::IsConnected() const noexcept {
    return m_network && m_network->IsConnected();
}

SignalState TrafficSignalNode::GetCurrentState() const {
    if (!m_controller) {
        return SignalState::UNKNOWN;
    }
    return m_controller->GetCurrentState();
}

std::string TrafficSignalNode::GetHealthStatus() const {
    if (m_isHeartbeatTimedOut.load() ||
        (m_controller && m_controller->GetCurrentState() == SignalState::FLASHING_YELLOW)) {
        return "DEGRADED";
    }
    if (!IsConnected()) {
        return "NETWORK_DISCONNECTED";
    }
    return "HEALTHY";
}

TrafficController& TrafficSignalNode::GetController() noexcept {
    return *m_controller;
}

const TrafficController& TrafficSignalNode::GetController() const noexcept {
    return *m_controller;
}

} // namespace traffic
} // namespace edge
} // namespace aegis
