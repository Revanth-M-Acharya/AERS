/**
 * @file AmbulanceRegistry.cpp
 * @brief Implementation of thread-safe AmbulanceRegistry.
 */

#include "ambulance/AmbulanceRegistry.h"
#include <cmath>

namespace aegis {
namespace ambulance {

namespace {

int PriorityToRank(medical::PriorityLevel priority) {
    switch (priority) {
        case medical::PriorityLevel::CRITICAL: return 4;
        case medical::PriorityLevel::HIGH:     return 3;
        case medical::PriorityLevel::MEDIUM:   return 2;
        case medical::PriorityLevel::LOW:      return 1;
        default:                               return 0;
    }
}

} // anonymous namespace

bool AmbulanceRegistry::RegisterAmbulance(const AmbulanceProfile& profile) {
    if (!profile.IsValid()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulances[profile.GetAmbulanceId()] = profile;
    return true;
}

bool AmbulanceRegistry::RemoveAmbulance(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    m_ambulances.erase(it);
    return true;
}

bool AmbulanceRegistry::UpdatePosition(const std::string& ambulanceId, const std::string& currentIntersection) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetCurrentIntersection(currentIntersection);
    return true;
}

bool AmbulanceRegistry::UpdatePosition(const std::string& ambulanceId, double lat, double lon, const std::string& currentIntersection) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetCoordinates(lat, lon);
    if (!currentIntersection.empty()) {
        it->second.SetCurrentIntersection(currentIntersection);
    }
    return true;
}

bool AmbulanceRegistry::UpdateETA(const std::string& ambulanceId, uint32_t etaMinutes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetETA(etaMinutes);
    return true;
}

bool AmbulanceRegistry::UpdatePatient(const std::string& ambulanceId, const medical::PatientProfile& patient) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetPatient(patient);
    return true;
}

bool AmbulanceRegistry::UpdatePriority(const std::string& ambulanceId, medical::PriorityLevel priority) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetPriority(priority);
    return true;
}

bool AmbulanceRegistry::UpdateStatus(const std::string& ambulanceId, AmbulanceStatus status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetStatus(status);
    return true;
}

bool AmbulanceRegistry::UpdateOperationalState(const std::string& ambulanceId, AmbulanceStatus state) {
    return UpdateStatus(ambulanceId, state);
}

bool AmbulanceRegistry::UpdateDestination(const std::string& ambulanceId, const std::string& destinationHospital) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetDestinationHospital(destinationHospital);
    return true;
}

bool AmbulanceRegistry::UpdateSpeed(const std::string& ambulanceId, double speedKmh) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetCurrentSpeed(speedKmh);
    return true;
}

bool AmbulanceRegistry::UpdateMission(const std::string& ambulanceId, const std::string& missionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetCurrentMission(missionId);
    return true;
}

bool AmbulanceRegistry::UpdatePriorityScore(const std::string& ambulanceId, double score) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetPriorityScore(score);
    return true;
}

bool AmbulanceRegistry::UpdateCorridor(const std::string& ambulanceId, const std::string& corridorId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it == m_ambulances.end()) {
        return false;
    }
    it->second.SetCorridorId(corridorId);
    return true;
}

std::optional<AmbulanceProfile> AmbulanceRegistry::SearchAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_ambulances.find(ambulanceId);
    if (it != m_ambulances.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<AmbulanceProfile> AmbulanceRegistry::GetHighestPriorityAmbulance() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ambulances.empty()) {
        return std::nullopt;
    }

    const AmbulanceProfile* best = nullptr;
    int highestRank = -1;
    double highestScore = -1.0;
    uint32_t lowestEta = UINT32_MAX;

    for (const auto& [id, amb] : m_ambulances) {
        int rank = PriorityToRank(amb.GetPriority());
        double score = amb.GetPriorityScore();
        if (score <= 0.0 && amb.HasPatient()) {
            score = medical::PriorityEngine::CalculateEmergencyScore(*amb.GetPatient());
        }
        uint32_t eta = amb.GetETA();

        if (best == nullptr) {
            best = &amb;
            highestRank = rank;
            highestScore = score;
            lowestEta = eta;
        } else if (rank > highestRank) {
            best = &amb;
            highestRank = rank;
            highestScore = score;
            lowestEta = eta;
        } else if (rank == highestRank) {
            if (score > highestScore + 0.001) {
                best = &amb;
                highestScore = score;
                lowestEta = eta;
            } else if (std::abs(score - highestScore) <= 0.001 && eta < lowestEta) {
                best = &amb;
                lowestEta = eta;
            }
        }
    }

    if (best != nullptr) {
        return *best;
    }
    return std::nullopt;
}

bool AmbulanceRegistry::HasAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ambulances.find(ambulanceId) != m_ambulances.end();
}

size_t AmbulanceRegistry::GetAmbulanceCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ambulances.size();
}

std::vector<AmbulanceProfile> AmbulanceRegistry::GetAllAmbulances() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AmbulanceProfile> result;
    result.reserve(m_ambulances.size());
    for (const auto& [id, profile] : m_ambulances) {
        result.push_back(profile);
    }
    return result;
}

std::vector<AmbulanceProfile> AmbulanceRegistry::GetAmbulancesByStatus(AmbulanceStatus status) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AmbulanceProfile> result;
    for (const auto& [id, profile] : m_ambulances) {
        if (profile.GetStatus() == status) {
            result.push_back(profile);
        }
    }
    return result;
}

std::vector<AmbulanceProfile> AmbulanceRegistry::GetAmbulancesByPriority(medical::PriorityLevel priority) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AmbulanceProfile> result;
    for (const auto& [id, profile] : m_ambulances) {
        if (profile.GetPriority() == priority) {
            result.push_back(profile);
        }
    }
    return result;
}

void AmbulanceRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ambulances.clear();
}

} // namespace ambulance
} // namespace aegis
