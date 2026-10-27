/**
 * @file ManualOverrideManager.cpp
 * @brief Implementation of ManualOverrideManager operator interventions and mandatory audit logging.
 */

#include "scheduler/ManualOverrideManager.h"
#include <chrono>
#include <sstream>
#include <iomanip>

namespace aegis {
namespace scheduler {

std::string OperatorOverride::ToString() const {
    std::ostringstream oss;
    oss << "OperatorOverride [" << overrideId << "]\n"
        << "  Ambulance    : " << ambulanceId << "\n"
        << "  Operator     : " << operatorId << "\n"
        << "  Level        : " << static_cast<int>(overrideLevel) << "\n"
        << "  Priority     : " << medical::PriorityLevelToString(assignedPriority) << "\n"
        << "  Score        : " << assignedScore << "\n"
        << "  Intersection : " << (intersectionId.empty() ? "None" : intersectionId) << "\n"
        << "  Active       : " << (isActive ? "YES" : "NO") << "\n"
        << "  Reason       : " << reason;
    return oss.str();
}

std::string OperatorOverride::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"override_id\":\"" << overrideId << "\","
        << "\"ambulance_id\":\"" << ambulanceId << "\","
        << "\"intersection_id\":\"" << intersectionId << "\","
        << "\"operator_id\":\"" << operatorId << "\","
        << "\"override_level\":" << static_cast<int>(overrideLevel) << ","
        << "\"priority\":\"" << medical::PriorityLevelToString(assignedPriority) << "\","
        << "\"score\":" << std::fixed << std::setprecision(2) << assignedScore << ","
        << "\"reason\":\"" << reason << "\","
        << "\"is_active\":" << (isActive ? "true" : "false") << ","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << "}";
    return oss.str();
}

uint64_t ManualOverrideManager::CurrentTimeMillis() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

ManualOverrideManager::ManualOverrideManager(std::shared_ptr<ambulance::AmbulanceRegistry> registry)
    : m_registry(std::move(registry)) {}

void ManualOverrideManager::SetRegistry(std::shared_ptr<ambulance::AmbulanceRegistry> registry) noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_registry = std::move(registry);
}

std::shared_ptr<ambulance::AmbulanceRegistry> ManualOverrideManager::GetRegistry() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_registry;
}

std::string ManualOverrideManager::GenerateOverrideId() {
    ++m_overrideSequence;
    std::ostringstream oss;
    oss << "OVR-" << m_overrideSequence;
    return oss.str();
}

bool ManualOverrideManager::ApplyOverrideLevel(
    const std::string& ambulanceId,
    uint8_t level,
    const std::string& operatorId,
    const std::string& reason) {

    if (ambulanceId.empty()) {
        return false;
    }

    OperatorOverride record;
    record.ambulanceId = ambulanceId;
    record.operatorId = operatorId;
    record.overrideLevel = level;
    record.reason = reason;
    record.timestampEpochMs = CurrentTimeMillis();
    record.isActive = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        record.overrideId = GenerateOverrideId();
        m_activeOverrides[ambulanceId] = record;
        m_overrideHistory.push_back(record);

        if (m_registry) {
            auto ambOpt = m_registry->SearchAmbulance(ambulanceId);
            if (ambOpt.has_value()) {
                auto prof = *ambOpt;
                prof.SetManualOverrideLevel(level);
                m_registry->RegisterAmbulance(prof);
            }
        }
    }

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] applied manual override level "
            << static_cast<int>(level) << " to ambulance " << ambulanceId << ". Rationale: " << reason;

    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        ambulanceId,
        "",
        logger::EventSeverity::WARNING
    );

    return true;
}

bool ManualOverrideManager::OverridePriority(
    const std::string& ambulanceId,
    medical::PriorityLevel newPriority,
    const std::string& operatorId,
    const std::string& reason) {

    if (ambulanceId.empty()) {
        return false;
    }

    OperatorOverride record;
    record.ambulanceId = ambulanceId;
    record.operatorId = operatorId;
    record.assignedPriority = newPriority;
    record.overrideLevel = 1; // elevates to override level 1
    record.reason = reason;
    record.timestampEpochMs = CurrentTimeMillis();
    record.isActive = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        record.overrideId = GenerateOverrideId();
        m_activeOverrides[ambulanceId] = record;
        m_overrideHistory.push_back(record);

        if (m_registry) {
            m_registry->UpdatePriority(ambulanceId, newPriority);
            auto ambOpt = m_registry->SearchAmbulance(ambulanceId);
            if (ambOpt.has_value()) {
                auto prof = *ambOpt;
                prof.SetManualOverrideLevel(1);
                m_registry->RegisterAmbulance(prof);
            }
        }
    }

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] manually escalated priority of "
            << ambulanceId << " to " << medical::PriorityLevelToString(newPriority)
            << ". Rationale: " << reason;

    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        ambulanceId,
        "",
        logger::EventSeverity::WARNING
    );

    return true;
}

