/**
 * @file HardwareAbstractionLayer.h
 * @brief SMART AEGIS (AERS) Phase 15A - Hardware Abstraction Layer (HAL) & Device Driver Interface
 *
 * Implements the centralized hardware abstraction layer separating all upper business logic
 * (Coordinator, Mission Engine, Incident Command, Traffic Network, Consensus, Cluster Manager,
 * System Diagnostics) from physical hardware and edge controller interfaces.
 *
 * Exposes generic, deterministic device driver interfaces for GPIO, PWM, SPI, UART, I2C,
 * Ethernet, LED, OLED, GPS, and Buzzer peripherals without leaking hardware-specific semantics.
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 Model B & POSIX compliant.
 * Zero third-party dependencies, thread-safe (7 disjoint mutexes), deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_HARDWARE_ABSTRACTION_LAYER_H
#define AEGIS_HARDWARE_ABSTRACTION_LAYER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>

namespace aegis {
namespace hal {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Generic hardware peripheral and device types supported by SMART AEGIS HAL.
 */
enum class DeviceType {
    GPIO,
    PWM,
    SPI,
    UART,
    I2C,
    ETHERNET,
    LED,
    OLED,
    GPS,
    BUZZER,
    UNKNOWN
};

/**
 * @brief Operational and lifecycle states of a hardware device.
 */
enum class DeviceState {
    OFFLINE,
    ONLINE,
    INITIALIZED,
    READY,
    BUSY,
    ERROR
};

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(DeviceType type);
std::string ToString(DeviceState state);

DeviceType DeviceTypeFromString(const std::string& str);
DeviceState DeviceStateFromString(const std::string& str);

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Execution and latency metrics for commands executed on hardware.
 */
struct HardwareStatistics {
    uint64_t commandsSent{0};
    uint64_t commandsSucceeded{0};
    uint64_t commandsFailed{0};
    double averageLatency{0.0};  ///< Milliseconds
    double lastLatency{0.0};     ///< Milliseconds
    uint64_t totalLatencySamples{0};
    double totalLatencySum{0.0};

    HardwareStatistics() = default;
};

/**
 * @brief Abstract representation of a physical or simulated hardware device.
 */
struct HardwareDevice {
    std::string deviceId;
    std::string deviceName;
    DeviceType deviceType{DeviceType::UNKNOWN};
    DeviceState deviceState{DeviceState::OFFLINE};
    bool isInitialized{false};
    bool isEnabled{false};
    bool driverLoaded{false};
    std::string devicePath;      ///< e.g., "/dev/gpio0", "/dev/ttyS0", "/dev/i2c-1"
    std::string lastError;
    uint64_t lastCommunication{0}; ///< Epoch milliseconds
    HardwareStatistics statistics;

    HardwareDevice() = default;

    HardwareDevice(const std::string& id,
                   DeviceType type = DeviceType::UNKNOWN,
                   const std::string& name = "",
                   const std::string& path = "")
        : deviceId(id),
          deviceName(name.empty() ? id : name),
          deviceType(type),
          deviceState(DeviceState::OFFLINE),
          isInitialized(false),
          isEnabled(false),
          driverLoaded(false),
          devicePath(path.empty() ? ("/dev/" + id) : path) {}
};

// =============================================================================
// HardwareAbstractionLayer Class
// =============================================================================

/**
 * @brief Master Hardware Abstraction Layer (HAL) for SMART AEGIS.
 */
class HardwareAbstractionLayer {
public:
    HardwareAbstractionLayer();
    ~HardwareAbstractionLayer() = default;

    // Non-copyable, non-movable to guarantee thread safety
    HardwareAbstractionLayer(const HardwareAbstractionLayer&) = delete;
    HardwareAbstractionLayer& operator=(const HardwareAbstractionLayer&) = delete;
    HardwareAbstractionLayer(HardwareAbstractionLayer&&) = delete;
    HardwareAbstractionLayer& operator=(HardwareAbstractionLayer&&) = delete;

    // -------------------------------------------------------------------------
    // Device Registration & Lifecycle
    // -------------------------------------------------------------------------
    bool RegisterDevice(const HardwareDevice& device);
    bool RegisterDevice(const std::string& deviceId,
                        DeviceType type,
                        const std::string& name = "",
                        const std::string& path = "");
    bool RemoveDevice(const std::string& deviceId);

    // -------------------------------------------------------------------------
    // Driver & Hardware Initialization
    // -------------------------------------------------------------------------
    bool LoadDriver(const std::string& deviceId);
    bool UnloadDriver(const std::string& deviceId);
    bool InitializeDevice(const std::string& deviceId);
    bool ShutdownDevice(const std::string& deviceId);
    bool EnableDevice(const std::string& deviceId);
    bool DisableDevice(const std::string& deviceId);
    bool ResetDevice(const std::string& deviceId);

    // -------------------------------------------------------------------------
    // Command & Status Interface
    // -------------------------------------------------------------------------
    bool SendCommand(const std::string& deviceId, const std::string& command);
    bool SendCommand(const std::string& deviceId,
                     const std::string& command,
                     std::string& outResponse);
    bool ReadStatus(const std::string& deviceId, std::string& outStatus) const;
    DeviceState ReadStatus(const std::string& deviceId) const;

    // -------------------------------------------------------------------------
    // Queries & Telemetry
    // -------------------------------------------------------------------------
    bool GetDevice(const std::string& deviceId, HardwareDevice& outDevice) const;
    std::vector<HardwareDevice> GetAllDevices() const;
    HardwareStatistics GetStatistics() const;
    bool GetDeviceStatistics(const std::string& deviceId, HardwareStatistics& outStats) const;

    // -------------------------------------------------------------------------
    // Structured ASCII Operational Reports
    // -------------------------------------------------------------------------
    std::string GenerateHardwareReport() const;
    std::string GenerateDriverReport() const;
    std::string GenerateDeviceReport(const std::string& deviceId = "") const;

    // -------------------------------------------------------------------------
    // Lifecycle Reset
    // -------------------------------------------------------------------------
    void Reset();

private:
    static uint64_t GetCurrentTimeMs();

    // Seven dedicated mutexes as strictly specified (no nested locking):
    mutable std::mutex registryMutex_;
    mutable std::mutex driverMutex_;
    mutable std::mutex commandMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex deviceMutex_;
    mutable std::mutex errorMutex_;

    // Registry data
    std::unordered_map<std::string, HardwareDevice> devices_;

    // Global aggregated statistics
    HardwareStatistics globalStatistics_;

    // Driver loaded registry
    std::unordered_map<std::string, bool> driversLoaded_;

    // Recent error log registry
    std::unordered_map<std::string, std::string> errorLogs_;
};

} // namespace hal

// Namespace alias for hardware_abstraction
namespace hardware_abstraction = hal;

} // namespace aegis

#endif // AEGIS_HARDWARE_ABSTRACTION_LAYER_H

