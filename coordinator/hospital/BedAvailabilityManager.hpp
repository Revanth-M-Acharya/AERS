/**
 * @file BedAvailabilityManager.hpp
 * @brief Dynamic bed reservation, capacity tracking, and admission confirmation manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/hospital/HospitalRegistry.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

namespace aegis::coordinator::hospital {

enum class BedType {
    GENERAL_EMERGENCY,
    ICU,
    TRAUMA,
    VENTILATOR
};

inline std::string BedTypeToString(BedType type) {
    switch (type) {
        case BedType::GENERAL_EMERGENCY: return "GENERAL_EMERGENCY";
        case BedType::ICU:               return "ICU";
        case BedType::TRAUMA:            return "TRAUMA";
        case BedType::VENTILATOR:        return "VENTILATOR";
        default:                         return "UNKNOWN";
    }
}

struct BedReservation {
    std::string reservationId;
    std::string hospitalId;
    std::string emergencyId;
    BedType bedType{BedType::GENERAL_EMERGENCY};
    bool confirmedAdmission{false};
    uint64_t reservedAtMonoMs{0};
};

class BedAvailabilityManager {
public:
    explicit BedAvailabilityManager(std::shared_ptr<HospitalRegistry> registry = nullptr);
    ~BedAvailabilityManager() = default;

    /**
     * @brief Reserve a specific bed type for an incoming patient.
     */
    bool ReserveBed(const std::string& hospitalId, BedType bedType, const std::string& emergencyId, uint64_t monoMs = 0);

    /**
     * @brief Release a bed reservation (e.g. if ambulance diverted or mission cancelled).
     */
    bool ReleaseBed(const std::string& hospitalId, const std::string& emergencyId);

    /**
     * @brief Confirm patient admission upon handover into ER or ICU.
     */
    bool ConfirmAdmission(const std::string& hospitalId, const std::string& emergencyId);

    uint32_t GetActiveReservationsCount(const std::string& hospitalId, BedType bedType) const;
    std::optional<BedReservation> GetReservation(const std::string& emergencyId) const;

    void Reset();

private:
    std::shared_ptr<HospitalRegistry> registry_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, BedReservation> reservationsByEmergency_;
};

} // namespace aegis::coordinator::hospital

