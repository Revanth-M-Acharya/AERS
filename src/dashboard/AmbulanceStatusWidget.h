/**
 * @file AmbulanceStatusWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Fleet & Patient Telemetry Monitor Widget.
 */

#ifndef AEGIS_DASHBOARD_AMBULANCE_STATUS_WIDGET_H
#define AEGIS_DASHBOARD_AMBULANCE_STATUS_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <memory>
#include <unordered_map>

namespace aegis {
namespace dashboard {

/**
 * @brief Displays ambulance fleet units, live telemetry, navigation target, and onboard patient vitals.
 */
class AmbulanceStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit AmbulanceStatusWidget(std::shared_ptr<DashboardDataModel> dataModel,
                                  QWidget* parent = nullptr);
    ~AmbulanceStatusWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    struct AmbulanceCardWidgets {
        QFrame* cardFrame{nullptr};
        QLabel* lblVehicleId{nullptr};
        QLabel* lblTierBadge{nullptr};
        QLabel* lblStatusBadge{nullptr};
        QLabel* lblSpeedHeading{nullptr};
        QLabel* lblDestination{nullptr};
        QLabel* lblEta{nullptr};
        QLabel* lblVitalsHr{nullptr};
        QLabel* lblVitalsSpo2{nullptr};
        QLabel* lblVitalsBp{nullptr};
        QLabel* lblVitalsGcs{nullptr};
    };

    std::shared_ptr<DashboardDataModel> m_dataModel;
    std::unordered_map<std::string, AmbulanceCardWidgets> m_ambulanceCards;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_AMBULANCE_STATUS_WIDGET_H

