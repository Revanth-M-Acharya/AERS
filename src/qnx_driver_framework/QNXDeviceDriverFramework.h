/**
 * @file QNXDeviceDriverFramework.h
 * @brief SMART AEGIS (AERS) Phase 15B - QNX Device Driver Framework & BSP Integration
 *
 * Implements the QNX Neutrino Device Driver Framework that interfaces with the Hardware
 * Abstraction Layer (HAL, Phase 15A) and orchestrates driver lifecycle, resource management,
 * interrupt enablement, simulated QNX resource manager path attachments (/dev/...), and device I/O.
 *
 * Sits in the hierarchy:
 * HardwareAbstractionLayer (Phase 15A) -> QNXDeviceDriverFramework (Phase 15B) -> Future BSP Drivers
 *
 * Strict Compliance:
 * - Standard C++17 only.
 * - Deterministic execution.
 * - Thread-safe with exactly seven disjoint-scope mutexes (zero nested locks).
 * - Compatible with QNX SDP 8.0 & Raspberry Pi 4 Model B architecture.
 * - Zero third-party dependencies (STL only).
 * - Zero direct hardware register access (no mmap, no BCM2711 register manipulation).
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_DEVICE_DRIVER_FRAMEWORK_H
#define AEGIS_QNX_DEVICE_DRIVER_FRAMEWORK_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace qnx_driver {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Driver peripheral classification types supported by the QNX driver framework.
 */
enum class DriverType {
    GPIO,
    SPI,
    I2C,
    UART,
    ETHERNET,
    TIMER,
    INTERRUPT,
    OLED,
    GPS,
    UNKNOWN
};

/**
 * @brief Lifecycle states of a QNX device driver.
 */
enum class DriverState {
    UNREGISTERED,
    REGISTERED,
    INITIALIZED,
    RUNNING,
    STOPPED,
    FAILED
};

/**
 * @brief Operational and functional capabilities of a device driver.
 */
enum class DriverCapability {
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE,
    INTERRUPT_CAPABLE,
    DMA_CAPABLE
};

/**
 * @brief Allocation state of an underlying hardware resource.
 */
enum class ResourceState {
    UNALLOCATED,
    ALLOCATED,
    ACTIVE,
    RELEASED
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(DriverType type);
std::string ToString(DriverState state);
std::string ToString(DriverCapability cap);
std::string ToString(ResourceState state);

DriverType DriverTypeFromString(const std::string& str);
DriverState DriverStateFromString(const std::string& str);
DriverCapability DriverCapabilityFromString(const std::string& str);
ResourceState ResourceStateFromString(const std::string& str);

// =============================================================================
// Data Structures
// =============================================================================

/**
 * @brief Representation of a hardware resource managed by the QNX BSP layer.
 */
struct HardwareResource {
    std::string resourceId;
    std::string resourceName;
    std::string resourceType; ///< e.g. "GPIO_PIN", "IRQ_LINE", "I2C_BUS", "SPI_BUS", "UART_PORT"
    bool allocated{false};
    std::string ownerDriver;
    ResourceState resourceState{ResourceState::UNALLOCATED};

    HardwareResource() = default;

    HardwareResource(const std::string& id,
                     const std::string& name = "",
                     const std::string& type = "IO",
                     bool isAlloc = false,
                     const std::string& owner = "",
                     ResourceState state = ResourceState::UNALLOCATED)
        : resourceId(id),
          resourceName(name.empty() ? id : name),
          resourceType(type),
          allocated(isAlloc),
          ownerDriver(owner),
          resourceState(state) {}
};

/**
 * @brief Descriptor tracking the lifecycle, capabilities, and telemetry of a device driver.
 */
struct DriverDescriptor {
    std::string driverId;
    std::string driverName;
    DriverType driverType{DriverType::UNKNOWN};
    DriverState driverState{DriverState::UNREGISTERED};
    DriverCapability capability{DriverCapability::READ_WRITE};
    std::string devicePath;      ///< QNX resource manager prefix, e.g. "/dev/gpio0"
    uint32_t majorVersion{1};
    uint32_t minorVersion{0};
    bool driverLoaded{false};
    bool resourceAllocated{false};
    bool interruptEnabled{false};
    bool isRunning{false};
    std::string lastError;
    uint64_t startupTimestamp{0};  ///< Epoch milliseconds
    uint64_t shutdownTimestamp{0}; ///< Epoch milliseconds
    std::string allocatedResourceId;

