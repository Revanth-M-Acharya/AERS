/**
 * @file SimulationRecorder.h
 * @brief Simulation and Mission Playback Timeline Recorder for SMART AEGIS.
 */

#ifndef AEGIS_LOGGER_SIMULATION_RECORDER_H
#define AEGIS_LOGGER_SIMULATION_RECORDER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

#include "logger/EventLogger.h"

namespace aegis {
namespace logger {

/**
 * @brief Discrete temporal snapshot frame in a simulation session.
 */
struct SimulationFrame {
    uint64_t frameIndex{0};
    uint64_t simulationTimeMs{0};
    std::string note{""};
    std::unordered_map<std::string, std::string> vehiclePositions{}; ///< ambId -> intersectionId or coordinates
    std::unordered_map<std::string, std::string> signalStates{};     ///< intersectionId -> RED / YELLOW / GREEN
    std::vector<std::string> activeCorridors{};
    std::vector<SystemEvent> frameEvents{};

    std::string ToJson() const;
};

/**
 * @brief Comprehensive recorder capturing time-series state and events for post-incident playback and simulation analytics.
 */
class SimulationRecorder {
public:
    SimulationRecorder();
    explicit SimulationRecorder(std::string simulationId);
    ~SimulationRecorder();

    // Session Management
    void StartRecording(const std::string& simulationId = "");
    void StopRecording();
    bool IsRecording() const noexcept;

    const std::string& GetSimulationId() const noexcept;
    uint64_t GetStartTimeEpochMs() const noexcept;
    uint64_t GetStopTimeEpochMs() const noexcept;
    uint64_t GetDurationMs() const noexcept;

    // Recording API
    void RecordEvent(const SystemEvent& event);
    void RecordFrame(SimulationFrame frame);
    void RecordSnapshot(uint64_t simulationTimeMs,
                        const std::string& note = "",
                        const std::unordered_map<std::string, std::string>& vehiclePositions = {},
                        const std::unordered_map<std::string, std::string>& signalStates = {},
                        const std::vector<std::string>& activeCorridors = {});

    // Integration with EventLogger
    void AttachToLogger(EventLogger& logger);
    void DetachFromLogger();

    // Inspection & Queries
    std::vector<SystemEvent> GetRecordedEvents() const;
    std::vector<SimulationFrame> GetRecordedFrames() const;
    size_t GetEventCount() const;
    size_t GetFrameCount() const;

    // Exporting
    std::string ExportRecordingJson() const;
    bool ExportRecordingToFile(const std::string& filePath) const;

    void Reset();

private:
    mutable std::mutex m_mutex;
    std::string m_simulationId{"SIM_DEFAULT"};
    bool m_isRecording{false};
    uint64_t m_startTimeEpochMs{0};
    uint64_t m_stopTimeEpochMs{0};
    uint64_t m_frameCounter{0};

    std::vector<SystemEvent> m_events{};
    std::vector<SimulationFrame> m_frames{};
    EventLogger* m_attachedLogger{nullptr};
};

} // namespace logger
} // namespace aegis

#endif // AEGIS_LOGGER_SIMULATION_RECORDER_H
