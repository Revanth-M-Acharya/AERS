/**
 * @file TrafficSignalNode.cpp
 * @brief Implementation of TrafficSignalNode software simulation module for AERS (Phase 6A).
 */

#include "TrafficSignalNode.h"
#include "aegis/common/json_helper.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace aegis {
namespace traffic_signal {

static std::string TrimString(const std::string& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

static std::string ToUpperString(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

std::string SignalStateToString(SignalState state) {
    switch (state) {
        case SignalState::RED:             return "RED";
        case SignalState::YELLOW:          return "YELLOW";
        case SignalState::GREEN:           return "GREEN";
        case SignalState::FLASHING_RED:    return "FLASHING_RED";
        case SignalState::FLASHING_YELLOW: return "FLASHING_YELLOW";
        case SignalState::ALL_RED:         return "ALL_RED";
        default:                           return "RED";
    }
}

bool TryStringToSignalState(const std::string& stateStr, SignalState& outState) {
    std::string upper = ToUpperString(TrimString(stateStr));
    if (upper == "RED")             { outState = SignalState::RED; return true; }
    if (upper == "YELLOW")          { outState = SignalState::YELLOW; return true; }
    if (upper == "GREEN")           { outState = SignalState::GREEN; return true; }
    if (upper == "FLASHING_RED")    { outState = SignalState::FLASHING_RED; return true; }
    if (upper == "FLASHING_YELLOW") { outState = SignalState::FLASHING_YELLOW; return true; }
    if (upper == "ALL_RED")         { outState = SignalState::ALL_RED; return true; }
    return false;
}

SignalState StringToSignalState(const std::string& stateStr) {
    SignalState state = SignalState::RED;
    if (TryStringToSignalState(stateStr, state)) {
        return state;
    }
    return SignalState::RED;
}

TrafficSignalNode::TrafficSignalNode(std::string intersectionId, SignalState initialState)
    : m_mutex()
    , m_intersectionId(std::move(intersectionId))
    , m_currentState(initialState)
    , m_transitionHistory()
    , m_callback(nullptr) {
}

TrafficSignalNode::TrafficSignalNode(TrafficSignalNode&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_intersectionId = std::move(other.m_intersectionId);
    m_currentState = other.m_currentState;
    m_transitionHistory = std::move(other.m_transitionHistory);
    m_callback = std::move(other.m_callback);
}

TrafficSignalNode& TrafficSignalNode::operator=(TrafficSignalNode&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_intersectionId = std::move(other.m_intersectionId);
        m_currentState = other.m_currentState;
        m_transitionHistory = std::move(other.m_transitionHistory);
        m_callback = std::move(other.m_callback);
    }
    return *this;
}

SignalState TrafficSignalNode::GetCurrentState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentState;
}

std::string TrafficSignalNode::GetCurrentStateString() const {
    return SignalStateToString(GetCurrentState());
}

const std::string& TrafficSignalNode::GetIntersectionId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersectionId;
}

void TrafficSignalNode::SetIntersectionId(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_intersectionId = id;
}

bool TrafficSignalNode::CanTransitionTo(SignalState target) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return CanTransitionInternal(m_currentState, target);
}

