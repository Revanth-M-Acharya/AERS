/**
 * @file AmbulanceStatusWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Fleet & Patient Telemetry Monitor Widget Implementation.
 */

#include "AmbulanceStatusWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>

namespace aegis {
namespace dashboard {

AmbulanceStatusWidget::AmbulanceStatusWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void AmbulanceStatusWidget::SetupUi() {
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

    const std::vector<std::pair<std::string, std::string>> initialFleet = {
        {"AMB_01", "ALS ADVANCED"},
        {"AMB_02", "ALS ADVANCED"},
        {"AMB_03", "BLS BASIC"}
    };

    for (const auto& [id, tier] : initialFleet) {
        auto card = new QFrame(container);
        card->setObjectName("cardFrame");
        DashboardTheme::ApplyCardStyle(card);

        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(6);

        // Header: ID + Tier Badge + Status Badge
        auto topRow = new QHBoxLayout();
        auto lblId = new QLabel(QString::fromStdString(id), card);
        lblId->setFont(DashboardTheme::HeaderFont(10, true));
        lblId->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
        topRow->addWidget(lblId);
        topRow->addStretch();

        auto lblTier = new QLabel(QString::fromStdString(tier), card);
        lblTier->setFont(DashboardTheme::BadgeFont(7, true));
        DashboardTheme::ApplyBadgeStyle(lblTier, QColor("#1E3A8A"), QColor("#93C5FD"));
        topRow->addWidget(lblTier);

        auto lblStatus = new QLabel("AVAILABLE", card);
        lblStatus->setFont(DashboardTheme::BadgeFont(7, true));
        DashboardTheme::ApplyBadgeStyle(lblStatus, QColor("#064E3B"), QColor("#6EE7B7"));
        topRow->addWidget(lblStatus);
        cardLayout->addLayout(topRow);

        // Movement Row: Speed / Heading / Dest / ETA
        auto moveRow = new QHBoxLayout();
        auto lblSpeed = new QLabel("0 km/h • 0°", card);
        lblSpeed->setFont(DashboardTheme::MetricFont(8, false));
        lblSpeed->setStyleSheet("color: #60A5FA; border: none;");
        moveRow->addWidget(lblSpeed);
        moveRow->addStretch();

        auto lblDest = new QLabel("Standby at Station", card);
        lblDest->setFont(DashboardTheme::BodyFont(8, false));
        lblDest->setStyleSheet("color: #9CA3AF; border: none;");
        moveRow->addWidget(lblDest);
        cardLayout->addLayout(moveRow);

        // Patient Vitals Box
        auto vitalsBox = new QFrame(card);
        vitalsBox->setStyleSheet("background-color: #030712; border: 1px solid #1F2937; border-radius: 4px;");
        auto vitalsLayout = new QHBoxLayout(vitalsBox);
        vitalsLayout->setContentsMargins(6, 4, 6, 4);

        auto lblHr = new QLabel("HR: --", vitalsBox);
        lblHr->setFont(DashboardTheme::MetricFont(8, false));
        lblHr->setStyleSheet("color: #EF4444; border: none;");

        auto lblSpo2 = new QLabel("SpO2: --", vitalsBox);
        lblSpo2->setFont(DashboardTheme::MetricFont(8, false));
        lblSpo2->setStyleSheet("color: #10B981; border: none;");

        auto lblBp = new QLabel("BP: --/--", vitalsBox);
        lblBp->setFont(DashboardTheme::MetricFont(8, false));
        lblBp->setStyleSheet("color: #60A5FA; border: none;");

        auto lblGcs = new QLabel("GCS: --", vitalsBox);
        lblGcs->setFont(DashboardTheme::MetricFont(8, false));
        lblGcs->setStyleSheet("color: #F59E0B; border: none;");

        vitalsLayout->addWidget(lblHr);
        vitalsLayout->addWidget(lblSpo2);
        vitalsLayout->addWidget(lblBp);
        vitalsLayout->addWidget(lblGcs);
        cardLayout->addWidget(vitalsBox);

        containerLayout->addWidget(card);

        AmbulanceCardWidgets w;
        w.cardFrame = card;
        w.lblVehicleId = lblId;
        w.lblTierBadge = lblTier;
        w.lblStatusBadge = lblStatus;
        w.lblSpeedHeading = lblSpeed;
        w.lblDestination = lblDest;
        w.lblEta = lblDest; // combined in row
        w.lblVitalsHr = lblHr;
        w.lblVitalsSpo2 = lblSpo2;
        w.lblVitalsBp = lblBp;
        w.lblVitalsGcs = lblGcs;

        m_ambulanceCards[id] = w;
    }

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

void AmbulanceStatusWidget::RefreshData() {
    if (!m_dataModel) return;

    auto fleet = m_dataModel->GetFleet();

    for (const auto& unit : fleet) {
        auto it = m_ambulanceCards.find(unit.vehicleId);
        if (it == m_ambulanceCards.end()) continue;

        auto& w = it->second;

        // Status badge
        w.lblStatusBadge->setText(QString::fromStdString(unit.status));
        if (unit.status == "AVAILABLE" || unit.status == "IDLE") {
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#064E3B"), QColor("#6EE7B7"));
        } else if (unit.status == "DISPATCHED" || unit.status == "EN_ROUTE_SCENE") {
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#1E3A8A"), QColor("#93C5FD"));
        } else if (unit.status == "ON_SCENE") {
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#78350F"), QColor("#FDE68A"));
        } else if (unit.status == "EN_ROUTE_HOSPITAL") {
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#064E3B"), QColor("#34D399"));
        } else {
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#374151"), QColor("#D1D5DB"));
        }

        // Movement
        w.lblSpeedHeading->setText(QString("%1 km/h • %2°")
            .arg(unit.speedKmh, 0, 'f', 1)
            .arg(static_cast<int>(unit.headingDeg)));

        if (!unit.assignedHospitalId.empty()) {
            w.lblDestination->setText(QString("Target: %1 • ETA: %2s")
                .arg(QString::fromStdString(unit.assignedHospitalId))
                .arg(unit.transitEtaSec, 0, 'f', 0));
        } else if (!unit.assignedIncidentId.empty()) {
            w.lblDestination->setText(QString("Incident: %1 • ETA: %2s")
                .arg(QString::fromStdString(unit.assignedIncidentId))
                .arg(unit.transitEtaSec, 0, 'f', 0));
        } else {
            w.lblDestination->setText("Standby at Base");
        }

        // Vitals
        if (unit.hasPatientOnboard) {
            w.lblVitalsHr->setText(QString("HR: %1 bpm").arg(unit.patientVitals.heartRateBpm));
            w.lblVitalsSpo2->setText(QString("SpO2: %1%").arg(unit.patientVitals.spo2Pct));
            w.lblVitalsBp->setText(QString("BP: %1/%2")
                .arg(unit.patientVitals.bloodPressureSystolic)
                .arg(unit.patientVitals.bloodPressureDiastolic));
            w.lblVitalsGcs->setText(QString("GCS: %1").arg(unit.patientVitals.gcsScore));
        } else {
            w.lblVitalsHr->setText("HR: --");
            w.lblVitalsSpo2->setText("SpO2: --");
            w.lblVitalsBp->setText("BP: --/--");
            w.lblVitalsGcs->setText("GCS: --");
        }
    }
}

} // namespace dashboard
} // namespace aegis

