/**
 * @file AmbulanceProfile.cpp
 * @brief Implementation of AmbulanceProfile methods, validation, and JSON serialization.
 */

#include "ambulance/AmbulanceProfile.h"
#include <iomanip>
#include <sstream>

namespace aegis {
namespace ambulance {

AmbulanceProfile::AmbulanceProfile()
    : m_ambulanceId("")
    , m_status(AmbulanceStatus::AVAILABLE)
    , m_currentPatient(std::nullopt)
    , m_currentPriority(medical::PriorityLevel::LOW)
    , m_currentRoute()
    , m_currentIntersection("")
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_destinationHospital("")
    , m_etaMinutes(0)
    , m_etaSeconds(0)
    , m_distanceRemainingMeters(0.0)
    , m_manualOverrideLevel(0)
    , m_currentMission("")
    , m_currentSpeedKmh(0.0)
    , m_priorityScore(0.0)
    , m_corridorId("") {}

AmbulanceProfile::AmbulanceProfile(std::string ambulanceId)
    : m_ambulanceId(std::move(ambulanceId))
    , m_status(AmbulanceStatus::AVAILABLE)
    , m_currentPatient(std::nullopt)
    , m_currentPriority(medical::PriorityLevel::LOW)
    , m_currentRoute()
    , m_currentIntersection("")
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_destinationHospital("")
    , m_etaMinutes(0)
    , m_etaSeconds(0)
    , m_distanceRemainingMeters(0.0)
    , m_manualOverrideLevel(0)
    , m_currentMission("")
    , m_currentSpeedKmh(0.0)
    , m_priorityScore(0.0)
    , m_corridorId("") {}

AmbulanceProfile::AmbulanceProfile(std::string ambulanceId,
                                   AmbulanceStatus status,
                                   std::string destinationHospital,
                                   uint32_t etaMinutes)
    : m_ambulanceId(std::move(ambulanceId))
    , m_status(status)
    , m_currentPatient(std::nullopt)
    , m_currentPriority(medical::PriorityLevel::LOW)
    , m_currentRoute()
    , m_currentIntersection("")
    , m_latitude(0.0)
    , m_longitude(0.0)
    , m_destinationHospital(std::move(destinationHospital))
    , m_etaMinutes(etaMinutes)
    , m_etaSeconds(etaMinutes * 60)
    , m_distanceRemainingMeters(0.0)
    , m_manualOverrideLevel(0)
    , m_currentMission("")
    , m_currentSpeedKmh(0.0)
    , m_priorityScore(0.0)
    , m_corridorId("") {}

void AmbulanceProfile::SetPatient(const medical::PatientProfile& patient) {
    m_currentPatient = patient;
    m_currentPriority = medical::PriorityEngine::CalculatePriority(patient);
    m_priorityScore = medical::PriorityEngine::CalculateEmergencyScore(patient);
}

void AmbulanceProfile::ClearPatient() noexcept {
    m_currentPatient = std::nullopt;
    m_currentPriority = medical::PriorityLevel::LOW;
    m_priorityScore = 0.0;
}

void AmbulanceProfile::AddRouteIntersection(const std::string& intersectionId) {
    if (!intersectionId.empty()) {
        m_currentRoute.push_back(intersectionId);
    }
}

bool AmbulanceProfile::IsValid() const noexcept {
    return !m_ambulanceId.empty();
}

std::string AmbulanceProfile::ToString() const {
    std::ostringstream oss;
    oss << "AmbulanceProfile [" << m_ambulanceId << "]\n"
        << "  Status         : " << GetStatusString() << "\n"
        << "  Priority       : " << GetPriorityString() << "\n"
        << "  Priority Score : " << m_priorityScore << "\n"
        << "  Mission        : " << (m_currentMission.empty() ? "None" : m_currentMission) << "\n"
        << "  Speed          : " << m_currentSpeedKmh << " km/h\n"
        << "  Corridor ID    : " << (m_corridorId.empty() ? "None" : m_corridorId) << "\n"
        << "  Intersection   : " << (m_currentIntersection.empty() ? "None" : m_currentIntersection) << "\n"
        << "  Hospital       : " << (m_destinationHospital.empty() ? "None" : m_destinationHospital) << "\n"
        << "  ETA            : " << m_etaMinutes << " mins (" << m_etaSeconds << "s)\n"
        << "  Patient        : " << (HasPatient() ? (m_currentPatient->GetPatientName() + " (" + m_currentPatient->GetPatientId() + ")") : "None") << "\n"
        << "  Route Stops    : " << m_currentRoute.size();
    return oss.str();
}

std::string AmbulanceProfile::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"ambulance_id\":\"" << m_ambulanceId << "\","
        << "\"status\":\"" << GetStatusString() << "\","
        << "\"priority\":\"" << GetPriorityString() << "\","
        << "\"priority_score\":" << std::fixed << std::setprecision(2) << m_priorityScore << ","
        << "\"current_mission\":\"" << m_currentMission << "\","
        << "\"current_speed_kmh\":" << std::fixed << std::setprecision(1) << m_currentSpeedKmh << ","
        << "\"corridor_id\":\"" << m_corridorId << "\","
        << "\"current_intersection\":\"" << m_currentIntersection << "\","
        << "\"latitude\":" << std::fixed << std::setprecision(6) << m_latitude << ","
        << "\"longitude\":" << std::fixed << std::setprecision(6) << m_longitude << ","
        << "\"destination_hospital\":\"" << m_destinationHospital << "\","
        << "\"eta_minutes\":" << m_etaMinutes << ","
        << "\"route\":[";

    for (size_t i = 0; i < m_currentRoute.size(); ++i) {
        oss << "\"" << m_currentRoute[i] << "\"";
        if (i + 1 < m_currentRoute.size()) {
            oss << ",";
        }
    }
    oss << "],";

    if (HasPatient()) {
        oss << "\"patient\":" << m_currentPatient->ToJson();
    } else {
        oss << "\"patient\":null";
    }

    oss << "}";
    return oss.str();
}

} // namespace ambulance
} // namespace aegis
