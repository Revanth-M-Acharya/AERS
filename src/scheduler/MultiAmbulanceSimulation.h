/**
 * @file MultiAmbulanceSimulation.h
 * @brief Comprehensive multi-emergency simulation scenarios and conflict resolution orchestrator.
 */

#ifndef AEGIS_SCHEDULER_MULTI_AMBULANCE_SIMULATION_H
#define AEGIS_SCHEDULER_MULTI_AMBULANCE_SIMULATION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "ambulance/AmbulanceProfile.h"
#include "ambulance/AmbulanceRegistry.h"
#include "medical/PatientProfile.h"
#include "medical/PriorityEngine.h"
#include "scheduler/MissionManager.h"
#include "scheduler/ConflictDetectionEngine.h"
#include "scheduler/IntelligentPriorityScheduler.h"
#include "scheduler/ConflictResolutionManager.h"
#include "scheduler/ManualOverrideManager.h"
#include "scheduler/AmbulanceAwarenessService.h"
#include "coordinator/CoordinatorManager.h"

namespace aegis {
namespace scheduler {

/**
 * @brief Registered emergency scenario descriptor.
 */
struct SimulationAmbulance {
    std::string ambulanceId{""};
    std::string diseaseName{""};
    medical::PatientProfile patient{};
    Mission mission{};
    std::vector<std::string> route{};
    std::string targetIntersection{""};
    double speedKmh{60.0};
    uint32_t etaSeconds{10};
    uint32_t waitingTimeSeconds{0};
    uint8_t manualOverrideLevel{0};
    ambulance::AmbulanceProfile profile{};
};

/**
 * @brief Aggregated outcome and audit telemetry of a multi-ambulance simulation execution.
 */
struct SimulationResult {
    bool success{false};
    size_t totalAmbulances{0};
    size_t totalConflictsDetected{0};
    size_t totalConflictsResolved{0};
    size_t totalCommandsGenerated{0};
    size_t totalAwarenessNotifications{0};
    std::vector<ConflictEvent> detectedConflicts{};
    std::vector<IntelligentScheduleDecision> decisions{};
    std::vector<ResolutionCommand> generatedCommands{};
    std::vector<AwarenessNotification> awarenessNotifications{};
    std::string summaryLog{""};

    std::string ToString() const;
    std::string ToJson() const;
};

/**
 * @brief Orchestrates simultaneous emergency missions covering 8 clinical scenarios:
 * Heart Attack, Stroke, Pregnancy, Organ Transport, Major Accident,
 * Cardiac Arrest, Pediatric Emergency, and Internal Bleeding.
 */
class MultiAmbulanceSimulation {
public:
    MultiAmbulanceSimulation();
    ~MultiAmbulanceSimulation() = default;

    // Subsystem Accessors
    std::shared_ptr<ambulance::AmbulanceRegistry> GetRegistry() const noexcept { return m_registry; }
    std::shared_ptr<MissionManager> GetMissionManager() const noexcept { return m_missionManager; }
    std::shared_ptr<ConflictDetectionEngine> GetDetectionEngine() const noexcept { return m_detectionEngine; }
    std::shared_ptr<IntelligentPriorityScheduler> GetScheduler() const noexcept { return m_scheduler; }
    std::shared_ptr<ConflictResolutionManager> GetResolutionManager() const noexcept { return m_resolutionManager; }
    std::shared_ptr<ManualOverrideManager> GetOverrideManager() const noexcept { return m_overrideManager; }
    std::shared_ptr<AmbulanceAwarenessService> GetAwarenessService() const noexcept { return m_awarenessService; }
    std::shared_ptr<coordinator::CoordinatorManager> GetCoordinator() const noexcept { return m_coordinator; }

    // Scenario Generation
    void SetupEightClinicalEmergencies();
    const std::vector<SimulationAmbulance>& GetSimulatedAmbulances() const noexcept { return m_simAmbulances; }

    // Execution
    SimulationResult RunFullSimulation();
    SimulationResult RunFourAmbulanceIntersectionContention();
    SimulationResult RunManualOverrideInterventionScenario();
    SimulationResult RunWaitingTimeStarvationPreventionScenario();

    void Reset();

private:
    std::shared_ptr<ambulance::AmbulanceRegistry> m_registry;
    std::shared_ptr<MissionManager> m_missionManager;
    std::shared_ptr<ConflictDetectionEngine> m_detectionEngine;
    std::shared_ptr<IntelligentPriorityScheduler> m_scheduler;
    std::shared_ptr<ConflictResolutionManager> m_resolutionManager;
    std::shared_ptr<ManualOverrideManager> m_overrideManager;
    std::shared_ptr<AmbulanceAwarenessService> m_awarenessService;
    std::shared_ptr<coordinator::CoordinatorManager> m_coordinator;

    std::vector<SimulationAmbulance> m_simAmbulances{};
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_SCHEDULER_MULTI_AMBULANCE_SIMULATION_H