bool TrafficSignalNode::CanTransitionInternal(SignalState from, SignalState target) const noexcept {
    // Requirement 5: Duplicate state changes are invalid/ignored
    if (from == target) {
        return false;
    }

    switch (from) {
        case SignalState::RED:
            // Standard cycle proceeds to GREEN. Emergency proceeds to ALL_RED. Degraded proceeds to FLASHING_*.
            // Direct RED -> YELLOW is prohibited.
            return (target == SignalState::GREEN ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::GREEN:
            // Standard cycle proceeds to YELLOW clearance. Emergency proceeds to ALL_RED.
            // Direct GREEN -> RED without YELLOW clearance is strictly rejected for safety.
            return (target == SignalState::YELLOW ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::YELLOW:
            // Yellow clearance proceeds to RED or ALL_RED. Degraded proceeds to FLASHING_*.
            // Direct jump from YELLOW back to GREEN is strictly prohibited.
            return (target == SignalState::RED ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::ALL_RED:
            // All-Red clearance can release to GREEN (green corridor), or revert to RED/YELLOW or FLASHING_*.
            return (target == SignalState::GREEN ||
                    target == SignalState::RED ||
                    target == SignalState::YELLOW ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        case SignalState::FLASHING_YELLOW:
        case SignalState::FLASHING_RED:
            // Flashing caution/hazard states may safely recover to RED or ALL_RED, or switch flashing mode.
            // Direct jumps to GREEN or YELLOW are rejected to protect uncoordinated traffic.
            return (target == SignalState::RED ||
                    target == SignalState::ALL_RED ||
                    target == SignalState::FLASHING_YELLOW ||
                    target == SignalState::FLASHING_RED);

        default:
            return false;
    }
}

bool TrafficSignalNode::SetSignalState(SignalState newState, const std::string& reason) {
    SignalTransitionRecord record;
    TransitionCallback cb = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // Requirement 5: Ignore duplicate state changes
        if (m_currentState == newState) {
            return false;
        }

        // Requirement 4: Reject invalid transitions
        if (!CanTransitionInternal(m_currentState, newState)) {
            return false;
        }

        SignalState prev = m_currentState;
        m_currentState = newState;

        record.timestampMs = CurrentTimestampMs();
        record.timestampStr = CurrentTimestampString();
        record.intersectionId = m_intersectionId;
        record.previousState = prev;
        record.newState = newState;
        record.reason = reason.empty() ? "State update requested" : reason;
        record.formattedLog = FormatTransitionLog(record.timestampStr,
                                                  record.intersectionId,
                                                  record.previousState,
                                                  record.newState,
                                                  record.reason);

        m_transitionHistory.push_back(record);
        cb = m_callback;
    }

    // Requirement 6: Log every successful transition
    std::cout << record.formattedLog << std::endl;

    if (cb) {
        cb(record);
    }

    return true;
}

bool TrafficSignalNode::SetSignalState(const std::string& newStateStr, const std::string& reason) {
    SignalState targetState;
    if (!TryStringToSignalState(newStateStr, targetState)) {
        return false;
    }
    return SetSignalState(targetState, reason);
}

bool TrafficSignalNode::HandleCoordinatorCommand(const std::string& rawCommand, const std::string& reason) {
    std::string cmd = TrimString(rawCommand);
    if (cmd.empty()) {
        return false;
    }

    std::string commandName = "";
    std::string extractedReason = "";

    // Support JSON format payloads: {"command": "SET_GREEN"} or {"action": "SET_GREEN"}
    if (cmd.front() == '{') {
        auto cmdOpt = common::JsonHelper::ExtractString(cmd, "command");
        if (cmdOpt.has_value()) {
            commandName = cmdOpt.value();
        } else {
            auto actionOpt = common::JsonHelper::ExtractString(cmd, "action");
            if (actionOpt.has_value()) {
                commandName = actionOpt.value();
            } else {
                auto stateOpt = common::JsonHelper::ExtractString(cmd, "state");
                if (stateOpt.has_value()) {
                    commandName = "SET_" + stateOpt.value();
                }
            }
        }
        auto reasonOpt = common::JsonHelper::ExtractString(cmd, "reason");
        if (reasonOpt.has_value()) {
            extractedReason = reasonOpt.value();
        }
    } else {
        commandName = cmd;
    }

    std::string normalized = ToUpperString(TrimString(commandName));
    std::string effectiveReason = !reason.empty() ? reason : (!extractedReason.empty() ? extractedReason : ("Coordinator command " + normalized));

    if (normalized == "SET_RED" || normalized == "RED") {
        return SetSignalState(SignalState::RED, effectiveReason);
    }
    if (normalized == "SET_YELLOW" || normalized == "YELLOW") {
        return SetSignalState(SignalState::YELLOW, effectiveReason);
    }
    if (normalized == "SET_GREEN" || normalized == "GREEN") {
        return SetSignalState(SignalState::GREEN, effectiveReason);
    }
    if (normalized == "SET_ALL_RED" || normalized == "ALL_RED") {
        return SetSignalState(SignalState::ALL_RED, effectiveReason);
    }
    if (normalized == "SET_FLASHING_RED" || normalized == "FLASHING_RED") {
        return SetSignalState(SignalState::FLASHING_RED, effectiveReason);
    }
    if (normalized == "SET_FLASHING_YELLOW" || normalized == "FLASHING_YELLOW") {
        return SetSignalState(SignalState::FLASHING_YELLOW, effectiveReason);
    }

    return false;
}

std::vector<SignalTransitionRecord> TrafficSignalNode::GetTransitionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_transitionHistory;
}

size_t TrafficSignalNode::GetTransitionCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_transitionHistory.size();
}

std::string TrafficSignalNode::GetLastTransitionLog() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_transitionHistory.empty()) {
        return "";
    }
    return m_transitionHistory.back().formattedLog;
}

void TrafficSignalNode::ClearTransitionHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_transitionHistory.clear();
}

void TrafficSignalNode::Reset(SignalState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentState = state;
}

void TrafficSignalNode::SetTransitionCallback(TransitionCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callback = std::move(callback);
}

std::string TrafficSignalNode::CurrentTimestampString() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
#if defined(_WIN32)
    localtime_s(&bt, &timer);
#else
    localtime_r(&timer, &bt);
#endif
    std::ostringstream oss;
    oss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << "."
        << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

uint64_t TrafficSignalNode::CurrentTimestampMs() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string TrafficSignalNode::FormatTransitionLog(const std::string& timestampStr,
                                                   const std::string& intersectionId,
                                                   SignalState previousState,
                                                   SignalState newState,
                                                   const std::string& reason) {
    std::ostringstream oss;
    oss << "[" << timestampStr << "]\n"
        << "Intersection ID: " << intersectionId << "\n"
        << "Previous State: " << SignalStateToString(previousState) << "\n"
        << "New State: " << SignalStateToString(newState) << "\n"
        << "Reason: " << reason;
    return oss.str();
}

} // namespace traffic_signal
} // namespace aegis
