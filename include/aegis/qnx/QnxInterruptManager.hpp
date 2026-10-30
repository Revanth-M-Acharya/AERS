/**
 * @file QnxInterruptManager.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - GPIO Interrupt-Driven Event Management.
 *
 * Implements native QNX interrupt attachment:
 * - InterruptAttachEvent() with SIGEV_PULSE binding
 * - InterruptDetach(), InterruptMask(), InterruptUnmask()
 * - Hardware line mappings on Raspberry Pi 4 (BCM2711):
 *   - GPIO 12: IR approach sensor
 *   - GPIO 13: IR departure sensor
 *   - GPIO 16: Emergency manual button
 *   - GPIO 24: Relay fault monitor
 * - Real-time event counter and latency telemetry
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_INTERRUPT_MANAGER_HPP
#define AEGIS_QNX_INTERRUPT_MANAGER_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <sys/neutrino.h>
    #include <sys/siginfo.h>
#endif

namespace aegis {
namespace qnx {

namespace GpioLine {
    constexpr int IR_APPROACH   = 12; ///< BCM 12: Vehicle entrance detection
    constexpr int IR_DEPARTURE  = 13; ///< BCM 13: Vehicle clear detection
    constexpr int EMERGENCY_BTN = 16; ///< BCM 16: Manual field emergency trigger
    constexpr int RELAY_FAULT   = 24; ///< BCM 24: Hardware relay feedback monitor
}

inline const char* GpioLineToString(int pin) noexcept {
    switch (pin) {
        case GpioLine::IR_APPROACH:   return "IR_APPROACH (GPIO 12)";
        case GpioLine::IR_DEPARTURE:  return "IR_DEPARTURE (GPIO 13)";
        case GpioLine::EMERGENCY_BTN: return "EMERGENCY_BTN (GPIO 16)";
        case GpioLine::RELAY_FAULT:   return "RELAY_FAULT (GPIO 24)";
        default:                      return "UNKNOWN_GPIO";
    }
}

enum class InterruptState {
    DETACHED,
    ATTACHED,
    MASKED
};

inline const char* InterruptStateToString(InterruptState state) noexcept {
    switch (state) {
        case InterruptState::DETACHED: return "DETACHED";
        case InterruptState::ATTACHED: return "ATTACHED";
        case InterruptState::MASKED:   return "MASKED";
        default:                       return "UNKNOWN";
    }
}

struct InterruptInfo {
    int interruptId{0};
    int gpioPin{0};
    int coid{0};
    int8_t pulseCode{0};
    int32_t pulseValue{0};
    int priority{25};
    InterruptState state{InterruptState::DETACHED};
    uint64_t triggerCount{0};
    uint64_t lastTriggerTimestampUs{0};
};

/**
 * @brief Manages QNX hardware interrupt attachment, masking, and event telemetry.
 */
class QnxInterruptManager {
public:
    QnxInterruptManager();
    ~QnxInterruptManager();

    // Prevent copying
    QnxInterruptManager(const QnxInterruptManager&) = delete;
    QnxInterruptManager& operator=(const QnxInterruptManager&) = delete;

    /**
     * @brief Attaches a GPIO interrupt line to a connection ID via SIGEV_PULSE.
     * @param gpioPin BCM GPIO pin number (12, 13, 16, 24).
     * @param coid Target channel connection ID.
     * @param pulseCode Application pulse code to deliver.
     * @param pulseValue 32-bit payload value.
     * @param priority Delivery priority (default 25).
     * @return Positive interrupt identifier on success, negative on error.
     */
    int AttachGpioInterrupt(int gpioPin, int coid, int8_t pulseCode, int32_t pulseValue, int priority = 25);

    /**
     * @brief Detaches a previously attached interrupt.
     * @param interruptId Interrupt identifier returned by AttachGpioInterrupt.
     * @return 0 on success, negative on error.
     */
    int DetachInterrupt(int interruptId);

    /**
     * @brief Masks an interrupt line to temporarily prevent notifications.
     */
    int MaskInterrupt(int interruptId);

    /**
     * @brief Unmasks an interrupt line to resume notifications.
     */
    int UnmaskInterrupt(int interruptId);

    /**
     * @brief Simulates a hardware interrupt trigger on the given GPIO pin.
     * Useful for automated tests and non-hardware environments.
     * @param gpioPin Target BCM pin.
     * @return 0 on success, -1 if pin not attached or masked.
     */
    int SimulateInterrupt(int gpioPin);

    /**
     * @brief Retrieves metadata for a specific interrupt.
     */
    bool GetInterruptInfo(int interruptId, InterruptInfo& outInfo) const;

    /**
     * @brief Retrieves metadata by GPIO pin.
     */
    bool GetInterruptInfoByPin(int gpioPin, InterruptInfo& outInfo) const;

    /**
     * @brief Retrieves a list of all registered interrupt lines.
     */
    std::vector<InterruptInfo> GetAllInterrupts() const;

    /**
     * @brief Returns total trigger count across all interrupt lines.
     */
    uint64_t GetTotalTriggerCount() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<int, InterruptInfo> m_interruptsById;
    std::unordered_map<int, int> m_pinToId;
    int m_nextInterruptId{10};
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_INTERRUPT_MANAGER_HPP

