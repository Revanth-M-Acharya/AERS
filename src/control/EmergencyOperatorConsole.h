/**
 * @file EmergencyOperatorConsole.h
 * @brief Control Center operator manual intervention console for SMART AEGIS.
 */

#ifndef AEGIS_CONTROL_EMERGENCY_OPERATOR_CONSOLE_H
#define AEGIS_CONTROL_EMERGENCY_OPERATOR_CONSOLE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>
#include <optional>
#include <cstdint>

#include "control/ManualOverride.h"
#include "ambulance/AmbulanceRegistry.h"
#include "scheduler/PriorityScheduler.h"

namespace aegis {
namespace control {

/**
 * @brief Status lifecycle of an operator-dispatched manual corridor.
 */
enum class ManualCorridorStatus {
    ACTIVE,
    PAUSED,
    CANCELLED,
    COMPLETED
};

inline std::string ManualCorridorStatusToString(ManualCorridorStatus status) {
    switch (status) {
        case ManualCorridorStatus::ACTIVE:    return "ACTIVE";
        case ManualCorridorStatus::PAUSED:    return "PAUSED";
        case ManualCorridorStatus::CANCELLED: return "CANCELLED";
        case ManualCorridorStatus::COMPLETED: return "COMPLETED";
        default:                              return "UNKNOWN";
    }
}

/**
 * @brief Represents a manually configured or overridden green wave corridor.
 */
struct ManualCorridor {
    std::string corridorId{""};
    std::string ambulanceId{""};
    std::vector<std::string> route{};
    size_t currentIntersectionIndex{0};
    ManualCorridorStatus status{ManualCorridorStatus::ACTIVE};
    uint64_t createdAtEpochMs{0};
};

/**
 * @brief Central operator dispatch console providing complete manual override authority over the network.
 */
class EmergencyOperatorConsole {
public:
    using RecalculateCallback = std::function<void(const ManualOverrideRecord& record)>;

    EmergencyOperatorConsole(
        std::shared_ptr<ambulance::AmbulanceRegistry> registry = nullptr,
        std::shared_ptr<scheduler::PriorityScheduler> scheduler = nullptr,
        std::string operatorId = "OPERATOR_01");

    ~EmergencyOperatorConsole() = default;

    // Operator ID Configuration
    void SetOperatorId(std::string id);
    const std::string& GetOperatorId() const noexcept;

    // =========================================================================
    // Core Manual Interventions
    // =========================================================================

    /**
     * @brief Elevates ambulance priority to the next tier and sets manual override flag.
     * @param ambulanceId Target vehicle identifier.
     * @param reason Clinical or operational rationale.
     * @return true if target found and escalated; false otherwise.
     */
    bool IncreasePriority(const std::string& ambulanceId, const std::string& reason = "");

    /**
     * @brief Decreases ambulance priority to the lower tier.
     * @param ambulanceId Target vehicle identifier.
     * @param reason Operational rationale.
     * @return true if target found and de-escalated; false otherwise.
     */
    bool DecreasePriority(const std::string& ambulanceId, const std::string& reason = "");

    /**
     * @brief Manually creates and activates an emergency green corridor.
     * @param corridorId Unique identifier for the manual corridor.
     * @param ambulanceId Assigned ambulance.
     * @param route Ordered sequence of intersection IDs.
     * @param reason Dispatch rationale.
     * @return true if successfully initialized; false on invalid parameters.
     */
    bool CreateCorridorManually(const std::string& corridorId,
                                const std::string& ambulanceId,
                                const std::vector<std::string>& route,
                                const std::string& reason = "");

    /**
     * @brief Aborts and cancels an active corridor mission.
     * @param corridorId Target corridor identifier.
     * @param reason Cancellation rationale.
     * @return true if corridor existed and was cancelled; false otherwise.
     */
    bool CancelCorridor(const std::string& corridorId, const std::string& reason = "");

    /**
     * @brief Pauses preemption progression along a corridor.
     * @param corridorId Target corridor identifier.
     * @param reason Pausing rationale.
     * @return true if corridor was active and transitioned to PAUSED; false otherwise.
     */
    bool PauseCorridor(const std::string& corridorId, const std::string& reason = "");

