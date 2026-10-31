/**
 * @file TrafficState.hpp
 * @brief State definitions and enums for deterministic traffic signal FSM.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>

namespace aegis::traffic {

/**
 * @brief Discrete traffic signal lamp states.
 */
enum class SignalState {
    RED,
    RED_YELLOW,
    GREEN,
    YELLOW,
    ALL_RED,
    FLASHING_YELLOW
};

/**
 * @brief High-level operational mode of the traffic intersection.
 */
enum class OperationalMode {
    NORMAL_MODE,
    EMERGENCY_MODE,
    FAILSAFE_MODE
};

inline std::string SignalStateToString(SignalState state) {
    switch (state) {
        case SignalState::RED:             return "RED";
        case SignalState::RED_YELLOW:      return "RED_YELLOW";
        case SignalState::GREEN:           return "GREEN";
        case SignalState::YELLOW:          return "YELLOW";
        case SignalState::ALL_RED:         return "ALL_RED";
        case SignalState::FLASHING_YELLOW: return "FLASHING_YELLOW";
        default:                           return "UNKNOWN";
    }
}

inline std::string OperationalModeToString(OperationalMode mode) {
    switch (mode) {
        case OperationalMode::NORMAL_MODE:    return "NORMAL_MODE";
        case OperationalMode::EMERGENCY_MODE: return "EMERGENCY_MODE";
        case OperationalMode::FAILSAFE_MODE:  return "FAILSAFE_MODE";
        default:                              return "UNKNOWN";
    }
}

} // namespace aegis::traffic

