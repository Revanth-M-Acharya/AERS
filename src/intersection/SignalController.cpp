/**
 * @file SignalController.cpp
 * @brief Implementation of SignalController.
 */

#include "SignalController.h"

namespace aegis {
namespace intersection {

const char* SignalStateToString(SignalState state) noexcept {
    switch (state) {
        case SignalState::RED:       return "RED";
        case SignalState::YELLOW:    return "YELLOW";
        case SignalState::GREEN:     return "GREEN";
        case SignalState::NORMAL:    return "NORMAL";
        case SignalState::EMERGENCY: return "EMERGENCY";
        default:                     return "UNKNOWN";
    }
}

SignalController::SignalController()
    : m_state(SignalState::RED)
    , m_isEmergency(false) {
}

SignalController::SignalController(SignalController&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_state = other.m_state;
    m_isEmergency = other.m_isEmergency;
}

SignalController& SignalController::operator=(SignalController&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_state = other.m_state;
        m_isEmergency = other.m_isEmergency;
    }
    return *this;
}

void SignalController::SetGreen() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = SignalState::GREEN;
}

void SignalController::SetRed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = SignalState::RED;
}

void SignalController::SetYellow() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = SignalState::YELLOW;
}

void SignalController::SetNormal() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = SignalState::NORMAL;
    m_isEmergency = false;
}

void SignalController::EnableEmergency() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isEmergency = true;
    m_state = SignalState::EMERGENCY;
}

void SignalController::DisableEmergency() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isEmergency = false;
    m_state = SignalState::NORMAL;
}

SignalState SignalController::CurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

bool SignalController::IsEmergency() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isEmergency;
}

bool SignalController::IsNormal() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return !m_isEmergency;
}

std::string SignalController::StateString() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return SignalStateToString(m_state);
}

} // namespace intersection
} // namespace aegis
