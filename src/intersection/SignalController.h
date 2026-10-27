/**
 * @file SignalController.h
 * @brief Software representation of a roadside intersection traffic signal.
 *
 * Tracks signal state (RED, YELLOW, GREEN) and operating mode (NORMAL, EMERGENCY).
 * Hardware-free and thread-safe.
 */

#ifndef AEGIS_INTERSECTION_SIGNAL_CONTROLLER_H
#define AEGIS_INTERSECTION_SIGNAL_CONTROLLER_H

#include <string>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace intersection {

/**
 * @brief Discrete operational states of the software traffic signal.
 */
enum class SignalState : uint8_t {
    RED       = 0, ///< Stop / Clearance phase
    YELLOW    = 1, ///< Caution transition phase
    GREEN     = 2, ///< Preempted green corridor phase
    NORMAL    = 3, ///< Standard cyclical operational mode
    EMERGENCY = 4  ///< High-priority emergency preemption mode
};

/**
 * @brief Convert SignalState enum to human-readable string.
 */
const char* SignalStateToString(SignalState state) noexcept;

/**
 * @brief Thread-safe software signal controller governing signal states and modes.
 */
class SignalController {
public:
    SignalController();
    ~SignalController() = default;

    // Non-copyable, movable
    SignalController(const SignalController&) = delete;
    SignalController& operator=(const SignalController&) = delete;
    SignalController(SignalController&&) noexcept;
    SignalController& operator=(SignalController&&) noexcept;

    // =========================================================================
    // State Transitions
    // =========================================================================

    /**
     * @brief Transition signal lamp to GREEN.
     */
    void SetGreen();

    /**
     * @brief Transition signal lamp to RED.
     */
    void SetRed();

    /**
     * @brief Transition signal lamp to YELLOW.
     */
    void SetYellow();

    /**
     * @brief Restore NORMAL cyclical mode and clear emergency preemption.
     */
    void SetNormal();

    /**
     * @brief Activate EMERGENCY preemption mode.
     */
    void EnableEmergency();

    /**
     * @brief Deactivate EMERGENCY preemption mode and restore standard mode.
     */
    void DisableEmergency();

    // =========================================================================
    // Status Inquiries
    // =========================================================================

    /**
     * @brief Get the current primary signal state or mode.
     */
    SignalState CurrentState() const;

    /**
     * @brief Check whether the controller is actively in EMERGENCY mode.
     */
    bool IsEmergency() const;

    /**
     * @brief Check whether the controller is operating in NORMAL cyclical mode.
     */
    bool IsNormal() const;

    /**
     * @brief Get string description of the current signal state.
     */
    std::string StateString() const;

private:
    mutable std::mutex m_mutex{};
    SignalState m_state{SignalState::RED};
    bool m_isEmergency{false};
};

} // namespace intersection
} // namespace aegis

#endif // AEGIS_INTERSECTION_SIGNAL_CONTROLLER_H
