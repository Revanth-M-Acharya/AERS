/**
 * @file aegis_control_centre_dashboard.cpp
 * @brief SMART AEGIS (AERS) Phase 17 - Control Centre Application Executable Entrypoint.
 */

#include "dashboard/ControlCentreWindow.h"
#include "dashboard/DashboardController.h"
#include "dashboard/DashboardDataModel.h"
#include "dashboard/DashboardConfig.h"
#include "runtime/CoordinatorRuntime.h"
#include "deployment/DeploymentManager.h"

#include <QtWidgets/QApplication>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    // Check if offscreen flag is set via command line
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--headless" || std::string(argv[i]) == "--offscreen") {
            qputenv("QT_QPA_PLATFORM", "offscreen");
            break;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("SMART AEGIS Control Centre");
    app.setApplicationVersion("1.0.0-phase17");

    std::string configPath = "config/dashboard.json";
    bool launchFullscreen = false;
    bool enableBackendRuntime = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--config" || arg == "-c") && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg == "--fullscreen" || arg == "-f") {
            launchFullscreen = true;
        } else if (arg == "--standalone" || arg == "--mock-only") {
            enableBackendRuntime = false;
        }
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Real-Time Municipal Control Centre Dashboard" << std::endl;
    std::cout << " Target Platform : Raspberry Pi 4 Model B / QNX SDP 8.0" << std::endl;
    std::cout << " GUI Framework   : Qt6 Widgets & 2D Vector Canvas" << std::endl;
    std::cout << " Configuration   : " << configPath << std::endl;
    std::cout << "==========================================================" << std::endl;

    aegis::dashboard::DashboardConfig config;
    aegis::dashboard::DashboardConfig::LoadFromFile(configPath, config);

    auto dataModel = std::make_shared<aegis::dashboard::DashboardDataModel>();
    auto controller = std::make_shared<aegis::dashboard::DashboardController>(dataModel, config);

    std::shared_ptr<aegis::runtime::CoordinatorRuntime> coordinator = nullptr;
    if (enableBackendRuntime) {
        coordinator = std::make_shared<aegis::runtime::CoordinatorRuntime>();
        coordinator->Initialize();
        controller->SetCoordinatorRuntime(coordinator);
        std::cout << "[INIT] Attached to CoordinatorRuntime distributed engine." << std::endl;
    }

    aegis::dashboard::ControlCentreWindow window(controller);

    if (launchFullscreen || config.fullscreen) {
        window.showFullScreen();
    } else {
        window.show();
    }

    controller->Start();
    std::cout << "[READY] SMART AEGIS Control Centre Dashboard active." << std::endl;

    int execRet = app.exec();

    controller->Stop();
    if (coordinator && coordinator->IsRunning()) {
        coordinator->Stop();
    }

    return execRet;
}

