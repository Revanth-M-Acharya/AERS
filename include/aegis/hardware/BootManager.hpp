/**
 * @file BootManager.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Production Hardware Deterministic Boot Sequence.
 *
 * Implements the rigorous 10-step production startup pipeline:
 * 1. BSP Initialization
 * 2. GPIO Reservation
 * 3. Relay Self Test (POST)
 * 4. Hardware Diagnostics
 * 5. Wi-Fi Startup
 * 6. Node Registration
 * 7. Coordinator Handshake
 * 8. Watchdog Activation
 * 9. Hardware Ready
 * 10. Accept Traffic Commands
 *
 * If any step fails, halts the boot sequence, logs the fault, and triggers safe mode (ALL_RED).
 */

#ifndef AEGIS_HARDWARE_BOOT_MANAGER_HPP
#define AEGIS_HARDWARE_BOOT_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class HardwareBootStep : uint8_t {
    POWER_ON = 0,
    BSP_INITIALIZATION = 1,
    GPIO_RESERVATION = 2,
    RELAY_SELF_TEST = 3,
    HARDWARE_DIAGNOSTICS = 4,
    WIFI_STARTUP = 5,
    NODE_REGISTRATION = 6,
    COORDINATOR_HANDSHAKE = 7,
    WATCHDOG_STARTUP = 8,
    HARDWARE_READY = 9,
    ACCEPT_TRAFFIC_COMMANDS = 10,
    BOOT_COMPLETED = 11,
    BOOT_FAILED = 12
};

/**
 * @brief SMART AEGIS Phase 2 - Granular 14-Stage Industrial Startup Sequence.
 */
enum class IndustrialBootStep : uint8_t {
    POWER_ON = 0,
    KERNEL_BOOT = 1,
    INIT_BSP = 2,
    INIT_GPIO = 3,
    RESERVE_GPIO = 4,
    INIT_RELAY_CONTROLLER = 5,
    RUN_RELAY_POST = 6,
    INIT_IR_SENSORS = 7,
    INIT_NETWORK_INTERFACE = 8,
    VERIFY_NETWORK_LINK = 9,
    REGISTER_WITH_COORDINATOR = 10,
    START_HARDWARE_WATCHDOG = 11,
    START_HEARTBEAT = 12,
    ENTER_OPERATIONAL_MODE = 13,
    READY = 14,
    BOOT_FAILED = 15
};

inline std::string IndustrialBootStepToString(IndustrialBootStep step) {
    switch (step) {
        case IndustrialBootStep::POWER_ON:                  return "POWER_ON";
        case IndustrialBootStep::KERNEL_BOOT:               return "1_KERNEL_BOOT";
        case IndustrialBootStep::INIT_BSP:                  return "2_INIT_BSP";
        case IndustrialBootStep::INIT_GPIO:                 return "3_INIT_GPIO";
        case IndustrialBootStep::RESERVE_GPIO:              return "4_RESERVE_GPIO";
        case IndustrialBootStep::INIT_RELAY_CONTROLLER:     return "5_INIT_RELAY_CONTROLLER";
        case IndustrialBootStep::RUN_RELAY_POST:            return "6_RUN_RELAY_POST";
        case IndustrialBootStep::INIT_IR_SENSORS:           return "7_INIT_IR_SENSORS";
        case IndustrialBootStep::INIT_NETWORK_INTERFACE:    return "8_INIT_NETWORK_INTERFACE";
        case IndustrialBootStep::VERIFY_NETWORK_LINK:       return "9_VERIFY_NETWORK_LINK";
        case IndustrialBootStep::REGISTER_WITH_COORDINATOR: return "10_REGISTER_WITH_COORDINATOR";
        case IndustrialBootStep::START_HARDWARE_WATCHDOG:   return "11_START_HARDWARE_WATCHDOG";
        case IndustrialBootStep::START_HEARTBEAT:           return "12_START_HEARTBEAT";
        case IndustrialBootStep::ENTER_OPERATIONAL_MODE:    return "13_ENTER_OPERATIONAL_MODE";
        case IndustrialBootStep::READY:                     return "14_READY";
        case IndustrialBootStep::BOOT_FAILED:               return "BOOT_FAILED";
        default:                                            return "UNKNOWN";
    }
}

