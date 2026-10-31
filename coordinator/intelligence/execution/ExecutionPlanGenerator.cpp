/**
 * @file ExecutionPlanGenerator.cpp
 * @brief Implementation of ExecutionPlanGenerator.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/execution/ExecutionPlanGenerator.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include "common/utils/TimeUtils.hpp"
#include <sstream>

namespace aegis::coordinator::intelligence {

namespace {

uint16_t ComputeCrc16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i]);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

} // namespace

std::string ExecutionPlan::SerializeToString() const {
    std::ostringstream oss;
    oss << planId << "|" << ambulanceId << "|" << priorityLevel << "|" << timestampMs << "|";
    for (const auto& inter : orderedIntersections) {
        oss << inter << ",";
    }
    oss << "|";
    for (const auto& cmd : signalCommands) {
        oss << cmd.intersectionId << ":" << cmd.signalCommand << ":"
            << cmd.activationTimeMonoMs << ":" << cmd.holdDurationMs << ":"
            << cmd.releaseCondition << ";";
    }
    oss << "|" << rollbackPlan;
    return oss.str();
}

uint16_t ExecutionPlan::ComputeChecksum() const {
    std::string serialized = SerializeToString();
    return ComputeCrc16(reinterpret_cast<const uint8_t*>(serialized.data()), serialized.size());
}

bool ExecutionPlan::VerifyChecksum() const {
    return (checksum == ComputeChecksum());
}

ExecutionPlan ExecutionPlanGenerator::CreateExecutionPlan(const CorridorPlan& corridor, uint32_t priorityLevel) {
    ExecutionPlan plan;
    plan.planId = corridor.planId;
    plan.ambulanceId = corridor.ambulanceId;
    plan.priorityLevel = priorityLevel;
    plan.timestampMs = common::TimeUtils::GetCurrentTimestampMs();
    plan.rollbackPlan = corridor.rollbackMode;

    for (const auto& item : corridor.schedule) {
        plan.orderedIntersections.push_back(item.intersectionId);
        plan.signalCommands.push_back(item);
    }

    plan.checksum = plan.ComputeChecksum();

    CoordinatorLogger::Instance().Info("EXECUTION", "Generated Execution Plan " + plan.planId +
                                       " [Priority: " + std::to_string(priorityLevel) +
                                       ", Intersections: " + std::to_string(plan.orderedIntersections.size()) +
                                       ", Checksum: 0x" + std::to_string(plan.checksum) + "]");
    return plan;
}

} // namespace aegis::coordinator::intelligence

