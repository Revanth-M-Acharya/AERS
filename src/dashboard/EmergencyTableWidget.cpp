/**
 * @file EmergencyTableWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Active Emergencies & Pipeline Visualizer Implementation.
 */

#include "EmergencyTableWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>

namespace aegis {
namespace dashboard {

EmergencyTableWidget::EmergencyTableWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void EmergencyTableWidget::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    // 1. Header Card
    auto headerCard = new QFrame(this);
    headerCard->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(headerCard);

    auto headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(10, 8, 10, 8);

    auto lblTitle = new QLabel("ACTIVE EMERGENCIES", headerCard);
    lblTitle->setFont(DashboardTheme::HeaderFont(11, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold;");

    m_lblIncidentCount = new QLabel("0 ACTIVE", headerCard);
    m_lblIncidentCount->setFont(DashboardTheme::BadgeFont(9, true));
    DashboardTheme::ApplyBadgeStyle(m_lblIncidentCount, QColor("#7F1D1D"), QColor("#FCA5A5"));

    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(m_lblIncidentCount);
    mainLayout->addWidget(headerCard);

    // 2. Incident Queue Table
    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setColumnCount(5);
    m_tableWidget->setHorizontalHeaderLabels({"PRIORITY", "ID", "CONDITION", "UNIT", "ETA"});
    m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setAlternatingRowColors(false);

    connect(m_tableWidget, &QTableWidget::cellClicked, this, &EmergencyTableWidget::OnTableRowSelected);
    mainLayout->addWidget(m_tableWidget, 1);

    // 3. Emergency Pipeline Visualization Card
    auto pipelineCard = new QFrame(this);
    pipelineCard->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(pipelineCard);

    auto pipelineLayout = new QVBoxLayout(pipelineCard);
    pipelineLayout->setContentsMargins(10, 8, 10, 8);
    pipelineLayout->setSpacing(6);

    auto pipelineTitleLayout = new QHBoxLayout();
    auto lblPipeTitle = new QLabel("EMERGENCY PIPELINE PROGRESS", pipelineCard);
    lblPipeTitle->setFont(DashboardTheme::TitleFont(10, true));
    lblPipeTitle->setStyleSheet("color: #9CA3AF;");

    m_lblPipelineIncident = new QLabel("NO SELECTION", pipelineCard);
    m_lblPipelineIncident->setFont(DashboardTheme::MonospaceFont(9, true));
    m_lblPipelineIncident->setStyleSheet("color: #06B6D4;");

    pipelineTitleLayout->addWidget(lblPipeTitle);
    pipelineTitleLayout->addStretch();
    pipelineTitleLayout->addWidget(m_lblPipelineIncident);
    pipelineLayout->addLayout(pipelineTitleLayout);

    m_lblCurrentStageName = new QLabel("Standby - Awaiting Emergency Ingestion", pipelineCard);
    m_lblCurrentStageName->setFont(DashboardTheme::HeaderFont(10, true));
    m_lblCurrentStageName->setStyleSheet("color: #F9FAFB;");
    pipelineLayout->addWidget(m_lblCurrentStageName);

    m_pipelineProgressBar = new QProgressBar(pipelineCard);
    m_pipelineProgressBar->setRange(0, 100);
    m_pipelineProgressBar->setValue(0);
    m_pipelineProgressBar->setFixedHeight(14);
    pipelineLayout->addWidget(m_pipelineProgressBar);

    // 10-Stage Mini Icons/Labels
    m_pipelineStagesContainer = new QWidget(pipelineCard);
    auto stagesLayout = new QHBoxLayout(m_pipelineStagesContainer);
    stagesLayout->setContentsMargins(0, 4, 0, 0);
    stagesLayout->setSpacing(2);

    const std::vector<QString> stageNames = {
        "1.Call", "2.Triage", "3.Fleet", "4.Hosp", "5.Route",
        "6.Repln", "7.Corridor", "8.Consensus", "9.Signal", "10.Arrived"
    };

    m_stageLabels.clear();
    for (const auto& name : stageNames) {
        auto lbl = new QLabel(name, m_pipelineStagesContainer);
        lbl->setFont(DashboardTheme::MonospaceFont(7, true));
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #4B5563; background-color: #1F2937; border-radius: 2px; padding: 2px;");
        stagesLayout->addWidget(lbl);
        m_stageLabels.push_back(lbl);
    }
    pipelineLayout->addWidget(m_pipelineStagesContainer);

    mainLayout->addWidget(pipelineCard);
}

void EmergencyTableWidget::RefreshData() {
    if (!m_dataModel) return;

    auto emergencies = m_dataModel->GetAllActiveEmergencies();
    m_lblIncidentCount->setText(QString("%1 ACTIVE").arg(emergencies.size()));

    m_tableWidget->setRowCount(static_cast<int>(emergencies.size()));
    for (size_t i = 0; i < emergencies.size(); ++i) {
        const auto& em = emergencies[i];

        // Col 0: Priority Pill
        auto pillLabel = new QLabel(QString::fromStdString(em.priorityLevel));
        pillLabel->setAlignment(Qt::AlignCenter);
        pillLabel->setFont(DashboardTheme::BadgeFont(8, true));
        pillLabel->setStyleSheet(DashboardTheme::PriorityPillStyleSheet(QString::fromStdString(em.priorityLevel)));
        m_tableWidget->setCellWidget(static_cast<int>(i), 0, pillLabel);

        // Col 1: ID
        auto item1 = new QTableWidgetItem(QString::fromStdString(em.incidentId));
        item1->setFont(DashboardTheme::MonospaceFont(9, true));
        m_tableWidget->setItem(static_cast<int>(i), 1, item1);

        // Col 2: Condition
        auto item2 = new QTableWidgetItem(QString::fromStdString(em.condition));
        m_tableWidget->setItem(static_cast<int>(i), 2, item2);

        // Col 3: Unit
        auto item3 = new QTableWidgetItem(QString::fromStdString(em.assignedAmbulanceId));
        item3->setFont(DashboardTheme::MonospaceFont(9));
        m_tableWidget->setItem(static_cast<int>(i), 3, item3);

        // Col 4: ETA
        auto item4 = new QTableWidgetItem(QString("%1s").arg(em.etaSeconds));
        item4->setFont(DashboardTheme::MonospaceFont(9, true));
        item4->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tableWidget->setItem(static_cast<int>(i), 4, item4);
    }

    if (!emergencies.empty()) {
        UpdatePipelineVisualizer(emergencies.front());
    } else {
        m_lblPipelineIncident->setText("NO SELECTION");
        m_lblCurrentStageName->setText("Standby - Awaiting Emergency Ingestion");
        m_pipelineProgressBar->setValue(0);
        for (auto* lbl : m_stageLabels) {
            lbl->setStyleSheet("color: #4B5563; background-color: #1F2937; border-radius: 2px; padding: 2px;");
        }
    }
}

void EmergencyTableWidget::UpdatePipelineVisualizer(const EmergencyRecord& em) {
    m_lblPipelineIncident->setText(QString::fromStdString(em.incidentId));
    m_lblCurrentStageName->setText(QString("Active Stage: %1").arg(QString::fromStdString(PipelineStageToString(em.currentStage))));

    int stageIdx = static_cast<int>(em.currentStage);
    int progress = std::min(100, std::max(0, (stageIdx * 10) + static_cast<int>(em.stageProgress * 10.0)));
    m_pipelineProgressBar->setValue(progress);

    // Update each stage indicator label
    for (size_t i = 0; i < m_stageLabels.size(); ++i) {
        if (static_cast<int>(i) < stageIdx) {
            // Completed
            m_stageLabels[i]->setStyleSheet("color: #FFFFFF; background-color: #065F46; border: 1px solid #10B981; border-radius: 2px; padding: 2px;");
        } else if (static_cast<int>(i) == stageIdx) {
            // Active
            m_stageLabels[i]->setStyleSheet("color: #FFFFFF; background-color: #1E3A8A; border: 1px solid #3B82F6; border-radius: 2px; padding: 2px; font-weight: bold;");
        } else {
            // Pending
            m_stageLabels[i]->setStyleSheet("color: #4B5563; background-color: #1F2937; border-radius: 2px; padding: 2px;");
        }
    }
}

void EmergencyTableWidget::OnTableRowSelected(int row, int /*column*/) {
    if (!m_dataModel) return;
    auto item = m_tableWidget->item(row, 1);
    if (!item) return;

    QString incId = item->text();
    EmergencyRecord em;
    if (m_dataModel->GetEmergency(incId.toStdString(), em)) {
        UpdatePipelineVisualizer(em);
        emit emergencySelected(incId);
    }
}

void EmergencyTableWidget::OnFilterChanged(const QString& priorityFilter) {
    m_activeFilter = priorityFilter;
    RefreshData();
}

} // namespace dashboard
} // namespace aegis

