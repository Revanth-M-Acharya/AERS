/**
 * @file HardwareController.cpp
 * @brief SMART AEGIS (AERS) Phase 20 - Master Physical Hardware Controller Implementation.
 */

#include "HardwareController.h"

namespace aegis {
namespace hardware {

HardwareController::HardwareController(std::string nodeId,
                                       const std::string& configFilePath)
    : m_nodeId(std::move(nodeId))
    , m_configFilePath(configFilePath)
    , m_logger(std::make_shared<HardwareEventLogger>())
    , m_hardware(std::make_shared<IntersectionHardware>(m_nodeId, m_logger))
    , m_wireless(std::make_shared<WirelessNodeManager>(m_nodeId, m_logger))
    , m_healthMonitor(std::make_shared<HardwareHealthMonitor>(m_nodeId, m_logger))
    , m_diagnostics(std::make_shared<HardwareDiagnostics>(m_nodeId, m_logger))
    , m_watchdog(std::make_shared<HardwareWatchdog>())
    , m_bootManager(std::make_shared<BootManager>())
{
    m_watchdog->SetFailsafeCallback([this](const std::string& reason) {
        TriggerFailSafe(reason);
    });
    m_bootManager->SetSafeModeHandler([this](const std::string& reason) {
        TriggerFailSafe(reason);
    });
    Initialize(configFilePath);
}

HardwareController::~HardwareController() {
    Stop();
}

bool HardwareController::Initialize(const std::string& configFilePath) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_configFilePath = configFilePath;

    m_config.LoadFromFile(m_configFilePath);

    // Initialize physical intersection hardware
    m_hardware->Initialize(m_config.GetSignalPins(),
                           m_config.GetRelays(),
                           m_config.GetIrSensors(),
                           m_config.GetTimings());

    // Initialize Wi-Fi Wireless Manager
    WifiNodeRecord nodeRec;
    if (m_config.GetNodeRecord(m_nodeId, nodeRec)) {
        m_wireless->ConfigureWirelessNode(nodeRec);
    } else {
        nodeRec.nodeId = m_nodeId;
        nodeRec.staticIp = "192.168.1.101";
        nodeRec.port = 9001;
        nodeRec.coordinatorIp = "192.168.1.100";
        nodeRec.coordinatorPort = 9000;
        m_wireless->ConfigureWirelessNode(nodeRec);
    }
    m_wireless->Connect();

    // Hook command callback for remote commands from Coordinator
    m_wireless->SetCommandCallback([this](const std::string& command, const std::string& payload) {
        if (command == "PREEMPTION") {
            TriggerEmergencyPreemption(payload.empty() ? "CORRIDOR_01" : payload, "ROAD_A");
        } else if (command == "FORCE_GREEN") {
            ForceGreen(payload.empty() ? "ROAD_A" : payload);
        } else if (command == "FORCE_RED") {
            ForceRed();
        } else if (command == "RESTORE") {
            RestoreNormalCycle();
        } else if (command == "FAILSAFE") {
            TriggerFailSafe(payload.empty() ? "REMOTE_COMMAND" : payload);
        }
    });

    // Attach to monitors
    m_healthMonitor->SetIntersectionHardware(m_hardware);
    m_healthMonitor->SetWirelessManager(m_wireless);

    m_diagnostics->SetIntersectionHardware(m_hardware);
    m_diagnostics->SetWirelessManager(m_wireless);

    m_logger->Info("HARDWARE_CONTROLLER", "HardwareController initialized for " + m_nodeId);
    return true;
}

void HardwareController::Start() {
    m_running.store(true);
    if (m_watchdog) {
        m_watchdog->Start();
    }
    m_logger->Info("HARDWARE_CONTROLLER", "HardwareController started for " + m_nodeId);
}

void HardwareController::Stop() {
    if (m_running.load()) {
        m_running.store(false);
        if (m_watchdog) {
            m_watchdog->Stop();
        }
        m_hardware->Shutdown();
        m_wireless->Disconnect();
        m_logger->Info("HARDWARE_CONTROLLER", "HardwareController stopped cleanly for " + m_nodeId);
    }
}

bool HardwareController::IsRunning() const {
    return m_running.load();
}

void HardwareController::Update(double dtSec) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_watchdog) {
        m_watchdog->KickNodeHeartbeat();
    }

    // If Wi-Fi link is interrupted, trigger fail-safe after timeout
    if (m_wireless->IsInterrupted() && !m_hardware->IsInFailSafe()) {
        m_hardware->TriggerFailSafe("WIFI_COMMUNICATION_LOSS");
    } else if (!m_wireless->IsInterrupted() && m_hardware->IsInFailSafe() &&
               m_hardware->GetFailSafeReason() == "WIFI_COMMUNICATION_LOSS") {
        m_hardware->RecoverFromFailSafe();
    }

    m_hardware->Update(dtSec);
    m_wireless->SendHeartbeat();
    m_healthMonitor->EvaluateHealth();
}

void HardwareController::SetNormalPhase(DualPhase phase) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->GetTrafficLightController()->SetPhase(phase);
}

DualPhase HardwareController::GetCurrentPhase() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_hardware->GetTrafficLightController()->GetCurrentPhase();
}

void HardwareController::TriggerEmergencyPreemption(const std::string& corridorId,
                                                    const std::string& direction) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->TriggerPreemption(corridorId, direction);
    m_wireless->SendStatusUpdate("PREEMPTION_ACTIVE:" + corridorId);
}

void HardwareController::ReleasePreemption() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->ReleasePreemption();
    m_wireless->SendStatusUpdate("PREEMPTION_RELEASED");
}

bool HardwareController::IsPreempted() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_hardware->IsPreempted();
}

void HardwareController::TriggerFailSafe(const std::string& reason) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->TriggerFailSafe(reason);
}

void HardwareController::RecoverFromFailSafe() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->RecoverFromFailSafe();
}

bool HardwareController::IsInFailSafe() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_hardware->IsInFailSafe();
}

void HardwareController::ForceGreen(const std::string& direction) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->ForceGreen(direction);
}

void HardwareController::ForceRed() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->ForceRed();
}

void HardwareController::RestoreNormalCycle() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->RestoreNormalCycle();
}

void HardwareController::SimulateVehicleArrival() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->InjectApproachDetection();
}

void HardwareController::SimulateVehicleDeparture() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_hardware->InjectDepartureDetection();
}

bool HardwareController::IsVehicleApproaching() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    bool detected = false;
    m_hardware->ReadApproachIrSensor(detected);
    return detected;
}

bool HardwareController::IsVehicleDeparting() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    bool detected = false;
    m_hardware->ReadDepartureIrSensor(detected);
    return detected;
}

void HardwareController::SimulateWifiDrop(bool dropped) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_wireless->SimulateWifiInterruption(dropped);
    if (!dropped) {
        m_wireless->TriggerAutoReconnect();
    }
}

bool HardwareController::IsWifiConnected() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_wireless->IsConnected();
}

bool HardwareController::RunDiagnostics() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_diagnostics->RunFullDiagnosticSuite();
    return m_diagnostics->AllTestsPassed();
}

std::string HardwareController::GetDiagnosticReport() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_diagnostics->GenerateDiagnosticReport();
}

std::string HardwareController::GetHealthReport() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_healthMonitor->GenerateHealthReport();
}

AggregateHardwareHealth HardwareController::GetHealth() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_healthMonitor->EvaluateHealth();
}

} // namespace hardware
} // namespace aegis
