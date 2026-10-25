/**
 * @file traffic_signal_node.cpp
 * @brief Implementation of TrafficSignalNode wrapping TrafficController.
 */

#include "aegis/edge/traffic_signal_node.hpp"
#include "aegis/logger/logger.hpp"

#include <thread>
#include <chrono>

namespace aegis {
namespace edge {
namespace traffic {

TrafficSignalNode::TrafficSignalNode()
    : m_controller(std::make_unique<TrafficController>()) {}

TrafficSignalNode::TrafficSignalNode(std::unique_ptr<TrafficController> controller)
    : m_controller(controller ? std::move(controller) : std::make_unique<TrafficController>()) {}

TrafficSignalNode::TrafficSignalNode(const TrafficTimingConfig& config)
    : m_controller(std::make_unique<TrafficController>(CreateSimulationGpio(), config)) {}

TrafficSignalNode::~TrafficSignalNode() {
    Stop();
}

bool TrafficSignalNode::Start() {
    if (!m_controller) {
        m_controller = std::make_unique<TrafficController>();
    }

    if (!m_controller->Initialize()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "SIGNAL_NODE", "Failed to initialize TrafficController.");
        return false;
    }

    if (!m_controller->Start()) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "SIGNAL_NODE", "Failed to start TrafficController.");
        return false;
    }

    m_running.store(true);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "SIGNAL_NODE", "TrafficSignalNode started.");
    return true;
}

void TrafficSignalNode::Stop() {
    if (m_running.exchange(false)) {
        if (m_controller) {
            m_controller->Stop();
        }
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "SIGNAL_NODE", "TrafficSignalNode stopped.");
    }
}

void TrafficSignalNode::Run(uint32_t iterations, uint32_t stepMs) {
    if (!m_running.load()) {
        if (!Start()) {
            return;
        }
    }

    if (iterations > 0) {
        for (uint32_t i = 0; i < iterations && m_running.load(); ++i) {
            Tick(stepMs);
        }
    } else {
        while (m_running.load()) {
            Tick(stepMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
        }
    }
}

void TrafficSignalNode::Tick(uint32_t deltaMs) {
    if (m_controller && m_running.load()) {
        m_controller->Tick(deltaMs);
    }
}

bool TrafficSignalNode::IsRunning() const noexcept {
    return m_running.load();
}

SignalState TrafficSignalNode::GetCurrentState() const {
    if (!m_controller) {
        return SignalState::UNKNOWN;
    }
    return m_controller->GetCurrentState();
}

TrafficController& TrafficSignalNode::GetController() noexcept {
    return *m_controller;
}

const TrafficController& TrafficSignalNode::GetController() const noexcept {
    return *m_controller;
}

} // namespace traffic
} // namespace edge
} // namespace aegis
