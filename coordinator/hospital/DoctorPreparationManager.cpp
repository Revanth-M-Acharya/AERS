/**
 * @file DoctorPreparationManager.cpp
 * @brief Implementation of doctor readiness milestones and acknowledgement generation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/DoctorPreparationManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::hospital {

bool DoctorPreparationManager::StartPreparation(const std::string& emergencyId,
                                                const std::string& hospitalId,
                                                const std::string& leadDoctorName,
                                                const std::string& department,
                                                uint64_t monoMs) {
    if (emergencyId.empty() || hospitalId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    if (records_.find(emergencyId) != records_.end()) {
        return false;
    }

    DoctorReadinessRecord rec;
    rec.emergencyId = emergencyId;
    rec.hospitalId = hospitalId;
    rec.leadDoctorName = leadDoctorName;
    rec.assignedDepartment = department;
    rec.currentStage = PreparationStage::PREPARATION_STARTED;
    rec.completedStages.push_back(PreparationStage::PREPARATION_STARTED);
    rec.startedAtMonoMs = monoMs;
    rec.isReady = false;

    records_[emergencyId] = rec;
    return true;
}

bool DoctorPreparationManager::AdvanceStage(const std::string& emergencyId,
                                           PreparationStage stage,
                                           uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it == records_.end()) return false;

    it->second.currentStage = stage;
    it->second.completedStages.push_back(stage);

    if (stage == PreparationStage::PREPARATION_COMPLETE) {
        it->second.isReady = true;
        it->second.completedAtMonoMs = monoMs;
        it->second.readinessAckCode = "ACK-DOC-" + emergencyId;
    }

    return true;
}

bool DoctorPreparationManager::IsPreparationComplete(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) {
        return it->second.isReady;
    }
    return false;
}

std::string DoctorPreparationManager::GenerateReadinessAck(const std::string& emergencyId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) {
        if (it->second.readinessAckCode.empty()) {
            it->second.readinessAckCode = "ACK-DOC-" + emergencyId;
        }
        return it->second.readinessAckCode;
    }
    return "";
}

std::optional<DoctorReadinessRecord> DoctorPreparationManager::GetRecord(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = records_.find(emergencyId);
    if (it != records_.end()) return it->second;
    return std::nullopt;
}

void DoctorPreparationManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    records_.clear();
}

} // namespace aegis::coordinator::hospital

