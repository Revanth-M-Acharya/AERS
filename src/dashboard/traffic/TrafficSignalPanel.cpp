/**
 * @file TrafficSignalPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Signal Monitor Panel Implementation.
 */

#include "TrafficSignalPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficSignalPanel::TrafficSignalPanel(std::shared_ptr<TrafficDashboardDataModel> dataModel,
                                       QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void TrafficSignalPanel::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto card = new QFrame(this);
    card->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(card);

    auto cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 10);
    cardLayout->setSpacing(10);

    // Header: Title + Node Selector
    auto headerLayout = new QHBoxLayout();
    auto lblTitle = new QLabel("TRAFFIC SIGNAL MONITOR", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();

    m_cmbIntersections = new QComboBox(card);
    m_cmbIntersections->addItem("INT_A — Brigade Rd", "INT_A");
    m_cmbIntersections->addItem("INT_B — MG Rd", "INT_B");
    m_cmbIntersections->addItem("INT_C — Hospital Appr", "INT_C");
    connect(m_cmbIntersections, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TrafficSignalPanel::OnIntersectionComboChanged);
    headerLayout->addWidget(m_cmbIntersections);
    cardLayout->addLayout(headerLayout);

    // Preemption banner
    m_lblPreemptionBadge = new QLabel("CYCLE: AUTONOMOUS FIXED", card);
    m_lblPreemptionBadge->setFont(DashboardTheme::BadgeFont(8, true));
    DashboardTheme::ApplyBadgeStyle(m_lblPreemptionBadge, QColor("#1F2937"), QColor("#9CA3AF"));
    cardLayout->addWidget(m_lblPreemptionBadge);

    // Signal Heads + Countdown Display
    auto signalDisplayLayout = new QHBoxLayout();
    signalDisplayLayout->setSpacing(16);

    // 1. North-South Signal Housing
    auto nsBox = new QFrame(card);
    nsBox->setStyleSheet("background-color: #030712; border: 1px solid #374151; border-radius: 8px;");
    auto nsLayout = new QVBoxLayout(nsBox);
    nsLayout->setContentsMargins(8, 8, 8, 8);
    nsLayout->setSpacing(8);

    auto lblNsTitle = new QLabel("N-S", nsBox);
    lblNsTitle->setFont(DashboardTheme::BadgeFont(7, true));
    lblNsTitle->setStyleSheet("color: #9CA3AF; border: none;");
    lblNsTitle->setAlignment(Qt::AlignCenter);
    nsLayout->addWidget(lblNsTitle);

    m_redLamp = new QLabel(nsBox);
    m_redLamp->setFixedSize(28, 28);
    m_redLamp->setStyleSheet("background-color: #451A1A; border-radius: 14px;");
    nsLayout->addWidget(m_redLamp);

    m_yellowLamp = new QLabel(nsBox);
    m_yellowLamp->setFixedSize(28, 28);
    m_yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
    nsLayout->addWidget(m_yellowLamp);

    m_greenLamp = new QLabel(nsBox);
    m_greenLamp->setFixedSize(28, 28);
    m_greenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");
    nsLayout->addWidget(m_greenLamp);
    signalDisplayLayout->addWidget(nsBox);

    // 2. Center Countdown & Phase Info
    auto centerLayout = new QVBoxLayout();
    centerLayout->setSpacing(4);

    auto lblPhaseTitle = new QLabel("CURRENT ACTIVE PHASE", card);
    lblPhaseTitle->setFont(DashboardTheme::BodyFont(7, true));
    lblPhaseTitle->setStyleSheet("color: #9CA3AF; border: none;");
    centerLayout->addWidget(lblPhaseTitle);

    m_lblPhaseBadge = new QLabel("NORTH-SOUTH GREEN", card);
    m_lblPhaseBadge->setFont(DashboardTheme::HeaderFont(10, true));
    m_lblPhaseBadge->setStyleSheet("color: #10B981; font-weight: bold; border: none;");
    centerLayout->addWidget(m_lblPhaseBadge);

    auto lblCountTitle = new QLabel("REMAINING PHASE DURATION", card);
    lblCountTitle->setFont(DashboardTheme::BodyFont(7, false));
    lblCountTitle->setStyleSheet("color: #9CA3AF; border: none;");
    centerLayout->addWidget(lblCountTitle);

    m_lblCountdownTimer = new QLabel("18s", card);
    m_lblCountdownTimer->setFont(DashboardTheme::MetricFont(20, true));
    m_lblCountdownTimer->setStyleSheet("color: #60A5FA; font-weight: bold; border: none;");
    centerLayout->addWidget(m_lblCountdownTimer);

    signalDisplayLayout->addLayout(centerLayout, 1);

    // 3. East-West Secondary Signal Housing
    auto ewBox = new QFrame(card);
    ewBox->setStyleSheet("background-color: #030712; border: 1px solid #374151; border-radius: 8px;");
    auto ewLayout = new QVBoxLayout(ewBox);
    ewLayout->setContentsMargins(8, 8, 8, 8);
    ewLayout->setSpacing(8);

    auto lblEwTitle = new QLabel("E-W", ewBox);
    lblEwTitle->setFont(DashboardTheme::BadgeFont(7, true));
    lblEwTitle->setStyleSheet("color: #9CA3AF; border: none;");
    lblEwTitle->setAlignment(Qt::AlignCenter);
    ewLayout->addWidget(lblEwTitle);

    m_ewRedLamp = new QLabel(ewBox);
    m_ewRedLamp->setFixedSize(28, 28);
    m_ewRedLamp->setStyleSheet("background-color: #EF4444; border-radius: 14px;");
    ewLayout->addWidget(m_ewRedLamp);

    m_ewYellowLamp = new QLabel(ewBox);
    m_ewYellowLamp->setFixedSize(28, 28);
    m_ewYellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
    ewLayout->addWidget(m_ewYellowLamp);

    m_ewGreenLamp = new QLabel(ewBox);
    m_ewGreenLamp->setFixedSize(28, 28);
    m_ewGreenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");
    ewLayout->addWidget(m_ewGreenLamp);
    signalDisplayLayout->addWidget(ewBox);

    // 4. Pedestrian Crossing Housing
    auto pedBox = new QFrame(card);
    pedBox->setStyleSheet("background-color: #030712; border: 1px solid #374151; border-radius: 8px;");
    auto pedLayout = new QVBoxLayout(pedBox);
    pedLayout->setContentsMargins(8, 8, 8, 8);
    pedLayout->setSpacing(8);

    auto lblPedTitle = new QLabel("PED", pedBox);
    lblPedTitle->setFont(DashboardTheme::BadgeFont(7, true));
    lblPedTitle->setStyleSheet("color: #9CA3AF; border: none;");
    lblPedTitle->setAlignment(Qt::AlignCenter);
    pedLayout->addWidget(lblPedTitle);

    m_lblPedestrianStop = new QLabel("DON'T\nWALK", pedBox);
    m_lblPedestrianStop->setFixedSize(44, 38);
    m_lblPedestrianStop->setFont(DashboardTheme::BadgeFont(7, true));
    m_lblPedestrianStop->setAlignment(Qt::AlignCenter);
    m_lblPedestrianStop->setStyleSheet("background-color: #EF4444; color: #FFFFFF; border-radius: 6px; font-weight: bold;");
    pedLayout->addWidget(m_lblPedestrianStop);

    m_lblPedestrianWalk = new QLabel("WALK", pedBox);
    m_lblPedestrianWalk->setFixedSize(44, 38);
    m_lblPedestrianWalk->setFont(DashboardTheme::BadgeFont(7, true));
    m_lblPedestrianWalk->setAlignment(Qt::AlignCenter);
    m_lblPedestrianWalk->setStyleSheet("background-color: #063726; color: #6EE7B7; border-radius: 6px; font-weight: bold;");
    pedLayout->addWidget(m_lblPedestrianWalk);

    signalDisplayLayout->addWidget(pedBox);

    cardLayout->addLayout(signalDisplayLayout);
    mainLayout->addWidget(card);
}

