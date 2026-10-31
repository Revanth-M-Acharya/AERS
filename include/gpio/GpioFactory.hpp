/**
 * @file GpioFactory.hpp
 * @brief Factory for instantiating hardware or simulation GPIO controllers.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "gpio/IGpioController.hpp"
#include "gpio/SimulationGpioController.hpp"
#include "gpio/QnxHardwareGpioController.hpp"
#include <memory>

namespace aegis::gpio {

class GpioFactory {
public:
    static std::shared_ptr<IGpioController> Create(int intersectionId, bool forceSimulation = false) {
        if (forceSimulation) {
            return std::make_shared<SimulationGpioController>(intersectionId);
        }
#if defined(__QNX__) || defined(__QNXNTO__)
        return std::make_shared<QnxHardwareGpioController>(intersectionId);
#else
        // On macOS/Linux development hosts, default to simulation unless hardware is explicitly available
        return std::make_shared<SimulationGpioController>(intersectionId);
#endif
    }
};

} // namespace aegis::gpio