    /**
     * @brief Resumes a paused corridor.
     * @param corridorId Target corridor identifier.
     * @param reason Resumption rationale.
     * @return true if corridor was paused and restored to ACTIVE; false otherwise.
     */
    bool ResumeCorridor(const std::string& corridorId, const std::string& reason = "");

    /**
     * @brief Manually forces an intersection signal to GREEN.
     *
     * Overrides automatic scheduler grants unconditionally.
     *
     * @param intersectionId Target roadside junction.
     * @param forAmbulanceId Optional beneficiary vehicle ID.
     * @param durationSec Duration to hold manual green.
     * @param reason Dispatch justification.
     * @return true if override established.
     */
    bool ForceGreen(const std::string& intersectionId,
                    const std::string& forAmbulanceId = "",
                    uint32_t durationSec = 30,
                    const std::string& reason = "");

    /**
     * @brief Manually forces an intersection signal to RED (hold all approaches).
     * @param intersectionId Target roadside junction.
     * @param durationSec Duration to hold manual red.
     * @param reason Safety justification.
     * @return true if override established.
     */
    bool ForceRed(const std::string& intersectionId,
                  uint32_t durationSec = 30,
                  const std::string& reason = "");

    /**
     * @brief Triggers network-wide immediate fail-safe stop.
     *
     * Cancels all active corridors, clears green grants, and halts all approaches.
     *
     * @param reason Critical justification.
     * @return true once emergency stop is verified active.
     */
    bool EmergencyStop(const std::string& reason = "OPERATOR EMERGENCY STOP TRIGGERED");

    /**
     * @brief Clears a manual force green or red override on an intersection.
     * @param intersectionId Target roadside junction.
     * @return true if override was active and cleared.
     */
    bool ClearIntersectionOverride(const std::string& intersectionId);

    /**
     * @brief Clears system-wide emergency stop state.
     */
    void ClearEmergencyStop();

    // =========================================================================
    // Status & Inspection Queries
    // =========================================================================

    bool IsIntersectionForcedGreen(const std::string& intersectionId) const;
    bool IsIntersectionForcedRed(const std::string& intersectionId) const;
    bool IsEmergencyStopActive() const noexcept;

    std::optional<ManualCorridor> GetManualCorridor(const std::string& corridorId) const;
    std::vector<ManualCorridor> GetAllManualCorridors() const;

    // =========================================================================
    // Audit Logging Queries
    // =========================================================================

    std::vector<ManualOverrideRecord> GetAuditLogs() const;
    std::vector<ManualOverrideRecord> GetLogsForAmbulance(const std::string& ambulanceId) const;
    std::vector<ManualOverrideRecord> GetLogsForIntersection(const std::string& intersectionId) const;
    size_t GetTotalActionCount() const;

    // =========================================================================
    // Subsystem Integration & Rescheduling Callbacks
    // =========================================================================

    void SetRecalculateCallback(RecalculateCallback callback);
    void TriggerRecalculation(const ManualOverrideRecord& record);

    std::shared_ptr<ambulance::AmbulanceRegistry> GetAmbulanceRegistry() const;
    std::shared_ptr<scheduler::PriorityScheduler> GetPriorityScheduler() const;

private:
    void LogAction(ManualOverrideRecord record);
    std::string GenerateOverrideId();

    mutable std::mutex m_mutex;
    std::string m_operatorId{"OPERATOR_01"};
    std::shared_ptr<ambulance::AmbulanceRegistry> m_ambulanceRegistry;
    std::shared_ptr<scheduler::PriorityScheduler> m_scheduler;
    RecalculateCallback m_recalculateCallback{nullptr};

    std::vector<ManualOverrideRecord> m_auditLogs{};
    std::unordered_map<std::string, ManualCorridor> m_corridors{};
    std::unordered_map<std::string, std::string> m_forcedGreenIntersections{}; ///< intersectionId -> ambulanceId
    std::unordered_map<std::string, bool> m_forcedRedIntersections{};          ///< intersectionId -> isForcedRed
    bool m_emergencyStopActive{false};
    uint64_t m_overrideCounter{0};
};

} // namespace control
} // namespace aegis

#endif // AEGIS_CONTROL_EMERGENCY_OPERATOR_CONSOLE_H
