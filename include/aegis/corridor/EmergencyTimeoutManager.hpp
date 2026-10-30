/**
 * @file EmergencyTimeoutManager.hpp
 * @brief SMART AEGIS (AERS) Phase 3 - Multi-Domain Emergency Timeout & Deadlock-Free Escalation Manager.
 *
 * Supervise every active emergency request across four distinct operational timeout domains:
 * 1. Stalled Ambulance Detection (lack of spatial progress along assigned corridor)
 * 2. Coordinator Inactivity Detection (central pulse / command silence)
 * 3. Communication Link Timeout Detection (TCP / Wi-Fi transport link silence)
 * 4. Intersection Acknowledgement Timeout Detection (unacknowledged signal preemption commands)
 *
 * Enforces configurable multi-tier escalation policies while mathematically guaranteeing
 * zero deadlocks across all real-time controller threads.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_CORRIDOR_EMERGENCY_TIMEOUT_MANAGER_HPP
#define AEGIS_CORRIDOR_EMERGENCY_TIMEOUT_MANAGER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cstdint>
#include <memory>
#include <sstream>
#include <iomanip>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"

namespace aegis {
namespace corridor {

/**
 * @brief Domains of real-time timeout supervision.
 */
enum class TimeoutDomain {
    AMBULANCE_STALL,        ///< Vehicle has ceased spatial progression between intersections
    COORDINATOR_INACTIVITY, ///< Central coordinator has missed cyclic dispatch ticks
    COMMUNICATION_LINK,     ///< Socket / TCP link heartbeat lost or degraded
    INTERSECTION_ACK        ///< Preemption command unacknowledged by edge intersection node
};

inline const char* TimeoutDomainToString(TimeoutDomain domain) noexcept {
    switch (domain) {
        case TimeoutDomain::AMBULANCE_STALL:        return "AMBULANCE_STALL";
        case TimeoutDomain::COORDINATOR_INACTIVITY: return "COORDINATOR_INACTIVITY";
        case TimeoutDomain::COMMUNICATION_LINK:     return "COMMUNICATION_LINK";
        case TimeoutDomain::INTERSECTION_ACK:       return "INTERSECTION_ACK";
        default:                                    return "UNKNOWN";
    }
}

/**
 * @brief Multi-tier timeout escalation policy levels.
 */
enum class EscalationTier {
    NOMINAL,                ///< Normal operation; zero timeout thresholds breached
    TIER_1_RETRY,           ///< Retry command transmission or request node status ping
    TIER_2_DEGRADE_ALERT,   ///< Escalate to operator, log alert, enable local autonomous fallback
    TIER_3_FAILSAFE_RECOVERY///< Autonomous safe mode transition or corridor abort/replan
};

inline const char* EscalationTierToString(EscalationTier tier) noexcept {
    switch (tier) {
        case EscalationTier::NOMINAL:                return "NOMINAL";
        case EscalationTier::TIER_1_RETRY:           return "TIER_1_RETRY";
        case EscalationTier::TIER_2_DEGRADE_ALERT:   return "TIER_2_DEGRADE_ALERT";
        case EscalationTier::TIER_3_FAILSAFE_RECOVERY: return "TIER_3_FAILSAFE_RECOVERY";
        default:                                     return "UNKNOWN";
    }
}

/**
 * @brief Configurable threshold parameters governing emergency supervision.
 */
struct EmergencyTimeoutPolicy {
    double ambulanceStallTimeoutSec{15.0};      ///< Max duration without waypoint advancement (seconds)
    double coordinatorInactivityTimeoutSec{5.0};///< Max duration of coordinator silence (seconds)
    double commLinkTimeoutSec{3.0};             ///< Max duration of edge node communication silence (seconds)
    double intersectionAckTimeoutMs{1000.0};    ///< Preemption ACK window in milliseconds
    uint32_t maxAckRetries{3};                  ///< Max retransmissions before Tier 2 escalation
    bool autoFailsafeOnTier3{true};             ///< Automatically force safe state on Tier 3
};

/**
 * @brief Structured event emitted during timeout escalation.
 */
struct TimeoutEscalationEvent {
    std::string requestId{""};
    std::string targetNodeId{""};
    TimeoutDomain domain{TimeoutDomain::AMBULANCE_STALL};
    EscalationTier tier{EscalationTier::NOMINAL};
    uint64_t timestampUs{0};
    double durationOverdueSec{0.0};
    std::string remediationAction{""};
    std::string details{""};

    std::string ToJson() const {
        std::ostringstream oss;
        oss << "{"
            << "\"requestId\":\"" << requestId << "\","
            << "\"targetNodeId\":\"" << targetNodeId << "\","
            << "\"domain\":\"" << TimeoutDomainToString(domain) << "\","
            << "\"tier\":\"" << EscalationTierToString(tier) << "\","
            << "\"timestampUs\":" << timestampUs << ","
            << "\"durationOverdueSec\":" << std::fixed << std::setprecision(3) << durationOverdueSec << ","
            << "\"remediationAction\":\"" << remediationAction << "\","
            << "\"details\":\"" << details << "\""
            << "}";
        return oss.str();
    }
};

