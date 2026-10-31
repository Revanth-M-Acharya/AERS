/**
 * @file DeploymentValidator.hpp
 * @brief Comprehensive Deployment Validation Engine for SMART AEGIS Cluster.
 * Validates Connectivity, Configuration, GPIO, Traffic Signal Driver, IR Sensor, EventBus, and Time Sync.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "hardware_integration/gpio/SymbolicGpioHAL.hpp"
#include "hardware_integration/relay/ProductionRelayDriver.hpp"
#include "hardware_integration/sensor/IRSensorDriver.hpp"
#include "hardware_integration/config/NodeConfigManager.hpp"
#include "hardware_integration/cluster/ClusterSupervisor.hpp"
#include "coordinator/orchestration/event_bus/EventBus.hpp"
#include <string>
#include <vector>
#include <memory>

namespace aegis::hardware_integration {

struct ValidationVectorResult {
    std::string vectorName;
    bool passed{false};
    uint64_t latencyUs{0};
    std::string diagnosticMessage;
};

struct DeploymentValidationReport {
    bool overallPassed{false};
    uint64_t timestampMonoMs{0};
    uint32_t totalVectors{0};
    uint32_t passedVectors{0};
    std::vector<ValidationVectorResult> results;
    std::string auditSummary;
};

class DeploymentValidator {
public:
    DeploymentValidator(std::shared_ptr<SymbolicGpioHAL> hal,
                        std::shared_ptr<ProductionRelayDriver> relayDriver,
                        std::shared_ptr<IRSensorDriver> irSensor,
                        std::shared_ptr<NodeConfigManager> configMgr,
                        std::shared_ptr<ClusterSupervisor> supervisor,
                        std::shared_ptr<coordinator::orchestration::EventBus> eventBus = nullptr);
    ~DeploymentValidator() = default;

    /**
     * @brief Run end-to-end 7-vector deployment validation audit.
     */
    DeploymentValidationReport ExecuteFullValidation();

    // Individual Vector Checks
    ValidationVectorResult ValidateConnectivity();
    ValidationVectorResult ValidateConfiguration();
    ValidationVectorResult ValidateGPIO();
    ValidationVectorResult ValidateRelay();
    ValidationVectorResult ValidateIRSensor();
    ValidationVectorResult ValidateEventBus();
    ValidationVectorResult ValidateTimeSync();

    static std::string FormatReportAscii(const DeploymentValidationReport& rep);

private:
    std::shared_ptr<SymbolicGpioHAL> hal_;
    std::shared_ptr<ProductionRelayDriver> relayDriver_;
    std::shared_ptr<IRSensorDriver> irSensor_;
    std::shared_ptr<NodeConfigManager> configMgr_;
    std::shared_ptr<ClusterSupervisor> supervisor_;
    std::shared_ptr<coordinator::orchestration::EventBus> eventBus_;
};

} // namespace aegis::hardware_integration
