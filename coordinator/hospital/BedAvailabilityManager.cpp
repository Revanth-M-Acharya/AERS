/**
 * @file BedAvailabilityManager.cpp
 * @brief Implementation of bed reservation, capacity decrements, and admissions.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/BedAvailabilityManager.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::coordinator::hospital {

BedAvailabilityManager::BedAvailabilityManager(std::shared_ptr<HospitalRegistry> registry)
    : registry_(std::move(registry)) {}

bool BedAvailabilityManager::ReserveBed(const std::string& hospitalId,
                                        BedType bedType,
                                        const std::string& emergencyId,
                                        uint64_t monoMs) {
    if (hospitalId.empty() || emergencyId.empty()) return false;
    if (monoMs == 0) monoMs = common::TimeUtils::GetMonotonicTimeMs();

    std::lock_guard<std::mutex> lock(mutex_);

    // Check if already reserved
    if (reservationsByEmergency_.find(emergencyId) != reservationsByEmergency_.end()) {
        return false;
    }

    if (registry_) {
        auto optHosp = registry_->GetHospital(hospitalId);
        if (!optHosp.has_value()) return false;

        auto hosp = optHosp.value();
        if (bedType == BedType::ICU) {
            if (hosp.icuBeds == 0) return false;
            registry_->UpdateBeds(hospitalId, hosp.availableBeds, hosp.icuBeds - 1);
        } else {
            if (hosp.availableBeds == 0) return false;
            registry_->UpdateBeds(hospitalId, hosp.availableBeds - 1, hosp.icuBeds);
        }
    }

    BedReservation res;
    res.reservationId = "RES-" + emergencyId;
    res.hospitalId = hospitalId;
    res.emergencyId = emergencyId;
    res.bedType = bedType;
    res.confirmedAdmission = false;
    res.reservedAtMonoMs = monoMs;

    reservationsByEmergency_[emergencyId] = res;
    return true;
}

bool BedAvailabilityManager::ReleaseBed(const std::string& hospitalId, const std::string& emergencyId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reservationsByEmergency_.find(emergencyId);
    if (it == reservationsByEmergency_.end()) return false;

    BedType type = it->second.bedType;
    reservationsByEmergency_.erase(it);

    if (registry_) {
        auto optHosp = registry_->GetHospital(hospitalId);
        if (optHosp.has_value()) {
            auto hosp = optHosp.value();
            if (type == BedType::ICU) {
                registry_->UpdateBeds(hospitalId, hosp.availableBeds, hosp.icuBeds + 1);
            } else {
                registry_->UpdateBeds(hospitalId, hosp.availableBeds + 1, hosp.icuBeds);
            }
        }
    }

    return true;
}

bool BedAvailabilityManager::ConfirmAdmission(const std::string& /*hospitalId*/, const std::string& emergencyId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reservationsByEmergency_.find(emergencyId);
    if (it == reservationsByEmergency_.end()) return false;

    it->second.confirmedAdmission = true;
    return true;
}

uint32_t BedAvailabilityManager::GetActiveReservationsCount(const std::string& hospitalId, BedType bedType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    uint32_t count = 0;
    for (const auto& [_, res] : reservationsByEmergency_) {
        if (res.hospitalId == hospitalId && res.bedType == bedType && !res.confirmedAdmission) {
            count++;
        }
    }
    return count;
}

std::optional<BedReservation> BedAvailabilityManager::GetReservation(const std::string& emergencyId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = reservationsByEmergency_.find(emergencyId);
    if (it != reservationsByEmergency_.end()) return it->second;
    return std::nullopt;
}

void BedAvailabilityManager::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    reservationsByEmergency_.clear();
}

} // namespace aegis::coordinator::hospital

