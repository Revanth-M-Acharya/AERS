/**
 * @file SystemManager.cpp
 * @brief Implementation of SystemManager integration layer.
 */

#include "SystemManager.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace aegis {
namespace system {

SystemManager::SystemManager()
    : SystemManager(Config{}) {
}

SystemManager::SystemManager(Config config)
    : m_config(std::move(config))
    , m_running(false)
    , m_coordinator(std::make_unique<coordinator::CoordinatorManager>())
    , m_nodes()
    , m_stateMutex()
    , m_syncMutex()
    , m_syncCv()
    , m_lastEventTimestampMs(0) {
    // Instantiate the three configured intersection nodes
    for (const auto& id : m_config.nodeIds) {
        m_nodes.push_back(std::make_shared<intersection::IntersectionNode>(
            id, m_config.heartbeatIntervalMs));
    }
}

SystemManager::~SystemManager() {
    StopSystem();
}

bool SystemManager::StartSystem(uint32_t timeoutMs) {
    if (m_running.load()) {
        return true;
    }

    // 1. Start Central Coordinator first
    if (!StartCoordinator()) {
        return false;
    }

    // 2. Start and connect all Intersection Nodes automatically
    if (!StartIntersectionNodes()) {
        StopCoordinator();
        return false;
    }

    // 3. Await complete network registration and heartbeat handshake
    if (!WaitForAllNodesRegistered(timeoutMs)) {
        StopSystem();
        return false;
    }

    m_running.store(true);
    return true;
}

void SystemManager::StopSystem() {
    m_running.store(false);

    // Stop edge nodes first to cleanly terminate socket connections and heartbeats
    StopIntersectionNodes();

    // Stop coordinator server and event dispatcher
    StopCoordinator();

    {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_syncCv.notify_all();
    }
}

bool SystemManager::IsRunning() const noexcept {
    return m_running.load();
}

bool SystemManager::StartCoordinator() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator) {
        m_coordinator = std::make_unique<coordinator::CoordinatorManager>();
    }

    if (m_coordinator->IsRunning()) {
        return true;
    }

    SetupCoordinatorEventHook();

    return m_coordinator->Start(m_config.coordinatorPort, m_config.coordinatorHost);
}

void SystemManager::StopCoordinator() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_coordinator && m_coordinator->IsRunning()) {
        m_coordinator->Stop();
    }
}

void SystemManager::SetupCoordinatorEventHook() {
    if (!m_coordinator) return;

    m_coordinator->SetEventHook([this](const coordinator::CoordinatorEvent& event) {
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            m_lastEventTimestampMs = event.timestamp;
        }
        m_syncCv.notify_all();
    });
}

bool SystemManager::StartIntersectionNodes() {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    for (auto& node : m_nodes) {
        if (!node) continue;
        // Node Start() connects, transmits REGISTER, awaits ACK, and begins heartbeat
        bool ok = node->Start(m_config.coordinatorHost,
                              m_config.coordinatorPort,
                              m_config.connectTimeoutMs,
                              m_config.ackTimeoutMs);
        if (!ok) {
            return false;
        }
    }
    return true;
}

void SystemManager::StopIntersectionNodes() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    for (auto& node : m_nodes) {
        if (node) {
            node->Stop();
        }
    }
}

bool SystemManager::WaitForAllNodesRegistered(uint32_t timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        bool allSatisfied = true;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (!m_coordinator || !m_coordinator->IsRunning()) {
                allSatisfied = false;
            } else {
                for (const auto& node : m_nodes) {
                    if (!node || !node->IsConnected() || !node->IsRegistered()) {
                        allSatisfied = false;
                        break;
                    }
                    coordinator::IntersectionEntry entry;
                    if (!m_coordinator->GetIntersectionRegistry().GetIntersection(
                            node->GetIntersectionId(), entry) || !entry.isOnline) {
                        allSatisfied = false;
                        break;
                    }
                }
            }
        }

        if (allSatisfied) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        uint32_t remaining = static_cast<uint32_t>(timeoutMs - elapsed);
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_syncCv.wait_for(lock, std::chrono::milliseconds(std::min<uint32_t>(remaining, 50)));
    }

    return false;
}

