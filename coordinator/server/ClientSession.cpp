/**
 * @file ClientSession.cpp
 * @brief Implementation of Traffic Signal Node client session.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/server/ClientSession.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include "common/network/SocketUtils.hpp"
#include <algorithm>

namespace aegis::coordinator {

ClientSession::ClientSession(int socketFd, const std::string& ip, uint16_t port)
    : socket_(socketFd),
      ip_(ip),
      port_(port),
      nodeId_(""),
      isRegistered_(false),
      connectedAtMs_(common::TimeUtils::GetCurrentTimestampMs()),
      lastHeartbeatMs_(common::TimeUtils::GetMonotonicTimeMs()) {
    // Configure client socket as non-blocking with TCP_NODELAY
    socket_.SetNonBlocking(true);
    socket_.SetNoDelay(true);
    rxBuffer_.reserve(4096);
}

ClientSession::~ClientSession() {
    Close();
}

std::string ClientSession::GetNodeId() const {
    std::lock_guard<std::mutex> lock(nodeMutex_);
    return nodeId_;
}

void ClientSession::SetNodeId(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(nodeMutex_);
    nodeId_ = nodeId;
}

void ClientSession::UpdateHeartbeat() {
    lastHeartbeatMs_.store(common::TimeUtils::GetMonotonicTimeMs());
}

bool ClientSession::IsTimedOut(uint64_t timeoutMs) const {
    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    uint64_t last = lastHeartbeatMs_.load();
    return (nowMono > last) && ((nowMono - last) > timeoutMs);
}

ssize_t ClientSession::ReadAvailable() {
    uint8_t tempBuf[2048];
    ssize_t bytesRead = socket_.Receive(tempBuf, sizeof(tempBuf));

    if (bytesRead > 0) {
        std::lock_guard<std::mutex> lock(rxMutex_);
        rxBuffer_.insert(rxBuffer_.end(), tempBuf, tempBuf + bytesRead);
        return bytesRead;
    } else if (bytesRead == 0) {
        // Socket closed cleanly by remote peer
        return 0;
    } else {
        // Check if would block
        if (network::SocketUtils::IsWouldBlock(errno)) {
            return -2; // Handled non-blocking indicator
        }
        return -1; // Socket error
    }
}

bool ClientSession::ExtractPackets(std::vector<protocol::Packet>& outPackets) {
    std::lock_guard<std::mutex> lock(rxMutex_);
    bool extractedAny = false;

    while (!rxBuffer_.empty()) {
        protocol::Packet pkt;
        size_t consumed = 0;
        bool success = protocol::Packet::Deserialize(rxBuffer_.data(), rxBuffer_.size(), pkt, consumed);

        if (success) {
            outPackets.push_back(pkt);
            rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + static_cast<std::ptrdiff_t>(consumed));
            extractedAny = true;
        } else {
            if (consumed > 0) {
                // Leading noise or corrupted packet skipped
                rxBuffer_.erase(rxBuffer_.begin(), rxBuffer_.begin() + static_cast<std::ptrdiff_t>(consumed));
            } else {
                // Incomplete packet in buffer; wait for next read
                break;
            }
        }
    }

    return extractedAny;
}

bool ClientSession::SendPacket(const protocol::Packet& packet) {
    std::vector<uint8_t> wireData = packet.Serialize();
    std::lock_guard<std::mutex> lock(sendMutex_);
    bool ok = socket_.SendAll(wireData);
    if (ok) {
        std::string currentId = GetNodeId();
        CoordinatorLogger::Instance().LogPacketTx(currentId.empty() ? ip_ : currentId, packet);
    }
    return ok;
}

void ClientSession::Close() {
    socket_.Close();
}

} // namespace aegis::coordinator

