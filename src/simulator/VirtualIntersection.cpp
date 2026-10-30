/**
 * @file VirtualIntersection.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Roadside Virtual Intersection & Signal Phase Machine Implementation.
 */

#include "VirtualIntersection.h"
#include <cmath>

namespace aegis {
namespace simulator {

std::string SignalPhaseStateToString(SignalPhaseState phase) {
    switch (phase) {
        case SignalPhaseState::NORTH_SOUTH_GREEN: return "NORTH_SOUTH_GREEN";
        case SignalPhaseState::CLEARANCE_YELLOW: return "CLEARANCE_YELLOW";
        case SignalPhaseState::ALL_RED_HOLD: return "ALL_RED_HOLD";
        case SignalPhaseState::EAST_WEST_GREEN: return "EAST_WEST_GREEN";
        case SignalPhaseState::PREEMPTION_HOLD: return "PREEMPTION_HOLD";
    }
    return "UNKNOWN";
}

VirtualIntersection::VirtualIntersection(std::string id, std::string name, double lat, double lon)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_lat(lat)
    , m_lon(lon)
{
}

void VirtualIntersection::PreemptForCorridor(const std::string& corridorId, double durationSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPreempted = true;
    m_activeCorridorId = corridorId;
    m_preemptionTimer = durationSec;
    m_phase = SignalPhaseState::PREEMPTION_HOLD;
    m_signalColor = "GREEN";
    m_countdownSec = static_cast<int>(std::ceil(durationSec));
    m_approachIrActive = true;
    m_irResetTimer = 5.0;
}

void VirtualIntersection::ReleasePreemption() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isPreempted = false;
    m_activeCorridorId.clear();
    m_phase = SignalPhaseState::CLEARANCE_YELLOW;
    m_signalColor = "YELLOW";
    m_phaseTimer = 4.0;
    m_countdownSec = 4;
}

void VirtualIntersection::TriggerApproachIR() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_approachIrActive = true;
    m_irResetTimer = 2.0;
}

void VirtualIntersection::TriggerDepartureIR() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_departureIrActive = true;
    m_irResetTimer = 2.0;
}

void VirtualIntersection::AddVehiclesToQueue(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queueCount += count;
}

void VirtualIntersection::RemoveVehiclesFromQueue(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queueCount >= count) m_queueCount -= count;
    else m_queueCount = 0;
}

void VirtualIntersection::SetQueueCount(uint32_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queueCount = count;
}

void VirtualIntersection::SetApproachingAmbulance(const std::string& ambId, double etaSec) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentAmbulanceId = ambId;
    m_ambulanceEtaSec = etaSec;
}

void VirtualIntersection::Update(double dtSec) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Reset IR pulse timers
    if (m_irResetTimer > 0.0) {
        m_irResetTimer -= dtSec;
        if (m_irResetTimer <= 0.0) {
            m_approachIrActive = false;
            m_departureIrActive = false;
        }
    }

    if (m_isPreempted) {
        m_preemptionTimer -= dtSec;
        m_countdownSec = static_cast<int>(std::max(0.0, std::ceil(m_preemptionTimer)));
        if (m_preemptionTimer <= 0.0) {
            m_isPreempted = false;
            m_activeCorridorId.clear();
            m_phase = SignalPhaseState::CLEARANCE_YELLOW;
            m_signalColor = "YELLOW";
            m_phaseTimer = 4.0;
            m_countdownSec = 4;
        }
        return;
    }

    // Normal Cycle
    m_phaseTimer -= dtSec;
    m_countdownSec = static_cast<int>(std::max(0.0, std::ceil(m_phaseTimer)));

    if (m_phaseTimer <= 0.0) {
        switch (m_phase) {
            case SignalPhaseState::NORTH_SOUTH_GREEN:
                m_phase = SignalPhaseState::CLEARANCE_YELLOW;
                m_signalColor = "YELLOW";
                m_phaseTimer = 4.0;
                break;
            case SignalPhaseState::CLEARANCE_YELLOW:
                m_phase = SignalPhaseState::ALL_RED_HOLD;
                m_signalColor = "RED";
                m_phaseTimer = 2.0;
                break;
            case SignalPhaseState::ALL_RED_HOLD:
                m_phase = SignalPhaseState::EAST_WEST_GREEN;
                m_signalColor = "RED"; // Main avenue is RED while cross street is GREEN
                m_phaseTimer = 18.0;
                break;
            case SignalPhaseState::EAST_WEST_GREEN:
                m_phase = SignalPhaseState::NORTH_SOUTH_GREEN;
                m_signalColor = "GREEN";
                m_phaseTimer = 22.0;
                break;
            case SignalPhaseState::PREEMPTION_HOLD:
                m_phase = SignalPhaseState::CLEARANCE_YELLOW;
                m_signalColor = "YELLOW";
                m_phaseTimer = 4.0;
                break;
        }
        m_countdownSec = static_cast<int>(std::ceil(m_phaseTimer));
    }

    // Queue dissipation when green
    if (m_signalColor == "GREEN" && m_queueCount > 0) {
        if (static_cast<int>(m_countdownSec) % 2 == 0) {
            m_queueCount--;
        }
    }
}

std::string VirtualIntersection::GetSignalState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_signalColor;
}

SignalPhaseState VirtualIntersection::GetCurrentPhase() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_phase;
}

int VirtualIntersection::GetCountdownSec() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_countdownSec;
}

bool VirtualIntersection::IsPreempted() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isPreempted;
}

std::string VirtualIntersection::GetActiveCorridorId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeCorridorId;
}

uint32_t VirtualIntersection::GetQueueCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queueCount;
}

bool VirtualIntersection::IsApproachIrTriggered() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_approachIrActive;
}

bool VirtualIntersection::IsDepartureIrTriggered() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_departureIrActive;
}

std::string VirtualIntersection::GetCurrentApproachingAmbulance() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentAmbulanceId;
}

double VirtualIntersection::GetAmbulanceEtaSec() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ambulanceEtaSec;
}

} // namespace simulator
} // namespace aegis

