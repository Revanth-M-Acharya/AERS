/**
 * @file FaultInjector.cpp
 * @brief Implementation of fault injection engine.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/fault_injection/FaultInjector.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::production {

FaultInjector::FaultInjector() = default;

void FaultInjector::ArmFault(const std::string& faultId, FaultType type, const std::string& target, double probability, uint32_t delayMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    FaultDescriptor f;
    f.faultId = faultId;
    f.type = type;
    f.targetComponentId = target;
    f.probability = (probability < 0.0) ? 0.0 : ((probability > 1.0) ? 1.0 : probability);
    f.delayMs = delayMs;
    f.isActive = true;
    f.triggerCount = 0;
    faults_[faultId] = f;
}

void FaultInjector::DisarmFault(const std::string& faultId) {
    std::lock_guard<std::mutex> lock(mutex_);
    faults_.erase(faultId);
}

void FaultInjector::DisarmAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    faults_.clear();
    pressureBuffer_.clear();
}

bool FaultInjector::ShouldDropPacket(const std::string& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, f] : faults_) {
        if (f.isActive && f.type == FaultType::PACKET_LOSS) {
            if (f.targetComponentId.empty() || f.targetComponentId == target) {
                f.triggerCount++;
                totalTriggeredCount_++;
                return true;
            }
        }
    }
    return false;
}

uint32_t FaultInjector::GetInjectedDelayMs(const std::string& target) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, f] : faults_) {
        if (f.isActive && f.type == FaultType::PACKET_DELAY) {
            if (f.targetComponentId.empty() || f.targetComponentId == target) {
                f.triggerCount++;
                totalTriggeredCount_++;
                return f.delayMs;
            }
        }
    }
    return 0;
}

bool FaultInjector::IsNodeFaulted(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, f] : faults_) {
        if (f.isActive && (f.targetComponentId.empty() || f.targetComponentId == nodeId)) {
            if (f.type == FaultType::NODE_FAILURE ||
                f.type == FaultType::TRAFFIC_NODE_OFFLINE ||
                f.type == FaultType::HOSPITAL_OFFLINE ||
                f.type == FaultType::CAMERA_FAILURE) {
                return true;
            }
        }
    }
    return false;
}

void FaultInjector::SimulateCpuOverload(uint32_t durationMs) {
    uint64_t start = common::TimeUtils::GetMonotonicTimeMs();
    volatile uint64_t burn = 0;
    while (common::TimeUtils::GetMonotonicTimeMs() - start < durationMs) {
        for (int i = 0; i < 1000; ++i) {
            burn += static_cast<uint64_t>(i);
        }
    }
    (void)burn;
}

void FaultInjector::SimulateMemoryPressure(size_t pressureBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    pressureBuffer_.assign(pressureBytes, 0xAA);
}

size_t FaultInjector::GetActiveFaultCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : faults_) {
        if (v.isActive) count++;
    }
    return count;
}

uint64_t FaultInjector::GetTotalTriggeredCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalTriggeredCount_;
}

std::string FaultInjector::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"active_faults_count\": " << faults_.size() << ",\n"
       << "  \"total_triggered\": " << totalTriggeredCount_ << ",\n"
       << "  \"faults\": [\n";

    bool first = true;
    for (const auto& [id, f] : faults_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    {\n"
           << "      \"fault_id\": \"" << f.faultId << "\",\n"
           << "      \"type\": \"" << FaultTypeToString(f.type) << "\",\n"
           << "      \"target\": \"" << f.targetComponentId << "\",\n"
           << "      \"probability\": " << f.probability << ",\n"
           << "      \"delay_ms\": " << f.delayMs << ",\n"
           << "      \"active\": " << (f.isActive ? "true" : "false") << ",\n"
           << "      \"triggered\": " << f.triggerCount << "\n"
           << "    }";
    }
    ss << "\n  ]\n}";
    return ss.str();
}

} // namespace aegis::production

