/**
 * @file CoordinatorManager.cpp
 * @brief Implementation of CoordinatorManager.
 */

#include "CoordinatorManager.h"
#include "aegis/common/json_helper.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace aegis {
namespace coordinator {

CoordinatorManager::CoordinatorManager()
    : m_intersectionRegistry()
    , m_eventQueue(10000)
    , m_socketServer()
    , m_intersectionState()
    , m_corridor()
    , m_trafficCommandManager("COORDINATOR")
    , m_commandsMutex()
    , m_generatedCommands()
    , m_running(false)
    , m_nodeTableMutex()
    , m_connectedNodes()
    , m_nodeDisplayOrder()
    , m_clientIdToNodeId() {
    m_nodeDisplayOrder = {"INT_A", "INT_B", "INT_C", "AMB_01", "HOSPITAL"};
    m_connectedNodes["INT_A"] = ConnectedNodeInfo{"INT_A", "Intersection", "", 0, "OFFLINE", 0, -1, false};
    m_connectedNodes["INT_B"] = ConnectedNodeInfo{"INT_B", "Intersection", "", 0, "OFFLINE", 0, -1, false};
    m_connectedNodes["INT_C"] = ConnectedNodeInfo{"INT_C", "Intersection", "", 0, "OFFLINE", 0, -1, false};
    m_connectedNodes["AMB_01"] = ConnectedNodeInfo{"AMB_01", "Ambulance", "", 0, "OFFLINE", 0, -1, false};
    m_connectedNodes["HOSPITAL"] = ConnectedNodeInfo{"HOSPITAL", "Hospital", "", 0, "OFFLINE", 0, -1, false};
}

CoordinatorManager::~CoordinatorManager() {
    Stop();
}

CoordinatorManager::CoordinatorManager(CoordinatorManager&& other) noexcept {
    Stop();
    other.Stop();

    m_intersectionRegistry = std::move(other.m_intersectionRegistry);
    m_eventQueue = std::move(other.m_eventQueue);
    m_socketServer = std::move(other.m_socketServer);
    m_intersectionState = std::move(other.m_intersectionState);
    m_corridor = std::move(other.m_corridor);
    m_trafficCommandManager = std::move(other.m_trafficCommandManager);
    m_generatedCommands = std::move(other.m_generatedCommands);
    m_customHandlers = std::move(other.m_customHandlers);
    m_eventHook = std::move(other.m_eventHook);
    m_connectedNodes = std::move(other.m_connectedNodes);
    m_nodeDisplayOrder = std::move(other.m_nodeDisplayOrder);
    m_clientIdToNodeId = std::move(other.m_clientIdToNodeId);
}

CoordinatorManager& CoordinatorManager::operator=(CoordinatorManager&& other) noexcept {
    if (this != &other) {
        Stop();
        other.Stop();

        m_intersectionRegistry = std::move(other.m_intersectionRegistry);
        m_eventQueue = std::move(other.m_eventQueue);
        m_socketServer = std::move(other.m_socketServer);
        m_intersectionState = std::move(other.m_intersectionState);
        m_corridor = std::move(other.m_corridor);
        m_trafficCommandManager = std::move(other.m_trafficCommandManager);
        m_generatedCommands = std::move(other.m_generatedCommands);
        m_customHandlers = std::move(other.m_customHandlers);
        m_eventHook = std::move(other.m_eventHook);
        m_connectedNodes = std::move(other.m_connectedNodes);
        m_nodeDisplayOrder = std::move(other.m_nodeDisplayOrder);
        m_clientIdToNodeId = std::move(other.m_clientIdToNodeId);
    }
    return *this;
}

bool CoordinatorManager::Start(uint16_t port, const std::string& host) {
    if (m_running.load()) {
        return true;
    }

    // Configure SocketServer event hooks to feed into EventQueue
    m_socketServer.SetConnectHandler([this](int clientId, const std::string& ip, uint16_t clientPort) {
        std::string details = ip + ":" + std::to_string(clientPort);
        CoordinatorEvent event(CoordinatorEventType::CLIENT_CONNECTED,
                               clientId,
                               "",
                               communication::Message(),
                               communication::Message::CurrentTimestampMs(),
                               std::move(details));
        m_eventQueue.PushEvent(std::move(event));
    });

    m_socketServer.SetDisconnectHandler([this](int clientId) {
        CoordinatorEvent event(CoordinatorEventType::CLIENT_DISCONNECTED,
                               clientId,
                               "",
                               communication::Message(),
                               communication::Message::CurrentTimestampMs(),
                               "Client connection terminated");
        m_eventQueue.PushEvent(std::move(event));
    });

    m_socketServer.SetMessageHandler([this](int clientId, const communication::Message& msg) {
        CoordinatorEvent event(CoordinatorEventType::MESSAGE_RECEIVED,
                               clientId,
                               msg.GetSenderId(),
                               msg,
                               msg.GetTimestamp(),
                               "");
        m_eventQueue.PushEvent(std::move(event));
    });

    // Start network server
    if (!m_socketServer.Start(port, host)) {
        return false;
    }

    m_running.store(true);
    m_dispatchWorker = std::thread(&CoordinatorManager::DispatchWorkerLoop, this);

    return true;
}

void CoordinatorManager::Stop() {
    if (!m_running.exchange(false)) {
        m_socketServer.Stop();
        return;
    }

    // Unblock and join dispatch thread
    m_eventQueue.Shutdown();
    if (m_dispatchWorker.joinable()) {
        m_dispatchWorker.join();
    }

    // Stop communication server
    m_socketServer.Stop();

    // Mark remaining registered intersections as offline
    auto intersections = m_intersectionRegistry.GetAllIntersections();
    for (const auto& entry : intersections) {
        m_intersectionRegistry.SetOnlineStatus(entry.intersectionId, false);
        m_intersectionState.SetOffline(entry.intersectionId);
    }

    {
        std::lock_guard<std::mutex> lock(m_nodeTableMutex);
        for (auto& pair : m_connectedNodes) {
            pair.second.connectionStatus = "OFFLINE";
            pair.second.clientId = -1;
        }
        m_clientIdToNodeId.clear();
    }
}

bool CoordinatorManager::IsRunning() const noexcept {
    return m_running.load();
}

void CoordinatorManager::SetHandler(communication::MessageType type, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_customHandlers[type] = std::move(handler);
}

void CoordinatorManager::SetEventHook(EventHook hook) {
    std::lock_guard<std::mutex> lock(m_handlersMutex);
    m_eventHook = std::move(hook);
}

void CoordinatorManager::DispatchWorkerLoop() {
    while (m_running.load()) {
        CoordinatorEvent event;
        if (m_eventQueue.PopEvent(event, 50)) {
            DispatchEvent(event);
        }
    }
}

void CoordinatorManager::DispatchEvent(const CoordinatorEvent& event) {
    EventHook hook = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_handlersMutex);
        hook = m_eventHook;
    }
    if (hook) {
        hook(event);
    }

    switch (event.type) {
        case CoordinatorEventType::CLIENT_CONNECTED: {
            std::cout << "[CONNECT] Intersection node connected from " << event.details << std::endl;
            break;
        }

        case CoordinatorEventType::CLIENT_DISCONNECTED: {
            std::string nodeId;
            {
                std::lock_guard<std::mutex> lock(m_nodeTableMutex);
                auto it = m_clientIdToNodeId.find(event.clientId);
                if (it != m_clientIdToNodeId.end()) {
                    nodeId = it->second;
                    m_clientIdToNodeId.erase(it);
                }
            }
            if (nodeId.empty()) {
                m_intersectionRegistry.FindByClientId(event.clientId, nodeId);
            }

            if (!nodeId.empty()) {
                m_intersectionRegistry.SetOnlineStatus(nodeId, false);
                m_intersectionState.SetOffline(nodeId);
                {
                    std::lock_guard<std::mutex> lock(m_nodeTableMutex);
                    auto it = m_connectedNodes.find(nodeId);
                    if (it != m_connectedNodes.end()) {
                        it->second.connectionStatus = "OFFLINE";
                        it->second.clientId = -1;
                    }
                }
                std::cout << "[DISCONNECT] " << nodeId << " disconnected" << std::endl;
            }
            break;
        }

        case CoordinatorEventType::MESSAGE_RECEIVED: {
            // Built-in handlers for registration, heartbeat, and status
            switch (event.message.GetType()) {
                case communication::MessageType::REGISTER:
                    HandleRegister(event.clientId, event.message);
                    break;
                case communication::MessageType::HEARTBEAT:
                    HandleHeartbeat(event.clientId, event.message);
                    break;
                case communication::MessageType::SIGNAL_STATUS:
                    HandleSignalStatus(event.clientId, event.message);
                    break;
                default:
                    break;
            }

            // Invoke registered custom handler if present
            MessageHandler customHandler = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_handlersMutex);
                auto it = m_customHandlers.find(event.message.GetType());
                if (it != m_customHandlers.end()) {
                    customHandler = it->second;
                }
            }
            if (customHandler) {
                customHandler(event.clientId, event.message);
            }
            break;
        }

        default:
            break;
    }
}

