/**
 * @file QnxDiagnostics.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Native QNX Diagnostics & Health Telemetry.
 *
 * Aggregates system-wide QNX RTOS telemetry:
 * - Resource Manager device nodes (/dev/aegis nodes) status & I/O counts
 * - Pulse metrics: total sent, received, dropped, per-code breakdown
 * - High-resolution timer jitter & overruns
 * - Real-time thread priority hierarchy verification
 * - GPIO interrupt event counters & state (IR approach, departure, emergency, relay fault)
 * - IPC round-trip latency & memory telemetry
 * - Formatted JSON and ASCII table outputs
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_DIAGNOSTICS_HPP
#define AEGIS_QNX_DIAGNOSTICS_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "aegis/qnx/QnxResourceManager.hpp"
#include "aegis/qnx/QnxPulseManager.hpp"
#include "aegis/qnx/QnxTimerService.hpp"
#include "aegis/qnx/QnxThreadScheduler.hpp"
#include "aegis/qnx/QnxInterruptManager.hpp"

namespace aegis {
namespace qnx {

/**
 * @brief Complete diagnostic state snapshot of the QNX Neutrino RTOS runtime.
 */
struct QnxDiagnosticSnapshot {
    uint64_t timestampUs{0};
    std::string osVersion;
    std::string hardwareTarget;

    // Resource Manager
    std::vector<DeviceNodeInfo> deviceNodes;

    // Pulse Telemetry
    PulseStats pulseStats;

    // Thread Scheduling
    std::vector<ThreadScheduleInfo> threads;

    // Hardware Interrupts
    std::vector<InterruptInfo> interrupts;

    // System Telemetry
    double cpuUsagePercent{0.0};
    double ramUsageMB{0.0};
    double cpuTempC{0.0};
    uint64_t uptimeSec{0};
    bool priorityInversionSafe{true};
};

/**
 * @brief Aggregator for native QNX diagnostic telemetry and health reporting.
 */
class QnxDiagnostics {
public:
    QnxDiagnostics(
        std::shared_ptr<QnxPulseManager> pulseMgr = nullptr,
        std::shared_ptr<QnxTimerService> timerService = nullptr,
        std::shared_ptr<QnxThreadScheduler> scheduler = nullptr,
        std::shared_ptr<QnxInterruptManager> intrMgr = nullptr
    );

    ~QnxDiagnostics() = default;

    void SetPulseManager(std::shared_ptr<QnxPulseManager> pulseMgr);
    void SetTimerService(std::shared_ptr<QnxTimerService> timerService);
    void SetThreadScheduler(std::shared_ptr<QnxThreadScheduler> scheduler);
    void SetInterruptManager(std::shared_ptr<QnxInterruptManager> intrMgr);

    /**
     * @brief Captures a live diagnostic snapshot across all subsystems.
     */
    QnxDiagnosticSnapshot CaptureSnapshot() const;

    /**
     * @brief Generates structured JSON representation of system state.
     */
    std::string GenerateJsonReport() const;

    /**
     * @brief Generates formatted ASCII dashboard for CLI/operator review.
     */
    std::string GenerateAsciiReport() const;

private:
    std::shared_ptr<QnxPulseManager> m_pulseMgr;
    std::shared_ptr<QnxTimerService> m_timerService;
    std::shared_ptr<QnxThreadScheduler> m_scheduler;
    std::shared_ptr<QnxInterruptManager> m_intrMgr;
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_DIAGNOSTICS_HPP
