/**
 * @file VirtualHospital.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Municipal Hospital Resource & Capacity Digital Twin Implementation.
 */

#include "VirtualHospital.h"
#include <algorithm>

namespace aegis {
namespace simulator {

VirtualHospital::VirtualHospital(std::string id,
                                 std::string name,
                                 double lat,
                                 double lon,
                                 uint8_t traumaLevel,
                                 uint32_t totalIcuBeds,
                                 uint32_t totalEdBeds,
                                 uint32_t doctors)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_lat(lat)
    , m_lon(lon)
    , m_traumaLevel(traumaLevel)
    , m_totalIcuBeds(totalIcuBeds)
    , m_availableIcuBeds(totalIcuBeds * 2 / 3)
    , m_totalEdBeds(totalEdBeds)
    , m_availableEdBeds(totalEdBeds * 3 / 4)
    , m_doctors(doctors)
{
}

bool VirtualHospital::ReserveBed(bool requireIcu) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isDiverting) return false;

    if (requireIcu) {
        if (m_availableIcuBeds > 0) {
            m_availableIcuBeds--;
            return true;
        }
        return false;
    } else {
        if (m_availableEdBeds > 0) {
            m_availableEdBeds--;
            return true;
        }
        return false;
    }
}

void VirtualHospital::ReleaseBed(bool wasIcu) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (wasIcu) {
        if (m_availableIcuBeds < m_totalIcuBeds) {
            m_availableIcuBeds++;
        }
    } else {
        if (m_availableEdBeds < m_totalEdBeds) {
            m_availableEdBeds++;
        }
    }
}

void VirtualHospital::SetDiversion(bool divert) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isDiverting = divert;
}

void VirtualHospital::Update(double dtSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dischargeTimer += dtSec;

    // Slowly fluctuate bed availability every 30 simulated seconds
    if (m_dischargeTimer >= 30.0) {
        m_dischargeTimer = 0.0;
        if (m_availableEdBeds < m_totalEdBeds) {
            m_availableEdBeds++;
        }
    }
}

uint32_t VirtualHospital::GetAvailableIcuBeds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_availableIcuBeds;
}

uint32_t VirtualHospital::GetAvailableEdBeds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_availableEdBeds;
}

bool VirtualHospital::IsDiverting() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isDiverting;
}

double VirtualHospital::GetLoadPercentage() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t totalBeds = m_totalIcuBeds + m_totalEdBeds;
    if (totalBeds == 0) return 0.0;
    uint32_t occupied = (m_totalIcuBeds - m_availableIcuBeds) + (m_totalEdBeds - m_availableEdBeds);
    return (static_cast<double>(occupied) / static_cast<double>(totalBeds)) * 100.0;
}

} // namespace simulator
} // namespace aegis

