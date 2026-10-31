/**
 * @file CoordinatorLogger.hpp
 * @brief Thread-safe structured logging facility for SMART AEGIS Coordinator Node.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "common/protocol/Packet.hpp"
#include <string>
#include <mutex>
#include <fstream>
#include <iostream>

namespace aegis::coordinator {

enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class CoordinatorLogger {
public:
    static CoordinatorLogger& Instance();

    void Initialize(const std::string& logFilePath, bool logToConsole = true, LogLevel minLevel = LogLevel::INFO);
    void Shutdown();

    // General logging
    void Log(LogLevel level, const std::string& tag, const std::string& message);
    void Debug(const std::string& tag, const std::string& message);
    void Info(const std::string& tag, const std::string& message);
    void Warn(const std::string& tag, const std::string& message);
    void Error(const std::string& tag, const std::string& message);

    // Specific network connection logging
    void LogConnect(int fd, const std::string& ip, uint16_t port);
    void LogRegister(const std::string& nodeId, const std::string& ip, uint16_t port, int fd);
    void LogDisconnect(const std::string& nodeId, int fd, const std::string& reason);
    void LogPacketRx(const std::string& nodeId, const protocol::Packet& packet);
    void LogPacketTx(const std::string& nodeId, const protocol::Packet& packet);
    void LogTimeout(const std::string& nodeId, uint64_t elapsedMs, uint64_t thresholdMs);

private:
    CoordinatorLogger() = default;
    ~CoordinatorLogger();
    CoordinatorLogger(const CoordinatorLogger&) = delete;
    CoordinatorLogger& operator=(const CoordinatorLogger&) = delete;

    std::string LevelToString(LogLevel level) const;

    std::mutex mutex_;
    std::ofstream fileStream_;
    bool logToConsole_{true};
    LogLevel minLevel_{LogLevel::INFO};
    bool initialized_{false};
};

} // namespace aegis::coordinator

