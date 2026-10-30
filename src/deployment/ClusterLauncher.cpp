/**
 * @file ClusterLauncher.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - 4-Node Cluster Launcher & Coordinator CLI Implementation.
 */

#include "ClusterLauncher.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

namespace aegis {
namespace deployment {

ClusterLauncher::ClusterLauncher() = default;

ClusterLauncher::~ClusterLauncher() {
    StopCluster();
}

bool ClusterLauncher::InitializeCluster(uint16_t coordinatorPort,
                                        const std::vector<std::string>& edgeNodeIds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordPort = coordinatorPort;
    m_edgeNodeIds = edgeNodeIds.empty() ? std::vector<std::string>{"INT_A", "INT_B", "INT_C"} : edgeNodeIds;

    // Configure Coordinator
    NodeConfiguration coordConfig = NodeConfiguration::DefaultCoordinator("COORDINATOR", m_coordPort);
    coordConfig.expectedNodes = m_edgeNodeIds;
    coordConfig.logDirectory = "logs/coordinator";

    m_coordinator = std::make_shared<DeploymentManager>();
    if (!m_coordinator->Initialize(coordConfig)) {
        return false;
    }

    // Configure Edge Nodes
    m_edgeNodes.clear();
    uint16_t baseEdgePort = static_cast<uint16_t>(m_coordPort + 1);

    for (size_t i = 0; i < m_edgeNodeIds.size(); ++i) {
        const std::string& id = m_edgeNodeIds[i];
        NodeConfiguration edgeConfig = NodeConfiguration::DefaultTrafficNode(id, "127.0.0.1", m_coordPort);
        edgeConfig.port = static_cast<uint16_t>(baseEdgePort + i);
        edgeConfig.logDirectory = "logs/" + id;

        auto edgeMgr = std::make_shared<DeploymentManager>();
        if (!edgeMgr->Initialize(edgeConfig)) {
            return false;
        }
        m_edgeNodes[id] = edgeMgr;
    }

    return true;
}

bool ClusterLauncher::StartCluster() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running) return true;

    if (!m_coordinator || !m_coordinator->Start()) {
        std::cerr << "[CLUSTER] Failed to start Central Coordinator" << std::endl;
        return false;
    }

    // Give Coordinator listener 50ms to open TCP backlog
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Boot edge traffic nodes
    for (auto& kv : m_edgeNodes) {
        if (!kv.second->Start()) {
            std::cerr << "[CLUSTER] Warning: Failed to boot Edge Node " << kv.first << std::endl;
        }
    }

    // Connect edge nodes with coordinator's heartbeat supervisor
    for (const auto& kv : m_edgeNodes) {
        NodeTelemetry t = HeartbeatSupervisor::GatherLocalTelemetry(kv.first, "RED");
        m_coordinator->GetHeartbeatSupervisor().RecordHeartbeat(kv.first, t);
    }

    m_running = true;
    return true;
}

void ClusterLauncher::StopCluster() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running) return;

    for (auto& kv : m_edgeNodes) {
        kv.second->Stop();
    }
    m_edgeNodes.clear();

    if (m_coordinator) {
        m_coordinator->Stop();
    }

    m_running = false;
}

bool ClusterLauncher::CanAcceptEmergencies() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_running || !m_coordinator) return false;
    return m_coordinator->CanAcceptEmergencies();
}

std::shared_ptr<DeploymentManager> ClusterLauncher::GetEdgeNode(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_edgeNodes.find(nodeId);
    if (it == m_edgeNodes.end()) return nullptr;
    return it->second;
}

