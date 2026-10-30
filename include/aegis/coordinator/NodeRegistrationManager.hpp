/**
 * @file NodeRegistrationManager.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Central Coordinator Node Registration Service.
 *
 * Tracks distributed intersection nodes, validates node identities, prevents duplicate
 * registrations, monitors heartbeat freshness, and manages automatic stale node pruning.
 */

#ifndef AEGIS_COORDINATOR_NODE_REGISTRATION_MANAGER_HPP
#define AEGIS_COORDINATOR_NODE_REGISTRATION_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>

namespace aegis {
namespace coordinator {

enum class NodeOnlineState : uint8_t {
    ONLINE = 0,
    DEGRADED,
    OFFLINE
};

inline std::string NodeOnlineStateToString(NodeOnlineState state) {
    switch (state) {
        case NodeOnlineState::ONLINE:   return "ONLINE";
        case NodeOnlineState::DEGRADED: return "DEGRADED";
        case NodeOnlineState::OFFLINE:  return "OFFLINE";
        default:                        return "UNKNOWN";
    }
}

/**
 * @brief Metadata and physical diagnostic record submitted by an intersection node.
 */
struct NodeRegistrationRecord {
    std::string nodeId;
    std::string hostname{"rpi4-node"};
    std::string ipAddress;
    std::string macAddress{"DC:A6:32:00:00:01"};
    std::string softwareVersion{"20.7.0"};
    std::string hardwareVersion{"Raspberry Pi 4 Model B"};
    std::string relayStatus{"4_CHANNELS_PASS"};
    std::string gpioStatus{"RESERVED_PINS_OK"};
    std::string currentStatus{"READY"};
    bool readyState{true};
    std::vector<std::string> capabilities;
    uint64_t bootTimestampUs{0};
    uint64_t registeredTimestampUs{0};
    uint64_t lastHeartbeatTimestampUs{0};
    NodeOnlineState onlineState{NodeOnlineState::ONLINE};
    bool inRecovery{false};

    // Convenience alias for heartbeat time
    uint64_t GetHeartbeatTimeUs() const noexcept { return lastHeartbeatTimestampUs; }
};

/**
 * @brief Central Coordinator registration authority managing intersection nodes across Wi-Fi.
 */
class NodeRegistrationManager {
public:
    using NodeAlertCallback = std::function<void(const std::string& nodeId, NodeOnlineState state, const std::string& reason)>;

    NodeRegistrationManager();
    ~NodeRegistrationManager() = default;

    // Non-copyable, non-movable
    NodeRegistrationManager(const NodeRegistrationManager&) = delete;
    NodeRegistrationManager& operator=(const NodeRegistrationManager&) = delete;

    /**
     * @brief Process and register an incoming intersection node registration payload.
     * @param record Registration data submitted by node.
     * @param outError Populated with error description if rejected.
     * @return true if accepted, false if rejected (e.g. duplicate ID from different IP).
     */
    bool RegisterNode(const NodeRegistrationRecord& record, std::string& outError);

    /**
     * @brief Explicitly unregister a node on shutdown.
     */
    bool UnregisterNode(const std::string& nodeId, const std::string& reason = "Normal shutdown");

    /**
     * @brief Update heartbeat timestamp for a registered node.
     */
    bool RecordHeartbeat(const std::string& nodeId);

    /**
     * @brief Evict or mark OFFLINE any nodes whose heartbeat has exceeded the timeout.
     * @param timeoutSeconds Inactivity threshold in seconds (default 5.0s).
     * @return Number of stale nodes marked offline/pruned.
     */
    size_t PruneStaleNodes(double timeoutSeconds = 5.0);

    /**
     * @brief Update node recovery state flag.
     */
    bool SetNodeRecoveryState(const std::string& nodeId, bool inRecovery);

    // Queries
    bool IsNodeRegistered(const std::string& nodeId) const;
    bool IsNodeOnline(const std::string& nodeId) const;
    bool GetNodeRecord(const std::string& nodeId, NodeRegistrationRecord& outRecord) const;
    std::vector<NodeRegistrationRecord> GetAllNodes() const;
    size_t GetOnlineNodeCount() const;
    size_t GetTotalNodeCount() const;

    void SetAlertCallback(NodeAlertCallback cb);
    std::string GenerateReport() const;
    void Reset();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, NodeRegistrationRecord> m_nodes;
    NodeAlertCallback m_callback{nullptr};
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_NODE_REGISTRATION_MANAGER_HPP

