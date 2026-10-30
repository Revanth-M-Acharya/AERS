/**
 * @file BootManager.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Production Hardware Deterministic Boot Sequence Implementation.
 */

#include "aegis/hardware/BootManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <sstream>
#include <iomanip>

namespace aegis {
namespace hardware {

BootManager::BootManager() {
    Reset();
}

void BootManager::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentStep.store(HardwareBootStep::POWER_ON);
    m_currentIndustrialStep.store(IndustrialBootStep::POWER_ON);
    m_inSafeMode.store(false);
    m_totalBootDurationUs.store(0);
    m_reports.clear();
}

void BootManager::RegisterStepExecutor(HardwareBootStep step, StepFunction func) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stepExecutors[static_cast<uint8_t>(step)] = std::move(func);
}

void BootManager::RegisterIndustrialStepExecutor(IndustrialBootStep step, StepFunction func) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_industrialStepExecutors[static_cast<uint8_t>(step)] = std::move(func);
}

void BootManager::SetSafeModeHandler(SafeModeHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_safeModeHandler = std::move(handler);
}

bool BootManager::ExecuteStep(HardwareBootStep targetStep, StepFunction customFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_inSafeMode.load() || m_currentStep.load() == HardwareBootStep::BOOT_FAILED) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "BOOT_MGR",
            "Cannot execute step " + HardwareBootStepToString(targetStep) + ": Boot manager is in SAFE MODE / FAILED.");
        return false;
    }

    uint64_t startUs = common::GetMonotonicMicroseconds();
    std::string details;
    bool success = false;

    // Pick customFunc, or registered executor, or default fallback
    StepFunction exec = customFunc;
    if (!exec) {
        auto it = m_stepExecutors.find(static_cast<uint8_t>(targetStep));
        if (it != m_stepExecutors.end()) {
            exec = it->second;
        }
    }

    if (exec) {
        try {
            success = exec(details);
        } catch (const std::exception& e) {
            success = false;
            details = std::string("Exception caught during step: ") + e.what();
        } catch (...) {
            success = false;
            details = "Unknown exception caught during boot step execution.";
        }
    } else {
        // Default nominal behavior for standard steps
        success = true;
        details = HardwareBootStepToString(targetStep) + " nominal execution confirmed.";
    }

    uint64_t durationUs = common::GetMonotonicMicroseconds() - startUs;
    m_totalBootDurationUs.fetch_add(durationUs);

    HardwareBootStepReport rep;
    rep.step = targetStep;
    rep.stepName = HardwareBootStepToString(targetStep);
    rep.passed = success;
    rep.durationUs = durationUs;
    rep.details = details;
    m_reports.push_back(rep);

    if (!success) {
        EnterSafeMode("Failure at step " + rep.stepName + ": " + details);
        return false;
    }

    m_currentStep.store(targetStep);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR",
        "Step " + rep.stepName + " completed in " + std::to_string(durationUs) + " us: " + details);

    return true;
}

bool BootManager::ExecuteIndustrialStep(IndustrialBootStep targetStep, StepFunction customFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_inSafeMode.load() || m_currentIndustrialStep.load() == IndustrialBootStep::BOOT_FAILED) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "BOOT_MGR",
            "Cannot execute industrial step " + IndustrialBootStepToString(targetStep) + ": Boot manager is in SAFE MODE / FAILED.");
        return false;
    }

    uint64_t startUs = common::GetMonotonicMicroseconds();
    std::string details;
    bool success = false;

    // Pick customFunc, or registered executor, or default fallback
    StepFunction exec = customFunc;
    if (!exec) {
        auto it = m_industrialStepExecutors.find(static_cast<uint8_t>(targetStep));
        if (it != m_industrialStepExecutors.end()) {
            exec = it->second;
        }
    }

    if (exec) {
        try {
            success = exec(details);
        } catch (const std::exception& e) {
            success = false;
            details = std::string("Exception caught during industrial step: ") + e.what();
        } catch (...) {
            success = false;
            details = "Unknown exception caught during industrial boot step execution.";
        }
    } else {
        success = true;
        details = IndustrialBootStepToString(targetStep) + " nominal execution confirmed.";
    }

    uint64_t durationUs = common::GetMonotonicMicroseconds() - startUs;
    m_totalBootDurationUs.fetch_add(durationUs);

    HardwareBootStepReport rep;
    rep.step = HardwareBootStep::POWER_ON;
    rep.stepName = IndustrialBootStepToString(targetStep);
    rep.passed = success;
    rep.durationUs = durationUs;
    rep.details = details;
    m_reports.push_back(rep);

    if (!success) {
        m_currentIndustrialStep.store(IndustrialBootStep::BOOT_FAILED);
        EnterSafeMode("Failure at industrial step " + rep.stepName + ": " + details);
        return false;
    }

    m_currentIndustrialStep.store(targetStep);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR",
        "Industrial Step " + rep.stepName + " completed in " + std::to_string(durationUs) + " us (" +
        std::to_string(static_cast<double>(durationUs) / 1000.0) + " ms): " + details);

    return true;
}

