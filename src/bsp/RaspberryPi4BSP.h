/**
 * @file RaspberryPi4BSP.h
 * @brief SMART AEGIS (AERS) Phase 16A - Raspberry Pi 4 Board Support Package (BSP) & Low-Level GPIO Driver Layer
 *
 * Implements the hardware abstraction and simulation layer for the Raspberry Pi 4 Model B
 * single-board computer, acting as the foundation driver layer immediately beneath the Phase 15B
 * QNX Device Driver Framework (QNXDeviceDriverFramework).
 *
 * Provides safe, deterministic abstractions for:
 * - Board lifecycle (initialization, shutdown, reset)
 * - Hardware telemetry (BCM2711 CPU, RAM, clock, temperature, core voltage)
 * - 58-pin GPIO controller management (Banks 0, 1, 2)
 * - Pin reservation and exclusive driver ownership
 * - Direction, pull-up/pull-down resistor configuration, and digital I/O
 * - Hardware interrupt event triggering and callback dispatching
 * - Hardware peripheral reservation (SPI0/1, I2C0/1, UART0/1, PWM0/1, Ethernet)
 *
 * Strict Compliance:
 * - Standard C++17 only.
 * - Deterministic, safety-critical execution.
 * - Thread-safe with exactly seven disjoint-scope mutexes (zero nested locks).
 * - Compatible with QNX SDP 8.0 & Raspberry Pi 4 Model B architecture.
 * - Zero third-party dependencies (STL only).
 * - Zero direct hardware register access (no mmap, no /dev/mem, no wiringPi, no pigpio).
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_RASPBERRY_PI4_BSP_H
#define AEGIS_RASPBERRY_PI4_BSP_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <memory>
#include <chrono>
#include <functional>
#include <array>

#if defined(__QNX__) || defined(__QNXNTO__)
#include <sys/neutrino.h>
#include <sys/mman.h>
#include <hw/inout.h>
#endif

namespace aegis {
namespace bsp {

// =============================================================================
// Enumerations
// =============================================================================

/**
 * @brief Hardware revisions of the Raspberry Pi 4 Model B board.
 */
enum class BoardRevision {
    RPI4_B_REV1_1,
    RPI4_B_REV1_2,
    RPI4_B_REV1_4,
    RPI4_B_REV1_5,
    UNKNOWN
};

/**
 * @brief GPIO controller banks on the BCM2711 SoC.
 */
enum class GPIOBank {
    BANK0, ///< GPIO 0 - 27 (Exposed on 40-pin header)
    BANK1, ///< GPIO 28 - 45
    BANK2, ///< GPIO 46 - 57
    UNKNOWN
};

/**
 * @brief Pin functional configuration modes.
 */
enum class PinMode {
    INPUT,
    OUTPUT,
    ALT0,
    ALT1,
    ALT2,
    ALT3,
    ALT4,
    ALT5,
    UNKNOWN
};

/**
 * @brief Digital logic levels on a GPIO pin.
 */
enum class PinState {
    LOW,
    HIGH,
    FLOATING,
    UNKNOWN
};

/**
 * @brief Signal edge or level triggering conditions for hardware interrupts.
 */
enum class InterruptTrigger {
    NONE,
    RISING_EDGE,
    FALLING_EDGE,
    BOTH_EDGES,
    HIGH_LEVEL,
    LOW_LEVEL
};

/**
 * @brief Internal pull-up and pull-down resistor configuration.
 */
enum class PullMode {
    NONE,
    PULL_DOWN,
    PULL_UP
};

/**
 * @brief Internal clock generator sources on the BCM2711 SoC.
 */
enum class ClockSource {
    OSCILLATOR_24MHZ,
    PLLC,
    PLLD,
    ARM_CLK,
    CORE_CLK,
    UNKNOWN
};

/**
 * @brief Power supply and thermal operational states.
 */
enum class PowerState {
    POWER_OFF,
    STANDBY,
    POWER_ON,
    LOW_POWER,
    OVERHEATING
};

/**
 * @brief Reservation and allocation states for hardware peripherals.
 */
enum class PeripheralState {
    UNRESERVED,
    RESERVED,
    ACTIVE,
    SUSPENDED,
    ERROR
};

// =============================================================================
// BCM2711 Hardware Register Constants (Raspberry Pi 4 Model B)
// =============================================================================

