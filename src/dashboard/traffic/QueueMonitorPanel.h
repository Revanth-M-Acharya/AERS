/**
 * @file QueueMonitorPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Intersection Queue & Congestion Monitor Panel.
 */

#ifndef AEGIS_DASHBOARD_QUEUE_MONITOR_PANEL_H
#define AEGIS_DASHBOARD_QUEUE_MONITOR_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Displays vehicle queue lengths, waiting times, traffic density, and lane occupancy.
 */
class QueueMonitorPanel : public QWidget {
    Q_OBJECT

public:
    explicit QueueMonitorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                               QWidget* parent = nullptr);
    ~QueueMonitorPanel() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    QLabel* m_lblQueueLength{nullptr};
    QLabel* m_lblWaitTime{nullptr};
    QLabel* m_lblThroughput{nullptr};
    QLabel* m_lblPeakBadge{nullptr};

    QProgressBar* m_progressDensity{nullptr};
    QLabel* m_lblDensityPct{nullptr};

    QProgressBar* m_progressOccupancy{nullptr};
    QLabel* m_lblOccupancyPct{nullptr};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_QUEUE_MONITOR_PANEL_H

