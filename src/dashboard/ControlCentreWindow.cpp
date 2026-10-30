/**
 * @file ControlCentreWindow.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Main Application Window Implementation.
 */

#include "ControlCentreWindow.h"
#include "DashboardTheme.h"
#include "DashboardIcons.h"

#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtGui/QKeyEvent>
#include <QtGui/QResizeEvent>
#include <QtCore/QDateTime>

namespace aegis {
namespace dashboard {

ControlCentreWindow::ControlCentreWindow(std::shared_ptr<DashboardController> controller,
                                         QWidget* parent)
    : QMainWindow(parent)
    , m_controller(std::move(controller))
{
    setWindowTitle("SMART AEGIS — Real-Time Municipal Emergency Operations Centre (QNX / RPi4)");
    resize(1600, 960);
    setMinimumSize(1280, 720);

    DashboardTheme::ApplyTheme(this);

    SetupUi();
    SetupShortcuts();

    // Connect controller signals
    if (m_controller) {
        connect(m_controller.get(), &DashboardController::alertRaised,
                this, &ControlCentreWindow::OnNotificationAlert);
        connect(m_controller.get(), &DashboardController::simulationPausedChanged,
                this, [this](bool paused) {
                    m_btnPause->setText(paused ? "▶ RESUME" : "⏸ PAUSE");
                });

        auto model = m_controller->GetDataModel();
        if (model) {
            connect(model.get(), &DashboardDataModel::dataModelUpdated,
                    this, &ControlCentreWindow::RefreshAllViews);
        }
    }

    // Live clock timer
    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, [this]() {
        if (m_lblClock) {
            m_lblClock->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss UTC"));
        }
    });
    m_clockTimer->start(1000);

