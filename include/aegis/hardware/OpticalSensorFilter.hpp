/**
 * @file OpticalSensorFilter.hpp
 * @brief SMART AEGIS (AERS) Phase 20.6 - Optical IR Sensor Noise Filter & Debounce Engine.
 *
 * Implements deterministic digital noise filtration, false-trigger rejection,
 * and debouncing for roadside optical IR approach and departure sensors on Raspberry Pi 4.
 */

#ifndef AEGIS_HARDWARE_OPTICAL_SENSOR_FILTER_HPP
#define AEGIS_HARDWARE_OPTICAL_SENSOR_FILTER_HPP

#include <cstdint>
#include <mutex>
#include <string>

namespace aegis {
namespace hardware {

struct OpticalFilterStats {
    uint64_t rawSamplesProcessed{0};
    uint64_t noiseSpikesFiltered{0};
    uint64_t qualifiedActivations{0};
    uint64_t qualifiedDeactivations{0};
    uint64_t stuckFaultCount{0};
    uint32_t currentActiveDurationMs{0};
    bool isBeamBroken{false};
    bool isStuckFault{false};
};

class OpticalSensorFilter {
public:
    explicit OpticalSensorFilter(uint32_t debounceMs = 50, uint32_t stuckTimeoutMs = 30000);
    ~OpticalSensorFilter() = default;

    // Prevent copy/move
    OpticalSensorFilter(const OpticalSensorFilter&) = delete;
    OpticalSensorFilter& operator=(const OpticalSensorFilter&) = delete;

    /**
     * @brief Process a digital logic sample from the GPIO IR sensor pin.
     * @param rawLevelActive True if the optical beam is interrupted (active LOW or HIGH per wiring).
     * @param sampleDtMs Time step since previous sample in milliseconds.
     * @return True if a new qualified state change occurred on this sample.
     */
    bool ProcessSample(bool rawLevelActive, uint32_t sampleDtMs);

    /**
     * @brief Reset filter state.
     */
    void Reset();

    // State queries
    bool IsBeamBroken() const;
    bool IsStuckFault() const;
    uint32_t GetActiveDurationMs() const;
    uint64_t GetNoiseSpikeCount() const;
    uint64_t GetQualifiedActivationCount() const;
    uint64_t GetQualifiedDeactivationCount() const;
    OpticalFilterStats GetStatistics() const;

    // Parameter configuration
    void SetDebounceMs(uint32_t debounceMs);
    uint32_t GetDebounceMs() const;
    void SetStuckTimeoutMs(uint32_t stuckTimeoutMs);
    uint32_t GetStuckTimeoutMs() const;

private:
    mutable std::mutex m_mutex;
    uint32_t m_debounceMs{50};
    uint32_t m_stuckTimeoutMs{30000};

    bool m_rawState{false};
    bool m_filteredState{false};
    bool m_stuckFault{false};

    uint32_t m_continuousRawActiveMs{0};
    uint32_t m_continuousRawInactiveMs{0};
    uint32_t m_continuousFilteredActiveMs{0};

    uint64_t m_rawSamplesProcessed{0};
    uint64_t m_noiseSpikesFiltered{0};
    uint64_t m_qualifiedActivations{0};
    uint64_t m_qualifiedDeactivations{0};
    uint64_t m_stuckFaultCount{0};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_OPTICAL_SENSOR_FILTER_HPP

