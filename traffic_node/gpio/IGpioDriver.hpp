/**
 * @file IGpioDriver.hpp
 * @brief Abstract GPIO driver interface isolating low-level board interactions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>

namespace aegis::traffic {

class IGpioDriver {
public:
    virtual ~IGpioDriver() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool ConfigureOutput(int pin) = 0;
    virtual bool ConfigureInput(int pin) = 0;
    virtual bool WritePin(int pin, bool high) = 0;
    virtual bool ReadPin(int pin) = 0;
    virtual std::string GetDriverName() const = 0;
};

} // namespace aegis::traffic

