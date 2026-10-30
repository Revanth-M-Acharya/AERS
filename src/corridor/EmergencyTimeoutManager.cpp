/**
 * @file EmergencyTimeoutManager.cpp
 * @brief Implementation of Phase 3 Emergency Timeout Manager for SMART AEGIS.
 */

#include "aegis/corridor/EmergencyTimeoutManager.hpp"
#include "aegis/logger/logger.hpp"

#include <iostream>

namespace aegis {
namespace corridor {

[[maybe_unused]] static void LogInfo(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::INFO, "TIMEOUT_MGR", msg);
}

[[maybe_unused]] static void LogWarn(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::WARN, "TIMEOUT_MGR", msg);
}

[[maybe_unused]] static void LogError(const std::string& msg) {
    aegis::logger::Logger::Instance().Log(aegis::logger::LogLevel::ERR, "TIMEOUT_MGR", msg);
}

EmergencyTimeoutManager::EmergencyTimeoutManager(const EmergencyTimeoutPolicy& policy)
    : m_policy(policy) {
    m_lastCoordinatorActivityUs = aegis::common::GetMonotonicMicroseconds();
}

uint64_t EmergencyTimeoutManager::GetCurrentTimeUs() const noexcept {
    return aegis::common::GetMonotonicMicroseconds();
}