void CoordinatorManager::HandleRegister(int clientId, const communication::Message& msg) {
    const std::string& nodeId = msg.GetSenderId();
    if (nodeId.empty()) {
        return;
    }

    std::string ip = "0.0.0.0";
    uint16_t port = 0;
    m_socketServer.GetClientInfo(clientId, ip, port);

    // Register node in registry and state tracker
    m_intersectionRegistry.RegisterIntersection(nodeId, ip, port, clientId);
    m_intersectionState.RegisterIntersection(nodeId);
    m_intersectionState.SetOnline(nodeId, msg.GetTimestamp());

    // Send ACK back to the intersection node
    communication::Message ackMsg(communication::MessageType::ACK,
                                  "COORDINATOR",
                                  "REGISTER_OK",
                                  communication::Message::CurrentTimestampMs());
    m_socketServer.SendToClient(clientId, ackMsg);

    std::string role = ExtractNodeRole(nodeId, msg.GetPayload());

    bool isReconnect = false;
    {
        std::lock_guard<std::mutex> lock(m_nodeTableMutex);
        auto it = m_connectedNodes.find(nodeId);
        if (it != m_connectedNodes.end()) {
            if (it->second.everRegistered && it->second.connectionStatus == "OFFLINE") {
                isReconnect = true;
            }
            it->second.role = role;
            it->second.ipAddress = ip;
            it->second.port = port;
            it->second.connectionStatus = "ONLINE";
            it->second.lastHeartbeatTimestamp = (msg.GetTimestamp() > 0) ? msg.GetTimestamp() : communication::Message::CurrentTimestampMs();
            it->second.clientId = clientId;
            it->second.everRegistered = true;
        } else {
            ConnectedNodeInfo info;
            info.nodeId = nodeId;
            info.role = role;
            info.ipAddress = ip;
            info.port = port;
            info.connectionStatus = "ONLINE";
            info.lastHeartbeatTimestamp = (msg.GetTimestamp() > 0) ? msg.GetTimestamp() : communication::Message::CurrentTimestampMs();
            info.clientId = clientId;
            info.everRegistered = true;
            m_connectedNodes[nodeId] = info;
            m_nodeDisplayOrder.push_back(nodeId);
        }
        m_clientIdToNodeId[clientId] = nodeId;
    }

    if (isReconnect) {
        std::cout << "[RECONNECT] " << nodeId << " reconnected" << std::endl;
    }

    std::cout << "[REGISTER] Node " << nodeId << " (" << role << ") registered successfully" << std::endl;
}

