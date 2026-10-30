/**
 * @file IntersectionHardware.h
 * @brief SMART AEGIS (AERS) Phase 20 - Roadside Intersection Physical Hardware Enclosure.
 */

#ifndef AEGIS_HARDWARE_INTERSECTION_HARDWARE_H
#define AEGIS_HARDWARE_INTERSECTION_HARDWARE_H

#include "HardwareConfiguration.h"
#include "RelayController.h"
#include "TrafficLightController.h"
#include "HardwareEventLogger.h"
#include "bsp/RaspberryPi4BSP.h"
#include "aegis/hardware/OpticalSensorFilter.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace hardware {

/**
 * @brief Telemetry status of an optical IR sensor channel.
 */
struct IrSensorStatus {
    uint32_t gpioPin{12};
    bool beamBroken{false};
    uint64_t detectionCount{0};
    double continuousObstructionSec{0.0};
    bool isStuckFault{false};
    bool isHealthy{true};
};

/**
 * @brief Physical hardware device abstraction for one Raspberry Pi 4 traffic intersection.
 */
class IntersectionHardware {
public:
    explicit IntersectionHardware(std::string nodeId = "INT_A",
                                 std::shared_ptr<HardwareEventLogger> logger = nullptr);
    ~IntersectionHardware();

    bool Initialize(const DualSignalPins& pins,
                    const std::vector<RelayChannelDef>& relays,
                    const IrSensorParameters& irConfig,
                    const TimingParameters& timings);

    void Update(double dtSec);
    void Shutdown();

    // Intersection Identity
    std::string GetNodeId() const { return m_nodeId; }

    // Preemption Controls
    void TriggerPreemption(const std::string& corridorId, const std::string& roadDirection = "ROAD_A");
    void ReleasePreemption();
    bool IsPreempted() const;
    std::string GetActiveCorridorId() const;

    // Fail-Safe Controls
    void TriggerFailSafe(const std::string& reason);
    void RecoverFromFailSafe();
    bool IsInFailSafe() const;
    std::string GetFailSafeReason() const;

    // Manual Dispatcher Overrides
    void ForceGreen(const std::string& roadDirection = "ROAD_A");
    void ForceRed();
    void RestoreNormalCycle();

    // IR Sensors
    bool ReadApproachIrSensor(bool& detected);
    bool ReadDepartureIrSensor(bool& detected);
    void InjectApproachDetection();
    void InjectDepartureDetection();
    IrSensorStatus GetApproachSensorStatus() const;
    IrSensorStatus GetDepartureSensorStatus() const;

    // Phase 20.6 Noise Filtered IR Sensor Processing
    bool ProcessIrSample(bool rawApproachLevel, bool rawDepartureLevel, uint32_t sampleDtMs);
    OpticalFilterStats GetApproachFilterStats() const;
    OpticalFilterStats GetDepartureFilterStats() const;

    // Subsystem Accessors
    std::shared_ptr<TrafficLightController> GetTrafficLightController() const { return m_trafficLights; }
    std::shared_ptr<RelayController> GetRelayController() const { return m_relayController; }
    std::shared_ptr<bsp::RaspberryPi4BSP> GetBsp() const { return m_bsp; }

private:
    void AdvanceNormalCycle(double dtSec);
    void UpdateIrSensors(double dtSec);

    mutable std::recursive_mutex m_mutex;
    std::string m_nodeId;
    std::shared_ptr<HardwareEventLogger> m_logger;

    std::shared_ptr<bsp::RaspberryPi4BSP> m_bsp;
    std::shared_ptr<RelayController> m_relayController;
    std::shared_ptr<TrafficLightController> m_trafficLights;

    DualSignalPins m_pins;
    IrSensorParameters m_irConfig;
    TimingParameters m_timings;

    // Normal Cycle State
    double m_phaseTimer{0.0};
    double m_flashTimer{0.0};
    bool m_flashState{false};

    // Preemption
    bool m_isPreempted{false};
    std::string m_activeCorridorId;
    std::string m_preemptDirection{"ROAD_A"};
    double m_preemptionElapsedSec{0.0};

    // Fail-safe
    bool m_inFailSafe{false};
    std::string m_failsafeReason;

    // IR Sensor State
    IrSensorStatus m_approachIr;
    IrSensorStatus m_departureIr;
    double m_approachPulseTimer{0.0};
    double m_departurePulseTimer{0.0};

    OpticalSensorFilter m_approachFilter;
    OpticalSensorFilter m_departureFilter;

    bool m_initialized{false};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_INTERSECTION_HARDWARE_H
