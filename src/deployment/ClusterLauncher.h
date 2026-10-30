/**
 * @file ClusterLauncher.h
 * @brief SMART AEGIS (AERS) Phase 16D - 4-Node Cluster Launcher & Coordinator CLI.
 *
 * Orchestrates the full 4-Raspberry Pi cluster (1 Coordinator + 3 Intersections)
 * and provides interactive and programmatic command-line interface:
 * cluster status, list nodes, node health, shutdown node, restart node, broadcast message, simulate failure.
 */

#ifndef AEGIS_DEPLOYMENT_CLUSTER_LAUNCHER_H
#define AEGIS_DEPLOYMENT_CLUSTER_LAUNCHER_H

#include "DeploymentManager.h"
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <mutex>

namespace aegis {
namespace deployment {

class ClusterLauncher {
public:
    ClusterLauncher();
    ~ClusterLauncher();

    // Non-copyable, non-movable
    ClusterLauncher(const ClusterLauncher&) = delete;
    ClusterLauncher& operator=(const ClusterLauncher&) = delete;

    /**
     * @brief Initialize cluster configuration for 4 Raspberry Pi boards.
     * @param coordinatorPort TCP listening port for Coordinator (default 9000).
     * @param edgeNodeIds List of edge node IDs (default {"INT_A", "INT_B", "INT_C"}).
     * @return true on success; false on configuration failure.
     */
    bool InitializeCluster(uint16_t coordinatorPort = 9000,
                           const std::vector<std::string>& edgeNodeIds = {"INT_A", "INT_B", "INT_C"});

    /**
     * @brief Start Central Coordinator and all edge traffic nodes in deterministic order.
     * @return true if all nodes start and register successfully; false otherwise.
     */
    bool StartCluster();

    /**
     * @brief Gracefully terminate all cluster nodes and release hardware resources.
     */
    void StopCluster();

    /**
     * @brief Check whether cluster coordinator and edge nodes are running.
     */
    bool IsClusterRunning() const noexcept { return m_running; }

    /**
     * @brief Coordinator gating: verify whether every expected node is online.
     * Coordinator waits until every node is online before accepting emergencies.
     */
    bool CanAcceptEmergencies() const;

    /**
     * @brief Execute a Coordinator CLI command line string.
     * Supported commands:
     * - "cluster status"
     * - "list nodes"
     * - "node health [node_id]"
     * - "shutdown node <node_id>"
     * - "restart node <node_id>"
     * - "broadcast message <text>"
     * - "simulate failure <node_id>"
     * - "help"
     * @param commandLine String containing command and arguments.
     * @return Formatted command output string.
     */
    std::string ExecuteCliCommand(const std::string& commandLine);

    /**
     * @brief Access Coordinator DeploymentManager.
     */
    std::shared_ptr<DeploymentManager> GetCoordinator() const { return m_coordinator; }

    /**
     * @brief Access an Edge Node DeploymentManager by ID.
     */
    std::shared_ptr<DeploymentManager> GetEdgeNode(const std::string& nodeId) const;

    /**
     * @brief Get list of all configured edge node IDs.
     */
    const std::vector<std::string>& GetEdgeNodeIds() const noexcept { return m_edgeNodeIds; }

private:
    mutable std::mutex m_mutex;
    bool m_running{false};
    uint16_t m_coordPort{9000};
    std::vector<std::string> m_edgeNodeIds{};

    std::shared_ptr<DeploymentManager> m_coordinator{nullptr};
    std::map<std::string, std::shared_ptr<DeploymentManager>> m_edgeNodes{};
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_CLUSTER_LAUNCHER_H
