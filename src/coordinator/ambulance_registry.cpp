#include "aegis/core/ambulance_registry.hpp"
#include "aegis/common/time_utils.hpp"

namespace aegis {
namespace core {
namespace coordinator {

bool AmbulanceRegistry::RegisterAmbulance(const AmbulanceRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (record.unitId.empty()) return false;
    m_fleet[record.unitId] = record;
    if (m_fleet[record.unitId].lastHeartbeatMs == 0) {
        m_fleet[record.unitId].lastHeartbeatMs = common::GetUtcMilliseconds();
    }
    return true;
}

common::Result<AmbulanceRecord> AmbulanceRegistry::GetAmbulance(const std::string& unitId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleet.find(unitId);
    if (it != m_fleet.end()) {
        return common::Result<AmbulanceRecord>::Ok(it->second);
    }
    return common::Result<AmbulanceRecord>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
}

std::vector<AmbulanceRecord> AmbulanceRegistry::GetAllAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AmbulanceRecord> out;
    out.reserve(m_fleet.size());
    for (const auto& kv : m_fleet) {
        out.push_back(kv.second);
    }
    return out;
}

std::vector<AmbulanceRecord> AmbulanceRegistry::GetAvailableAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AmbulanceRecord> avail;
    for (const auto& kv : m_fleet) {
        if (kv.second.state == common::AmbulanceState::AVAILABLE) {
            avail.push_back(kv.second);
        }
    }
    return avail;
}

bool AmbulanceRegistry::ReserveUnit(const std::string& unitId, const std::string& incidentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleet.find(unitId);
    if (it == m_fleet.end()) return false;

    if (it->second.state != common::AmbulanceState::AVAILABLE) {
        return false;
    }

    it->second.state = common::AmbulanceState::DISPATCHED;
    it->second.assignedIncidentId = incidentId;
    return true;
}

bool AmbulanceRegistry::ReleaseUnit(const std::string& unitId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleet.find(unitId);
    if (it == m_fleet.end()) return false;

    it->second.state = common::AmbulanceState::AVAILABLE;
    it->second.assignedIncidentId.clear();
    return true;
}

bool AmbulanceRegistry::UpdateTelemetry(const std::string& unitId, const common::GeoCoordinate& loc, float speed, float bearing) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleet.find(unitId);
    if (it == m_fleet.end()) return false;

    it->second.location = loc;
    it->second.speedKmH = speed;
    it->second.bearingDeg = bearing;
    it->second.lastHeartbeatMs = common::GetUtcMilliseconds();
    return true;
}

bool AmbulanceRegistry::UpdateState(const std::string& unitId, common::AmbulanceState newState) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fleet.find(unitId);
    if (it == m_fleet.end()) return false;

    it->second.state = newState;
    return true;
}

size_t AmbulanceRegistry::GetTotalFleetCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_fleet.size();
}

void AmbulanceRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fleet.clear();
}

} // namespace coordinator
} // namespace core
} // namespace aegis
