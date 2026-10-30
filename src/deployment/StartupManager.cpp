/**
 * @file StartupManager.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Node Startup & Environment Supervisor Implementation.
 */

#include "StartupManager.h"
#include <fstream>
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>

namespace aegis {
namespace deployment {

namespace {
std::function<void(int)> g_signalHandlerCallback;

void GlobalSignalDispatcher(int signum) {
    if (g_signalHandlerCallback) {
        g_signalHandlerCallback(signum);
    }
}
} // anonymous namespace

StartupManager::StartupManager() = default;
StartupManager::~StartupManager() = default;

bool StartupManager::InitializeEnvironment(const std::string& logDirectory) {
    ::mkdir(logDirectory.c_str(), 0755);
    ::mkdir((logDirectory + "/crash_dumps").c_str(), 0755);
    return true;
}

bool StartupManager::CreatePidFile(const std::string& pidFilePath) {
    if (pidFilePath.empty()) return false;
    std::ofstream ofs(pidFilePath);
    if (!ofs.is_open()) return false;
    ofs << ::getpid() << "\n";
    return true;
}

bool StartupManager::RemovePidFile(const std::string& pidFilePath) {
    if (pidFilePath.empty()) return false;
    return (::unlink(pidFilePath.c_str()) == 0);
}

bool StartupManager::SetupSignalHandlers(std::function<void(int)> handler) {
    g_signalHandlerCallback = std::move(handler);
    ::signal(SIGINT, GlobalSignalDispatcher);
    ::signal(SIGTERM, GlobalSignalDispatcher);
#if defined(SIGPIPE)
    ::signal(SIGPIPE, SIG_IGN);
#endif
    return true;
}

bool StartupManager::WaitForClusterOnline(const std::vector<std::string>& expectedNodes,
                                         std::function<bool(const std::string&)> isNodeOnline,
                                         uint32_t timeoutMs) {
    if (expectedNodes.empty()) return true;
    if (!isNodeOnline) return false;

    auto start = std::chrono::steady_clock::now();
    while (true) {
        bool allOnline = true;
        for (const auto& nodeId : expectedNodes) {
            if (!isNodeOnline(nodeId)) {
                allOnline = false;
                break;
            }
        }
        if (allOnline) {
            return true;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= timeoutMs) {
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
}

} // namespace deployment
} // namespace aegis
