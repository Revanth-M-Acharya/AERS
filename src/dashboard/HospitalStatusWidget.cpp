/**
 * @file HospitalStatusWidget.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Municipal Hospital Capacity & Resource Monitor Widget Implementation.
 */

#include "HospitalStatusWidget.h"
#include "DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>

namespace aegis {
namespace dashboard {

HospitalStatusWidget::HospitalStatusWidget(std::shared_ptr<DashboardDataModel> dataModel, QWidget* parent)
    : QWidget(parent)
    , m_dataModel(std::move(dataModel))
{
    SetupUi();
    RefreshData();
}

void HospitalStatusWidget::SetupUi() {
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

    const std::vector<std::tuple<std::string, std::string, std::string, std::string>> initialHospitals = {
        {"HOSP_CITY_TRAUMA", "City Trauma Center", "LEVEL 1 TRAUMA", "Trauma Resuscitation • Neurosurgery • ECMO"},
        {"HOSP_METRO_GENERAL", "Metro General Hospital", "LEVEL 2 TRAUMA", "Emergency Surgery • Acute Cardiac Care"},
        {"HOSP_ST_JUDE", "St. Jude Heart Institute", "CARDIAC SPECIALTY", "24/7 Cath Lab • Stroke Interventions"}
    };

    for (const auto& [id, name, trauma, specs] : initialHospitals) {
        auto card = new QFrame(container);
        card->setObjectName("cardFrame");
        DashboardTheme::ApplyCardStyle(card);

        auto cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(10, 8, 10, 8);
        cardLayout->setSpacing(6);

        // Header
        auto topRow = new QHBoxLayout();
        auto lblName = new QLabel(QString::fromStdString(name), card);
        lblName->setFont(DashboardTheme::HeaderFont(10, true));
        lblName->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
        topRow->addWidget(lblName);
        topRow->addStretch();

        auto lblTrauma = new QLabel(QString::fromStdString(trauma), card);
        lblTrauma->setFont(DashboardTheme::BadgeFont(7, true));
        DashboardTheme::ApplyBadgeStyle(lblTrauma, QColor("#1E3A8A"), QColor("#93C5FD"));
        topRow->addWidget(lblTrauma);

        auto lblStatus = new QLabel("ACCEPTING", card);
        lblStatus->setFont(DashboardTheme::BadgeFont(7, true));
        DashboardTheme::ApplyBadgeStyle(lblStatus, QColor("#064E3B"), QColor("#6EE7B7"));
        topRow->addWidget(lblStatus);
        cardLayout->addLayout(topRow);

        // Specialties
        auto lblSpecialty = new QLabel(QString::fromStdString(specs), card);
        lblSpecialty->setFont(DashboardTheme::BodyFont(8, false));
        lblSpecialty->setStyleSheet("color: #9CA3AF; border: none;");
        cardLayout->addWidget(lblSpecialty);

        // ICU Bar
        auto icuRow = new QHBoxLayout();
        auto lblIcuTitle = new QLabel("ICU BEDS:", card);
        lblIcuTitle->setFont(DashboardTheme::BodyFont(8, true));
        lblIcuTitle->setStyleSheet("color: #D1D5DB; border: none;");
        lblIcuTitle->setFixedWidth(65);

        auto progressIcu = new QProgressBar(card);
        progressIcu->setFixedHeight(8);
        progressIcu->setTextVisible(false);
        progressIcu->setRange(0, 100);
        progressIcu->setValue(75);
        DashboardTheme::ApplyProgressBarStyle(progressIcu, QColor("#10B981"));

        auto lblIcuText = new QLabel("6/8 avail", card);
        lblIcuText->setFont(DashboardTheme::MetricFont(8, false));
        lblIcuText->setStyleSheet("color: #6EE7B7; border: none;");
        lblIcuText->setFixedWidth(60);

        icuRow->addWidget(lblIcuTitle);
        icuRow->addWidget(progressIcu);
        icuRow->addWidget(lblIcuText);
        cardLayout->addLayout(icuRow);

        // ED Bar
        auto edRow = new QHBoxLayout();
        auto lblEdTitle = new QLabel("ED BEDS:", card);
        lblEdTitle->setFont(DashboardTheme::BodyFont(8, true));
        lblEdTitle->setStyleSheet("color: #D1D5DB; border: none;");
        lblEdTitle->setFixedWidth(65);

        auto progressEd = new QProgressBar(card);
        progressEd->setFixedHeight(8);
        progressEd->setTextVisible(false);
        progressEd->setRange(0, 100);
        progressEd->setValue(60);
        DashboardTheme::ApplyProgressBarStyle(progressEd, QColor("#3B82F6"));

        auto lblEdText = new QLabel("12/20 avail", card);
        lblEdText->setFont(DashboardTheme::MetricFont(8, false));
        lblEdText->setStyleSheet("color: #93C5FD; border: none;");
        lblEdText->setFixedWidth(60);

        edRow->addWidget(lblEdTitle);
        edRow->addWidget(progressEd);
        edRow->addWidget(lblEdText);
        cardLayout->addLayout(edRow);

        containerLayout->addWidget(card);

        HospitalCardWidgets w;
        w.cardFrame = card;
        w.lblName = lblName;
        w.lblTraumaLevel = lblTrauma;
        w.lblStatusBadge = lblStatus;
        w.progressIcu = progressIcu;
        w.lblIcuText = lblIcuText;
        w.progressEd = progressEd;
        w.lblEdText = lblEdText;
        w.lblSpecialty = lblSpecialty;

        m_hospitalCards[id] = w;
    }

    containerLayout->addStretch();
    scrollArea->setWidget(container);
    mainLayout->addWidget(scrollArea);
}

void HospitalStatusWidget::RefreshData() {
    if (!m_dataModel) return;

    auto hospitals = m_dataModel->GetHospitals();

    for (const auto& hosp : hospitals) {
        auto it = m_hospitalCards.find(hosp.hospitalId);
        if (it == m_hospitalCards.end()) continue;

        auto& w = it->second;

        // Divert status
        if (hosp.isDiverting) {
            w.lblStatusBadge->setText("DIVERT");
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#7F1D1D"), QColor("#FCA5A5"));
        } else if (hosp.availableIcuBeds == 0) {
            w.lblStatusBadge->setText("ICU FULL");
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#78350F"), QColor("#FDE68A"));
        } else {
            w.lblStatusBadge->setText("ACCEPTING");
            DashboardTheme::ApplyBadgeStyle(w.lblStatusBadge, QColor("#064E3B"), QColor("#6EE7B7"));
        }

        // ICU
        int icuPct = hosp.totalIcuBeds > 0 ? static_cast<int>((hosp.availableIcuBeds * 100) / hosp.totalIcuBeds) : 0;
        w.progressIcu->setValue(icuPct);
        w.lblIcuText->setText(QString("%1/%2 avail").arg(hosp.availableIcuBeds).arg(hosp.totalIcuBeds));

        // ED
        int edPct = hosp.totalEdBeds > 0 ? static_cast<int>((hosp.availableEdBeds * 100) / hosp.totalEdBeds) : 0;
        w.progressEd->setValue(edPct);
        w.lblEdText->setText(QString("%1/%2 avail").arg(hosp.availableEdBeds).arg(hosp.totalEdBeds));
    }
}

} // namespace dashboard
} // namespace aegis
