/**
 * @file HeartbeatManager.cpp
 * @brief Implementation of autonomous heartbeat engine with diagnostic telemetry.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/client/HeartbeatManager.hpp"
#include "traffic_node/logger/TrafficLogger.hpp"
#include "common/utils/ThreadUtils.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::traffic {

HeartbeatManager::HeartbeatManager(std::string nodeId, uint64_t intervalMs, uint64_t timeoutMs,
                                   HeartbeatSendCallback sendCb, HeartbeatTimeoutCallback timeoutCb,
                                   HeartbeatPayloadGenerator payloadGen)
    : nodeId_(std::move(nodeId)),
      intervalMs_(intervalMs),
      timeoutMs_(timeoutMs),
      sendCallback_(std::move(sendCb)),
      timeoutCallback_(std::move(timeoutCb)),
      payloadGenerator_(std::move(payloadGen)),
      bootTimeMonoMs_(common::TimeUtils::GetMonotonicTimeMs()) {}

HeartbeatManager::~HeartbeatManager() {
    Stop();
}

void HeartbeatManager::Start() {
    if (isRunning_.load()) {
        return;
    }
    Reset();
    isRunning_.store(true);
    workerThread_ = std::thread(&HeartbeatManager::HeartbeatLoop, this);
    TrafficLogger::Instance().Info("HEARTBEAT", "Heartbeat Manager started (Interval: " +
                                  std::to_string(intervalMs_) + "ms, Timeout: " +
                                  std::to_string(timeoutMs_) + "ms).");
}

void HeartbeatManager::Stop() {
    if (!isRunning_.exchange(false)) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(cvMutex_);
        cv_.notify_all();
    }

    if (workerThread_.joinable()) {
        workerThread_.join();
    }
    TrafficLogger::Instance().Info("HEARTBEAT", "Heartbeat Manager stopped.");
}

void HeartbeatManager::Reset() {
    lastAckMonotonicMs_.store(common::TimeUtils::GetMonotonicTimeMs());
    heartbeatCounter_.store(0);
}

void HeartbeatManager::OnHeartbeatAckReceived() {
    lastAckMonotonicMs_.store(common::TimeUtils::GetMonotonicTimeMs());
    TrafficLogger::Instance().LogHeartbeatAckReceived();
}

std::string HeartbeatManager::GenerateDefaultTelemetry() const {
    uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
    uint64_t uptimeSec = (nowMono > bootTimeMonoMs_) ? (nowMono - bootTimeMonoMs_) / 1000 : 0;
    uint64_t timestamp = common::TimeUtils::GetCurrentTimestampMs();

    std::ostringstream oss;
    oss << "NODE:" << nodeId_
        << "|STATE:ALL_RED"
        << "|RELAY:1001"
        << "|CPU:1.5%"
        << "|MEM:42.5MB"
        << "|TEMP:41.0C"
        << "|UPTIME:" << uptimeSec << "s"
        << "|TIME:" << timestamp;
    return oss.str();
}

void HeartbeatManager::HeartbeatLoop() {
    common::ThreadUtils::SetCurrentThreadName("TrafficHBeat");

    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(cvMutex_);
        cv_.wait_for(lock, std::chrono::milliseconds(intervalMs_), [this]() {
            return !isRunning_.load();
        });

        if (!isRunning_.load()) {
            break;
        }

        if (petCallback_) {
            petCallback_();
        }

        uint64_t nowMono = common::TimeUtils::GetMonotonicTimeMs();
        uint64_t lastAck = lastAckMonotonicMs_.load();

        // Check if Coordinator heartbeat ACK timeout exceeded
        if (lastAck > 0 && (nowMono > lastAck) && ((nowMono - lastAck) > timeoutMs_)) {
            TrafficLogger::Instance().Warn("HEARTBEAT", "Heartbeat ACK timeout (" +
                                          std::to_string(nowMono - lastAck) + "ms > " +
                                          std::to_string(timeoutMs_) + "ms). Triggering reconnection.");
            if (timeoutCallback_) {
                timeoutCallback_();
            }
            break;
        }

        // Generate enriched diagnostic payload
        std::string payload;
        if (payloadGenerator_) {
            payload = payloadGenerator_();
        } else {
            payload = GenerateDefaultTelemetry();
        }

        ++heartbeatCounter_;
        protocol::Packet hbPkt(protocol::MessageType::HEARTBEAT, nodeId_, payload);
        if (sendCallback_) {
            if (sendCallback_(hbPkt)) {
                TrafficLogger::Instance().LogHeartbeatSent(hbPkt.GetTimestamp());
            }
        }
    }
}

} // namespace aegis::traffic
