/**
 * @file ManualControlPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Operator Manual Control & Override Panel.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_MANUAL_CONTROL_PANEL_H
#define AEGIS_DASHBOARD_TRAFFIC_MANUAL_CONTROL_PANEL_H

#include "TrafficDashboardController.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Operator console for intersection overrides, preemption testing,
 *        node restarts, and monitoring lifecycle control.
 */
class ManualControlPanel : public QWidget {
    Q_OBJECT

public:
    explicit ManualControlPanel(std::shared_ptr<TrafficDashboardController> controller,
                                QWidget* parent = nullptr);
    ~ManualControlPanel() override = default;

    void RefreshData();

private slots:
    void OnBtnMonitoringClicked();
    void OnBtnResetIntersectionClicked();
    void OnBtnRestartNodeClicked();
    void OnBtnForceGreenClicked();
    void OnBtnHoldRedClicked();
    void OnBtnReleaseOverrideClicked();
    void OnBtnPreemptionTestClicked();
    void OnBtnReleasePreemptionClicked();
    void OnBtnClearNotificationsClicked();

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardController> m_controller;

    QPushButton* m_btnToggleMonitoring{nullptr};
    QComboBox* m_cmbTargetNode{nullptr};
    QPushButton* m_btnResetIntersection{nullptr};
    QPushButton* m_btnRestartNode{nullptr};
    QPushButton* m_btnForceGreen{nullptr};
    QPushButton* m_btnHoldRed{nullptr};
    QPushButton* m_btnReleaseOverride{nullptr};
    QPushButton* m_btnPreemptionTest{nullptr};
    QPushButton* m_btnReleasePreemption{nullptr};
    QPushButton* m_btnClearNotifications{nullptr};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_MANUAL_CONTROL_PANEL_H

