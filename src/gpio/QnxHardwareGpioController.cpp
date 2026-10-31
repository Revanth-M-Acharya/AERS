/**
 * @file QnxHardwareGpioController.cpp
 * @brief Implementation of QnxHardwareGpioController for Raspberry Pi 4 BCM2711 on QNX SDP 8.0.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "gpio/QnxHardwareGpioController.hpp"
#include <iostream>
#include <thread>
#include <chrono>

#if defined(__QNX__) || defined(__QNXNTO__)
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#define BCM2711_PERIPH_BASE 0xFE000000ULL
#define BCM2711_GPIO_OFFSET 0x00200000ULL
#define BCM2711_GPIO_BASE   (BCM2711_PERIPH_BASE + BCM2711_GPIO_OFFSET) // 0xFE200000
#define BCM2711_GPIO_SIZE   0x1000

// Register word offsets (each uint32_t is 4 bytes)
#define GPFSEL0_INDEX 0
#define GPSET0_INDEX  7   // 0x1C / 4
#define GPCLR0_INDEX  10  // 0x28 / 4
#define GPLEV0_INDEX  13  // 0x34 / 4
#endif

namespace aegis::gpio {

QnxHardwareGpioController::QnxHardwareGpioController(int intersectionId)
    : intersectionId_(intersectionId)
    , hasRed_(intersectionId != 3) // Node 3 has NO Red LED due to hardware shortage
    , hasBlue_(true)
    , pinRed_(5)
    , pinBlue_(6) {
}

QnxHardwareGpioController::~QnxHardwareGpioController() {
    shutdown();
}

bool QnxHardwareGpioController::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load()) {
        return true;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    // Request I/O privileges on QNX Neutrino microkernel
    if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
        std::cerr << "[WARN] QnxHardwareGpio: ThreadCtl(_NTO_TCTL_IO) failed. Running without direct I/O privileges." << std::endl;
    }

    gpioBaseAddr_ = mmap_device_memory(nullptr, BCM2711_GPIO_SIZE,
                                       PROT_READ | PROT_WRITE | PROT_NOCACHE,
                                       0, BCM2711_GPIO_BASE);
    if (gpioBaseAddr_ != MAP_FAILED && gpioBaseAddr_ != nullptr) {
        isHardwareMapped_.store(true);
        std::cout << "[HARDWARE_GPIO] QNX Neutrino: BCM2711 GPIO successfully mapped at virtual address "
                  << gpioBaseAddr_ << " (Phys: 0xFE200000)" << std::endl;
    } else {
        gpioBaseAddr_ = nullptr;
        isHardwareMapped_.store(false);
        std::cerr << "[WARN] QnxHardwareGpio: mmap_device_memory failed for BCM2711 GPIO. Engaging fallback emulation." << std::endl;
    }
#else
    gpioBaseAddr_ = nullptr;
    isHardwareMapped_.store(false);
    std::cout << "[HARDWARE_GPIO] Host platform is not QNX Neutrino; engaging cross-platform hardware emulation." << std::endl;
#endif

    // Configure outputs
    if (hasBlue_) {
        configurePinOutput(pinBlue_);
        writePinRaw(pinBlue_, false);
        blueState_.store(false);
    }

    if (hasRed_) {
        configurePinOutput(pinRed_);
        writePinRaw(pinRed_, false);
        redState_.store(false);
    } else {
        std::cout << "[HARDWARE_GPIO] Node " << intersectionId_
                  << ": Red LED disabled (hardware shortage specification)." << std::endl;
    }

    initialized_.store(true);
    return true;
}

void QnxHardwareGpioController::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.exchange(false)) {
        return;
    }

    // Extinguish all LEDs safely
    if (hasBlue_) {
        writePinRaw(pinBlue_, false);
        blueState_.store(false);
    }
    if (hasRed_) {
        writePinRaw(pinRed_, false);
        redState_.store(false);
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    if (isHardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
        munmap_device_memory(gpioBaseAddr_, BCM2711_GPIO_SIZE);
        gpioBaseAddr_ = nullptr;
    }
#endif
    isHardwareMapped_.store(false);
    std::cout << "[HARDWARE_GPIO] Node " << intersectionId_ << " shutdown cleanly. LEDs extinguished." << std::endl;
}

void QnxHardwareGpioController::configurePinOutput(int pin) {
    if (pin < 0 || pin > 27) return;

#if defined(__QNX__) || defined(__QNXNTO__)
    if (isHardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
        volatile uint32_t* gpfsel = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_) + (pin / 10);
        uint32_t shift = (pin % 10) * 3;
        uint32_t val = *gpfsel;
        val &= ~(7U << shift);
        val |= (1U << shift); // 001 = Output
        *gpfsel = val;
        return;
    }
#endif
}

void QnxHardwareGpioController::writePinRaw(int pin, bool high) {
    if (pin < 0 || pin > 27) return;

#if defined(__QNX__) || defined(__QNXNTO__)
    if (isHardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
        volatile uint32_t* regBase = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_);
        if (high) {
            regBase[GPSET0_INDEX] = (1U << pin);
        } else {
            regBase[GPCLR0_INDEX] = (1U << pin);
        }
        return;
    }
#endif
    (void)pin;
    (void)high;
}

bool QnxHardwareGpioController::readPinRaw(int pin) const {
    if (pin < 0 || pin > 27) return false;

#if defined(__QNX__) || defined(__QNXNTO__)
    if (isHardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
        volatile uint32_t* regBase = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_);
        return (regBase[GPLEV0_INDEX] & (1U << pin)) != 0;
    }
#endif
    if (pin == pinBlue_) return blueState_.load();
    if (pin == pinRed_) return redState_.load();
    return false;
}

bool QnxHardwareGpioController::setBlue(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    writePinRaw(pinBlue_, on);
    blueState_.store(on);
    std::cout << "[HARDWARE_GPIO] Node " << intersectionId_ << " | BCM" << pinBlue_
              << " (Header Pin 31) -> " << (on ? "HIGH (3.3V) [BLUE ON]" : "LOW (0V) [BLUE OFF]") << std::endl;
    return true;
}

bool QnxHardwareGpioController::setRed(bool on) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (!hasRed_) {
        // Safe no-op on Intersection 3
        std::cout << "[HARDWARE_GPIO] Node " << intersectionId_
                  << " | Red LED requested but omitted (hardware shortage specification)" << std::endl;
        return false;
    }

    writePinRaw(pinRed_, on);
    redState_.store(on);
    std::cout << "[HARDWARE_GPIO] Node " << intersectionId_ << " | BCM" << pinRed_
              << " (Header Pin 29) -> " << (on ? "HIGH (3.3V) [RED ON]" : "LOW (0V) [RED OFF]") << std::endl;
    return true;
}

bool QnxHardwareGpioController::getBlue() const {
    return blueState_.load();
}

bool QnxHardwareGpioController::getRed() const {
    if (!hasRed_) return false;
    return redState_.load();
}

bool QnxHardwareGpioController::flashBlue(uint32_t durationMs, uint32_t count) {
    if (!initialized_.load()) return false;
    for (uint32_t i = 0; i < count; ++i) {
        setBlue(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        setBlue(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }
    return true;
}

bool QnxHardwareGpioController::flashRed(uint32_t durationMs, uint32_t count) {
    if (!hasRed_ || !initialized_.load()) return false;
    for (uint32_t i = 0; i < count; ++i) {
        setRed(true);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
        setRed(false);
        std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));
    }
    return true;
}

void QnxHardwareGpioController::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (hasBlue_) {
        writePinRaw(pinBlue_, false);
        blueState_.store(false);
    }
    if (hasRed_) {
        writePinRaw(pinRed_, false);
        redState_.store(false);
    }
    std::cout << "[HARDWARE_GPIO] Node " << intersectionId_ << " | RESET complete. All LEDs OFF." << std::endl;
}

} // namespace aegis::gpio
