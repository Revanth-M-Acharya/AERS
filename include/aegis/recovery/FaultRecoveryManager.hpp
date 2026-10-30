/**
 * @file FaultRecoveryManager.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Autonomous Real-Time Fault Recovery Engine.
 *
 * Implements deterministic autonomous recovery across five distinct failure classes:
 * 1. Communication failures (automatic socket reconnect, queue replay & resynchronization)
 * 2. Watchdog events (targeted subsystem reset without node reboot)
 * 3. Edge node reboots (automated post-boot corridor state & signal push)
 * 4. Temporary network loss (autonomous optical fallback and seamless post-restoration handoff)
 * 5. Corridor interruptions (automated alternate route replanning and orphan preemption release)
 *
 * Resumes nominal real-time operations without full system restart wherever possible.
 * Enters SAFE MODE (All-Red / Flashing Yellow) strictly as a last resort when recovery is impossible.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_RECOVERY_FAULT_RECOVERY_MANAGER_HPP
#define AEGIS_RECOVERY_FAULT_RECOVERY_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iomanip>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"

namespace aegis {
namespace recovery {

/**
 * @brief Categorization of fault classes handled by the recovery engine.
 */
enum class FaultCategory {
    COMMUNICATION_FAILURE,      ///< TCP / Socket connection dropped or packet storm
    WATCHDOG_EVENT,             ///< Subsystem heartbeat expired or relay readback mismatch
    NODE_REBOOT,                ///< Remote edge intersection node restarted
    TEMPORARY_NETWORK_LOSS,     ///< Wi-Fi link lost temporarily
    CORRIDOR_INTERRUPTION,      ///< Intersection along emergency route blocked or degraded
    FATAL_HARDWARE_FAILURE      ///< Catastrophic short circuit or permanent bus loss
};

inline const char* FaultCategoryToString(FaultCategory cat) noexcept {
    switch (cat) {
        case FaultCategory::COMMUNICATION_FAILURE:  return "COMMUNICATION_FAILURE";
        case FaultCategory::WATCHDOG_EVENT:         return "WATCHDOG_EVENT";
        case FaultCategory::NODE_REBOOT:            return "NODE_REBOOT";
        case FaultCategory::TEMPORARY_NETWORK_LOSS: return "TEMPORARY_NETWORK_LOSS";
        case FaultCategory::CORRIDOR_INTERRUPTION:  return "CORRIDOR_INTERRUPTION";
        case FaultCategory::FATAL_HARDWARE_FAILURE: return "FATAL_HARDWARE_FAILURE";
        default:                                    return "UNKNOWN";
    }
}

/**
 * @brief Deterministic recovery action executed to remediate a fault.
 */
enum class RecoveryStrategy {
    RECONNECT_SOCKET_AND_RESYNC,
    RESET_SUBSYSTEM_WITHOUT_REBOOT,
    PUSH_CORRIDOR_STATE_TO_NODE,
    SWITCH_AUTONOMOUS_LOCAL_THEN_RESYNC,
    REPLAN_CORRIDOR_AND_RELEASE_OBSOLETE,
    ENTER_SAFE_MODE_ALL_RED
};

inline const char* RecoveryStrategyToString(RecoveryStrategy strat) noexcept {
    switch (strat) {
        case RecoveryStrategy::RECONNECT_SOCKET_AND_RESYNC:          return "RECONNECT_SOCKET_AND_RESYNC";
        case RecoveryStrategy::RESET_SUBSYSTEM_WITHOUT_REBOOT:        return "RESET_SUBSYSTEM_WITHOUT_REBOOT";
        case RecoveryStrategy::PUSH_CORRIDOR_STATE_TO_NODE:          return "PUSH_CORRIDOR_STATE_TO_NODE";
        case RecoveryStrategy::SWITCH_AUTONOMOUS_LOCAL_THEN_RESYNC: return "SWITCH_AUTONOMOUS_LOCAL_THEN_RESYNC";
        case RecoveryStrategy::REPLAN_CORRIDOR_AND_RELEASE_OBSOLETE:  return "REPLAN_CORRIDOR_AND_RELEASE_OBSOLETE";
        case RecoveryStrategy::ENTER_SAFE_MODE_ALL_RED:               return "ENTER_SAFE_MODE_ALL_RED";
        default:                                                     return "UNKNOWN";
    }
}

/**
 * @brief Structured log record detailing an executed fault recovery operation.
 */
