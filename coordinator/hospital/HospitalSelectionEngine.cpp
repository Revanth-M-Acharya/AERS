/**
 * @file HospitalSelectionEngine.cpp
 * @brief Implementation of hospital selection, capability scoring, and automatic divert failover.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/hospital/HospitalSelectionEngine.hpp"
#include "common/utils/TimeUtils.hpp"
#include <cmath>
#include <algorithm>

namespace aegis::coordinator::hospital {

HospitalSelectionEngine::HospitalSelectionEngine(std::shared_ptr<HospitalRegistry> registry,
                                                 std::shared_ptr<orchestration::IEventBus> eventBus,
                                                 std::shared_ptr<orchestration::EmergencySessionManager> sessionManager)
    : registry_(std::move(registry)),
      eventBus_(std::move(eventBus)),
      sessionManager_(std::move(sessionManager)) {}

double HospitalSelectionEngine::CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2) {
    // Equirectangular / Haversine approximation in kilometers
    double dLat = (lat2 - lat1) * (M_PI / 180.0);
    double dLon = (lon2 - lon1) * (M_PI / 180.0);
    double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
               std::cos(lat1 * (M_PI / 180.0)) * std::cos(lat2 * (M_PI / 180.0)) *
               std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return 6371.0 * c;
}

std::optional<HospitalData> HospitalSelectionEngine::SelectBestHospital(control_center::EmergencyType emergencyType,
                                                                       control_center::PriorityLevel priority,
                                                                       double originLat,
                                                                       double originLon,
                                                                       const std::string& excludedHospitalId) {
    if (!registry_) return std::nullopt;

    std::string requiredDept = priorityEngine_.DetermineRequiredDepartment(emergencyType);
    auto candidates = registry_->GetAllHospitals();
    if (candidates.empty()) return std::nullopt;

    // Filter valid hospitals
    std::vector<HospitalData> eligible;
    for (const auto& hosp : candidates) {
        if (!excludedHospitalId.empty() && hosp.hospitalId == excludedHospitalId) continue;
        if (hosp.status == HospitalStatus::OFFLINE ||
            hosp.status == HospitalStatus::FULL ||
            hosp.status == HospitalStatus::UNDER_MAINTENANCE) continue;

        if (hosp.availableBeds == 0) continue;

        // EMERGENCY_ONLY hospitals only take Priority 1 or 2
        if (hosp.status == HospitalStatus::EMERGENCY_ONLY &&
            priority != control_center::PriorityLevel::Priority1 &&
            priority != control_center::PriorityLevel::Priority2) {
            continue;
        }

        // Must have required department
        if (hosp.HasDepartment(requiredDept) || hosp.HasDepartment("GENERAL")) {
            eligible.push_back(hosp);
        }
    }

    if (eligible.empty()) {
        // Fallback: any available online hospital
        for (const auto& hosp : candidates) {
            if (!excludedHospitalId.empty() && hosp.hospitalId == excludedHospitalId) continue;
            if (hosp.status == HospitalStatus::ONLINE && hosp.availableBeds > 0) {
                eligible.push_back(hosp);
            }
        }
    }

    if (eligible.empty()) return std::nullopt;

    // Score and rank candidates
    HospitalData bestHospital = eligible[0];
    double highestScore = -1.0;

    for (const auto& hosp : eligible) {
        double distKm = (originLat != 0.0 || originLon != 0.0) ?
            CalculateDistanceKm(originLat, originLon, hosp.latitude, hosp.longitude) : 5.0;
        if (distKm < 0.1) distKm = 0.1;

        double distScore = 100.0 / distKm;
        double bedScore = static_cast<double>(hosp.availableBeds);
        double icuScore = static_cast<double>(hosp.icuBeds) * 2.0;
        double traumaScore = static_cast<double>(4 - std::min(hosp.traumaLevel, 3u)) * 10.0;
        double deptScore = hosp.HasDepartment(requiredDept) ? 50.0 : 0.0;

        double totalScore = (distScore * 0.35) + (deptScore * 0.25) +
                            (traumaScore * 0.15) + (bedScore * 0.15) + (icuScore * 0.10);

        if (totalScore > highestScore) {
            highestScore = totalScore;
            bestHospital = hosp;
        } else if (std::abs(totalScore - highestScore) < 0.001) {
            // Tie breaking: more available beds, then lower prep time, then ID
            if (hosp.availableBeds > bestHospital.availableBeds) {
                bestHospital = hosp;
            } else if (hosp.availableBeds == bestHospital.availableBeds &&
                       hosp.avgPrepTimeSec < bestHospital.avgPrepTimeSec) {
                bestHospital = hosp;
            } else if (hosp.availableBeds == bestHospital.availableBeds &&
                       hosp.avgPrepTimeSec == bestHospital.avgPrepTimeSec &&
                       hosp.hospitalId < bestHospital.hospitalId) {
                bestHospital = hosp;
            }
        }
    }

    return bestHospital;
}

std::optional<HospitalData> HospitalSelectionEngine::HandleHospitalDivert(const std::string& emergencyId,
                                                                         const std::string& failedHospitalId,
                                                                         double currentLat,
                                                                         double currentLon,
                                                                         uint64_t currentMonoMs) {
    if (currentMonoMs == 0) currentMonoMs = common::TimeUtils::GetMonotonicTimeMs();

    // Select alternative hospital
    auto optNewHospital = SelectBestHospital(control_center::EmergencyType::Trauma,
                                             control_center::PriorityLevel::Priority1,
                                             currentLat,
                                             currentLon,
                                             failedHospitalId);
    if (!optNewHospital.has_value()) {
        return std::nullopt;
    }

    const HospitalData& newHosp = optNewHospital.value();

    // Update active emergency session
    if (sessionManager_) {
        auto optSess = sessionManager_->GetSession(emergencyId);
        if (optSess.has_value()) {
            auto sess = optSess.value();
            sess.hospitalDestination = newHosp.hospitalId;
            sessionManager_->CreateSession(sess.emergencyId, sess.ambulanceId, newHosp.hospitalId, sess.priorityLevel, currentMonoMs);
        }
    }

    // Publish event
    if (eventBus_) {
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::HospitalChanged,
            emergencyId,
            newHosp.hospitalId,
            "Diverted from " + failedHospitalId + " to " + newHosp.hospitalId,
            currentMonoMs
        ));
        eventBus_->Publish(orchestration::Event(
            orchestration::EventType::RouteChanged,
            emergencyId,
            newHosp.hospitalId,
            "Rerouting to alternative hospital " + newHosp.hospitalId,
            currentMonoMs
        ));
    }

    return newHosp;
}

} // namespace aegis::coordinator::hospital

