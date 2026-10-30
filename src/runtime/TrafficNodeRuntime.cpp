/**
 * @file TrafficNodeRuntime.cpp
 * @brief SMART AEGIS (AERS) Phase 16C - Edge Traffic Node Distributed Runtime Implementation.
 */

#include "TrafficNodeRuntime.h"
#include <iostream>
#include <chrono>
#include <sstream>

namespace aegis {
namespace runtime {

TrafficNodeRuntime::TrafficNodeRuntime(std::string nodeId)
    : m_nodeId(std::move(nodeId))
    , m_tcpClient(m_nodeId)
    , m_signalNode(m_nodeId)
    , m_signalController()
    , m_hal()
    , m_qnxFramework()
    , m_bsp()
{
}

TrafficNodeRuntime::~TrafficNodeRuntime() {
    Stop();
}

bool TrafficNodeRuntime::Initialize(const std::string& nodeId, const std::string& configFilePath) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (m_isInitialized.load()) {
        return true;
    }

    m_nodeId = nodeId;
    m_tcpClient.SetNodeId(m_nodeId);

    // 1. Load configuration (Task 9)
    if (!configFilePath.empty()) {
        m_configManager.LoadProfile(configFilePath);
    }

    // 2. Initialize BCM2711 BSP & Reserve GPIO Pins (Task 3)
    if (!m_bsp.InitializeBoard()) {
        std::cerr << "[WARN] [TRAFFIC_NODE] BSP initialization returned false; continuing in simulated mode\n";
    }

    // Reserve physical pins for Approach 1 (North-South) and Approach 2 (East-West)
    m_bsp.ReserveGPIO(m_pinRedNS, m_nodeId + "_RED_NS");
    m_bsp.ReserveGPIO(m_pinYellowNS, m_nodeId + "_YEL_NS");
    m_bsp.ReserveGPIO(m_pinGreenNS, m_nodeId + "_GRN_NS");
    m_bsp.ReserveGPIO(m_pinRedEW, m_nodeId + "_RED_EW");
    m_bsp.ReserveGPIO(m_pinYellowEW, m_nodeId + "_YEL_EW");
    m_bsp.ReserveGPIO(m_pinGreenEW, m_nodeId + "_GRN_EW");
    m_bsp.ReserveGPIO(m_pinWatchdog, m_nodeId + "_WDT");

    // Configure pin directions to OUTPUT
    m_bsp.ConfigurePinMode(m_pinRedNS, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinYellowNS, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinGreenNS, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinRedEW, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinYellowEW, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinGreenEW, aegis::bsp::PinMode::OUTPUT);
    m_bsp.ConfigurePinMode(m_pinWatchdog, aegis::bsp::PinMode::OUTPUT);

    // Initial baseline safe state: North-South RED, East-West RED
    m_bsp.WriteGPIO(m_pinRedNS, aegis::bsp::PinState::HIGH);
    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinGreenNS, aegis::bsp::PinState::LOW);

    m_bsp.WriteGPIO(m_pinRedEW, aegis::bsp::PinState::HIGH);
    m_bsp.WriteGPIO(m_pinYellowEW, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinGreenEW, aegis::bsp::PinState::LOW);

    // 3. Initialize HAL and Driver Framework
    m_hal.RegisterDevice(m_nodeId + "_GPIO", aegis::hal::DeviceType::GPIO, "Edge GPIO Controller", "/dev/gpio");
    m_hal.InitializeDevice(m_nodeId + "_GPIO");
    m_hal.EnableDevice(m_nodeId + "_GPIO");

    m_qnxFramework.RegisterDriver(m_nodeId + "_DRV", "QNX BCM2711 Driver", aegis::qnx_driver::DriverType::GPIO, aegis::qnx_driver::DriverCapability::READ_WRITE, "/dev/gpio0");
    m_qnxFramework.InitializeDriver(m_nodeId + "_DRV");
    m_qnxFramework.StartDriver(m_nodeId + "_DRV");

    // 4. Initialize Safety Monitor & Watchdog
    m_safetyMonitor.Initialize(1000, 5000);
    m_watchdog.Initialize(5000);

    // 5. Configure Network Message Handler
    m_tcpClient.SetOnMessage([this](const aegis::network::Message& msg) {
        HandleNetworkMessage(msg);
    });

    m_tcpClient.SetOnConnectionChange([this](bool connected) {
        if (!connected && m_isRunning.load()) {
            DegradeToFailSafe();
        }
    });

    m_isInitialized.store(true);
    return true;
}

