/**
 * @file EmergencyOperatorConsole.cpp
 * @brief Implementation of Control Center Emergency Operator Console and manual interventions.
 */

#include "control/EmergencyOperatorConsole.h"
#include <chrono>
#include <sstream>

namespace aegis {
namespace control {

EmergencyOperatorConsole::EmergencyOperatorConsole(
    std::shared_ptr<ambulance::AmbulanceRegistry> registry,
    std::shared_ptr<scheduler::PriorityScheduler> scheduler,
    std::string operatorId)
    : m_operatorId(std::move(operatorId))
    , m_ambulanceRegistry(registry ? registry : std::make_shared<ambulance::AmbulanceRegistry>())
    , m_scheduler(scheduler ? scheduler : std::make_shared<scheduler::PriorityScheduler>()) {}

void EmergencyOperatorConsole::SetOperatorId(std::string id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_operatorId = std::move(id);
}

const std::string& EmergencyOperatorConsole::GetOperatorId() const noexcept {
    return m_operatorId;
}

std::string EmergencyOperatorConsole::GenerateOverrideId() {
    std::ostringstream oss;
    oss << "OVR-" << (++m_overrideCounter);
    return oss.str();
}

void EmergencyOperatorConsole::LogAction(ManualOverrideRecord record) {
    if (record.timestampEpochMs == 0) {
        record.timestampEpochMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }
    if (record.overrideId.empty()) {
        record.overrideId = GenerateOverrideId();
    }
    m_auditLogs.push_back(record);
}

bool EmergencyOperatorConsole::IncreasePriority(const std::string& ambulanceId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto ambOpt = m_ambulanceRegistry->SearchAmbulance(ambulanceId);
    if (!ambOpt.has_value()) {
        return false;
    }

    auto amb = *ambOpt;
    medical::PriorityLevel prevPriority = amb.GetPriority();
    medical::PriorityLevel newPriority = prevPriority;

    switch (prevPriority) {
        case medical::PriorityLevel::LOW:
            newPriority = medical::PriorityLevel::MEDIUM;
            break;
        case medical::PriorityLevel::MEDIUM:
            newPriority = medical::PriorityLevel::HIGH;
            break;
        case medical::PriorityLevel::HIGH:
        case medical::PriorityLevel::CRITICAL:
        default:
            newPriority = medical::PriorityLevel::CRITICAL;
            break;
    }

    uint8_t currentOverride = amb.GetManualOverrideLevel();
    amb.SetManualOverrideLevel(static_cast<uint8_t>(currentOverride == 0 ? 1 : currentOverride + 1));
    amb.SetPriority(newPriority);
    m_ambulanceRegistry->RegisterAmbulance(amb);

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::INCREASE_PRIORITY;
    record.targetAmbulanceId = ambulanceId;
    record.previousPriority = prevPriority;
    record.newPriority = newPriority;
    record.reason = reason.empty() ? "Manual dispatcher escalation" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::DecreasePriority(const std::string& ambulanceId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto ambOpt = m_ambulanceRegistry->SearchAmbulance(ambulanceId);
    if (!ambOpt.has_value()) {
        return false;
    }

    auto amb = *ambOpt;
    medical::PriorityLevel prevPriority = amb.GetPriority();
    medical::PriorityLevel newPriority = prevPriority;

    switch (prevPriority) {
        case medical::PriorityLevel::CRITICAL:
            newPriority = medical::PriorityLevel::HIGH;
            break;
        case medical::PriorityLevel::HIGH:
            newPriority = medical::PriorityLevel::MEDIUM;
            break;
        case medical::PriorityLevel::MEDIUM:
        case medical::PriorityLevel::LOW:
        default:
            newPriority = medical::PriorityLevel::LOW;
            break;
    }

    uint8_t currentOverride = amb.GetManualOverrideLevel();
    if (currentOverride > 0) {
        amb.SetManualOverrideLevel(static_cast<uint8_t>(currentOverride - 1));
    }
    amb.SetPriority(newPriority);
    m_ambulanceRegistry->RegisterAmbulance(amb);

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::DECREASE_PRIORITY;
    record.targetAmbulanceId = ambulanceId;
    record.previousPriority = prevPriority;
    record.newPriority = newPriority;
    record.reason = reason.empty() ? "Manual dispatcher de-escalation" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::CreateCorridorManually(const std::string& corridorId,
                                                     const std::string& ambulanceId,
                                                     const std::vector<std::string>& route,
                                                     const std::string& reason) {
    if (corridorId.empty() || route.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    uint64_t nowMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    ManualCorridor corridor;
    corridor.corridorId = corridorId;
    corridor.ambulanceId = ambulanceId;
    corridor.route = route;
    corridor.currentIntersectionIndex = 0;
    corridor.status = ManualCorridorStatus::ACTIVE;
    corridor.createdAtEpochMs = nowMs;

    m_corridors[corridorId] = corridor;

    // Preempt first intersection on the corridor
    m_forcedGreenIntersections[route.front()] = ambulanceId;
    m_forcedRedIntersections.erase(route.front());

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::CREATE_CORRIDOR_MANUAL;
    record.targetCorridorId = corridorId;
    record.targetAmbulanceId = ambulanceId;
    record.targetIntersectionId = route.front();
    record.route = route;
    record.reason = reason.empty() ? "Manual corridor dispatch" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::CancelCorridor(const std::string& corridorId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end()) {
        return false;
    }

    it->second.status = ManualCorridorStatus::CANCELLED;

    // Clear forced green on the corridor route
    for (const auto& intId : it->second.route) {
        m_forcedGreenIntersections.erase(intId);
    }

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::CANCEL_CORRIDOR;
    record.targetCorridorId = corridorId;
    record.targetAmbulanceId = it->second.ambulanceId;
    record.reason = reason.empty() ? "Manual corridor abort" : reason;
    record.status = OverrideStatus::CLEARED;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::PauseCorridor(const std::string& corridorId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end() || it->second.status != ManualCorridorStatus::ACTIVE) {
        return false;
    }

    it->second.status = ManualCorridorStatus::PAUSED;

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::PAUSE_CORRIDOR;
    record.targetCorridorId = corridorId;
    record.targetAmbulanceId = it->second.ambulanceId;
    record.reason = reason.empty() ? "Manual corridor pause" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::ResumeCorridor(const std::string& corridorId, const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_corridors.find(corridorId);
    if (it == m_corridors.end() || it->second.status != ManualCorridorStatus::PAUSED) {
        return false;
    }

    it->second.status = ManualCorridorStatus::ACTIVE;

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::RESUME_CORRIDOR;
    record.targetCorridorId = corridorId;
    record.targetAmbulanceId = it->second.ambulanceId;
    record.reason = reason.empty() ? "Manual corridor resume" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::ForceGreen(const std::string& intersectionId,
                                         const std::string& forAmbulanceId,
                                         uint32_t durationSec,
                                         const std::string& reason) {
    if (intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_forcedRedIntersections.erase(intersectionId);
    m_forcedGreenIntersections[intersectionId] = forAmbulanceId.empty() ? "MANUAL_DISPATCH" : forAmbulanceId;

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::FORCE_GREEN;
    record.targetIntersectionId = intersectionId;
    record.targetAmbulanceId = forAmbulanceId;
    record.durationSeconds = durationSec;
    record.reason = reason.empty() ? "Manual Force Green directive" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::ForceRed(const std::string& intersectionId,
                                       uint32_t durationSec,
                                       const std::string& reason) {
    if (intersectionId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_forcedGreenIntersections.erase(intersectionId);
    m_forcedRedIntersections[intersectionId] = true;

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::FORCE_RED;
    record.targetIntersectionId = intersectionId;
    record.durationSeconds = durationSec;
    record.reason = reason.empty() ? "Manual Force Red directive" : reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::EmergencyStop(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);

    m_emergencyStopActive = true;
    m_forcedGreenIntersections.clear();

    for (auto& [id, corridor] : m_corridors) {
        if (corridor.status == ManualCorridorStatus::ACTIVE || corridor.status == ManualCorridorStatus::PAUSED) {
            corridor.status = ManualCorridorStatus::CANCELLED;
        }
    }

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::EMERGENCY_STOP;
    record.reason = reason;
    record.status = OverrideStatus::ACTIVE;

    LogAction(record);
    TriggerRecalculation(record);

    return true;
}

bool EmergencyOperatorConsole::ClearIntersectionOverride(const std::string& intersectionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    bool cleared = false;
    if (m_forcedGreenIntersections.erase(intersectionId) > 0) cleared = true;
    if (m_forcedRedIntersections.erase(intersectionId) > 0) cleared = true;

    if (cleared) {
        ManualOverrideRecord record;
        record.overrideId = GenerateOverrideId();
        record.operatorId = m_operatorId;
        record.type = OverrideType::FORCE_GREEN; // Reset
        record.targetIntersectionId = intersectionId;
        record.reason = "Manual override cleared - returning to automatic phasing";
        record.status = OverrideStatus::CLEARED;
        LogAction(record);
        TriggerRecalculation(record);
    }
    return cleared;
}

void EmergencyOperatorConsole::ClearEmergencyStop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_emergencyStopActive = false;

    ManualOverrideRecord record;
    record.overrideId = GenerateOverrideId();
    record.operatorId = m_operatorId;
    record.type = OverrideType::EMERGENCY_STOP;
    record.reason = "Emergency Stop Released by Operator";
    record.status = OverrideStatus::CLEARED;
    LogAction(record);
    TriggerRecalculation(record);
}

bool EmergencyOperatorConsole::IsIntersectionForcedGreen(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_forcedGreenIntersections.find(intersectionId) != m_forcedGreenIntersections.end();
}

bool EmergencyOperatorConsole::IsIntersectionForcedRed(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_emergencyStopActive) return true;
    return m_forcedRedIntersections.find(intersectionId) != m_forcedRedIntersections.end();
}

bool EmergencyOperatorConsole::IsEmergencyStopActive() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_emergencyStopActive;
}

std::optional<ManualCorridor> EmergencyOperatorConsole::GetManualCorridor(const std::string& corridorId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_corridors.find(corridorId);
    if (it != m_corridors.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<ManualCorridor> EmergencyOperatorConsole::GetAllManualCorridors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ManualCorridor> list;
    list.reserve(m_corridors.size());
    for (const auto& [id, c] : m_corridors) {
        list.push_back(c);
    }
    return list;
}

std::vector<ManualOverrideRecord> EmergencyOperatorConsole::GetAuditLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auditLogs;
}

std::vector<ManualOverrideRecord> EmergencyOperatorConsole::GetLogsForAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ManualOverrideRecord> list;
    for (const auto& log : m_auditLogs) {
        if (log.targetAmbulanceId == ambulanceId) {
            list.push_back(log);
        }
    }
    return list;
}

std::vector<ManualOverrideRecord> EmergencyOperatorConsole::GetLogsForIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ManualOverrideRecord> list;
    for (const auto& log : m_auditLogs) {
        if (log.targetIntersectionId == intersectionId) {
            list.push_back(log);
        }
    }
    return list;
}

size_t EmergencyOperatorConsole::GetTotalActionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_auditLogs.size();
}

void EmergencyOperatorConsole::SetRecalculateCallback(RecalculateCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_recalculateCallback = std::move(callback);
}

void EmergencyOperatorConsole::TriggerRecalculation(const ManualOverrideRecord& record) {
    // 1. Recalculate automatic scheduling if scheduler and registry are active
    if (m_scheduler && m_ambulanceRegistry) {
        if (m_emergencyStopActive) {
            m_scheduler->Clear();
        } else {
            auto fleet = m_ambulanceRegistry->GetAllAmbulances();
            m_scheduler->ScheduleAmbulances(fleet);

            // Enforce manual overrides directly on top of automatic scheduler
            for (const auto& [intId, forcedAmb] : m_forcedGreenIntersections) {
                // If not also forced red, manual force green wins
                if (m_forcedRedIntersections.find(intId) == m_forcedRedIntersections.end()) {
                    m_scheduler->ForceGreenGrant(intId, forcedAmb);
                }
            }

            for (const auto& [intId, isRed] : m_forcedRedIntersections) {
                m_scheduler->RevokeGreenGrant(intId);
            }
        }
    }

    // 2. Invoke user/coordinator notification callback
    if (m_recalculateCallback) {
        m_recalculateCallback(record);
    }
}

std::shared_ptr<ambulance::AmbulanceRegistry> EmergencyOperatorConsole::GetAmbulanceRegistry() const {
    return m_ambulanceRegistry;
}

std::shared_ptr<scheduler::PriorityScheduler> EmergencyOperatorConsole::GetPriorityScheduler() const {
    return m_scheduler;
}

} // namespace control
} // namespace aegis