    DriverDescriptor() = default;

    DriverDescriptor(const std::string& id,
                     const std::string& name = "",
                     DriverType type = DriverType::UNKNOWN,
                     DriverCapability cap = DriverCapability::READ_WRITE,
                     const std::string& path = "",
                     uint32_t major = 1,
                     uint32_t minor = 0)
        : driverId(id),
          driverName(name.empty() ? id : name),
          driverType(type),
          driverState(DriverState::UNREGISTERED),
          capability(cap),
          devicePath(path.empty() ? ("/dev/" + id) : path),
          majorVersion(major),
          minorVersion(minor),
          driverLoaded(false),
          resourceAllocated(false),
          interruptEnabled(false),
          isRunning(false),
          lastError(""),
          startupTimestamp(0),
          shutdownTimestamp(0),
          allocatedResourceId("") {}
};

/**
 * @brief Aggregated performance, latency, and resource metrics for the framework.
 */
struct ResourceStatistics {
    uint64_t totalDrivers{0};
    uint64_t runningDrivers{0};
    uint64_t failedDrivers{0};
    uint64_t allocatedResources{0};
    uint64_t releasedResources{0};
    uint64_t driverInitializations{0};
    uint64_t driverShutdowns{0};
    uint64_t resourceAllocations{0};
    uint64_t resourceReleases{0};
    double averageInitializationLatency{0.0}; ///< Milliseconds
    double averageShutdownLatency{0.0};       ///< Milliseconds

    // Detailed internal accounting
    uint64_t totalInitSamples{0};
    double totalInitSum{0.0};
    uint64_t totalShutdownSamples{0};
    double totalShutdownSum{0.0};

    uint64_t readOperations{0};
    double averageReadLatency{0.0}; ///< Milliseconds
    uint64_t totalReadSamples{0};
    double totalReadSum{0.0};

    uint64_t writeOperations{0};
    double averageWriteLatency{0.0}; ///< Milliseconds
    uint64_t totalWriteSamples{0};
    double totalWriteSum{0.0};

    uint64_t ioctlOperations{0};

    ResourceStatistics() = default;
};

// =============================================================================
// QNXDeviceDriverFramework Class
// =============================================================================

/**
 * @brief Master QNX Neutrino Device Driver Framework for SMART AEGIS.
 */
class QNXDeviceDriverFramework {
public:
    QNXDeviceDriverFramework();
    ~QNXDeviceDriverFramework() = default;

    // Non-copyable, non-movable to guarantee thread safety
    QNXDeviceDriverFramework(const QNXDeviceDriverFramework&) = delete;
    QNXDeviceDriverFramework& operator=(const QNXDeviceDriverFramework&) = delete;
    QNXDeviceDriverFramework(QNXDeviceDriverFramework&&) = delete;
    QNXDeviceDriverFramework& operator=(QNXDeviceDriverFramework&&) = delete;

    // -------------------------------------------------------------------------
    // Registration Interface
    // -------------------------------------------------------------------------
    bool RegisterDriver(const DriverDescriptor& descriptor);
    bool RegisterDriver(const std::string& driverId,
                        const std::string& driverName,
                        DriverType type,
                        DriverCapability capability = DriverCapability::READ_WRITE,
                        const std::string& devicePath = "",
                        uint32_t major = 1,
                        uint32_t minor = 0);
    bool RemoveDriver(const std::string& driverId);

    // -------------------------------------------------------------------------
    // Driver Lifecycle Interface
    // -------------------------------------------------------------------------
    bool InitializeDriver(const std::string& driverId);
    bool StartDriver(const std::string& driverId);
    bool StopDriver(const std::string& driverId);
    bool ShutdownDriver(const std::string& driverId);
    bool RestartDriver(const std::string& driverId);

