/**
 * @file DashboardAdapter.h
 * @brief SMART AEGIS (AERS) Phase 19 - Event-Driven Digital Twin Dashboard Publisher Adapter.
 */

#ifndef AEGIS_SIMULATOR_DASHBOARD_ADAPTER_H
#define AEGIS_SIMULATOR_DASHBOARD_ADAPTER_H

#include "dashboard/DashboardDataModel.h"
#include "dashboard/traffic/TrafficDashboardDataModel.h"
#include "CityModel.h"

#include <memory>
#include <mutex>

namespace aegis {
namespace simulator {

/**
 * @brief Streams real-time digital twin state directly to both Phase 17 and Phase 18 dashboards.
 */
class DashboardAdapter {
public:
    DashboardAdapter();
    ~DashboardAdapter() = default;

    void SetControlCentreDataModel(std::shared_ptr<dashboard::DashboardDataModel> model);
    void SetTrafficDashboardDataModel(std::shared_ptr<dashboard::traffic::TrafficDashboardDataModel> model);

    // Publish World Snapshot
    void PublishSnapshot(const CityModel& city, double elapsedSimSeconds);

private:
    mutable std::mutex m_mutex;
    std::shared_ptr<dashboard::DashboardDataModel> m_controlCentreModel{nullptr};
    std::shared_ptr<dashboard::traffic::TrafficDashboardDataModel> m_trafficModel{nullptr};
};

} // namespace simulator
} // namespace aegis

#endif // AEGIS_SIMULATOR_DASHBOARD_ADAPTER_H

