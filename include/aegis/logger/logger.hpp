#ifndef AEGIS_LOGGER_LOGGER_HPP
#define AEGIS_LOGGER_LOGGER_HPP

#include <string_view>
#include <cstdint>

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

/**
 * @brief Asynchronous structured logger supporting standard streams and QNX slogger2.
 * TODO: Implement non-blocking ring-buffer queue and slogger2 binding.
 */
class Logger {
public:
    static Logger& Instance();

    void Initialize(std::string_view logPath, LogLevel minLevel);
    void Log(LogLevel level, std::string_view moduleTag, std::string_view message);

private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace logger
} // namespace aegis

#endif // AEGIS_LOGGER_LOGGER_HPP
