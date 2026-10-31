/**
 * @file IGpioController.hpp
 * @brief Common abstract interface for GPIO hardware and simulation controllers.
 *
 * Exposes a uniform, RAII-compliant LED control interface ensuring the Coordinator
 * and application services remain strictly decoupled from the underlying hardware backend.
 *
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::gpio {

class IGpioController {
public:
    virtual ~IGpioController() = default;

    /**
     * @brief Initialize GPIO controller hardware registers or simulation state.
     * @return true if initialized successfully; false otherwise.
     */
    virtual bool initialize() = 0;

    /**
     * @brief Safely release GPIO pins, turn off LEDs, and unmap registers.
     */
    virtual void shutdown() = 0;

    /**
     * @brief Control the Blue Emergency Corridor LED.
     * @param on True for ON (3.3V / High), False for OFF (0V / Low).
     * @return true if state applied successfully; false on hardware error.
     */
    virtual bool setBlue(bool on) = 0;

    /**
     * @brief Control the Red Stop LED.
     * @param on True for ON (3.3V / High), False for OFF (0V / Low).
     * @return true if state applied; false if not supported (e.g. Node 3) or on error.
     */
    virtual bool setRed(bool on) = 0;

    /**
     * @brief Read the current logical state of the Blue LED.
     * @return true if ON; false if OFF.
     */
    virtual bool getBlue() const = 0;

    /**
     * @brief Read the current logical state of the Red LED.
     * @return true if ON; false if OFF or not connected.
     */
    virtual bool getRed() const = 0;

    /**
     * @brief Execute an alert flash cycle on the Blue LED.
     * @param durationMs Duration of each ON/OFF phase in milliseconds.
     * @param count Number of flash cycles.
     * @return true if flash cycle executed; false on error.
     */
    virtual bool flashBlue(uint32_t durationMs = 250, uint32_t count = 3) = 0;

    /**
     * @brief Execute an alert flash cycle on the Red LED.
     * @param durationMs Duration of each ON/OFF phase in milliseconds.
     * @param count Number of flash cycles.
     * @return true if flash cycle executed; false if Red LED not supported or on error.
     */
    virtual bool flashRed(uint32_t durationMs = 250, uint32_t count = 3) = 0;

    /**
     * @brief Reset all connected LEDs to safe default OFF state.
     */
    virtual void reset() = 0;

    /**
     * @brief Check if this node physically possesses a Red LED.
     * @return false for Intersection 3 (Hardware shortage); true for Intersections 1 & 2.
     */
    virtual bool hasRedLed() const = 0;

    /**
     * @brief Check if this node physically possesses a Blue LED.
     * @return true across all nodes.
     */
    virtual bool hasBlueLed() const = 0;

    /**
     * @brief Get the Intersection numerical ID (1, 2, or 3).
     */
    virtual int getIntersectionId() const = 0;

    /**
     * @brief Get backend descriptor string (e.g., "QNX_HARDWARE_BCM2711" or "SIMULATION_IN_MEMORY").
     */
    virtual std::string getBackendType() const = 0;
};

} // namespace aegis::gpio
