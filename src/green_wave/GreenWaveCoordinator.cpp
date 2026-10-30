/**
 * @file GreenWaveCoordinator.cpp
 * @brief Implementation of Dynamic Green Wave Coordination Engine for SMART AEGIS (Phase 7B).
 */

#include "GreenWaveCoordinator.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace aegis {
namespace green_wave {

static std::string ToLowerString(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

GreenWaveCoordinator::GreenWaveCoordinator()
    : m_mutex()
    , m_safetyBufferSeconds(3.0)
    , m_clearanceSeconds(5.0)
    , m_command("GREEN")
    , m_autoLogging(true)
    , m_nonSignalNodes()
    , m_currentPlan()
    , m_lastLog("") {
}

GreenWaveCoordinator::GreenWaveCoordinator(double safetyBufferSeconds, double clearanceSeconds)
    : GreenWaveCoordinator() {
    m_safetyBufferSeconds = (safetyBufferSeconds >= 0.0) ? safetyBufferSeconds : 3.0;
    m_clearanceSeconds = (clearanceSeconds >= 0.0) ? clearanceSeconds : 5.0;
}

GreenWaveCoordinator::GreenWaveCoordinator(GreenWaveCoordinator&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_safetyBufferSeconds = other.m_safetyBufferSeconds;
    m_clearanceSeconds = other.m_clearanceSeconds;
    m_command = std::move(other.m_command);
    m_autoLogging = other.m_autoLogging;
    m_nonSignalNodes = std::move(other.m_nonSignalNodes);
    m_currentPlan = std::move(other.m_currentPlan);
    m_lastLog = std::move(other.m_lastLog);
}

GreenWaveCoordinator& GreenWaveCoordinator::operator=(GreenWaveCoordinator&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_safetyBufferSeconds = other.m_safetyBufferSeconds;
        m_clearanceSeconds = other.m_clearanceSeconds;
        m_command = std::move(other.m_command);
        m_autoLogging = other.m_autoLogging;
        m_nonSignalNodes = std::move(other.m_nonSignalNodes);
        m_currentPlan = std::move(other.m_currentPlan);
        m_lastLog = std::move(other.m_lastLog);
    }
    return *this;
}

void GreenWaveCoordinator::SetSafetyBufferSeconds(double buffer) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_safetyBufferSeconds = (buffer >= 0.0) ? buffer : 0.0;
}

double GreenWaveCoordinator::GetSafetyBufferSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_safetyBufferSeconds;
}

void GreenWaveCoordinator::SetClearanceSeconds(double clearance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_clearanceSeconds = (clearance >= 0.0) ? clearance : 0.0;
}

double GreenWaveCoordinator::GetClearanceSeconds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_clearanceSeconds;
}

void GreenWaveCoordinator::SetCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_command = command.empty() ? "GREEN" : command;
}

std::string GreenWaveCoordinator::GetCommand() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_command;
}

void GreenWaveCoordinator::AddNonSignalNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nonSignalNodes.insert(nodeId);
}

void GreenWaveCoordinator::RemoveNonSignalNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nonSignalNodes.erase(nodeId);
}

void GreenWaveCoordinator::ClearNonSignalNodes() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nonSignalNodes.clear();
}

bool GreenWaveCoordinator::IsSignalized(const std::string& nodeId, const std::string& hospitalId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_nonSignalNodes.find(nodeId) != m_nonSignalNodes.end()) {
        return false;
    }
    if (!hospitalId.empty() && nodeId == hospitalId) {
        return false;
    }
    if (ToLowerString(nodeId).find("hospital") != std::string::npos) {
        return false;
    }
    return true;
}

GreenWindow GreenWaveCoordinator::EstimateGreenWindow(const std::string& intersectionId,
                                                      double etaSeconds,
                                                      double safetyBufferSeconds,
                                                      double clearanceSeconds,
                                                      const std::string& command) const {
    double start = std::max(0.0, etaSeconds - safetyBufferSeconds);
    double end = etaSeconds + clearanceSeconds;
    return GreenWindow(intersectionId, start, end, command, safetyBufferSeconds, clearanceSeconds, etaSeconds);
}

bool GreenWaveCoordinator::EstimateGreenWindow(double etaSeconds,
                                               double safetyBufferSeconds,
                                               double clearanceSeconds,
                                               double& outStart,
                                               double& outEnd) const {
    if (etaSeconds < 0.0 || safetyBufferSeconds < 0.0 || clearanceSeconds < 0.0) {
        outStart = 0.0;
        outEnd = 0.0;
        return false;
    }
    outStart = std::max(0.0, etaSeconds - safetyBufferSeconds);
    outEnd = etaSeconds + clearanceSeconds;
    return true;
}