    // -------------------------------------------------------------------------
    // Resource Management Interface
    // -------------------------------------------------------------------------
    bool AllocateResource(const std::string& driverId, const HardwareResource& resource);
    bool AllocateResource(const std::string& driverId,
                          const std::string& resourceId,
                          const std::string& resourceName = "",
                          const std::string& resourceType = "IO");
    bool ReleaseResource(const std::string& resourceId);
    bool ReleaseResourceForDriver(const std::string& driverId);

    // -------------------------------------------------------------------------
    // Interrupt Interface
    // -------------------------------------------------------------------------
    bool EnableInterrupt(const std::string& driverId);
    bool DisableInterrupt(const std::string& driverId);

    // -------------------------------------------------------------------------
    // Resource Manager Interface (QNX Neutrino resmgr)
    // -------------------------------------------------------------------------
    bool CreateResourceManager(const std::string& driverId, const std::string& path);
    bool DestroyResourceManager(const std::string& driverId);

    // -------------------------------------------------------------------------
    // Device Interface
    // -------------------------------------------------------------------------
    bool OpenDevice(const std::string& driverId);
    bool CloseDevice(const std::string& driverId);
    bool ReadDevice(const std::string& driverId, std::vector<uint8_t>& outBuffer, size_t maxBytes = 256);
    bool ReadDevice(const std::string& driverId, std::string& outData);
    bool WriteDevice(const std::string& driverId, const std::vector<uint8_t>& data);
    bool WriteDevice(const std::string& driverId, const std::string& data);

    // -------------------------------------------------------------------------
    // IO Control Interface
    // -------------------------------------------------------------------------
    bool IoControl(const std::string& driverId, uint32_t cmd, const std::string& inArg, std::string& outResult);
    bool IoControl(const std::string& driverId, uint32_t cmd, void* arg = nullptr);

    // -------------------------------------------------------------------------
    // Status & Telemetry Queries
    // -------------------------------------------------------------------------
    bool GetDriver(const std::string& driverId, DriverDescriptor& outDriver) const;
    std::vector<DriverDescriptor> GetAllDrivers() const;
    std::vector<DriverDescriptor> GetRunningDrivers() const;
    ResourceStatistics GetStatistics() const;
    bool GetResource(const std::string& resourceId, HardwareResource& outResource) const;
    std::vector<HardwareResource> GetAllResources() const;
    std::string GetLastError(const std::string& driverId) const;

    // -------------------------------------------------------------------------
    // Structured ASCII Operational Reports
    // -------------------------------------------------------------------------
    std::string GenerateDriverFrameworkReport() const;
    std::string GenerateDriverReport(const std::string& driverId = "") const;
    std::string GenerateResourceReport(const std::string& resourceId = "") const;

    // -------------------------------------------------------------------------
    // Lifecycle Reset
    // -------------------------------------------------------------------------
    void Reset();

private:
    static uint64_t GetCurrentTimeMs();

    // Exactly seven mutexes as strictly specified:
    mutable std::mutex registryMutex_;
    mutable std::mutex driverMutex_;
    mutable std::mutex resourceMutex_;
    mutable std::mutex interruptMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex errorMutex_;

    // Registries & internal states
    std::unordered_map<std::string, DriverDescriptor> drivers_;
    std::unordered_map<std::string, HardwareResource> resources_;
    std::unordered_map<std::string, std::string> resmgrPaths_;
    std::unordered_map<std::string, bool> openDevices_;
    std::unordered_map<std::string, std::vector<uint8_t>> deviceBuffers_;

    // Saved resource mapping for deterministic RestartDriver sequence
    std::unordered_map<std::string, HardwareResource> driverResourceTemplates_;

    // Aggregated statistics
    ResourceStatistics stats_;

    // Forensic error logs (driverId -> error description)
    std::unordered_map<std::string, std::string> errorLogs_;
};

} // namespace qnx_driver

// Namespace alias
namespace driver_framework = qnx_driver;

} // namespace aegis

#endif // AEGIS_QNX_DEVICE_DRIVER_FRAMEWORK_H