void CoordinatorManager::HandleHeartbeat(int clientId, const communication::Message& msg) {
    const std::string& nodeId = msg.GetSenderId();
    if (nodeId.empty()) {
        return;
    }

    m_intersectionRegistry.UpdateHeartbeat(nodeId, msg.GetTimestamp());
    m_intersectionRegistry.AssociateClientId(nodeId, clientId);
    m_intersectionState.SetOnline(nodeId, msg.GetTimestamp());

    bool isReconnect = false;
    {
        std::lock_guard<std::mutex> lock(m_nodeTableMutex);
        auto it = m_connectedNodes.find(nodeId);
        if (it != m_connectedNodes.end()) {
            if (it->second.everRegistered && it->second.connectionStatus == "OFFLINE") {
                isReconnect = true;
            }
            it->second.connectionStatus = "ONLINE";
            it->second.lastHeartbeatTimestamp = (msg.GetTimestamp() > 0) ? msg.GetTimestamp() : communication::Message::CurrentTimestampMs();
            it->second.clientId = clientId;
        } else {
            ConnectedNodeInfo info;
            info.nodeId = nodeId;
            info.role = DetermineRoleFromNodeId(nodeId);
            info.connectionStatus = "ONLINE";
            info.lastHeartbeatTimestamp = (msg.GetTimestamp() > 0) ? msg.GetTimestamp() : communication::Message::CurrentTimestampMs();
            info.clientId = clientId;
            info.everRegistered = true;
            m_connectedNodes[nodeId] = info;
            m_nodeDisplayOrder.push_back(nodeId);
        }
        m_clientIdToNodeId[clientId] = nodeId;
    }

    if (isReconnect) {
        std::cout << "[RECONNECT] " << nodeId << " reconnected" << std::endl;
    }

    std::cout << "[HEARTBEAT] " << nodeId << " alive" << std::endl;
}

