/**
 * @file EventLogPanel.h
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Operations Event & Audit Log Panel.
 */

#ifndef AEGIS_DASHBOARD_TRAFFIC_EVENT_LOG_PANEL_H
#define AEGIS_DASHBOARD_TRAFFIC_EVENT_LOG_PANEL_H

#include "TrafficDashboardDataModel.h"
#include <QtWidgets/QWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <memory>

namespace aegis {
namespace dashboard {
namespace traffic {

/**
 * @brief Audit logging table with multi-criteria filtering, severity coloring, and CSV export.
 */
class EventLogPanel : public QWidget {
    Q_OBJECT

public:
    explicit EventLogPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                           QWidget* parent = nullptr);
    ~EventLogPanel() override = default;

    void RefreshData();
    bool ExportToCsv(const QString& filePath);
    void ClearLogs();

private slots:
    void OnSearchTextChanged(const QString& text);
    void OnCategoryFilterChanged(int index);
    void OnExportButtonClicked();

private:
    void SetupUi();
    void ApplyFilters();

    std::shared_ptr<TrafficDashboardDataModel> m_dataModel;

    QTableWidget* m_logTable{nullptr};
    QLineEdit* m_txtSearch{nullptr};
    QComboBox* m_cmbCategory{nullptr};
    QCheckBox* m_chkAutoScroll{nullptr};
    QPushButton* m_btnExport{nullptr};
    QPushButton* m_btnClear{nullptr};
    QLabel* m_lblCount{nullptr};

    size_t m_lastProcessedLogCount{0};
};

} // namespace traffic
} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_TRAFFIC_EVENT_LOG_PANEL_H

