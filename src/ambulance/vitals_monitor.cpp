#include "aegis/edge/vitals_monitor.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

VitalsMonitor::VitalsMonitor() = default;
VitalsMonitor::~VitalsMonitor() = default;

PatientVitals VitalsMonitor::SampleVitals() {
    PatientVitals v;
    // TODO: Ingest physiological data from biomedical serial monitor or mock stream
    return v;
}

bool VitalsMonitor::DetectAcuteDeterioration(const PatientVitals& v) const {
    if (v.spO2 > 0 && v.spO2 < 88) return true;
    if (v.systolicBP > 0 && v.systolicBP < 80) return true;
    return false;
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