void CoordinatorManager::HandleSignalStatus(int clientId, const communication::Message& msg) {
    const std::string& nodeId = msg.GetSenderId();
    if (!nodeId.empty()) {
        m_intersectionRegistry.UpdateHeartbeat(nodeId, msg.GetTimestamp());
        m_intersectionRegistry.AssociateClientId(nodeId, clientId);
        m_intersectionState.SetOnline(nodeId, msg.GetTimestamp());

        std::lock_guard<std::mutex> lock(m_nodeTableMutex);
        auto it = m_connectedNodes.find(nodeId);
        if (it != m_connectedNodes.end()) {
            it->second.connectionStatus = "ONLINE";
            it->second.lastHeartbeatTimestamp = (msg.GetTimestamp() > 0) ? msg.GetTimestamp() : communication::Message::CurrentTimestampMs();
            it->second.clientId = clientId;
        }
    }
}

bool CoordinatorManager::CreateCorridor(const std::string& corridorId,
                                        const std::vector<std::string>& intersectionIds) {
    if (corridorId.empty() || intersectionIds.empty()) {
        return false;
    }

    for (const auto& id : intersectionIds) {
        m_intersectionState.RegisterIntersection(id);
    }

    return m_corridor.CreateCorridor(corridorId, intersectionIds);
}

bool CoordinatorManager::ActivateCorridor() {
    if (!m_corridor.Activate()) {
        return false;
    }

    std::string currentInt = m_corridor.CurrentIntersection();
    if (currentInt.empty()) {
        return false;
    }

    std::vector<communication::Message> generated;

    // 1. Generate GREEN command for current active intersection
    communication::Message greenCmd = m_trafficCommandManager.CreateGreenCommand(currentInt);
    m_intersectionState.UpdateState(currentInt, SignalState::GREEN);
    generated.push_back(greenCmd);

    // 2. Generate RED commands for other intersections in the corridor
    std::vector<std::string> allInts = m_corridor.GetIntersections();
    for (const auto& id : allInts) {
        if (id != currentInt) {
            communication::Message redCmd = m_trafficCommandManager.CreateRedCommand(id);
            m_intersectionState.UpdateState(id, SignalState::RED);
            generated.push_back(redCmd);
        }
    }

    // 3. Store generated commands (do not transmit automatically)
    {
        std::lock_guard<std::mutex> lock(m_commandsMutex);
        m_generatedCommands.insert(m_generatedCommands.end(), generated.begin(), generated.end());
    }

    return true;
}

