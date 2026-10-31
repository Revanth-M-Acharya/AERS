/**
 * @file CommissioningManager.hpp
 * @brief Automated Commissioning & Hardware Self-Test Engine for SMART AEGIS Raspberry Pi Nodes.
 * Executes on node boot to certify relay, GPIO, IR sensor, and fail-safe safety interlocks.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/relay/ProductionRelayDriver.hpp"
#include "hardware_integration/sensor/IRSensorDriver.hpp"
#include "hardware_integration/config/NodeConfigManager.hpp"
#include <string>
#include <vector>
#include <memory>

namespace aegis::hardware_integration {

struct CommissioningStepResult {
    std::string stepName;
    bool passed{false};
    uint64_t durationMs{0};
    std::string details;
};

struct CommissioningReport {
    std::string nodeId;
    std::string nodeRole;
    bool overallSuccess{false};
    uint64_t startTimestampMonoMs{0};
    uint64_t totalDurationMs{0};
    uint32_t totalSteps{0};
    uint32_t passedSteps{0};
    std::vector<CommissioningStepResult> stepResults;
    std::string summary;
};

class CommissioningManager {
public:
    CommissioningManager(std::shared_ptr<SymbolicGpioHAL> hal,
                         std::shared_ptr<ProductionRelayDriver> relayDriver,
                         std::shared_ptr<IRSensorDriver> irSensor,
                         std::shared_ptr<NodeConfigManager> configMgr);
    ~CommissioningManager() = default;

    /**
     * @brief Run complete automated commissioning self-test sequence.
     * @param channelDwellMs Delay between relay steps (e.g. 50ms for tests, 100ms for hardware).
     */
    CommissioningReport RunCommissioning(uint32_t channelDwellMs = 50);

    /**
     * @brief Check if last commissioning run was successful.
     */
    bool IsCommissioned() const { return isCommissioned_; }

    /**
     * @brief Get cached commissioning report.
     */
    CommissioningReport GetLastReport() const { return lastReport_; }

    /**
     * @brief Format commissioning report as clean ASCII text block.
     */
    static std::string FormatReportAscii(const CommissioningReport& report);

private:
    std::shared_ptr<SymbolicGpioHAL> hal_;
    std::shared_ptr<ProductionRelayDriver> relayDriver_;
    std::shared_ptr<IRSensorDriver> irSensor_;
    std::shared_ptr<NodeConfigManager> configMgr_;

    bool isCommissioned_{false};
    CommissioningReport lastReport_;
};

} // namespace aegis::hardware_integration
