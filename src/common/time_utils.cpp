#include "aegis/common/time_utils.hpp"
#include <iomanip>
#include <sstream>
#include <ctime>

namespace aegis {
namespace common {

uint64_t GetMonotonicMicroseconds() noexcept {
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now).count());
}

uint64_t GetUtcMilliseconds() noexcept {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

std::string FormatUtcIso8601(uint64_t epochMs) {
    std::time_t seconds = static_cast<std::time_t>(epochMs / 1000);
    uint32_t millis = static_cast<uint32_t>(epochMs % 1000);

    std::tm tmBuf{};
#if defined(_WIN32)
    gmtime_s(&tmBuf, &seconds);
#else
    gmtime_r(&seconds, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%dT%H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << millis << "Z";
    return oss.str();
}

Stopwatch::Stopwatch() {
    Reset();
}

void Stopwatch::Start() noexcept {
    m_startTime = std::chrono::steady_clock::now();
    m_isRunning = true;
}

void Stopwatch::Stop() noexcept {
    if (m_isRunning) {
        m_stopTime = std::chrono::steady_clock::now();
        m_isRunning = false;
    }
}

void Stopwatch::Reset() noexcept {
    m_startTime = std::chrono::steady_clock::time_point{};
    m_stopTime = std::chrono::steady_clock::time_point{};
    m_isRunning = false;
}

uint64_t Stopwatch::ElapsedMicroseconds() const noexcept {
    if (m_isRunning) {
        auto now = std::chrono::steady_clock::now();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count());
    }
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(m_stopTime - m_startTime).count());
}

uint64_t Stopwatch::ElapsedMilliseconds() const noexcept {
    return ElapsedMicroseconds() / 1000;
}

bool Stopwatch::IsRunning() const noexcept {
    return m_isRunning;
}

} // namespace common
} // namespace aegis
