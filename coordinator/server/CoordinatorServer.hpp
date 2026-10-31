/**
 * @file CoordinatorServer.hpp
 * @brief Production-grade Multi-Client Coordinator TCP Server for QNX SDP 8.0 Raspberry Pi 4.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/config/ServerConfig.hpp"
#include "common/network/TcpSocket.hpp"
#include "common/protocol/Packet.hpp"
#include "coordinator/server/ClientSession.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <functional>

namespace aegis::coordinator {

// Callback signatures
using PacketCallback = std::function<void(const std::string& nodeId, const protocol::Packet& packet)>;
using ConnectCallback = std::function<void(const std::string& nodeId, const std::string& ip, uint16_t port)>;
using DisconnectCallback = std::function<void(const std::string& nodeId, const std::string& reason)>;

class CoordinatorServer {
public:
    CoordinatorServer();
    explicit CoordinatorServer(const config::ServerConfig& config);
    ~CoordinatorServer();

    // Prevent copying
    CoordinatorServer(const CoordinatorServer&) = delete;
    CoordinatorServer& operator=(const CoordinatorServer&) = delete;

    /**
     * @brief Configure the server parameters.
     */
    void SetConfig(const config::ServerConfig& config);
    const config::ServerConfig& GetConfig() const { return config_; }

    /**
     * @brief Start the Coordinator TCP Server (binds, listens, spawns worker threads).
     * @return true on successful start, false on failure.
     */
    bool Start();

    /**
     * @brief Stop the server gracefully, closing all sockets and joining threads.
     */
    void Stop();

    /**
     * @brief Check if server is running.
     */
    bool IsRunning() const { return isRunning_.load(); }

    /**
     * @brief Send a packet to a specific registered Traffic Node.
     * @param nodeId Unique node identifier (e.g. "INT_A", "INT_B").
     * @param packet Packet to transmit.
     * @return true if transmitted successfully, false if node not found or send error.
     */
    bool SendToNode(const std::string& nodeId, const protocol::Packet& packet);

    /**
     * @brief Broadcast a packet to all currently registered Traffic Nodes.
     * @param packet Packet to broadcast.
     * @return Number of nodes to which the packet was sent successfully.
     */
    size_t Broadcast(const protocol::Packet& packet);

    /**
     * @brief Manually disconnect a specific node.
     */
    void DisconnectNode(const std::string& nodeId, const std::string& reason = "Manual disconnect");

    /**
     * @brief Get list of registered node identifiers.
     */
    std::vector<std::string> GetRegisteredNodeIds() const;

    /**
     * @brief Get count of registered nodes.
     */
    size_t GetRegisteredNodeCount() const;

    /**
     * @brief Check whether a specific node is currently registered and connected.
     */
    bool IsNodeRegistered(const std::string& nodeId) const;

    // Callbacks for future protocol extensions and corridor logic
    void SetPacketCallback(PacketCallback cb) { packetCallback_ = std::move(cb); }
    void SetConnectCallback(ConnectCallback cb) { connectCallback_ = std::move(cb); }
    void SetDisconnectCallback(DisconnectCallback cb) { disconnectCallback_ = std::move(cb); }

private:
    void AcceptLoop();
    void IoWorkerLoop();

    void HandleIncomingPacket(const std::shared_ptr<ClientSession>& session, const protocol::Packet& packet);
    void RemoveSession(int fd, const std::string& reason);
    void CheckHeartbeatTimeouts();

    config::ServerConfig config_;
    network::TcpSocket listeningSocket_;
    std::atomic<bool> isRunning_{false};

    // Threading
    std::thread acceptThread_;
    std::thread ioWorkerThread_;

    // Client Registries
    mutable std::mutex sessionMutex_;
    // Map fd -> ClientSession (all active sockets)
    std::unordered_map<int, std::shared_ptr<ClientSession>> sessionsByFd_;
    // Map nodeId -> ClientSession (registered nodes only)
    std::unordered_map<std::string, std::shared_ptr<ClientSession>> registeredNodes_;

    // Extensibility Callbacks
    PacketCallback packetCallback_;
    ConnectCallback connectCallback_;
    DisconnectCallback disconnectCallback_;
};

} // namespace aegis::coordinator