void TrafficSignalPanel::OnIntersectionComboChanged(int index) {
    if (m_dataModel) {
        std::string nodeId = m_cmbIntersections->itemData(index).toString().toStdString();
        m_dataModel->SetSelectedIntersectionId(nodeId);
    }
}

void TrafficSignalPanel::RefreshData() {
    if (!m_dataModel) return;

    std::string selectedId = m_dataModel->GetSelectedIntersectionId();

    // Sync combo box without recursion
    for (int i = 0; i < m_cmbIntersections->count(); ++i) {
        if (m_cmbIntersections->itemData(i).toString().toStdString() == selectedId) {
            if (m_cmbIntersections->currentIndex() != i) {
                m_cmbIntersections->blockSignals(true);
                m_cmbIntersections->setCurrentIndex(i);
                m_cmbIntersections->blockSignals(false);
            }
            break;
        }
    }

    IntersectionTelemetry inter;
    if (!m_dataModel->GetIntersection(selectedId, inter)) {
        return;
    }

    // Preemption banner
    if (inter.isPreempted) {
        m_lblPreemptionBadge->setText(QString("EMERGENCY PREEMPTION LOCKED [%1]")
                                      .arg(QString::fromStdString(inter.activeCorridorId)));
        DashboardTheme::ApplyBadgeStyle(m_lblPreemptionBadge, QColor("#064E3B"), QColor("#34D399"));
    } else if (inter.manualOverrideActive) {
        m_lblPreemptionBadge->setText(QString("MANUAL OVERRIDE: %1")
                                      .arg(QString::fromStdString(inter.manualOverrideState)));
        DashboardTheme::ApplyBadgeStyle(m_lblPreemptionBadge, QColor("#78350F"), QColor("#FDE68A"));
    } else {
        m_lblPreemptionBadge->setText("CYCLE: AUTONOMOUS FIXED");
        DashboardTheme::ApplyBadgeStyle(m_lblPreemptionBadge, QColor("#1F2937"), QColor("#9CA3AF"));
    }

    // Countdown and Phase
    m_lblCountdownTimer->setText(QString("%1s").arg(inter.countdownSec));
    m_lblPhaseBadge->setText(QString::fromStdString(inter.currentPhase));

    // North-South Lamps
    if (inter.signalState == "RED") {
        m_redLamp->setStyleSheet("background-color: #EF4444; border-radius: 14px;");
        m_yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
        m_greenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");
        m_lblPhaseBadge->setStyleSheet("color: #EF4444; font-weight: bold; border: none;");

        // East-West is GREEN
        m_ewRedLamp->setStyleSheet("background-color: #451A1A; border-radius: 14px;");
        m_ewYellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
        m_ewGreenLamp->setStyleSheet("background-color: #10B981; border-radius: 14px;");

        // Pedestrian can walk across N-S
        m_lblPedestrianWalk->setStyleSheet("background-color: #10B981; color: #FFFFFF; border-radius: 6px; font-weight: bold;");
        m_lblPedestrianStop->setStyleSheet("background-color: #451A1A; color: #FCA5A5; border-radius: 6px; font-weight: bold;");
    } else if (inter.signalState == "YELLOW" || inter.signalState == "FLASHING_YELLOW") {
        m_redLamp->setStyleSheet("background-color: #451A1A; border-radius: 14px;");
        m_yellowLamp->setStyleSheet("background-color: #F59E0B; border-radius: 14px;");
        m_greenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");
        m_lblPhaseBadge->setStyleSheet("color: #F59E0B; font-weight: bold; border: none;");

        // East-West is RED
        m_ewRedLamp->setStyleSheet("background-color: #EF4444; border-radius: 14px;");
        m_ewYellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
        m_ewGreenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");

        // Pedestrian DON'T WALK
        m_lblPedestrianWalk->setStyleSheet("background-color: #063726; color: #6EE7B7; border-radius: 6px; font-weight: bold;");
        m_lblPedestrianStop->setStyleSheet("background-color: #EF4444; color: #FFFFFF; border-radius: 6px; font-weight: bold;");
    } else { // GREEN
        m_redLamp->setStyleSheet("background-color: #451A1A; border-radius: 14px;");
        m_yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
        m_greenLamp->setStyleSheet("background-color: #10B981; border-radius: 14px;");
        m_lblPhaseBadge->setStyleSheet("color: #10B981; font-weight: bold; border: none;");

        // East-West is RED
        m_ewRedLamp->setStyleSheet("background-color: #EF4444; border-radius: 14px;");
        m_ewYellowLamp->setStyleSheet("background-color: #45300B; border-radius: 14px;");
        m_ewGreenLamp->setStyleSheet("background-color: #063726; border-radius: 14px;");

        // Pedestrian DON'T WALK
        m_lblPedestrianWalk->setStyleSheet("background-color: #063726; color: #6EE7B7; border-radius: 6px; font-weight: bold;");
        m_lblPedestrianStop->setStyleSheet("background-color: #EF4444; color: #FFFFFF; border-radius: 6px; font-weight: bold;");
    }
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

