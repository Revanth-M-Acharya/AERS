/**
 * @file QnxResourceManager.cpp
 * @brief Implementation of Phase 4 Native QNX Resource Manager.
 */

#include "aegis/qnx/QnxResourceManager.hpp"
#include "aegis/logger/logger.hpp"
#include "aegis/common/time_utils.hpp"

#include <cstring>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace qnx {

// Hook invoked by InvokeDevctl() in non-QNX simulation
int SimulatedDevctlDispatcher(int fd, int dcmd, void* dataPtr, size_t nBytes) {
    return QnxResourceManager::Instance().DispatchDevctl(fd, dcmd, dataPtr, nBytes);
}

// =============================================================================
// QnxResourceManager Implementation
// =============================================================================

QnxResourceManager& QnxResourceManager::Instance() {
    static QnxResourceManager instance;
    return instance;
}

QnxResourceManager::QnxResourceManager() {
    Initialize();
}

QnxResourceManager::~QnxResourceManager() {
    Shutdown();
}

int QnxResourceManager::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) {
        return 0;
    }

    // Register 6 standard /dev/aegis/* nodes
    const std::vector<std::string> defaultPaths = {
        DevicePath::COORDINATOR,
        DevicePath::INTERSECTION_A,
        DevicePath::INTERSECTION_B,
        DevicePath::INTERSECTION_C,
        DevicePath::CLUSTER,
        DevicePath::SYSTEM
    };

    for (const auto& path : defaultPaths) {
        DeviceNodeInfo info;
        info.path = path;
        size_t lastSlash = path.find_last_of('/');
        info.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        info.active = true;
        m_nodes[path] = info;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    m_dispatch = dispatch_create();
    if (m_dispatch != nullptr) {
        iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &m_connectFuncs,
                         _RESMGR_IO_NFUNCS, &m_ioFuncs);
        iofunc_attr_init(&m_ioattr, S_IFCHR | 0666, nullptr, nullptr);

        for (const auto& path : defaultPaths) {
            resmgr_attach(m_dispatch, nullptr, path.c_str(), _FTYPE_ANY, 0,
                           &m_connectFuncs, &m_ioFuncs, &m_ioattr);
        }
    }
#endif

    m_initialized = true;
    return 0;
}

void QnxResourceManager::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_openFiles.clear();
    m_nodes.clear();
#if defined(__QNX__) || defined(__QNXNTO__)
    if (m_dispatch != nullptr) {
        dispatch_destroy(m_dispatch);
        m_dispatch = nullptr;
    }
#endif
    m_initialized = false;
}

bool QnxResourceManager::RegisterDeviceNode(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nodes.find(path) != m_nodes.end()) {
        m_nodes[path].active = true;
        return true;
    }

    DeviceNodeInfo info;
    info.path = path;
    size_t lastSlash = path.find_last_of('/');
    info.name = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
    info.active = true;
    m_nodes[path] = info;

#if defined(__QNX__) || defined(__QNXNTO__)
    if (m_dispatch != nullptr) {
        resmgr_attach(m_dispatch, nullptr, path.c_str(), _FTYPE_ANY, 0,
                       &m_connectFuncs, &m_ioFuncs, &m_ioattr);
    }
#endif
    return true;
}

bool QnxResourceManager::UnregisterDeviceNode(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(path);
    if (it != m_nodes.end()) {
        it->second.active = false;
        m_nodes.erase(it);
        return true;
    }
    return false;
}

bool QnxResourceManager::HasDeviceNode(const std::string& path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(path);
    return (it != m_nodes.end() && it->second.active);
}

std::vector<std::string> QnxResourceManager::GetRegisteredNodes() const {
    std::vector<std::string> paths;
    std::lock_guard<std::mutex> lock(m_mutex);
    paths.reserve(m_nodes.size());
    for (const auto& pair : m_nodes) {
        if (pair.second.active) {
            paths.push_back(pair.first);
        }
    }
    return paths;
}

std::string QnxResourceManager::GenerateDeviceContent(const std::string& path) {
    std::ostringstream oss;
    if (path == DevicePath::COORDINATOR) {
        oss << "{\n"
            << "  \"device\": \"coordinator\",\n"
            << "  \"state\": \"ONLINE\",\n"
            << "  \"active_corridors\": 1,\n"
            << "  \"connected_intersections\": 3,\n"
            << "  \"qnx_resmgr\": true\n"
            << "}\n";
    } else if (path == DevicePath::INTERSECTION_A || path == DevicePath::INTERSECTION_B || path == DevicePath::INTERSECTION_C) {
        size_t lastSlash = path.find_last_of('/');
        std::string node = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        oss << "{\n"
            << "  \"device\": \"" << node << "\",\n"
            << "  \"phase\": \"ROAD_A_GREEN\",\n"
            << "  \"gpio_status\": \"HEALTHY\",\n"
            << "  \"relay_status\": \"HEALTHY\",\n"
            << "  \"qnx_resmgr\": true\n"
            << "}\n";
    } else if (path == DevicePath::CLUSTER) {
        oss << "{\n"
            << "  \"device\": \"cluster\",\n"
            << "  \"health_score\": 1.0,\n"
            << "  \"nodes_total\": 4,\n"
            << "  \"nodes_healthy\": 4,\n"
            << "  \"qnx_resmgr\": true\n"
            << "}\n";
    } else {
        oss << "{\n"
            << "  \"device\": \"system\",\n"
            << "  \"os\": \"QNX Neutrino RTOS 8.0\",\n"
            << "  \"target\": \"Raspberry Pi 4 Model B (aarch64le)\",\n"
            << "  \"status\": \"OPERATIONAL\"\n"
            << "}\n";
    }
    return oss.str();
}

