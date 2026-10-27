/**
 * @file AmbulanceProfile.h
 * @brief Dynamic Ambulance Profile and Fleet Tracking Model for SMART AEGIS.
 */

#ifndef AEGIS_AMBULANCE_AMBULANCE_PROFILE_H
#define AEGIS_AMBULANCE_AMBULANCE_PROFILE_H

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <sstream>

#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"

namespace aegis {
namespace ambulance {

/**
 * @brief Operational status lifecycle of an active emergency ambulance unit.
 */
enum class AmbulanceStatus {
    AVAILABLE,
    ON_CALL,
    TRANSPORTING,
    AT_HOSPITAL,
    RETURNING
};

inline std::string AmbulanceStatusToString(AmbulanceStatus status) {
    switch (status) {
        case AmbulanceStatus::AVAILABLE:    return "AVAILABLE";
        case AmbulanceStatus::ON_CALL:      return "ON_CALL";
        case AmbulanceStatus::TRANSPORTING: return "TRANSPORTING";
        case AmbulanceStatus::AT_HOSPITAL:  return "AT_HOSPITAL";
        case AmbulanceStatus::RETURNING:    return "RETURNING";
        default:                           return "UNKNOWN";
    }
}

inline AmbulanceStatus StringToAmbulanceStatus(const std::string& str) {
    if (str == "AVAILABLE" || str == "available") return AmbulanceStatus::AVAILABLE;
    if (str == "ON_CALL" || str == "on_call") return AmbulanceStatus::ON_CALL;
    if (str == "TRANSPORTING" || str == "transporting") return AmbulanceStatus::TRANSPORTING;
    if (str == "AT_HOSPITAL" || str == "at_hospital") return AmbulanceStatus::AT_HOSPITAL;
    if (str == "RETURNING" || str == "returning") return AmbulanceStatus::RETURNING;
    return AmbulanceStatus::AVAILABLE;
}

/**
 * @brief Comprehensive profile containing vehicle identity, kinematics, telemetry, route, patient, and triage priority.
 */
class AmbulanceProfile {
public:
    AmbulanceProfile();

    explicit AmbulanceProfile(std::string ambulanceId);

    AmbulanceProfile(std::string ambulanceId,
                     AmbulanceStatus status,
                     std::string destinationHospital = "",
                     uint32_t etaMinutes = 0);

    ~AmbulanceProfile() = default;

    // Ambulance ID
    const std::string& GetAmbulanceId() const noexcept { return m_ambulanceId; }
    void SetAmbulanceId(std::string id) { m_ambulanceId = std::move(id); }

    // Status
    AmbulanceStatus GetStatus() const noexcept { return m_status; }
    void SetStatus(AmbulanceStatus status) noexcept { m_status = status; }
    std::string GetStatusString() const { return AmbulanceStatusToString(m_status); }

    // Patient
    bool HasPatient() const noexcept { return m_currentPatient.has_value(); }
    const std::optional<medical::PatientProfile>& GetPatient() const noexcept { return m_currentPatient; }
    void SetPatient(const medical::PatientProfile& patient);
    void ClearPatient() noexcept;

    // Priority
    medical::PriorityLevel GetPriority() const noexcept { return m_currentPriority; }
    void SetPriority(medical::PriorityLevel priority) noexcept { m_currentPriority = priority; }
    std::string GetPriorityString() const { return medical::PriorityLevelToString(m_currentPriority); }

    // Route & Topological Intersection
    const std::vector<std::string>& GetCurrentRoute() const noexcept { return m_currentRoute; }
    void SetCurrentRoute(std::vector<std::string> route) { m_currentRoute = std::move(route); }
    void AddRouteIntersection(const std::string& intersectionId);
    void ClearRoute() noexcept { m_currentRoute.clear(); }

    const std::string& GetCurrentIntersection() const noexcept { return m_currentIntersection; }
    void SetCurrentIntersection(std::string intersection) { m_currentIntersection = std::move(intersection); }

    // Spatial Coordinates
    double GetLatitude() const noexcept { return m_latitude; }
    double GetLongitude() const noexcept { return m_longitude; }
    void SetCoordinates(double lat, double lon) noexcept {
        m_latitude = lat;
        m_longitude = lon;
    }

    // Destination Hospital
    const std::string& GetDestinationHospital() const noexcept { return m_destinationHospital; }
    void SetDestinationHospital(std::string hospital) { m_destinationHospital = std::move(hospital); }

    // Estimated Time of Arrival (minutes & seconds)
    uint32_t GetETA() const noexcept { return m_etaMinutes; }
    void SetETA(uint32_t etaMinutes) noexcept {
        m_etaMinutes = etaMinutes;
        m_etaSeconds = etaMinutes * 60;
    }

    uint32_t GetETASeconds() const noexcept { return m_etaSeconds; }
    void SetETASeconds(uint32_t etaSec) noexcept {
        m_etaSeconds = etaSec;
        m_etaMinutes = (etaSec + 59) / 60;
    }

    // Kinematic Distance to Target
    double GetDistanceRemainingMeters() const noexcept { return m_distanceRemainingMeters; }
    void SetDistanceRemainingMeters(double distMeters) noexcept { m_distanceRemainingMeters = distMeters; }

    // Manual Dispatcher Override
    uint8_t GetManualOverrideLevel() const noexcept { return m_manualOverrideLevel; }
    void SetManualOverrideLevel(uint8_t level) noexcept { m_manualOverrideLevel = level; }

    // Mission Lifecycle
    const std::string& GetCurrentMission() const noexcept { return m_currentMission; }
    void SetCurrentMission(std::string mission) { m_currentMission = std::move(mission); }

    // Vehicle Speed (km/h)
    double GetCurrentSpeed() const noexcept { return m_currentSpeedKmh; }
    void SetCurrentSpeed(double speedKmh) noexcept { m_currentSpeedKmh = speedKmh; }

    // Continuous Priority / Emergency Score
    double GetPriorityScore() const noexcept { return m_priorityScore; }
    void SetPriorityScore(double score) noexcept { m_priorityScore = score; }

    // Green Wave Corridor ID
    const std::string& GetCorridorId() const noexcept { return m_corridorId; }
    void SetCorridorId(std::string corridorId) { m_corridorId = std::move(corridorId); }

    // Operational State
    AmbulanceStatus GetOperationalState() const noexcept { return m_status; }
    void SetOperationalState(AmbulanceStatus state) noexcept { m_status = state; }

    // Validation & Serialization
    bool IsValid() const noexcept;
    std::string ToString() const;
    std::string ToJson() const;

private:
    std::string m_ambulanceId{""};
    AmbulanceStatus m_status{AmbulanceStatus::AVAILABLE};
    std::optional<medical::PatientProfile> m_currentPatient{std::nullopt};
    medical::PriorityLevel m_currentPriority{medical::PriorityLevel::LOW};
    std::vector<std::string> m_currentRoute{};
    std::string m_currentIntersection{""};
    double m_latitude{0.0};
    double m_longitude{0.0};
    std::string m_destinationHospital{""};
    uint32_t m_etaMinutes{0};
    uint32_t m_etaSeconds{0};
    double m_distanceRemainingMeters{0.0};
    uint8_t m_manualOverrideLevel{0};
    std::string m_currentMission{""};
    double m_currentSpeedKmh{0.0};
    double m_priorityScore{0.0};
    std::string m_corridorId{""};
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_AMBULANCE_PROFILE_H
