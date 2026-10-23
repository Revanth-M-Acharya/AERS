#include "aegis/dashboard/hospital_dashboard_backend.hpp"

namespace aegis {
namespace dashboard {

HospitalDashboardBackend::HospitalDashboardBackend() = default;
HospitalDashboardBackend::~HospitalDashboardBackend() = default;

bool HospitalDashboardBackend::Initialize(uint16_t webSocketPort) {
    m_port = webSocketPort;
    // TODO: Bind WebSocket server for real-time browser vitals streaming
    return true;
}

void HospitalDashboardBackend::BroadcastVitals(const std::string& /*patientId*/, const std::string& /*vitalsPayload*/) {
    // TODO: Push framed vitals packet to connected hospital web clients
}

} // namespace dashboard
} // namespace aegis
