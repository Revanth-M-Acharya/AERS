/**
 * @file ambulance_registry.hpp
 * @brief Thread-safe fleet state storage, capabilities, and atomic reservation.
 */

#ifndef AEGIS_CORE_AMBULANCE_REGISTRY_HPP
#define AEGIS_CORE_AMBULANCE_REGISTRY_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "aegis/common/types.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace core {
namespace coordinator {

struct AmbulanceRecord {
    std::string unitId;
    std::string vehicleType{"ALS"}; // "ALS" or "BLS"
    common::AmbulanceState state{common::AmbulanceState::AVAILABLE};
    common::GeoCoordinate location{12.9716, 77.5946, 0.0};
    float speedKmH{0.0f};
    float bearingDeg{0.0f};
    std::string assignedIncidentId;
    std::vector<std::string> capabilities;
    uint64_t lastHeartbeatMs{0};
};

class AmbulanceRegistry {
public:
    AmbulanceRegistry() = default;
    ~AmbulanceRegistry() = default;

    bool RegisterAmbulance(const AmbulanceRecord& record);
    common::Result<AmbulanceRecord> GetAmbulance(const std::string& unitId) const;
    std::vector<AmbulanceRecord> GetAllAmbulances() const;
    std::vector<AmbulanceRecord> GetAvailableAmbulances() const;

    // Atomic reservation lock
    bool ReserveUnit(const std::string& unitId, const std::string& incidentId);
    bool ReleaseUnit(const std::string& unitId);

    bool UpdateTelemetry(const std::string& unitId, const common::GeoCoordinate& loc, float speed, float bearing);
    bool UpdateState(const std::string& unitId, common::AmbulanceState newState);

    size_t GetTotalFleetCount() const noexcept;
    void Clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, AmbulanceRecord> m_fleet;
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_AMBULANCE_REGISTRY_HPP
