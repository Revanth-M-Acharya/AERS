/**
 * @file PredictiveTrafficEngine.h
 * @brief AI Predictive Traffic Intelligence Engine for SMART AEGIS (Phase 10B).
 *
 * Implements deterministic traffic prediction algorithms suitable for safety-critical QNX systems.
 * Predicts future traffic congestion, road speeds, and corridor ETAs before congestion occurs,
 * allowing proactive corridor selection, Green Wave coordination, and dynamic route replanning.
 */

#ifndef AEGIS_PREDICTIVE_TRAFFIC_ENGINE_H
#define AEGIS_PREDICTIVE_TRAFFIC_ENGINE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <memory>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <iostream>

#include "green_corridor/EmergencyCorridorPlanner.h"
#include "green_wave/GreenWaveCoordinator.h"
#include "replanning/DynamicRouteReplanner.h"

namespace aegis {
namespace predictive_traffic {

// Upstream type aliases
using EmergencyRoute = aegis::green_corridor::EmergencyRoute;
using RoadSegment = aegis::green_corridor::RoadSegment;
using TrafficCondition = aegis::replanning::TrafficCondition;

// =============================================================================
// Data Models
// =============================================================================

/**
 * @brief Historical statistical profile of traffic on a specific road segment.
 */
struct HistoricalTrafficRecord {
    std::string roadSegmentId{""};
    uint32_t dayOfWeek{0};       ///< 0 = Sunday, 1 = Monday, ..., 6 = Saturday
    uint32_t hourOfDay{0};       ///< 0 to 23
    double averageSpeedKmH{50.0};
    double freeFlowSpeedKmH{60.0};
    uint32_t sampleCount{100};
    double variance{16.0};       ///< Speed variance (sigma^2)
    uint64_t lastUpdatedMs{0};

    HistoricalTrafficRecord() = default;
    HistoricalTrafficRecord(std::string roadId,
                            uint32_t day,
                            uint32_t hour,
                            double avgSpeed,
                            double freeFlow = 60.0,
                            uint32_t count = 100,
                            double var = 16.0,
                            uint64_t ts = 0)
        : roadSegmentId(std::move(roadId))
        , dayOfWeek(day)
        , hourOfDay(hour)
        , averageSpeedKmH(avgSpeed)
        , freeFlowSpeedKmH(freeFlow)
        , sampleCount(count)
        , variance(var)
        , lastUpdatedMs(ts) {}

    bool operator==(const HistoricalTrafficRecord& other) const noexcept {
        return roadSegmentId == other.roadSegmentId &&
               dayOfWeek == other.dayOfWeek &&
               hourOfDay == other.hourOfDay;
    }
};

/**
 * @brief Environmental and contextual parameters input into a road prediction query.
 */
struct PredictionInput {
    std::string roadSegmentId{""};
    double currentSpeedKmH{50.0};
    double futureLookaheadSeconds{300.0}; ///< Target lookahead horizon (seconds)
    uint32_t dayOfWeek{1};                ///< Day of week (0-6)
    uint32_t hourOfDay{8};                ///< Hour of day (0-23)
    double weatherMultiplier{1.0};        ///< 1.0 = Clear, 1.25 = Rain, 1.5 = Storm
    double eventMultiplier{1.0};          ///< 1.0 = Normal, 1.3 = Stadium/Concert
    double historicalWeight{0.4};
    double trendWeight{0.3};
    double recentSamplesWeight{0.3};

    PredictionInput() = default;
    PredictionInput(std::string roadId,
                    double currentSpeed,
                    double lookahead = 300.0,
                    uint32_t day = 1,
                    uint32_t hour = 8,
                    double weatherMult = 1.0,
                    double eventMult = 1.0,
                    double histWeight = 0.4,
                    double trWeight = 0.3,
                    double recentWeight = 0.3)
        : roadSegmentId(std::move(roadId))
        , currentSpeedKmH(currentSpeed)
        , futureLookaheadSeconds(lookahead)
        , dayOfWeek(day)
        , hourOfDay(hour)
        , weatherMultiplier(weatherMult)
        , eventMultiplier(eventMult)
        , historicalWeight(histWeight)
        , trendWeight(trWeight)
        , recentSamplesWeight(recentWeight) {}
};

/**
 * @brief Forward-looking traffic prediction for a discrete road segment.
 */
struct TrafficPrediction {
    std::string roadSegmentId{""};
    double currentSpeedKmH{0.0};
    double predictedSpeedKmH{0.0};
    double freeFlowSpeedKmH{60.0};
    double congestionFactor{0.0};         ///< 0.0 = Free flow, 1.0 = Gridlock
    double predictionWindowSeconds{300.0};
    double confidenceScore{95.0};         ///< 0.0% to 100.0%
    std::string trend{"STABLE"};          ///< "IMPROVING", "STABLE", "CONGESTING"
    bool isAnomaly{false};
    std::string recommendation{"Optimal Corridor"};
    uint64_t timestamp{0};

