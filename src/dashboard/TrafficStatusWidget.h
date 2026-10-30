/**
 * @file TrafficStatusWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Traffic Signal Intersection Monitor Widget.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_STATUS_WIDGET_H
#define AEGIS_DASHBOARD_TRAFFIC_STATUS_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <memory>
#include <unordered_map>

namespace aegis {
namespace dashboard {

/**
 * @brief Displays detailed real-time state for each traffic intersection in the municipal network.
 */
class TrafficStatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit TrafficStatusWidget(std::shared_ptr<DashboardDataModel> dataModel,
                                QWidget* parent = nullptr);
    ~TrafficStatusWidget() override = default;

    void RefreshData();

private:
    void SetupUi();

    struct LampWidget {
        QLabel* redLamp{nullptr};
        QLabel* yellowLamp{nullptr};
        QLabel* greenLamp{nullptr};
    };

    struct IntersectionCardWidgets {
        QFrame* cardFrame{nullptr};
        QLabel* lblName{nullptr};
        QLabel* lblPreemptionBadge{nullptr};
        LampWidget lamps;
        QLabel* lblPhase{nullptr};
        QLabel* lblCountdown{nullptr};
        QLabel* lblIrStatus{nullptr};
        QLabel* lblOverride{nullptr};
    };

    std::shared_ptr<DashboardDataModel> m_dataModel;
    std::unordered_map<std::string, IntersectionCardWidgets> m_intersectionCards;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_STATUS_WIDGET_H

