/**
 * @file BootManager.h
 * @brief SMART AEGIS (AERS) Phase 16D - Deterministic Boot Manager.
 *
 * Enforces the formal 8-step deterministic startup sequence:
 * Power On -> Read config -> Init BSP -> Init HAL -> Init GPIO -> Init TCP -> Register -> Heartbeat -> Ready
 */

#ifndef AEGIS_DEPLOYMENT_BOOT_MANAGER_H
#define AEGIS_DEPLOYMENT_BOOT_MANAGER_H

#include "NodeConfiguration.h"
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace aegis {
namespace deployment {

enum class BootStage {
    POWER_ON = 0,
    CONFIG_READ,
    BSP_INIT,
    HAL_INIT,
    GPIO_INIT,
    TCP_INIT,
    REGISTERING,
    HEARTBEATING,
    READY,
    BOOT_FAILED
};

inline std::string BootStageToString(BootStage stage) {
    switch (stage) {
        case BootStage::POWER_ON:     return "POWER_ON";
        case BootStage::CONFIG_READ:  return "CONFIG_READ";
        case BootStage::BSP_INIT:     return "BSP_INIT";
        case BootStage::HAL_INIT:     return "HAL_INIT";
        case BootStage::GPIO_INIT:    return "GPIO_INIT";
        case BootStage::TCP_INIT:     return "TCP_INIT";
        case BootStage::REGISTERING:  return "REGISTERING";
        case BootStage::HEARTBEATING: return "HEARTBEATING";
        case BootStage::READY:        return "READY";
        case BootStage::BOOT_FAILED:  return "BOOT_FAILED";
        default:                      return "UNKNOWN";
    }
}

struct BootStepReport {
    BootStage stage{BootStage::POWER_ON};
    std::string stageName{""};
    uint64_t durationUs{0};
    bool success{false};
    std::string details{""};
};

class BootManager {
public:
    using StepExecutor = std::function<bool(std::string& outDetails)>;

    BootManager();
    ~BootManager() = default;

    /**
     * @brief Reset boot manager back to POWER_ON stage.
     */
    void Reset();

    /**
     * @brief Advance deterministic boot sequence through next stage.
     * @param targetStage Stage being executed.
     * @param executor Lambda executing the specific stage logic.
     * @return true if stage executed successfully; false if failed.
     */
    bool ExecuteStage(BootStage targetStage, StepExecutor executor);

    /**
     * @brief Run complete automated deterministic boot sequence.
     * @param config Node configuration to execute with.
     * @return true if node reaches READY stage; false on any stage failure.
     */
    bool RunFullBootSequence(const NodeConfiguration& config);

    /**
     * @brief Query current boot stage.
     */
    BootStage GetCurrentStage() const noexcept { return m_currentStage; }

    /**
     * @brief Query whether node is in READY state.
     */
    bool IsReady() const noexcept { return m_currentStage == BootStage::READY; }

    /**
     * @brief Query whether boot sequence failed.
     */
    bool IsFailed() const noexcept { return m_currentStage == BootStage::BOOT_FAILED; }

    /**
     * @brief Get chronological list of all executed boot step reports.
     */
    const std::vector<BootStepReport>& GetBootReports() const noexcept { return m_reports; }

    /**
     * @brief Get total boot time in microseconds across all stages.
     */
    uint64_t GetTotalBootTimeUs() const noexcept { return m_totalBootTimeUs; }

    /**
     * @brief Get error message if boot failed.
     */
    const std::string& GetLastError() const noexcept { return m_lastError; }

private:
    BootStage m_currentStage{BootStage::POWER_ON};
    std::vector<BootStepReport> m_reports{};
    uint64_t m_totalBootTimeUs{0};
    std::string m_lastError{""};
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_BOOT_MANAGER_H
