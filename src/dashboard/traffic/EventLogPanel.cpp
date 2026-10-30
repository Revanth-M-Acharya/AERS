/**
 * @file EventLogPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Operations Event & Audit Log Panel Implementation.
 */

#include "EventLogPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QFileDialog>
#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QDateTime>

namespace aegis {
namespace dashboard {
namespace traffic {

EventLogPanel::EventLogPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                             QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void EventLogPanel::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 6, 8, 8);
    mainLayout->setSpacing(6);

    // Toolbar
    auto toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(8);

    auto lblTitle = new QLabel("TRAFFIC AUDIT & EVENT LOG", this);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    toolbarLayout->addWidget(lblTitle);

    m_lblCount = new QLabel("0 EVENTS", this);
    m_lblCount->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblCount, QColor("#1F2937"), QColor("#9CA3AF"));
    toolbarLayout->addWidget(m_lblCount);

    toolbarLayout->addSpacing(16);

    // Search filter
    m_txtSearch = new QLineEdit(this);
    m_txtSearch->setPlaceholderText("Filter messages or nodes...");
    m_txtSearch->setFixedWidth(220);
    connect(m_txtSearch, &QLineEdit::textChanged, this, &EventLogPanel::OnSearchTextChanged);
    toolbarLayout->addWidget(m_txtSearch);

    // Category filter combo
    m_cmbCategory = new QComboBox(this);
    m_cmbCategory->addItem("ALL CATEGORIES", "ALL");
    m_cmbCategory->addItem("SIGNAL ONLY", "SIGNAL");
    m_cmbCategory->addItem("HEARTBEAT ONLY", "HEARTBEAT");
    m_cmbCategory->addItem("REGISTRATION ONLY", "REGISTRATION");
    m_cmbCategory->addItem("FAILURE ONLY", "FAILURE");
    m_cmbCategory->addItem("RECOVERY ONLY", "RECOVERY");
    m_cmbCategory->addItem("PREEMPTION ONLY", "PREEMPTION");
    m_cmbCategory->addItem("OVERRIDE ONLY", "OVERRIDE");
    m_cmbCategory->addItem("DIAGNOSTIC ONLY", "DIAGNOSTIC");
    connect(m_cmbCategory, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EventLogPanel::OnCategoryFilterChanged);
    toolbarLayout->addWidget(m_cmbCategory);

    toolbarLayout->addStretch();

    // Auto-scroll
    m_chkAutoScroll = new QCheckBox("Auto-Scroll", this);
    m_chkAutoScroll->setChecked(true);
    toolbarLayout->addWidget(m_chkAutoScroll);

    // Export CSV
    m_btnExport = new QPushButton("Export CSV", this);
    m_btnExport->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 8pt; }");
    connect(m_btnExport, &QPushButton::clicked, this, &EventLogPanel::OnExportButtonClicked);
    toolbarLayout->addWidget(m_btnExport);

    // Clear
    m_btnClear = new QPushButton("Clear", this);
    m_btnClear->setStyleSheet("QPushButton { padding: 4px 10px; font-size: 8pt; }");
    connect(m_btnClear, &QPushButton::clicked, this, &EventLogPanel::ClearLogs);
    toolbarLayout->addWidget(m_btnClear);

    mainLayout->addLayout(toolbarLayout);

    // Table
    m_logTable = new QTableWidget(this);
    m_logTable->setColumnCount(5);
    m_logTable->setHorizontalHeaderLabels({"TIMESTAMP", "NODE", "SEVERITY", "CATEGORY", "MESSAGE"});
    m_logTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_logTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    m_logTable->setColumnWidth(0, 130);
    m_logTable->setColumnWidth(1, 80);
    m_logTable->setColumnWidth(2, 85);
    m_logTable->setColumnWidth(3, 110);
    m_logTable->verticalHeader()->setVisible(false);
    m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_logTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_logTable->setAlternatingRowColors(true);

    mainLayout->addWidget(m_logTable);
}

