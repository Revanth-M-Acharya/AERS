/**
 * @file ClientSession.hpp
 * @brief Represents an active Traffic Signal Node TCP session on the Coordinator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/network/TcpSocket.hpp"
#include "common/protocol/Packet.hpp"
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>

namespace aegis::coordinator {

class ClientSession {
public:
    ClientSession(int socketFd, const std::string& ip, uint16_t port);
    ~ClientSession();

    // Prevent copying
    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    // Node identification & state
    int GetFd() const { return socket_.GetFd(); }
    const std::string& GetIp() const { return ip_; }
    uint16_t GetPort() const { return port_; }
    std::string GetNodeId() const;
    void SetNodeId(const std::string& nodeId);

    bool IsRegistered() const { return isRegistered_.load(); }
    void SetRegistered(bool reg) { isRegistered_.store(reg); }

    uint64_t GetConnectedAtMs() const { return connectedAtMs_; }
    uint64_t GetLastHeartbeatMs() const { return lastHeartbeatMs_.load(); }
    void UpdateHeartbeat();
    bool IsTimedOut(uint64_t timeoutMs) const;

    // Socket I/O
    ssize_t ReadAvailable();
    bool ExtractPackets(std::vector<protocol::Packet>& outPackets);
    bool SendPacket(const protocol::Packet& packet);
    void Close();

private:
    network::TcpSocket socket_;
    std::string ip_;
    uint16_t port_;

    mutable std::mutex nodeMutex_;
    std::string nodeId_;

    std::atomic<bool> isRegistered_{false};
    uint64_t connectedAtMs_{0};
    std::atomic<uint64_t> lastHeartbeatMs_{0};

    std::mutex rxMutex_;
    std::vector<uint8_t> rxBuffer_;

    std::mutex sendMutex_;
};

} // namespace aegis::coordinator