inline std::string HardwareBootStepToString(HardwareBootStep step) {
    switch (step) {
        case HardwareBootStep::POWER_ON:                return "POWER_ON";
        case HardwareBootStep::BSP_INITIALIZATION:      return "1_BSP_INITIALIZATION";
        case HardwareBootStep::GPIO_RESERVATION:        return "2_GPIO_RESERVATION";
        case HardwareBootStep::RELAY_SELF_TEST:         return "3_RELAY_SELF_TEST";
        case HardwareBootStep::HARDWARE_DIAGNOSTICS:    return "4_HARDWARE_DIAGNOSTICS";
        case HardwareBootStep::WIFI_STARTUP:            return "5_WIFI_STARTUP";
        case HardwareBootStep::NODE_REGISTRATION:       return "6_NODE_REGISTRATION";
        case HardwareBootStep::COORDINATOR_HANDSHAKE:   return "7_COORDINATOR_HANDSHAKE";
        case HardwareBootStep::WATCHDOG_STARTUP:        return "8_WATCHDOG_STARTUP";
        case HardwareBootStep::HARDWARE_READY:          return "9_HARDWARE_READY";
        case HardwareBootStep::ACCEPT_TRAFFIC_COMMANDS: return "10_ACCEPT_TRAFFIC_COMMANDS";
        case HardwareBootStep::BOOT_COMPLETED:          return "BOOT_COMPLETED";
        case HardwareBootStep::BOOT_FAILED:             return "BOOT_FAILED";
        default:                                        return "UNKNOWN";
    }
}

struct HardwareBootStepReport {
    HardwareBootStep step{HardwareBootStep::POWER_ON};
    std::string stepName;
    bool passed{false};
    uint64_t durationUs{0};
    std::string details;
};

/**
 * @brief Production Hardware Boot Manager executing deterministic 10-step startup.
 */
class BootManager {
public:
    using StepFunction = std::function<bool(std::string& outDetails)>;
    using SafeModeHandler = std::function<void(const std::string& reason)>;

    BootManager();
    ~BootManager() = default;

    // Non-copyable, non-movable
    BootManager(const BootManager&) = delete;
    BootManager& operator=(const BootManager&) = delete;

    /**
     * @brief Reset boot manager back to initial POWER_ON state.
     */
    void Reset();

    /**
     * @brief Register custom step executor lambda for any of the 10 boot steps.
     */
    void RegisterStepExecutor(HardwareBootStep step, StepFunction func);

    /**
     * @brief Register safe mode callback to invoke immediately if boot fails.
     */
    void SetSafeModeHandler(SafeModeHandler handler);

    /**
     * @brief Execute a single specific boot step sequentially.
     */
    bool ExecuteStep(HardwareBootStep targetStep, StepFunction customFunc = nullptr);

    /**
     * @brief Run the complete 10-step production boot sequence deterministically.
     * @return true if all 10 steps succeed and node is ready to accept traffic commands;
     *         false if any step fails (automatically invokes safe mode).
     */
    bool RunFullBootSequence();

    /**
     * @brief Register custom step executor lambda for any of the 14 industrial boot steps (Phase 2).
     */
    void RegisterIndustrialStepExecutor(IndustrialBootStep step, StepFunction func);

    /**
     * @brief Execute a single specific industrial boot step sequentially (Phase 2).
     */
    bool ExecuteIndustrialStep(IndustrialBootStep targetStep, StepFunction customFunc = nullptr);

    /**
     * @brief Run the full 14-stage Phase 2 industrial startup sequence:
     * Kernel Boot -> Initialize BSP -> Initialize GPIO -> Reserve GPIO ->
     * Initialize Relay Controller -> Run Relay POST -> Initialize IR Sensors ->
     * Initialize Network Interface -> Verify Link -> Register with Coordinator ->
     * Start Hardware Watchdog -> Start Heartbeat -> Enter Operational Mode -> READY.
     * @return true if all 14 steps succeed and node enters READY state;
     *         false if any step fails (automatically invokes safe mode).
     */
    bool RunIndustrialBootSequence();

    // State Queries
    HardwareBootStep GetCurrentStep() const noexcept;
    IndustrialBootStep GetCurrentIndustrialStep() const noexcept;
    bool IsBootCompleted() const noexcept;
    bool IsBootFailed() const noexcept;
    bool IsInSafeMode() const noexcept;

    // Boot Timing Measurements
    uint64_t GetTotalBootDurationUs() const noexcept;
    double GetTotalBootDurationMs() const noexcept;

    std::vector<HardwareBootStepReport> GetStepReports() const;
    std::string GenerateBootReport() const;
    std::string GenerateIndustrialBootReport() const;

private:
    void EnterSafeMode(const std::string& failureReason);

    mutable std::mutex m_mutex;
    std::atomic<HardwareBootStep> m_currentStep{HardwareBootStep::POWER_ON};
    std::atomic<IndustrialBootStep> m_currentIndustrialStep{IndustrialBootStep::POWER_ON};
    std::atomic<bool> m_inSafeMode{false};
    std::atomic<uint64_t> m_totalBootDurationUs{0};
    std::vector<HardwareBootStepReport> m_reports;

    std::unordered_map<uint8_t, StepFunction> m_stepExecutors;
    std::unordered_map<uint8_t, StepFunction> m_industrialStepExecutors;
    SafeModeHandler m_safeModeHandler{nullptr};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_BOOT_MANAGER_HPP
