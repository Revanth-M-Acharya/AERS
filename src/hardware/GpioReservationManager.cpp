/**
 * @file GpioReservationManager.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Production BCM2711 GPIO Reservation Manager Implementation.
 */

#include "aegis/hardware/GpioReservationManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <sstream>
#include <iomanip>
#include <unordered_set>

namespace aegis {
namespace hardware {

GpioReservationManager& GpioReservationManager::Instance() {
    static GpioReservationManager s_instance;
    return s_instance;
}

GpioReservationManager::GpioReservationManager() = default;

GpioReservationManager::~GpioReservationManager() {
    ReleaseAll("SYSTEM");
}

bool GpioReservationManager::ValidateBcmPin(uint32_t pin) const noexcept {
    // Valid user accessible GPIOs on Raspberry Pi 4 BCM2711 40-pin header: GPIO 0 to 27
    return pin <= MAX_BCM_PIN;
}

bool GpioReservationManager::ReservePin(uint32_t pin, const std::string& owner, const std::string& purpose,
                                        bsp::PinMode direction) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!ValidateBcmPin(pin)) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "GPIO_RES",
            "Reservation failed: Pin GPIO" + std::to_string(pin) + " exceeds valid BCM2711 range (0..27).");
        return false;
    }

    auto it = m_reservations.find(pin);
    if (it != m_reservations.end()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::WARN, "GPIO_RES",
            "Conflict: Pin GPIO" + std::to_string(pin) + " already reserved by " +
            it->second.owner + " [" + it->second.purpose + "]. Rejected request from " + owner + ".");
        return false;
    }

    GpioReservation res;
    res.pin = pin;
    res.owner = owner;
    res.purpose = purpose;
    res.direction = direction;
    res.timestampUs = common::GetMonotonicMicroseconds();

    m_reservations[pin] = res;

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_RES",
        "Reserved GPIO" + std::to_string(pin) + " for " + owner + " (" + purpose +
        ") Dir: " + (direction == bsp::PinMode::OUTPUT ? "OUTPUT" : "INPUT"));

    return true;
}

bool GpioReservationManager::ReleasePin(uint32_t pin, const std::string& owner) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_reservations.find(pin);
    if (it == m_reservations.end()) {
        return false;
    }

    if (owner != "SYSTEM" && owner != "FORCE" && !owner.empty() && it->second.owner != owner) {
        logger::Logger::Instance().Log(
            logger::LogLevel::WARN, "GPIO_RES",
            "Unauthorized release: Owner mismatch for GPIO" + std::to_string(pin) +
            " (Owned by " + it->second.owner + ", requested by " + owner + ").");
        return false;
    }

    m_reservations.erase(it);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_RES",
        "Released GPIO" + std::to_string(pin) + " previously owned by " + owner + ".");

    return true;
}

void GpioReservationManager::ReleaseAll(const std::string& owner) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (owner.empty() || owner == "SYSTEM" || owner == "FORCE") {
        m_reservations.clear();
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "GPIO_RES", "Released all GPIO reservations.");
    } else {
        for (auto it = m_reservations.begin(); it != m_reservations.end(); ) {
            if (it->second.owner == owner) {
                it = m_reservations.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool GpioReservationManager::IsPinReserved(uint32_t pin) const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_reservations.find(pin) != m_reservations.end();
}

bool GpioReservationManager::GetReservation(uint32_t pin, GpioReservation& outRes) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_reservations.find(pin);
    if (it == m_reservations.end()) {
        return false;
    }
    outRes = it->second;
    return true;
}

std::vector<GpioReservation> GpioReservationManager::GetAllReservations() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<GpioReservation> result;
    result.reserve(m_reservations.size());
    for (const auto& kv : m_reservations) {
        result.push_back(kv.second);
    }
    return result;
}

size_t GpioReservationManager::GetReservedPinCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_reservations.size();
}

bool GpioReservationManager::CanReservePins(const std::vector<uint32_t>& pins, std::string& outConflictReason) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::unordered_set<uint32_t> seenPins;
    for (uint32_t pin : pins) {
        if (!ValidateBcmPin(pin)) {
            outConflictReason = "Pin GPIO" + std::to_string(pin) + " exceeds valid range (0..27).";
            return false;
        }
        if (!seenPins.insert(pin).second) {
            outConflictReason = "Duplicate pin in candidate reservation request: GPIO" + std::to_string(pin);
            return false;
        }
        auto it = m_reservations.find(pin);
        if (it != m_reservations.end()) {
            outConflictReason = "Pin GPIO" + std::to_string(pin) + " is already reserved by " +
                                it->second.owner + " [" + it->second.purpose + "].";
            return false;
        }
    }

    outConflictReason.clear();
    return true;
}

std::string GpioReservationManager::GenerateDiagnostics() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: BCM2711 GPIO RESERVATION TABLE\n";
    oss << " Total Reserved Pins: " << m_reservations.size() << "\n";
    oss << "==========================================================\n";
    oss << std::left << std::setw(10) << "PIN"
        << std::setw(22) << "OWNER"
        << std::setw(10) << "DIR"
        << "PURPOSE\n";
    oss << "----------------------------------------------------------\n";

    for (const auto& [pin, res] : m_reservations) {
        oss << std::left << std::setw(10) << ("GPIO" + std::to_string(pin))
            << std::setw(22) << res.owner
            << std::setw(10) << (res.direction == bsp::PinMode::OUTPUT ? "OUTPUT" : "INPUT")
            << res.purpose << "\n";
    }
    oss << "==========================================================\n";

    return oss.str();
}

void GpioReservationManager::Reset() {
    ReleaseAll("SYSTEM");
}

} // namespace hardware
} // namespace aegis
