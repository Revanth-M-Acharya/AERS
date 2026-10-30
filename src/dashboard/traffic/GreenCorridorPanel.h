/**
 * @file GreenCorridorPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Dynamic Green Corridor & Preemption Visualizer Panel.
 */

#ifndef AEGIS_DASHBOARD_GREEN_CORRIDOR_PANEL_H
#define AEGIS_DASHBOARD_GREEN_CORRIDOR_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <memory>
#include <vector>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Displays active green corridor sequence, preemption progress,
 *        direction, ETA, and priority vehicle telemetry.
 */
class GreenCorridorPanel : public QWidget {
    Q_OBJECT

public:
    explicit GreenCorridorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                QWidget* parent = nullptr);
    ~GreenCorridorPanel() override = default;

    void RefreshData();

private:
    void SetupUi();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    QLabel* m_lblCorridorId{nullptr};
    QLabel* m_lblPriorityBadge{nullptr};
    QLabel* m_lblVehicleId{nullptr};
    QLabel* m_lblDirection{nullptr};
    QLabel* m_lblEtaSec{nullptr};
    QLabel* m_lblTransitionStage{nullptr};

    QProgressBar* m_progressPreemption{nullptr};
    QLabel* m_lblPreemptionWindow{nullptr};

    // Step-by-step intersection progression sequence
    struct SequenceStepWidgets {
        QLabel* lblNode;
        QLabel* lblArrow;
    };
    std::vector<SequenceStepWidgets> m_stepWidgets;
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_GREEN_CORRIDOR_PANEL_H

