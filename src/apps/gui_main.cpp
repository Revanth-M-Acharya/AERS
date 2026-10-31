#if defined(HAS_QT6) || defined(QT_CORE_LIB) || defined(QT_WIDGETS_LIB)

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGroupBox>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>

#include <iostream>
#include <vector>
#include <string>

class SimulatorWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SimulatorWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("SMART AEGIS - Desktop Corridor Simulator");
        resize(1000, 700);
        SetupUi();
        ApplyDarkTheme();

        // Animation / telemetry timer
        timer_ = new QTimer(this);
        connect(timer_, &QTimer::timeout, this, &SimulatorWindow::OnTick);
    }

private slots:
    void OnStartClicked() {
        LogEvent("Simulation started - System in baseline standby.");
        statusLabel_->setText("Status: RUNNING (STANDBY)");
        statusLabel_->setStyleSheet("color: #A6E3A1; font-weight: bold;");
        isRunning_ = true;
    }

    void OnStopClicked() {
        LogEvent("Simulation stopped by operator.");
        statusLabel_->setText("Status: STOPPED");
        statusLabel_->setStyleSheet("color: #F38BA8; font-weight: bold;");
        timer_->stop();
        isRunning_ = false;
        ResetLeds();
    }

    void OnResetClicked() {
        LogEvent("System Reset - All corridor LEDs set to default OFF.");
        ResetLeds();
        progress_ = 0;
        progressBar_->setValue(0);
        etaLabel_->setText("ETA: --");
        routeLabel_->setText("Route: Standby");
        if (posLabel_) posLabel_->setText("Position: Station Alpha");
    }

    void OnEmergencyTriggerClicked() {
        LogEvent("EMERGENCY ALERT: Cardiac Intake Dispatched (Priority 1) -> Preempting Corridor.");
        statusLabel_->setText("Status: EMERGENCY ACTIVE");
        statusLabel_->setStyleSheet("color: #89B4FA; font-weight: bold;");
        routeLabel_->setText("Route: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL");
        progress_ = 0;
        stage_ = 1;
        timer_->start(1000);
    }

    void OnToggleTheme() {
        darkTheme_ = !darkTheme_;
        if (darkTheme_) {
            ApplyDarkTheme();
        } else {
            ApplyLightTheme();
        }
    }

    void OnTick() {
        if (!isRunning_ && stage_ == 0) return;

        progress_ += 16;
        if (progress_ > 100) progress_ = 100;
        progressBar_->setValue(progress_);

        if (progress_ < 33) {
            etaLabel_->setText("ETA: 32.0s");
            if (posLabel_) posLabel_->setText("Position: En route to Intersection 1");
            SetLed(node1Blue_, true);
            SetLed(node1Red_, false);
            SetLed(node2Blue_, false);
            SetLed(node3Blue_, false);
            if (stage_ == 1) {
                LogEvent("Intersection 1: Blue LED ON [Corridor Preemption]");
                stage_ = 2;
            }
        } else if (progress_ < 66) {
            etaLabel_->setText("ETA: 18.5s");
            if (posLabel_) posLabel_->setText("Position: Approaching Intersection 2");
            SetLed(node1Blue_, false);
            SetLed(node2Blue_, true);
            SetLed(node2Red_, false);
            SetLed(node3Blue_, false);
            if (stage_ == 2) {
                LogEvent("Intersection 1 Cleared -> Intersection 2: Blue LED ON");
                stage_ = 3;
            }
        } else if (progress_ < 100) {
            etaLabel_->setText("ETA: 6.0s");
            if (posLabel_) posLabel_->setText("Position: Approaching Intersection 3");
            SetLed(node1Blue_, false);
            SetLed(node2Blue_, false);
            SetLed(node3Blue_, true);
            if (stage_ == 3) {
                LogEvent("Intersection 2 Cleared -> Intersection 3: Blue LED ON");
                stage_ = 4;
            }
        } else {
            etaLabel_->setText("ETA: 0.0s (ARRIVED)");
            if (posLabel_) posLabel_->setText("Position: Hospital Trauma Center");
            ResetLeds();
            LogEvent("HOSPITAL REACHED: Emergency corridor safely released. All Blue LEDs OFF.");
            statusLabel_->setText("Status: MISSION CONCLUDED");
            statusLabel_->setStyleSheet("color: #A6E3A1; font-weight: bold;");
            timer_->stop();
            stage_ = 0;
        }
    }