constexpr uintptr_t BCM2711_PERIPH_PHYS_BASE = 0xFE000000UL;
constexpr uintptr_t BCM2711_GPIO_OFFSET      = 0x00200000UL;
constexpr uintptr_t BCM2711_GPIO_PHYS_BASE   = BCM2711_PERIPH_PHYS_BASE + BCM2711_GPIO_OFFSET; // 0xFE200000
constexpr size_t    BCM2711_GPIO_BLOCK_SIZE  = 0x1000UL; // 4096 bytes

// Register Offsets relative to BCM2711_GPIO_PHYS_BASE
constexpr uintptr_t BCM2711_GPFSEL0          = 0x00UL; // GPIO 0-9 Function Select
constexpr uintptr_t BCM2711_GPFSEL1          = 0x04UL; // GPIO 10-19 Function Select
constexpr uintptr_t BCM2711_GPFSEL2          = 0x08UL; // GPIO 20-29 Function Select
constexpr uintptr_t BCM2711_GPFSEL3          = 0x0CUL; // GPIO 30-39 Function Select
constexpr uintptr_t BCM2711_GPFSEL4          = 0x10UL; // GPIO 40-49 Function Select
constexpr uintptr_t BCM2711_GPFSEL5          = 0x14UL; // GPIO 50-57 Function Select
constexpr uintptr_t BCM2711_GPSET0           = 0x1CUL; // GPIO 0-31 Output Set
constexpr uintptr_t BCM2711_GPSET1           = 0x20UL; // GPIO 32-57 Output Set
constexpr uintptr_t BCM2711_GPCLR0           = 0x28UL; // GPIO 0-31 Output Clear
constexpr uintptr_t BCM2711_GPCLR1           = 0x2CUL; // GPIO 32-57 Output Clear
constexpr uintptr_t BCM2711_GPLEV0           = 0x34UL; // GPIO 0-31 Pin Level
constexpr uintptr_t BCM2711_GPLEV1           = 0x38UL; // GPIO 32-57 Pin Level
constexpr uintptr_t BCM2711_GPPUPPDN0        = 0xE4UL; // GPIO 0-15 Pull-up/pull-down
constexpr uintptr_t BCM2711_GPPUPPDN1        = 0xE8UL; // GPIO 16-31 Pull-up/pull-down
constexpr uintptr_t BCM2711_GPPUPPDN2        = 0xECUL; // GPIO 32-47 Pull-up/pull-down
constexpr uintptr_t BCM2711_GPPUPPDN3        = 0xF0UL; // GPIO 48-57 Pull-up/pull-down

// =============================================================================
// String Conversion Helpers
// =============================================================================

std::string ToString(BoardRevision revision);
std::string ToString(GPIOBank bank);
std::string ToString(PinMode mode);
std::string ToString(PinState state);
std::string ToString(InterruptTrigger trigger);
std::string ToString(PullMode pull);
std::string ToString(ClockSource clock);
std::string ToString(PowerState power);
std::string ToString(PeripheralState peripheral);

BoardRevision BoardRevisionFromString(const std::string& str);
GPIOBank GPIOBankFromString(const std::string& str);
PinMode PinModeFromString(const std::string& str);
PinState PinStateFromString(const std::string& str);
InterruptTrigger InterruptTriggerFromString(const std::string& str);
PullMode PullModeFromString(const std::string& str);
ClockSource ClockSourceFromString(const std::string& str);
PowerState PowerStateFromString(const std::string& str);
PeripheralState PeripheralStateFromString(const std::string& str);

// =============================================================================
// Data Structures
// =============================================================================

/**
 * @brief Detailed hardware information and live sensor telemetry for the Raspberry Pi 4.
 */
struct BoardInformation {
    BoardRevision boardRevision{BoardRevision::RPI4_B_REV1_4};
    std::string serialNumber{"10000000a1b2c3d4"};
    std::string cpuModel{"Broadcom BCM2711, Quad-core Cortex-A72 (ARM v8) 64-bit SoC @ 1.8GHz"};
    uint64_t ramSizeMB{4096};
    std::string bootloaderVersion{"v2026.01-bcm2711"};
    std::string firmwareVersion{"1.20260210"};
    double temperature{45.0};             ///< Celsius
    double voltage{1.20};                 ///< Volts
    uint64_t clockFrequencyHz{1800000000ULL}; ///< Core clock (1.8 GHz)
    PowerState currentPowerState{PowerState::POWER_ON};

    BoardInformation() = default;
};

