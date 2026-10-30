/**
 * @file TrafficDashboardWindow.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Intersection Operations Console Window.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_WINDOW_H
#define AEGIS_DASHBOARD_TRAFFIC_WINDOW_H

#include "TrafficDashboardController.h"
#include "IntersectionMapPanel.h"
#include "TrafficSignalPanel.h"
#include "QueueMonitorPanel.h"
#include "IrSensorPanel.h"
#include "NodeHealthPanel.h"
#include "GreenCorridorPanel.h"
#include "EventLogPanel.h"
#include "TrafficNotificationPanel.h"
#include "ManualControlPanel.h"

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Master application window for municipal traffic intersection monitoring and control.
 */
class TrafficDashboardWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit TrafficDashboardWindow(std::shared_ptr<TrafficDashboardController> controller,
                                    QWidget* parent = nullptr);
    ~TrafficDashboardWindow() override = default;

    std::shared_ptr<TrafficDashboardController> GetController() const { return m_controller; }
    std::shared_ptr<TrafficDashboardDataModel> GetDataModel() const;

    // View Accessors for testing & verification
    IntersectionMapPanel* GetMapPanel() const { return m_mapPanel; }
    TrafficSignalPanel* GetSignalPanel() const { return m_signalPanel; }
    QueueMonitorPanel* GetQueuePanel() const { return m_queuePanel; }
    IrSensorPanel* GetIrPanel() const { return m_irPanel; }
    NodeHealthPanel* GetHealthPanel() const { return m_healthPanel; }
    GreenCorridorPanel* GetCorridorPanel() const { return m_corridorPanel; }
    EventLogPanel* GetLogPanel() const { return m_logPanel; }
    ManualControlPanel* GetControlPanel() const { return m_controlPanel; }
    TrafficNotificationPanel* GetNotificationPanel() const { return m_notificationPanel; }

public slots:
    void RefreshAllViews();
    void OnNotificationAlert(const QString& title, const QString& message, const QString& severity);

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void SetupUi();
    void SetupToolbar();
    void SetupStatusBar();

    std::shared_ptr<TrafficDashboardController> m_controller;

    // Header Toolbar widgets
    QLabel* m_lblClock{nullptr};
    QPushButton* m_btnToggleMonitoring{nullptr};
    QPushButton* m_btnResetCamera{nullptr};
    QLabel* m_lblStatusSummary{nullptr};

    // Subsystem Panels
    IntersectionMapPanel* m_mapPanel{nullptr};
    TrafficSignalPanel* m_signalPanel{nullptr};
    QueueMonitorPanel* m_queuePanel{nullptr};
    IrSensorPanel* m_irPanel{nullptr};
    NodeHealthPanel* m_healthPanel{nullptr};
    GreenCorridorPanel* m_corridorPanel{nullptr};
    EventLogPanel* m_logPanel{nullptr};
    ManualControlPanel* m_controlPanel{nullptr};
    TrafficNotificationPanel* m_notificationPanel{nullptr};

    QTimer* m_clockTimer{nullptr};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_WINDOW_H