bool TrafficNodeRuntime::Start(const std::string& host, uint16_t port) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    if (!m_isInitialized.load()) {
        Initialize(m_nodeId);
    }
    if (m_isRunning.load()) {
        return true;
    }

    m_coordinatorHost = host;
    m_coordinatorPort = port;

    // Connect to Central Coordinator (sends REGISTER frame automatically)
    if (!m_tcpClient.Connect(m_coordinatorHost, m_coordinatorPort, 3000)) {
        std::cerr << "[WARN] [TRAFFIC_NODE] Initial connection to Coordinator at "
                  << m_coordinatorHost << ":" << m_coordinatorPort << " failed; auto-reconnect active.\n";
    }

    // Start 1 Hz Heartbeat generator (Task 5)
    m_tcpClient.StartHeartbeat(1000);
    m_tcpClient.SetAutoReconnect(true, 200);

    m_isRunning.store(true);

    // Launch Dedicated Edge Worker Threads (Task 6)
    m_trafficFsmThread = std::thread(&TrafficNodeRuntime::TrafficFsmWorkerLoop, this);
    m_gpioMonitorThread = std::thread(&TrafficNodeRuntime::GpioMonitorWorkerLoop, this);
    m_loggerThread = std::thread(&TrafficNodeRuntime::LoggerWorkerLoop, this);
    m_safetyThread = std::thread(&TrafficNodeRuntime::SafetyWorkerLoop, this);

    return true;
}

void TrafficNodeRuntime::Stop() {
    if (!m_isRunning.load()) {
        return;
    }

    m_isRunning.store(false);

    // Safe clearance transition prior to shutdown
    if (m_isPreempted.load()) {
        RestoreNormalCycle();
    }

    // Stop edge worker threads
    if (m_trafficFsmThread.joinable()) m_trafficFsmThread.join();
    if (m_gpioMonitorThread.joinable()) m_gpioMonitorThread.join();
    if (m_loggerThread.joinable()) m_loggerThread.join();
    if (m_safetyThread.joinable()) m_safetyThread.join();

    // Stop heartbeat & disconnect client
    m_tcpClient.StopHeartbeat();
    m_tcpClient.Disconnect();

    // Safe hardware pin reset & shutdown
    m_bsp.WriteGPIO(m_pinGreenNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinRedNS, aegis::bsp::PinState::HIGH);
    m_bsp.ShutdownBoard();

    m_qnxFramework.StopDriver(m_nodeId + "_DRV");
    m_qnxFramework.ShutdownDriver(m_nodeId + "_DRV");
    m_hal.DisableDevice(m_nodeId + "_GPIO");
    m_hal.ShutdownDevice(m_nodeId + "_GPIO");

    m_isInitialized.store(false);
}

bool TrafficNodeRuntime::IsRunning() const noexcept {
    return m_isRunning.load();
}

bool TrafficNodeRuntime::IsConnected() const noexcept {
    return m_tcpClient.IsConnected();
}

void TrafficNodeRuntime::HandleNetworkMessage(const aegis::network::Message& msg) {
    if (msg.GetType() == aegis::network::MessageType::COMMAND) {
        std::string commandStr = msg.GetPayload();
        ExecuteCommand(commandStr);
    }
}

bool TrafficNodeRuntime::ExecuteCommand(const std::string& commandStr) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    std::istringstream iss(commandStr);
    std::string opcode;
    iss >> opcode;

    std::string targetNode;
    iss >> targetNode;

    // Check if command is for this node or broadcast
    if (!targetNode.empty() && targetNode != m_nodeId && targetNode != "ALL") {
        return false; // Command addressed to different intersection
    }

    if (opcode == "PREEMPT_GREEN") {
        uint32_t duration = 30;
        iss >> duration;
        TriggerPreemption(duration);

        // Dispatch immediate ACK frame (Task 5)
        std::string ackPayload = "ACK_PREEMPTION_ACTIVE " + m_nodeId + " GREEN";
        aegis::network::Message ack(aegis::network::MessageType::ACK, m_nodeId, ackPayload);
        m_tcpClient.Send(ack);

        if (m_commandCallback) {
            m_commandCallback(commandStr, "GREEN");
        }
        return true;
    } else if (opcode == "RESTORE_CYCLE") {
        RestoreNormalCycle();

        std::string ackPayload = "ACK_RESTORED " + m_nodeId + " RED";
        aegis::network::Message ack(aegis::network::MessageType::ACK, m_nodeId, ackPayload);
        m_tcpClient.Send(ack);

        if (m_commandCallback) {
            m_commandCallback(commandStr, "RED");
        }
        return true;
    } else if (opcode == "FLASHING_YELLOW") {
        DegradeToFailSafe();

        std::string ackPayload = "ACK_FAILSAFE " + m_nodeId + " FLASHING_YELLOW";
        aegis::network::Message ack(aegis::network::MessageType::ACK, m_nodeId, ackPayload);
        m_tcpClient.Send(ack);

        if (m_commandCallback) {
            m_commandCallback(commandStr, "FLASHING_YELLOW");
        }
        return true;
    }

    return false;
}

