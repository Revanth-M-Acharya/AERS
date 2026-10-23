#ifndef AEGIS_LOGGER_LOGGER_HPP
#define AEGIS_LOGGER_LOGGER_HPP

#include <string_view>
#include "aegis/common/types.hpp"

namespace aegis {
namespace logger {

enum class LogLevel : uint8_t {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL
};

class Logger {
public:
    static Logger& Instance();

    void Initialize(std::string_view logPath, LogLevel minLevel);
    void Log(LogLevel level, std::string_view moduleTag, std::string_view message);

private:
    Logger() = default;
    ~Logger() = default;
};

} // namespace logger
} // namespace aegis

#endif // AEGIS_LOGGER_LOGGER_HPP
