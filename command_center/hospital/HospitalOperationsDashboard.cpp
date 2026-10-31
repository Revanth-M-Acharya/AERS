/**
 * @file HospitalOperationsDashboard.cpp
 * @brief Implementation of Hospital Operations Dashboard.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "command_center/hospital/HospitalOperationsDashboard.hpp"
#include <algorithm>

namespace aegis::command_center {

HospitalOperationsDashboard::HospitalOperationsDashboard() {
    InitializeDefaultHospitals();
}

void HospitalOperationsDashboard::RecalculateStatus(HospitalOperationalRecord& record) {
    if (record.totalBeds > 0) {
        uint32_t occupiedBeds = (record.totalBeds >= record.availableBeds) ? (record.totalBeds - record.availableBeds) : record.totalBeds;
        record.hospitalLoadPercent = (static_cast<double>(occupiedBeds) / static_cast<double>(record.totalBeds)) * 100.0;
    } else {
        record.hospitalLoadPercent = 100.0;
    }

    if (record.hospitalLoadPercent >= 90.0 || record.availableIcuBeds == 0) {
        record.status = HospitalConditionStatus::RED;
    } else if (record.hospitalLoadPercent >= 75.0 || record.availableIcuBeds <= 2) {
        record.status = HospitalConditionStatus::YELLOW;
    } else {
        record.status = HospitalConditionStatus::GREEN;
    }
}

void HospitalOperationsDashboard::InitializeDefaultHospitals() {
    std::lock_guard<std::mutex> lock(mutex_);
    hospitals_.clear();
    displayOrder_.clear();

    auto add = [this](const std::string& id, const std::string& name,
                      uint32_t totalBeds, uint32_t availBeds,
                      uint32_t totalIcu, uint32_t availIcu,
                      uint32_t erBeds, uint32_t availEr,
                      uint32_t theatres, uint32_t availTheatres,
                      uint32_t doctors,
                      const std::vector<std::string>& incoming,
                      double arrivalSec, const std::string& priority) {
        HospitalOperationalRecord r;
        r.hospitalId = id;
        r.hospitalName = name;
        r.totalBeds = totalBeds;
        r.availableBeds = availBeds;
        r.totalIcuBeds = totalIcu;
        r.availableIcuBeds = availIcu;
        r.emergencyBeds = erBeds;
        r.availableErBeds = availEr;
        r.operatingTheatres = theatres;
        r.availableOperatingTheatres = availTheatres;
        r.doctorsAvailable = doctors;
        r.incomingAmbulances = incoming;
        r.expectedArrivalSec = arrivalSec;
        r.patientPriority = priority;
        RecalculateStatus(r);

        hospitals_[id] = r;
        displayOrder_.push_back(id);
    };

    // 6 Regional Hospitals with exactly 3 incoming patients across the network
    add("HOSP-01", "City Trauma Hospital",          120, 36, 24, 7, 20, 6, 8, 3, 16, {"AMB-01", "AMB-09"}, 42.0, "CRITICAL (Code Red)");
    add("HOSP-02", "Metro Heart Institute",         100, 28, 20, 5, 15, 4, 6, 2, 12, {"AMB-10"},           65.0, "HIGH (Code Yellow)");
    add("HOSP-03", "St. Jude Multi-Specialty",       150, 22, 30, 2, 25, 3, 10, 1, 18, {},                   0.0,  "NONE");
    add("HOSP-04", "Apex Pediatric Hospital",       80,  30, 16, 6, 12, 5, 4,  2, 10, {},                   0.0,  "NONE");
    add("HOSP-05", "Greenfield Emergency Hospital", 90,  35, 18, 5, 14, 6, 5,  3, 11, {},                   0.0,  "NONE");
    add("HOSP-06", "Central Government Hospital",   200, 15, 35, 1, 30, 1, 12, 0, 20, {},                   0.0,  "NONE");
}

bool HospitalOperationsDashboard::RegisterHospital(const HospitalOperationalRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto rec = record;
    RecalculateStatus(rec);
    if (hospitals_.find(rec.hospitalId) == hospitals_.end()) {
        displayOrder_.push_back(rec.hospitalId);
    }
    hospitals_[rec.hospitalId] = rec;
    return true;
}

bool HospitalOperationsDashboard::UpdateCapacity(const std::string& hospitalId,
                                                uint32_t availBeds,
                                                uint32_t availIcu,
                                                uint32_t availEr,
                                                uint32_t availTheatres,
                                                uint32_t doctors) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    it->second.availableBeds = availBeds;
    it->second.availableIcuBeds = availIcu;
    it->second.availableErBeds = availEr;
    it->second.availableOperatingTheatres = availTheatres;
    it->second.doctorsAvailable = doctors;
    RecalculateStatus(it->second);
    return true;
}

bool HospitalOperationsDashboard::AddIncomingAmbulance(const std::string& hospitalId,
                                                      const std::string& ambulanceId,
                                                      double expectedArrivalSec,
                                                      const std::string& patientPriority) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    if (std::find(it->second.incomingAmbulances.begin(), it->second.incomingAmbulances.end(), ambulanceId) == it->second.incomingAmbulances.end()) {
        it->second.incomingAmbulances.push_back(ambulanceId);
    }
    it->second.expectedArrivalSec = expectedArrivalSec;
    it->second.patientPriority = patientPriority;
    return true;
}

bool HospitalOperationsDashboard::RemoveIncomingAmbulance(const std::string& hospitalId, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return false;
    }
    auto pos = std::find(it->second.incomingAmbulances.begin(), it->second.incomingAmbulances.end(), ambulanceId);
    if (pos != it->second.incomingAmbulances.end()) {
        it->second.incomingAmbulances.erase(pos);
        if (it->second.incomingAmbulances.empty()) {
            it->second.expectedArrivalSec = 0.0;
            it->second.patientPriority = "NONE";
        }
        return true;
    }
    return false;
}

std::optional<HospitalOperationalRecord> HospitalOperationsDashboard::GetHospital(const std::string& hospitalId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<HospitalOperationalRecord> HospitalOperationsDashboard::GetAllHospitals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HospitalOperationalRecord> results;
    results.reserve(displayOrder_.size());
    for (const auto& id : displayOrder_) {
        auto it = hospitals_.find(id);
        if (it != hospitals_.end()) {
            results.push_back(it->second);
        }
    }
    return results;
}

size_t HospitalOperationsDashboard::GetTotalIncomingPatientsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [_, h] : hospitals_) {
        count += h.incomingAmbulances.size();
    }
    return count;
}

size_t HospitalOperationsDashboard::GetHospitalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hospitals_.size();
}

void HospitalOperationsDashboard::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    hospitals_.clear();
    displayOrder_.clear();
}

} // namespace aegis::command_center