bool TrafficNodeRuntime::TriggerPreemption(uint32_t durationSec) {
    m_isPreempted.store(true);
    m_preemptionRemainingSec.store(durationSec);

    // 1. Actuate Signal FSM
    m_signalNode.SetSignalState(aegis::traffic_signal::SignalState::GREEN, "PREEMPTION_LOCK");
    m_signalController.SetGreen();

    // 2. Drive Physical / Simulated BCM2711 GPIO Pins (Task 3)
    // North-South turns Green
    m_bsp.WriteGPIO(m_pinRedNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinGreenNS, aegis::bsp::PinState::HIGH);

    // East-West turns Red
    m_bsp.WriteGPIO(m_pinGreenEW, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowEW, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinRedEW, aegis::bsp::PinState::HIGH);

    m_eventLogger.Log(aegis::logger::EventType::TRAFFIC_COMMAND,
                       aegis::logger::EventSeverity::INFO,
                       "Node " + m_nodeId + " preempted GREEN for " + std::to_string(durationSec) + "s");
    return true;
}

bool TrafficNodeRuntime::RestoreNormalCycle() {
    m_isPreempted.store(false);
    m_preemptionRemainingSec.store(0);

    // Safe clearance transition: Yellow clearance -> Red
    m_bsp.WriteGPIO(m_pinGreenNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::HIGH);

    // Transition to Red
    m_signalNode.SetSignalState(aegis::traffic_signal::SignalState::RED, "CYCLE_RESTORED");
    m_signalController.SetRed();

    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinRedNS, aegis::bsp::PinState::HIGH);

    m_eventLogger.Log(aegis::logger::EventType::TRAFFIC_COMMAND,
                       aegis::logger::EventSeverity::INFO,
                       "Node " + m_nodeId + " preemption cleared, restored cyclic operation");
    return true;
}

void TrafficNodeRuntime::DegradeToFailSafe() {
    m_isPreempted.store(false);
    m_preemptionRemainingSec.store(0);

    m_signalNode.SetSignalState(aegis::traffic_signal::SignalState::FLASHING_YELLOW, "COMMUNICATION_LOSS");

    // All greens and reds off; yellow active
    m_bsp.WriteGPIO(m_pinGreenNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinRedNS, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowNS, aegis::bsp::PinState::HIGH);

    m_bsp.WriteGPIO(m_pinGreenEW, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinRedEW, aegis::bsp::PinState::LOW);
    m_bsp.WriteGPIO(m_pinYellowEW, aegis::bsp::PinState::HIGH);
}

std::string TrafficNodeRuntime::GetSignalStateString() const {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    return m_signalNode.GetCurrentStateString();
}

aegis::bsp::PinState TrafficNodeRuntime::GetPinState(uint8_t pin) const {
    aegis::bsp::PinState state = aegis::bsp::PinState::LOW;
    const_cast<aegis::bsp::RaspberryPi4BSP&>(m_bsp).ReadGPIO(pin, state);
    return state;
}

void TrafficNodeRuntime::SetOnCommandExecuted(CommandExecutedCallback callback) {
    std::lock_guard<std::mutex> lock(m_stateMutex);
    m_commandCallback = std::move(callback);
}

// =============================================================================
// Worker Thread Loops (Task 6)
// =============================================================================

void TrafficNodeRuntime::TrafficFsmWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 Hz
        if (m_isPreempted.load()) {
            uint32_t rem = m_preemptionRemainingSec.load();
            if (rem > 0) {
                // Countdown preemption hold
            }
        }
    }
}

void TrafficNodeRuntime::GpioMonitorWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        // Toggle watchdog pin 26 as heartbeat pulse
        m_bsp.ToggleGPIO(m_pinWatchdog);
    }
}

void TrafficNodeRuntime::LoggerWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

void TrafficNodeRuntime::SafetyWorkerLoop() {
    while (m_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        m_safetyMonitor.Pulse();
        m_watchdog.Kick();
    }
}

} // namespace runtime
} // namespace aegis
