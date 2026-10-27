/**
 * @file ManualOverride.cpp
 * @brief Implementation of ManualOverride data formatting and serialization.
 */

#include "control/ManualOverride.h"
#include <iomanip>
#include <sstream>

namespace aegis {
namespace control {

ManualOverride::ManualOverride(ManualOverrideRecord record)
    : m_record(std::move(record)) {}

std::string ManualOverrideRecord::ToString() const {
    std::ostringstream oss;
    oss << "ManualOverride [" << overrideId << "] Action: " << OverrideTypeToString(type)
        << " by " << operatorId << " (Status: " << OverrideStatusToString(status) << ")\n";

    if (!targetAmbulanceId.empty()) {
        oss << "  Target Ambulance : " << targetAmbulanceId << "\n";
        oss << "  Priority Shift   : " << medical::PriorityLevelToString(previousPriority)
            << " -> " << medical::PriorityLevelToString(newPriority) << "\n";
    }
    if (!targetIntersectionId.empty()) {
        oss << "  Target Junction  : " << targetIntersectionId << "\n";
    }
    if (!targetCorridorId.empty()) {
        oss << "  Target Corridor  : " << targetCorridorId << "\n";
    }
    if (!route.empty()) {
        oss << "  Route Stops      : " << route.size() << " [";
        for (size_t i = 0; i < route.size(); ++i) {
            oss << route[i] << (i + 1 < route.size() ? ", " : "");
        }
        oss << "]\n";
    }
    if (durationSeconds > 0) {
        oss << "  Hold Duration    : " << durationSeconds << " secs\n";
    }
    oss << "  Reason           : " << (reason.empty() ? "None provided" : reason) << "\n"
        << "  Timestamp Epoch  : " << timestampEpochMs << " ms";
    return oss.str();
}

std::string ManualOverrideRecord::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"override_id\":\"" << overrideId << "\","
        << "\"operator_id\":\"" << operatorId << "\","
        << "\"type\":\"" << OverrideTypeToString(type) << "\","
        << "\"target_ambulance\":\"" << targetAmbulanceId << "\","
        << "\"target_intersection\":\"" << targetIntersectionId << "\","
        << "\"target_corridor\":\"" << targetCorridorId << "\","
        << "\"previous_priority\":\"" << medical::PriorityLevelToString(previousPriority) << "\","
        << "\"new_priority\":\"" << medical::PriorityLevelToString(newPriority) << "\","
        << "\"duration_seconds\":" << durationSeconds << ","
        << "\"reason\":\"" << reason << "\","
        << "\"timestamp_epoch_ms\":" << timestampEpochMs << ","
        << "\"status\":\"" << OverrideStatusToString(status) << "\","
        << "\"route\":[";

    for (size_t i = 0; i < route.size(); ++i) {
        oss << "\"" << route[i] << "\"";
        if (i + 1 < route.size()) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

} // namespace control
} // namespace aegis