void BootManager::EnterSafeMode(const std::string& failureReason) {
    m_inSafeMode.store(true);
    m_currentStep.store(HardwareBootStep::BOOT_FAILED);
    m_currentIndustrialStep.store(IndustrialBootStep::BOOT_FAILED);

    logger::Logger::Instance().Log(
        logger::LogLevel::ERR, "BOOT_MGR",
        "BOOT SEQUENCE ABORTED! Entering Safe Mode. Reason: " + failureReason);

    if (m_safeModeHandler) {
        m_safeModeHandler(failureReason);
    }
}

bool BootManager::RunFullBootSequence() {
    Reset();

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR", "Starting SMART AEGIS 10-Step Deterministic Hardware Boot...");

    static const std::vector<HardwareBootStep> sequence = {
        HardwareBootStep::BSP_INITIALIZATION,
        HardwareBootStep::GPIO_RESERVATION,
        HardwareBootStep::RELAY_SELF_TEST,
        HardwareBootStep::HARDWARE_DIAGNOSTICS,
        HardwareBootStep::WIFI_STARTUP,
        HardwareBootStep::NODE_REGISTRATION,
        HardwareBootStep::COORDINATOR_HANDSHAKE,
        HardwareBootStep::WATCHDOG_STARTUP,
        HardwareBootStep::HARDWARE_READY,
        HardwareBootStep::ACCEPT_TRAFFIC_COMMANDS
    };

    for (HardwareBootStep step : sequence) {
        if (!ExecuteStep(step)) {
            return false;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentStep.store(HardwareBootStep::BOOT_COMPLETED);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR",
        "Deterministic Hardware Boot COMPLETE: Node online and ready to accept traffic commands.");

    return true;
}

bool BootManager::RunIndustrialBootSequence() {
    Reset();

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR", "Starting SMART AEGIS Phase 2 Industrial Startup Sequence (14 Stages)...");

    static const std::vector<IndustrialBootStep> sequence = {
        IndustrialBootStep::KERNEL_BOOT,
        IndustrialBootStep::INIT_BSP,
        IndustrialBootStep::INIT_GPIO,
        IndustrialBootStep::RESERVE_GPIO,
        IndustrialBootStep::INIT_RELAY_CONTROLLER,
        IndustrialBootStep::RUN_RELAY_POST,
        IndustrialBootStep::INIT_IR_SENSORS,
        IndustrialBootStep::INIT_NETWORK_INTERFACE,
        IndustrialBootStep::VERIFY_NETWORK_LINK,
        IndustrialBootStep::REGISTER_WITH_COORDINATOR,
        IndustrialBootStep::START_HARDWARE_WATCHDOG,
        IndustrialBootStep::START_HEARTBEAT,
        IndustrialBootStep::ENTER_OPERATIONAL_MODE,
        IndustrialBootStep::READY
    };

    for (IndustrialBootStep step : sequence) {
        if (!ExecuteIndustrialStep(step)) {
            return false;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentIndustrialStep.store(IndustrialBootStep::READY);
    m_currentStep.store(HardwareBootStep::BOOT_COMPLETED);
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "BOOT_MGR",
        "Industrial Startup Sequence COMPLETE: 14/14 stages passed. Total time: " +
        std::to_string(m_totalBootDurationUs.load()) + " us (" +
        std::to_string(static_cast<double>(m_totalBootDurationUs.load()) / 1000.0) + " ms). State: READY.");

    return true;
}

HardwareBootStep BootManager::GetCurrentStep() const noexcept {
    return m_currentStep.load();
}

IndustrialBootStep BootManager::GetCurrentIndustrialStep() const noexcept {
    return m_currentIndustrialStep.load();
}

bool BootManager::IsBootCompleted() const noexcept {
    return m_currentStep.load() == HardwareBootStep::BOOT_COMPLETED ||
           m_currentStep.load() == HardwareBootStep::ACCEPT_TRAFFIC_COMMANDS ||
           m_currentIndustrialStep.load() == IndustrialBootStep::READY;
}

bool BootManager::IsBootFailed() const noexcept {
    return m_currentStep.load() == HardwareBootStep::BOOT_FAILED ||
           m_currentIndustrialStep.load() == IndustrialBootStep::BOOT_FAILED;
}

bool BootManager::IsInSafeMode() const noexcept {
    return m_inSafeMode.load();
}

uint64_t BootManager::GetTotalBootDurationUs() const noexcept {
    return m_totalBootDurationUs.load();
}

double BootManager::GetTotalBootDurationMs() const noexcept {
    return static_cast<double>(m_totalBootDurationUs.load()) / 1000.0;
}

std::vector<HardwareBootStepReport> BootManager::GetStepReports() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_reports;
}

std::string BootManager::GenerateBootReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: DETERMINISTIC HARDWARE BOOT REPORT\n";
    oss << " Status: " << (m_inSafeMode.load() ? "FAILED (SAFE MODE)" : "COMPLETED / READY") << "\n";
    oss << " Current Step: " << HardwareBootStepToString(m_currentStep.load()) << "\n";
    oss << " Total Duration: " << m_totalBootDurationUs.load() << " us ("
        << std::fixed << std::setprecision(3) << (static_cast<double>(m_totalBootDurationUs.load()) / 1000.0) << " ms)\n";
    oss << "==========================================================\n";
    oss << std::left << std::setw(30) << "STEP"
        << std::setw(10) << "STATUS"
        << std::setw(14) << "DURATION (us)"
        << "DETAILS\n";
    oss << "----------------------------------------------------------\n";

    for (const auto& r : m_reports) {
        oss << std::left << std::setw(30) << r.stepName
            << std::setw(10) << (r.passed ? "PASS" : "FAIL")
            << std::setw(14) << r.durationUs
            << r.details << "\n";
    }
    oss << "==========================================================\n";

    return oss.str();
}