struct RecoveryAuditRecord {
    uint64_t recoveryId{0};
    FaultCategory fault{FaultCategory::COMMUNICATION_FAILURE};
    RecoveryStrategy strategy{RecoveryStrategy::RECONNECT_SOCKET_AND_RESYNC};
    std::string targetId{""};
    uint64_t timestampUs{0};
    double recoveryDurationMs{0.0};
    bool success{false};
    bool resumedWithoutSystemRestart{true};
    bool enteredSafeMode{false};
    std::string details{""};

    std::string ToJson() const {
        std::ostringstream oss;
        oss << "{"
            << "\"recoveryId\":" << recoveryId << ","
            << "\"fault\":\"" << FaultCategoryToString(fault) << "\","
            << "\"strategy\":\"" << RecoveryStrategyToString(strategy) << "\","
            << "\"targetId\":\"" << targetId << "\","
            << "\"timestampUs\":" << timestampUs << ","
            << "\"recoveryDurationMs\":" << std::fixed << std::setprecision(3) << recoveryDurationMs << ","
            << "\"success\":" << (success ? "true" : "false") << ","
            << "\"resumedWithoutRestart\":" << (resumedWithoutSystemRestart ? "true" : "false") << ","
            << "\"enteredSafeMode\":" << (enteredSafeMode ? "true" : "false") << ","
            << "\"details\":\"" << details << "\""
            << "}";
        return oss.str();
    }
};

/**
 * @class FaultRecoveryManager
 * @brief Autonomous recovery coordinator resolving system faults without full restarts.
 */
class FaultRecoveryManager {
public:
    using ReconnectAction = std::function<bool(const std::string& nodeId)>;
    using SubsystemResetAction = std::function<bool(const std::string& subsystemName)>;
    using NodeSyncAction = std::function<bool(const std::string& nodeId)>;
    using CorridorReplanAction = std::function<bool(const std::string& corridorId, const std::string& failedNodeId)>;
    using SafeModeAction = std::function<void(const std::string& targetId, const std::string& reason)>;

    FaultRecoveryManager();
    ~FaultRecoveryManager() = default;

    FaultRecoveryManager(const FaultRecoveryManager&) = delete;
    FaultRecoveryManager& operator=(const FaultRecoveryManager&) = delete;
    FaultRecoveryManager(FaultRecoveryManager&&) = delete;
    FaultRecoveryManager& operator=(FaultRecoveryManager&&) = delete;

    // 1. Recover from communication failures
    bool RecoverCommunicationFailure(const std::string& nodeId, uint64_t timestampUs = 0);

    // 2. Recover from watchdog events
    bool RecoverWatchdogEvent(const std::string& subsystemName, const std::string& reason, uint64_t timestampUs = 0);

    // 3. Recover from node reboot
    bool RecoverNodeReboot(const std::string& nodeId, uint64_t newBootTimeUs, uint64_t timestampUs = 0);

    // 4. Recover from temporary network loss
    bool RecoverTemporaryNetworkLoss(const std::string& nodeId, bool isLinkRestored, uint64_t timestampUs = 0);

    // 5. Recover from corridor interruption
    bool RecoverCorridorInterruption(const std::string& corridorId, const std::string& failedIntersectionId, uint64_t timestampUs = 0);

    // Fatal unrecoverable transition
    void TriggerSafeMode(const std::string& targetId, const std::string& reason, uint64_t timestampUs = 0);

    // Query capabilities
    bool CanResumeWithoutSystemRestart(FaultCategory fault) const noexcept;
    bool IsInSafeMode() const;
    size_t GetTotalRecoveries() const;
    size_t GetSuccessfulRecoveries() const;

    // Handlers
    void SetReconnectAction(ReconnectAction action);
    void SetSubsystemResetAction(SubsystemResetAction action);
    void SetNodeSyncAction(NodeSyncAction action);
    void SetCorridorReplanAction(CorridorReplanAction action);
    void SetSafeModeAction(SafeModeAction action);

    std::vector<RecoveryAuditRecord> GetRecoveryHistory() const;
    std::string GenerateRecoveryAuditReport() const;
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;

    mutable std::mutex m_mutex;
    uint64_t m_nextRecoveryId{1};
    bool m_inSafeMode{false};

    ReconnectAction m_reconnectAction{nullptr};
    SubsystemResetAction m_resetAction{nullptr};
    NodeSyncAction m_syncAction{nullptr};
    CorridorReplanAction m_replanAction{nullptr};
    SafeModeAction m_safeModeAction{nullptr};

    std::vector<RecoveryAuditRecord> m_records;
};

} // namespace recovery
} // namespace aegis

#endif // AEGIS_RECOVERY_FAULT_RECOVERY_MANAGER_HPP