SystemHealthReport SystemManager::CheckSystemHealth() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);

    SystemHealthReport report;
    report.coordinatorRunning = (m_coordinator && m_coordinator->IsRunning());
    report.totalConfiguredNodes = m_nodes.size();
    report.registeredNodesCount = 0;
    report.onlineNodesCount = 0;

    for (const auto& node : m_nodes) {
        if (!node) continue;

        NodeHealthStatus status;
        status.nodeId = node->GetIntersectionId();
        status.connected = node->IsConnected();
        status.registered = node->IsRegistered();
        status.heartbeatActive = node->GetHeartbeatManager().IsRunning();
        status.signalState = node->GetSignalController().CurrentState();

        if (m_coordinator && report.coordinatorRunning) {
            coordinator::IntersectionEntry entry;
            if (m_coordinator->GetIntersectionRegistry().GetIntersection(status.nodeId, entry)) {
                status.coordinatorOnline = entry.isOnline;
                status.lastHeartbeatTimestampMs = entry.lastHeartbeatTimestamp;
            }
        }

        if (status.registered) {
            report.registeredNodesCount++;
        }
        if (status.coordinatorOnline) {
            report.onlineNodesCount++;
        }

        report.nodeStatuses.push_back(std::move(status));
    }

    report.isFullyHealthy = (report.coordinatorRunning &&
                             report.totalConfiguredNodes > 0 &&
                             report.onlineNodesCount == report.totalConfiguredNodes &&
                             report.registeredNodesCount == report.totalConfiguredNodes);

    return report;
}

bool SystemManager::IsSystemHealthy() const {
    return CheckSystemHealth().isFullyHealthy;
}

bool SystemManager::CreateCorridorRoute(const std::string& corridorId,
                                        const std::vector<std::string>& route) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator || !m_coordinator->IsRunning()) {
        return false;
    }
    return m_coordinator->CreateCorridor(corridorId, route);
}

bool SystemManager::ActivateCorridor() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator || !m_coordinator->IsRunning()) {
        return false;
    }
    if (!m_coordinator->ActivateCorridor()) {
        return false;
    }
    DispatchGeneratedCommands();
    return true;
}

bool SystemManager::AdvanceCorridor() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator || !m_coordinator->IsRunning()) {
        return false;
    }
    if (!m_coordinator->AdvanceCorridor()) {
        return false;
    }
    DispatchGeneratedCommands();
    return true;
}

bool SystemManager::CompleteCorridor() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator || !m_coordinator->IsRunning()) {
        return false;
    }
    if (!m_coordinator->CompleteCorridor()) {
        return false;
    }
    DispatchGeneratedCommands();
    return true;
}

bool SystemManager::CancelCorridor() {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_coordinator || !m_coordinator->IsRunning()) {
        return false;
    }
    if (!m_coordinator->CancelCorridor()) {
        return false;
    }
    DispatchGeneratedCommands();
    return true;
}

size_t SystemManager::DispatchGeneratedCommands() {
    if (!m_coordinator) {
        return 0;
    }

    auto commands = m_coordinator->PopGeneratedCommands();
    size_t dispatched = 0;

    for (const auto& cmd : commands) {
        std::string action, target;
        if (coordinator::TrafficCommandManager::ParseCommand(cmd, action, target)) {
            if (m_coordinator->SendMessageToIntersection(target, cmd)) {
                dispatched++;
            }
        }
    }
    return dispatched;
}

bool SystemManager::VerifyCorridorPhasing(const std::string& expectedGreenNodeId,
                                         const std::vector<std::string>& expectedRedNodeIds,
                                         uint32_t timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        bool match = true;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);

            auto* greenNode = GetIntersectionNode(expectedGreenNodeId);
            if (!greenNode || greenNode->GetSignalController().CurrentState() != intersection::SignalState::GREEN) {
                match = false;
            }

            if (match) {
                for (const auto& redId : expectedRedNodeIds) {
                    auto* redNode = GetIntersectionNode(redId);
                    if (!redNode || redNode->GetSignalController().CurrentState() != intersection::SignalState::RED) {
                        match = false;
                        break;
                    }
                }
            }
        }

        if (match) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        uint32_t remaining = static_cast<uint32_t>(timeoutMs - elapsed);
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_syncCv.wait_for(lock, std::chrono::milliseconds(std::min<uint32_t>(remaining, 20)));
    }

    return false;
}

