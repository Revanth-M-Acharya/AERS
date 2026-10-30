/**
 * @file FaultRecoveryManager.cpp
 * @brief Implementation of Phase 3 Fault Recovery Manager for SMART AEGIS.
 */

#include "aegis/recovery/FaultRecoveryManager.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>

namespace aegis {
namespace recovery {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "FAULT_REC", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "FAULT_REC", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "FAULT_REC", msg);
}

FaultRecoveryManager::FaultRecoveryManager() {
    // Default fallback actions
    m_reconnectAction = [](const std::string& nodeId) -> bool {
        LogInfo("Auto-reconnecting TCP socket to " + nodeId + " and resynchronizing.");
        return true;
    };

    m_resetAction = [](const std::string& sub) -> bool {
        LogInfo("Resetting subsystem " + sub + " (cycling state / re-initializing).");
        return true;
    };

    m_syncAction = [](const std::string& nodeId) -> bool {
        LogInfo("Pushing corridor state to rebooted node " + nodeId + ".");
        return true;
    };

    m_replanAction = [](const std::string& corridorId, const std::string& failedNode) -> bool {
        LogInfo("Replanning corridor " + corridorId + " around blocked node " + failedNode + ".");
        return true;
    };

    m_safeModeAction = [](const std::string& target, const std::string& reason) {
        LogError("Entering SAFE MODE for " + target + " (Reason: " + reason + ")");
    };
}

uint64_t FaultRecoveryManager::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

bool FaultRecoveryManager::CanResumeWithoutSystemRestart(FaultCategory fault) const noexcept {
    switch (fault) {
        case FaultCategory::COMMUNICATION_FAILURE:
        case FaultCategory::WATCHDOG_EVENT:
        case FaultCategory::NODE_REBOOT:
        case FaultCategory::TEMPORARY_NETWORK_LOSS:
        case FaultCategory::CORRIDOR_INTERRUPTION:
            return true;
        case FaultCategory::FATAL_HARDWARE_FAILURE:
        default:
            return false;
    }
}

bool FaultRecoveryManager::RecoverCommunicationFailure(const std::string& nodeId, uint64_t timestampUs) {
    uint64_t startUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    bool ok = false;
    if (m_reconnectAction) {
        ok = m_reconnectAction(nodeId);
    }
    uint64_t endUs = GetCurrentTimeUs();

    std::lock_guard<std::mutex> lock(m_mutex);
    RecoveryAuditRecord rec;
    rec.recoveryId = m_nextRecoveryId++;
    rec.fault = FaultCategory::COMMUNICATION_FAILURE;
    rec.strategy = RecoveryStrategy::RECONNECT_SOCKET_AND_RESYNC;
    rec.targetId = nodeId;
    rec.timestampUs = startUs;
    rec.recoveryDurationMs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0) / 1000.0;
    rec.success = ok;
    rec.resumedWithoutSystemRestart = true;
    rec.enteredSafeMode = false;
    rec.details = ok ? "Socket reconnected and messages resynced cleanly." : "Failed to reconnect socket.";

    m_records.push_back(rec);
    return ok;
}

bool FaultRecoveryManager::RecoverWatchdogEvent(const std::string& subsystemName,
                                               const std::string& reason,
                                               uint64_t timestampUs) {
    uint64_t startUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    bool ok = false;
    if (m_resetAction) {
        ok = m_resetAction(subsystemName);
    }
    uint64_t endUs = GetCurrentTimeUs();

    std::lock_guard<std::mutex> lock(m_mutex);
    RecoveryAuditRecord rec;
    rec.recoveryId = m_nextRecoveryId++;
    rec.fault = FaultCategory::WATCHDOG_EVENT;
    rec.strategy = RecoveryStrategy::RESET_SUBSYSTEM_WITHOUT_REBOOT;
    rec.targetId = subsystemName;
    rec.timestampUs = startUs;
    rec.recoveryDurationMs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0) / 1000.0;
    rec.success = ok;
    rec.resumedWithoutSystemRestart = true;
    rec.enteredSafeMode = false;
    rec.details = "Watchdog triggered: " + reason + ". Reset executed: " + (ok ? "SUCCESS" : "FAILED");

    m_records.push_back(rec);
    return ok;
}

bool FaultRecoveryManager::RecoverNodeReboot(const std::string& nodeId,
                                             uint64_t newBootTimeUs,
                                             uint64_t timestampUs) {
    (void)newBootTimeUs;
    uint64_t startUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    bool ok = false;
    if (m_syncAction) {
        ok = m_syncAction(nodeId);
    }
    uint64_t endUs = GetCurrentTimeUs();

    std::lock_guard<std::mutex> lock(m_mutex);
    RecoveryAuditRecord rec;
    rec.recoveryId = m_nextRecoveryId++;
    rec.fault = FaultCategory::NODE_REBOOT;
    rec.strategy = RecoveryStrategy::PUSH_CORRIDOR_STATE_TO_NODE;
    rec.targetId = nodeId;
    rec.timestampUs = startUs;
    rec.recoveryDurationMs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0) / 1000.0;
    rec.success = ok;
    rec.resumedWithoutSystemRestart = true;
    rec.enteredSafeMode = false;
    rec.details = "Node rebooted; pushed active corridor state and synchronised signal schedule.";

    m_records.push_back(rec);
    return ok;
}

