/**
 * @file TrafficDashboardWindow.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Real-Time Traffic Intersection Operations Console Window Implementation.
 */

#include "TrafficDashboardWindow.h"
#include "dashboard/DashboardTheme.h"
#include "dashboard/DashboardIcons.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtCore/QDateTime>

namespace aegis {
namespace dashboard {
namespace traffic {

TrafficDashboardWindow::TrafficDashboardWindow(std::shared_ptr<TrafficDashboardController> controller,
                                               QWidget* parent)
    : QMainWindow(parent)
    , m_controller(std::move(controller))
{
    setWindowTitle("SMART AEGIS — Traffic Operations & Signal Monitoring Console (QNX RTOS / RPi4)");
    resize(1600, 980);
    setMinimumSize(1280, 720);

    DashboardTheme::ApplyTheme(this);

    SetupToolbar();
    SetupUi();
    SetupStatusBar();

    if (m_controller) {
        connect(m_controller.get(), &TrafficDashboardController::monitoringStateChanged,
                this, [this](bool running) {
                    m_btnToggleMonitoring->setText(running ? "⏹ STOP MONITORING" : "▶ START MONITORING");
                    if (running) {
                        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #7F1D1D; color: #FCA5A5; font-weight: bold; padding: 5px 12px; }");
                        m_lblStatusSummary->setText("STATUS: REAL-TIME MONITORING ACTIVE (20 Hz)");
                        m_lblStatusSummary->setStyleSheet("color: #10B981; font-weight: bold;");
                    } else {
                        m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; padding: 5px 12px; }");
                        m_lblStatusSummary->setText("STATUS: MONITORING PAUSED");
                        m_lblStatusSummary->setStyleSheet("color: #F59E0B; font-weight: bold;");
                    }
                });

        auto model = m_controller->GetDataModel();
        if (model) {
            connect(model.get(), &TrafficDashboardDataModel::dataModelUpdated,
                    this, &TrafficDashboardWindow::RefreshAllViews);
            connect(model.get(), &TrafficDashboardDataModel::notificationAdded,
                    this, &TrafficDashboardWindow::OnNotificationAlert);
        }
    }

    // UTC Clock Timer
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, [this]() {
        if (m_lblClock) {
            m_lblClock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss UTC"));
        }
    });
    m_clockTimer->start(1000);

    RefreshAllViews();
}

std::shared_ptr<TrafficDashboardDataModel> TrafficDashboardWindow::GetDataModel() const {
    return m_controller ? m_controller->GetDataModel() : nullptr;
}

void TrafficDashboardWindow::SetupToolbar() {
    auto toolbarCard = new QFrame(this);
    toolbarCard->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(toolbarCard);

    auto toolbarLayout = new QHBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(12, 8, 12, 8);
    toolbarLayout->setSpacing(12);

    // App Branding
    auto logoLayout = new QHBoxLayout();
    logoLayout->setSpacing(8);

    auto lblLogo = new QLabel(toolbarCard);
    lblLogo->setPixmap(DashboardIcons::TrafficLightIcon(24).pixmap(24, 24));
    logoLayout->addWidget(lblLogo);

    auto titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    auto lblAppTitle = new QLabel("SMART AEGIS TRAFFIC CONSOLE", toolbarCard);
    lblAppTitle->setFont(DashboardTheme::HeaderFont(12, true));
    lblAppTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; letter-spacing: 1px;");

    auto lblSubtitle = new QLabel("MUNICIPAL SIGNAL OPERATIONS & GREEN CORRIDOR MONITOR • QNX RTOS", toolbarCard);
    lblSubtitle->setFont(DashboardTheme::BodyFont(7, false));
    lblSubtitle->setStyleSheet("color: #9CA3AF;");

    titleLayout->addWidget(lblAppTitle);
    titleLayout->addWidget(lblSubtitle);
    logoLayout->addLayout(titleLayout);
    toolbarLayout->addLayout(logoLayout);

    toolbarLayout->addStretch();

    // Reset Camera
    m_btnResetCamera = new QPushButton("⌖ Reset Map Camera", toolbarCard);
    m_btnResetCamera->setFont(DashboardTheme::BodyFont(8, false));
    m_btnResetCamera->setStyleSheet("QPushButton { padding: 4px 10px; }");
    connect(m_btnResetCamera, &QPushButton::clicked, this, [this]() {
        if (m_mapPanel) m_mapPanel->ResetCamera();
    });
    toolbarLayout->addWidget(m_btnResetCamera);

    // Monitoring Toggle
    m_btnToggleMonitoring = new QPushButton("▶ START MONITORING", toolbarCard);
    m_btnToggleMonitoring->setFont(DashboardTheme::BodyFont(9, true));
    m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; padding: 5px 12px; }");
    connect(m_btnToggleMonitoring, &QPushButton::clicked, this, [this]() {
        if (m_controller) {
            if (m_controller->IsMonitoring()) m_controller->StopMonitoring();
            else m_controller->StartMonitoring();
        }
    });
    toolbarLayout->addWidget(m_btnToggleMonitoring);

