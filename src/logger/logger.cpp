#include "aegis/logger/logger.hpp"
#include <iostream>

namespace aegis {
namespace logger {

Logger& Logger::Instance() {
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() = default;
Logger::~Logger() = default;

void Logger::Initialize(std::string_view /*logPath*/, LogLevel /*minLevel*/) {
    // TODO: Initialize QNX slogger2 channel or local file ring buffer
}

void Logger::Log(LogLevel /*level*/, std::string_view moduleTag, std::string_view message) {
    // TODO: Non-blocking asynchronous buffer write
    std::cout << "[" << moduleTag << "] " << message << std::endl;
}

} // namespace logger
} // namespace aegis
