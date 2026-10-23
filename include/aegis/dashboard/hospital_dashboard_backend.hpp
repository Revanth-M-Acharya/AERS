#ifndef AEGIS_DASHBOARD_HOSPITAL_DASHBOARD_BACKEND_HPP
#define AEGIS_DASHBOARD_HOSPITAL_DASHBOARD_BACKEND_HPP

#include <string>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Backend telemetry service for Hospital Emergency Room Intake Console.
 * TODO: Implement WebSocket server and clinical advice dispatcher.
 */
class HospitalDashboardBackend {
public:
    HospitalDashboardBackend();
    ~HospitalDashboardBackend();

    bool Initialize(uint16_t webSocketPort);
    void BroadcastVitals(const std::string& patientId, const std::string& vitalsPayload);

private:
    uint16_t m_port{9001};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_HOSPITAL_DASHBOARD_BACKEND_HPP
