/**
 * @file DistributedRuntimeOrchestrator.cpp
 * @brief SMART AEGIS (AERS) Phase 16C - Distributed Runtime Orchestrator Implementation.
 */

#include "DistributedRuntimeOrchestrator.h"
#include <iostream>
#include <chrono>

namespace aegis {
namespace runtime {

DistributedRuntimeOrchestrator::DistributedRuntimeOrchestrator()
    : m_coordinator()
{
}

DistributedRuntimeOrchestrator::~DistributedRuntimeOrchestrator() {
    StopCluster();
}

bool DistributedRuntimeOrchestrator::InitializeCluster(uint16_t coordinatorPort,
                                                      const std::vector<std::string>& nodeIds) {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    if (m_isInitialized.load()) {
        return true;
    }

    m_coordinatorPort = coordinatorPort;
    m_configuredNodeIds = nodeIds;

    // 1. Initialize Central Coordinator Runtime (Task 2)
    if (!m_coordinator.Initialize()) {
        std::cerr << "[ERROR] [ORCHESTRATOR] Coordinator runtime initialization failed!\n";
        return false;
    }

    // 2. Initialize Edge Traffic Node Runtimes (Task 3)
    m_trafficNodes.clear();
    for (const auto& nodeId : m_configuredNodeIds) {
        auto nodeRuntime = std::make_unique<TrafficNodeRuntime>(nodeId);
        if (!nodeRuntime->Initialize(nodeId)) {
            std::cerr << "[ERROR] [ORCHESTRATOR] Failed to initialize edge node: " << nodeId << "\n";
            return false;
        }
        m_trafficNodes[nodeId] = std::move(nodeRuntime);
    }

    m_isInitialized.store(true);
    return true;
}

bool DistributedRuntimeOrchestrator::StartCluster() {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    if (!m_isInitialized.load()) {
        InitializeCluster(m_coordinatorPort, m_configuredNodeIds);
    }
    if (m_isRunning.load()) {
        return true;
    }

    // Step 1: Start Central Coordinator Server (Startup Ordering - Task 7)
    if (!m_coordinator.Start(m_coordinatorHost, m_coordinatorPort)) {
        std::cerr << "[ERROR] [ORCHESTRATOR] Failed to start Coordinator server on port " << m_coordinatorPort << "\n";
        return false;
    }

    // Brief pause to ensure TCP listen socket is ready
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Step 2: Start Edge Traffic Node Clients sequentially
    for (const auto& nodeId : m_configuredNodeIds) {
        auto it = m_trafficNodes.find(nodeId);
        if (it != m_trafficNodes.end()) {
            it->second->Start(m_coordinatorHost, m_coordinatorPort);
        }
    }

    // Brief pause for registration handshakes
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    m_isRunning.store(true);
    return true;
}

void DistributedRuntimeOrchestrator::StopCluster() {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    if (!m_isRunning.load()) {
        return;
    }

    // Step 1: Stop Edge Traffic Node Clients (Orderly Shutdown - Task 7)
    for (auto& pair : m_trafficNodes) {
        if (pair.second) {
            pair.second->Stop();
        }
    }

    // Step 2: Stop Central Coordinator Server
    m_coordinator.Stop();

    m_isRunning.store(false);
    m_isInitialized.store(false);
}

bool DistributedRuntimeOrchestrator::IsClusterRunning() const noexcept {
    return m_isRunning.load();
}

EmergencyPipelineResult DistributedRuntimeOrchestrator::TriggerEmergency(const EmergencyPipelineInput& input) {
    if (!m_isRunning.load()) {
        EmergencyPipelineResult failureRes;
        failureRes.success = false;
        failureRes.failureReason = "Cluster is not running";
        return failureRes;
    }

    // Trigger full end-to-end pipeline through coordinator (Task 4)
    return m_coordinator.ExecuteEmergencyPipeline(input);
}

bool DistributedRuntimeOrchestrator::SimulateNodeDisconnect(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    auto it = m_trafficNodes.find(nodeId);
    if (it != m_trafficNodes.end() && it->second) {
        it->second->GetTcpClient().Disconnect();
        it->second->DegradeToFailSafe();
        return true;
    }
    return false;
}

bool DistributedRuntimeOrchestrator::ReconnectNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    auto it = m_trafficNodes.find(nodeId);
    if (it != m_trafficNodes.end() && it->second) {
        return it->second->GetTcpClient().Connect(m_coordinatorHost, m_coordinatorPort);
    }
    return false;
}

bool DistributedRuntimeOrchestrator::IsClusterHealthy() const {
    if (!m_isRunning.load() || !m_coordinator.IsRunning()) {
        return false;
    }
    for (const auto& pair : m_trafficNodes) {
        if (!pair.second || !pair.second->IsRunning()) {
            return false;
        }
    }
    return true;
}

TrafficNodeRuntime* DistributedRuntimeOrchestrator::GetTrafficNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_orchestratorMutex);
    auto it = m_trafficNodes.find(nodeId);
    if (it != m_trafficNodes.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<std::string> DistributedRuntimeOrchestrator::GetNodeIds() const {
    return m_configuredNodeIds;
}

} // namespace runtime
} // namespace aegis
