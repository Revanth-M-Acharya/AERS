/**
 * @file ManualOverrideManager.h
 * @brief Dispatcher and Operator Manual Override Management for SMART AEGIS.
 */

#ifndef AEGIS_SCHEDULER_MANUAL_OVERRIDE_MANAGER_H
#define AEGIS_SCHEDULER_MANUAL_OVERRIDE_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <optional>
#include <cstdint>

#include "ambulance/AmbulanceRegistry.h"
#include "medical/PriorityEngine.h"
#include "logger/EventLogger.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Audit record of an operator manual intervention.
 */
struct OperatorOverride {
    std::string overrideId{""};
    std::string ambulanceId{""};
    std::string intersectionId{""};
    std::string operatorId{"DISPATCH_OPERATOR"};
    uint8_t overrideLevel{1};
    medical::PriorityLevel assignedPriority{medical::PriorityLevel::CRITICAL};
    double assignedScore{100.0};
    std::string reason{""};
    uint64_t timestampEpochMs{0};
    bool isActive{true};

    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Manager governing human operator interventions and priority escalations with mandatory EventLogger auditing.
 */
class ManualOverrideManager {
public:
    explicit ManualOverrideManager(std::shared_ptr<ambulance::AmbulanceRegistry> registry = nullptr);
    ~ManualOverrideManager() = default;

    // Registry Binding
    void SetRegistry(std::shared_ptr<ambulance::AmbulanceRegistry> registry) noexcept;
    std::shared_ptr<ambulance::AmbulanceRegistry> GetRegistry() const noexcept;

    // Operator Interventions (all logged through EventLogger)
    bool ApplyOverrideLevel(const std::string& ambulanceId,
                            uint8_t level,
                            const std::string& operatorId = "OPERATOR_01",
                            const std::string& reason = "Dispatcher manual priority elevation");

    bool OverridePriority(const std::string& ambulanceId,
                          medical::PriorityLevel newPriority,
                          const std::string& operatorId = "OPERATOR_01",
                          const std::string& reason = "Clinical re-evaluation by medical dispatcher");

    bool OverridePriorityScore(const std::string& ambulanceId,
                               double score,
                               const std::string& operatorId = "OPERATOR_01",
                               const std::string& reason = "Manual triage score override");

    bool ForceGreenPreemption(const std::string& intersectionId,
                              const std::string& ambulanceId,
                              const std::string& operatorId = "OPERATOR_01",
                              const std::string& reason = "Manual junction force green");

    bool ClearOverride(const std::string& ambulanceId,
                       const std::string& operatorId = "OPERATOR_01");

    void ClearAllOverrides(const std::string& operatorId = "OPERATOR_01");

    // Queries
    bool HasOverride(const std::string& ambulanceId) const;
    uint8_t GetOverrideLevel(const std::string& ambulanceId) const;
    std::optional<OperatorOverride> GetOverride(const std::string& ambulanceId) const;
    std::vector<OperatorOverride> GetActiveOverrides() const;
    std::vector<OperatorOverride> GetOverrideHistory() const;

    void Clear();

private:
    static uint64_t CurrentTimeMillis();
    std::string GenerateOverrideId();

    mutable std::mutex m_mutex;
    std::shared_ptr<ambulance::AmbulanceRegistry> m_registry{nullptr};
    std::unordered_map<std::string, OperatorOverride> m_activeOverrides{};
    std::vector<OperatorOverride> m_overrideHistory{};
    uint64_t m_overrideSequence{0};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_MANUAL_OVERRIDE_MANAGER_H
