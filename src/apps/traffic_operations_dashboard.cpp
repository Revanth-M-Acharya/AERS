/**
 * @file traffic_operations_dashboard.cpp
 * @brief SMART AEGIS (AERS) Phase 18 - Traffic Intersection Operations Console Executable Entrypoint.
 */

#include "dashboard/traffic/TrafficDashboardWindow.h"
#include "dashboard/traffic/TrafficDashboardController.h"
#include "dashboard/traffic/TrafficDashboardDataModel.h"
#include "dashboard/traffic/TrafficDashboardConfig.h"
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
    app.setApplicationName("SMART AEGIS Traffic Intersection Dashboard");
    app.setApplicationVersion("1.0.0-phase18");

    std::string configPath = "config/traffic_dashboard.json";
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
    std::cout << " SMART AEGIS: Traffic Operations & Signal Monitoring Console" << std::endl;
    std::cout << " Target Platform : Raspberry Pi 4 Model B / QNX SDP 8.0" << std::endl;
    std::cout << " GUI Framework   : Qt6 Widgets & 2D Vector Canvas" << std::endl;
    std::cout << " Configuration   : " << configPath << std::endl;
    std::cout << "==========================================================" << std::endl;

    aegis::dashboard::traffic::TrafficDashboardConfig config;
    aegis::dashboard::traffic::TrafficDashboardConfig::LoadFromFile(configPath, config);

    auto dataModel = std::make_shared<aegis::dashboard::traffic::TrafficDashboardDataModel>();
    auto controller = std::make_shared<aegis::dashboard::traffic::TrafficDashboardController>(dataModel, config);

    std::shared_ptr<aegis::runtime::CoordinatorRuntime> coordinator = nullptr;
    if (enableBackendRuntime) {
        coordinator = std::make_shared<aegis::runtime::CoordinatorRuntime>();
        coordinator->Initialize();
        controller->SetCoordinatorRuntime(coordinator);
        std::cout << "[INIT] Attached to CoordinatorRuntime distributed engine." << std::endl;
    }

    aegis::dashboard::traffic::TrafficDashboardWindow window(controller);

    if (launchFullscreen || config.fullscreen) {
        window.showFullScreen();
    } else {
        window.show();
    }

    controller->StartMonitoring();
    std::cout << "[READY] SMART AEGIS Traffic Intersection Dashboard active." << std::endl;

    int execRet = app.exec();

    controller->StopMonitoring();
    if (coordinator && coordinator->IsRunning()) {
        coordinator->Stop();
    }

    return execRet;
}

