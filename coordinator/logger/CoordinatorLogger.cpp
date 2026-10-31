/**
 * @file CoordinatorLogger.cpp
 * @brief Implementation of thread-safe structured logging for Coordinator Node.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <iomanip>
#include <sstream>

namespace aegis::coordinator {

CoordinatorLogger& CoordinatorLogger::Instance() {
    static CoordinatorLogger instance;
    return instance;
}

CoordinatorLogger::~CoordinatorLogger() {
    Shutdown();
}

void CoordinatorLogger::Initialize(const std::string& logFilePath, bool logToConsole, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(mutex_);
    logToConsole_ = logToConsole;
    minLevel_ = minLevel;

    if (!logFilePath.empty()) {
        fileStream_.open(logFilePath, std::ios::out | std::ios::app);
    }
    initialized_ = true;
}

void CoordinatorLogger::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
    initialized_ = false;
}

std::string CoordinatorLogger::LevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

void CoordinatorLogger::Log(LogLevel level, const std::string& tag, const std::string& message) {
    if (level < minLevel_) {
        return;
    }

    uint64_t nowMs = common::TimeUtils::GetCurrentTimestampMs();
    std::string timestampStr = common::TimeUtils::FormatTimestampIso(nowMs);
    std::string levelStr = LevelToString(level);

    std::ostringstream oss;
    oss << "[" << timestampStr << "] [" << levelStr << "] [" << tag << "] " << message;
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

void CoordinatorLogger::Debug(const std::string& tag, const std::string& message) {
    Log(LogLevel::DEBUG, tag, message);
}

void CoordinatorLogger::Info(const std::string& tag, const std::string& message) {
    Log(LogLevel::INFO, tag, message);
}

void CoordinatorLogger::Warn(const std::string& tag, const std::string& message) {
    Log(LogLevel::WARN, tag, message);
}

void CoordinatorLogger::Error(const std::string& tag, const std::string& message) {
    Log(LogLevel::ERROR, tag, message);
}

void CoordinatorLogger::LogConnect(int fd, const std::string& ip, uint16_t port) {
    std::ostringstream oss;
    oss << "[CONNECT] Client connected on fd: " << fd << " from " << ip << ":" << port;
    Info("COORD_NET", oss.str());
}

void CoordinatorLogger::LogRegister(const std::string& nodeId, const std::string& ip, uint16_t port, int fd) {
    std::ostringstream oss;
    oss << "[REGISTER] Node registered: " << nodeId << " from " << ip << ":" << port << " (fd: " << fd << ")";
    Info("COORD_NET", oss.str());
}

void CoordinatorLogger::LogDisconnect(const std::string& nodeId, int fd, const std::string& reason) {
    std::ostringstream oss;
    oss << "[DISCONNECT] Node: " << (nodeId.empty() ? "UNKNOWN" : nodeId)
        << " (fd: " << fd << ") disconnected. Reason: " << reason;
    Info("COORD_NET", oss.str());
}

void CoordinatorLogger::LogPacketRx(const std::string& nodeId, const protocol::Packet& packet) {
    std::ostringstream oss;
    oss << "[RX] Type: " << protocol::MessageTypeToString(packet.GetType())
        << ", Node: " << nodeId
        << ", PayloadLen: " << packet.GetPayloadSize()
        << ", Timestamp: " << packet.GetTimestamp();
    Info("COORD_NET", oss.str());
}

void CoordinatorLogger::LogPacketTx(const std::string& nodeId, const protocol::Packet& packet) {
    std::ostringstream oss;
    oss << "[TX] Type: " << protocol::MessageTypeToString(packet.GetType())
        << ", Node: " << nodeId
        << ", PayloadLen: " << packet.GetPayloadSize();
    Info("COORD_NET", oss.str());
}

void CoordinatorLogger::LogTimeout(const std::string& nodeId, uint64_t elapsedMs, uint64_t thresholdMs) {
    std::ostringstream oss;
    oss << "[TIMEOUT] Node: " << nodeId << " silent for " << elapsedMs
        << "ms (threshold: " << thresholdMs << "ms). Terminating session.";
    Warn("COORD_NET", oss.str());
}

} // namespace aegis::coordinator