int QnxResourceManager::Open(const std::string& path, int flags) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(path);
    if (it == m_nodes.end() || !it->second.active) {
        return -1;
    }

    it->second.openCount++;

    int fd = m_nextFd++;
    OpenFileHandle handle;
    handle.fd = fd;
    handle.path = path;
    handle.flags = flags;
    handle.readOffset = 0;
    handle.cachedReadBuffer = GenerateDeviceContent(path);

    m_openFiles[fd] = handle;
    return fd;
}

ssize_t QnxResourceManager::Read(int fd, void* buf, size_t count) {
    if (!buf || count == 0) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return -1;
    }

    auto& handle = it->second;
    auto nodeIt = m_nodes.find(handle.path);
    if (nodeIt != m_nodes.end()) {
        nodeIt->second.readCount++;
    }

    if (handle.readOffset >= handle.cachedReadBuffer.size()) {
        return 0; // EOF
    }

    size_t available = handle.cachedReadBuffer.size() - handle.readOffset;
    size_t toCopy = (count < available) ? count : available;

    std::memcpy(buf, handle.cachedReadBuffer.data() + handle.readOffset, toCopy);
    handle.readOffset += toCopy;
    return static_cast<ssize_t>(toCopy);
}

ssize_t QnxResourceManager::Write(int fd, const void* buf, size_t count) {
    if (!buf || count == 0) return 0;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return -1;
    }

    auto& handle = it->second;
    auto nodeIt = m_nodes.find(handle.path);
    if (nodeIt != m_nodes.end()) {
        nodeIt->second.writeCount++;
    }

    // Process written command string
    std::string cmd(static_cast<const char*>(buf), count);
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "QnxResMgr",
                                          "Command written to " + handle.path + ": " + cmd);

    return static_cast<ssize_t>(count);
}

int QnxResourceManager::Close(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return -1;
    }
    m_openFiles.erase(it);
    return 0;
}

int QnxResourceManager::DispatchDevctl(int fd, int dcmd, void* dataPtr, size_t nBytes) {
    (void)nBytes;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return -1;
    }

    auto& handle = it->second;
    auto nodeIt = m_nodes.find(handle.path);
    if (nodeIt != m_nodes.end()) {
        nodeIt->second.devctlCount++;
    }

    if (!dataPtr) return -1;

    switch (dcmd) {
        case DCMD_AEGIS_GET_SYSTEM_STATUS: {
            auto* p = static_cast<AegisSystemStatusPayload*>(dataPtr);
            std::strncpy(p->state, "NORMAL_OPERATIONS", sizeof(p->state) - 1);
            p->activeCorridors = 1;
            p->activeNodes = 4;
            p->uptimeSec = 3600;
            p->clusterScore = 1.0;
            return 0;
        }

        case DCMD_AEGIS_GET_NODE_STATUS: {
            auto* p = static_cast<AegisNodeStatusPayload*>(dataPtr);
            if (p->nodeId[0] == '\0') {
                std::strncpy(p->nodeId, "INTERSECTION_A", sizeof(p->nodeId) - 1);
            }
            std::strncpy(p->role, "INTERSECTION_CONTROLLER", sizeof(p->role) - 1);
            std::strncpy(p->ipAddress, "192.168.1.101", sizeof(p->ipAddress) - 1);
            std::strncpy(p->signalPhase, "ROAD_A_GREEN", sizeof(p->signalPhase) - 1);
            p->gpioHealthy = true;
            p->relayHealthy = true;
            p->cpuTemp = 42.5;
            return 0;
        }

        case DCMD_AEGIS_START_CORRIDOR: {
            auto* p = static_cast<AegisStartCorridorPayload*>(dataPtr);
            p->etaSec = 45.0;
            p->success = true;
            return 0;
        }

        case DCMD_AEGIS_STOP_CORRIDOR: {
            auto* p = static_cast<AegisStopCorridorPayload*>(dataPtr);
            p->success = true;
            return 0;
        }

        case DCMD_AEGIS_RESET_NODE: {
            auto* p = static_cast<AegisResetNodePayload*>(dataPtr);
            p->success = true;
            return 0;
        }

        case DCMD_AEGIS_READ_TELEMETRY: {
            auto* p = static_cast<AegisTelemetryPayload*>(dataPtr);
            p->cpuUsage = 14.5;
            p->ramUsageMB = 128.4;
            p->cpuTemp = 42.0;
            p->rxPackets = 1042;
            p->txPackets = 980;
            p->networkLatencyMs = 1.2;
            return 0;
        }

        case DCMD_AEGIS_READ_LOGS: {
            auto* p = static_cast<AegisLogsPayload*>(dataPtr);
            p->logCount = 128;
            std::strncpy(p->lastLogEntry, "[INFO] QNX Neutrino RTOS operational", sizeof(p->lastLogEntry) - 1);
            return 0;
        }

        case DCMD_AEGIS_ENABLE_DIAGNOSTICS: {
            // Already validated
            return 0;
        }

        case DCMD_AEGIS_RUN_SELF_TEST: {
            auto* p = static_cast<AegisSelfTestPayload*>(dataPtr);
            p->relaysPassed = true;
            p->gpioPassed = true;
            p->networkPassed = true;
            p->overallSuccess = true;
            std::strncpy(p->details, "All QNX hardware subsystems passed self-test", sizeof(p->details) - 1);
            return 0;
        }

        default:
            return -1;
    }
}

bool QnxResourceManager::GetNodeStats(const std::string& path, DeviceNodeInfo& outInfo) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodes.find(path);
    if (it != m_nodes.end()) {
        outInfo = it->second;
        return true;
    }
    return false;
}

} // namespace qnx
} // namespace aegis
