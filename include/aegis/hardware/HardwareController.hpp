/**
 * @file HardwareController.hpp
 * @brief Public forwarding header for SMART AEGIS Phase 20 Physical Hardware Integration.
 */

#ifndef AEGIS_HARDWARE_HARDWARE_CONTROLLER_HPP
#define AEGIS_HARDWARE_HARDWARE_CONTROLLER_HPP

#include "../../src/hardware/HardwareController.h"
#include "../../src/hardware/HardwareConfiguration.h"
#include "../../src/hardware/RelayController.h"
#include "../../src/hardware/TrafficLightController.h"
#include "../../src/hardware/IntersectionHardware.h"
#include "../../src/hardware/WirelessNodeManager.h"
#include "../../src/hardware/HardwareHealthMonitor.h"
#include "../../src/hardware/HardwareDiagnostics.h"
#include "../../src/hardware/HardwareEventLogger.h"
#include "aegis/hardware/GpioReservationManager.hpp"
#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/hardware/WirelessQualityMonitor.hpp"
#include "aegis/hardware/BootManager.hpp"
#include "aegis/hardware/ShutdownManager.hpp"
#include "aegis/coordinator/NodeRegistrationManager.hpp"

#endif // AEGIS_HARDWARE_HARDWARE_CONTROLLER_HPP

