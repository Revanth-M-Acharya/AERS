/**
 * @file TrafficStatusWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Traffic Signal Intersection Monitor Widget Implementation.
 */

#include "TrafficStatusWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>

namespace aegis {
namespace dashboard {

TrafficStatusWidget::TrafficStatusWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void TrafficStatusWidget::SetupUi() {
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background: transparent;");

    auto container = new QWidget(scrollArea);
    auto containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(4, 4, 4, 4);
    containerLayout->setSpacing(8);

    const std::vector<std::pair<std::string, std::string>> initialIntersections = {
        {"INT_A", "Brigade Road / MG Intersection"},
        {"INT_B", "MG Road / Residency Intersection"},
        {"INT_C", "Hospital Approach Intersection"}
    };

    for (const auto& [nodeId, name] : initialIntersections) {
        auto card = new QFrame(container);
        card->setObjectName("cardFrame");
        DashboardTheme::ApplyCardStyle(card);

        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(6);

        // Header: Name + Preemption Badge
        auto topRow = new QHBoxLayout();
        auto lblName = new QLabel(QString::fromStdString(nodeId + " — " + name), card);
        lblName->setFont(DashboardTheme::HeaderFont(10, true));
        lblName->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
        topRow->addWidget(lblName);
        topRow->addStretch();

        auto lblPreemption = new QLabel("CYCLE: NORMAL", card);
        lblPreemption->setFont(DashboardTheme::BadgeFont(8, true));
        DashboardTheme::ApplyBadgeStyle(lblPreemption, QColor("#1F2937"), QColor("#9CA3AF"));
        topRow->addWidget(lblPreemption);
        cardLayout->addLayout(topRow);

        // Middle Row: 3-Lamp Traffic Signal + Phase Info + Countdown
        auto midRow = new QHBoxLayout();
        midRow->setSpacing(12);

        // Signal Lamp Housing
        auto signalHousing = new QFrame(card);
        signalHousing->setStyleSheet("background-color: #030712; border: 1px solid #374151; border-radius: 6px;");
        auto lampLayout = new QHBoxLayout(signalHousing);
        lampLayout->setContentsMargins(6, 4, 6, 4);
        lampLayout->setSpacing(6);

        auto redLamp = new QLabel(signalHousing);
        redLamp->setFixedSize(14, 14);
        redLamp->setStyleSheet("background-color: #451A1A; border-radius: 7px;");

        auto yellowLamp = new QLabel(signalHousing);
        yellowLamp->setFixedSize(14, 14);
        yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 7px;");

        auto greenLamp = new QLabel(signalHousing);
        greenLamp->setFixedSize(14, 14);
        greenLamp->setStyleSheet("background-color: #063726; border-radius: 7px;");

        lampLayout->addWidget(redLamp);
        lampLayout->addWidget(yellowLamp);
        lampLayout->addWidget(greenLamp);
        midRow->addWidget(signalHousing);

        // Phase Details
        auto phaseLayout = new QVBoxLayout();
        auto lblPhase = new QLabel("PHASE: ALL-RED HOLD", card);
        lblPhase->setFont(DashboardTheme::BodyFont(9, true));
        lblPhase->setStyleSheet("color: #E5E7EB; border: none;");

        auto lblCountdown = new QLabel("REMAINING: 0s", card);
        lblCountdown->setFont(DashboardTheme::MetricFont(9, false));
        lblCountdown->setStyleSheet("color: #60A5FA; border: none;");

        phaseLayout->addWidget(lblPhase);
        phaseLayout->addWidget(lblCountdown);
        midRow->addLayout(phaseLayout);
        midRow->addStretch();

        // IR & Override telemetry
        auto telLayout = new QVBoxLayout();
        auto lblIr = new QLabel("IR SENSOR: ACTIVE (CLEAR)", card);
        lblIr->setFont(DashboardTheme::BodyFont(8, false));
        lblIr->setStyleSheet("color: #9CA3AF; border: none;");

        auto lblOver = new QLabel("CONTROL: AUTONOMOUS", card);
        lblOver->setFont(DashboardTheme::BodyFont(8, false));
        lblOver->setStyleSheet("color: #10B981; border: none;");

        telLayout->addWidget(lblIr);
        telLayout->addWidget(lblOver);
        midRow->addLayout(telLayout);

        cardLayout->addLayout(midRow);
        containerLayout->addWidget(card);

        IntersectionCardWidgets w;
        w.cardFrame = card;
        w.lblName = lblName;
        w.lblPreemptionBadge = lblPreemption;
        w.lamps.redLamp = redLamp;
        w.lamps.yellowLamp = yellowLamp;
        w.lamps.greenLamp = greenLamp;
        w.lblPhase = lblPhase;
        w.lblCountdown = lblCountdown;
        w.lblIrStatus = lblIr;
        w.lblOverride = lblOver;

        m_intersectionCards[nodeId] = w;
    }

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

void TrafficStatusWidget::RefreshData() {
    if (!m_dataModel) return;

    auto intersections = m_dataModel->GetIntersections();

    for (const auto& inter : intersections) {
        auto it = m_intersectionCards.find(inter.nodeId);
        if (it == m_intersectionCards.end()) continue;

        auto& w = it->second;

        // Preemption Badge
        if (inter.isPreempted) {
            w.lblPreemptionBadge->setText(QString("PREEMPTED [%1]").arg(QString::fromStdString(inter.activeCorridorId)));
            DashboardTheme::ApplyBadgeStyle(w.lblPreemptionBadge, QColor("#064E3B"), QColor("#34D399"));
            w.cardFrame->setStyleSheet(
                "QFrame#cardFrame { background-color: #111827; border: 1px solid #10B981; border-radius: 8px; }"
            );
        } else {
            w.lblPreemptionBadge->setText("CYCLE: NORMAL");
            DashboardTheme::ApplyBadgeStyle(w.lblPreemptionBadge, QColor("#1F2937"), QColor("#9CA3AF"));
            w.cardFrame->setStyleSheet(
                "QFrame#cardFrame { background-color: #111827; border: 1px solid #1F2937; border-radius: 8px; }"
            );
        }

        // 3-Lamp Lighting
        if (inter.signalState == "RED") {
            w.lamps.redLamp->setStyleSheet("background-color: #EF4444; border-radius: 7px;");
            w.lamps.yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 7px;");
            w.lamps.greenLamp->setStyleSheet("background-color: #063726; border-radius: 7px;");
        } else if (inter.signalState == "YELLOW") {
            w.lamps.redLamp->setStyleSheet("background-color: #451A1A; border-radius: 7px;");
            w.lamps.yellowLamp->setStyleSheet("background-color: #F59E0B; border-radius: 7px;");
            w.lamps.greenLamp->setStyleSheet("background-color: #063726; border-radius: 7px;");
        } else { // GREEN or PREEMPTION_GREEN
            w.lamps.redLamp->setStyleSheet("background-color: #451A1A; border-radius: 7px;");
            w.lamps.yellowLamp->setStyleSheet("background-color: #45300B; border-radius: 7px;");
            w.lamps.greenLamp->setStyleSheet("background-color: #10B981; border-radius: 7px;");
        }

        w.lblPhase->setText(QString("PHASE: %1").arg(QString::fromStdString(inter.currentPhase)));
        w.lblCountdown->setText(QString("REMAINING: %1s").arg(inter.countdownSec));
        w.lblIrStatus->setText(QString("IR SENSOR: %1").arg(QString::fromStdString(inter.irSensorActive ? "VEHICLE DETECTED" : "CLEAR")));
        w.lblOverride->setText(QString("CONTROL: %1").arg(QString::fromStdString(inter.manualOverrideActive ? "MANUAL OVERRIDE" : "AUTONOMOUS")));
        if (inter.manualOverrideActive) {
            w.lblOverride->setStyleSheet("color: #F59E0B; border: none; font-weight: bold;");
        } else {
            w.lblOverride->setStyleSheet("color: #10B981; border: none;");
        }
    }
}

} // namespace dashboard
} // namespace aegis

