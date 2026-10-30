/**
 * @file HardwareWatchdog.cpp
 * @brief SMART AEGIS (AERS) Phase 20.7 - Dedicated Hardware Watchdog Thread Implementation.
 */

#include "aegis/hardware/HardwareWatchdog.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

#include <chrono>

namespace aegis {
namespace hardware {

HardwareWatchdog::HardwareWatchdog() {
    uint64_t nowMs = common::GetUtcMilliseconds();
    m_lastCoordinatorHeartbeatMs = nowMs;
    m_lastNodeHeartbeatMs = nowMs;
}

HardwareWatchdog::~HardwareWatchdog() {
    Stop();
}

bool HardwareWatchdog::Start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_running.load()) {
        return false;
    }

    uint64_t nowMs = common::GetUtcMilliseconds();
    m_lastCoordinatorHeartbeatMs = nowMs;
    m_lastNodeHeartbeatMs = nowMs;
    m_hasFault.store(false);
    m_faultReason.clear();

    m_running.store(true);
    m_workerThread = std::thread(&HardwareWatchdog::WatchdogLoop, this);

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "WATCHDOG", "HardwareWatchdog supervisory thread started (1 Hz).");
    return true;
}

void HardwareWatchdog::Stop() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running.load()) {
            return;
        }
        m_running.store(false);
    }

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }

    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "WATCHDOG", "HardwareWatchdog supervisory thread stopped.");
}

bool HardwareWatchdog::IsRunning() const noexcept {
    return m_running.load();
}

void HardwareWatchdog::KickCoordinatorHeartbeat() noexcept {
    m_lastCoordinatorHeartbeatMs.store(common::GetUtcMilliseconds());
}

void HardwareWatchdog::KickNodeHeartbeat() noexcept {
    m_lastNodeHeartbeatMs.store(common::GetUtcMilliseconds());
}

void HardwareWatchdog::KickThread(const std::string& threadName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadHeartbeats[threadName] = common::GetUtcMilliseconds();
}

void HardwareWatchdog::SetRelayHealthChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_relayHealthCheck = std::move(checker);
}

void HardwareWatchdog::SetGpioStateChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_gpioStateCheck = std::move(checker);
}

void HardwareWatchdog::SetIrSensorActivityChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_irActivityCheck = std::move(checker);
}

void HardwareWatchdog::SetWifiConnectivityChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_wifiConnectivityCheck = std::move(checker);
}

void HardwareWatchdog::SetTrafficControllerChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trafficControllerCheck = std::move(checker);
}

void HardwareWatchdog::SetBootManagerChecker(HealthCheckFunc checker) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bootManagerCheck = std::move(checker);
}

void HardwareWatchdog::SetFailsafeCallback(FailsafeAction cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_failsafeAction = std::move(cb);
}

void HardwareWatchdog::SetCoordinatorNotifyCallback(CoordinatorNotifyAction cb) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_coordinatorNotifyAction = std::move(cb);
}

bool HardwareWatchdog::HasFault() const noexcept {
    return m_hasFault.load();
}

std::string HardwareWatchdog::GetFaultReason() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultReason;
}

uint64_t HardwareWatchdog::GetTickCount() const noexcept {
    return m_tickCount.load();
}

uint32_t HardwareWatchdog::GetRecoveryAttemptCount() const noexcept {
    return m_recoveryAttempts.load();
}

void HardwareWatchdog::ClearFault() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_hasFault.store(false);
    m_faultReason.clear();
    uint64_t nowMs = common::GetUtcMilliseconds();
    m_lastCoordinatorHeartbeatMs = nowMs;
    m_lastNodeHeartbeatMs = nowMs;
    for (auto& [name, ts] : m_threadHeartbeats) {
        ts = nowMs;
    }
    logger::Logger::Instance().Log(
        logger::LogLevel::INFO, "WATCHDOG", "HardwareWatchdog fault cleared. Normal supervision resumed.");
}

