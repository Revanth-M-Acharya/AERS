/**
 * @file IntersectionNode.cpp
 * @brief Implementation of IntersectionNode.
 */

#include "IntersectionNode.h"
#include "aegis/common/json_helper.hpp"
#include <thread>
#include <iostream>

namespace aegis {
namespace intersection {

IntersectionNode::IntersectionNode(std::string intersectionId, uint32_t heartbeatIntervalMs)
    : m_intersectionId(std::move(intersectionId))
    , m_coordinatorIp("")
    , m_coordinatorPort(0)
    , m_connected(false)
    , m_registered(false)
    , m_running(false)
    , m_socketClient()
    , m_heartbeatManager(m_intersectionId, heartbeatIntervalMs)
    , m_signalController()
    , m_trafficSignalNode(m_intersectionId, traffic_signal::SignalState::RED) {
}

IntersectionNode::~IntersectionNode() {
    Stop();
}

IntersectionNode::IntersectionNode(IntersectionNode&& other) noexcept {
    Stop();
    other.Stop();

    m_intersectionId = std::move(other.m_intersectionId);
    m_coordinatorIp = std::move(other.m_coordinatorIp);
    m_coordinatorPort = other.m_coordinatorPort;
    m_connected.store(other.m_connected.load());
    m_registered.store(other.m_registered.load());
    m_running.store(other.m_running.load());

    m_socketClient = std::move(other.m_socketClient);
    m_heartbeatManager = std::move(other.m_heartbeatManager);
    m_signalController = std::move(other.m_signalController);
    m_trafficSignalNode = std::move(other.m_trafficSignalNode);

    std::lock_guard<std::mutex> lock(other.m_configMutex);
    m_commandCallback = std::move(other.m_commandCallback);
}

IntersectionNode& IntersectionNode::operator=(IntersectionNode&& other) noexcept {
    if (this != &other) {
        Stop();
        other.Stop();

        m_intersectionId = std::move(other.m_intersectionId);
        m_coordinatorIp = std::move(other.m_coordinatorIp);
        m_coordinatorPort = other.m_coordinatorPort;
        m_connected.store(other.m_connected.load());
        m_registered.store(other.m_registered.load());
        m_running.store(other.m_running.load());

        m_socketClient = std::move(other.m_socketClient);
        m_heartbeatManager = std::move(other.m_heartbeatManager);
        m_signalController = std::move(other.m_signalController);
        m_trafficSignalNode = std::move(other.m_trafficSignalNode);

        std::scoped_lock lock(m_configMutex, other.m_configMutex);
        m_commandCallback = std::move(other.m_commandCallback);
    }
    return *this;
}

bool IntersectionNode::Start(const std::string& coordinatorIp,
                             uint16_t coordinatorPort,
                             uint32_t connectTimeoutMs,
                             uint32_t ackTimeoutMs) {
    if (m_running.load()) {
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_coordinatorIp = coordinatorIp;
        m_coordinatorPort = coordinatorPort;
    }

    // Configure SocketClient callbacks
    m_socketClient.SetMessageHandler([this](const communication::Message& msg) {
        HandleIncomingMessage(msg);
    });

    m_socketClient.SetConnectionHandler([this](bool connected) {
        OnConnectionStateChanged(connected);
    });

    m_socketClient.SetAutoReconnect(true, 1000, 0);

    // Configure HeartbeatManager callback to push heartbeat via SocketClient
    m_heartbeatManager.SetCallback([this](const communication::Message& hbMsg) {
        if (m_connected.load() && m_registered.load()) {
            m_socketClient.Send(hbMsg);
        }
    });

    // Connect to Coordinator
    if (!ConnectToCoordinator(coordinatorIp, coordinatorPort, connectTimeoutMs)) {
        return false;
    }

    m_running.store(true);

    // Perform registration handshake
    return RegisterToCoordinator(ackTimeoutMs);
}

void IntersectionNode::Stop() {
    if (!m_running.exchange(false)) {
        m_heartbeatManager.Stop();
        m_socketClient.Disconnect();
        return;
    }

    m_heartbeatManager.Stop();
    m_socketClient.Disconnect();

    m_connected.store(false);
    m_registered.store(false);

    m_ackCv.notify_all();
}

void IntersectionNode::Disconnect() {
    m_heartbeatManager.Stop();
    m_socketClient.Disconnect();
    m_connected.store(false);
    m_registered.store(false);
    m_ackCv.notify_all();
}

bool IntersectionNode::ConnectToCoordinator(const std::string& coordinatorIp,
                                            uint16_t coordinatorPort,
                                            uint32_t timeoutMs) {
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        m_coordinatorIp = coordinatorIp;
        m_coordinatorPort = coordinatorPort;
    }

