/**
 * @file CoordinatorManager.cpp
 * @brief Implementation of CoordinatorManager.
 */

#include "CoordinatorManager.h"
#include <iostream>

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
    , m_running(false) {
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
        case CoordinatorEventType::CLIENT_DISCONNECTED: {
            std::string intersectionId;
            if (m_intersectionRegistry.FindByClientId(event.clientId, intersectionId)) {
                m_intersectionRegistry.SetOnlineStatus(intersectionId, false);
                m_intersectionState.SetOffline(intersectionId);
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
    const std::string& intersectionId = msg.GetSenderId();
    if (intersectionId.empty()) {
        return;
    }

    std::string ip = "0.0.0.0";
    uint16_t port = 0;
    m_socketServer.GetClientInfo(clientId, ip, port);

    // Register node in registry and state tracker
    m_intersectionRegistry.RegisterIntersection(intersectionId, ip, port, clientId);
    m_intersectionState.RegisterIntersection(intersectionId);
    m_intersectionState.SetOnline(intersectionId, msg.GetTimestamp());

    // Send ACK back to the intersection node
    communication::Message ackMsg(communication::MessageType::ACK,
                                  "COORDINATOR",
                                  "REGISTER_OK",
                                  communication::Message::CurrentTimestampMs());
    m_socketServer.SendToClient(clientId, ackMsg);
}

void CoordinatorManager::HandleHeartbeat(int clientId, const communication::Message& msg) {
    const std::string& intersectionId = msg.GetSenderId();
    if (!intersectionId.empty()) {
        m_intersectionRegistry.UpdateHeartbeat(intersectionId, msg.GetTimestamp());
        m_intersectionRegistry.AssociateClientId(intersectionId, clientId);
        m_intersectionState.SetOnline(intersectionId, msg.GetTimestamp());
    }
}

void CoordinatorManager::HandleSignalStatus(int clientId, const communication::Message& msg) {
    const std::string& intersectionId = msg.GetSenderId();
    if (!intersectionId.empty()) {
        m_intersectionRegistry.UpdateHeartbeat(intersectionId, msg.GetTimestamp());
        m_intersectionRegistry.AssociateClientId(intersectionId, clientId);
        m_intersectionState.SetOnline(intersectionId, msg.GetTimestamp());
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

} // namespace coordinator
} // namespace aegis
