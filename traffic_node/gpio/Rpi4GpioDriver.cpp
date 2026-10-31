/**
 * @file Rpi4GpioDriver.cpp
 * @brief Implementation of Raspberry Pi 4 BCM2711 GPIO driver for QNX SDP 8.0.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "traffic_node/gpio/Rpi4GpioDriver.hpp"
#include <iostream>

#if defined(__QNX__) || defined(__QNXNTO__)
#include <sys/mman.h>
#include <sys/neutrino.h>
#include <hw/inout.h>
#define BCM2711_GPIO_BASE 0xFE200000ULL
#define BCM2711_GPIO_SIZE 0x1000
#endif

namespace aegis::traffic {

Rpi4GpioDriver::Rpi4GpioDriver() = default;

Rpi4GpioDriver::~Rpi4GpioDriver() {
    Shutdown();
}

bool Rpi4GpioDriver::Initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_.load()) {
        return true;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    // Request I/O privileges on QNX Neutrino
    ThreadCtl(_NTO_TCTL_IO, 0);

    gpioBaseAddr_ = mmap_device_memory(nullptr, BCM2711_GPIO_SIZE,
                                       PROT_READ | PROT_WRITE | PROT_NOCACHE,
                                       0, BCM2711_GPIO_BASE);
    if (gpioBaseAddr_ != MAP_FAILED && gpioBaseAddr_ != nullptr) {
        hardwareMapped_.store(true);
    } else {
        gpioBaseAddr_ = nullptr;
        hardwareMapped_.store(false);
    }
#else
    gpioBaseAddr_ = nullptr;
    hardwareMapped_.store(false);
#endif

    initialized_.store(true);
    return true;
}

void Rpi4GpioDriver::Shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.exchange(false)) {
        return;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    if (gpioBaseAddr_ != nullptr) {
        munmap_device_memory(gpioBaseAddr_, BCM2711_GPIO_SIZE);
        gpioBaseAddr_ = nullptr;
    }
#endif
    hardwareMapped_.store(false);
    fallbackPinState_.clear();
}

bool Rpi4GpioDriver::ConfigureOutput(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (hardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
#if defined(__QNX__) || defined(__QNXNTO__)
        volatile uint32_t* gpfsel = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_) + (pin / 10);
        uint32_t shift = (pin % 10) * 3;
        uint32_t val = *gpfsel;
        val &= ~(7U << shift);
        val |= (1U << shift); // 001 = Output
        *gpfsel = val;
        return true;
#endif
    }

    fallbackPinState_[pin] = false;
    return true;
}

bool Rpi4GpioDriver::ConfigureInput(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (hardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
#if defined(__QNX__) || defined(__QNXNTO__)
        volatile uint32_t* gpfsel = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_) + (pin / 10);
        uint32_t shift = (pin % 10) * 3;
        uint32_t val = *gpfsel;
        val &= ~(7U << shift); // 000 = Input
        *gpfsel = val;
        return true;
#else
        (void)pin;
#endif
    }
    (void)pin;
    return true;
}

bool Rpi4GpioDriver::WritePin(int pin, bool high) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (hardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
#if defined(__QNX__) || defined(__QNXNTO__)
        volatile uint32_t* reg = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_);
        if (high) {
            reg[7 + (pin / 32)] = (1U << (pin % 32)); // GPSET0 / GPSET1
        } else {
            reg[10 + (pin / 32)] = (1U << (pin % 32)); // GPCLR0 / GPCLR1
        }
        return true;
#endif
    }

    fallbackPinState_[pin] = high;
    return true;
}

bool Rpi4GpioDriver::ReadPin(int pin) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_.load()) return false;

    if (hardwareMapped_.load() && gpioBaseAddr_ != nullptr) {
#if defined(__QNX__) || defined(__QNXNTO__)
        volatile uint32_t* reg = reinterpret_cast<volatile uint32_t*>(gpioBaseAddr_);
        uint32_t lev = reg[13 + (pin / 32)]; // GPLEV0 / GPLEV1
        return (lev & (1U << (pin % 32))) != 0;
#endif
    }

    auto it = fallbackPinState_.find(pin);
    if (it != fallbackPinState_.end()) {
        return it->second;
    }
    return false;
}

} // namespace aegis::traffic