    // Live UTC Clock
    m_lblClock = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss UTC"), toolbarCard);
    m_lblClock->setFont(DashboardTheme::MetricFont(9, false));
    m_lblClock->setStyleSheet("color: #D1D5DB; padding: 4px 10px; background-color: #111827; border-radius: 4px; border: 1px solid #374151;");
    toolbarLayout->addWidget(m_lblClock);

    // Add to main layout in SetupUi
}

void TrafficDashboardWindow::SetupStatusBar() {
    auto status = statusBar();
    status->setStyleSheet("QStatusBar { background-color: #0B0F19; color: #9CA3AF; border-top: 1px solid #1F2937; }");

    m_lblStatusSummary = new QLabel("STATUS: MONITORING PAUSED", this);
    m_lblStatusSummary->setFont(DashboardTheme::BodyFont(8, true));
    m_lblStatusSummary->setStyleSheet("color: #F59E0B;");
    status->addWidget(m_lblStatusSummary);

    auto lblClusterSummary = new QLabel(" | CLUSTER: 3 INTERSECTION RSUs (INT_A, INT_B, INT_C) | QNX SDP 8.0 READY", this);
    lblClusterSummary->setFont(DashboardTheme::BodyFont(8, false));
    lblClusterSummary->setStyleSheet("color: #6B7280;");
    status->addPermanentWidget(lblClusterSummary);
}

void TrafficDashboardWindow::SetupUi() {
    auto centralWidget = new QWidget(this);
    auto rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(10, 8, 10, 8);
    rootLayout->setSpacing(8);

    // 1. Top Header Toolbar Card
    auto toolbarCard = new QFrame(centralWidget);
    toolbarCard->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(toolbarCard);

    auto toolbarLayout = new QHBoxLayout(toolbarCard);
    toolbarLayout->setContentsMargins(12, 8, 12, 8);
    toolbarLayout->setSpacing(12);

    auto logoLayout = new QHBoxLayout();
    logoLayout->setSpacing(8);

    auto lblLogo = new QLabel(toolbarCard);
    lblLogo->setPixmap(DashboardIcons::TrafficLightIcon(24).pixmap(24, 24));
    logoLayout->addWidget(lblLogo);

    auto titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    auto lblAppTitle = new QLabel("SMART AEGIS TRAFFIC CONSOLE", toolbarCard);
    lblAppTitle->setFont(DashboardTheme::HeaderFont(12, true));
    lblAppTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; letter-spacing: 1px;");

    auto lblSubtitle = new QLabel("MUNICIPAL SIGNAL OPERATIONS & GREEN CORRIDOR MONITOR • QNX RTOS", toolbarCard);
    lblSubtitle->setFont(DashboardTheme::BodyFont(7, false));
    lblSubtitle->setStyleSheet("color: #9CA3AF;");

    titleLayout->addWidget(lblAppTitle);
    titleLayout->addWidget(lblSubtitle);
    logoLayout->addLayout(titleLayout);
    toolbarLayout->addLayout(logoLayout);

    toolbarLayout->addStretch();

    m_btnResetCamera = new QPushButton("⌖ Reset Map Camera", toolbarCard);
    m_btnResetCamera->setFont(DashboardTheme::BodyFont(8, false));
    m_btnResetCamera->setStyleSheet("QPushButton { padding: 4px 10px; }");
    connect(m_btnResetCamera, &QPushButton::clicked, this, [this]() {
        if (m_mapPanel) m_mapPanel->ResetCamera();
    });
    toolbarLayout->addWidget(m_btnResetCamera);

    m_btnToggleMonitoring = new QPushButton("▶ START MONITORING", toolbarCard);
    m_btnToggleMonitoring->setFont(DashboardTheme::BodyFont(9, true));
    m_btnToggleMonitoring->setStyleSheet("QPushButton { background-color: #064E3B; color: #6EE7B7; font-weight: bold; padding: 5px 12px; }");
    connect(m_btnToggleMonitoring, &QPushButton::clicked, this, [this]() {
        if (m_controller) {
            if (m_controller->IsMonitoring()) m_controller->StopMonitoring();
            else m_controller->StartMonitoring();
        }
    });
    toolbarLayout->addWidget(m_btnToggleMonitoring);

    m_lblClock = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss UTC"), toolbarCard);
    m_lblClock->setFont(DashboardTheme::MetricFont(9, false));
    m_lblClock->setStyleSheet("color: #D1D5DB; padding: 4px 10px; background-color: #111827; border-radius: 4px; border: 1px solid #374151;");
    toolbarLayout->addWidget(m_lblClock);

    rootLayout->addWidget(toolbarCard);

    // 2. Main Vertical Splitter (Work Area vs Event Logs)
    auto vSplitter = new QSplitter(Qt::Vertical, centralWidget);
    vSplitter->setHandleWidth(4);

    // 3. Main Horizontal Splitter (Map vs Tabs)
    auto hSplitter = new QSplitter(Qt::Horizontal, vSplitter);
    hSplitter->setHandleWidth(4);

    auto model = GetDataModel();

    // Left: 2D Vector Map Canvas
    m_mapPanel = new IntersectionMapPanel(model, TrafficDashboardConfig(), hSplitter);
    m_mapPanel->setMinimumWidth(620);
    hSplitter->addWidget(m_mapPanel);

    // Right: Tabbed Control Panels
    auto rightTabs = new QTabWidget(hSplitter);
    rightTabs->setMinimumWidth(440);

    // Tab 1: Signals & Queues
    auto tab1 = new QWidget();
    auto tab1Layout = new QVBoxLayout(tab1);
    tab1Layout->setContentsMargins(4, 4, 4, 4);
    tab1Layout->setSpacing(8);
    m_signalPanel = new TrafficSignalPanel(model, tab1);
    m_queuePanel = new QueueMonitorPanel(model, tab1);
    tab1Layout->addWidget(m_signalPanel);
    tab1Layout->addWidget(m_queuePanel);
    rightTabs->addTab(tab1, "SIGNALS & QUEUE");

    // Tab 2: Node Health & Hardware Sensors
    auto tab2 = new QWidget();
    auto tab2Layout = new QVBoxLayout(tab2);
    tab2Layout->setContentsMargins(4, 4, 4, 4);
    tab2Layout->setSpacing(8);
    m_healthPanel = new NodeHealthPanel(model, tab2);
    m_irPanel = new IrSensorPanel(model, tab2);
    tab2Layout->addWidget(m_healthPanel);
    tab2Layout->addWidget(m_irPanel);
    rightTabs->addTab(tab2, "NODE HEALTH & SENSORS");

    // Tab 3: Green Wave Corridor
    m_corridorPanel = new GreenCorridorPanel(model, rightTabs);
    rightTabs->addTab(m_corridorPanel, "GREEN CORRIDOR");

    // Tab 4: Manual Control
    m_controlPanel = new ManualControlPanel(m_controller, rightTabs);
    rightTabs->addTab(m_controlPanel, "MANUAL OVERRIDE");

    hSplitter->addWidget(rightTabs);
    hSplitter->setSizes({820, 580});
    vSplitter->addWidget(hSplitter);

    // Lower: Event & Audit Log Panel
    m_logPanel = new EventLogPanel(model, vSplitter);
    m_logPanel->setMinimumHeight(150);
    vSplitter->addWidget(m_logPanel);

    vSplitter->setSizes({680, 220});
    rootLayout->addWidget(vSplitter);

    setCentralWidget(centralWidget);

    // Floating Notification Panel
    m_notificationPanel = new TrafficNotificationPanel(this);
}

void TrafficDashboardWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        if (m_controller) {
            if (m_controller->IsMonitoring()) m_controller->StopMonitoring();
            else m_controller->StartMonitoring();
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_1) {
        if (m_controller) m_controller->SelectIntersection("INT_A");
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_2) {
        if (m_controller) m_controller->SelectIntersection("INT_B");
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_3) {
        if (m_controller) m_controller->SelectIntersection("INT_C");
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_G) {
        if (m_controller && GetDataModel()) {
            m_controller->EnableManualOverride(GetDataModel()->GetSelectedIntersectionId(), "FORCE_GREEN");
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_R) {
        if (m_controller && GetDataModel()) {
            m_controller->EnableManualOverride(GetDataModel()->GetSelectedIntersectionId(), "HOLD_RED");
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_A) {
        if (m_controller && GetDataModel()) {
            m_controller->DisableManualOverride(GetDataModel()->GetSelectedIntersectionId());
        }
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_P) {
        if (m_controller) {
            m_controller->TriggerEmergencyPreemptionTest("KEY_CORRIDOR_TEST");
        }
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) showNormal();
        else showFullScreen();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void TrafficDashboardWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_notificationPanel && m_notificationPanel->isVisible()) {
        int x = width() - m_notificationPanel->width() - 20;
        int y = 50;
        m_notificationPanel->move(x, y);
    }
}

void TrafficDashboardWindow::OnNotificationAlert(const QString& title, const QString& message, const QString& severity) {
    if (m_notificationPanel) {
        m_notificationPanel->ShowNotification(title, message, severity);
        int x = width() - m_notificationPanel->width() - 20;
        int y = 50;
        m_notificationPanel->move(x, y);
    }
}

void TrafficDashboardWindow::RefreshAllViews() {
    if (m_mapPanel) m_mapPanel->RefreshData();
    if (m_signalPanel) m_signalPanel->RefreshData();
    if (m_queuePanel) m_queuePanel->RefreshData();
    if (m_healthPanel) m_healthPanel->RefreshData();
    if (m_irPanel) m_irPanel->RefreshData();
    if (m_corridorPanel) m_corridorPanel->RefreshData();
    if (m_controlPanel) m_controlPanel->RefreshData();
    if (m_logPanel) m_logPanel->RefreshData();
}

} // namespace traffic
} // namespace dashboard
} // namespace aegis

