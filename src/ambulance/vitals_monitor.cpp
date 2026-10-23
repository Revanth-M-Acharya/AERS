#include "vitals_monitor.hpp"

namespace aegis {
namespace ambulance {

PatientVitals VitalsMonitor::SampleVitals() {
    PatientVitals v;
    // Placeholder: Ingest from biomedical sensor interface
    return v;
}

bool VitalsMonitor::DetectAcuteDeterioration(const PatientVitals& v) const {
    if (v.spO2 > 0 && v.spO2 < 88) return true;
    if (v.systolicBP > 0 && v.systolicBP < 80) return true;
    return false;
}

} // namespace ambulance
} // namespace aegis
