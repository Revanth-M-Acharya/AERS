/**
 * @file ManualControlPanel.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Operator Manual Control & Override Panel Implementation.
 */

#include "ManualControlPanel.h"
#include "dashboard/DashboardTheme.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFrame>

namespace aegis {
namespace dashboard {
namespace traffic {

ManualControlPanel::ManualControlPanel(std::shared_ptr<TrafficDashboardController> controller,
                                       QWidget* parent)
    : QWidget(parent)
    , m_controller(std::move(controller))
{
    SetupUi();
    RefreshData();

    if (m_controller) {
        connect(m_controller.get(), &TrafficDashboardController::monitoringStateChanged,
                this, [this](bool running) {
                    m_btnToggleMonitoring->setText(running ? "⏹ STOP MONITORING" : "▶ START MONITORING");
                    if (running) {
                        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #7F1D1D; color: #FCA5A5; font-weight: bold; }");
                    } else {
                        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; }");
                    }
                });
    }
}

void ManualControlPanel::SetupUi() {
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
    auto lblTitle = new QLabel("OPERATOR CONTROL & OVERRIDE", card);
    lblTitle->setFont(DashboardTheme::HeaderFont(10, true));
    lblTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; border: none;");
    headerLayout->addWidget(lblTitle);
    headerLayout->addStretch();

    m_btnToggleMonitoring = new QPushButton("▶ START MONITORING", card);
    m_btnToggleMonitoring->setFont(DashboardTheme::BodyFont(8, true));
    m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; padding: 4px 10px; }");
    connect(m_btnToggleMonitoring, &QPushButton::clicked, this, &ManualControlPanel::OnBtnMonitoringClicked);
    headerLayout->addWidget(m_btnToggleMonitoring);
    cardLayout->addLayout(headerLayout);

    // Target Selection Row
    auto selRow = new QHBoxLayout();
    auto lblTarget = new QLabel("Target Intersection:", card);
    lblTarget->setFont(DashboardTheme::BodyFont(8, true));
    lblTarget->setStyleSheet("color: #D1D5DB; border: none;");
    selRow->addWidget(lblTarget);

    m_cmbTargetNode = new QComboBox(card);
    m_cmbTargetNode->addItem("INT_A (Brigade Rd)", "INT_A");
    m_cmbTargetNode->addItem("INT_B (MG Rd)", "INT_B");
    m_cmbTargetNode->addItem("INT_C (Hospital Appr)", "INT_C");
    connect(m_cmbTargetNode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
                if (m_controller) {
                    m_controller->SelectIntersection(m_cmbTargetNode->itemData(index).toString().toStdString());
                }
            });
    selRow->addWidget(m_cmbTargetNode, 1);
    cardLayout->addLayout(selRow);

    // Action Grid
    auto grid = new QGridLayout();
    grid->setSpacing(6);

    // Row 0: Node Management
    m_btnResetIntersection = new QPushButton("↺ Reset Cycle", card);
    m_btnResetIntersection->setFont(DashboardTheme::BodyFont(8, false));
    connect(m_btnResetIntersection, &QPushButton::clicked, this, &ManualControlPanel::OnBtnResetIntersectionClicked);
    grid->addWidget(m_btnResetIntersection, 0, 0);

    m_btnRestartNode = new QPushButton("⚡ Restart Node", card);
    m_btnRestartNode->setFont(DashboardTheme::BodyFont(8, false));
    connect(m_btnRestartNode, &QPushButton::clicked, this, &ManualControlPanel::OnBtnRestartNodeClicked);
    grid->addWidget(m_btnRestartNode, 0, 1);

    // Row 1: Manual Overrides
    m_btnForceGreen = new QPushButton("Force Green", card);
    m_btnForceGreen->setFont(DashboardTheme::BodyFont(8, true));
    m_btnForceGreen->setStyleSheet("QPushButton { background-color: #064E3B; color: #34D399; }");
    connect(m_btnForceGreen, &QPushButton::clicked, this, &ManualControlPanel::OnBtnForceGreenClicked);
    grid->addWidget(m_btnForceGreen, 1, 0);

    m_btnHoldRed = new QPushButton("Hold Red", card);
    m_btnHoldRed->setFont(DashboardTheme::BodyFont(8, true));
    m_btnHoldRed->setStyleSheet("QPushButton { background-color: #7F1D1D; color: #FCA5A5; }");
    connect(m_btnHoldRed, &QPushButton::clicked, this, &ManualControlPanel::OnBtnHoldRedClicked);
    grid->addWidget(m_btnHoldRed, 1, 1);

