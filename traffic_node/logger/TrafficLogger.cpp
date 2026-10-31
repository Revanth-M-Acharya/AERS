/**
 * @file TrafficLogger.cpp
 * @brief Implementation of structured diagnostic logger for Traffic Signal Node.
 * Formats: [Timestamp] [Severity] [NodeID] [Thread:Name] [Subsystem] Message
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/logger/TrafficLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iomanip>
#include <sstream>
#include <thread>
#include <pthread.h>

namespace aegis::traffic {

TrafficLogger& TrafficLogger::Instance() {
    static TrafficLogger instance;
    return instance;
}

TrafficLogger::~TrafficLogger() {
    Shutdown();
}

void TrafficLogger::Initialize(const std::string& nodeId, const std::string& logFilePath,
                               bool logToConsole, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    nodeId_ = nodeId;
    logToConsole_ = logToConsole;
    minLevel_ = minLevel;

    if (!logFilePath.empty()) {
        fileStream_.open(logFilePath, std::ios::out | std::ios::app);
    }
    initialized_ = true;
}

void TrafficLogger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
    initialized_ = false;
}

std::string TrafficLogger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

std::string TrafficLogger::GetCurrentThreadName() const {
    char nameBuf[32]{};
#if defined(__APPLE__)
    pthread_getname_np(pthread_self(), nameBuf, sizeof(nameBuf));
#elif defined(__linux__) || defined(__QNX__) || defined(__QNXNTO__)
    pthread_getname_np(pthread_self(), nameBuf, sizeof(nameBuf));
#endif
    if (nameBuf[0] != '\0') {
        return nameBuf;
    }
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

void TrafficLogger::Log(LogLevel level, const std::string& subsystem, const std::string& message) {
    if (level < minLevel_) {
        return;
    }

    uint64_t nowMs = common::TimeUtils::GetCurrentTimestampMs();
    std::string timestampStr = common::TimeUtils::FormatTimestampIso(nowMs);
    std::string levelStr = LevelToString(level);
    std::string threadStr = GetCurrentThreadName();

    std::ostringstream oss;
    oss << "[" << timestampStr << "] [" << levelStr << "] [" << nodeId_
        << "] [Thread:" << threadStr << "] [" << subsystem << "] " << message;
    std::string line = oss.str();

    std::lock_guard<std::mutex> lock(mutex_);
    if (logToConsole_) {
        if (level == LogLevel::ERROR) {
            std::cerr << line << "\n";
        } else {
            std::cout << line << "\n";
        }
    }
    if (fileStream_.is_open()) {
        fileStream_ << line << "\n";
        fileStream_.flush();
    }
}

void TrafficLogger::Debug(const std::string& subsystem, const std::string& message) {
    Log(LogLevel::DEBUG, subsystem, message);
}

void TrafficLogger::Info(const std::string& subsystem, const std::string& message) {
    Log(LogLevel::INFO, subsystem, message);
}

void TrafficLogger::Warn(const std::string& subsystem, const std::string& message) {
    Log(LogLevel::WARN, subsystem, message);
}

void TrafficLogger::Error(const std::string& subsystem, const std::string& message) {
    Log(LogLevel::ERROR, subsystem, message);
}

void TrafficLogger::LogConnect(const std::string& ip, uint16_t port) {
    std::ostringstream oss;
    oss << "Connected to Coordinator TCP Server at " << ip << ":" << port;
    Info("NET", oss.str());
}

void TrafficLogger::LogDisconnect(const std::string& reason) {
    std::ostringstream oss;
    oss << "Connection to Coordinator lost: " << reason;
    Warn("NET", oss.str());
}

void TrafficLogger::LogReconnecting(uint32_t attempt, uint64_t delayMs) {
    std::ostringstream oss;
    oss << "Attempt #" << attempt << " in " << delayMs << "ms...";
    Info("NET", oss.str());
}

void TrafficLogger::LogHeartbeatSent(uint64_t timestamp) {
    std::ostringstream oss;
    oss << "Dispatched HEARTBEAT telemetry frame (Timestamp: " << timestamp << ")";
    Debug("HEARTBEAT", oss.str());
}

void TrafficLogger::LogHeartbeatAckReceived() {
    Debug("HEARTBEAT", "Received Coordinator heartbeat acknowledgment (HEARTBEAT_ACK)");
}

void TrafficLogger::LogPacketRx(const protocol::Packet& packet) {
    std::ostringstream oss;
    oss << "Type: " << protocol::MessageTypeToString(packet.GetType())
        << ", From: " << packet.GetNodeId()
        << ", PayloadLen: " << packet.GetPayloadSize();
    Info("NET", oss.str());
}

void TrafficLogger::LogPacketTx(const protocol::Packet& packet) {
    std::ostringstream oss;
    oss << "Type: " << protocol::MessageTypeToString(packet.GetType())
        << ", To: " << packet.GetNodeId()
        << ", PayloadLen: " << packet.GetPayloadSize();
    Info("NET", oss.str());
}

void TrafficLogger::LogFsmTransition(const std::string& fromState, const std::string& toState, const std::string& reason) {
    std::ostringstream oss;
    oss << "Transition: " << fromState << " -> " << toState << " [Reason: " << reason << "]";
    Info("FSM", oss.str());
}

void TrafficLogger::LogIllegalTransition(const std::string& fromState, const std::string& attemptedState) {
    std::ostringstream oss;
    oss << "ILLEGAL TRANSITION REJECTED: " << fromState << " -> " << attemptedState
        << " violates safety invariant (Direct GREEN->RED forbidden!)";
    Error("FSM", oss.str());
}

void TrafficLogger::LogRelayChange(bool red, bool yellow, bool green, bool aux) {
    std::ostringstream oss;
    oss << "Actuation -> RED:" << (red ? "ON" : "OFF")
        << " | YELLOW:" << (yellow ? "ON" : "OFF")
        << " | GREEN:" << (green ? "ON" : "OFF")
        << " | AUX:" << (aux ? "ON" : "OFF");
    Debug("RELAY", oss.str());
}

void TrafficLogger::LogWatchdogCheckin(const std::string& threadName) {
    Debug("WATCHDOG", "Thread pet received from " + threadName);
}

} // namespace aegis::traffic
