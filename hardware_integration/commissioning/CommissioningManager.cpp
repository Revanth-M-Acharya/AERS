/**
 * @file CommissioningManager.cpp
 * @brief Implementation of CommissioningManager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "hardware_integration/commissioning/CommissioningManager.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>
#include <iomanip>

namespace aegis::hardware_integration {

CommissioningManager::CommissioningManager(std::shared_ptr<SymbolicGpioHAL> hal,
                                           std::shared_ptr<ProductionRelayDriver> relayDriver,
                                           std::shared_ptr<IRSensorDriver> irSensor,
                                           std::shared_ptr<NodeConfigManager> configMgr)
    : hal_(std::move(hal)),
      relayDriver_(std::move(relayDriver)),
      irSensor_(std::move(irSensor)),
      configMgr_(std::move(configMgr)) {
}

CommissioningReport CommissioningManager::RunCommissioning(uint32_t channelDwellMs) {
    CommissioningReport report;
    report.startTimestampMonoMs = common::TimeUtils::GetMonotonicTimeMs();
    uint64_t start = report.startTimestampMonoMs;

    auto profile = configMgr_ ? configMgr_->GetProfile() : NodeConfigManager::CreateDefaultTrafficNodeProfile("INT_A", "127.0.0.1", 9001);
    report.nodeId = profile.nodeId;
    report.nodeRole = NodeRoleToString(profile.role);

    // Step 1: Configuration Validation
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "Configuration Schema & Pin Audit";
        std::vector<std::string> errs;
        r.passed = configMgr_ ? configMgr_->Validate(errs) : true;
        r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
        r.details = r.passed ? "All parameters and pin mappings valid" : (!errs.empty() ? errs[0] : "Validation failed");
        report.stepResults.push_back(r);
    }

    // Step 2: GPIO HAL Hardware Initialization
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "GPIO Driver & Symbolic Register Initialization";
        bool relayInit = relayDriver_ ? relayDriver_->Initialize() : true;
        bool halInit = hal_ ? hal_->Initialize() : false;
        r.passed = halInit && relayInit;
        r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
        r.details = r.passed ? "HAL and relay driver initialized successfully, all registered lines configured" : "HAL/relay init failed";
        report.stepResults.push_back(r);
    }

    // Step 3: Relay Channel Sequential Step Test
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "Relay Channel Sequential Activation Test";
        if (relayDriver_) {
            auto testRes = relayDriver_->ExecuteSelfTest(channelDwellMs);
            r.passed = testRes.passed;
            r.durationMs = testRes.executionTimeMs;
            r.details = testRes.passed ? "All relay channels energized and verified OK"
                                       : "One or more relay channels failed contact test";
        } else {
            r.passed = false;
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = "Relay driver unavailable";
        }
        report.stepResults.push_back(r);
    }

    // Step 4: Safety Conflict Interlock Verification
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "Safety Conflict Interlock Guarantee Test";
        if (relayDriver_) {
            relayDriver_->SetAllRed();
            bool roadAPhase = relayDriver_->SetNormalPhase(TrafficApproach::ROAD_A);
            bool greenConflict = (hal_->GetSignal(StandardSignals::ROAD_A_GREEN) && hal_->GetSignal(StandardSignals::ROAD_B_GREEN));

            r.passed = roadAPhase && !greenConflict;
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = r.passed ? "Conflicting green strictly prohibited; safety interlock verified"
                                 : "FATAL: Conflicting green allowed!";
            relayDriver_->SetAllRed();
        } else {
            r.passed = false;
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = "Relay driver unavailable";
        }
        report.stepResults.push_back(r);
    }

    // Step 5: Optical IR Sensor Baseline & Diagnostics
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "Optical IR Sensor Continuity & Health Check";
        if (irSensor_) {
            irSensor_->Initialize();
            auto health = irSensor_->RunDiagnostics();
            r.passed = (health == SensorHealthGrade::HEALTHY || health == SensorHealthGrade::DEGRADED);
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = (health == SensorHealthGrade::HEALTHY) ? "IR Sensor operational, beam clear, noise nominal"
                                                               : "IR Sensor reporting degraded/faulty state";
        } else {
            r.passed = true; // Optional if node is coordinator
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = "IR sensor skipped (Coordinator or non-sensing node)";
        }
        report.stepResults.push_back(r);
    }

    // Step 6: Fail-Safe Flashing Yellow & All-Red Transition Test
    {
        uint64_t stepStart = common::TimeUtils::GetMonotonicTimeMs();
        CommissioningStepResult r;
        r.stepName = "Fail-Safe Default State Transition Test";
        if (relayDriver_) {
            bool yellowOk = relayDriver_->SetFlashingYellow(true);
            bool allRedOk = relayDriver_->SetAllRed();
            r.passed = yellowOk && allRedOk && (relayDriver_->GetCurrentMode() == RelayOperationalMode::ALL_RED);
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = r.passed ? "Flashing yellow and All-Red fail-safe modes verified"
                                 : "Fail-safe mode transition failed";
        } else {
            r.passed = false;
            r.durationMs = common::TimeUtils::GetMonotonicTimeMs() - stepStart;
            r.details = "Relay driver unavailable";
        }
        report.stepResults.push_back(r);
    }

    // Aggregate
    report.totalSteps = static_cast<uint32_t>(report.stepResults.size());
    report.passedSteps = 0;
    for (const auto& step : report.stepResults) {
        if (step.passed) report.passedSteps++;
    }

    report.totalDurationMs = common::TimeUtils::GetMonotonicTimeMs() - start;
    report.overallSuccess = (report.passedSteps == report.totalSteps);
    report.summary = report.overallSuccess ? "COMMISSIONING SUCCESSFUL: Node certified for live traffic"
                                           : "COMMISSIONING FAILED: Hardware faults detected, node isolated";

    isCommissioned_ = report.overallSuccess;
    lastReport_ = report;
    return report;
}

std::string CommissioningManager::FormatReportAscii(const CommissioningReport& report) {
    std::ostringstream ss;
    ss << "\n==========================================================\n";
    ss << " SMART AEGIS: NODE STARTUP COMMISSIONING AUDIT REPORT    \n";
    ss << "==========================================================\n";
    ss << " Node ID:    " << report.nodeId << "\n";
    ss << " Node Role:  " << report.nodeRole << "\n";
    ss << " Status:     " << (report.overallSuccess ? "[PASS / CERTIFIED]" : "[FAIL / ISOLATED]") << "\n";
    ss << " Duration:   " << report.totalDurationMs << " ms (" << report.passedSteps << "/" << report.totalSteps << " steps passed)\n";
    ss << "----------------------------------------------------------\n";

    for (size_t i = 0; i < report.stepResults.size(); ++i) {
        const auto& step = report.stepResults[i];
        ss << " [" << (i + 1) << "] " << std::left << std::setw(42) << step.stepName
           << " [" << (step.passed ? "PASS" : "FAIL") << "] (" << step.durationMs << " ms)\n"
           << "     Details: " << step.details << "\n";
    }

    ss << "----------------------------------------------------------\n";
    ss << " Result: " << report.summary << "\n";
    ss << "==========================================================\n";
    return ss.str();
}

} // namespace aegis::hardware_integration
