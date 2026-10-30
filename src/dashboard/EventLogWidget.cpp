/**
 * @file EventLogWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Audit Event Log & Filter Widget Implementation.
 */

#include "EventLogWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>

namespace aegis {
namespace dashboard {

EventLogWidget::EventLogWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void EventLogWidget::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    auto lblTitle = new QLabel("AUDIT & EVENT LOG", this);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold;");
    toolbarLayout->addWidget(lblTitle);

    m_lblCount = new QLabel("0 EVENTS", this);
    m_lblCount->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblCount, QColor("#1F2937"), QColor("#9CA3AF"));
    toolbarLayout->addWidget(m_lblCount);

    toolbarLayout->addSpacing(16);

    // Search filter
    m_txtSearch = new QLineEdit(this);
    m_txtSearch->setPlaceholderText("Filter messages or components...");
    m_txtSearch->setFixedWidth(240);
    connect(m_txtSearch, &QLineEdit::textChanged, this, &EventLogWidget::OnSearchTextChanged);
    toolbarLayout->addWidget(m_txtSearch);

    // Severity combo
    m_cmbSeverity = new QComboBox(this);
    m_cmbSeverity->addItem("ALL SEVERITIES", "ALL");
    m_cmbSeverity->addItem("INFO ONLY", "INFO");
    m_cmbSeverity->addItem("WARN ONLY", "WARN");
    m_cmbSeverity->addItem("ERROR ONLY", "ERROR");
    m_cmbSeverity->addItem("CRITICAL ONLY", "CRITICAL");
    connect(m_cmbSeverity, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EventLogWidget::OnSeverityFilterChanged);
    toolbarLayout->addWidget(m_cmbSeverity);

    toolbarLayout->addStretch();

    // Auto scroll checkbox
    m_chkAutoScroll = new QCheckBox("Auto-Scroll", this);
    m_chkAutoScroll->setChecked(true);
    toolbarLayout->addWidget(m_chkAutoScroll);

    // Export button
    m_btnExport = new QPushButton("Export CSV", this);
    m_btnExport->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 8pt; }");
    connect(m_btnExport, &QPushButton::clicked, this, &EventLogWidget::OnExportButtonClicked);
    toolbarLayout->addWidget(m_btnExport);

    // Clear button
    m_btnClear = new QPushButton("Clear", this);
    m_btnClear->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 8pt; }");
    connect(m_btnClear, &QPushButton::clicked, this, &EventLogWidget::ClearLogs);
    toolbarLayout->addWidget(m_btnClear);

    mainLayout->addLayout(toolbarLayout);

    // Table
    m_logTable = new QTableWidget(this);
    m_logTable->setColumnCount(4);
    m_logTable->setHorizontalHeaderLabels({"TIMESTAMP", "SEVERITY", "COMPONENT", "MESSAGE"});
    m_logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_logTable->setColumnWidth(0, 140);
    m_logTable->setColumnWidth(1, 90);
    m_logTable->setColumnWidth(2, 130);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);

    mainLayout->addWidget(m_logTable);
}