    bool success = m_socketClient.Connect(coordinatorIp, coordinatorPort, timeoutMs);
    m_connected.store(success);
    return success;
}

bool IntersectionNode::RegisterToCoordinator(uint32_t ackTimeoutMs) {
    if (!m_connected.load()) {
        return false;
    }

    m_registered.store(false);

    communication::Message regMsg(communication::MessageType::REGISTER,
                                  m_intersectionId,
                                  "{\"nodeType\":\"INTERSECTION\"}",
                                  communication::Message::CurrentTimestampMs());

    if (!m_socketClient.Send(regMsg)) {
        return false;
    }

    // Wait for ACK
    std::unique_lock<std::mutex> lock(m_ackMutex);
    bool ok = m_ackCv.wait_for(lock, std::chrono::milliseconds(ackTimeoutMs), [this]() {
        return m_registered.load() || !m_connected.load() || !m_running.load();
    });

    if (ok && m_registered.load()) {
        // Start heartbeat automatically upon successful registration
        m_heartbeatManager.Start();
        return true;
    }

    return false;
}

bool IntersectionNode::Reconnect(uint32_t timeoutMs) {
    m_running.store(true);
    Disconnect();

    std::string ip;
    uint16_t port;
    {
        std::lock_guard<std::mutex> lock(m_configMutex);
        ip = m_coordinatorIp;
        port = m_coordinatorPort;
    }

    if (!ConnectToCoordinator(ip, port, timeoutMs)) {
        return false;
    }

    return RegisterToCoordinator(timeoutMs);
}

void IntersectionNode::HandleIncomingMessage(const communication::Message& message) {
    switch (message.GetType()) {
        case communication::MessageType::ACK: {
            m_registered.store(true);
            m_heartbeatManager.Start();
            m_ackCv.notify_all();
            break;
        }

        case communication::MessageType::COMMAND: {
            ProcessCoordinatorCommand(message);
            break;
        }

        default:
            break;
    }
}

bool IntersectionNode::ProcessCoordinatorCommand(const communication::Message& commandMessage) {
    const std::string& payload = commandMessage.GetPayload();
    std::string commandName = payload;
    std::string reason = "Coordinator Command";

    if (!payload.empty() && payload.front() == '{') {
        auto cmdOpt = common::JsonHelper::ExtractString(payload, "command");
        if (cmdOpt.has_value()) {
            commandName = cmdOpt.value();
        } else {
            auto actOpt = common::JsonHelper::ExtractString(payload, "action");
            if (actOpt.has_value()) {
                commandName = actOpt.value();
            }
        }
        auto reasonOpt = common::JsonHelper::ExtractString(payload, "reason");
        if (reasonOpt.has_value()) {
            reason = reasonOpt.value();
        }
    }

    return ProcessCoordinatorCommand(commandName, reason);
}

bool IntersectionNode::ProcessCoordinatorCommand(const std::string& commandName, const std::string& reason) {
    // Requirement 10: IntersectionNode shall log: [COMMAND RECEIVED]
    std::cout << "[COMMAND RECEIVED]" << std::endl;

    {
        std::lock_guard<std::mutex> lock(m_receivedCommandsMutex);
        m_commandsReceivedCount++;
        m_lastReceivedCommand = commandName;
        m_receivedCommands.push_back(commandName);
    }

    // Requirement 5 & 6: Forward to owned TrafficSignalNode
    bool success = m_trafficSignalNode.HandleCoordinatorCommand(commandName, reason);

    // Keep legacy SignalController in sync for backwards compatibility with earlier phases
    if (commandName.find("SET_GREEN") != std::string::npos || commandName == "GREEN") {
        m_signalController.SetGreen();
    } else if (commandName.find("SET_RED") != std::string::npos || commandName == "RED" || commandName.find("SET_ALL_RED") != std::string::npos) {
        m_signalController.SetRed();
    } else if (commandName.find("SET_YELLOW") != std::string::npos || commandName == "YELLOW") {
        m_signalController.SetYellow();
    } else if (commandName.find("EMERGENCY_MODE_ON") != std::string::npos) {
        m_signalController.EnableEmergency();
        success = true;
    } else if (commandName.find("EMERGENCY_MODE_OFF") != std::string::npos) {
        m_signalController.DisableEmergency();
        success = true;
    } else if (commandName.find("SET_NORMAL") != std::string::npos) {
        m_signalController.SetNormal();
        success = true;
    }

    // Trigger command callback if registered
    {
        CommandCallback cb = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            cb = m_commandCallback;
        }
        if (cb) {
            cb(commandName);
        }
    }

