/**
 * @file GreenCorridorPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Dynamic Green Corridor & Preemption Visualizer Panel Implementation.
 */

#include "GreenCorridorPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

GreenCorridorPanel::GreenCorridorPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                       QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void GreenCorridorPanel::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto card = new QFrame(this);
    card->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(card);

    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(8);

    // Header
    auto headerLayout = new QHBoxLayout();
    auto lblTitle = new QLabel("GREEN WAVE CORRIDOR PREEMPTION", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();

    m_lblPriorityBadge = new QLabel("STANDBY", card);
    m_lblPriorityBadge->setFont(DashboardTheme::BadgeFont(7, true));
    DashboardTheme::ApplyBadgeStyle(m_lblPriorityBadge, QColor("#1F2937"), QColor("#9CA3AF"));
    headerLayout->addWidget(m_lblPriorityBadge);
    cardLayout->addLayout(headerLayout);

    // Meta details row
    auto metaRow = new QHBoxLayout();
    metaRow->setSpacing(12);

    auto cLayout = new QVBoxLayout();
    auto lblCTitle = new QLabel("CORRIDOR ID", card);
    lblCTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblCTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblCorridorId = new QLabel("STANDBY", card);
    m_lblCorridorId->setFont(DashboardTheme::MetricFont(10, true));
    m_lblCorridorId->setStyleSheet("color: #60A5FA; font-weight: bold; border: none;");
    cLayout->addWidget(lblCTitle);
    cLayout->addWidget(m_lblCorridorId);
    metaRow->addLayout(cLayout);

    auto vLayout = new QVBoxLayout();
    auto lblVTitle = new QLabel("EMERGENCY VEHICLE", card);
    lblVTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblVTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblVehicleId = new QLabel("AMB_01", card);
    m_lblVehicleId->setFont(DashboardTheme::MetricFont(10, true));
    m_lblVehicleId->setStyleSheet("color: #10B981; font-weight: bold; border: none;");
    vLayout->addWidget(lblVTitle);
    vLayout->addWidget(m_lblVehicleId);
    metaRow->addLayout(vLayout);

    auto dLayout = new QVBoxLayout();
    auto lblDTitle = new QLabel("DIRECTION / STAGE", card);
    lblDTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblDTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblDirection = new QLabel("NORTH_BOUND • STANDBY", card);
    m_lblDirection->setFont(DashboardTheme::BodyFont(8, true));
    m_lblDirection->setStyleSheet("color: #E5E7EB; border: none;");
    dLayout->addWidget(lblDTitle);
    dLayout->addWidget(m_lblDirection);
    metaRow->addLayout(dLayout);

    auto etaLayout = new QVBoxLayout();
    auto lblEtaTitle = new QLabel("EST. TRANSIT ETA", card);
    lblEtaTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblEtaTitle->setStyleSheet("color: #9CA3AF; border: none;");
    m_lblEtaSec = new QLabel("0s", card);
    m_lblEtaSec->setFont(DashboardTheme::MetricFont(10, true));
    m_lblEtaSec->setStyleSheet("color: #F59E0B; font-weight: bold; border: none;");
    etaLayout->addWidget(lblEtaTitle);
    etaLayout->addWidget(m_lblEtaSec);
    metaRow->addLayout(etaLayout);

    cardLayout->addLayout(metaRow);

    // Sequence Ribbon Box
    auto seqBox = new QFrame(card);
    seqBox->setStyleSheet("background-color: #030712; border: 1px solid #1F2937; border-radius: 6px;");
    auto seqLayout = new QHBoxLayout(seqBox);
    seqLayout->setContentsMargins(8, 6, 8, 6);
    seqLayout->setSpacing(6);

    const std::vector<std::string> defaultSeq = {"INT_A", "INT_B", "INT_C"};
    m_stepWidgets.clear();

    for (size_t i = 0; i < defaultSeq.size(); ++i) {
        auto lblNode = new QLabel(QString::fromStdString(defaultSeq[i]), seqBox);
        lblNode->setFont(DashboardTheme::BadgeFont(8, true));
        DashboardTheme::ApplyBadgeStyle(lblNode, QColor("#1F2937"), QColor("#9CA3AF"));
        seqLayout->addWidget(lblNode);

        QLabel* lblArrow = nullptr;
        if (i + 1 < defaultSeq.size()) {
            lblArrow = new QLabel("➔", seqBox);
            lblArrow->setFont(DashboardTheme::BodyFont(9, true));
            lblArrow->setStyleSheet("color: #4B5563; border: none;");
            seqLayout->addWidget(lblArrow);
        }
        m_stepWidgets.push_back({lblNode, lblArrow});
    }
    seqLayout->addStretch();
    cardLayout->addWidget(seqBox);

    // Preemption window progress bar
    auto winRow = new QHBoxLayout();
    m_lblPreemptionWindow = new QLabel("Preemption Window: Inactive", card);
    m_lblPreemptionWindow->setFont(DashboardTheme::BodyFont(7, false));
    m_lblPreemptionWindow->setStyleSheet("color: #9CA3AF; border: none;");
    winRow->addWidget(m_lblPreemptionWindow);

    m_progressPreemption = new QProgressBar(card);
    m_progressPreemption->setFixedHeight(6);
    m_progressPreemption->setTextVisible(false);
    m_progressPreemption->setRange(0, 100);
    m_progressPreemption->setValue(0);
    DashboardTheme::ApplyProgressBarStyle(m_progressPreemption, QColor("#10B981"));
    winRow->addWidget(m_progressPreemption);
    cardLayout->addLayout(winRow);

    mainLayout->addWidget(card);
}

