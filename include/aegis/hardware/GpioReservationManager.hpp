/**
 * @file GpioReservationManager.hpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Production BCM2711 GPIO Reservation Manager.
 *
 * Enforces strict ownership, duplicate allocation prevention, BCM pin validation,
 * and conflict detection across all hardware subsystems.
 */

#ifndef AEGIS_HARDWARE_GPIO_RESERVATION_MANAGER_HPP
#define AEGIS_HARDWARE_GPIO_RESERVATION_MANAGER_HPP

#include "bsp/RaspberryPi4BSP.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include <cstddef>

namespace aegis {
namespace hardware {

/**
 * @brief Record representing a reserved GPIO pin and its owning subsystem.
 */
struct GpioReservation {
    uint32_t pin{0};
    std::string owner;
    std::string purpose;
    bsp::PinMode direction{bsp::PinMode::OUTPUT};
    uint64_t timestampUs{0};
};

/**
 * @brief Production GPIO Reservation Manager maintaining an authoritative table
 * of BCM2711 pin allocations to guarantee mutual exclusion and collision-free wiring.
 */
class GpioReservationManager {
public:
    static GpioReservationManager& Instance();

    GpioReservationManager();
    ~GpioReservationManager();

    // Non-copyable, non-movable
    GpioReservationManager(const GpioReservationManager&) = delete;
    GpioReservationManager& operator=(const GpioReservationManager&) = delete;

    /**
     * @brief Validate that the given pin is a valid BCM2711 GPIO number (0..27).
     * @param pin Pin number to validate.
     * @return true if pin is within valid user GPIO range, false otherwise.
     */
    bool ValidateBcmPin(uint32_t pin) const noexcept;

    /**
     * @brief Reserve a GPIO pin for a specific subsystem and purpose.
     * @param pin BCM pin number (0..27).
     * @param owner Subsystem/class name requesting reservation (e.g. "RelayController").
     * @param purpose Description of pin function (e.g. "Road A Red").
     * @param direction Input or Output direction.
     * @return true if successfully reserved; false if pin is invalid or already reserved.
     */
    bool ReservePin(uint32_t pin, const std::string& owner, const std::string& purpose,
                    bsp::PinMode direction = bsp::PinMode::OUTPUT);

    /**
     * @brief Release a previously reserved GPIO pin.
     * @param pin The GPIO pin number to release.
     * @param owner The owner requesting release (must match reservation or be "SYSTEM").
     * @return true if successfully released; false if pin wasn't reserved or owner mismatch.
     */
    bool ReleasePin(uint32_t pin, const std::string& owner);

    /**
     * @brief Release all pins owned by the specified owner, or all pins if owner is empty/"SYSTEM".
     * @param owner Specific owner or empty for all.
     */
    void ReleaseAll(const std::string& owner = "");

    /**
     * @brief Check whether a pin is currently reserved.
     * @param pin The GPIO pin number.
     * @return true if reserved, false otherwise.
     */
    bool IsPinReserved(uint32_t pin) const noexcept;

    /**
     * @brief Get details of an active reservation.
     * @param pin The GPIO pin number.
     * @param outRes Output record populated if reserved.
     * @return true if found, false otherwise.
     */
    bool GetReservation(uint32_t pin, GpioReservation& outRes) const;

    /**
     * @brief Return a snapshot list of all active reservations.
     */
    std::vector<GpioReservation> GetAllReservations() const;

    /**
     * @brief Get the total number of actively reserved pins.
     */
    size_t GetReservedPinCount() const noexcept;

    /**
     * @brief Verify whether a set of pins can be safely reserved without conflicts.
     * @param pins List of pin numbers to test.
     * @param outConflictReason Diagnostic error string if conflict detected.
     * @return true if all pins are valid and free; false if any conflict or invalid.
     */
    bool CanReservePins(const std::vector<uint32_t>& pins, std::string& outConflictReason) const;

    /**
     * @brief Produce a formatted ASCII table of current reservations.
     */
    std::string GenerateDiagnostics() const;

    /**
     * @brief Clear all reservations (used during hard reset or between tests).
     */
    void Reset();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<uint32_t, GpioReservation> m_reservations;
    static constexpr uint32_t MAX_BCM_PIN = 27;
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_GPIO_RESERVATION_MANAGER_HPP