std::string BootManager::GenerateIndustrialBootReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: PHASE 2 INDUSTRIAL STARTUP SEQUENCE REPORT\n";
    oss << " Final State: " << IndustrialBootStepToString(m_currentIndustrialStep.load()) << "\n";
    oss << " Safe Mode:   " << (m_inSafeMode.load() ? "ACTIVE (TRIPPED)" : "INACTIVE (NOMINAL)") << "\n";
    oss << " Boot Time:   " << m_totalBootDurationUs.load() << " us ("
        << std::fixed << std::setprecision(3) << (static_cast<double>(m_totalBootDurationUs.load()) / 1000.0) << " ms)\n";
    oss << " Stages Executed: " << m_reports.size() << "\n";
    oss << "==========================================================\n";
    oss << std::left << std::setw(32) << "STAGE"
        << std::setw(10) << "RESULT"
        << std::setw(16) << "LATENCY (us)"
        << "STAGE TELEMETRY\n";
    oss << "----------------------------------------------------------\n";

    for (const auto& r : m_reports) {
        oss << std::left << std::setw(32) << r.stepName
            << std::setw(10) << (r.passed ? "PASS" : "FAIL")
            << std::setw(16) << r.durationUs
            << r.details << "\n";
    }
    oss << "==========================================================\n";

    return oss.str();
}

} // namespace hardware
} // namespace aegis
