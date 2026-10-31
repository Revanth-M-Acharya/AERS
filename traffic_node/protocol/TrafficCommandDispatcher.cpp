/**
 * @file TrafficCommandDispatcher.cpp
 * @brief Implementation of TrafficCommandDispatcher.
 * Directs validated wire packets to SignalController.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/protocol/TrafficCommandDispatcher.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"

namespace aegis::traffic {

TrafficCommandDispatcher::TrafficCommandDispatcher(std::string nodeId,
                                                   std::shared_ptr<SignalController> signalController,
                                                   DispatchResponseCallback sendResponseCb)
    : nodeId_(std::move(nodeId)),
      signalController_(std::move(signalController)),
      sendResponseCb_(std::move(sendResponseCb)) {}

TrafficCommandDispatcher::TrafficCommandDispatcher(std::string nodeId,
                                                   std::shared_ptr<TrafficSignalFSM> fsm,
                                                   DispatchResponseCallback sendResponseCb)
    : nodeId_(std::move(nodeId)),
      signalController_(std::make_shared<SignalController>(std::move(fsm))),
      sendResponseCb_(std::move(sendResponseCb)) {}

bool TrafficCommandDispatcher::DispatchPacket(const protocol::Packet& packet) {
    protocol::MessageType type = packet.GetType();

    // Reject unknown packets safely
    if (type == protocol::MessageType::UNKNOWN) {
        ++rejectedCount_;
        TrafficLogger::Instance().Error("DISPATCHER", "Received UNKNOWN packet type from " + packet.GetNodeId() + ". Rejecting safely.");
        return false;
    }

    TrafficLogger::Instance().LogPacketRx(packet);

    bool handled = false;

    switch (type) {
        case protocol::MessageType::GREEN:
            if (signalController_) {
                handled = signalController_->CommandGreen("Coordinator GREEN Command");
            }
            break;

        case protocol::MessageType::YELLOW:
            if (signalController_) {
                handled = signalController_->CommandYellow("Coordinator YELLOW Command");
            }
            break;

        case protocol::MessageType::RED:
            if (signalController_) {
                handled = signalController_->CommandRed("Coordinator RED Command");
            }
            break;

        case protocol::MessageType::ALL_RED:
            if (signalController_) {
                handled = signalController_->CommandAllRed("Coordinator ALL_RED Command");
            }
            break;

        case protocol::MessageType::NORMAL_MODE:
            if (signalController_) {
                signalController_->CommandNormalMode("Coordinator NORMAL_MODE Command");
                handled = true;
            }
            break;

        case protocol::MessageType::EMERGENCY_MODE:
            if (signalController_) {
                std::string corrId = packet.GetPayload().empty() ? "COORDINATOR_DISPATCH" : packet.GetPayload();
                signalController_->CommandEmergencyMode(corrId);
                handled = true;
            }
            break;

        case protocol::MessageType::PING:
            if (sendResponseCb_) {
                std::string stateStr = signalController_ ? SignalStateToString(signalController_->GetCurrentState()) : "IDLE";
                protocol::Packet statusPkt(protocol::MessageType::STATUS, nodeId_, "ONLINE: " + stateStr);
                sendResponseCb_(statusPkt);
                handled = true;
            }
            break;

        case protocol::MessageType::ACK:
            if (ackCallback_) {
                ackCallback_(packet);
            }
            handled = true;
            break;

        default:
            TrafficLogger::Instance().Warn("DISPATCHER", "Unhandled packet type: " +
                                          protocol::MessageTypeToString(type));
            ++rejectedCount_;
            return false;
    }

    if (handled) {
        ++processedCount_;
    } else {
        ++rejectedCount_;
    }

    return handled;
}

} // namespace aegis::traffic
