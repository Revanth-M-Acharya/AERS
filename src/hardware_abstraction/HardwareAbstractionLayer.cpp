/**
 * @file HardwareAbstractionLayer.cpp
 * @brief Implementation of SMART AEGIS (AERS) Phase 15A Hardware Abstraction Layer (HAL)
 *
 * Pure C++17, QNX SDP 8.0 & POSIX compliant.
 * Zero third-party dependencies.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "HardwareAbstractionLayer.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <set>

namespace aegis {
namespace hal {

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(DeviceType type) {
    switch (type) {
        case DeviceType::GPIO:     return "GPIO";
        case DeviceType::PWM:      return "PWM";
        case DeviceType::SPI:      return "SPI";
        case DeviceType::UART:     return "UART";
        case DeviceType::I2C:      return "I2C";
        case DeviceType::ETHERNET: return "ETHERNET";
        case DeviceType::LED:      return "LED";
        case DeviceType::OLED:     return "OLED";
        case DeviceType::GPS:      return "GPS";
        case DeviceType::BUZZER:   return "BUZZER";
        case DeviceType::UNKNOWN:
        default:                   return "UNKNOWN";
    }
}

std::string ToString(DeviceState state) {
    switch (state) {
        case DeviceState::OFFLINE:     return "OFFLINE";
        case DeviceState::ONLINE:      return "ONLINE";
        case DeviceState::INITIALIZED: return "INITIALIZED";
        case DeviceState::READY:       return "READY";
        case DeviceState::BUSY:        return "BUSY";
        case DeviceState::ERROR:       return "ERROR";
        default:                       return "OFFLINE";
    }
}

DeviceType DeviceTypeFromString(const std::string& str) {
    if (str == "GPIO")     return DeviceType::GPIO;
    if (str == "PWM")      return DeviceType::PWM;
    if (str == "SPI")      return DeviceType::SPI;
    if (str == "UART")     return DeviceType::UART;
    if (str == "I2C")      return DeviceType::I2C;
    if (str == "ETHERNET") return DeviceType::ETHERNET;
    if (str == "LED")      return DeviceType::LED;
    if (str == "OLED")     return DeviceType::OLED;
    if (str == "GPS")      return DeviceType::GPS;
    if (str == "BUZZER")   return DeviceType::BUZZER;
    return DeviceType::UNKNOWN;
}

DeviceState DeviceStateFromString(const std::string& str) {
    if (str == "ONLINE")      return DeviceState::ONLINE;
    if (str == "INITIALIZED") return DeviceState::INITIALIZED;
    if (str == "READY")       return DeviceState::READY;
    if (str == "BUSY")        return DeviceState::BUSY;
    if (str == "ERROR")       return DeviceState::ERROR;
    return DeviceState::OFFLINE;
}

// =============================================================================
// Constructor & Reset
// =============================================================================

HardwareAbstractionLayer::HardwareAbstractionLayer() {
    Reset();
}

void HardwareAbstractionLayer::Reset() {
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        devices_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(driverMutex_);
        driversLoaded_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(statisticsMutex_);
        globalStatistics_ = HardwareStatistics();
    }
    {
        std::lock_guard<std::mutex> lock(errorMutex_);
        errorLogs_.clear();
    }
}

uint64_t HardwareAbstractionLayer::GetCurrentTimeMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

// =============================================================================
// Device Registration & Lifecycle (Rule 9)
// =============================================================================

bool HardwareAbstractionLayer::RegisterDevice(const HardwareDevice& device) {
    if (device.deviceId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(registryMutex_);
    // Rule 9: Duplicate device IDs are rejected
    if (devices_.find(device.deviceId) != devices_.end()) {
        return false;
    }

    HardwareDevice copy = device;
    if (copy.lastCommunication == 0) {
        copy.lastCommunication = GetCurrentTimeMs();
    }
    devices_[copy.deviceId] = copy;
    return true;
}

bool HardwareAbstractionLayer::RegisterDevice(const std::string& deviceId,
                                              DeviceType type,
                                              const std::string& name,
                                              const std::string& path) {
    HardwareDevice dev(deviceId, type, name, path);
    return RegisterDevice(dev);
}

bool HardwareAbstractionLayer::RemoveDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        devices_.erase(it);
        return true;
    }
    return false;
}

// =============================================================================
// Driver & Hardware Initialization (Rules 1, 2, 7)
// =============================================================================

bool HardwareAbstractionLayer::LoadDriver(const std::string& deviceId) {
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = devices_.find(deviceId);
        if (it != devices_.end()) {
            it->second.driverLoaded = true;
            if (it->second.deviceState == DeviceState::OFFLINE) {
                it->second.deviceState = DeviceState::ONLINE;
            }
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> dLock(driverMutex_);
        driversLoaded_[deviceId] = true;
    }

    return found;
}

bool HardwareAbstractionLayer::UnloadDriver(const std::string& deviceId) {
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(registryMutex_);
        auto it = devices_.find(deviceId);
        if (it != devices_.end()) {
            it->second.driverLoaded = false;
            it->second.isInitialized = false;
            it->second.isEnabled = false;
            it->second.deviceState = DeviceState::OFFLINE;
            found = true;
        }
    }

    if (found) {
        std::lock_guard<std::mutex> dLock(driverMutex_);
        driversLoaded_[deviceId] = false;
    }

    return found;
}

bool HardwareAbstractionLayer::InitializeDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }

    // Rule 2: A device cannot initialize unless driverLoaded == true
    if (!it->second.driverLoaded) {
        std::lock_guard<std::mutex> errLock(errorMutex_);
        it->second.lastError = "Driver not loaded";
        errorLogs_[deviceId] = "Initialization rejected: Driver not loaded";
        return false;
    }

    it->second.isInitialized = true;
    it->second.isEnabled = true;
    it->second.deviceState = DeviceState::READY;
    it->second.lastCommunication = GetCurrentTimeMs();
    return true;
}

bool HardwareAbstractionLayer::ShutdownDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }

    it->second.isEnabled = false;
    it->second.isInitialized = false;
    it->second.deviceState = DeviceState::OFFLINE;
    return true;
}

bool HardwareAbstractionLayer::EnableDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }

    // Must be initialized before enabling
    if (!it->second.isInitialized) {
        return false;
    }

    it->second.isEnabled = true;
    if (it->second.deviceState != DeviceState::ERROR) {
        it->second.deviceState = DeviceState::READY;
    }
    return true;
}

bool HardwareAbstractionLayer::DisableDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }

    it->second.isEnabled = false;
    if (it->second.deviceState == DeviceState::READY || it->second.deviceState == DeviceState::BUSY) {
        it->second.deviceState = DeviceState::INITIALIZED;
    }
    return true;
}

bool HardwareAbstractionLayer::ResetDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it == devices_.end()) {
        return false;
    }

    // Rule 7: Device reset clears error state
    it->second.lastError.clear();
    if (it->second.deviceState == DeviceState::ERROR) {
        it->second.deviceState = it->second.isInitialized ? DeviceState::READY : DeviceState::ONLINE;
    }
    it->second.lastCommunication = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> errLock(errorMutex_);
        errorLogs_.erase(deviceId);
    }

    return true;
}

// =============================================================================
// Command & Status Interface (Rules 1, 3, 4, 5, 6, 10)
// =============================================================================

bool HardwareAbstractionLayer::SendCommand(const std::string& deviceId, const std::string& command) {
    std::string response;
    return SendCommand(deviceId, command, response);
}

bool HardwareAbstractionLayer::SendCommand(const std::string& deviceId,
                                           const std::string& command,
                                           std::string& outResponse) {
    auto start = std::chrono::high_resolution_clock::now();
    bool success = false;
    std::string failureReason;
    double latencyMs = 0.0;

    {
        std::lock_guard<std::mutex> cmdLock(commandMutex_);
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = devices_.find(deviceId);

        if (it == devices_.end()) {
            failureReason = "Device not registered";
        } else {
            // Rule 3: Offline devices reject all commands
            if (it->second.deviceState == DeviceState::OFFLINE) {
                failureReason = "Device is offline";
            }
            // Rule 1: A device cannot receive commands before initialization
            else if (!it->second.isInitialized) {
                failureReason = "Device not initialized";
            }
            // Rule 4: Disabled devices reject commands
            else if (!it->second.isEnabled) {
                failureReason = "Device is disabled";
            }
            // Rule 2 check: Driver loaded
            else if (!it->second.driverLoaded) {
                failureReason = "Driver not loaded";
            } else {
                // Command allowed: simulate device bus processing (Rule 10: HAL never exposes hardware details)
                it->second.deviceState = DeviceState::BUSY;
                it->second.lastCommunication = GetCurrentTimeMs();

                // Abstract generic response
                outResponse = "ACK:" + command + ":OK";
                it->second.deviceState = DeviceState::READY;
                success = true;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        latencyMs = std::chrono::duration<double, std::milli>(end - start).count();

        if (it != devices_.end()) {
            // Rule 5: Every command updates statistics
            it->second.statistics.commandsSent++;
            it->second.statistics.lastLatency = latencyMs;
            it->second.statistics.totalLatencySum += latencyMs;
            it->second.statistics.totalLatencySamples++;
            it->second.statistics.averageLatency =
                it->second.statistics.totalLatencySum / static_cast<double>(it->second.statistics.totalLatencySamples);

            if (success) {
                it->second.statistics.commandsSucceeded++;
            } else {
                it->second.statistics.commandsFailed++;
                // Rule 6: Failed commands are logged
                it->second.lastError = failureReason;
            }
        }
    }

    // Update global statistics (Rule 5)
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        globalStatistics_.commandsSent++;
        globalStatistics_.lastLatency = latencyMs;
        globalStatistics_.totalLatencySum += latencyMs;
        globalStatistics_.totalLatencySamples++;
        globalStatistics_.averageLatency =
            globalStatistics_.totalLatencySum / static_cast<double>(globalStatistics_.totalLatencySamples);

        if (success) {
            globalStatistics_.commandsSucceeded++;
        } else {
            globalStatistics_.commandsFailed++;
        }
    }

    // Rule 6: Log error if command failed
    if (!success) {
        std::lock_guard<std::mutex> errLock(errorMutex_);
        errorLogs_[deviceId] = failureReason;
    }

    return success;
}

bool HardwareAbstractionLayer::ReadStatus(const std::string& deviceId, std::string& outStatus) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        outStatus = ToString(it->second.deviceState);
        return true;
    }
    outStatus = "UNKNOWN";
    return false;
}

DeviceState HardwareAbstractionLayer::ReadStatus(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        return it->second.deviceState;
    }
    return DeviceState::OFFLINE;
}

// =============================================================================
// Queries & Telemetry
// =============================================================================

bool HardwareAbstractionLayer::GetDevice(const std::string& deviceId, HardwareDevice& outDevice) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        outDevice = it->second;
        return true;
    }
    return false;
}

std::vector<HardwareDevice> HardwareAbstractionLayer::GetAllDevices() const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    std::vector<HardwareDevice> result;
    result.reserve(devices_.size());
    for (const auto& pair : devices_) {
        result.push_back(pair.second);
    }
    return result;
}

HardwareStatistics HardwareAbstractionLayer::GetStatistics() const {
    std::lock_guard<std::mutex> lock(statisticsMutex_);
    return globalStatistics_;
}

bool HardwareAbstractionLayer::GetDeviceStatistics(const std::string& deviceId,
                                                   HardwareStatistics& outStats) const {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = devices_.find(deviceId);
    if (it != devices_.end()) {
        outStats = it->second.statistics;
        return true;
    }
    return false;
}

// =============================================================================
// Structured ASCII Operational Reports (Rule 8)
// =============================================================================

std::string HardwareAbstractionLayer::GenerateHardwareReport() const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    std::vector<HardwareDevice> devs;
    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        devs.reserve(devices_.size());
        for (const auto& pair : devices_) {
            devs.push_back(pair.second);
        }
    }

    HardwareStatistics stats;
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        stats = globalStatistics_;
    }

    uint32_t onlineCount = 0;
    uint32_t offlineCount = 0;
    uint32_t initializedCount = 0;

    for (const auto& d : devs) {
        if (d.deviceState == DeviceState::OFFLINE) {
            offlineCount++;
        } else {
            onlineCount++;
        }
        if (d.isInitialized) {
            initializedCount++;
        }
    }

    std::ostringstream oss;
    oss << "[HARDWARE]\n\n"
        << "HAL Status:\nOPERATIONAL\n\n"
        << "Total Devices:\n" << devs.size() << "\n\n"
        << "Online:\n" << onlineCount << "\n\n"
        << "Offline:\n" << offlineCount << "\n\n"
        << "Initialized:\n" << initializedCount << "\n\n"
        << "Commands Sent:\n" << stats.commandsSent << "\n\n"
        << "Commands Succeeded:\n" << stats.commandsSucceeded << "\n\n"
        << std::fixed << std::setprecision(2)
        << "Average Latency:\n" << stats.averageLatency << " ms\n";

    return oss.str();
}

std::string HardwareAbstractionLayer::GenerateDeviceReport(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    HardwareDevice dev;
    bool found = false;

    if (!deviceId.empty()) {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = devices_.find(deviceId);
        if (it != devices_.end()) {
            dev = it->second;
            found = true;
        }
    } else {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        if (!devices_.empty()) {
            dev = devices_.begin()->second;
            found = true;
        }
    }

    std::ostringstream oss;
    oss << "[DEVICE]\n\n";

    if (found) {
        oss << "Device:\n" << dev.deviceId << "\n\n"
            << "Name:\n" << dev.deviceName << "\n\n"
            << "Type:\n" << ToString(dev.deviceType) << "\n\n"
            << "State:\n" << ToString(dev.deviceState) << "\n\n"
            << "Initialized:\n" << (dev.isInitialized ? "YES" : "NO") << "\n\n"
            << "Enabled:\n" << (dev.isEnabled ? "YES" : "NO") << "\n\n"
            << "Driver Loaded:\n" << (dev.driverLoaded ? "YES" : "NO") << "\n\n"
            << "Commands Sent:\n" << dev.statistics.commandsSent << "\n\n"
            << std::fixed << std::setprecision(2)
            << "Last Latency:\n" << dev.statistics.lastLatency << " ms\n";
    } else {
        oss << "Device:\nNONE\n\n"
            << "State:\nOFFLINE\n\n"
            << "Initialized:\nNO\n";
    }

    return oss.str();
}

std::string HardwareAbstractionLayer::GenerateDriverReport() const {
    std::lock_guard<std::mutex> lock(reportMutex_);

    uint32_t loadedCount = 0;
    std::set<std::string> busTypes;
    size_t totalDevs = 0;

    {
        std::lock_guard<std::mutex> dLock(driverMutex_);
        for (const auto& pair : driversLoaded_) {
            if (pair.second) {
                loadedCount++;
            }
        }
    }

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        totalDevs = devices_.size();
        for (const auto& pair : devices_) {
            busTypes.insert(ToString(pair.second.deviceType));
        }
    }

    std::ostringstream busStream;
    size_t bIdx = 0;
    for (const auto& b : busTypes) {
        if (bIdx++ > 0) busStream << ", ";
        busStream << b;
    }
    if (busTypes.empty()) {
        busStream << "NONE";
    }

    std::ostringstream oss;
    oss << "[DRIVER]\n\n"
        << "HAL Driver Architecture:\nSMART_AEGIS_HAL\n\n"
        << "Loaded Drivers:\n" << loadedCount << "\n\n"
        << "Total Devices:\n" << totalDevs << "\n\n"
        << "Driver Status:\nREADY\n\n"
        << "Active Bus Types:\n" << busStream.str() << "\n";

    return oss.str();
}

} // namespace hal
} // namespace aegis