    RefreshAllViews();
}

std::shared_ptr<DashboardDataModel> ControlCentreWindow::GetDataModel() const {
    return m_controller ? m_controller->GetDataModel() : nullptr;
}

void ControlCentreWindow::SetupUi() {
    auto centralWidget = new QWidget(this);
    auto rootLayout = new QVBoxLayout(centralWidget);
    rootLayout->setContentsMargins(10, 8, 10, 8);
    rootLayout->setSpacing(8);

    // 1. Top Bar Header & Controls
    SetupHeader();
    rootLayout->addWidget(m_cityStatus);

    // 2. Main Work Area Splitter (Vertical: Upper Work Area / Lower Event Logs)
    auto vSplitter = new QSplitter(Qt::Vertical, centralWidget);
    vSplitter->setHandleWidth(4);

    // 3. Horizontal Splitter (Left: Emergencies | Center: Map | Right: Telemetry Tabs)
    auto hSplitter = new QSplitter(Qt::Horizontal, vSplitter);
    hSplitter->setHandleWidth(4);

    auto model = GetDataModel();

    // Left: Active Emergencies & Pipeline
    m_emergencyTable = new EmergencyTableWidget(model, hSplitter);
    m_emergencyTable->setMinimumWidth(360);
    hSplitter->addWidget(m_emergencyTable);

    // Center: Vector Map Scene
    m_mapScene = new MapScene(model, DashboardConfig(), hSplitter);
    m_mapScene->setMinimumWidth(540);
    hSplitter->addWidget(m_mapScene);

    // Right: Tabbed Hardware & Municipal Metrics
    m_rightTabs = new QTabWidget(hSplitter);
    m_rightTabs->setMinimumWidth(360);

    // Tab 1: Cluster Health
    auto tabCluster = new QWidget();
    auto tabClusterLayout = new QVBoxLayout(tabCluster);
    tabClusterLayout->setContentsMargins(4, 4, 4, 4);
    tabClusterLayout->setSpacing(8);
    m_clusterStatus = new ClusterStatusWidget(model, tabCluster);
    m_nodeHealth = new NodeHealthWidget(model, tabCluster);
    tabClusterLayout->addWidget(m_clusterStatus);
    tabClusterLayout->addWidget(m_nodeHealth);
    m_rightTabs->addTab(tabCluster, "CLUSTER HEALTH");

    // Tab 2: Traffic Signals
    m_trafficStatus = new TrafficStatusWidget(model, m_rightTabs);
    m_rightTabs->addTab(m_trafficStatus, "TRAFFIC SIGNALS");

    // Tab 3: Fleet Units
    m_ambulanceStatus = new AmbulanceStatusWidget(model, m_rightTabs);
    m_rightTabs->addTab(m_ambulanceStatus, "FLEET TELEMETRY");

    // Tab 4: Hospitals
    m_hospitalStatus = new HospitalStatusWidget(model, m_rightTabs);
    m_rightTabs->addTab(m_hospitalStatus, "HOSPITALS");

    hSplitter->addWidget(m_rightTabs);

    // Proportions for horizontal splitter: 28% left, 46% center, 26% right
    hSplitter->setSizes({380, 720, 420});

    vSplitter->addWidget(hSplitter);

    // Bottom: Event & Audit Log
    m_eventLog = new EventLogWidget(model, vSplitter);
    m_eventLog->setMinimumHeight(140);
    vSplitter->addWidget(m_eventLog);

    // Proportions for vertical splitter: 75% upper, 25% lower
    vSplitter->setSizes({680, 220});

    rootLayout->addWidget(vSplitter);
    setCentralWidget(centralWidget);

    // Overlaid floating notification panel
    m_notificationPanel = new NotificationPanel(this);
}

void ControlCentreWindow::SetupHeader() {
    auto headerCard = new QFrame(this);
    headerCard->setObjectName("cardFrame");
    DashboardTheme::ApplyCardStyle(headerCard);

    auto headerLayout = new QHBoxLayout(headerCard);
    headerLayout->setContentsMargins(12, 8, 12, 8);
    headerLayout->setSpacing(12);

    // Logo & App Name
    auto logoLayout = new QHBoxLayout();
    logoLayout->setSpacing(8);

    auto lblLogo = new QLabel(headerCard);
    lblLogo->setPixmap(DashboardIcons::AmbulanceIcon(24).pixmap(24, 24));
    logoLayout->addWidget(lblLogo);

    auto titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    auto lblAppTitle = new QLabel("SMART AEGIS", headerCard);
    lblAppTitle->setFont(DashboardTheme::HeaderFont(12, true));
    lblAppTitle->setStyleSheet("color: #F9FAFB; font-weight: bold; letter-spacing: 1px;");

    auto lblSubtitle = new QLabel("AUTONOMOUS EMERGENCY CORRIDOR SYSTEM • QNX REAL-TIME EOC", headerCard);
    lblSubtitle->setFont(DashboardTheme::BodyFont(7, false));
    lblSubtitle->setStyleSheet("color: #9CA3AF;");

    titleLayout->addWidget(lblAppTitle);
    titleLayout->addWidget(lblSubtitle);
    logoLayout->addLayout(titleLayout);
    headerLayout->addLayout(logoLayout);

    headerLayout->addStretch();

    // Live UTC Clock
    m_lblClock = new QLabel(QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm:ss UTC"), headerCard);
    m_lblClock->setFont(DashboardTheme::MetricFont(10, false));
    m_lblClock->setStyleSheet("color: #D1D5DB; padding: 4px 10px; background-color: #111827; border-radius: 4px; border: 1px solid #374151;");
    headerLayout->addWidget(m_lblClock);

    // Controls: Pause / Play
    m_btnPause = new QPushButton("⏸ PAUSE", headerCard);
    m_btnPause->setFont(DashboardTheme::BodyFont(9, true));
    m_btnPause->setStyleSheet("QPushButton { padding: 5px 12px; }");
    connect(m_btnPause, &QPushButton::clicked, this, &ControlCentreWindow::OnTogglePauseClicked);
    headerLayout->addWidget(m_btnPause);

    // Controls: Speed Selector
    m_cmbSpeed = new QComboBox(headerCard);
    m_cmbSpeed->addItem("1.0x Real-Time", 1.0);
    m_cmbSpeed->addItem("2.0x Simulation", 2.0);
    m_cmbSpeed->addItem("5.0x Accelerated", 5.0);
    m_cmbSpeed->addItem("10.0x Warp", 10.0);
    connect(m_cmbSpeed, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ControlCentreWindow::OnSpeedChanged);
    headerLayout->addWidget(m_cmbSpeed);

    // Controls: Reset
    m_btnReset = new QPushButton("↺ RESET", headerCard);
    m_btnReset->setFont(DashboardTheme::BodyFont(9, true));
    m_btnReset->setStyleSheet("QPushButton { padding: 5px 12px; }");
    connect(m_btnReset, &QPushButton::clicked, this, &ControlCentreWindow::OnResetClicked);
    headerLayout->addWidget(m_btnReset);

    // Action: Trigger Emergency
    m_btnTriggerEmergency = new QPushButton("＋ DISPATCH EMERGENCY", headerCard);
    m_btnTriggerEmergency->setFont(DashboardTheme::BodyFont(9, true));
    m_btnTriggerEmergency->setStyleSheet(
        "QPushButton { background-color: #DC2626; color: #FFFFFF; font-weight: bold; padding: 6px 14px; border-radius: 6px; border: 1px solid #EF4444; }"
        "QPushButton:hover { background-color: #B91C1C; }"
        "QPushButton:pressed { background-color: #991B1B; }"
    );
    connect(m_btnTriggerEmergency, &QPushButton::clicked,
            this, &ControlCentreWindow::OnTriggerEmergencyClicked);
    headerLayout->addWidget(m_btnTriggerEmergency);

    // Create City Status Widget
    auto model = GetDataModel();
    m_cityStatus = new CityStatusWidget(model, this);

    // Add header card to window top
    auto layout = qobject_cast<QVBoxLayout*>(centralWidget() ? centralWidget()->layout() : nullptr);
    if (layout) {
        layout->addWidget(headerCard);
    }
}

void ControlCentreWindow::SetupShortcuts() {
    // Shortcuts are handled in keyPressEvent
}

void ControlCentreWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space) {
        OnTogglePauseClicked();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_1) {
        m_cmbSpeed->setCurrentIndex(0);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_2) {
        m_cmbSpeed->setCurrentIndex(1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_5) {
        m_cmbSpeed->setCurrentIndex(2);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_0) {
        m_cmbSpeed->setCurrentIndex(3);
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_E) {
        OnTriggerEmergencyClicked();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_F11) {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}

void ControlCentreWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (m_notificationPanel && m_notificationPanel->isVisible()) {
        int x = width() - m_notificationPanel->width() - 20;
        int y = 50;
        m_notificationPanel->move(x, y);
    }
}

void ControlCentreWindow::OnTogglePauseClicked() {
    if (m_controller) {
        m_controller->TogglePause();
    }
}

void ControlCentreWindow::OnSpeedChanged(int index) {
    if (m_controller) {
        double speed = m_cmbSpeed->itemData(index).toDouble();
        m_controller->SetSimulationSpeed(speed);
    }
}

void ControlCentreWindow::OnResetClicked() {
    if (m_controller) {
        m_controller->ResetSimulation();
    }
}

void ControlCentreWindow::OnTriggerEmergencyClicked() {
    if (!m_controller) return;

    static int callSequence = 101;
    std::string callId = "CALL_" + std::to_string(callSequence++);

    // Quick dispatch: Cardiac arrest at Brigade Road
    m_controller->TriggerQuickEmergency(
        callId,
        "Acute Cardiac Arrest with Ventricular Tachycardia",
        12.9716, 77.5946, 142, 86
    );
}

void ControlCentreWindow::OnNotificationAlert(const QString& title, const QString& message, const QString& severity) {
    if (m_notificationPanel) {
        m_notificationPanel->ShowNotification(title, message, severity);
        int x = width() - m_notificationPanel->width() - 20;
        int y = 50;
        m_notificationPanel->move(x, y);
    }
}

void ControlCentreWindow::RefreshAllViews() {
    if (m_cityStatus) m_cityStatus->RefreshData();
    if (m_emergencyTable) m_emergencyTable->RefreshData();
    if (m_clusterStatus) m_clusterStatus->RefreshData();
    if (m_nodeHealth) m_nodeHealth->RefreshData();
    if (m_trafficStatus) m_trafficStatus->RefreshData();
    if (m_ambulanceStatus) m_ambulanceStatus->RefreshData();
    if (m_hospitalStatus) m_hospitalStatus->RefreshData();
    if (m_eventLog) m_eventLog->RefreshData();
    if (m_mapScene) m_mapScene->UpdateScene();
}

} // namespace dashboard
} // namespace aegis
