/**
 * @file QnxResourceManager.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Native QNX Resource Manager (/dev/aegis nodes).
 *
 * Implements native QNX resource manager attaching device nodes under /dev/aegis:
 * - /dev/aegis/coordinator
 * - /dev/aegis/intersectionA
 * - /dev/aegis/intersectionB
 * - /dev/aegis/intersectionC
 * - /dev/aegis/cluster
 * - /dev/aegis/system
 *
 * Supports POSIX open(), read(), write(), close() and native devctl() handler dispatching.
 * Provides complete dual-mode operation for native QNX SDP 8.0 and simulated host testing.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_RESOURCE_MANAGER_HPP
#define AEGIS_QNX_RESOURCE_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <sys/types.h>

#include "aegis/qnx/QnxDevctl.hpp"

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <sys/iofunc.h>
    #include <sys/dispatch.h>
#endif

namespace aegis {
namespace qnx {

// Standard device node path constants
namespace DevicePath {
    inline const std::string COORDINATOR    = "/dev/aegis/coordinator";
    inline const std::string INTERSECTION_A = "/dev/aegis/intersectionA";
    inline const std::string INTERSECTION_B = "/dev/aegis/intersectionB";
    inline const std::string INTERSECTION_C = "/dev/aegis/intersectionC";
    inline const std::string CLUSTER        = "/dev/aegis/cluster";
    inline const std::string SYSTEM         = "/dev/aegis/system";
}

/**
 * @brief Internal representation of a registered device node.
 */
struct DeviceNodeInfo {
    std::string path;
    std::string name;
    uint64_t openCount{0};
    uint64_t readCount{0};
    uint64_t writeCount{0};
    uint64_t devctlCount{0};
    bool active{true};
};

/**
 * @brief Virtual file descriptor handle in simulation.
 */
struct OpenFileHandle {
    int fd{0};
    std::string path;
    int flags{0};
    size_t readOffset{0};
    std::string cachedReadBuffer;
};

/**
 * @brief Native QNX Resource Manager class.
 */
class QnxResourceManager {
public:
    static QnxResourceManager& Instance();

    /**
     * @brief Initializes and attaches all default /dev/aegis nodes.
     * @return 0 on success, negative on error.
     */
    int Initialize();

    /**
     * @brief Shuts down resource manager and detaches device paths.
     */
    void Shutdown();

    /**
     * @brief Registers an individual device path.
     */
    bool RegisterDeviceNode(const std::string& path);

    /**
     * @brief Unregisters a device path.
     */
    bool UnregisterDeviceNode(const std::string& path);

    /**
     * @brief Checks if a device path is registered and active.
     */
    bool HasDeviceNode(const std::string& path) const;

    /**
     * @brief Retrieves all registered device paths.
     */
    std::vector<std::string> GetRegisteredNodes() const;

    /**
     * @brief Opens a device node by path.
     * @return Simulated or native file descriptor (>= 0), or -1 on error.
     */
    int Open(const std::string& path, int flags = 0);

    /**
     * @brief Reads status data from an open device node descriptor.
     */
    ssize_t Read(int fd, void* buf, size_t count);

    /**
     * @brief Writes a command to an open device node descriptor.
     */
    ssize_t Write(int fd, const void* buf, size_t count);

    /**
     * @brief Closes an open device node descriptor.
     */
    int Close(int fd);

    /**
     * @brief Dispatches a devctl() command on an open device descriptor.
     */
    int DispatchDevctl(int fd, uint32_t dcmd, void* dataPtr, size_t nBytes);
    int DispatchDevctl(int fd, int dcmd, void* dataPtr, size_t nBytes);

    /**
     * @brief Retrieves usage statistics for a device node.
     */
    bool GetNodeStats(const std::string& path, DeviceNodeInfo& outInfo) const;

private:
    QnxResourceManager();
    ~QnxResourceManager();

    // Prevent copying
    QnxResourceManager(const QnxResourceManager&) = delete;
    QnxResourceManager& operator=(const QnxResourceManager&) = delete;

    std::string GenerateDeviceContent(const std::string& path);

    mutable std::mutex m_mutex;
    bool m_initialized{false};
    std::unordered_map<std::string, DeviceNodeInfo> m_nodes;
    std::unordered_map<int, OpenFileHandle> m_openFiles;
    int m_nextFd{100};

#if defined(__QNX__) || defined(__QNXNTO__)
    dispatch_t* m_dispatch{nullptr};
    resmgr_connect_funcs_t m_connectFuncs;
    resmgr_io_funcs_t m_ioFuncs;
    iofunc_attr_t m_ioattr;
#endif
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_RESOURCE_MANAGER_HPP
