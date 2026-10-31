/**
 * @file DemoHardwareSignals.hpp
 * @brief Standard symbolic signal identifiers and constants for Phase 12 Demonstration.
 * Eliminates all hardcoded GPIO pins across Coordinator and Intersection nodes.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"

namespace aegis::hardware_integration::demo {

/**
 * @brief Symbolic signal names for the physical demonstration hardware.
 */
struct DemoSignals {
    // Coordinator Pi Controls
    static constexpr const char* COORDINATOR_BUZZER     = "COORDINATOR_BUZZER";
    static constexpr const char* EMERGENCY_SWITCH       = "EMERGENCY_SWITCH";
    static constexpr const char* POLICE_OVERRIDE_SWITCH = "POLICE_OVERRIDE_SWITCH";
    static constexpr const char* STATUS_LED             = "COORDINATOR_STATUS_LED";
    static constexpr const char* OLED_SDA               = "OLED_I2C_SDA";
    static constexpr const char* OLED_SCL               = "OLED_I2C_SCL";

    // Intersection 1 (Road A & Road B, 1 Approach IR Sensor)
    static constexpr const char* INT1_ROAD_A_RED        = "INT1_ROAD_A_RED";
    static constexpr const char* INT1_ROAD_A_YELLOW     = "INT1_ROAD_A_YELLOW";
    static constexpr const char* INT1_ROAD_A_GREEN      = "INT1_ROAD_A_GREEN";
    static constexpr const char* INT1_ROAD_B_RED        = "INT1_ROAD_B_RED";
    static constexpr const char* INT1_ROAD_B_YELLOW     = "INT1_ROAD_B_YELLOW";
    static constexpr const char* INT1_ROAD_B_GREEN      = "INT1_ROAD_B_GREEN";
    static constexpr const char* INT1_APPROACH_IR       = "INT1_APPROACH_IR";

    // Intersection 2 (Road A & Road B, 2 Approach IR Sensors for Priority Scheduling)
    static constexpr const char* INT2_ROAD_A_RED        = "INT2_ROAD_A_RED";
    static constexpr const char* INT2_ROAD_A_YELLOW     = "INT2_ROAD_A_YELLOW";
    static constexpr const char* INT2_ROAD_A_GREEN      = "INT2_ROAD_A_GREEN";
    static constexpr const char* INT2_ROAD_B_RED        = "INT2_ROAD_B_RED";
    static constexpr const char* INT2_ROAD_B_YELLOW     = "INT2_ROAD_B_YELLOW";
    static constexpr const char* INT2_ROAD_B_GREEN      = "INT2_ROAD_B_GREEN";
    static constexpr const char* INT2_APPROACH_IR_TOP   = "INT2_APPROACH_IR_TOP";
    static constexpr const char* INT2_APPROACH_IR_RIGHT = "INT2_APPROACH_IR_RIGHT";

    // Intersection 3 (Road A & Road B, 1 Approach IR Sensor)
    static constexpr const char* INT3_ROAD_A_RED        = "INT3_ROAD_A_RED";
    static constexpr const char* INT3_ROAD_A_YELLOW     = "INT3_ROAD_A_YELLOW";
    static constexpr const char* INT3_ROAD_A_GREEN      = "INT3_ROAD_A_GREEN";
    static constexpr const char* INT3_ROAD_B_RED        = "INT3_ROAD_B_RED";
    static constexpr const char* INT3_ROAD_B_YELLOW     = "INT3_ROAD_B_YELLOW";
    static constexpr const char* INT3_ROAD_B_GREEN      = "INT3_ROAD_B_GREEN";
    static constexpr const char* INT3_APPROACH_IR       = "INT3_APPROACH_IR";
};

} // namespace aegis::hardware_integration::demo