bool CoordinatorManager::AdvanceCorridor() {
    std::string prevInt = m_corridor.CurrentIntersection();
    bool hasNext = m_corridor.AdvanceToNextIntersection();
    std::string currentInt = m_corridor.CurrentIntersection();

    std::vector<communication::Message> generated;

    if (hasNext && !currentInt.empty()) {
        // Set new active intersection to GREEN
        communication::Message greenCmd = m_trafficCommandManager.CreateGreenCommand(currentInt);
        m_intersectionState.UpdateState(currentInt, SignalState::GREEN);
        generated.push_back(greenCmd);

        // Previous intersection reset to RED
        if (!prevInt.empty() && prevInt != currentInt) {
            communication::Message prevRedCmd = m_trafficCommandManager.CreateRedCommand(prevInt);
            m_intersectionState.UpdateState(prevInt, SignalState::RED);
            generated.push_back(prevRedCmd);
        }

        // Remaining down-path intersections hold RED
        std::vector<std::string> allInts = m_corridor.GetIntersections();
        for (const auto& id : allInts) {
            if (id != currentInt && id != prevInt) {
                communication::Message redCmd = m_trafficCommandManager.CreateRedCommand(id);
                m_intersectionState.UpdateState(id, SignalState::RED);
                generated.push_back(redCmd);
            }
        }
    } else {
        // Corridor concluded, restore normal phasing for all corridor intersections
        std::vector<std::string> allInts = m_corridor.GetIntersections();
        for (const auto& id : allInts) {
            communication::Message normCmd = m_trafficCommandManager.CreateNormalCommand(id);
            m_intersectionState.UpdateState(id, SignalState::RED);
            generated.push_back(normCmd);
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_commandsMutex);
        m_generatedCommands.insert(m_generatedCommands.end(), generated.begin(), generated.end());
    }

    return hasNext;
}

bool CoordinatorManager::CompleteCorridor() {
    if (!m_corridor.Complete()) {
        return false;
    }

    std::vector<communication::Message> generated;
    std::vector<std::string> allInts = m_corridor.GetIntersections();
    for (const auto& id : allInts) {
        communication::Message normCmd = m_trafficCommandManager.CreateNormalCommand(id);
        m_intersectionState.UpdateState(id, SignalState::RED);
        generated.push_back(normCmd);
    }

    {
        std::lock_guard<std::mutex> lock(m_commandsMutex);
        m_generatedCommands.insert(m_generatedCommands.end(), generated.begin(), generated.end());
    }

    return true;
}

bool CoordinatorManager::CancelCorridor() {
    if (!m_corridor.Cancel()) {
        return false;
    }

    std::vector<communication::Message> generated;
    std::vector<std::string> allInts = m_corridor.GetIntersections();
    for (const auto& id : allInts) {
        communication::Message normCmd = m_trafficCommandManager.CreateNormalCommand(id);
        m_intersectionState.UpdateState(id, SignalState::RED);
        generated.push_back(normCmd);
    }

    {
        std::lock_guard<std::mutex> lock(m_commandsMutex);
        m_generatedCommands.insert(m_generatedCommands.end(), generated.begin(), generated.end());
    }

    return true;
}

std::vector<communication::Message> CoordinatorManager::GetGeneratedCommands() const {
    std::lock_guard<std::mutex> lock(m_commandsMutex);
    return m_generatedCommands;
}

std::vector<communication::Message> CoordinatorManager::PopGeneratedCommands() {
    std::lock_guard<std::mutex> lock(m_commandsMutex);
    std::vector<communication::Message> result = std::move(m_generatedCommands);
    m_generatedCommands.clear();
    return result;
}

void CoordinatorManager::ClearGeneratedCommands() {
    std::lock_guard<std::mutex> lock(m_commandsMutex);
    m_generatedCommands.clear();
}

size_t CoordinatorManager::GetGeneratedCommandsCount() const {
    std::lock_guard<std::mutex> lock(m_commandsMutex);
    return m_generatedCommands.size();
}

bool CoordinatorManager::SendMessageToIntersection(const std::string& intersectionId,
                                                   const communication::Message& message) {
    IntersectionEntry entry;
    if (!m_intersectionRegistry.GetIntersection(intersectionId, entry)) {
        return false;
    }

    if (!entry.isOnline || entry.clientId < 0) {
        return false;
    }

    return m_socketServer.SendToClient(entry.clientId, message);
}

bool CoordinatorManager::BroadcastMessage(const communication::Message& message) {
    return m_socketServer.Broadcast(message);
}

size_t CoordinatorManager::ProcessPendingEvents(size_t maxEvents) {
    size_t processed = 0;
    CoordinatorEvent event;
    while ((maxEvents == 0 || processed < maxEvents) && m_eventQueue.PopEvent(event, 0)) {
        DispatchEvent(event);
        processed++;
    }
    return processed;
}

std::string CoordinatorManager::DetermineRoleFromNodeId(const std::string& nodeId) {
    if (nodeId.rfind("INT", 0) == 0) {
        return "Intersection";
    }
    if (nodeId.rfind("AMB", 0) == 0) {
        return "Ambulance";
    }
    if (nodeId.rfind("HOSP", 0) == 0) {
        return "Hospital";
    }
    if (nodeId.rfind("COORD", 0) == 0) {
        return "Coordinator";
    }
    return "Node";
}