bool GreenWaveCoordinator::GenerateGreenWaveSchedule(const EmergencyRoute& route, GreenWavePlan& outPlan) {
    if (!route.isValid || route.intersections.empty() || route.timeline.empty()) {
        outPlan = GreenWavePlan{};
        outPlan.isValid = false;
        return false;
    }

    double buffer;
    double clearance;
    std::string cmd;
    bool doLogging;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        buffer = m_safetyBufferSeconds;
        clearance = m_clearanceSeconds;
        cmd = m_command;
        doLogging = m_autoLogging;
    }

    // Map ETAs from route timeline
    std::unordered_map<std::string, double> etaMap;
    for (const auto& etaItem : route.timeline) {
        etaMap[etaItem.intersectionId] = etaItem.etaSeconds;
    }

    GreenWavePlan plan;
    plan.ambulanceId = route.ambulanceId.empty() ? "AMBULANCE" : route.ambulanceId;
    plan.hospitalId = route.hospitalId.empty() ? "HOSPITAL" : route.hospitalId;
    plan.routeId = plan.ambulanceId + "_" + plan.hospitalId;

    for (const auto& intersectionId : route.intersections) {
        // Skip non-signal destinations like Hospital
        if (!IsSignalized(intersectionId, route.hospitalId)) {
            continue;
        }

        auto it = etaMap.find(intersectionId);
        if (it == etaMap.end()) {
            // Missing intersection in timeline
            outPlan = GreenWavePlan{};
            outPlan.isValid = false;
            return false;
        }

        double eta = it->second;
        GreenWindow window = EstimateGreenWindow(intersectionId, eta, buffer, clearance, cmd);
        plan.windows.push_back(window);
    }

    if (plan.windows.empty()) {
        // Missing intersections / no signalized intersections found
        outPlan = GreenWavePlan{};
        outPlan.isValid = false;
        return false;
    }

    plan.totalDurationSeconds = plan.windows.back().endTimeSeconds;
    plan.isValid = true;

    std::string logStr = FormatLog(plan);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentPlan = plan;
        m_lastLog = logStr;
    }

    if (doLogging) {
        PrintLog(plan);
    }

    outPlan = plan;
    return true;
}

GreenWavePlan GreenWaveCoordinator::GenerateGreenWaveSchedule(const EmergencyRoute& route) {
    GreenWavePlan plan;
    GenerateGreenWaveSchedule(route, plan);
    return plan;
}

GreenWavePlan GreenWaveCoordinator::GetCurrentPlan() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentPlan;
}

void GreenWaveCoordinator::ClearPlan() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentPlan = GreenWavePlan{};
    m_lastLog.clear();
}

std::string GreenWaveCoordinator::FormatPlan(const GreenWavePlan& plan) const {
    return FormatLog(plan);
}

std::string GreenWaveCoordinator::FormatPlan() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return FormatLog(m_currentPlan);
}

std::string GreenWaveCoordinator::FormatLog(const GreenWavePlan& plan) {
    std::ostringstream oss;
    oss << "[GREEN WAVE GENERATED]\n";
    oss << "Ambulance: " << (plan.ambulanceId.empty() ? "AMBULANCE" : plan.ambulanceId) << "\n";
    oss << "Hospital: " << (plan.hospitalId.empty() ? "HOSPITAL" : plan.hospitalId) << "\n";
    oss << "Total Intersections: " << plan.windows.size() << "\n\n";

    for (size_t i = 0; i < plan.windows.size(); ++i) {
        const auto& w = plan.windows[i];
        oss << w.intersectionId << "\n";
        oss << w.command << "\n";
        oss << static_cast<long long>(std::round(w.startTimeSeconds)) << " -> "
            << static_cast<long long>(std::round(w.endTimeSeconds)) << " sec";
        if (i + 1 < plan.windows.size()) {
            oss << "\n\n";
        }
    }
    return oss.str();
}

std::string GreenWaveCoordinator::GetLastLog() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastLog;
}

void GreenWaveCoordinator::PrintLog(const GreenWavePlan& plan) const {
    std::cout << FormatLog(plan) << std::endl;
}

void GreenWaveCoordinator::EnableAutoLogging(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_autoLogging = enable;
}

} // namespace green_wave
} // namespace aegis