    // Row 2: Release Override & Notifications
    m_btnReleaseOverride = new QPushButton("Release Auto", card);
    m_btnReleaseOverride->setFont(DashboardTheme::BodyFont(8, false));
    connect(m_btnReleaseOverride, &QPushButton::clicked, this, &ManualControlPanel::OnBtnReleaseOverrideClicked);
    grid->addWidget(m_btnReleaseOverride, 2, 0);

    m_btnClearNotifications = new QPushButton("Clear Alerts", card);
    m_btnClearNotifications->setFont(DashboardTheme::BodyFont(8, false));
    connect(m_btnClearNotifications, &QPushButton::clicked, this, &ManualControlPanel::OnBtnClearNotificationsClicked);
    grid->addWidget(m_btnClearNotifications, 2, 1);

    // Row 3: Preemption Testing
    m_btnPreemptionTest = new QPushButton("🚨 Test Preemption", card);
    m_btnPreemptionTest->setFont(DashboardTheme::BodyFont(8, true));
    m_btnPreemptionTest->setStyleSheet("QPushButton { background-color: #B91C1C; color: #FFFFFF; font-weight: bold; }");
    connect(m_btnPreemptionTest, &QPushButton::clicked, this, &ManualControlPanel::OnBtnPreemptionTestClicked);
    grid->addWidget(m_btnPreemptionTest, 3, 0);

    m_btnReleasePreemption = new QPushButton("Clear Preemption", card);
    m_btnReleasePreemption->setFont(DashboardTheme::BodyFont(8, false));
    connect(m_btnReleasePreemption, &QPushButton::clicked, this, &ManualControlPanel::OnBtnReleasePreemptionClicked);
    grid->addWidget(m_btnReleasePreemption, 3, 1);

    cardLayout->addLayout(grid);
    mainLayout->addWidget(card);
}

void ManualControlPanel::RefreshData() {
    if (!m_controller) return;

    bool isRunning = m_controller->IsMonitoring();
    m_btnToggleMonitoring->setText(isRunning ? "⏹ STOP MONITORING" : "▶ START MONITORING");
    if (isRunning) {
        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #7F1D1D; color: #FCA5A5; font-weight: bold; }");
    } else {
        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; }");
    }

    auto model = m_controller->GetDataModel();
    if (model) {
        std::string sel = model->GetSelectedIntersectionId();
        for (int i = 0; i < m_cmbTargetNode->count(); ++i) {
            if (m_cmbTargetNode->itemData(i).toString().toStdString() == sel) {
                if (m_cmbTargetNode->currentIndex() != i) {
                    m_cmbTargetNode->blockSignals(true);
                    m_cmbTargetNode->setCurrentIndex(i);
                    m_cmbTargetNode->blockSignals(false);
                }
                break;
            }
        }
    }
}

void ManualControlPanel::OnBtnMonitoringClicked() {
    if (!m_controller) return;
    if (m_controller->IsMonitoring()) {
        m_controller->StopMonitoring();
    } else {
        m_controller->StartMonitoring();
    }
}

void ManualControlPanel::OnBtnResetIntersectionClicked() {
    if (!m_controller) return;
    std::string nodeId = m_cmbTargetNode->currentData().toString().toStdString();
    m_controller->ResetIntersection(nodeId);
}

void ManualControlPanel::OnBtnRestartNodeClicked() {
    if (!m_controller) return;
    std::string nodeId = m_cmbTargetNode->currentData().toString().toStdString();
    m_controller->RestartNode(nodeId);
}

void ManualControlPanel::OnBtnForceGreenClicked() {
    if (!m_controller) return;
    std::string nodeId = m_cmbTargetNode->currentData().toString().toStdString();
    m_controller->EnableManualOverride(nodeId, "FORCE_GREEN");
}

void ManualControlPanel::OnBtnHoldRedClicked() {
    if (!m_controller) return;
    std::string nodeId = m_cmbTargetNode->currentData().toString().toStdString();
    m_controller->EnableManualOverride(nodeId, "HOLD_RED");
}

void ManualControlPanel::OnBtnReleaseOverrideClicked() {
    if (!m_controller) return;
    std::string nodeId = m_cmbTargetNode->currentData().toString().toStdString();
    m_controller->DisableManualOverride(nodeId);
}

void ManualControlPanel::OnBtnPreemptionTestClicked() {
    if (!m_controller) return;
    m_controller->TriggerEmergencyPreemptionTest("CORRIDOR_TEST_ALPHA");
}

void ManualControlPanel::OnBtnReleasePreemptionClicked() {
    if (!m_controller) return;
    m_controller->ReleaseEmergencyPreemption();
}

void ManualControlPanel::OnBtnClearNotificationsClicked() {
    if (!m_controller) return;
    m_controller->ClearNotifications();
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

