/**
 * @file DashboardController.h
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Presenter & Runtime Controller.
 */

#ifndef AEGIS_DASHBOARD_DASHBOARD_CONTROLLER_H
#define AEGIS_DASHBOARD_DASHBOARD_CONTROLLER_H

#include "DashboardDataModel.h"
#include "DashboardConfig.h"
#include "runtime/CoordinatorRuntime.h"
#include "deployment/DeploymentManager.h"
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <memory>
#include <string>

namespace aegis {
namespace dashboard {

/**
 * @brief Coordinates runtime telemetry streaming, event subscription, simulation playback,
 *        and emergency dispatch operations between the UI data model and backend engines.
 */
class DashboardController : public QObject {
    Q_OBJECT

public:
    explicit DashboardController(std::shared_ptr<DashboardDataModel> dataModel,
                                 const DashboardConfig& config = DashboardConfig(),
                                 QObject* parent = nullptr);
    ~DashboardController() override;

    // Backend Binding
    void SetCoordinatorRuntime(std::shared_ptr<runtime::CoordinatorRuntime> coordinator);
    void SetDeploymentManager(std::shared_ptr<deployment::DeploymentManager> deployment);

    std::shared_ptr<runtime::CoordinatorRuntime> GetCoordinatorRuntime() const { return m_coordinatorRuntime; }
    std::shared_ptr<deployment::DeploymentManager> GetDeploymentManager() const { return m_deploymentManager; }
    std::shared_ptr<DashboardDataModel> GetDataModel() const { return m_dataModel; }

    // Lifecycle
    void Start();
    void Stop();
    bool IsRunning() const;

    // Simulation Controls
    void SetSimulationSpeed(double factor);
    double GetSimulationSpeed() const { return m_simSpeed; }

    void PauseSimulation();
    void ResumeSimulation();
    void TogglePause();
    bool IsSimulationPaused() const { return m_isPaused; }

    void ResetSimulation();

    // Emergency Operations
    runtime::EmergencyPipelineResult TriggerEmergency(const runtime::EmergencyPipelineInput& input);
    runtime::EmergencyPipelineResult TriggerQuickEmergency(const std::string& callId,
                                                          const std::string& description,
                                                          double lat = 12.9716,
                                                          double lon = 77.5946,
                                                          uint16_t hr = 135,
                                                          uint16_t spo2 = 88);

    // Synchronize telemetry from backend engines into data model
    void SyncFromBackend();

signals:
    void simulationSpeedChanged(double speed);
    void simulationPausedChanged(bool paused);
    void emergencyTriggered(const QString& emergencyId);
    void alertRaised(const QString& title, const QString& message, const QString& severity);

private slots:
    void OnTimerTick();

private:
    void AdvanceSimulationStep(double dtSec);

    std::shared_ptr<DashboardDataModel> m_dataModel;
    DashboardConfig m_config;

    std::shared_ptr<runtime::CoordinatorRuntime> m_coordinatorRuntime{nullptr};
    std::shared_ptr<deployment::DeploymentManager> m_deploymentManager{nullptr};

    QTimer* m_syncTimer{nullptr};
    double m_simSpeed{1.0};
    bool m_isPaused{false};
    uint64_t m_tickCount{0};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_DASHBOARD_CONTROLLER_H

