/**
 * @file OpticalSensorFilter.cpp
 * @brief SMART AEGIS (AERS) Phase 20.6 - Optical IR Sensor Filter Implementation.
 */

#include "aegis/hardware/OpticalSensorFilter.hpp"

namespace aegis {
namespace hardware {

OpticalSensorFilter::OpticalSensorFilter(uint32_t debounceMs, uint32_t stuckTimeoutMs)
    : m_debounceMs(debounceMs)
    , m_stuckTimeoutMs(stuckTimeoutMs)
{
}

bool OpticalSensorFilter::ProcessSample(bool rawLevelActive, uint32_t sampleDtMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rawSamplesProcessed++;
    bool stateChanged = false;

    if (rawLevelActive) {
        m_continuousRawActiveMs += sampleDtMs;
        m_continuousRawInactiveMs = 0;

        // Transition from inactive to active only after debounce threshold is met
        if (!m_filteredState) {
            if (m_continuousRawActiveMs >= m_debounceMs) {
                m_filteredState = true;
                m_continuousFilteredActiveMs = m_continuousRawActiveMs;
                m_qualifiedActivations++;
                stateChanged = true;
            }
        } else {
            // Already filtered active, accumulate duration
            m_continuousFilteredActiveMs += sampleDtMs;

            // Check stuck condition
            if (m_stuckTimeoutMs > 0 && m_continuousFilteredActiveMs >= m_stuckTimeoutMs) {
                if (!m_stuckFault) {
                    m_stuckFault = true;
                    m_stuckFaultCount++;
                }
            }
        }
    } else {
        // Raw level is inactive
        m_continuousRawInactiveMs += sampleDtMs;

        // If raw level previously showed a brief active spike that didn't meet debounce threshold
        if (!m_filteredState && m_continuousRawActiveMs > 0 && m_continuousRawActiveMs < m_debounceMs) {
            m_noiseSpikesFiltered++;
        }
        m_continuousRawActiveMs = 0;

        // Transition from active to inactive only after release debounce threshold is met
        if (m_filteredState) {
            if (m_continuousRawInactiveMs >= m_debounceMs) {
                m_filteredState = false;
                m_stuckFault = false;
                m_continuousFilteredActiveMs = 0;
                m_qualifiedDeactivations++;
                stateChanged = true;
            } else {
                // Still in release debounce window, continue counting active
                m_continuousFilteredActiveMs += sampleDtMs;
            }
        }
    }

    m_rawState = rawLevelActive;
    return stateChanged;
}

void OpticalSensorFilter::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rawState = false;
    m_filteredState = false;
    m_stuckFault = false;
    m_continuousRawActiveMs = 0;
    m_continuousRawInactiveMs = 0;
    m_continuousFilteredActiveMs = 0;
}

bool OpticalSensorFilter::IsBeamBroken() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_filteredState;
}

bool OpticalSensorFilter::IsStuckFault() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stuckFault;
}

uint32_t OpticalSensorFilter::GetActiveDurationMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_continuousFilteredActiveMs;
}

uint64_t OpticalSensorFilter::GetNoiseSpikeCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_noiseSpikesFiltered;
}

uint64_t OpticalSensorFilter::GetQualifiedActivationCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_qualifiedActivations;
}

uint64_t OpticalSensorFilter::GetQualifiedDeactivationCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_qualifiedDeactivations;
}

OpticalFilterStats OpticalSensorFilter::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    OpticalFilterStats stats;
    stats.rawSamplesProcessed = m_rawSamplesProcessed;
    stats.noiseSpikesFiltered = m_noiseSpikesFiltered;
    stats.qualifiedActivations = m_qualifiedActivations;
    stats.qualifiedDeactivations = m_qualifiedDeactivations;
    stats.stuckFaultCount = m_stuckFaultCount;
    stats.currentActiveDurationMs = m_continuousFilteredActiveMs;
    stats.isBeamBroken = m_filteredState;
    stats.isStuckFault = m_stuckFault;
    return stats;
}

void OpticalSensorFilter::SetDebounceMs(uint32_t debounceMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_debounceMs = debounceMs;
}

uint32_t OpticalSensorFilter::GetDebounceMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_debounceMs;
}

void OpticalSensorFilter::SetStuckTimeoutMs(uint32_t stuckTimeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stuckTimeoutMs = stuckTimeoutMs;
}

uint32_t OpticalSensorFilter::GetStuckTimeoutMs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stuckTimeoutMs;
}

} // namespace hardware
} // namespace aegis