bool SystemManager::VerifyAllNodesNormal(uint32_t timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        bool match = true;
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            for (const auto& node : m_nodes) {
                if (!node || node->GetSignalController().CurrentState() != intersection::SignalState::NORMAL) {
                    match = false;
                    break;
                }
            }
        }

        if (match) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        uint32_t remaining = static_cast<uint32_t>(timeoutMs - elapsed);
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_syncCv.wait_for(lock, std::chrono::milliseconds(std::min<uint32_t>(remaining, 20)));
    }

    return false;
}

bool SystemManager::DisconnectNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    auto* node = GetIntersectionNode(nodeId);
    if (!node) {
        return false;
    }
    node->Disconnect();
    return true;
}

bool SystemManager::ReconnectNode(const std::string& nodeId, uint32_t timeoutMs) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    auto* node = GetIntersectionNode(nodeId);
    if (!node) {
        return false;
    }
    return node->Reconnect(timeoutMs);
}

bool SystemManager::WaitForNodeOnline(const std::string& nodeId, uint32_t timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_coordinator && m_coordinator->IsRunning()) {
                coordinator::IntersectionEntry entry;
                if (m_coordinator->GetIntersectionRegistry().GetIntersection(nodeId, entry) &&
                    entry.isOnline) {
                    return true;
                }
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        uint32_t remaining = static_cast<uint32_t>(timeoutMs - elapsed);
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_syncCv.wait_for(lock, std::chrono::milliseconds(std::min<uint32_t>(remaining, 30)));
    }

    return false;
}

bool SystemManager::WaitForNodeOffline(const std::string& nodeId, uint32_t timeoutMs) {
    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        {
            std::lock_guard<std::mutex> lock(m_stateMutex);
            if (m_coordinator && m_coordinator->IsRunning()) {
                coordinator::IntersectionEntry entry;
                if (!m_coordinator->GetIntersectionRegistry().GetIntersection(nodeId, entry) ||
                    !entry.isOnline) {
                    return true;
                }
            }
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();
        if (elapsed >= timeoutMs) {
            break;
        }

        uint32_t remaining = static_cast<uint32_t>(timeoutMs - elapsed);
        std::unique_lock<std::mutex> lock(m_syncMutex);
        m_syncCv.wait_for(lock, std::chrono::milliseconds(std::min<uint32_t>(remaining, 30)));
    }

    return false;
}

coordinator::CoordinatorManager& SystemManager::GetCoordinator() noexcept {
    return *m_coordinator;
}

const coordinator::CoordinatorManager& SystemManager::GetCoordinator() const noexcept {
    return *m_coordinator;
}

intersection::IntersectionNode* SystemManager::GetNodeA() {
    return GetIntersectionNode(m_config.nodeIds.empty() ? "INT_A" : m_config.nodeIds[0]);
}

intersection::IntersectionNode* SystemManager::GetNodeB() {
    return GetIntersectionNode(m_config.nodeIds.size() > 1 ? m_config.nodeIds[1] : "INT_B");
}

intersection::IntersectionNode* SystemManager::GetNodeC() {
    return GetIntersectionNode(m_config.nodeIds.size() > 2 ? m_config.nodeIds[2] : "INT_C");
}

intersection::IntersectionNode* SystemManager::GetIntersectionNode(const std::string& nodeId) {
    for (auto& node : m_nodes) {
        if (node && node->GetIntersectionId() == nodeId) {
            return node.get();
        }
    }
    return nullptr;
}

const intersection::IntersectionNode* SystemManager::GetIntersectionNode(const std::string& nodeId) const {
    for (const auto& node : m_nodes) {
        if (node && node->GetIntersectionId() == nodeId) {
            return node.get();
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<intersection::IntersectionNode>> SystemManager::GetAllNodes() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_nodes;
}

} // namespace system
} // namespace aegis