/**
 * @brief Representation of an individual BCM2711 GPIO pin.
 */
struct GPIOPin {
    uint32_t pinNumber{0};
    GPIOBank bank{GPIOBank::BANK0};
    PinMode mode{PinMode::INPUT};
    PullMode pullConfiguration{PullMode::NONE};
    PinState currentState{PinState::LOW};
    bool interruptEnabled{false};
    InterruptTrigger interruptTrigger{InterruptTrigger::NONE};
    bool reservationStatus{false};
    std::string ownerDriver{""};
    uint64_t lastUpdateTime{0};
    std::string lastError{""};

    GPIOPin() = default;

    explicit GPIOPin(uint32_t pin)
        : pinNumber(pin),
          bank(pin <= 27 ? GPIOBank::BANK0 : (pin <= 45 ? GPIOBank::BANK1 : GPIOBank::BANK2)),
          mode(PinMode::INPUT),
          pullConfiguration(PullMode::NONE),
          currentState(PinState::LOW),
          interruptEnabled(false),
          interruptTrigger(InterruptTrigger::NONE),
          reservationStatus(false),
          ownerDriver(""),
          lastUpdateTime(0),
          lastError("") {}
};

/**
 * @brief Operational statistics and latency telemetry for the BSP layer.
 */
struct BSPStatistics {
    uint64_t gpioReads{0};
    uint64_t gpioWrites{0};
    uint64_t gpioToggles{0};
    uint64_t interruptCount{0};
    uint64_t reservedPins{0};
    uint64_t configuredPins{0};
    double averageReadLatency{0.0};  ///< Milliseconds
    double averageWriteLatency{0.0}; ///< Milliseconds

    // Internal accounting
    uint64_t totalReadSamples{0};
    double totalReadSum{0.0};
    uint64_t totalWriteSamples{0};
    double totalWriteSum{0.0};

    BSPStatistics() = default;
};

/**
 * @brief Callback signature for edge/level interrupt dispatching.
 */
using InterruptCallback = std::function<void(uint32_t pin, PinState state, uint64_t timestamp)>;

// =============================================================================
// RaspberryPi4BSP Class
// =============================================================================

/**
 * @brief Master Board Support Package (BSP) abstraction for the Raspberry Pi 4 Model B.
 */
class RaspberryPi4BSP {
public:
    RaspberryPi4BSP();
    ~RaspberryPi4BSP();

    // Non-copyable, non-movable to guarantee strict thread safety
    RaspberryPi4BSP(const RaspberryPi4BSP&) = delete;
    RaspberryPi4BSP& operator=(const RaspberryPi4BSP&) = delete;
    RaspberryPi4BSP(RaspberryPi4BSP&&) = delete;
    RaspberryPi4BSP& operator=(RaspberryPi4BSP&&) = delete;

    // -------------------------------------------------------------------------
    // Board Lifecycle
    // -------------------------------------------------------------------------
    bool InitializeBoard(BoardRevision revision = BoardRevision::RPI4_B_REV1_4);
    bool ShutdownBoard();
    bool ResetBoard();
    bool IsBoardInitialized() const;

    // -------------------------------------------------------------------------
    // Board Information & Telemetry
    // -------------------------------------------------------------------------
    BoardInformation ReadBoardInformation() const;
    double ReadTemperature() const;
    double ReadVoltage() const;
    uint64_t ReadClock(ClockSource source = ClockSource::CORE_CLK) const;

    // -------------------------------------------------------------------------
    // GPIO Reservation Interface
    // -------------------------------------------------------------------------
    bool ReserveGPIO(uint32_t pin, const std::string& ownerDriver);
    bool ReleaseGPIO(uint32_t pin);
    bool IsGPIOReserved(uint32_t pin) const;

    // -------------------------------------------------------------------------
    // Pin Configuration Interface
    // -------------------------------------------------------------------------
    bool ConfigurePinMode(uint32_t pin, PinMode mode);
    bool ConfigurePullResistor(uint32_t pin, PullMode pull);

    // -------------------------------------------------------------------------
    // GPIO Read / Write / Toggle Interface
    // -------------------------------------------------------------------------
    bool WriteGPIO(uint32_t pin, PinState state);
    bool ReadGPIO(uint32_t pin, PinState& outState);
    PinState ReadGPIO(uint32_t pin);
    bool ToggleGPIO(uint32_t pin);