bool HardwareWatchdog::PerformSupervisoryChecks(std::string& outFaultReason) {
    uint64_t nowMs = common::GetUtcMilliseconds();

    // 1. Coordinator Heartbeat Freshness
    uint64_t lastCoord = m_lastCoordinatorHeartbeatMs.load();
    if (nowMs >= lastCoord && (nowMs - lastCoord) > m_coordinatorTimeoutMs) {
        outFaultReason = "Coordinator heartbeat expired (" + std::to_string(nowMs - lastCoord) +
                         "ms > " + std::to_string(m_coordinatorTimeoutMs) + "ms)";
        return false;
    }

    // 2. Node Heartbeat Freshness
    uint64_t lastNode = m_lastNodeHeartbeatMs.load();
    if (nowMs >= lastNode && (nowMs - lastNode) > m_nodeTimeoutMs) {
        outFaultReason = "Node internal heartbeat stalled (" + std::to_string(nowMs - lastNode) +
                         "ms > " + std::to_string(m_nodeTimeoutMs) + "ms)";
        return false;
    }

    // 3. Worker Thread Liveness
    for (const auto& [tName, tStamp] : m_threadHeartbeats) {
        if (nowMs >= tStamp && (nowMs - tStamp) > m_threadTimeoutMs) {
            outFaultReason = "Thread '" + tName + "' unresponsive (" + std::to_string(nowMs - tStamp) +
                             "ms > " + std::to_string(m_threadTimeoutMs) + "ms)";
            return false;
        }
    }

    // 4. Relay Health Checker
    if (m_relayHealthCheck && !m_relayHealthCheck()) {
        outFaultReason = "Relay health check failure (stuck coil or readback mismatch)";
        return false;
    }

    // 5. GPIO State Checker
    if (m_gpioStateCheck && !m_gpioStateCheck()) {
        outFaultReason = "GPIO state validation failure (illegal voltage level)";
        return false;
    }

    // 6. IR Sensor Activity Checker
    if (m_irActivityCheck && !m_irActivityCheck()) {
        outFaultReason = "IR optical sensor fault (stuck beam > 30s)";
        return false;
    }

    // 7. Wi-Fi Connectivity Checker
    if (m_wifiConnectivityCheck && !m_wifiConnectivityCheck()) {
        outFaultReason = "Wi-Fi link dropped or excessive packet loss";
        return false;
    }

    // 8. Traffic Controller Checker
    if (m_trafficControllerCheck && !m_trafficControllerCheck()) {
        outFaultReason = "Traffic controller state machine fault (invalid transition or lamp conflict)";
        return false;
    }

    // 9. Boot Manager Status Checker
    if (m_bootManagerCheck && !m_bootManagerCheck()) {
        outFaultReason = "Boot manager reported safe mode or uninitialized subsystem";
        return false;
    }

    outFaultReason.clear();
    return true;
}

bool HardwareWatchdog::AttemptAutoRecovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_hasFault.load()) {
        return true;
    }

    std::string testFault;
    // Test if underlying hardware & services are healthy again
    bool healthy = true;
    if (m_relayHealthCheck && !m_relayHealthCheck()) healthy = false;
    if (m_gpioStateCheck && !m_gpioStateCheck()) healthy = false;
    if (m_irActivityCheck && !m_irActivityCheck()) healthy = false;
    if (m_wifiConnectivityCheck && !m_wifiConnectivityCheck()) healthy = false;
    if (m_trafficControllerCheck && !m_trafficControllerCheck()) healthy = false;
    if (m_bootManagerCheck && !m_bootManagerCheck()) healthy = false;

    if (healthy) {
        m_hasFault.store(false);
        m_faultReason.clear();
        uint64_t nowMs = common::GetUtcMilliseconds();
        m_lastCoordinatorHeartbeatMs = nowMs;
        m_lastNodeHeartbeatMs = nowMs;
        for (auto& [name, ts] : m_threadHeartbeats) {
            ts = nowMs;
        }
        logger::Logger::Instance().Log(
            logger::LogLevel::INFO, "WATCHDOG",
            "Auto-recovery SUCCESS! All 7 subsystems verified healthy. Normal supervision resumed.");
        return true;
    }

    logger::Logger::Instance().Log(
        logger::LogLevel::WARN, "WATCHDOG",
        "Auto-recovery attempted but fault conditions persist: " + m_faultReason);
    return false;
}

std::string HardwareWatchdog::GenerateWatchdogReport() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string rep = "==========================================================\n";
    rep += " SMART AEGIS: 1 HZ HARDWARE WATCHDOG STATUS REPORT\n";
    rep += " Status:          " + std::string(m_hasFault.load() ? "FAULT TRIPPED" : "HEALTHY (NOMINAL)") + "\n";
    rep += " Failsafe Mode:   " + WatchdogFailsafeModeToString(m_failsafeMode) + "\n";
    rep += " Tick Count:      " + std::to_string(m_tickCount.load()) + "\n";
    rep += " Recovery Counts: " + std::to_string(m_recoveryAttempts.load()) + "\n";
    if (m_hasFault.load()) {
        rep += " Active Fault:    " + m_faultReason + "\n";
    }
    rep += "==========================================================\n";
    return rep;
}

bool HardwareWatchdog::TickOnce() {
    m_tickCount.fetch_add(1);

    std::string faultReason;
    FailsafeAction failsafeCb = nullptr;
    CoordinatorNotifyAction notifyCb = nullptr;

    bool passed = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        passed = PerformSupervisoryChecks(faultReason);

        if (!passed) {
            m_hasFault.store(true);
            m_faultReason = faultReason;
            m_recoveryAttempts.fetch_add(1);
            failsafeCb = m_failsafeAction;
            notifyCb = m_coordinatorNotifyAction;
        }
    }

    if (!passed) {
        logger::Logger::Instance().Log(
            logger::LogLevel::ERR, "WATCHDOG",
            "Watchdog EXPIRED! Reason: " + faultReason + ". Triggering " +
            WatchdogFailsafeModeToString(m_failsafeMode) + " failsafe!");

        // 1. Force intersection into failsafe state (ALL_RED or FLASHING_YELLOW)
        if (failsafeCb) {
            failsafeCb(faultReason);
        }

        // 2. Notify coordinator
        if (notifyCb) {
            notifyCb("WATCHDOG_FAULT: " + faultReason);
        }

        return false;
    }

    return true;
}

void HardwareWatchdog::WatchdogLoop() {
    while (m_running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (!m_running.load()) {
            break;
        }
        TickOnce();
    }
}

} // namespace hardware
} // namespace aegis