/**
 * @brief Tracking structure for an active emergency mission supervision.
 */
struct EmergencySupervisionRecord {
    std::string requestId{""};
    std::string ambulanceId{""};
    std::string currentIntersection{""};
    uint64_t lastProgressTimestampUs{0};
    bool isStalled{false};

    // Preemption ACK tracking
    std::string pendingAckIntersectionId{""};
    uint64_t ackRequestTimestampUs{0};
    uint32_t ackRetryCount{0};
    bool isAckPending{false};

    EscalationTier activeEscalationTier{EscalationTier::NOMINAL};
    bool isCompleted{false};
};

/**
 * @class EmergencyTimeoutManager
 * @brief Real-time multi-domain timeout supervisor and deadlock-free escalation engine.
 */
class EmergencyTimeoutManager {
public:
    using EscalationHandler = std::function<void(const TimeoutEscalationEvent&)>;

    explicit EmergencyTimeoutManager(const EmergencyTimeoutPolicy& policy = EmergencyTimeoutPolicy{});
    ~EmergencyTimeoutManager() = default;

    EmergencyTimeoutManager(const EmergencyTimeoutManager&) = delete;
    EmergencyTimeoutManager& operator=(const EmergencyTimeoutManager&) = delete;
    EmergencyTimeoutManager(EmergencyTimeoutManager&&) = delete;
    EmergencyTimeoutManager& operator=(EmergencyTimeoutManager&&) = delete;

    /**
     * @brief Registers an emergency mission request for multi-domain supervision.
     */
    bool RegisterRequest(const std::string& requestId,
                         const std::string& ambulanceId,
                         const std::string& startIntersection,
                         uint64_t timestampUs = 0);

    /**
     * @brief Records progress of an ambulance (resets stall detector).
     */
    bool RecordAmbulanceProgress(const std::string& requestId,
                                const std::string& currentIntersection,
                                uint64_t timestampUs = 0);

    /**
     * @brief Records heartbeat / pulse activity from the central coordinator.
     */
    void RecordCoordinatorActivity(uint64_t timestampUs = 0);

    /**
     * @brief Records communication activity (ping/message) from an edge intersection node.
     */
    void RecordNodeCommunication(const std::string& nodeId, uint64_t timestampUs = 0);

    /**
     * @brief Arms an expected preemption ACK timeout window for a specific intersection.
     */
    bool ExpectIntersectionAck(const std::string& requestId,
                               const std::string& intersectionId,
                               uint64_t timestampUs = 0);

    /**
     * @brief Acknowledges preemption confirmation from an intersection (clears ACK pending).
     */
    bool ReceiveIntersectionAck(const std::string& requestId,
                                const std::string& intersectionId,
                                uint64_t timestampUs = 0);

    /**
     * @brief Concludes supervision for an emergency request.
     */
    bool CompleteRequest(const std::string& requestId);

    /**
     * @brief Evaluates all 4 timeout domains and executes deterministic escalation.
     * Guarantees deadlock-free execution: callbacks executed strictly outside lock.
     * @return Number of escalation events generated in this cycle.
     */
    size_t CheckTimeouts(uint64_t currentTimestampUs = 0);

    /**
     * @brief Installs custom escalation handler.
     */
    void SetEscalationHandler(EscalationHandler handler);

    /**
     * @brief Updates the supervision policy configuration.
     */
    void SetPolicy(const EmergencyTimeoutPolicy& policy);

    EmergencyTimeoutPolicy GetPolicy() const;

    /**
     * @brief Checks if an emergency request is currently flagged as stalled.
     */
    bool IsAmbulanceStalled(const std::string& requestId) const;

    /**
     * @brief Checks if coordinator inactivity has breached configured threshold.
     */
    bool IsCoordinatorInactive(uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Checks if communication with a specific node has timed out.
     */
    bool IsNodeCommunicationTimedOut(const std::string& nodeId, uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Retrieves all historical escalation events.
     */
    std::vector<TimeoutEscalationEvent> GetEscalationEvents() const;

    /**
     * @brief Returns ASCII diagnostic audit report of all supervised domains.
     */
    std::string GenerateTimeoutReport(uint64_t currentTimestampUs = 0) const;

    /**
     * @brief Clears all internal tracking tables.
     */
    void ClearAll();

private:
    uint64_t GetCurrentTimeUs() const noexcept;

    mutable std::mutex m_mutex;
    EmergencyTimeoutPolicy m_policy;

    std::unordered_map<std::string, EmergencySupervisionRecord> m_requests;
    std::unordered_map<std::string, uint64_t> m_nodeCommTimestampsUs;
    uint64_t m_lastCoordinatorActivityUs{0};

    std::vector<TimeoutEscalationEvent> m_events;
    EscalationHandler m_handler{nullptr};
};

} // namespace corridor
} // namespace aegis

#endif // AEGIS_CORRIDOR_EMERGENCY_TIMEOUT_MANAGER_HPP
