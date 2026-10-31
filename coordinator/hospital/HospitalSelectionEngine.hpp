/**
 * @file HospitalSelectionEngine.hpp
 * @brief Algorithmic selection and automatic failover engine for emergency hospital destination.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "coordinator/hospital/HospitalRegistry.hpp"
#include "coordinator/control_center/EmergencyPriorityEngine.hpp"
#include "coordinator/orchestration/event_bus/IEventBus.hpp"
#include "coordinator/orchestration/session/EmergencySessionManager.hpp"
#include <memory>
#include <optional>
#include <string>

namespace aegis::coordinator::hospital {

struct HospitalScore {
    std::string hospitalId;
    double compositeScore{0.0};
    double distanceKm{0.0};
    uint32_t availableBeds{0};
    uint32_t traumaLevel{1};
};

class HospitalSelectionEngine {
public:
    HospitalSelectionEngine(std::shared_ptr<HospitalRegistry> registry,
                            std::shared_ptr<orchestration::IEventBus> eventBus = nullptr,
                            std::shared_ptr<orchestration::EmergencySessionManager> sessionManager = nullptr);
    ~HospitalSelectionEngine() = default;

    /**
     * @brief Automatically select the optimal hospital for an incoming emergency request.
     */
    std::optional<HospitalData> SelectBestHospital(control_center::EmergencyType emergencyType,
                                                  control_center::PriorityLevel priority,
                                                  double originLat,
                                                  double originLon,
                                                  const std::string& excludedHospitalId = "");

    /**
     * @brief Handle hospital failure or diversion (dynamic rerouting to backup facility).
     */
    std::optional<HospitalData> HandleHospitalDivert(const std::string& emergencyId,
                                                    const std::string& failedHospitalId,
                                                    double currentLat,
                                                    double currentLon,
                                                    uint64_t currentMonoMs = 0);

private:
    static double CalculateDistanceKm(double lat1, double lon1, double lat2, double lon2);

    std::shared_ptr<HospitalRegistry> registry_;
    std::shared_ptr<orchestration::IEventBus> eventBus_;
    std::shared_ptr<orchestration::EmergencySessionManager> sessionManager_;
    control_center::EmergencyPriorityEngine priorityEngine_;
};

} // namespace aegis::coordinator::hospital

