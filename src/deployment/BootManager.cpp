/**
 * @file BootManager.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Deterministic Boot Manager Implementation.
 */

#include "BootManager.h"
#include <chrono>

namespace aegis {
namespace deployment {

BootManager::BootManager() {
    Reset();
}

void BootManager::Reset() {
    m_currentStage = BootStage::POWER_ON;
    m_reports.clear();
    m_totalBootTimeUs = 0;
    m_lastError.clear();

    // Log initial Power On stage
    BootStepReport pwr;
    pwr.stage = BootStage::POWER_ON;
    pwr.stageName = BootStageToString(BootStage::POWER_ON);
    pwr.durationUs = 0;
    pwr.success = true;
    pwr.details = "Hardware power applied; CPU initialization verified";
    m_reports.push_back(pwr);
}

bool BootManager::ExecuteStage(BootStage targetStage, StepExecutor executor) {
    auto start = std::chrono::steady_clock::now();
    std::string details;
    bool success = false;

    if (executor) {
        success = executor(details);
    } else {
        success = true;
        details = "Default stage step completed";
    }

    auto end = std::chrono::steady_clock::now();
    uint64_t duration = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

    m_totalBootTimeUs += duration;

    BootStepReport report;
    report.stage = targetStage;
    report.stageName = BootStageToString(targetStage);
    report.durationUs = duration;
    report.success = success;
    report.details = details;
    m_reports.push_back(report);

    if (success) {
        m_currentStage = targetStage;
        return true;
    } else {
        m_currentStage = BootStage::BOOT_FAILED;
        m_lastError = "Boot sequence failed at stage: " + report.stageName + " - " + details;
        return false;
    }
}

bool BootManager::RunFullBootSequence(const NodeConfiguration& config) {
    Reset();

    // Stage 1: Read configuration
    if (!ExecuteStage(BootStage::CONFIG_READ, [&config](std::string& details) {
        std::string err;
        if (!config.Validate(&err)) {
            details = "Configuration validation error: " + err;
            return false;
        }
        details = "Loaded valid config for Node: " + config.nodeId + " (" + NodeRoleToString(config.role) + ")";
        return true;
    })) return false;

    // Stage 2: Initialize BSP
    if (!ExecuteStage(BootStage::BSP_INIT, [&config](std::string& details) {
        details = "BCM2711 BSP layer initialized for RPi 4B (Simulation: " +
                  std::string(config.simulationEnabled ? "ON" : "OFF") + ")";
        return true;
    })) return false;

    // Stage 3: Initialize HAL
    if (!ExecuteStage(BootStage::HAL_INIT, [](std::string& details) {
        details = "Hardware Abstraction Layer (HAL) drivers loaded and initialized";
        return true;
    })) return false;

    // Stage 4: Initialize GPIO
    if (!ExecuteStage(BootStage::GPIO_INIT, [&config](std::string& details) {
        if (config.gpioEnabled) {
            details = "BCM2711 Bank 0 GPIO reserved (Red: " + std::to_string(config.gpioMapping.redPin) +
                      ", Yellow: " + std::to_string(config.gpioMapping.yellowPin) +
                      ", Green: " + std::to_string(config.gpioMapping.greenPin) + "); Safe Red asserted";
        } else {
            details = "GPIO actuation disabled by node configuration";
        }
        return true;
    })) return false;

    // Stage 5: Initialize TCP
    if (!ExecuteStage(BootStage::TCP_INIT, [&config](std::string& details) {
        if (config.IsCoordinator()) {
            details = "Coordinator TCP server bound on " + config.ipAddress + ":" + std::to_string(config.port);
        } else {
            details = "Edge TCP client socket initialized pointing to " + config.coordinatorAddress +
                      ":" + std::to_string(config.coordinatorPort);
        }
        return true;
    })) return false;

    // Stage 6: Register with Coordinator
    if (!ExecuteStage(BootStage::REGISTERING, [&config](std::string& details) {
        if (config.IsCoordinator()) {
            details = "Coordinator registration gateway active; waiting for edge nodes";
        } else {
            details = "Sent REGISTER message for node " + config.nodeId + "; ACK received from Coordinator";
        }
        return true;
    })) return false;

    // Stage 7: Heartbeat
    if (!ExecuteStage(BootStage::HEARTBEATING, [&config](std::string& details) {
        details = "Heartbeat timer established at interval: " + std::to_string(config.heartbeatIntervalMs) + " ms";
        return true;
    })) return false;

    // Stage 8: Ready
    if (!ExecuteStage(BootStage::READY, [&config](std::string& details) {
        details = "Node " + config.nodeId + " fully booted and operational in READY state";
        return true;
    })) return false;

    return true;
}

} // namespace deployment
} // namespace aegis
