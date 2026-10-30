/**
 * @file CityStatusWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal City Operations Status Bar Widget.
 */

#ifndef AEGIS_DASHBOARD_CITY_STATUS_WIDGET_H
#define AEGIS_DASHBOARD_CITY_STATUS_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Top-level municipal situational awareness status bar displaying city alert levels and fleet load.
 */
class CityStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit CityStatusWidget(std::shared_ptr<DashboardDataModel> dataModel,
                              QWidget* parent = nullptr);
    ~CityStatusWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<DashboardDataModel> m_dataModel;

    QLabel* m_lblAlertLevel{nullptr};
    QLabel* m_lblActiveCorridors{nullptr};
    QLabel* m_lblPreemptedCount{nullptr};
    QLabel* m_lblAvgTransitEta{nullptr};
    QProgressBar* m_progressFleet{nullptr};
    QLabel* m_lblFleetPct{nullptr};
    QLabel* m_lblTotalHandled{nullptr};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_CITY_STATUS_WIDGET_H