    // Requirement 11, 12, 13: Send acknowledgement to Coordinator
    // If invalid transition/command, return error acknowledgement
    std::string currentStateStr = m_trafficSignalNode.GetCurrentStateString();
    std::string status = success ? "OK" : "ERROR";

    std::string ackPayload = "{\"status\":\"" + status +
                             "\",\"intersectionId\":\"" + m_intersectionId +
                             "\",\"currentState\":\"" + currentStateStr +
                             "\",\"command\":\"" + commandName + "\"}";

    communication::Message ackMsg(communication::MessageType::ACK,
                                  m_intersectionId,
                                  ackPayload,
                                  communication::Message::CurrentTimestampMs());

    if (m_socketClient.IsConnected()) {
        m_socketClient.Send(ackMsg);
    }

    {
        std::lock_guard<std::mutex> lock(m_ackCbMutex);
        if (m_ackCallback) {
            m_ackCallback(ackMsg);
        }
    }

    return success;
}

void IntersectionNode::OnConnectionStateChanged(bool connected) {
    m_connected.store(connected);

    if (!connected) {
        m_registered.store(false);
        m_heartbeatManager.Stop();
        m_ackCv.notify_all();
    } else {
        // Reconnected! Re-register and resume heartbeat automatically
        if (m_running.load()) {
            std::thread(&IntersectionNode::AutoRegisterWorker, this).detach();
        }
    }
}

void IntersectionNode::AutoRegisterWorker() {
    RegisterToCoordinator(3000);
}

std::string IntersectionNode::GetCoordinatorIp() const {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_coordinatorIp;
}

uint16_t IntersectionNode::GetCoordinatorPort() const noexcept {
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_coordinatorPort;
}

bool IntersectionNode::IsConnected() const noexcept {
    return m_connected.load() && m_socketClient.IsConnected();
}

bool IntersectionNode::IsRegistered() const noexcept {
    return m_registered.load();
}

bool IntersectionNode::IsRunning() const noexcept {
    return m_running.load();
}

void IntersectionNode::SetCommandCallback(CommandCallback callback) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_commandCallback = std::move(callback);
}

void IntersectionNode::SetAckCallback(AckCallback callback) {
    std::lock_guard<std::mutex> lock(m_ackCbMutex);
    m_ackCallback = std::move(callback);
}

size_t IntersectionNode::GetCommandsReceivedCount() const {
    std::lock_guard<std::mutex> lock(m_receivedCommandsMutex);
    return m_commandsReceivedCount;
}

std::vector<std::string> IntersectionNode::GetReceivedCommands() const {
    std::lock_guard<std::mutex> lock(m_receivedCommandsMutex);
    return m_receivedCommands;
}

std::string IntersectionNode::GetLastReceivedCommand() const {
    std::lock_guard<std::mutex> lock(m_receivedCommandsMutex);
    return m_lastReceivedCommand;
}

void IntersectionNode::ClearReceivedCommands() {
    std::lock_guard<std::mutex> lock(m_receivedCommandsMutex);
    m_commandsReceivedCount = 0;
    m_lastReceivedCommand.clear();
    m_receivedCommands.clear();
}

} // namespace intersection
} // namespace aegis