void EventLogPanel::RefreshData() {
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

        auto itemNode = new QTableWidgetItem(QString::fromStdString(log.nodeId));
        itemNode->setFont(DashboardTheme::BodyFont(8, true));
        itemNode->setTextAlignment(Qt::AlignCenter);
        itemNode->setForeground(QBrush(QColor("#60A5FA")));

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

        auto itemCat = new QTableWidgetItem(QString::fromStdString(log.category));
        itemCat->setFont(DashboardTheme::BadgeFont(8, false));
        itemCat->setForeground(QBrush(QColor("#06B6D4")));

        auto itemMsg = new QTableWidgetItem(QString::fromStdString(log.message));
        itemMsg->setFont(DashboardTheme::BodyFont(8, false));
        itemMsg->setForeground(QBrush(QColor("#F3F4F6")));

        m_logTable->setItem(r, 0, itemTime);
        m_logTable->setItem(r, 1, itemNode);
        m_logTable->setItem(r, 2, itemSev);
        m_logTable->setItem(r, 3, itemCat);
        m_logTable->setItem(r, 4, itemMsg);
    }

    m_lastProcessedLogCount = logs.size();
    ApplyFilters();

    if (m_chkAutoScroll->isChecked() && m_logTable->rowCount() > 0) {
        m_logTable->scrollToBottom();
    }
}

void EventLogPanel::ApplyFilters() {
    QString search = m_txtSearch->text().trimmed();
    QString catFilter = m_cmbCategory->currentData().toString();

    int rowCount = m_logTable->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        bool show = true;

        if (catFilter != "ALL") {
            auto itemCat = m_logTable->item(r, 3);
            if (itemCat && itemCat->text() != catFilter) {
                show = false;
            }
        }

        if (show && !search.isEmpty()) {
            auto itemNode = m_logTable->item(r, 1);
            auto itemMsg = m_logTable->item(r, 4);
            QString nodeText = itemNode ? itemNode->text() : "";
            QString msgText = itemMsg ? itemMsg->text() : "";

            if (!nodeText.contains(search, Qt::CaseInsensitive) &&
                !msgText.contains(search, Qt::CaseInsensitive)) {
                show = false;
            }
        }

        m_logTable->setRowHidden(r, !show);
    }
}

void EventLogPanel::OnSearchTextChanged(const QString& /*text*/) {
    ApplyFilters();
}

void EventLogPanel::OnCategoryFilterChanged(int /*index*/) {
    ApplyFilters();
}

void EventLogPanel::ClearLogs() {
    if (m_dataModel) {
        m_dataModel->ClearLogs();
    }
    m_logTable->setRowCount(0);
    m_lastProcessedLogCount = 0;
    m_lblCount->setText("0 EVENTS");
}

bool EventLogPanel::ExportToCsv(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "Timestamp,Node,Severity,Category,Message\n";

    int rowCount = m_logTable->rowCount();
    for (int r = 0; r < rowCount; ++r) {
        QString time = m_logTable->item(r, 0) ? m_logTable->item(r, 0)->text() : "";
        QString node = m_logTable->item(r, 1) ? m_logTable->item(r, 1)->text() : "";
        QString sev = m_logTable->item(r, 2) ? m_logTable->item(r, 2)->text() : "";
        QString cat = m_logTable->item(r, 3) ? m_logTable->item(r, 3)->text() : "";
        QString msg = m_logTable->item(r, 4) ? m_logTable->item(r, 4)->text() : "";

        msg.replace("\"", "\"\"");
        out << "\"" << time << "\",\"" << node << "\",\"" << sev << "\",\"" << cat << "\",\"" << msg << "\"\n";
    }

    file.close();
    return true;
}

void EventLogPanel::OnExportButtonClicked() {
    QString defaultName = QString("traffic_audit_log_%1.csv")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString filePath = QFileDialog::getSaveFileName(this, "Export Traffic Audit Logs", defaultName, "CSV Files (*.csv)");
    if (!filePath.isEmpty()) {
        ExportToCsv(filePath);
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

