/**
 * @file TrafficLightController.h
 * @brief SMART AEGIS (AERS) Phase 20 - Dual-Module Complementary Traffic Light Controller.
 */

#ifndef AEGIS_HARDWARE_TRAFFIC_LIGHT_CONTROLLER_H
#define AEGIS_HARDWARE_TRAFFIC_LIGHT_CONTROLLER_H

#include "HardwareConfiguration.h"
#include "RelayController.h"
#include "bsp/RaspberryPi4BSP.h"

#include <string>
#include <memory>
#include <mutex>
#include <cstdint>

namespace aegis {
namespace hardware {

enum class LightColor : uint8_t {
    RED = 0,
    YELLOW,
    GREEN,
    OFF
};

std::string LightColorToString(LightColor color);

enum class DualPhase : uint8_t {
    ROAD_A_GREEN_ROAD_B_RED = 0,
    ROAD_A_YELLOW_ROAD_B_RED,
    ALL_RED_CLEARANCE,
    ROAD_A_RED_ROAD_B_GREEN,
    ROAD_A_RED_ROAD_B_YELLOW,
    PREEMPTION_ROAD_A,
    PREEMPTION_ROAD_B,
    FAILSAFE_FLASHING_YELLOW
};

std::string DualPhaseToString(DualPhase phase);

/**
 * @brief Drives two physical Traffic Signal Modules with hardware mutual exclusion interlocks.
 */
class TrafficLightController {
public:
    explicit TrafficLightController(std::shared_ptr<bsp::RaspberryPi4BSP> bsp = nullptr,
                                    std::shared_ptr<RelayController> relayController = nullptr);
    ~TrafficLightController();

    bool Initialize(const DualSignalPins& pins);
    void Shutdown();

    // High-Level Phase Control
    bool SetPhase(DualPhase phase);
    DualPhase GetCurrentPhase() const;

    // Manual/Explicit Light Control (subject to mutual exclusion safety check)
    bool SetSignals(LightColor roadA, LightColor roadB, bool bypassInterlockForTest = false);

    // Current Lamp State
    LightColor GetRoadAColor() const;
    LightColor GetRoadBColor() const;

    // Safety Interlocks & Conflict Detection
    bool IsConflictingGreenDetected() const;
    bool HasHardwareFault() const;
    void ClearFault();

    // Pedestrian & Warning Outputs
    bool SetPedestrianSignal(bool walk);
    bool SetPreemptionStrobe(bool active);
    bool SetBuzzer(bool active);

private:
    void ApplyHardwareOutputs();
    bool EnforceMutualExclusion(LightColor roadA, LightColor roadB);

    mutable std::mutex m_mutex;
    std::shared_ptr<bsp::RaspberryPi4BSP> m_bsp;
    std::shared_ptr<RelayController> m_relayController;

    DualSignalPins m_pins;
    DualPhase m_currentPhase{DualPhase::ROAD_A_RED_ROAD_B_GREEN};

    LightColor m_roadAColor{LightColor::RED};
    LightColor m_roadBColor{LightColor::GREEN};

    bool m_pedWalk{false};
    bool m_preemptStrobe{false};
    bool m_buzzerActive{false};

    bool m_conflictDetected{false};
    bool m_hardwareFault{false};
    bool m_initialized{false};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_TRAFFIC_LIGHT_CONTROLLER_H

