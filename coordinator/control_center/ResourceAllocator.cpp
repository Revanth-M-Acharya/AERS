/**
 * @file ResourceAllocator.cpp
 * @brief Implementation of resource mutual exclusion, failover, and release.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/control_center/ResourceAllocator.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::control_center {

ResourceAllocator::ResourceAllocator(std::shared_ptr<orchestration::IEventBus> eventBus)
    : eventBus_(std::move(eventBus)) {}

bool ResourceAllocator::Allocate(const std::string& incidentId,
                                 const std::string& primaryAmbulanceId,
                                 const std::string& backupAmbulanceId,
                                 const std::string& hospitalId,
                                 const std::string& dispatcherId,
                                 const std::vector<std::string>& corridorIntersections,
                                 uint64_t monoMs) {
    if (incidentId.empty() || primaryAmbulanceId.empty() || hospitalId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if incident already has allocation
    if (allocationsByIncident_.find(incidentId) != allocationsByIncident_.end()) {
        return false;
    }

    // Check if primary ambulance is already assigned to another incident
    auto itAmb = ambulanceToIncident_.find(primaryAmbulanceId);
    if (itAmb != ambulanceToIncident_.end() && itAmb->second != incidentId) {
        return false; // Prevent double-assignment
    }

    // Check dispatcher
    if (!dispatcherId.empty()) {
        auto itDisp = dispatcherToIncident_.find(dispatcherId);
        if (itDisp != dispatcherToIncident_.end() && itDisp->second != incidentId) {
            return false;
        }
    }

    AllocationDetails details;
    details.incidentId = incidentId;
    details.primaryAmbulanceId = primaryAmbulanceId;
    details.backupAmbulanceId = backupAmbulanceId;
    details.hospitalId = hospitalId;
    details.dispatcherId = dispatcherId;
    details.corridorIntersections = corridorIntersections;
    details.allocatedAtMonoMs = monoMs;

    allocationsByIncident_[incidentId] = details;
    ambulanceToIncident_[primaryAmbulanceId] = incidentId;
    if (!dispatcherId.empty()) {
        dispatcherToIncident_[dispatcherId] = incidentId;
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::ResourceAllocated,
            incidentId,
            primaryAmbulanceId,
            "Hosp=" + hospitalId + ";Disp=" + dispatcherId,
            monoMs
        ));
    }

    return true;
}

bool ResourceAllocator::Release(const std::string& incidentId, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();
    std::string freedAmbulance;
    std::string freedHospital;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = allocationsByIncident_.find(incidentId);
        if (it == allocationsByIncident_.end()) {
            return false;
        }

        freedAmbulance = it->second.primaryAmbulanceId;
        freedHospital = it->second.hospitalId;

        ambulanceToIncident_.erase(it->second.primaryAmbulanceId);
        if (!it->second.backupAmbulanceId.empty()) {
            ambulanceToIncident_.erase(it->second.backupAmbulanceId);
        }
        if (!it->second.dispatcherId.empty()) {
            dispatcherToIncident_.erase(it->second.dispatcherId);
        }

        allocationsByIncident_.erase(it);
    }

    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::ResourceReleased,
            incidentId,
            freedAmbulance,
            "Released from Hosp=" + freedHospital,
            monoMs
        ));
    }

    return true;
}

bool ResourceAllocator::ReassignAmbulance(const std::string& incidentId,
                                         const std::string& newAmbulanceId,
                                         uint64_t monoMs) {
    if (newAmbulanceId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocationsByIncident_.find(incidentId);
    if (it == allocationsByIncident_.end()) return false;

    // Check if new ambulance is available
    auto itAmb = ambulanceToIncident_.find(newAmbulanceId);
    if (itAmb != ambulanceToIncident_.end() && itAmb->second != incidentId) {
        return false;
    }

    // Free old ambulance
    ambulanceToIncident_.erase(it->second.primaryAmbulanceId);

    // Assign new
    it->second.primaryAmbulanceId = newAmbulanceId;
    ambulanceToIncident_[newAmbulanceId] = incidentId;
    return true;
}

bool ResourceAllocator::ReassignHospital(const std::string& incidentId,
                                        const std::string& newHospitalId,
                                        uint64_t monoMs) {
    if (newHospitalId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocationsByIncident_.find(incidentId);
    if (it == allocationsByIncident_.end()) return false;

    it->second.hospitalId = newHospitalId;
    return true;
}

bool ResourceAllocator::FailoverToBackupAmbulance(const std::string& incidentId, uint64_t monoMs) {
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocationsByIncident_.find(incidentId);
    if (it == allocationsByIncident_.end()) return false;
    if (it->second.backupAmbulanceId.empty()) return false;

    std::string backup = it->second.backupAmbulanceId;
    ambulanceToIncident_.erase(it->second.primaryAmbulanceId);
    it->second.primaryAmbulanceId = backup;
    it->second.backupAmbulanceId = "";
    ambulanceToIncident_[backup] = incidentId;
    return true;
}

bool ResourceAllocator::IsAmbulanceBusy(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ambulanceToIncident_.find(ambulanceId) != ambulanceToIncident_.end();
}

bool ResourceAllocator::IsDispatcherBusy(const std::string& dispatcherId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dispatcherToIncident_.find(dispatcherId) != dispatcherToIncident_.end();
}

std::optional<AllocationDetails> ResourceAllocator::GetAllocation(const std::string& incidentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = allocationsByIncident_.find(incidentId);
    if (it != allocationsByIncident_.end()) return it->second;
    return std::nullopt;
}

void ResourceAllocator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    allocationsByIncident_.clear();
    ambulanceToIncident_.clear();
    dispatcherToIncident_.clear();
}

} // namespace aegis::coordinator::control_center

