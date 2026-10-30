/**
 * @file DeploymentGpioDriver.h
 * @brief SMART AEGIS (AERS) Phase 16D - Low-Level BCM2711 GPIO Hardware Driver Adapter.
 *
 * Integrates the Hardware Abstraction Layer (HAL) and RaspberryPi4BSP with
 * physical/simulated Traffic LEDs, Pedestrian signals, IR vehicle sensors,
 * emergency override button, and future buzzer audio output.
 */

#ifndef AEGIS_DEPLOYMENT_GPIO_DRIVER_H
#define AEGIS_DEPLOYMENT_GPIO_DRIVER_H

#include "NodeConfiguration.h"
#include "bsp/RaspberryPi4BSP.h"
#include "hardware_abstraction/HardwareAbstractionLayer.h"
#include <mutex>
#include <string>
#include <vector>

namespace aegis {
namespace deployment {

class DeploymentGpioDriver {
public:
    DeploymentGpioDriver();
    ~DeploymentGpioDriver();

    // Non-copyable, non-movable
    DeploymentGpioDriver(const DeploymentGpioDriver&) = delete;
    DeploymentGpioDriver& operator=(const DeploymentGpioDriver&) = delete;

    /**
     * @brief Initialize all GPIO pins, configure modes, and set initial safe state.
     * @param mapping Pin mapping specifying pins for all signals and sensors.
     * @param simulation Whether to run in simulated mode or physical BCM2711 hardware.
     * @return true on success, false on reservation or initialization failure.
     */
    bool Initialize(const GPIOPinMapping& mapping, bool simulation = true);

    /**
     * @brief Read digital state from any configured GPIO pin.
     * @param pin GPIO pin number (0-27).
     * @param outState Output boolean (true = HIGH / 3.3V, false = LOW / 0V).
     * @return true on successful read; false otherwise.
     */
    bool Read(uint32_t pin, bool& outState);

    /**
     * @brief Write digital state to any configured output GPIO pin.
     * @param pin GPIO pin number (0-27).
     * @param state Boolean state to write.
     * @return true on successful write; false otherwise.
     */
    bool Write(uint32_t pin, bool state);

    /**
     * @brief Toggle digital state of an output GPIO pin.
     * @param pin GPIO pin number (0-27).
     * @return true on successful toggle; false otherwise.
     */
    bool Toggle(uint32_t pin);

    /**
     * @brief Cleanup, de-energize all lamps, return pins to safe state, and release reservations.
     * @return true on clean release; false otherwise.
     */
    bool Cleanup();

    // =========================================================================
    // High-Level Signal Operations
    // =========================================================================

    /**
     * @brief Actuate main intersection traffic lights.
     * @param state Signal state ("RED", "YELLOW", "GREEN", "FLASHING_YELLOW", "ALL_RED").
     */
    bool SetTrafficSignalState(const std::string& state);

    /**
     * @brief Actuate pedestrian signal lamps (Walk / Don't Walk).
     * @param walk true for Walk (White/Green), false for Don't Walk (Orange/Red).
     */
    bool SetPedestrianSignal(bool walk);

    /**
     * @brief Actuate acoustic warning buzzer.
     */
    bool SetBuzzer(bool active);

    /**
     * @brief Actuate emergency preemption strobe LED.
     */
    bool SetPreemptionStrobe(bool active);

    /**
     * @brief Read North-South IR vehicle detection sensor.
     */
    bool ReadIRSensorNorthSouth(bool& vehicleDetected);

    /**
     * @brief Read East-West IR vehicle detection sensor.
     */
    bool ReadIRSensorEastWest(bool& vehicleDetected);

    /**
     * @brief Read emergency manual override pushbutton.
     */
    bool ReadEmergencyOverrideButton(bool& buttonPressed);

    /**
     * @brief Check whether driver has been successfully initialized.
     */
    bool IsInitialized() const noexcept { return m_initialized; }

    /**
     * @brief Query current formatted status report of all lamps and sensors.
     */
    std::string GetStatusString() const;

    /**
     * @brief Get reference to internal BSP driver.
     */
    bsp::RaspberryPi4BSP& GetBSP() noexcept { return m_bsp; }

private:
    mutable std::mutex m_mutex;
    GPIOPinMapping m_mapping{};
    mutable bsp::RaspberryPi4BSP m_bsp;
    bool m_initialized{false};
    bool m_simulation{true};

    // Simulated sensor states for testing
    bool m_simulatedIrNS{false};
    bool m_simulatedIrEW{false};
    bool m_simulatedOverride{false};
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_GPIO_DRIVER_H