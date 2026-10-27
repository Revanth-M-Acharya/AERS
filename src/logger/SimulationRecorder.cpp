/**
 * @file SimulationRecorder.cpp
 * @brief Implementation of Simulation and Mission Playback Timeline Recorder.
 */

#include "logger/SimulationRecorder.h"
#include <chrono>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace aegis {
namespace logger {

static std::string EscapeJsonString(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 8);
    for (char c : input) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c; break;
        }
    }
    return out;
}

std::string SimulationFrame::ToJson() const {
    std::ostringstream oss;
    oss << "{"
        << "\"frameIndex\":" << frameIndex << ","
        << "\"simulationTimeMs\":" << simulationTimeMs << ","
        << "\"note\":\"" << EscapeJsonString(note) << "\","
        << "\"vehiclePositions\":{";

    size_t vIdx = 0;
    for (const auto& [amb, pos] : vehiclePositions) {
        oss << "\"" << EscapeJsonString(amb) << "\":\"" << EscapeJsonString(pos) << "\"";
        if (++vIdx < vehiclePositions.size()) oss << ",";
    }

    oss << "},\"signalStates\":{";
    size_t sIdx = 0;
    for (const auto& [junction, state] : signalStates) {
        oss << "\"" << EscapeJsonString(junction) << "\":\"" << EscapeJsonString(state) << "\"";
        if (++sIdx < signalStates.size()) oss << ",";
    }

    oss << "},\"activeCorridors\":[";
    for (size_t i = 0; i < activeCorridors.size(); ++i) {
        oss << "\"" << EscapeJsonString(activeCorridors[i]) << "\"";
        if (i + 1 < activeCorridors.size()) oss << ",";
    }

    oss << "],\"frameEvents\":[";
    for (size_t i = 0; i < frameEvents.size(); ++i) {
        oss << frameEvents[i].ToJson();
        if (i + 1 < frameEvents.size()) oss << ",";
    }
    oss << "]}";

    return oss.str();
}

SimulationRecorder::SimulationRecorder() = default;

SimulationRecorder::SimulationRecorder(std::string simulationId)
    : m_simulationId(std::move(simulationId)) {}

SimulationRecorder::~SimulationRecorder() {
    DetachFromLogger();
}

void SimulationRecorder::StartRecording(const std::string& simulationId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!simulationId.empty()) {
        m_simulationId = simulationId;
    }
    m_events.clear();
    m_frames.clear();
    m_frameCounter = 0;
    m_startTimeEpochMs = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    m_stopTimeEpochMs = 0;
    m_isRecording = true;
}

void SimulationRecorder::StopRecording() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRecording) {
        m_stopTimeEpochMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        m_isRecording = false;
    }
}

bool SimulationRecorder::IsRecording() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_isRecording;
}

const std::string& SimulationRecorder::GetSimulationId() const noexcept {
    return m_simulationId;
}

uint64_t SimulationRecorder::GetStartTimeEpochMs() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_startTimeEpochMs;
}

uint64_t SimulationRecorder::GetStopTimeEpochMs() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stopTimeEpochMs;
}

uint64_t SimulationRecorder::GetDurationMs() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_startTimeEpochMs == 0) return 0;
    if (m_isRecording) {
        uint64_t nowMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        return nowMs >= m_startTimeEpochMs ? (nowMs - m_startTimeEpochMs) : 0;
    }
    return m_stopTimeEpochMs >= m_startTimeEpochMs ? (m_stopTimeEpochMs - m_startTimeEpochMs) : 0;
}

void SimulationRecorder::RecordEvent(const SystemEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRecording) {
        m_events.push_back(event);
    }
}

void SimulationRecorder::RecordFrame(SimulationFrame frame) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isRecording) {
        if (frame.frameIndex == 0) {
            frame.frameIndex = ++m_frameCounter;
        }
        m_frames.push_back(std::move(frame));
    }
}

void SimulationRecorder::RecordSnapshot(uint64_t simulationTimeMs,
                                       const std::string& note,
                                       const std::unordered_map<std::string, std::string>& vehiclePositions,
                                       const std::unordered_map<std::string, std::string>& signalStates,
                                       const std::vector<std::string>& activeCorridors) {
    SimulationFrame frame;
    frame.simulationTimeMs = simulationTimeMs;
    frame.note = note;
    frame.vehiclePositions = vehiclePositions;
    frame.signalStates = signalStates;
    frame.activeCorridors = activeCorridors;
    RecordFrame(std::move(frame));
}

void SimulationRecorder::AttachToLogger(EventLogger& logger) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_attachedLogger = &logger;
    logger.RegisterListener([this](const SystemEvent& evt) {
        RecordEvent(evt);
    });
}

void SimulationRecorder::DetachFromLogger() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_attachedLogger = nullptr;
}

std::vector<SystemEvent> SimulationRecorder::GetRecordedEvents() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::vector<SimulationFrame> SimulationRecorder::GetRecordedFrames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frames;
}

size_t SimulationRecorder::GetEventCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events.size();
}

size_t SimulationRecorder::GetFrameCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frames.size();
}

std::string SimulationRecorder::ExportRecordingJson() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ostringstream oss;
    oss << "{"
        << "\"simulationId\":\"" << EscapeJsonString(m_simulationId) << "\","
        << "\"isRecording\":" << (m_isRecording ? "true" : "false") << ","
        << "\"startTimeEpochMs\":" << m_startTimeEpochMs << ","
        << "\"stopTimeEpochMs\":" << m_stopTimeEpochMs << ","
        << "\"durationMs\":" << (m_stopTimeEpochMs >= m_startTimeEpochMs ? (m_stopTimeEpochMs - m_startTimeEpochMs) : 0) << ","
        << "\"eventCount\":" << m_events.size() << ","
        << "\"frameCount\":" << m_frames.size() << ","
        << "\"events\":[";

    for (size_t i = 0; i < m_events.size(); ++i) {
        oss << m_events[i].ToJson();
        if (i + 1 < m_events.size()) oss << ",";
    }

    oss << "],\"frames\":[";
    for (size_t i = 0; i < m_frames.size(); ++i) {
        oss << m_frames[i].ToJson();
        if (i + 1 < m_frames.size()) oss << ",";
    }
    oss << "]}";

    return oss.str();
}

bool SimulationRecorder::ExportRecordingToFile(const std::string& filePath) const {
    std::string jsonStr = ExportRecordingJson();
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }
    outFile << jsonStr;
    outFile.close();
    return true;
}

void SimulationRecorder::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.clear();
    m_frames.clear();
    m_frameCounter = 0;
    m_startTimeEpochMs = 0;
    m_stopTimeEpochMs = 0;
    m_isRecording = false;
}

} // namespace logger
} // namespace aegis
