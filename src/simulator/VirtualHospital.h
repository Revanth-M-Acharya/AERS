/**
 * @file VirtualHospital.h
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Hospital Resource & Capacity Digital Twin.
 */

#ifndef AEGIS_SIMULATOR_VIRTUAL_HOSPITAL_H
#define AEGIS_SIMULATOR_VIRTUAL_HOSPITAL_H

#include <string>
#include <cstdint>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Digital twin model of an acute care hospital facility.
 */
class VirtualHospital {
public:
    VirtualHospital(std::string id,
                    std::string name,
                    double lat,
                    double lon,
                    uint8_t traumaLevel = 1,
                    uint32_t totalIcuBeds = 12,
                    uint32_t totalEdBeds = 24,
                    uint32_t doctors = 8);
    ~VirtualHospital() = default;

    void Update(double dtSec);

    // Bed Reservation
    bool ReserveBed(bool requireIcu);
    void ReleaseBed(bool wasIcu);
    void SetDiversion(bool divert);

    // Getters
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    double GetLatitude() const { return m_lat; }
    double GetLongitude() const { return m_lon; }
    uint8_t GetTraumaLevel() const { return m_traumaLevel; }

    uint32_t GetAvailableIcuBeds() const;
    uint32_t GetTotalIcuBeds() const { return m_totalIcuBeds; }
    uint32_t GetAvailableEdBeds() const;
    uint32_t GetTotalEdBeds() const { return m_totalEdBeds; }
    uint32_t GetDoctorsOnDuty() const { return m_doctors; }

    bool IsDiverting() const;
    double GetLoadPercentage() const;

private:
    mutable std::mutex m_mutex;
    std::string m_id;
    std::string m_name;
    double m_lat{0.0};
    double m_lon{0.0};
    uint8_t m_traumaLevel{1};

    uint32_t m_totalIcuBeds{12};
    uint32_t m_availableIcuBeds{8};
    uint32_t m_totalEdBeds{24};
    uint32_t m_availableEdBeds{16};
    uint32_t m_doctors{8};
    bool m_isDiverting{false};

    double m_dischargeTimer{0.0};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_VIRTUAL_HOSPITAL_H

