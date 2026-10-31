/**
 * @file HospitalOperationsDashboard.hpp
 * @brief Real-time operational tracking of regional hospital bed capacity, trauma readiness, and inbound patient flow.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace aegis::command_center {

enum class HospitalConditionStatus {
    GREEN,  // Normal capacity (0-75% load)
    YELLOW, // High load (75-90% load)
    RED     // Critical / Diversion (>90% load)
};

inline std::string HospitalConditionStatusToString(HospitalConditionStatus status) {
    switch (status) {
        case HospitalConditionStatus::GREEN:  return "GREEN";
        case HospitalConditionStatus::YELLOW: return "YELLOW";
        case HospitalConditionStatus::RED:    return "RED";
        default:                              return "UNKNOWN";
    }
}

struct HospitalOperationalRecord {
    std::string hospitalId;
    std::string hospitalName{"City Trauma Hospital"};
    uint32_t totalBeds{120};
    uint32_t availableBeds{32};
    uint32_t totalIcuBeds{24};
    uint32_t availableIcuBeds{6};
    uint32_t emergencyBeds{18};
    uint32_t availableErBeds{5};
    uint32_t operatingTheatres{8};
    uint32_t availableOperatingTheatres{3};
    uint32_t doctorsAvailable{14};
    std::vector<std::string> incomingAmbulances;
    double expectedArrivalSec{42.0};
    std::string patientPriority{"CRITICAL (Code Red)"};
    double hospitalLoadPercent{73.3};
    HospitalConditionStatus status{HospitalConditionStatus::GREEN};
};

class HospitalOperationsDashboard {
public:
    HospitalOperationsDashboard();
    ~HospitalOperationsDashboard() = default;

    // Prevent copying
    HospitalOperationsDashboard(const HospitalOperationsDashboard&) = delete;
    HospitalOperationsDashboard& operator=(const HospitalOperationsDashboard&) = delete;

    /**
     * @brief Populate 6 regional hospitals (matching Command Center specification).
     */
    void InitializeDefaultHospitals();

    /**
     * @brief Register or update hospital status.
     */
    bool RegisterHospital(const HospitalOperationalRecord& record);

    /**
     * @brief Ingest dynamic bed/capacity update from HospitalSelectionEngine / ResourceAllocator.
     */
    bool UpdateCapacity(const std::string& hospitalId,
                        uint32_t availBeds,
                        uint32_t availIcu,
                        uint32_t availEr,
                        uint32_t availTheatres,
                        uint32_t doctors);

    /**
     * @brief Ingest inbound ambulance dispatch.
     */
    bool AddIncomingAmbulance(const std::string& hospitalId,
                             const std::string& ambulanceId,
                             double expectedArrivalSec,
                             const std::string& patientPriority);

    /**
     * @brief Remove incoming ambulance when arrival/handover completes.
     */
    bool RemoveIncomingAmbulance(const std::string& hospitalId, const std::string& ambulanceId);

    /**
     * @brief Query single hospital status.
     */
    std::optional<HospitalOperationalRecord> GetHospital(const std::string& hospitalId) const;

    /**
     * @brief Query all hospitals.
     */
    std::vector<HospitalOperationalRecord> GetAllHospitals() const;

    /**
     * @brief Total count of incoming patients across all hospitals.
     */
    size_t GetTotalIncomingPatientsCount() const;

    /**
     * @brief Total hospitals managed.
     */
    size_t GetHospitalCount() const;

    /**
     * @brief Clear all records.
     */
    void Clear();

private:
    void RecalculateStatus(HospitalOperationalRecord& record);

    mutable std::mutex mutex_;
    std::unordered_map<std::string, HospitalOperationalRecord> hospitals_;
    std::vector<std::string> displayOrder_;
};

} // namespace aegis::command_center
