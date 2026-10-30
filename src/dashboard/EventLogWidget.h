/**
 * @file EventLogWidget.h
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Audit Event Log & Filter Widget.
 */

#ifndef AEGIS_DASHBOARD_EVENT_LOG_WIDGET_H
#define AEGIS_DASHBOARD_EVENT_LOG_WIDGET_H

#include "DashboardDataModel.h"
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

/**
 * @brief Audit logging table with multi-criteria filtering, severity coloring, and CSV export.
 */
class EventLogWidget : public QWidget {
    Q_OBJECT

public:
    explicit EventLogWidget(std::shared_ptr<DashboardDataModel> dataModel,
                            QWidget* parent = nullptr);
    ~EventLogWidget() override = default;

    void RefreshData();
    bool ExportToCsv(const QString& filePath);
    void ClearLogs();

private slots:
    void OnSearchTextChanged(const QString& text);
    void OnSeverityFilterChanged(int index);
    void OnExportButtonClicked();

private:
    void SetupUi();
    void ApplyFilters();

    std::shared_ptr<DashboardDataModel> m_dataModel;

    QTableWidget* m_logTable{nullptr};
    QLineEdit* m_txtSearch{nullptr};
    QComboBox* m_cmbSeverity{nullptr};
    QCheckBox* m_chkAutoScroll{nullptr};
    QPushButton* m_btnExport{nullptr};
    QPushButton* m_btnClear{nullptr};
    QLabel* m_lblCount{nullptr};

    size_t m_lastProcessedLogCount{0};
};

} // namespace dashboard
} // namespace aegis

#endif // AEGIS_DASHBOARD_EVENT_LOG_WIDGET_H