void EventLogWidget::RefreshData() {
    if (!m_dataModel) return;

    auto logs = m_dataModel->GetLogs();
    if (logs.size() == m_lastProcessedLogCount) {
        return;
    }

    m_logTable->setRowCount(static_cast<int>(logs.size()));
    m_lblCount->setText(QString("%1 EVENTS").arg(logs.size()));

    for (int r = 0; r < static_cast<int>(logs.size()); ++r) {
        const auto& log = logs[static_cast<size_t>(r)];

        auto itemTime = new QTableWidgetItem(QString::fromStdString(log.timestampIso));
        itemTime->setFont(DashboardTheme::MetricFont(8, false));
        itemTime->setForeground(QBrush(QColor("#9CA3AF")));

        auto itemSev = new QTableWidgetItem(QString::fromStdString(log.severity));
        itemSev->setFont(DashboardTheme::BadgeFont(8, true));
        itemSev->setTextAlignment(Qt::AlignCenter);

        if (log.severity == "CRITICAL") {
            itemSev->setForeground(QBrush(QColor("#EF4444")));
        } else if (log.severity == "ERROR") {
            itemSev->setForeground(QBrush(QColor("#F87171")));
        } else if (log.severity == "WARN") {
            itemSev->setForeground(QBrush(QColor("#F59E0B")));
        } else {
            itemSev->setForeground(QBrush(QColor("#10B981")));
        }

        auto itemComp = new QTableWidgetItem(QString::fromStdString(log.component));
        itemComp->setFont(DashboardTheme::BodyFont(8, true));
        itemComp->setForeground(QBrush(QColor("#60A5FA")));

        auto itemMsg = new QTableWidgetItem(QString::fromStdString(log.message));
        itemMsg->setFont(DashboardTheme::BodyFont(8, false));
        itemMsg->setForeground(QBrush(QColor("#F3F4F6")));

        m_logTable->setItem(r, 0, itemTime);
        m_logTable->setItem(r, 1, itemSev);
        m_logTable->setItem(r, 2, itemComp);
        m_logTable->setItem(r, 3, itemMsg);
    }

    m_lastProcessedLogCount = logs.size();
    ApplyFilters();

    if (m_chkAutoScroll->isChecked() && m_logTable->rowCount() > 0) {
        m_logTable->scrollToBottom();
    }
}

void EventLogWidget::ApplyFilters() {
    QString search = m_txtSearch->text().trimmed();
    QString sevFilter = m_cmbSeverity->currentData().toString();

    int rowCount = m_logTable->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        bool show = true;

        if (sevFilter != "ALL") {
            auto itemSev = m_logTable->item(r, 1);
            if (itemSev && itemSev->text() != sevFilter) {
                show = false;
            }
        }

        if (show && !search.isEmpty()) {
            auto itemComp = m_logTable->item(r, 2);
            auto itemMsg = m_logTable->item(r, 3);
            QString compText = itemComp ? itemComp->text() : "";
            QString msgText = itemMsg ? itemMsg->text() : "";

            if (!compText.contains(search, Qt::CaseInsensitive) &&
                !msgText.contains(search, Qt::CaseInsensitive)) {
                show = false;
            }
        }

        m_logTable->setRowHidden(r, !show);
    }
}

void EventLogWidget::OnSearchTextChanged(const QString& /*text*/) {
    ApplyFilters();
}

void EventLogWidget::OnSeverityFilterChanged(int /*index*/) {
    ApplyFilters();
}

void EventLogWidget::ClearLogs() {
    if (m_dataModel) {
        m_dataModel->ClearLogs();
    }
    m_logTable->setRowCount(0);
    m_lastProcessedLogCount = 0;
    m_lblCount->setText("0 EVENTS");
}

bool EventLogWidget::ExportToCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "Timestamp,Severity,Component,Message\n";

    int rowCount = m_logTable->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        QString time = m_logTable->item(r, 0) ? m_logTable->item(r, 0)->text() : "";
        QString sev = m_logTable->item(r, 1) ? m_logTable->item(r, 1)->text() : "";
        QString comp = m_logTable->item(r, 2) ? m_logTable->item(r, 2)->text() : "";
        QString msg = m_logTable->item(r, 3) ? m_logTable->item(r, 3)->text() : "";

        msg.replace("\"", "\"\"");
        out << "\"" << time << "\",\"" << sev << "\",\"" << comp << "\",\"" << msg << "\"\n";
    }

    file.close();
    return true;
}

void EventLogWidget::OnExportButtonClicked() {
    QString defaultName = QString("aegis_audit_log_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "Export Audit Logs", defaultName, "CSV Files (*.csv)");
    if (!filePath.isEmpty()) {
        ExportToCsv(filePath);
    }
}

} // namespace dashboard
} // namespace aegis

