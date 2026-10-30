/**
 * @file TrafficSignalPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Signal Monitor Panel.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_SIGNAL_PANEL_H
#define AEGIS_DASHBOARD_TRAFFIC_SIGNAL_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Displays detailed real-time traffic signal state, countdown timer,
 *        pedestrian walk/stop indications, and preemption locks for the selected intersection.
 */
class TrafficSignalPanel : public QWidget {
    Q_OBJECT

public:
    explicit TrafficSignalPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                QWidget* parent = nullptr);
    ~TrafficSignalPanel() override = default;

    void RefreshData();

private slots:
    void OnIntersectionComboChanged(int index);

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    QComboBox* m_cmbIntersections{nullptr};
    QLabel* m_lblPhaseBadge{nullptr};
    QLabel* m_lblCountdownTimer{nullptr};
    QLabel* m_lblPreemptionBadge{nullptr};

    // Primary Signal Housing (North-South)
    QLabel* m_redLamp{nullptr};
    QLabel* m_yellowLamp{nullptr};
    QLabel* m_greenLamp{nullptr};

    // Secondary Signal Housing (East-West)
    QLabel* m_ewRedLamp{nullptr};
    QLabel* m_ewYellowLamp{nullptr};
    QLabel* m_ewGreenLamp{nullptr};

    // Pedestrian Crossing
    QLabel* m_lblPedestrianWalk{nullptr};
    QLabel* m_lblPedestrianStop{nullptr};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_SIGNAL_PANEL_H

