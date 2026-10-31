/**
 * @file ProductionRelayDriver.hpp
 * @brief Refactored alias for PrototypeLedDriver in SMART AEGIS hardware architecture.
 * Relays and traffic modules are completely eliminated; replaced with direct GPIO LED control.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/led/PrototypeLedDriver.hpp"

namespace aegis::hardware_integration {

// Refactored to direct GPIO PrototypeLedDriver for backwards compatibility
using ProductionRelayDriver = PrototypeLedDriver;

} // namespace aegis::hardware_integration