    TrafficPrediction() = default;
    TrafficPrediction(std::string roadId,
                      double currentSpeed,
                      double predictedSpeed,
                      double freeFlow,
                      double congestion,
                      double windowSec,
                      double confidence,
                      std::string tr,
                      bool anomaly,
                      std::string rec,
                      uint64_t ts)
        : roadSegmentId(std::move(roadId))
        , currentSpeedKmH(currentSpeed)
        , predictedSpeedKmH(predictedSpeed)
        , freeFlowSpeedKmH(freeFlow)
        , congestionFactor(congestion)
        , predictionWindowSeconds(windowSec)
        , confidenceScore(confidence)
        , trend(std::move(tr))
        , isAnomaly(anomaly)
        , recommendation(std::move(rec))
        , timestamp(ts) {}
};

/**
 * @brief Concise representation of predicted road physical condition.
 */
struct PredictedRoadCondition {
    std::string roadSegmentId{""};
    double predictedSpeedKmH{0.0};
    double predictedTravelTimeSeconds{0.0};
    std::string congestionLevel{"LOW"};   ///< "LOW", "MODERATE", "HEAVY", "SEVERE"
    bool isBlocked{false};
    double confidence{95.0};

    PredictedRoadCondition() = default;
    PredictedRoadCondition(std::string roadId,
                           double speed,
                           double travelTime,
                           std::string level,
                           bool blocked = false,
                           double conf = 95.0)
        : roadSegmentId(std::move(roadId))
        , predictedSpeedKmH(speed)
        , predictedTravelTimeSeconds(travelTime)
        , congestionLevel(std::move(level))
        , isBlocked(blocked)
        , confidence(conf) {}
};

/**
 * @brief Comprehensive prediction and scoring outcome across an entire route corridor.
 */
struct PredictionResult {
    std::string corridorId{""};
    double predictedTotalEtaSeconds{0.0};
    double averagePredictedSpeedKmH{0.0};
    double compositeConfidenceScore{95.0};
    std::vector<TrafficPrediction> roadPredictions{};
    double corridorScore{0.0};            ///< Lower is superior (composite cost)
    std::string recommendation{"Optimal Corridor"};
    uint64_t timestamp{0};

    PredictionResult() = default;
    PredictionResult(std::string id,
                     double eta,
                     double avgSpeed,
                     double confidence,
                     std::vector<TrafficPrediction> predictions,
                     double score,
                     std::string rec,
                     uint64_t ts)
        : corridorId(std::move(id))
        , predictedTotalEtaSeconds(eta)
        , averagePredictedSpeedKmH(avgSpeed)
        , compositeConfidenceScore(confidence)
        , roadPredictions(std::move(predictions))
        , corridorScore(score)
        , recommendation(std::move(rec))
        , timestamp(ts) {}
};

/**
 * @brief Runtime operational telemetry and caching metrics.
 */
struct PredictionStatistics {
    uint64_t totalPredictionsGenerated{0};
    uint64_t anomaliesDetected{0};
    uint64_t cacheHits{0};
    uint64_t cacheMisses{0};
    double averageLatencyMs{0.0};
};

/**
 * @brief In-memory cache holding temporal prediction records.
 */
struct PredictionCacheEntry {
    TrafficPrediction prediction{};
    uint64_t expiryTimestampMs{0};
};

// =============================================================================
// Engine Class
// =============================================================================

/**
 * @class PredictiveTrafficEngine
 * @brief Thread-safe deterministic AI predictive traffic intelligence engine.
 */
class PredictiveTrafficEngine {
public:
    PredictiveTrafficEngine();
    ~PredictiveTrafficEngine() = default;

    // Non-copyable, movable
    PredictiveTrafficEngine(const PredictiveTrafficEngine&) = delete;
    PredictiveTrafficEngine& operator=(const PredictiveTrafficEngine&) = delete;
    PredictiveTrafficEngine(PredictiveTrafficEngine&&) noexcept;
    PredictiveTrafficEngine& operator=(PredictiveTrafficEngine&&) noexcept;

    // =========================================================================
    // Ingestion APIs
    // =========================================================================

    /**
     * @brief Ingest or update historical traffic statistics for a road segment.
     */
    bool UpdateHistoricalRecord(const HistoricalTrafficRecord& record);

    /**
     * @brief Ingest live speed telemetry from edge sensors or connected vehicle feeds.
     */
    bool UpdateLiveTraffic(const std::string& roadSegmentId,
                           double currentSpeedKmH,
                           uint64_t timestampMs = 0);

