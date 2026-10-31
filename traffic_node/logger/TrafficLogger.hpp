/**
 * @file TrafficLogger.hpp
 * @brief Thread-safe structured logging facility for SMART AEGIS Traffic Signal Node.
 * Enforces Timestamp, Thread, Node ID, Severity, and Subsystem Categorization.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

namespace aegis::traffic {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class TrafficLogger {
public:
    static TrafficLogger& Instance();

    void Initialize(const std::string& nodeId, const std::string& logFilePath,
                    bool logToConsole = true, LogLevel minLevel = LogLevel::INFO);
    void Shutdown();

    // Standard Logging
    void Log(LogLevel level, const std::string& subsystem, const std::string& message);
    void Debug(const std::string& subsystem, const std::string& message);
    void Info(const std::string& subsystem, const std::string& message);
    void Warn(const std::string& subsystem, const std::string& message);
    void Error(const std::string& subsystem, const std::string& message);

    // Specialized Telemetry & State Logging
    void LogConnect(const std::string& ip, uint16_t port);
    void LogDisconnect(const std::string& reason);
    void LogReconnecting(uint32_t attempt, uint64_t delayMs);
    void LogHeartbeatSent(uint64_t timestamp);
    void LogHeartbeatAckReceived();
    void LogPacketRx(const protocol::Packet& packet);
    void LogPacketTx(const protocol::Packet& packet);
    void LogFsmTransition(const std::string& fromState, const std::string& toState, const std::string& reason);
    void LogIllegalTransition(const std::string& fromState, const std::string& attemptedState);
    void LogRelayChange(bool red, bool yellow, bool green, bool aux);
    void LogWatchdogCheckin(const std::string& threadName);

private:
    TrafficLogger() = default;
    ~TrafficLogger();
    TrafficLogger(const TrafficLogger&) = delete;
    TrafficLogger& operator=(const TrafficLogger&) = delete;

    std::string LevelToString(LogLevel level) const;
    std::string GetCurrentThreadName() const;

    std::mutex mutex_;
    std::string nodeId_{"TRAFFIC_NODE"};
    std::ofstream fileStream_;
    bool logToConsole_{true};
    LogLevel minLevel_{LogLevel::INFO};
    bool initialized_{false};
};

} // namespace aegis::traffic
