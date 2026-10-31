/**
 * @file TrafficCommandDispatcher.hpp
 * @brief Command router and packet validation engine for Traffic Signal Node.
 * Directs validated wire packets to SignalController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include "traffic_node/signal_controller/SignalController.hpp"
#include <string>
#include <memory>
#include <functional>
#include <atomic>

namespace aegis::traffic {

using DispatchResponseCallback = std::function<bool(const protocol::Packet& packet)>;
using AckNotificationCallback = std::function<void(const protocol::Packet& packet)>;

class TrafficCommandDispatcher {
public:
    TrafficCommandDispatcher(std::string nodeId,
                             std::shared_ptr<SignalController> signalController,
                             DispatchResponseCallback sendResponseCb = nullptr);
    // Backward-compatible constructor accepting TrafficSignalFSM directly
    TrafficCommandDispatcher(std::string nodeId,
                             std::shared_ptr<TrafficSignalFSM> fsm,
                             DispatchResponseCallback sendResponseCb = nullptr);
    ~TrafficCommandDispatcher() = default;

    /**
     * @brief Validate message type and dispatch command to SignalController.
     * @param packet Validated incoming protocol packet.
     * @return true if successfully processed; false if unknown or rejected.
     */
    bool DispatchPacket(const protocol::Packet& packet);

    void SetResponseCallback(DispatchResponseCallback cb) {
        sendResponseCb_ = std::move(cb);
    }

    void SetAckCallback(AckNotificationCallback cb) {
        ackCallback_ = std::move(cb);
    }

    uint64_t GetProcessedCount() const { return processedCount_.load(); }
    uint64_t GetRejectedCount() const { return rejectedCount_.load(); }
    std::shared_ptr<SignalController> GetSignalController() const { return signalController_; }

private:
    std::string nodeId_;
    std::shared_ptr<SignalController> signalController_;
    DispatchResponseCallback sendResponseCb_;
    AckNotificationCallback ackCallback_;

    std::atomic<uint64_t> processedCount_{0};
    std::atomic<uint64_t> rejectedCount_{0};
};

} // namespace aegis::traffic
