/**
 * @file VirtualIntersection.h
 * @brief SMART AEGIS (AERS) Phase 19 - Roadside Virtual Intersection & Signal Phase Machine.
 */

#ifndef AEGIS_SIMULATOR_VIRTUAL_INTERSECTION_H
#define AEGIS_SIMULATOR_VIRTUAL_INTERSECTION_H

#include <string>
#include <cstdint>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Autonomous signal phase sequence state.
 */
enum class SignalPhaseState : uint8_t {
    NORTH_SOUTH_GREEN = 0,
    CLEARANCE_YELLOW,
    ALL_RED_HOLD,
    EAST_WEST_GREEN,
    PREEMPTION_HOLD
};

std::string SignalPhaseStateToString(SignalPhaseState phase);

/**
 * @brief Digital twin model of a roadside intersection RSU with 4-phase traffic controller.
 */
class VirtualIntersection {
public:
    VirtualIntersection(std::string id, std::string name, double lat, double lon);
    ~VirtualIntersection() = default;

    void Update(double dtSec);

    // Preemption Controls
    void PreemptForCorridor(const std::string& corridorId, double durationSec = 45.0);
    void ReleasePreemption();

    // IR Sensor Simulation
    void TriggerApproachIR();
    void TriggerDepartureIR();

    // Queue & Traffic
    void AddVehiclesToQueue(uint32_t count);
    void RemoveVehiclesFromQueue(uint32_t count);
    void SetQueueCount(uint32_t count);

    // Getters & Telemetry
    std::string GetId() const { return m_id; }
    std::string GetName() const { return m_name; }
    double GetLatitude() const { return m_lat; }
    double GetLongitude() const { return m_lon; }

    std::string GetSignalState() const; // "RED", "YELLOW", "GREEN", "FLASHING_YELLOW"
    SignalPhaseState GetCurrentPhase() const;
    int GetCountdownSec() const;

    bool IsPreempted() const;
    std::string GetActiveCorridorId() const;

    uint32_t GetQueueCount() const;
    bool IsApproachIrTriggered() const;
    bool IsDepartureIrTriggered() const;

    std::string GetCurrentApproachingAmbulance() const;
    void SetApproachingAmbulance(const std::string& ambId, double etaSec);
    double GetAmbulanceEtaSec() const;

private:
    mutable std::mutex m_mutex;
    std::string m_id;
    std::string m_name;
    double m_lat{0.0};
    double m_lon{0.0};

    SignalPhaseState m_phase{SignalPhaseState::NORTH_SOUTH_GREEN};
    std::string m_signalColor{"GREEN"};
    double m_phaseTimer{22.0};
    int m_countdownSec{22};

    bool m_isPreempted{false};
    std::string m_activeCorridorId;
    double m_preemptionTimer{0.0};

    uint32_t m_queueCount{4};
    bool m_approachIrActive{false};
    bool m_departureIrActive{false};
    double m_irResetTimer{0.0};

    std::string m_currentAmbulanceId;
    double m_ambulanceEtaSec{0.0};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_VIRTUAL_INTERSECTION_H

