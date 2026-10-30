/**
 * @file IncidentCommandEngine.cpp
 * @brief SMART AEGIS (AERS) Phase 12B - Real-Time Incident Command & Decision Support Engine
 *
 * Pure C++17, QNX SDP 8.0, Raspberry Pi 4 & POSIX compliant.
 * Zero third-party dependencies, thread-safe, deterministic.
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "incident_command/IncidentCommandEngine.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

namespace aegis {
namespace incident_command {

// =============================================================================
// String Conversion Utilities
// =============================================================================

std::string ToString(IncidentSeverity severity) {
    switch (severity) {
        case IncidentSeverity::NORMAL:   return "NORMAL";
        case IncidentSeverity::HIGH:     return "HIGH";
        case IncidentSeverity::CRITICAL: return "CRITICAL";
        case IncidentSeverity::DISASTER: return "DISASTER";
        default:                         return "NORMAL";
    }
}

std::string ToString(CommandDecision decision) {
    switch (decision) {
        case CommandDecision::MONITOR:                return "MONITOR";
        case CommandDecision::ESCALATE:               return "ESCALATE";
        case CommandDecision::DEPLOY_EXTRA_AMBULANCE: return "DEPLOY_EXTRA_AMBULANCE";
        case CommandDecision::REQUEST_POLICE:         return "REQUEST_POLICE";
        case CommandDecision::REQUEST_FIRE:           return "REQUEST_FIRE";
        case CommandDecision::REQUEST_DISASTER_TEAM:  return "REQUEST_DISASTER_TEAM";
        case CommandDecision::REQUEST_HELICOPTER:     return "REQUEST_HELICOPTER";
        case CommandDecision::REROUTE_ALL:            return "REROUTE_ALL";
        case CommandDecision::LOCK_CORRIDORS:         return "LOCK_CORRIDORS";
        case CommandDecision::EVACUATE_AREA:          return "EVACUATE_AREA";
        case CommandDecision::REASSIGN_HOSPITAL:      return "REASSIGN_HOSPITAL";
        default:                                      return "MONITOR";
    }
}

std::string ToString(IncidentStatus status) {
    switch (status) {
        case IncidentStatus::ACTIVE:     return "ACTIVE";
        case IncidentStatus::RESOLVED:   return "RESOLVED";
        case IncidentStatus::MONITORING: return "MONITORING";
        case IncidentStatus::ESCALATED:  return "ESCALATED";
        case IncidentStatus::CLOSED:     return "CLOSED";
        default:                         return "ACTIVE";
    }
}

std::string ToString(OperatorAction action) {
    switch (action) {
        case OperatorAction::NONE:                   return "NONE";
        case OperatorAction::MANUAL_OVERRIDE:        return "MANUAL_OVERRIDE";
        case OperatorAction::FORCE_REROUTE:          return "FORCE_REROUTE";
        case OperatorAction::FORCE_HOSPITAL_CHANGE:  return "FORCE_HOSPITAL_CHANGE";
        case OperatorAction::FORCE_AMBULANCE_CHANGE: return "FORCE_AMBULANCE_CHANGE";
        case OperatorAction::CANCEL_CORRIDOR:        return "CANCEL_CORRIDOR";
        case OperatorAction::RESTART_CORRIDOR:       return "RESTART_CORRIDOR";
        default:                                     return "NONE";
    }
}

IncidentSeverity SeverityFromString(const std::string& str) {
    if (str == "HIGH")     return IncidentSeverity::HIGH;
    if (str == "CRITICAL") return IncidentSeverity::CRITICAL;
    if (str == "DISASTER") return IncidentSeverity::DISASTER;
    return IncidentSeverity::NORMAL;
}

CommandDecision DecisionFromString(const std::string& str) {
    if (str == "ESCALATE")               return CommandDecision::ESCALATE;
    if (str == "DEPLOY_EXTRA_AMBULANCE") return CommandDecision::DEPLOY_EXTRA_AMBULANCE;
    if (str == "REQUEST_POLICE")         return CommandDecision::REQUEST_POLICE;
    if (str == "REQUEST_FIRE")           return CommandDecision::REQUEST_FIRE;
    if (str == "REQUEST_DISASTER_TEAM")  return CommandDecision::REQUEST_DISASTER_TEAM;
    if (str == "REQUEST_HELICOPTER")     return CommandDecision::REQUEST_HELICOPTER;
    if (str == "REROUTE_ALL")            return CommandDecision::REROUTE_ALL;
    if (str == "LOCK_CORRIDORS")         return CommandDecision::LOCK_CORRIDORS;
    if (str == "EVACUATE_AREA")          return CommandDecision::EVACUATE_AREA;
    if (str == "REASSIGN_HOSPITAL")      return CommandDecision::REASSIGN_HOSPITAL;
    return CommandDecision::MONITOR;
}

IncidentStatus StatusFromString(const std::string& str) {
    if (str == "RESOLVED")   return IncidentStatus::RESOLVED;
    if (str == "MONITORING") return IncidentStatus::MONITORING;
    if (str == "ESCALATED")  return IncidentStatus::ESCALATED;
    if (str == "CLOSED")     return IncidentStatus::CLOSED;
    return IncidentStatus::ACTIVE;
}

OperatorAction ActionFromString(const std::string& str) {
    if (str == "MANUAL_OVERRIDE")        return OperatorAction::MANUAL_OVERRIDE;
    if (str == "FORCE_REROUTE")          return OperatorAction::FORCE_REROUTE;
    if (str == "FORCE_HOSPITAL_CHANGE")  return OperatorAction::FORCE_HOSPITAL_CHANGE;
    if (str == "FORCE_AMBULANCE_CHANGE") return OperatorAction::FORCE_AMBULANCE_CHANGE;
    if (str == "CANCEL_CORRIDOR")        return OperatorAction::CANCEL_CORRIDOR;
    if (str == "RESTART_CORRIDOR")       return OperatorAction::RESTART_CORRIDOR;
    return OperatorAction::NONE;
}

// =============================================================================
// Helper Methods
// =============================================================================

uint64_t IncidentCommandEngine::GetCurrentTimeMs() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

std::string IncidentCommandEngine::FormatTimestamp(uint64_t tsMs) {
    std::time_t rawTime = static_cast<std::time_t>(tsMs / 1000);
    uint32_t millis = static_cast<uint32_t>(tsMs % 1000);
    std::tm timeInfo{};
#if defined(_WIN32)
    localtime_s(&timeInfo, &rawTime);
#else
    localtime_r(&rawTime, &timeInfo);
#endif
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << timeInfo.tm_hour << ":"
        << std::setw(2) << timeInfo.tm_min << ":"
        << std::setw(2) << timeInfo.tm_sec << "."
        << std::setw(3) << millis;
    return oss.str();
}

// =============================================================================
// Constructor & Destructor
// =============================================================================

IncidentCommandEngine::IncidentCommandEngine() {
    statistics_ = IncidentStatistics{};
}

// =============================================================================
// Incident Lifecycle Management
// =============================================================================

bool IncidentCommandEngine::CreateIncident(const IncidentRecord& record) {
    if (record.incidentId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> regLock(registryMutex_);
    if (incidents_.find(record.incidentId) != incidents_.end() ||
        archivedIncidents_.find(record.incidentId) != archivedIncidents_.end()) {
        return false;
    }

    IncidentRecord inc = record;
    uint64_t now = GetCurrentTimeMs();
    if (inc.creationTimestamp == 0) {
        inc.creationTimestamp = now;
    }
    if (inc.lastUpdateTimestamp == 0) {
        inc.lastUpdateTimestamp = inc.creationTimestamp;
    }

    incidents_[inc.incidentId] = inc;

    // Update Statistics
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        statistics_.totalIncidents++;
        statistics_.activeIncidents++;
        if (inc.severity == IncidentSeverity::CRITICAL) {
            statistics_.criticalIncidents++;
        } else if (inc.severity == IncidentSeverity::DISASTER) {
            statistics_.disasters++;
        }
    }

    // Record initial registration in decision timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, inc.incidentId, inc.currentDecision, OperatorAction::NONE, inc.operatorName, "Incident registered in City Command Center");
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[inc.incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::ResolveIncident(const std::string& incidentId) {
    uint64_t now = GetCurrentTimeMs();
    double durationMin = 0.0;
    bool wasActive = false;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        if (it->second.status == IncidentStatus::RESOLVED || it->second.status == IncidentStatus::CLOSED) {
            return false;
        }

        wasActive = (it->second.status == IncidentStatus::ACTIVE ||
                     it->second.status == IncidentStatus::ESCALATED ||
                     it->second.status == IncidentStatus::MONITORING);

        it->second.status = IncidentStatus::RESOLVED;
        it->second.resolutionTimestamp = now;
        it->second.lastUpdateTimestamp = now;
        it->second.currentDecision = CommandDecision::MONITOR;

        if (now >= it->second.creationTimestamp) {
            durationMin = static_cast<double>(now - it->second.creationTimestamp) / 60000.0;
        }
    }

    // Update Statistics
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        if (wasActive && statistics_.activeIncidents > 0) {
            statistics_.activeIncidents--;
        }
        statistics_.resolvedIncidents++;
        totalResolutionTimeMinutes_ += durationMin;
        if (statistics_.resolvedIncidents > 0) {
            statistics_.averageResolutionTime = totalResolutionTimeMinutes_ / static_cast<double>(statistics_.resolvedIncidents);
        }
    }

    // Record in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, CommandDecision::MONITOR, OperatorAction::NONE, "SYSTEM_AUTOMATION", "Incident resolved; operations normal");
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::CloseIncident(const std::string& incidentId) {
    uint64_t now = GetCurrentTimeMs();
    bool wasActive = false;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        if (it->second.status == IncidentStatus::CLOSED) {
            return false;
        }

        wasActive = (it->second.status == IncidentStatus::ACTIVE ||
                     it->second.status == IncidentStatus::ESCALATED ||
                     it->second.status == IncidentStatus::MONITORING);

        it->second.status = IncidentStatus::CLOSED;
        it->second.lastUpdateTimestamp = now;
    }

    if (wasActive) {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        if (statistics_.activeIncidents > 0) {
            statistics_.activeIncidents--;
        }
    }

    // Record in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, CommandDecision::MONITOR, OperatorAction::NONE, "SYSTEM_AUTOMATION", "Incident closed");
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::EscalateIncident(const std::string& incidentId, const std::string& reason) {
    uint64_t now = GetCurrentTimeMs();
    IncidentSeverity newSev = IncidentSeverity::NORMAL;
    std::string opName = "COMMANDER";

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        switch (it->second.severity) {
            case IncidentSeverity::NORMAL:
                it->second.severity = IncidentSeverity::HIGH;
                break;
            case IncidentSeverity::HIGH:
                it->second.severity = IncidentSeverity::CRITICAL;
                break;
            case IncidentSeverity::CRITICAL:
            case IncidentSeverity::DISASTER:
                it->second.severity = IncidentSeverity::DISASTER;
                break;
        }

        it->second.status = IncidentStatus::ESCALATED;
        it->second.currentDecision = CommandDecision::ESCALATE;
        it->second.lastUpdateTimestamp = now;
        newSev = it->second.severity;
        opName = it->second.operatorName;
    }

    // Update Statistics
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        if (newSev == IncidentSeverity::CRITICAL) {
            statistics_.criticalIncidents++;
        } else if (newSev == IncidentSeverity::DISASTER) {
            statistics_.disasters++;
        }
    }

    // Record in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        std::string rsn = reason.empty() ? ("Escalated to " + ToString(newSev)) : reason;
        DecisionHistory dh(now, incidentId, CommandDecision::ESCALATE, OperatorAction::NONE, opName, rsn);
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::DeEscalateIncident(const std::string& incidentId, const std::string& reason) {
    uint64_t now = GetCurrentTimeMs();
    IncidentSeverity newSev = IncidentSeverity::NORMAL;
    std::string opName = "COMMANDER";

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        switch (it->second.severity) {
            case IncidentSeverity::DISASTER:
                it->second.severity = IncidentSeverity::CRITICAL;
                break;
            case IncidentSeverity::CRITICAL:
                it->second.severity = IncidentSeverity::HIGH;
                break;
            case IncidentSeverity::HIGH:
            case IncidentSeverity::NORMAL:
                it->second.severity = IncidentSeverity::NORMAL;
                break;
        }

        it->second.status = IncidentStatus::ACTIVE;
        it->second.lastUpdateTimestamp = now;
        newSev = it->second.severity;
        opName = it->second.operatorName;
    }

    // Record in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        std::string rsn = reason.empty() ? ("De-escalated to " + ToString(newSev)) : reason;
        DecisionHistory dh(now, incidentId, CommandDecision::MONITOR, OperatorAction::NONE, opName, rsn);
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::AssignCommander(const std::string& incidentId, const std::string& commander) {
    uint64_t now = GetCurrentTimeMs();
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }

    it->second.assignedCommander = commander;
    it->second.lastUpdateTimestamp = now;

    std::lock_guard<std::mutex> histLock(historyMutex_);
    DecisionHistory dh(now, incidentId, it->second.currentDecision, OperatorAction::NONE, it->second.operatorName, "Commander assigned: " + commander);
    dh.formattedTime = FormatTimestamp(now);
    decisionHistory_.push_back(dh);
    incidentDecisionMap_[incidentId].push_back(dh);

    return true;
}

bool IncidentCommandEngine::AssignMission(const std::string& incidentId, const std::string& missionId) {
    uint64_t now = GetCurrentTimeMs();
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }

    it->second.missionId = missionId;
    it->second.lastUpdateTimestamp = now;

    std::lock_guard<std::mutex> histLock(historyMutex_);
    DecisionHistory dh(now, incidentId, it->second.currentDecision, OperatorAction::NONE, it->second.operatorName, "Mission bound: " + missionId);
    dh.formattedTime = FormatTimestamp(now);
    decisionHistory_.push_back(dh);
    incidentDecisionMap_[incidentId].push_back(dh);

    return true;
}

bool IncidentCommandEngine::UpdateSeverity(const std::string& incidentId, IncidentSeverity severity) {
    uint64_t now = GetCurrentTimeMs();
    IncidentSeverity oldSev = IncidentSeverity::NORMAL;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        oldSev = it->second.severity;
        it->second.severity = severity;
        it->second.lastUpdateTimestamp = now;
    }

    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        if (oldSev != IncidentSeverity::CRITICAL && severity == IncidentSeverity::CRITICAL) {
            statistics_.criticalIncidents++;
        }
        if (oldSev != IncidentSeverity::DISASTER && severity == IncidentSeverity::DISASTER) {
            statistics_.disasters++;
        }
    }

    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, CommandDecision::MONITOR, OperatorAction::NONE, "SYSTEM", "Severity updated: " + ToString(oldSev) + " -> " + ToString(severity));
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::UpdateStatus(const std::string& incidentId, IncidentStatus status) {
    uint64_t now = GetCurrentTimeMs();
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }

    it->second.status = status;
    it->second.lastUpdateTimestamp = now;

    std::lock_guard<std::mutex> histLock(historyMutex_);
    DecisionHistory dh(now, incidentId, it->second.currentDecision, OperatorAction::NONE, it->second.operatorName, "Status updated to " + ToString(status));
    dh.formattedTime = FormatTimestamp(now);
    decisionHistory_.push_back(dh);
    incidentDecisionMap_[incidentId].push_back(dh);

    return true;
}

// =============================================================================
// Automatic Decisions & Operator Intervention
// =============================================================================

CommandDecision IncidentCommandEngine::EvaluateAutomaticDecisionInternal(const IncidentRecord& record, std::string& outReason) const {
    // Rule 2: Mass casualty / Disaster -> Deploy disaster response
    if (record.isMassCasualty || record.severity == IncidentSeverity::DISASTER || record.affectedPopulation >= 100) {
        outReason = "Rule 2: Mass casualty / disaster detected -> Deploy disaster response";
        return CommandDecision::REQUEST_DISASTER_TEAM;
    }

    // Rule 6: Fire detected -> Notify fire department
    if (record.hasFire ||
        record.notes.find("fire") != std::string::npos ||
        record.notes.find("FIRE") != std::string::npos) {
        outReason = "Rule 6: Fire detected -> Notify fire department";
        return CommandDecision::REQUEST_FIRE;
    }

    // Rule 3: Traffic collapse -> Force reroute
    if (record.trafficCollapse) {
        outReason = "Rule 3: Traffic collapse -> Force reroute";
        return CommandDecision::REROUTE_ALL;
    }

    // Rule 8: Large congestion -> Lock green corridors
    if (record.largeCongestion) {
        outReason = "Rule 8: Large congestion -> Lock green corridors";
        return CommandDecision::LOCK_CORRIDORS;
    }

    // Rule 5: Ambulance failure -> Deploy backup ambulance
    if (record.ambulanceFailed) {
        outReason = "Rule 5: Ambulance failure -> Deploy backup ambulance";
        return CommandDecision::DEPLOY_EXTRA_AMBULANCE;
    }

    // Rule 4: Hospital unavailable -> Reassign hospital
    if (record.hospitalUnavailable) {
        outReason = "Rule 4: Hospital unavailable -> Reassign hospital";
        return CommandDecision::REASSIGN_HOSPITAL;
    }

    // Rule 7: Incident affecting multiple missions -> Escalate
    if (record.multipleMissionsAffected) {
        outReason = "Rule 7: Incident affecting multiple missions -> Escalate";
        return CommandDecision::ESCALATE;
    }

    // Rule 1: Critical mission -> Deploy police automatically
    if (record.severity == IncidentSeverity::CRITICAL ||
        record.priority == "CRITICAL" ||
        record.priority == "HIGH_CRITICAL") {
        outReason = "Rule 1: Critical mission -> Deploy police automatically";
        return CommandDecision::REQUEST_POLICE;
    }

    // Default monitoring
    outReason = "Normal operations -> Continuous monitoring";
    return CommandDecision::MONITOR;
}

CommandDecision IncidentCommandEngine::GenerateAutomaticDecision(const std::string& incidentId) {
    uint64_t now = GetCurrentTimeMs();
    CommandDecision decision = CommandDecision::MONITOR;
    std::string reason;

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return CommandDecision::MONITOR;
        }

        std::lock_guard<std::mutex> decLock(decisionMutex_);
        decision = EvaluateAutomaticDecisionInternal(it->second, reason);
        it->second.currentDecision = decision;
        it->second.lastUpdateTimestamp = now;
    }

    // Update Statistics
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        statistics_.automaticDecisions++;
    }

    // Rule 10: Every command decision stored in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, decision, OperatorAction::NONE, "SYSTEM_AUTOMATION", reason);
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return decision;
}

bool IncidentCommandEngine::ApplyDecision(const std::string& incidentId,
                                          CommandDecision decision,
                                          const std::string& reason,
                                          const std::string& operatorName) {
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        std::lock_guard<std::mutex> decLock(decisionMutex_);
        it->second.currentDecision = decision;
        it->second.operatorName = operatorName.empty() ? "COMMANDER" : operatorName;
        it->second.lastUpdateTimestamp = now;
    }

    // Rule 10: Store decision in timeline
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, decision, OperatorAction::NONE, operatorName, reason);
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

bool IncidentCommandEngine::ManualOverride(const std::string& incidentId,
                                           OperatorAction action,
                                           CommandDecision decision,
                                           const std::string& reason,
                                           const std::string& operatorName) {
    uint64_t now = GetCurrentTimeMs();

    {
        std::lock_guard<std::mutex> regLock(registryMutex_);
        auto it = incidents_.find(incidentId);
        if (it == incidents_.end()) {
            return false;
        }

        std::lock_guard<std::mutex> opLock(operatorMutex_);
        it->second.currentDecision = decision;
        it->second.operatorName = operatorName.empty() ? "DISPATCHER" : operatorName;
        it->second.lastUpdateTimestamp = now;
    }

    // Update Statistics (Manual override counter)
    {
        std::lock_guard<std::mutex> statLock(statisticsMutex_);
        statistics_.manualOverrides++;
    }

    // Rule 9: Manual override always logged.
    // Rule 10: Every command decision stored in timeline.
    {
        std::lock_guard<std::mutex> histLock(historyMutex_);
        DecisionHistory dh(now, incidentId, decision, action, operatorName, reason);
        dh.formattedTime = FormatTimestamp(now);
        decisionHistory_.push_back(dh);
        incidentDecisionMap_[incidentId].push_back(dh);
    }

    return true;
}

// =============================================================================
// Queries
// =============================================================================

bool IncidentCommandEngine::GetIncident(const std::string& incidentId, IncidentRecord& outRecord) const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it != incidents_.end()) {
        outRecord = it->second;
        return true;
    }
    auto itArch = archivedIncidents_.find(incidentId);
    if (itArch != archivedIncidents_.end()) {
        outRecord = itArch->second;
        return true;
    }
    return false;
}

std::vector<IncidentRecord> IncidentCommandEngine::GetAllIncidents() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<IncidentRecord> result;
    result.reserve(incidents_.size());
    for (const auto& kv : incidents_) {
        result.push_back(kv.second);
    }
    return result;
}

std::vector<IncidentRecord> IncidentCommandEngine::GetActiveIncidents() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<IncidentRecord> result;
    for (const auto& kv : incidents_) {
        if (kv.second.status == IncidentStatus::ACTIVE ||
            kv.second.status == IncidentStatus::ESCALATED ||
            kv.second.status == IncidentStatus::MONITORING) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<IncidentRecord> IncidentCommandEngine::GetCriticalIncidents() const {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::vector<IncidentRecord> result;
    for (const auto& kv : incidents_) {
        if (kv.second.severity == IncidentSeverity::CRITICAL ||
            kv.second.severity == IncidentSeverity::DISASTER) {
            result.push_back(kv.second);
        }
    }
    return result;
}

std::vector<DecisionHistory> IncidentCommandEngine::GetDecisionHistory(const std::string& incidentId) const {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = incidentDecisionMap_.find(incidentId);
    if (it != incidentDecisionMap_.end()) {
        return it->second;
    }
    return {};
}

std::vector<DecisionHistory> IncidentCommandEngine::GetAllDecisionHistory() const {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    return decisionHistory_;
}

// =============================================================================
// Reports & Situational Dashboards
// =============================================================================

std::string IncidentCommandEngine::GenerateIncidentReport(const std::string& incidentId) const {
    std::lock_guard<std::mutex> repLock(reportMutex_);
    IncidentRecord inc;
    if (!GetIncident(incidentId, inc)) {
        return "[INCIDENT]\n\nError: Incident not found: " + incidentId + "\n";
    }

    std::ostringstream oss;
    oss << "[INCIDENT]\n\n"
        << "Incident:\n"
        << inc.incidentId << "\n\n"
        << "Mission:\n"
        << inc.missionId << "\n\n"
        << "Severity:\n"
        << ToString(inc.severity) << "\n\n"
        << "Commander:\n"
        << inc.assignedCommander << "\n\n"
        << "Decision:\n"
        << ToString(inc.currentDecision) << "\n\n"
        << "Status:\n"
        << ToString(inc.status) << "\n\n"
        << "Operator:\n"
        << inc.operatorName << "\n\n"
        << "Created:\n"
        << inc.creationTimestamp << "\n\n"
        << "Last Update:\n"
        << inc.lastUpdateTimestamp << "\n";

    return oss.str();
}

std::string IncidentCommandEngine::GenerateCitySituationReport() const {
    std::lock_guard<std::mutex> repLock(reportMutex_);
    IncidentStatistics stats = GetStatistics();

    std::ostringstream oss;
    oss << "==========================================================\n"
        << " CITY SITUATION REPORT (INCIDENT COMMAND CENTER)\n"
        << "==========================================================\n"
        << " Total Incidents:       " << stats.totalIncidents << "\n"
        << " Active Incidents:      " << stats.activeIncidents << "\n"
        << " Resolved Incidents:    " << stats.resolvedIncidents << "\n"
        << " Critical Incidents:    " << stats.criticalIncidents << "\n"
        << " Disasters:             " << stats.disasters << "\n"
        << " Manual Overrides:      " << stats.manualOverrides << "\n"
        << " Automatic Decisions:   " << stats.automaticDecisions << "\n"
        << " Avg Resolution Time:   " << std::fixed << std::setprecision(2) << stats.averageResolutionTime << " min\n"
        << "==========================================================\n";

    return oss.str();
}

std::string IncidentCommandEngine::GenerateDecisionTimeline(const std::string& incidentId) const {
    std::lock_guard<std::mutex> repLock(reportMutex_);
    auto history = GetDecisionHistory(incidentId);

    std::ostringstream oss;
    oss << "[DECISION TIMELINE: " << incidentId << "]\n";
    for (const auto& entry : history) {
        oss << "[" << entry.timestamp << "] DECISION: " << ToString(entry.decision);
        if (entry.action != OperatorAction::NONE) {
            oss << " | ACTION: " << ToString(entry.action);
        }
        oss << " | OPERATOR: " << entry.operatorName
            << " | REASON: " << entry.reason << "\n";
    }

    return oss.str();
}

// =============================================================================
// Statistics & Lifecycle Utilities
// =============================================================================

IncidentStatistics IncidentCommandEngine::GetStatistics() const {
    std::lock_guard<std::mutex> statLock(statisticsMutex_);
    return statistics_;
}

bool IncidentCommandEngine::ArchiveIncident(const std::string& incidentId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }

    archivedIncidents_[incidentId] = it->second;
    incidents_.erase(it);
    return true;
}

bool IncidentCommandEngine::RestoreIncident(const std::string& incidentId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = archivedIncidents_.find(incidentId);
    if (it == archivedIncidents_.end()) {
        return false;
    }

    incidents_[incidentId] = it->second;
    archivedIncidents_.erase(it);
    return true;
}

bool IncidentCommandEngine::DeleteIncident(const std::string& incidentId) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    bool erased = false;
    if (incidents_.erase(incidentId) > 0) {
        erased = true;
    }
    if (archivedIncidents_.erase(incidentId) > 0) {
        erased = true;
    }
    return erased;
}

void IncidentCommandEngine::Reset() {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    std::lock_guard<std::mutex> histLock(historyMutex_);
    std::lock_guard<std::mutex> statLock(statisticsMutex_);

    incidents_.clear();
    archivedIncidents_.clear();
    decisionHistory_.clear();
    incidentDecisionMap_.clear();
    statistics_ = IncidentStatistics{};
    totalResolutionTimeMinutes_ = 0.0;
}

void IncidentCommandEngine::ClearHistory() {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    decisionHistory_.clear();
    incidentDecisionMap_.clear();
}

// =============================================================================
// Contextual Trigger Setters (Rule Triggers)
// =============================================================================

bool IncidentCommandEngine::SetFireDetected(const std::string& incidentId, bool detected) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.hasFire = detected;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetMassCasualty(const std::string& incidentId, bool massCasualty, uint32_t affectedPop) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.isMassCasualty = massCasualty;
    if (affectedPop > 0) {
        it->second.affectedPopulation = affectedPop;
    }
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetTrafficCollapse(const std::string& incidentId, bool collapse) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.trafficCollapse = collapse;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetHospitalUnavailable(const std::string& incidentId, bool unavailable) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.hospitalUnavailable = unavailable;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetAmbulanceFailed(const std::string& incidentId, bool failed) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.ambulanceFailed = failed;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetMultipleMissionsAffected(const std::string& incidentId, bool affected) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.multipleMissionsAffected = affected;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

bool IncidentCommandEngine::SetLargeCongestion(const std::string& incidentId, bool congestion) {
    std::lock_guard<std::mutex> regLock(registryMutex_);
    auto it = incidents_.find(incidentId);
    if (it == incidents_.end()) {
        return false;
    }
    it->second.largeCongestion = congestion;
    it->second.lastUpdateTimestamp = GetCurrentTimeMs();
    return true;
}

} // namespace incident_command
} // namespace aegis

