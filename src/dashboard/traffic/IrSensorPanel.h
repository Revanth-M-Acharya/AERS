/**
 * @file IrSensorPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Optical IR Vehicle Detection Sensor Panel.
 */

#ifndef AEGIS_DASHBOARD_IR_SENSOR_PANEL_H
#define AEGIS_DASHBOARD_IR_SENSOR_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Displays approach and departure IR sensor states, health, detection counts,
 *        and timestamps for Phase 20 physical sensor hardware integration.
 */
class IrSensorPanel : public QWidget {
    Q_OBJECT

public:
    explicit IrSensorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                           QWidget* parent = nullptr);
    ~IrSensorPanel() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    // Approach Sensor Widgets
    QLabel* m_lblApproachStatus{nullptr};
    QLabel* m_lblApproachHealth{nullptr};
    QLabel* m_lblApproachCount{nullptr};

    // Departure Sensor Widgets
    QLabel* m_lblDepartureStatus{nullptr};
    QLabel* m_lblDepartureHealth{nullptr};
    QLabel* m_lblDepartureCount{nullptr};

    // Last detection timestamp
    QLabel* m_lblLastTimestamp{nullptr};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_IR_SENSOR_PANEL_H

