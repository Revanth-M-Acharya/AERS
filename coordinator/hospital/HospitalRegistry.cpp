/**
 * @file HospitalRegistry.cpp
 * @brief Implementation of hospital directory, capability indexing, and config parsing.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/HospitalRegistry.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace aegis::coordinator::hospital {

static std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool HospitalData::HasDepartment(const std::string& dept) const {
    for (const auto& d : departments) {
        if (d == dept) return true;
    }
    return false;
}

HospitalRegistry::HospitalRegistry(std::shared_ptr<orchestration::CoordinatorStateManager> stateManager)
    : stateManager_(std::move(stateManager)) {}

bool HospitalRegistry::RegisterHospital(const HospitalData& hospital) {
    if (hospital.hospitalId.empty()) return false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        hospitals_[hospital.hospitalId] = hospital;
    }

    if (stateManager_) {
        orchestration::HospitalRecord hr;
        hr.hospitalId = hospital.hospitalId;
        hr.name = hospital.hospitalName;
        hr.location = std::to_string(hospital.latitude) + "," + std::to_string(hospital.longitude);
        hr.totalCapacity = hospital.emergencyCapacity;
        hr.availableBeds = hospital.availableBeds;
        hr.traumaLevel = hospital.traumaLevel;
        stateManager_->RegisterHospital(hr);
    }

    return true;
}

bool HospitalRegistry::UpdateStatus(const std::string& hospitalId, HospitalStatus status) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it == hospitals_.end()) return false;

    it->second.status = status;
    it->second.isAvailable = (status == HospitalStatus::ONLINE || status == HospitalStatus::EMERGENCY_ONLY);
    return true;
}

bool HospitalRegistry::UpdateBeds(const std::string& hospitalId, uint32_t availableBeds, uint32_t icuBeds) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = hospitals_.find(hospitalId);
        if (it == hospitals_.end()) return false;

        it->second.availableBeds = availableBeds;
        if (icuBeds > 0) it->second.icuBeds = icuBeds;
        if (availableBeds == 0) {
            it->second.status = HospitalStatus::FULL;
            it->second.isAvailable = false;
        }
    }

    if (stateManager_) {
        stateManager_->UpdateHospitalCapacity(hospitalId, availableBeds);
    }

    return true;
}

std::optional<HospitalData> HospitalRegistry::GetHospital(const std::string& hospitalId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) return it->second;
    return std::nullopt;
}

std::vector<HospitalData> HospitalRegistry::GetAllHospitals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HospitalData> all;
    all.reserve(hospitals_.size());
    for (const auto& [_, hosp] : hospitals_) {
        all.push_back(hosp);
    }
    return all;
}

std::vector<HospitalData> HospitalRegistry::GetHospitalsByDepartment(const std::string& department) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HospitalData> matching;
    for (const auto& [_, hosp] : hospitals_) {
        if (hosp.HasDepartment(department)) {
            matching.push_back(hosp);
        }
    }
    return matching;
}

size_t HospitalRegistry::GetHospitalCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return hospitals_.size();
}

bool HospitalRegistry::LoadFromConfigFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    std::string line;
    HospitalData current;
    bool inHospital = false;

    auto finalizeCurrent = [this, &current, &inHospital]() {
        if (inHospital && !current.hospitalId.empty()) {
            this->RegisterHospital(current);
        }
        current = HospitalData();
        inHospital = false;
    };

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line == "[HOSPITAL]") {
            finalizeCurrent();
            inHospital = true;
            continue;
        }

        if (!inHospital) continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        try {
            if (key == "id") current.hospitalId = val;
            else if (key == "name") current.hospitalName = val;
            else if (key == "latitude") current.latitude = std::stod(val);
            else if (key == "longitude") current.longitude = std::stod(val);
            else if (key == "trauma_level") current.traumaLevel = static_cast<uint32_t>(std::stoul(val));
            else if (key == "available_beds") current.availableBeds = static_cast<uint32_t>(std::stoul(val));
            else if (key == "icu_beds") current.icuBeds = static_cast<uint32_t>(std::stoul(val));
            else if (key == "ventilator_count") current.ventilatorCount = static_cast<uint32_t>(std::stoul(val));
            else if (key == "emergency_capacity") current.emergencyCapacity = static_cast<uint32_t>(std::stoul(val));
            else if (key == "avg_prep_time_sec") current.avgPrepTimeSec = static_cast<uint32_t>(std::stoul(val));
            else if (key == "is_available") current.isAvailable = (val == "true" || val == "1");
            else if (key == "status") current.status = StringToHospitalStatus(val);
            else if (key == "departments") {
                current.departments.clear();
                std::stringstream ss(val);
                std::string item;
                while (std::getline(ss, item, ',')) {
                    item = Trim(item);
                    if (!item.empty()) current.departments.push_back(item);
                }
            }
        } catch (...) {}
    }

    finalizeCurrent();
    return GetHospitalCount() > 0;
}

void HospitalRegistry::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    hospitals_.clear();
}

} // namespace aegis::coordinator::hospital