bool ManualOverrideManager::OverridePriorityScore(
    const std::string& ambulanceId,
    double score,
    const std::string& operatorId,
    const std::string& reason) {

    if (ambulanceId.empty()) {
        return false;
    }

    OperatorOverride record;
    record.ambulanceId = ambulanceId;
    record.operatorId = operatorId;
    record.assignedScore = score;
    record.overrideLevel = 1;
    record.reason = reason;
    record.timestampEpochMs = CurrentTimeMillis();
    record.isActive = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        record.overrideId = GenerateOverrideId();
        m_activeOverrides[ambulanceId] = record;
        m_overrideHistory.push_back(record);

        if (m_registry) {
            m_registry->UpdatePriorityScore(ambulanceId, score);
            auto ambOpt = m_registry->SearchAmbulance(ambulanceId);
            if (ambOpt.has_value()) {
                auto prof = *ambOpt;
                prof.SetManualOverrideLevel(1);
                prof.SetPriorityScore(score);
                m_registry->RegisterAmbulance(prof);
            }
        }
    }

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] set clinical score override of "
            << ambulanceId << " to " << std::fixed << std::setprecision(1) << score
            << ". Rationale: " << reason;

    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        ambulanceId,
        "",
        logger::EventSeverity::WARNING
    );

    return true;
}

bool ManualOverrideManager::ForceGreenPreemption(
    const std::string& intersectionId,
    const std::string& ambulanceId,
    const std::string& operatorId,
    const std::string& reason) {

    if (intersectionId.empty()) {
        return false;
    }

    OperatorOverride record;
    record.ambulanceId = ambulanceId;
    record.intersectionId = intersectionId;
    record.operatorId = operatorId;
    record.overrideLevel = 2; // High priority force override
    record.reason = reason;
    record.timestampEpochMs = CurrentTimeMillis();
    record.isActive = true;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        record.overrideId = GenerateOverrideId();
        m_activeOverrides[ambulanceId] = record;
        m_overrideHistory.push_back(record);

        if (m_registry && !ambulanceId.empty()) {
            auto ambOpt = m_registry->SearchAmbulance(ambulanceId);
            if (ambOpt.has_value()) {
                auto prof = *ambOpt;
                prof.SetManualOverrideLevel(2);
                m_registry->RegisterAmbulance(prof);
            }
        }
    }

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] forced green preemption at "
            << intersectionId << " for ambulance " << (ambulanceId.empty() ? "ALL" : ambulanceId)
            << ". Rationale: " << reason;

    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        ambulanceId,
        intersectionId,
        logger::EventSeverity::WARNING
    );

    return true;
}

bool ManualOverrideManager::ClearOverride(
    const std::string& ambulanceId,
    const std::string& operatorId) {

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeOverrides.find(ambulanceId);
    if (it == m_activeOverrides.end()) {
        return false;
    }

    it->second.isActive = false;
    m_activeOverrides.erase(it);

    if (m_registry) {
        auto ambOpt = m_registry->SearchAmbulance(ambulanceId);
        if (ambOpt.has_value()) {
            auto prof = *ambOpt;
            prof.SetManualOverrideLevel(0);
            m_registry->RegisterAmbulance(prof);
        }
    }

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] cleared manual override for ambulance " << ambulanceId;
    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        ambulanceId,
        "",
        logger::EventSeverity::INFO
    );

    return true;
}

void ManualOverrideManager::ClearAllOverrides(const std::string& operatorId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& [id, ovr] : m_activeOverrides) {
        ovr.isActive = false;
        if (m_registry) {
            auto ambOpt = m_registry->SearchAmbulance(id);
            if (ambOpt.has_value()) {
                auto prof = *ambOpt;
                prof.SetManualOverrideLevel(0);
                m_registry->RegisterAmbulance(prof);
            }
        }
    }
    m_activeOverrides.clear();

    std::ostringstream descOss;
    descOss << "Operator [" << operatorId << "] cleared all active manual overrides across network";
    logger::EventLogger::Instance().LogManualOverride(
        descOss.str(),
        "",
        "",
        logger::EventSeverity::INFO
    );
}

bool ManualOverrideManager::HasOverride(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeOverrides.find(ambulanceId) != m_activeOverrides.end();
}

uint8_t ManualOverrideManager::GetOverrideLevel(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeOverrides.find(ambulanceId);
    if (it != m_activeOverrides.end()) {
        return it->second.overrideLevel;
    }
    return 0;
}

std::optional<OperatorOverride> ManualOverrideManager::GetOverride(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_activeOverrides.find(ambulanceId);
    if (it != m_activeOverrides.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<OperatorOverride> ManualOverrideManager::GetActiveOverrides() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<OperatorOverride> result;
    for (const auto& [id, ovr] : m_activeOverrides) {
        result.push_back(ovr);
    }
    return result;
}

std::vector<OperatorOverride> ManualOverrideManager::GetOverrideHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_overrideHistory;
}

void ManualOverrideManager::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeOverrides.clear();
    m_overrideHistory.clear();
}

} // namespace scheduler
} // namespace aegis