bool EmergencyTimeoutManager::RegisterRequest(const std::string& requestId,
                                              const std::string& ambulanceId,
                                              const std::string& startIntersection,
                                              uint64_t timestampUs) {
    if (requestId.empty() || ambulanceId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_requests.find(requestId) != m_requests.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();

    EmergencySupervisionRecord rec;
    rec.requestId = requestId;
    rec.ambulanceId = ambulanceId;
    rec.currentIntersection = startIntersection;
    rec.lastProgressTimestampUs = nowUs;
    rec.isStalled = false;
    rec.pendingAckIntersectionId = "";
    rec.ackRequestTimestampUs = 0;
    rec.ackRetryCount = 0;
    rec.isAckPending = false;
    rec.activeEscalationTier = EscalationTier::NOMINAL;
    rec.isCompleted = false;

    m_requests[requestId] = std::move(rec);
    return true;
}

bool EmergencyTimeoutManager::RecordAmbulanceProgress(const std::string& requestId,
                                                     const std::string& currentIntersection,
                                                     uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(requestId);
    if (it == m_requests.end()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.currentIntersection = currentIntersection;
    it->second.lastProgressTimestampUs = nowUs;
    it->second.isStalled = false;
    it->second.activeEscalationTier = EscalationTier::NOMINAL;
    return true;
}

void EmergencyTimeoutManager::RecordCoordinatorActivity(uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastCoordinatorActivityUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
}

void EmergencyTimeoutManager::RecordNodeCommunication(const std::string& nodeId, uint64_t timestampUs) {
    if (nodeId.empty()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nodeCommTimestampsUs[nodeId] = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
}

bool EmergencyTimeoutManager::ExpectIntersectionAck(const std::string& requestId,
                                                    const std::string& intersectionId,
                                                    uint64_t timestampUs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(requestId);
    if (it == m_requests.end() || intersectionId.empty()) {
        return false;
    }

    uint64_t nowUs = (timestampUs > 0) ? timestampUs : GetCurrentTimeUs();
    it->second.pendingAckIntersectionId = intersectionId;
    it->second.ackRequestTimestampUs = nowUs;
    it->second.ackRetryCount = 0;
    it->second.isAckPending = true;
    return true;
}

bool EmergencyTimeoutManager::ReceiveIntersectionAck(const std::string& requestId,
                                                    const std::string& intersectionId,
                                                    uint64_t timestampUs) {
    (void)timestampUs;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(requestId);
    if (it == m_requests.end()) {
        return false;
    }

    if (it->second.isAckPending && it->second.pendingAckIntersectionId == intersectionId) {
        it->second.isAckPending = false;
        it->second.pendingAckIntersectionId = "";
        it->second.ackRetryCount = 0;
        return true;
    }
    return false;
}

bool EmergencyTimeoutManager::CompleteRequest(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(requestId);
    if (it == m_requests.end()) {
        return false;
    }
    it->second.isCompleted = true;
    it->second.isAckPending = false;
    return true;
}

size_t EmergencyTimeoutManager::CheckTimeouts(uint64_t currentTimestampUs) {
    std::vector<TimeoutEscalationEvent> newlyEscalated;
    EscalationHandler localHandler = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();
        localHandler = m_handler;

        // 1. Check Ambulance Stalls
        for (auto& pair : m_requests) {
            auto& req = pair.second;
            if (req.isCompleted) continue;

            if (nowUs >= req.lastProgressTimestampUs) {
                double timeSinceProgressSec = static_cast<double>(nowUs - req.lastProgressTimestampUs) / 1000000.0;
                if (timeSinceProgressSec > m_policy.ambulanceStallTimeoutSec) {
                    if (!req.isStalled) {
                        req.isStalled = true;
                        req.activeEscalationTier = EscalationTier::TIER_2_DEGRADE_ALERT;

                        TimeoutEscalationEvent ev;
                        ev.requestId = req.requestId;
                        ev.targetNodeId = req.currentIntersection;
                        ev.domain = TimeoutDomain::AMBULANCE_STALL;
                        ev.tier = EscalationTier::TIER_2_DEGRADE_ALERT;
                        ev.timestampUs = nowUs;
                        ev.durationOverdueSec = timeSinceProgressSec - m_policy.ambulanceStallTimeoutSec;
                        ev.remediationAction = "ALERT_OPERATOR_CHECK_AMBULANCE_PROGRESS";
                        ev.details = "Ambulance " + req.ambulanceId + " stalled at " + req.currentIntersection;

                        m_events.push_back(ev);
                        newlyEscalated.push_back(ev);
                        LogWarn(ev.details);
                    }
                }
            }

            // 2. Check Intersection Preemption ACK Timeouts
            if (req.isAckPending && nowUs >= req.ackRequestTimestampUs) {
                double ackWaitMs = static_cast<double>(nowUs - req.ackRequestTimestampUs) / 1000.0;
                if (ackWaitMs > m_policy.intersectionAckTimeoutMs) {
                    req.ackRetryCount++;
                    if (req.ackRetryCount <= m_policy.maxAckRetries) {
                        req.ackRequestTimestampUs = nowUs; // reset for next retry interval
                        TimeoutEscalationEvent ev;
                        ev.requestId = req.requestId;
                        ev.targetNodeId = req.pendingAckIntersectionId;
                        ev.domain = TimeoutDomain::INTERSECTION_ACK;
                        ev.tier = EscalationTier::TIER_1_RETRY;
                        ev.timestampUs = nowUs;
                        ev.durationOverdueSec = (ackWaitMs - m_policy.intersectionAckTimeoutMs) / 1000.0;
                        ev.remediationAction = "RETRANSMIT_PREEMPTION_COMMAND_RETRY_" + std::to_string(req.ackRetryCount);
                        ev.details = "ACK timeout for " + req.pendingAckIntersectionId + "; attempting retry " +
                                     std::to_string(req.ackRetryCount) + "/" + std::to_string(m_policy.maxAckRetries);

                        m_events.push_back(ev);
                        newlyEscalated.push_back(ev);
                        LogWarn(ev.details);
                    } else {
                        // Max retries exhausted -> Escalate to TIER 2 or TIER 3
                        EscalationTier targetTier = m_policy.autoFailsafeOnTier3 ? EscalationTier::TIER_3_FAILSAFE_RECOVERY : EscalationTier::TIER_2_DEGRADE_ALERT;
                        req.activeEscalationTier = targetTier;
                        req.isAckPending = false; // Disarm further ACK polling

                        TimeoutEscalationEvent ev;
                        ev.requestId = req.requestId;
                        ev.targetNodeId = req.pendingAckIntersectionId;
                        ev.domain = TimeoutDomain::INTERSECTION_ACK;
                        ev.tier = targetTier;
                        ev.timestampUs = nowUs;
                        ev.durationOverdueSec = (ackWaitMs - m_policy.intersectionAckTimeoutMs) / 1000.0;
                        ev.remediationAction = (targetTier == EscalationTier::TIER_3_FAILSAFE_RECOVERY)
                                                   ? "TRIGGER_AUTONOMOUS_FAILSAFE_OR_REPLAN"
                                                   : "FALLBACK_LOCAL_PREEMPTION";
                        ev.details = "ACK retries exhausted for " + req.pendingAckIntersectionId + "; escalating to " +
                                     EscalationTierToString(targetTier);

                        m_events.push_back(ev);
                        newlyEscalated.push_back(ev);
                        LogError(ev.details);
                    }
                }
            }
        }

        // 3. Check Coordinator Inactivity
        if (nowUs >= m_lastCoordinatorActivityUs) {
            double coordSilenceSec = static_cast<double>(nowUs - m_lastCoordinatorActivityUs) / 1000000.0;
            if (coordSilenceSec > m_policy.coordinatorInactivityTimeoutSec) {
                TimeoutEscalationEvent ev;
                ev.requestId = "SYSTEM_COORDINATOR";
                ev.targetNodeId = "COORDINATOR_01";
                ev.domain = TimeoutDomain::COORDINATOR_INACTIVITY;
                ev.tier = EscalationTier::TIER_3_FAILSAFE_RECOVERY;
                ev.timestampUs = nowUs;
                ev.durationOverdueSec = coordSilenceSec - m_policy.coordinatorInactivityTimeoutSec;
                ev.remediationAction = "TRIGGER_CLUSTER_STANDBY_COORDINATOR_OR_LOCAL_FALLBACK";
                ev.details = "Coordinator silence (" + std::to_string(coordSilenceSec) + "s) exceeds threshold (" +
                             std::to_string(m_policy.coordinatorInactivityTimeoutSec) + "s)";

                m_events.push_back(ev);
                newlyEscalated.push_back(ev);
                LogError(ev.details);
            }
        }

        // 4. Check Communication Link Timeouts
        for (const auto& pair : m_nodeCommTimestampsUs) {
            const std::string& nodeId = pair.first;
            uint64_t lastSeenUs = pair.second;
            if (nowUs >= lastSeenUs) {
                double linkSilenceSec = static_cast<double>(nowUs - lastSeenUs) / 1000000.0;
                if (linkSilenceSec > m_policy.commLinkTimeoutSec) {
                    TimeoutEscalationEvent ev;
                    ev.requestId = "LINK_" + nodeId;
                    ev.targetNodeId = nodeId;
                    ev.domain = TimeoutDomain::COMMUNICATION_LINK;
                    ev.tier = EscalationTier::TIER_2_DEGRADE_ALERT;
                    ev.timestampUs = nowUs;
                    ev.durationOverdueSec = linkSilenceSec - m_policy.commLinkTimeoutSec;
                    ev.remediationAction = "AUTO_RECONNECT_SOCKET_AND_DEGRADE_QOS";
                    ev.details = "Communication link to " + nodeId + " silent for " +
                                 std::to_string(linkSilenceSec) + "s";

                    m_events.push_back(ev);
                    newlyEscalated.push_back(ev);
                    LogWarn(ev.details);
                }
            }
        }
    }

    // Deadlock freedom: callbacks run strictly outside lock
    if (localHandler && !newlyEscalated.empty()) {
        for (const auto& ev : newlyEscalated) {
            localHandler(ev);
        }
    }

    return newlyEscalated.size();
}

void EmergencyTimeoutManager::SetEscalationHandler(EscalationHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handler = std::move(handler);
}

void EmergencyTimeoutManager::SetPolicy(const EmergencyTimeoutPolicy& policy) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_policy = policy;
}

EmergencyTimeoutPolicy EmergencyTimeoutManager::GetPolicy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_policy;
}