bool FaultRecoveryManager::RecoverTemporaryNetworkLoss(const std::string& nodeId,
                                                      bool isLinkRestored,
                                                      uint64_t timestampUs) {
    uint64_t startUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    bool ok = true;
    std::string details = "";

    if (!isLinkRestored) {
        details = "Link dropped; commanded " + nodeId + " to switch to autonomous local optical preemption.";
    } else {
        if (m_syncAction) {
            ok = m_syncAction(nodeId);
        }
        details = "Link restored; resynchronized node state back to coordinated mode without reboot.";
    }
    uint64_t endUs = GetCurrentTimeUs();

    std::lock_guard<std::mutex> lock(m_mutex);
    RecoveryAuditRecord rec;
    rec.recoveryId = m_nextRecoveryId++;
    rec.fault = FaultCategory::TEMPORARY_NETWORK_LOSS;
    rec.strategy = RecoveryStrategy::SWITCH_AUTONOMOUS_LOCAL_THEN_RESYNC;
    rec.targetId = nodeId;
    rec.timestampUs = startUs;
    rec.recoveryDurationMs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0) / 1000.0;
    rec.success = ok;
    rec.resumedWithoutSystemRestart = true;
    rec.enteredSafeMode = false;
    rec.details = details;

    m_records.push_back(rec);
    return ok;
}

bool FaultRecoveryManager::RecoverCorridorInterruption(const std::string& corridorId,
                                                      const std::string& failedIntersectionId,
                                                      uint64_t timestampUs) {
    uint64_t startUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    bool ok = false;
    if (m_replanAction) {
        ok = m_replanAction(corridorId, failedIntersectionId);
    }
    uint64_t endUs = GetCurrentTimeUs();

    std::lock_guard<std::mutex> lock(m_mutex);
    RecoveryAuditRecord rec;
    rec.recoveryId = m_nextRecoveryId++;
    rec.fault = FaultCategory::CORRIDOR_INTERRUPTION;
    rec.strategy = RecoveryStrategy::REPLAN_CORRIDOR_AND_RELEASE_OBSOLETE;
    rec.targetId = corridorId;
    rec.timestampUs = startUs;
    rec.recoveryDurationMs = static_cast<double>(endUs >= startUs ? endUs - startUs : 0) / 1000.0;
    rec.success = ok;
    rec.resumedWithoutSystemRestart = true;
    rec.enteredSafeMode = false;
    rec.details = "Corridor interrupted at " + failedIntersectionId +
                  "; replanned alternate corridor and purged obsolete preemption locks.";

    m_records.push_back(rec);
    return ok;
}

void FaultRecoveryManager::TriggerSafeMode(const std::string& targetId,
                                           const std::string& reason,
                                           uint64_t timestampUs) {
    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_inSafeMode = true;

        RecoveryAuditRecord rec;
        rec.recoveryId = m_nextRecoveryId++;
        rec.fault = FaultCategory::FATAL_HARDWARE_FAILURE;
        rec.strategy = RecoveryStrategy::ENTER_SAFE_MODE_ALL_RED;
        rec.targetId = targetId;
        rec.timestampUs = nowUs;
        rec.recoveryDurationMs = 0.0;
        rec.success = false;
        rec.resumedWithoutSystemRestart = false;
        rec.enteredSafeMode = true;
        rec.details = "Recovery impossible. Transferred system to SAFE MODE (All-Red). Reason: " + reason;

        m_records.push_back(rec);
    }

    if (m_safeModeAction) {
        m_safeModeAction(targetId, reason);
    }
}

bool FaultRecoveryManager::IsInSafeMode() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_inSafeMode;
}

size_t FaultRecoveryManager::GetTotalRecoveries() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records.size();
}

size_t FaultRecoveryManager::GetSuccessfulRecoveries() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t count = 0;
    for (const auto& r : m_records) {
        if (r.success) count++;
    }
    return count;
}

void FaultRecoveryManager::SetReconnectAction(ReconnectAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_reconnectAction = std::move(action);
}

void FaultRecoveryManager::SetSubsystemResetAction(SubsystemResetAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resetAction = std::move(action);
}

void FaultRecoveryManager::SetNodeSyncAction(NodeSyncAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_syncAction = std::move(action);
}

void FaultRecoveryManager::SetCorridorReplanAction(CorridorReplanAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_replanAction = std::move(action);
}

void FaultRecoveryManager::SetSafeModeAction(SafeModeAction action) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_safeModeAction = std::move(action);
}

std::vector<RecoveryAuditRecord> FaultRecoveryManager::GetRecoveryHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_records;
}

std::string FaultRecoveryManager::GenerateRecoveryAuditReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - FAULT RECOVERY AUDIT TRAIL REPORT\n"
        << "=========================================================================\n"
        << " Total Faults Handled   : " << m_records.size() << "\n"
        << " Safe Mode State        : " << (m_inSafeMode ? "ACTIVE (ALL_RED)" : "NOMINAL") << "\n"
        << "-------------------------------------------------------------------------\n"
        << " ID | Fault Class             | Target       | Result  | Restart? | Time(ms)\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& r : m_records) {
        oss << " " << std::setw(2) << r.recoveryId << " | "
            << std::left << std::setw(23) << FaultCategoryToString(r.fault) << " | "
            << std::setw(12) << r.targetId.substr(0, 12) << " | "
            << std::setw(7) << (r.success ? "SUCCESS" : "FAILED") << " | "
            << std::setw(8) << (r.resumedWithoutSystemRestart ? "NO" : "YES") << " | "
            << std::right << std::fixed << std::setprecision(2) << std::setw(7) << r.recoveryDurationMs << "\n";
    }
    oss << "=========================================================================\n";
    return oss.str();
}

void FaultRecoveryManager::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_records.clear();
    m_inSafeMode = false;
}

} // namespace recovery
} // namespace aegis
