/**
 * @file AmbulanceRegistry.h
 * @brief Thread-safe Central Fleet Registry for Emergency Ambulances.
 */

#ifndef AEGIS_AMBULANCE_AMBULANCE_REGISTRY_H
#define AEGIS_AMBULANCE_AMBULANCE_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <optional>
#include <cstdint>

#include "ambulance/AmbulanceProfile.h"

namespace aegis {
namespace ambulance {

/**
 * @brief Thread-safe coordinator registry managing active fleet profiles, positions, and priorities.
 */
class AmbulanceRegistry {
public:
    AmbulanceRegistry() = default;
    ~AmbulanceRegistry() = default;

    // Core Fleet Operations
    bool RegisterAmbulance(const AmbulanceProfile& profile);
    bool RemoveAmbulance(const std::string& ambulanceId);
    
    // Telemetry & State Mutators
    bool UpdatePosition(const std::string& ambulanceId, const std::string& currentIntersection);
    bool UpdatePosition(const std::string& ambulanceId, double lat, double lon, const std::string& currentIntersection = "");
    bool UpdateETA(const std::string& ambulanceId, uint32_t etaMinutes);
    bool UpdatePatient(const std::string& ambulanceId, const medical::PatientProfile& patient);
    bool UpdatePriority(const std::string& ambulanceId, medical::PriorityLevel priority);
    bool UpdateStatus(const std::string& ambulanceId, AmbulanceStatus status);
    bool UpdateOperationalState(const std::string& ambulanceId, AmbulanceStatus state);
    bool UpdateDestination(const std::string& ambulanceId, const std::string& destinationHospital);
    bool UpdateSpeed(const std::string& ambulanceId, double speedKmh);
    bool UpdateMission(const std::string& ambulanceId, const std::string& missionId);
    bool UpdatePriorityScore(const std::string& ambulanceId, double score);
    bool UpdateCorridor(const std::string& ambulanceId, const std::string& corridorId);

    // Queries & Search
    std::optional<AmbulanceProfile> SearchAmbulance(const std::string& ambulanceId) const;
    std::optional<AmbulanceProfile> GetHighestPriorityAmbulance() const;
    bool HasAmbulance(const std::string& ambulanceId) const;
    size_t GetAmbulanceCount() const noexcept;

    // Bulk Filtering
    std::vector<AmbulanceProfile> GetAllAmbulances() const;
    std::vector<AmbulanceProfile> GetAmbulancesByStatus(AmbulanceStatus status) const;
    std::vector<AmbulanceProfile> GetAmbulancesByPriority(medical::PriorityLevel priority) const;

    // Reset
    void Clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, AmbulanceProfile> m_ambulances{};
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_AMBULANCE_REGISTRY_H