    // =========================================================================
    // Prediction APIs
    // =========================================================================

    /**
     * @brief Predict future traffic condition for a discrete road segment using full inputs.
     */
    TrafficPrediction PredictRoadCondition(const PredictionInput& input);

    /**
     * @brief Convenient overload predicting road condition with default environmental parameters.
     */
    TrafficPrediction PredictRoadCondition(const std::string& roadSegmentId,
                                           double futureLookaheadSeconds = 300.0);

    /**
     * @brief Predict future traffic speeds and arrival times across an entire EmergencyRoute.
     */
    PredictionResult PredictRoute(const EmergencyRoute& route,
                                  double futureLookaheadSeconds = 300.0,
                                  double weatherMultiplier = 1.0,
                                  double eventMultiplier = 1.0);

    /**
     * @brief Estimate overall future travel time (ETA in seconds) for an EmergencyRoute.
     */
    double PredictETA(const EmergencyRoute& route,
                      double weatherMultiplier = 1.0,
                      double eventMultiplier = 1.0);

    /**
     * @brief Select and return the single optimal corridor among competing candidate routes.
     */
    PredictionResult PredictBestCorridor(const std::vector<EmergencyRoute>& candidateRoutes,
                                         double weatherMultiplier = 1.0,
                                         double eventMultiplier = 1.0);

    /**
     * @brief Rank all candidate emergency corridors from most optimal to least optimal.
     */
    std::vector<PredictionResult> RankCorridors(const std::vector<EmergencyRoute>& candidateRoutes,
                                                double weatherMultiplier = 1.0,
                                                double eventMultiplier = 1.0);

    // =========================================================================
    // Analytics & Detection APIs
    // =========================================================================

    /**
     * @brief Calculate prediction confidence score based on sample density, variance, and horizon.
     */
    double CalculateConfidence(const std::string& roadSegmentId,
                               uint32_t sampleCount,
                               double variance,
                               double timeHorizonSeconds) const;

    /**
     * @brief Analyze moving average trend to classify trajectory ("IMPROVING", "STABLE", "CONGESTING").
     */
    std::string DetectTrafficTrend(const std::string& roadSegmentId) const;

    /**
     * @brief Detect whether current speed represents a statistical outlier/anomaly.
     */
    bool DetectTrafficAnomaly(const std::string& roadSegmentId,
                              double currentSpeedKmH,
                              double zScoreThreshold = 2.5) const;

    // =========================================================================
    // Reporting & History APIs
    // =========================================================================

    /**
     * @brief Generate a formatted human-readable summary report for a corridor prediction.
     */
    std::string GeneratePredictionReport(const PredictionResult& result) const;

    /**
     * @brief Retrieve snapshot of prediction history.
     */
    std::vector<TrafficPrediction> GetPredictionHistory() const;

    /**
     * @brief Clear historical prediction logs.
     */
    void ClearHistory();

    /**
     * @brief Clear all internal databases (historical records, live samples, history, cache).
     */
    void ClearAll();

    // =========================================================================
    // Audit Logging
    // =========================================================================

    /**
     * @brief Format single road prediction into exact SMART AEGIS specification audit format.
     */
    static std::string FormatAuditLog(const TrafficPrediction& prediction);

    // =========================================================================
    // Configuration & Telemetry
    // =========================================================================

    void SetCacheEnabled(bool enabled);
    bool IsCacheEnabled() const;
    void ClearCache();

    PredictionStatistics GetStatistics() const;
    size_t GetHistoricalRecordCount() const;
    size_t GetLiveRoadCount() const;

    void SetDefaultFreeFlowSpeed(double speedKmH);
    double GetDefaultFreeFlowSpeed() const;

private:
    uint64_t CurrentTimeMs() const;
    std::string MakeHistKey(const std::string& roadId, uint32_t day, uint32_t hour) const;

    mutable std::mutex m_mutex;

    // In-memory historical records: key = "roadId_day_hour"
    std::unordered_map<std::string, HistoricalTrafficRecord> m_historicalDb;

    // Recent live samples for trend and moving average calculation (up to 10 per road)
    std::unordered_map<std::string, std::deque<double>> m_liveSpeedHistory;
    std::unordered_map<std::string, double> m_latestSpeed;

    // Prediction cache: key = "roadId_window"
    std::unordered_map<std::string, PredictionCacheEntry> m_cache;
    bool m_cacheEnabled{true};
    uint64_t m_cacheTtlMs{5000}; // 5-second TTL

    std::vector<TrafficPrediction> m_predictionHistory;
    PredictionStatistics m_statistics{};

    double m_defaultFreeFlowSpeedKmH{60.0};
};

} // namespace predictive_traffic
} // namespace aegis

#endif // AEGIS_PREDICTIVE_TRAFFIC_ENGINE_H
