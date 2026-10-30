/**
 * @file DeploymentLogger.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Node Deployment Multi-File Logger Implementation.
 */

#include "DeploymentLogger.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>

namespace aegis {
namespace deployment {

namespace {
void EnsureDirectory(const std::string& path) {
    if (path.empty()) return;
    std::string current;
    for (size_t i = 0; i < path.size(); ++i) {
        current += path[i];
        if (path[i] == '/' || i == path.size() - 1) {
            ::mkdir(current.c_str(), 0755);
        }
    }
}
} // anonymous namespace

DeploymentLogger::DeploymentLogger() = default;

DeploymentLogger::~DeploymentLogger() {
    Close();
}

std::string DeploymentLogger::GetTimestampString() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuffer;
#if defined(_WIN32)
    localtime_s(&tmBuffer, &t);
#else
    localtime_r(&t, &tmBuffer);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuffer, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

bool DeploymentLogger::Initialize(const std::string& logDirectory, const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    Close();

    m_logDir = logDirectory.empty() ? "logs" : logDirectory;
    m_nodeId = nodeId.empty() ? "NODE" : nodeId;

    // Create directory recursively if not existing
    EnsureDirectory(m_logDir);

    m_runtimeFile.open(m_logDir + "/runtime.log", std::ios::out | std::ios::app);
    m_networkFile.open(m_logDir + "/network.log", std::ios::out | std::ios::app);
    m_gpioFile.open(m_logDir + "/gpio.log", std::ios::out | std::ios::app);
    m_diagnosticsFile.open(m_logDir + "/diagnostics.log", std::ios::out | std::ios::app);
    m_crashFile.open(m_logDir + "/crash.log", std::ios::out | std::ios::app);

    m_initialized = (m_runtimeFile.is_open() && m_networkFile.is_open() &&
                     m_gpioFile.is_open() && m_diagnosticsFile.is_open() &&
                     m_crashFile.is_open());

    if (m_initialized) {
        std::string ts = GetTimestampString();
        m_runtimeFile << "[" << ts << "] [" << m_nodeId << "] [BOOT] Deployment Logger initialized\n";
        m_runtimeFile.flush();
    }

    return m_initialized;
}

void DeploymentLogger::Close() {
    if (m_runtimeFile.is_open()) m_runtimeFile.close();
    if (m_networkFile.is_open()) m_networkFile.close();
    if (m_gpioFile.is_open()) m_gpioFile.close();
    if (m_diagnosticsFile.is_open()) m_diagnosticsFile.close();
    if (m_crashFile.is_open()) m_crashFile.close();
    m_initialized = false;
}

void DeploymentLogger::LogRuntime(const std::string& level, const std::string& component, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_runtimeFile.is_open()) return;

    m_runtimeFile << "[" << GetTimestampString() << "] [" << m_nodeId << "] ["
                  << level << "] [" << component << "] " << message << "\n";
    m_runtimeFile.flush();
}

void DeploymentLogger::LogNetwork(const std::string& direction,
                                  const std::string& peerNode,
                                  const std::string& messageType,
                                  size_t bytes,
                                  const std::string& details) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_networkFile.is_open()) return;

    m_networkFile << "[" << GetTimestampString() << "] [" << m_nodeId << "] ["
                  << direction << "] Peer: " << peerNode << " | Type: " << messageType
                  << " | Size: " << bytes << "B | " << details << "\n";
    m_networkFile.flush();
}

void DeploymentLogger::LogGpio(uint32_t pin, const std::string& action, const std::string& state, const std::string& details) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_gpioFile.is_open()) return;

    m_gpioFile << "[" << GetTimestampString() << "] [" << m_nodeId << "] Pin: " << pin
               << " | Action: " << action << " | State: " << state << " | " << details << "\n";
    m_gpioFile.flush();
}

void DeploymentLogger::LogDiagnostics(const std::string& component,
                                      double cpuPercent,
                                      double memoryPercent,
                                      double temperatureC,
                                      double healthScore,
                                      const std::string& notes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_diagnosticsFile.is_open()) return;

    m_diagnosticsFile << "[" << GetTimestampString() << "] [" << m_nodeId << "] ["
                      << component << "] CPU: " << cpuPercent << "% | RAM: " << memoryPercent
                      << "% | Temp: " << temperatureC << "C | Health: " << healthScore
                      << "/100 | " << notes << "\n";
    m_diagnosticsFile.flush();
}

void DeploymentLogger::LogCrash(const std::string& reason, const std::string& location, const std::string& details) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized || !m_crashFile.is_open()) return;

    m_crashFile << "[" << GetTimestampString() << "] [" << m_nodeId << "] [CRASH] Reason: "
                << reason << " | Location: " << location << " | " << details << "\n";
    m_crashFile.flush();
}

} // namespace deployment
} // namespace aegis