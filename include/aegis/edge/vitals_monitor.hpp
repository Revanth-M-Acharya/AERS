#ifndef AEGIS_EDGE_VITALS_MONITOR_HPP
#define AEGIS_EDGE_VITALS_MONITOR_HPP

#include <cstdint>
#include "aegis/common/types.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

struct PatientVitals {
    uint16_t heartRate{75};
    uint8_t spO2{98};
    uint16_t systolicBP{120};
    uint16_t diastolicBP{80};
    uint8_t respirationRate{16};
    uint8_t gcsScore{15};
    uint64_t timestampMs{0};
};

/**
 * @brief Interfaces with biomedical monitors and detects acute clinical deterioration.
 * TODO: Implement sensor serial/BLE parser and threshold alarm triggers.
 */
class VitalsMonitor {
public:
    VitalsMonitor();
    ~VitalsMonitor();

    PatientVitals SampleVitals();
    bool DetectAcuteDeterioration(const PatientVitals& v) const;
};

} // namespace ambulance
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_VITALS_MONITOR_HPP