std::string ClusterLauncher::ExecuteCliCommand(const std::string& commandLine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::istringstream iss(commandLine);
    std::string verb;
    iss >> verb;

    if (verb == "help" || verb == "?") {
        return "SMART AEGIS Cluster CLI Commands:\n"
               "  cluster status              Show overall cluster status and emergency gating\n"
               "  list nodes                  List all registered nodes and connection state\n"
               "  node health [node_id]       Display detailed telemetry for a node or all nodes\n"
               "  shutdown node <node_id>     Gracefully terminate a specific edge node\n"
               "  restart node <node_id>      Reboot a specific edge node\n"
               "  broadcast message <text>    Broadcast an administrative preemption message\n"
               "  simulate failure <node_id>  Simulate connection loss or watchdog fail-safe\n";
    }

    if (verb == "cluster") {
        std::string sub;
        iss >> sub;
        if (sub == "status") {
            std::ostringstream oss;
            oss << "Cluster Running     : " << (m_running ? "YES" : "NO") << "\n";
            oss << "Emergency Ready     : " << (m_coordinator && m_coordinator->CanAcceptEmergencies() ? "READY" : "WAITING_FOR_NODES") << "\n";
            oss << "Configured Nodes    : " << m_edgeNodeIds.size() << "\n";
            size_t onlineCount = 0;
            if (m_coordinator) {
                for (const auto& id : m_edgeNodeIds) {
                    if (m_coordinator->GetHeartbeatSupervisor().IsNodeOnline(id)) {
                        onlineCount++;
                    }
                }
            }
            oss << "Online Edge Nodes   : " << onlineCount << " / " << m_edgeNodeIds.size() << "\n";
            return oss.str();
        }
    }

    if (verb == "list") {
        std::string sub;
        iss >> sub;
        if (sub == "nodes") {
            if (!m_coordinator) return "Coordinator not initialized.\n";
            std::ostringstream oss;
            oss << std::left << std::setw(14) << "Node ID"
                << std::setw(14) << "Role"
                << std::setw(12) << "Status"
                << "Ready State\n";
            oss << "--------------------------------------------------------\n";
            oss << std::left << std::setw(14) << m_coordinator->GetConfiguration().nodeId
                << std::setw(14) << "COORDINATOR"
                << std::setw(12) << (m_coordinator->IsRunning() ? "ONLINE" : "OFFLINE")
                << (m_coordinator->IsReady() ? "READY" : "NOT_READY") << "\n";

            for (const auto& id : m_edgeNodeIds) {
                auto it = m_edgeNodes.find(id);
                bool online = m_coordinator->GetHeartbeatSupervisor().IsNodeOnline(id);
                bool ready = (it != m_edgeNodes.end() && it->second->IsReady());
                oss << std::left << std::setw(14) << id
                    << std::setw(14) << "TRAFFIC_NODE"
                    << std::setw(12) << (online ? "ONLINE" : "OFFLINE")
                    << (ready ? "READY" : "NOT_READY") << "\n";
            }
            return oss.str();
        }
    }

    if (verb == "node") {
        std::string sub;
        iss >> sub;
        if (sub == "health") {
            std::string targetNode;
            iss >> targetNode;
            if (m_coordinator) {
                if (targetNode.empty()) {
                    return m_coordinator->GetHeartbeatSupervisor().GenerateDashboardView();
                } else {
                    NodeTelemetry t;
                    if (m_coordinator->GetHeartbeatSupervisor().GetNodeTelemetry(targetNode, t)) {
                        std::ostringstream oss;
                        oss << "Node ID     : " << t.nodeId << "\n";
                        oss << "Status      : " << (t.isOnline ? "ONLINE" : "OFFLINE") << "\n";
                        oss << "Signal State: " << t.signalState << "\n";
                        oss << "CPU Usage   : " << t.cpuUsagePercent << " %\n";
                        oss << "Memory Usage: " << t.memoryUsagePercent << " %\n";
                        oss << "Temperature : " << t.temperatureCelsius << " C\n";
                        oss << "Health Score: " << t.nodeHealth << " / 100\n";
                        oss << "GPIO Status : " << t.gpioStatus << "\n";
                        return oss.str();
                    } else {
                        return "Node " + targetNode + " not found or has not sent telemetry.\n";
                    }
                }
            }
            return "Coordinator supervisor unavailable.\n";
        }
    }

    if (verb == "shutdown") {
        std::string sub, targetNode;
        iss >> sub >> targetNode;
        if (sub == "node" && !targetNode.empty()) {
            auto it = m_edgeNodes.find(targetNode);
            if (it != m_edgeNodes.end()) {
                it->second->Stop();
                if (m_coordinator) {
                    m_coordinator->GetHeartbeatSupervisor().SetNodeOffline(targetNode);
                }
                return "Node " + targetNode + " shut down successfully.\n";
            }
            return "Node " + targetNode + " not found.\n";
        }
    }

    if (verb == "restart") {
        std::string sub, targetNode;
        iss >> sub >> targetNode;
        if (sub == "node" && !targetNode.empty()) {
            auto it = m_edgeNodes.find(targetNode);
            if (it != m_edgeNodes.end()) {
                NodeConfiguration cfg = it->second->GetConfiguration();
                it->second->Stop();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                auto newMgr = std::make_shared<DeploymentManager>();
                if (newMgr->Initialize(cfg) && newMgr->Start()) {
                    it->second = newMgr;
                    if (m_coordinator) {
                        m_coordinator->RecoverNode(targetNode);
                    }
                    return "Node " + targetNode + " restarted successfully.\n";
                }
            }
            return "Node " + targetNode + " not found.\n";
        }
    }

    if (verb == "broadcast") {
        std::string sub, message;
        iss >> sub;
        std::getline(iss, message);
        if (sub == "message" && !message.empty()) {
            if (m_coordinator && m_coordinator->GetCoordinatorRuntime()) {
                aegis::network::Message msg = aegis::network::Message::CreateCommand(
                    "COORDINATOR", "ADMIN_BROADCAST: " + message);
                size_t count = m_coordinator->GetCoordinatorRuntime()->GetTcpServer().Broadcast(msg);
                return "Broadcast delivered to " + std::to_string(count) + " edge nodes.\n";
            }
            return "Coordinator server not available for broadcast.\n";
        }
    }

    if (verb == "simulate") {
        std::string sub, targetNode;
        iss >> sub >> targetNode;
        if (sub == "failure" && !targetNode.empty()) {
            auto it = m_edgeNodes.find(targetNode);
            if (it != m_edgeNodes.end()) {
                it->second->SimulateFailure(targetNode);
                if (m_coordinator) {
                    m_coordinator->SimulateFailure(targetNode);
                }
                return "Simulated failure injected on " + targetNode + " (Flashing Yellow triggered).\n";
            }
            return "Node " + targetNode + " not found.\n";
        }
    }

    return "Unknown command: '" + commandLine + "'. Type 'help' for supported commands.\n";
}

} // namespace deployment
} // namespace aegis