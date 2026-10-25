/**
 * @file gpio_interface.cpp
 * @brief Implementation of SimulationGpioInterface logging simulated lamp transitions.
 */

#include "aegis/edge/gpio_interface.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace edge {
namespace traffic {

SimulationGpioInterface::SimulationGpioInterface() = default;
SimulationGpioInterface::~SimulationGpioInterface() {
    Shutdown();
}

bool SimulationGpioInterface::Initialize() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isInitialized = true;
    m_lastAction = "INITIALIZED";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulation GPIO interface initialized.");
    return true;
}

void SimulationGpioInterface::SetRed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "SET_RED";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: RED (Stop)");
}

void SimulationGpioInterface::SetYellow() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "SET_YELLOW";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: YELLOW (Caution)");
}

void SimulationGpioInterface::SetGreen() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "SET_GREEN";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: GREEN (Proceed)");
}

void SimulationGpioInterface::FlashYellow() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "FLASH_YELLOW";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: FLASHING_YELLOW (Degraded Caution)");
}

void SimulationGpioInterface::FlashRed() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "FLASH_RED";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: FLASHING_RED (All-Way Stop Fail-Safe)");
}

void SimulationGpioInterface::AllOff() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastAction = "ALL_OFF";
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "GPIO_SIM", "Simulated Lamp State: ALL_OFF");
}

void SimulationGpioInterface::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isInitialized) {
        m_isInitialized = false;
        m_lastAction = "SHUTDOWN";
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "GPIO_SIM", "Simulation GPIO interface shut down.");
    }
}

std::string SimulationGpioInterface::GetLastAction() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastAction;
}

bool SimulationGpioInterface::IsInitialized() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isInitialized;
}

std::unique_ptr<IGpioInterface> CreateSimulationGpio() {
    return std::make_unique<SimulationGpioInterface>();
}

} // namespace traffic
} // namespace edge
} // namespace aegis
