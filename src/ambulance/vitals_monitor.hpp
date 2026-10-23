#ifndef AEGIS_AMBULANCE_VITALS_MONITOR_HPP
#define AEGIS_AMBULANCE_VITALS_MONITOR_HPP

#include <cstdint>

namespace aegis {
namespace ambulance {

struct PatientVitals {
    uint16_t heartRate{0};
    uint8_t spO2{0};
    uint16_t systolicBP{0};
    uint16_t diastolicBP{0};
    uint8_t respirationRate{0};
    uint8_t gcsScore{15};
};

class VitalsMonitor {
public:
    VitalsMonitor() = default;
    ~VitalsMonitor() = default;

    PatientVitals SampleVitals();
    bool DetectAcuteDeterioration(const PatientVitals& v) const;
};

} // namespace ambulance
} // namespace aegis

#endif // AEGIS_AMBULANCE_VITALS_MONITOR_HPP
