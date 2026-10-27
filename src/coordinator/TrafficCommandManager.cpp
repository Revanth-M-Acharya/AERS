/**
 * @file TrafficCommandManager.cpp
 * @brief Implementation of TrafficCommandManager command factory.
 */

#include "TrafficCommandManager.h"
#include <sstream>

namespace aegis {
namespace coordinator {

namespace {

std::string BuildPayload(const std::string& cmd,
                         const std::string& intersectionId,
                         const std::string& extraKey = "",
                         const std::string& extraVal = "",
                         bool extraIsNum = false) {
    std::ostringstream ss;
    ss << "{"
       << "\"command\":\"" << cmd << "\","
       << "\"intersectionId\":\"" << intersectionId << "\"";
    if (!extraKey.empty() && !extraVal.empty()) {
        ss << ",\"" << extraKey << "\":";
        if (extraIsNum) {
            ss << extraVal;
        } else {
            ss << "\"" << extraVal << "\"";
        }
    }
    ss << "}";
    return ss.str();
}

bool ExtractField(const std::string& json, const std::string& key, std::string& outVal) {
    std::string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) {
        // Try without string quotes
        search = "\"" + key + "\":";
        pos = json.find(search);
        if (pos == std::string::npos) return false;

        size_t start = pos + search.size();
        size_t end = json.find_first_of(",}", start);
        if (end == std::string::npos) return false;
        outVal = json.substr(start, end - start);
        return true;
    }

    size_t start = pos + search.size();
    size_t end = json.find('"', start);
    if (end == std::string::npos) return false;

    outVal = json.substr(start, end - start);
    return true;
}

} // anonymous namespace

TrafficCommandManager::TrafficCommandManager(std::string coordinatorId)
    : m_coordinatorId(std::move(coordinatorId)) {
}

communication::Message TrafficCommandManager::CreateGreenCommand(const std::string& intersectionId,
                                                                 uint32_t durationSec) const {
    std::string payload = BuildPayload(TrafficCommands::SET_GREEN,
                                       intersectionId,
                                       "duration",
                                       std::to_string(durationSec),
                                       true);
    return communication::Message(communication::MessageType::COMMAND,
                                  m_coordinatorId,
                                  std::move(payload),
                                  communication::Message::CurrentTimestampMs());
}

communication::Message TrafficCommandManager::CreateRedCommand(const std::string& intersectionId) const {
    std::string payload = BuildPayload(TrafficCommands::SET_RED, intersectionId);
    return communication::Message(communication::MessageType::COMMAND,
                                  m_coordinatorId,
                                  std::move(payload),
                                  communication::Message::CurrentTimestampMs());
}

communication::Message TrafficCommandManager::CreateNormalCommand(const std::string& intersectionId) const {
    std::string payload = BuildPayload(TrafficCommands::SET_NORMAL, intersectionId);
    return communication::Message(communication::MessageType::COMMAND,
                                  m_coordinatorId,
                                  std::move(payload),
                                  communication::Message::CurrentTimestampMs());
}

communication::Message TrafficCommandManager::CreateEmergencyOn(const std::string& intersectionId,
                                                                const std::string& corridorId) const {
    std::string payload = BuildPayload(TrafficCommands::EMERGENCY_MODE_ON,
                                       intersectionId,
                                       "corridorId",
                                       corridorId,
                                       false);
    return communication::Message(communication::MessageType::COMMAND,
                                  m_coordinatorId,
                                  std::move(payload),
                                  communication::Message::CurrentTimestampMs());
}

communication::Message TrafficCommandManager::CreateEmergencyOff(const std::string& intersectionId,
                                                                 const std::string& corridorId) const {
    std::string payload = BuildPayload(TrafficCommands::EMERGENCY_MODE_OFF,
                                       intersectionId,
                                       "corridorId",
                                       corridorId,
                                       false);
    return communication::Message(communication::MessageType::COMMAND,
                                  m_coordinatorId,
                                  std::move(payload),
                                  communication::Message::CurrentTimestampMs());
}

bool TrafficCommandManager::ParseCommand(const communication::Message& message,
                                         std::string& outCommand,
                                         std::string& outIntersectionId) {
    if (message.GetType() != communication::MessageType::COMMAND) {
        return false;
    }

    const std::string& payload = message.GetPayload();
    if (!ExtractField(payload, "command", outCommand)) {
        return false;
    }
    if (!ExtractField(payload, "intersectionId", outIntersectionId)) {
        return false;
    }

    return true;
}

} // namespace coordinator
} // namespace aegis
