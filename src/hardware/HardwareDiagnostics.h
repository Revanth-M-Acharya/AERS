/**
 * @file HardwareDiagnostics.h
 * @brief SMART AEGIS (AERS) Phase 20 - Hardware Self-Test & Diagnostic Suite.
 */

#ifndef AEGIS_HARDWARE_DIAGNOSTICS_H
#define AEGIS_HARDWARE_DIAGNOSTICS_H

#include "IntersectionHardware.h"
#include "WirelessNodeManager.h"
#include "HardwareEventLogger.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace aegis {
namespace hardware {

struct DiagnosticTestResult {
    std::string testName;
    bool passed{false};
    std::string details;
    double durationMs{0.0};
};

class HardwareDiagnostics {
public:
    explicit HardwareDiagnostics(std::string nodeId = "INT_A",
                                 std::shared_ptr<HardwareEventLogger> logger = nullptr);
    ~HardwareDiagnostics() = default;

    void SetIntersectionHardware(std::shared_ptr<IntersectionHardware> hardware);
    void SetWirelessManager(std::shared_ptr<WirelessNodeManager> wireless);

    // Individual Tests
    DiagnosticTestResult TestGpioPinIntegrity();
    DiagnosticTestResult TestRelayActuation();
    DiagnosticTestResult TestRelayPowerOnSelfTest();
    DiagnosticTestResult TestComplementaryInterlock();
    DiagnosticTestResult TestIrSensors();
    DiagnosticTestResult TestWirelessLink();
    DiagnosticTestResult TestFailsafeFallbackAndRecovery();

    // Full Suite Execution
    std::vector<DiagnosticTestResult> RunFullDiagnosticSuite();
    std::string GenerateDiagnosticReport();

    bool AllTestsPassed() const;

private:
    mutable std::mutex m_mutex;
    std::string m_nodeId;
    std::shared_ptr<HardwareEventLogger> m_logger;

    std::shared_ptr<IntersectionHardware> m_hardware;
    std::shared_ptr<WirelessNodeManager> m_wireless;

    std::vector<DiagnosticTestResult> m_lastResults;
    bool m_allPassed{false};
};

} // namespace hardware
} // namespace aegis

#endif // AEGIS_HARDWARE_DIAGNOSTICS_H

