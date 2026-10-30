/**
 * @file EmergencyTableWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Active Emergencies & Pipeline Visualizer Widget.
 */

#ifndef AEGIS_DASHBOARD_EMERGENCY_TABLE_WIDGET_H
#define AEGIS_DASHBOARD_EMERGENCY_TABLE_WIDGET_H

#include "DashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <memory>

namespace aegis {
namespace dashboard {

/**
 * @brief Left-side incident triage monitor displaying active calls, priority rankings, and the 10-stage animated pipeline.
 */
class EmergencyTableWidget : public QWidget {
    Q_OBJECT

public:
    explicit EmergencyTableWidget(std::shared_ptr<DashboardDataModel> dataModel,
                                 QWidget* parent = nullptr);
    ~EmergencyTableWidget() override = default;

    void RefreshData();

signals:
    void emergencySelected(const QString& incidentId);

private slots:
    void OnTableRowSelected(int row, int column);
    void OnFilterChanged(const QString& priorityFilter);

private:
    void SetupUi();
    void UpdatePipelineVisualizer(const EmergencyRecord& em);

    std::shared_ptr<DashboardDataModel> m_dataModel;
    QString m_activeFilter{"ALL"};

    QLabel* m_lblIncidentCount{nullptr};
    QTableWidget* m_tableWidget{nullptr};

    // Pipeline visualizer widgets
    QLabel* m_lblPipelineIncident{nullptr};
    QLabel* m_lblCurrentStageName{nullptr};
    QProgressBar* m_pipelineProgressBar{nullptr};
    QWidget* m_pipelineStagesContainer{nullptr};
    std::vector<QLabel*> m_stageLabels;
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_EMERGENCY_TABLE_WIDGET_H