bool EmergencyTimeoutManager::IsAmbulanceStalled(const std::string& requestId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_requests.find(requestId);
    if (it != m_requests.end()) {
        return it->second.isStalled;
    }
    return false;
}

bool EmergencyTimeoutManager::IsCoordinatorInactive(uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();
    if (nowUs >= m_lastCoordinatorActivityUs) {
        double silenceSec = static_cast<double>(nowUs - m_lastCoordinatorActivityUs) / 1000000.0;
        return silenceSec > m_policy.coordinatorInactivityTimeoutSec;
    }
    return false;
}

bool EmergencyTimeoutManager::IsNodeCommunicationTimedOut(const std::string& nodeId, uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_nodeCommTimestampsUs.find(nodeId);
    if (it == m_nodeCommTimestampsUs.end()) {
        return true;
    }
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();
    if (nowUs >= it->second) {
        double silenceSec = static_cast<double>(nowUs - it->second) / 1000000.0;
        return silenceSec > m_policy.commLinkTimeoutSec;
    }
    return false;
}

std::vector<TimeoutEscalationEvent> EmergencyTimeoutManager::GetEscalationEvents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::string EmergencyTimeoutManager::GenerateTimeoutReport(uint64_t currentTimestampUs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t nowUs = (currentTimestampUs > 0) ? currentTimestampUs : GetCurrentTimeUs();

    std::ostringstream oss;
    oss << "=========================================================================\n"
        << " SMART AEGIS - EMERGENCY TIMEOUT & ESCALATION REPORT\n"
        << "=========================================================================\n"
        << " Supervised Requests: " << m_requests.size() << "\n"
        << " Monitored Nodes    : " << m_nodeCommTimestampsUs.size() << "\n"
        << " Escalation Events  : " << m_events.size() << "\n"
        << "-------------------------------------------------------------------------\n"
        << " Request ID  | Ambulance  | Intersection | Stalled | ACK Pending | Tier\n"
        << "-------------------------------------------------------------------------\n";

    for (const auto& pair : m_requests) {
        const auto& r = pair.second;
        oss << " " << std::left << std::setw(11) << r.requestId.substr(0, 11) << " | "
            << std::setw(10) << r.ambulanceId.substr(0, 10) << " | "
            << std::setw(12) << r.currentIntersection.substr(0, 12) << " | "
            << std::setw(7) << (r.isStalled ? "YES" : "NO") << " | "
            << std::setw(11) << (r.isAckPending ? r.pendingAckIntersectionId.substr(0, 11) : "NONE") << " | "
            << EscalationTierToString(r.activeEscalationTier) << "\n";
    }

    double coordSilenceSec = 0.0;
    if (nowUs >= m_lastCoordinatorActivityUs) {
        coordSilenceSec = static_cast<double>(nowUs - m_lastCoordinatorActivityUs) / 1000000.0;
    }
    oss << "-------------------------------------------------------------------------\n"
        << " Coordinator Silence: " << std::fixed << std::setprecision(2) << coordSilenceSec << "s (Threshold: "
        << m_policy.coordinatorInactivityTimeoutSec << "s)\n"
        << "=========================================================================\n";
    return oss.str();
}

void EmergencyTimeoutManager::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_requests.clear();
    m_nodeCommTimestampsUs.clear();
    m_events.clear();
    m_lastCoordinatorActivityUs = GetCurrentTimeUs();
}

} // namespace corridor
} // namespace aegis