    // -------------------------------------------------------------------------
    // Interrupt Interface
    // -------------------------------------------------------------------------
    bool EnableInterrupt(uint32_t pin, InterruptTrigger trigger);
    bool DisableInterrupt(uint32_t pin);
    bool RegisterInterruptCallback(uint32_t pin, InterruptCallback callback);
    bool RemoveInterruptCallback(uint32_t pin);
    bool SimulateInterrupt(uint32_t pin, PinState state);

    // -------------------------------------------------------------------------
    // Peripheral Reservation Interface
    // -------------------------------------------------------------------------
    bool ReservePeripheral(const std::string& peripheralName, const std::string& ownerDriver);
    bool ReleasePeripheral(const std::string& peripheralName);
    bool IsPeripheralReserved(const std::string& peripheralName) const;

    // -------------------------------------------------------------------------
    // Status & Telemetry Queries
    // -------------------------------------------------------------------------
    bool GetPin(uint32_t pin, GPIOPin& outPin) const;
    std::vector<GPIOPin> GetAllPins() const;
    BSPStatistics GetStatistics() const;
    std::string GetLastError(const std::string& context = "") const;

    // -------------------------------------------------------------------------
    // Phase 1 Real Hardware Register & Standard GPIO Interface
    // -------------------------------------------------------------------------
    bool SetPin(uint32_t pin, PinState state);
    bool SetPin(uint32_t pin, bool high);
    PinState GetPin(uint32_t pin) const;
    bool ConfigurePin(uint32_t pin, PinMode mode);
    bool TogglePin(uint32_t pin);
    bool Cleanup();

    // -------------------------------------------------------------------------
    // Structured ASCII Operational Reports
    // -------------------------------------------------------------------------
    std::string GenerateBoardReport() const;
    std::string GenerateGPIOReport(uint32_t pin = 0xFFFFFFFF) const;
    std::string GenerateInterruptReport(uint32_t pin = 0xFFFFFFFF) const;

    // -------------------------------------------------------------------------
    // Lifecycle Reset
    // -------------------------------------------------------------------------
    void Reset();

private:
    static uint64_t GetCurrentTimeMs();
    void RecordError(const std::string& context, const std::string& message);

    // Exactly seven mutexes as strictly specified:
    mutable std::mutex boardMutex_;
    mutable std::mutex gpioMutex_;
    mutable std::mutex interruptMutex_;
    mutable std::mutex reservationMutex_;
    mutable std::mutex statisticsMutex_;
    mutable std::mutex reportMutex_;
    mutable std::mutex errorMutex_;

    // Board state
    bool isInitialized_{false};
    BoardInformation boardInfo_;

#if defined(__QNX__) || defined(__QNXNTO__)
    // Real QNX BCM2711 Hardware State (No std::map, no software-only pin states)
    struct QnxPinReservationRecord {
        bool reserved{false};
        char ownerDriver[64]{0};
        PinMode mode{PinMode::INPUT};
        PullMode pull{PullMode::NONE};
        bool interruptEnabled{false};
        InterruptTrigger interruptTrigger{InterruptTrigger::NONE};
        uint64_t lastUpdateTime{0};
    };
    std::array<QnxPinReservationRecord, 58> qnxReservations_{};
    uintptr_t gpioBase_{0};
    bool isMapped_{false};
    bool isMappedMemory_{false};

    PinMode ReadHardwarePinMode(uint32_t pin) const;
    PinState ReadHardwarePinState(uint32_t pin) const;
    bool WriteHardwarePinState(uint32_t pin, PinState state);
    bool ConfigureHardwarePinMode(uint32_t pin, PinMode mode);
    bool ConfigureHardwarePull(uint32_t pin, PullMode pull);
#else
    // GPIO pins table (pins 0 to 57 supported on BCM2711) - simulation mode
    std::unordered_map<uint32_t, GPIOPin> pins_;
#endif

    // Interrupt callbacks (pin -> callback)
    std::unordered_map<uint32_t, InterruptCallback> interruptCallbacks_;

    // Peripheral reservations (peripheralName -> ownerDriver)
    std::unordered_map<std::string, std::string> reservedPeripherals_;

    // Aggregated statistics
    BSPStatistics stats_;

    // Forensic error logs (context -> error description)
    std::unordered_map<std::string, std::string> errorLogs_;
    std::string lastError_;
};

} // namespace bsp
} // namespace aegis

#endif // AEGIS_RASPBERRY_PI4_BSP_H
