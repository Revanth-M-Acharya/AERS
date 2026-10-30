/**
 * @file TrafficDashboardController.h
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Controller & Presenter.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_CONTROLLER_H
#define AEGIS_DASHBOARD_TRAFFIC_CONTROLLER_H

#include "TrafficDashboardDataModel.h"
#include "TrafficDashboardConfig.h"
#include "runtime/CoordinatorRuntime.h"
#include "runtime/TrafficNodeRuntime.h"
#include "deployment/DeploymentManager.h"
#include "deployment/ClusterLauncher.h"

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <memory>
#include <string>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Coordinates runtime telemetry streaming, signal lifecycle orchestration,
 *        and manual traffic operator command routing.
 */
class TrafficDashboardController : public QObject {
    Q_OBJECT

public:
    explicit TrafficDashboardController(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                       const TrafficDashboardConfig& config = TrafficDashboardConfig(),
                                       QObject* parent = nullptr);
    ~TrafficDashboardController() override;

    // Runtime Bindings
    void SetCoordinatorRuntime(std::shared_ptr<runtime::CoordinatorRuntime> coordinator);
    void SetTrafficNodeRuntime(std::shared_ptr<runtime::TrafficNodeRuntime> trafficNode);
    void SetDeploymentManager(std::shared_ptr<deployment::DeploymentManager> deployment);
    void SetClusterLauncher(std::shared_ptr<deployment::ClusterLauncher> launcher);

    std::shared_ptr<runtime::CoordinatorRuntime> GetCoordinatorRuntime() const { return m_coordinatorRuntime; }
    std::shared_ptr<runtime::TrafficNodeRuntime> GetTrafficNodeRuntime() const { return m_trafficNodeRuntime; }
    std::shared_ptr<deployment::DeploymentManager> GetDeploymentManager() const { return m_deploymentManager; }
    std::shared_ptr<deployment::ClusterLauncher> GetClusterLauncher() const { return m_clusterLauncher; }
    std::shared_ptr<TrafficDashboardDataModel> GetDataModel() const { return m_dataModel; }

    // Monitoring Lifecycle
    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const;

    // Operator Controls
    void ResetIntersection(const std::string& nodeId);
    void RestartNode(const std::string& nodeId);
    void EnableManualOverride(const std::string& nodeId, const std::string& state = "FORCE_GREEN");
    void DisableManualOverride(const std::string& nodeId);
    void TriggerEmergencyPreemptionTest(const std::string& corridorId = "CORRIDOR_ALPHA");
    void ReleaseEmergencyPreemption();

    // Data Actions
    void SelectIntersection(const std::string& nodeId);
    void ClearNotifications();
    void ClearLogs();

    // Periodic Synchronization
    void SyncFromBackend();

signals:
    void monitoringStateChanged(bool isMonitoring);
    void manualOverrideToggled(const QString& nodeId, bool enabled, const QString& state);
    void preemptionTestTriggered(const QString& corridorId);
    void operatorActionCompleted(const QString& action, const QString& result);

private slots:
    void OnTimerTick();

private:
    void AdvanceCycleStep(double dtSec);

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;
    TrafficDashboardConfig m_config;

    std::shared_ptr<runtime::CoordinatorRuntime> m_coordinatorRuntime{nullptr};
    std::shared_ptr<runtime::TrafficNodeRuntime> m_trafficNodeRuntime{nullptr};
    std::shared_ptr<deployment::DeploymentManager> m_deploymentManager{nullptr};
    std::shared_ptr<deployment::ClusterLauncher> m_clusterLauncher{nullptr};

    QTimer* m_syncTimer{nullptr};
    uint64_t m_tickCounter{0};
    bool m_isMonitoring{false};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_CONTROLLER_H

