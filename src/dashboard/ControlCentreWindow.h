/**
 * @file ControlCentreWindow.h
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Main Application Window.
 */

#ifndef AEGIS_DASHBOARD_CONTROL_CENTRE_WINDOW_H
#define AEGIS_DASHBOARD_CONTROL_CENTRE_WINDOW_H

#include "DashboardController.h"
#include "EmergencyTableWidget.h"
#include "CityStatusWidget.h"
#include "ClusterStatusWidget.h"
#include "TrafficStatusWidget.h"
#include "HospitalStatusWidget.h"
#include "AmbulanceStatusWidget.h"
#include "NodeHealthWidget.h"
#include "EventLogWidget.h"
#include "NotificationPanel.h"
#include "MapScene.h"

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QLabel>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Main EOC Window integrating multi-panel municipal oversight, live vector map,
 *        pipeline visualizer, cluster diagnostics, and emergency dispatch controls.
 */
class ControlCentreWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ControlCentreWindow(std::shared_ptr<DashboardController> controller,
                                QWidget* parent = nullptr);
    ~ControlCentreWindow() override = default;

    std::shared_ptr<DashboardController> GetController() const { return m_controller; }
    std::shared_ptr<DashboardDataModel> GetDataModel() const;

    // View Accessors for testing & verification
    EmergencyTableWidget* GetEmergencyTable() const { return m_emergencyTable; }
    CityStatusWidget* GetCityStatus() const { return m_cityStatus; }
    ClusterStatusWidget* GetClusterStatus() const { return m_clusterStatus; }
    TrafficStatusWidget* GetTrafficStatus() const { return m_trafficStatus; }
    HospitalStatusWidget* GetHospitalStatus() const { return m_hospitalStatus; }
    AmbulanceStatusWidget* GetAmbulanceStatus() const { return m_ambulanceStatus; }
    NodeHealthWidget* GetNodeHealth() const { return m_nodeHealth; }
    EventLogWidget* GetEventLog() const { return m_eventLog; }
    NotificationPanel* GetNotificationPanel() const { return m_notificationPanel; }
    MapScene* GetMapScene() const { return m_mapScene; }

public slots:
    void OnTriggerEmergencyClicked();
    void OnTogglePauseClicked();
    void OnSpeedChanged(int index);
    void OnResetClicked();
    void OnNotificationAlert(const QString& title, const QString& message, const QString& severity);
    void RefreshAllViews();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void SetupUi();
    void SetupHeader();
    void SetupShortcuts();

    std::shared_ptr<DashboardController> m_controller;

    // Top Header Widgets
    QLabel* m_lblClock{nullptr};
    QPushButton* m_btnPause{nullptr};
    QComboBox* m_cmbSpeed{nullptr};
    QPushButton* m_btnReset{nullptr};
    QPushButton* m_btnTriggerEmergency{nullptr};

    // Panels
    CityStatusWidget* m_cityStatus{nullptr};
    EmergencyTableWidget* m_emergencyTable{nullptr};
    MapScene* m_mapScene{nullptr};
    QTabWidget* m_rightTabs{nullptr};
    ClusterStatusWidget* m_clusterStatus{nullptr};
    NodeHealthWidget* m_nodeHealth{nullptr};
    TrafficStatusWidget* m_trafficStatus{nullptr};
    AmbulanceStatusWidget* m_ambulanceStatus{nullptr};
    HospitalStatusWidget* m_hospitalStatus{nullptr};
    EventLogWidget* m_eventLog{nullptr};
    NotificationPanel* m_notificationPanel{nullptr};

    QTimer* m_clockTimer{nullptr};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_CONTROL_CENTRE_WINDOW_H