std::string CoordinatorManager::NormalizeRole(const std::string& rawRole, const std::string& nodeId) {
    if (rawRole.empty()) {
        return DetermineRoleFromNodeId(nodeId);
    }
    if (rawRole == "INTERSECTION" || rawRole == "intersection" || rawRole == "Intersection") {
        return "Intersection";
    }
    if (rawRole == "AMBULANCE" || rawRole == "ambulance" || rawRole == "Ambulance") {
        return "Ambulance";
    }
    if (rawRole == "HOSPITAL" || rawRole == "hospital" || rawRole == "Hospital") {
        return "Hospital";
    }
    if (rawRole == "COORDINATOR" || rawRole == "coordinator" || rawRole == "Coordinator") {
        return "Coordinator";
    }
    return rawRole;
}

std::string CoordinatorManager::ExtractNodeRole(const std::string& nodeId, const std::string& payload) {
    // 1. Try JSON extraction for "role"
    auto roleOpt = common::JsonHelper::ExtractString(payload, "role");
    if (roleOpt.has_value() && !roleOpt.value().empty()) {
        return NormalizeRole(roleOpt.value(), nodeId);
    }

    // 2. Try JSON extraction for "nodeType"
    auto typeOpt = common::JsonHelper::ExtractString(payload, "nodeType");
    if (typeOpt.has_value() && !typeOpt.value().empty()) {
        return NormalizeRole(typeOpt.value(), nodeId);
    }

    // 3. Try text heuristics in payload
    if (payload.find("AMBULANCE") != std::string::npos || payload.find("ambulance") != std::string::npos) {
        return "Ambulance";
    }
    if (payload.find("INTERSECTION") != std::string::npos || payload.find("intersection") != std::string::npos) {
        return "Intersection";
    }
    if (payload.find("HOSPITAL") != std::string::npos || payload.find("hospital") != std::string::npos) {
        return "Hospital";
    }

    // 4. Default role by node ID prefix
    return DetermineRoleFromNodeId(nodeId);
}

std::vector<ConnectedNodeInfo> CoordinatorManager::GetConnectedNodes() const {
    std::lock_guard<std::mutex> lock(m_nodeTableMutex);
    std::vector<ConnectedNodeInfo> result;
    result.reserve(m_nodeDisplayOrder.size());
    for (const auto& id : m_nodeDisplayOrder) {
        auto it = m_connectedNodes.find(id);
        if (it != m_connectedNodes.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

bool CoordinatorManager::GetConnectedNodeInfo(const std::string& nodeId, ConnectedNodeInfo& outInfo) const {
    std::lock_guard<std::mutex> lock(m_nodeTableMutex);
    auto it = m_connectedNodes.find(nodeId);
    if (it != m_connectedNodes.end()) {
        outInfo = it->second;
        return true;
    }
    return false;
}

std::string CoordinatorManager::GetNetworkStatusString() const {
    std::lock_guard<std::mutex> lock(m_nodeTableMutex);
    std::ostringstream oss;
    oss << "==============================\n";
    oss << "AERS NETWORK STATUS\n";
    oss << "Coordinator : " << (m_running.load() ? "ONLINE" : "OFFLINE") << "\n\n";
    oss << "Connected Nodes\n";
    oss << "------------------------------------\n";
    for (const auto& nodeId : m_nodeDisplayOrder) {
        auto it = m_connectedNodes.find(nodeId);
        std::string status = (it != m_connectedNodes.end()) ? it->second.connectionStatus : "OFFLINE";
        size_t width = std::max<size_t>(9, nodeId.length() + 1);
        oss << std::left << std::setw(static_cast<int>(width)) << nodeId << status << "\n";
    }
    oss << "------------------------------------\n";
    return oss.str();
}

void CoordinatorManager::PrintNetworkStatus() const {
    std::cout << GetNetworkStatusString();
}

void PrintNetworkStatus(const CoordinatorManager& coordinator) {
    coordinator.PrintNetworkStatus();
}

} // namespace coordinator
} // namespace aegis