private:
    void SetupUi() {
        auto central = new QWidget(this);
        setCentralWidget(central);
        auto mainLayout = new QVBoxLayout(central);

        // Header bar
        auto headerLayout = new QHBoxLayout();
        auto title = new QLabel("SMART AEGIS - Desktop Corridor Simulator", this);
        title->setStyleSheet("font-size: 18px; font-weight: bold;");
        statusLabel_ = new QLabel("Status: ONLINE (Standby)", this);
        statusLabel_->setStyleSheet("color: #A6E3A1; font-weight: bold;");
        themeBtn_ = new QPushButton("Toggle Theme", this);
        connect(themeBtn_, &QPushButton::clicked, this, &SimulatorWindow::OnToggleTheme);

        headerLayout->addWidget(title);
        headerLayout->addStretch();
        headerLayout->addWidget(statusLabel_);
        headerLayout->addWidget(themeBtn_);
        mainLayout->addLayout(headerLayout);

        // System Health / Status Bar
        auto sysHealthBox = new QFrame(this);
        sysHealthBox->setFrameShape(QFrame::StyledPanel);
        auto sysHealthLayout = new QHBoxLayout(sysHealthBox);
        coordStatusLabel_ = new QLabel("Coordinator: ONLINE (Pure Software)", this);
        coordStatusLabel_->setStyleSheet("color: #89B4FA; font-weight: bold;");
        hospStatusLabel_ = new QLabel("Hospital: TRAUMA BAY READY", this);
        hospStatusLabel_->setStyleSheet("color: #A6E3A1; font-weight: bold;");
        policeStatusLabel_ = new QLabel("Police: PATROL ACTIVE", this);
        policeStatusLabel_->setStyleSheet("color: #F9E2AF; font-weight: bold;");
        netStatusLabel_ = new QLabel("Network: MESH CONNECTED (RTT < 1ms)", this);
        netStatusLabel_->setStyleSheet("color: #A6E3A1; font-weight: bold;");

        sysHealthLayout->addWidget(coordStatusLabel_);
        sysHealthLayout->addStretch();
        sysHealthLayout->addWidget(hospStatusLabel_);
        sysHealthLayout->addStretch();
        sysHealthLayout->addWidget(policeStatusLabel_);
        sysHealthLayout->addStretch();
        sysHealthLayout->addWidget(netStatusLabel_);
        mainLayout->addWidget(sysHealthBox);

        // Middle Section (Nodes & Controls)
        auto midLayout = new QHBoxLayout();

        // Left Panel: Intersection Nodes Cards
        auto leftBox = new QGroupBox("Intersection Cluster Nodes", this);
        auto leftLayout = new QVBoxLayout(leftBox);

        // Node 1
        auto n1Box = new QFrame(this);
        n1Box->setFrameShape(QFrame::StyledPanel);
        auto n1Layout = new QHBoxLayout(n1Box);
        n1Layout->addWidget(new QLabel("Intersection 1 (10.204.179.245):"));
        node1Blue_ = CreateLedWidget("BLUE", "#89B4FA");
        node1Red_ = CreateLedWidget("RED", "#F38BA8");
        n1Layout->addWidget(node1Blue_);
        n1Layout->addWidget(node1Red_);
        leftLayout->addWidget(n1Box);

        // Node 2
        auto n2Box = new QFrame(this);
        n2Box->setFrameShape(QFrame::StyledPanel);
        auto n2Layout = new QHBoxLayout(n2Box);
        n2Layout->addWidget(new QLabel("Intersection 2 (10.204.179.130):"));
        node2Blue_ = CreateLedWidget("BLUE", "#89B4FA");
        node2Red_ = CreateLedWidget("RED", "#F38BA8");
        n2Layout->addWidget(node2Blue_);
        n2Layout->addWidget(node2Red_);
        leftLayout->addWidget(n2Box);

        // Node 3 (Shortage)
        auto n3Box = new QFrame(this);
        n3Box->setFrameShape(QFrame::StyledPanel);
        auto n3Layout = new QHBoxLayout(n3Box);
        n3Layout->addWidget(new QLabel("Intersection 3 (10.204.179.144):"));
        node3Blue_ = CreateLedWidget("BLUE", "#89B4FA");
        auto n3NoRed = new QLabel("Red: N/A (Shortage)", this);
        n3NoRed->setStyleSheet("color: #6C7086; font-size: 11px;");
        n3Layout->addWidget(node3Blue_);
        n3Layout->addWidget(n3NoRed);
        leftLayout->addWidget(n3Box);

        midLayout->addWidget(leftBox, 2);

        // Right Panel: Operational Controls
        auto rightBox = new QGroupBox("Simulation Controls", this);
        auto rightLayout = new QVBoxLayout(rightBox);

        auto btnEmergency = new QPushButton("🚨 Trigger Emergency Mission", this);
        btnEmergency->setStyleSheet("background-color: #F38BA8; color: #11111B; font-weight: bold; padding: 8px;");
        connect(btnEmergency, &QPushButton::clicked, this, &SimulatorWindow::OnEmergencyTriggerClicked);

        auto btnDemo = new QPushButton("▶ Run Demo", this);
        btnDemo->setStyleSheet("background-color: #89B4FA; color: #11111B; font-weight: bold; padding: 6px;");
        connect(btnDemo, &QPushButton::clicked, this, &SimulatorWindow::OnEmergencyTriggerClicked);

        auto btnStart = new QPushButton("Start Simulation", this);
        connect(btnStart, &QPushButton::clicked, this, &SimulatorWindow::OnStartClicked);

        auto btnReset = new QPushButton("Reset All Intersections", this);
        connect(btnReset, &QPushButton::clicked, this, &SimulatorWindow::OnResetClicked);

        auto btnStop = new QPushButton("Stop", this);
        connect(btnStop, &QPushButton::clicked, this, &SimulatorWindow::OnStopClicked);

        rightLayout->addWidget(btnEmergency);
        rightLayout->addWidget(btnDemo);
        rightLayout->addWidget(btnStart);
        rightLayout->addWidget(btnReset);
        rightLayout->addWidget(btnStop);
        rightLayout->addStretch();
        midLayout->addWidget(rightBox, 1);

        mainLayout->addLayout(midLayout);

        // Animation / Route progress bar
        auto routeBox = new QGroupBox("Ambulance Progression & Dynamic Route", this);
        auto rLayout = new QVBoxLayout(routeBox);
        routeLabel_ = new QLabel("Route: Standby", this);
        posLabel_ = new QLabel("Position: Station Alpha", this);
        etaLabel_ = new QLabel("ETA: --", this);
        progressBar_ = new QProgressBar(this);
        progressBar_->setRange(0, 100);
        progressBar_->setValue(0);
        progressBar_->setTextVisible(false);

        auto rInfoLayout = new QHBoxLayout();
        rInfoLayout->addWidget(routeLabel_);
        rInfoLayout->addStretch();
        rInfoLayout->addWidget(posLabel_);
        rInfoLayout->addStretch();
        rInfoLayout->addWidget(etaLabel_);

        rLayout->addLayout(rInfoLayout);
        rLayout->addWidget(progressBar_);
        mainLayout->addWidget(routeBox);

        // Log Panel
        auto logBox = new QGroupBox("Live Operations & Event Log", this);
        auto lLayout = new QVBoxLayout(logBox);
        logText_ = new QTextEdit(this);
        logText_->setReadOnly(true);
        lLayout->addWidget(logText_);
        mainLayout->addWidget(logBox);

        LogEvent("SMART AEGIS Simulator Initialized - Cross-platform simulation ready.");
    }

    QLabel* CreateLedWidget(const QString& name, const QString& colorHex) {
        auto lbl = new QLabel(name + ": OFF", this);
        lbl->setProperty("colorHex", colorHex);
        lbl->setStyleSheet("background-color: #313244; color: #A6ADC8; border-radius: 4px; padding: 4px 8px;");
        return lbl;
    }

    void SetLed(QLabel* lbl, bool on) {
        if (!lbl) return;
        QString hex = lbl->property("colorHex").toString();
        if (on) {
            lbl->setText(lbl->text().split(":")[0] + ": ON");
            lbl->setStyleSheet(QString("background-color: %1; color: #11111B; font-weight: bold; border-radius: 4px; padding: 4px 8px;").arg(hex));
        } else {
            lbl->setText(lbl->text().split(":")[0] + ": OFF");
            lbl->setStyleSheet("background-color: #313244; color: #A6ADC8; border-radius: 4px; padding: 4px 8px;");
        }
    }

    void ResetLeds() {
        SetLed(node1Blue_, false);
        SetLed(node1Red_, false);
        SetLed(node2Blue_, false);
        SetLed(node2Red_, false);
        SetLed(node3Blue_, false);
    }

    void LogEvent(const QString& msg) {
        QString ts = QDateTime::currentDateTime().toString("hh:mm:ss");
        logText_->append(QString("[%1] %2").arg(ts, msg));
    }

    void ApplyDarkTheme() {
        setStyleSheet(
            "QMainWindow { background-color: #1E1E2E; color: #CDD6F4; }"
            "QGroupBox { font-weight: bold; border: 1px solid #45475A; border-radius: 6px; margin-top: 10px; padding-top: 15px; color: #CDD6F4; }"
            "QPushButton { background-color: #313244; color: #CDD6F4; border: 1px solid #45475A; padding: 6px; border-radius: 4px; }"
            "QPushButton:hover { background-color: #45475A; }"
            "QTextEdit { background-color: #181825; color: #A6E3A1; border: 1px solid #313244; font-family: monospace; }"
            "QProgressBar { background-color: #313244; border-radius: 4px; height: 16px; }"
            "QProgressBar::chunk { background-color: #89B4FA; border-radius: 4px; }"
            "QLabel { color: #CDD6F4; }"
        );
    }

    void ApplyLightTheme() {
        setStyleSheet(
            "QMainWindow { background-color: #EFF1F5; color: #4C4F69; }"
            "QGroupBox { font-weight: bold; border: 1px solid #BCC0CC; border-radius: 6px; margin-top: 10px; padding-top: 15px; color: #4C4F69; }"
            "QPushButton { background-color: #E6E9EF; color: #4C4F69; border: 1px solid #BCC0CC; padding: 6px; border-radius: 4px; }"
            "QPushButton:hover { background-color: #CCD0DA; }"
            "QTextEdit { background-color: #FFFFFF; color: #40A02B; border: 1px solid #BCC0CC; font-family: monospace; }"
            "QProgressBar { background-color: #DCE0E8; border-radius: 4px; height: 16px; }"
            "QProgressBar::chunk { background-color: #1E66F5; border-radius: 4px; }"
            "QLabel { color: #4C4F69; }"
        );
    }

    QLabel* statusLabel_{nullptr};
    QLabel* coordStatusLabel_{nullptr};
    QLabel* hospStatusLabel_{nullptr};
    QLabel* policeStatusLabel_{nullptr};
    QLabel* netStatusLabel_{nullptr};
    QLabel* routeLabel_{nullptr};
    QLabel* posLabel_{nullptr};
    QLabel* etaLabel_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QTextEdit* logText_{nullptr};
    QPushButton* themeBtn_{nullptr};

    QLabel* node1Blue_{nullptr};
    QLabel* node1Red_{nullptr};
    QLabel* node2Blue_{nullptr};
    QLabel* node2Red_{nullptr};
    QLabel* node3Blue_{nullptr};

    QTimer* timer_{nullptr};
    bool isRunning_{false};
    bool darkTheme_{true};
    int progress_{0};
    int stage_{0};
};

#include "gui_main.moc"

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless" || std::string(argv[i]) == "--offscreen") {
            qputenv("QT_QPA_PLATFORM", "offscreen");
            break;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("SMART AEGIS Simulator");
    SimulatorWindow win;
    win.show();
    return app.exec();
}

#else

#include <iostream>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    std::cout << "[INFO] Qt6 GUI libraries were not enabled at build time.\n";
    std::cout << "[INFO] Running in headless console simulation mode.\n";
    std::cout << "[INFO] Run './simulation' or './coordinator --simulate' for the live console dashboard.\n";
    return 0;
}

#endif