void GreenCorridorPanel::RefreshData() {
    if (!m_dataModel) return;

    auto sched = m_dataModel->GetCorridorSchedule();

    if (sched.activeCorridorId.empty()) {
        m_lblPriorityBadge->setText("STANDBY");
        DashboardTheme::ApplyBadgeStyle(m_lblPriorityBadge, QColor("#1F2937"), QColor("#9CA3AF"));
        m_lblCorridorId->setText("NONE");
        m_lblVehicleId->setText("--");
        m_lblDirection->setText("AUTONOMOUS NORMAL CYCLE");
        m_lblEtaSec->setText("0s");
        m_lblPreemptionWindow->setText("Preemption Window: Standby");
        m_progressPreemption->setValue(0);

        for (auto& step : m_stepWidgets) {
            DashboardTheme::ApplyBadgeStyle(step.lblNode, QColor("#1F2937"), QColor("#9CA3AF"));
            if (step.lblArrow) step.lblArrow->setStyleSheet("color: #4B5563; border: none;");
        }
    } else {
        m_lblPriorityBadge->setText(QString::fromStdString(sched.emergencyVehiclePriority));
        DashboardTheme::ApplyBadgeStyle(m_lblPriorityBadge, QColor("#7F1D1D"), QColor("#FCA5A5"));

        m_lblCorridorId->setText(QString::fromStdString(sched.activeCorridorId));
        m_lblVehicleId->setText(QString::fromStdString(sched.assignedAmbulanceId));
        m_lblDirection->setText(QString("%1 • %2")
                                .arg(QString::fromStdString(sched.corridorDirection))
                                .arg(QString::fromStdString(sched.signalTransitionStage)));
        m_lblEtaSec->setText(QString("%1s").arg(sched.estimatedArrivalTimeSec, 0, 'f', 0));

        m_lblPreemptionWindow->setText(QString("Preemption Window: Active (%1s duration)")
                                       .arg(sched.preemptionWindowDurationSec, 0, 'f', 0));
        m_progressPreemption->setValue(75);

        for (auto& step : m_stepWidgets) {
            DashboardTheme::ApplyBadgeStyle(step.lblNode, QColor("#064E3B"), QColor("#34D399"));
            if (step.lblArrow) step.lblArrow->setStyleSheet("color: #10B981; border: none;");
        }
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

