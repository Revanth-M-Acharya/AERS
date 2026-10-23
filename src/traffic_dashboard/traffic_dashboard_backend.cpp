#include "aegis/dashboard/traffic_dashboard_backend.hpp"

namespace aegis {
namespace dashboard {

TrafficDashboardBackend::TrafficDashboardBackend() = default;
TrafficDashboardBackend::~TrafficDashboardBackend() = default;

bool TrafficDashboardBackend::Initialize(uint16_t webSocketPort) {
    m_port = webSocketPort;
    // TODO: Bind WebSocket server for municipal traffic supervisory console
    return true;
}

void TrafficDashboardBackend::BroadcastPhaseState(const std::string& /*intersectionId*/, const std::string& /*stateJson*/) {
    // TODO: Stream intersection state changes to connected GIS clients
}

} // namespace dashboard
} // namespace aegis
