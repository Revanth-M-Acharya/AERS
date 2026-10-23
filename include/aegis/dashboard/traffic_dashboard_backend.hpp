#ifndef AEGIS_DASHBOARD_TRAFFIC_DASHBOARD_BACKEND_HPP
#define AEGIS_DASHBOARD_TRAFFIC_DASHBOARD_BACKEND_HPP

#include <string>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Backend telemetry service for Municipal Traffic Supervisory Console.
 * TODO: Implement WebSocket server and authenticated manual override receiver.
 */
class TrafficDashboardBackend {
public:
    TrafficDashboardBackend();
    ~TrafficDashboardBackend();

    bool Initialize(uint16_t webSocketPort);
    void BroadcastPhaseState(const std::string& intersectionId, const std::string& stateJson);

private:
    uint16_t m_port{9002};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_DASHBOARD_BACKEND_HPP
