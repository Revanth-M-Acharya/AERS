/**
 * @file PredictiveTrafficEngine.cpp
 * @brief Implementation of AI Predictive Traffic Intelligence Engine (Phase 10B).
 */

#include "PredictiveTrafficEngine.h"

namespace aegis {
namespace predictive_traffic {

PredictiveTrafficEngine::PredictiveTrafficEngine() = default;

PredictiveTrafficEngine::PredictiveTrafficEngine(PredictiveTrafficEngine&& other) noexcept {
    std::unique_lock<std::mutex> lock(other.m_mutex);
    m_historicalDb = std::move(other.m_historicalDb);
    m_liveSpeedHistory = std::move(other.m_liveSpeedHistory);
    m_latestSpeed = std::move(other.m_latestSpeed);
    m_cache = std::move(other.m_cache);
    m_cacheEnabled = other.m_cacheEnabled;
    m_cacheTtlMs = other.m_cacheTtlMs;
    m_predictionHistory = std::move(other.m_predictionHistory);
    m_statistics = other.m_statistics;
    m_defaultFreeFlowSpeedKmH = other.m_defaultFreeFlowSpeedKmH;
}

PredictiveTrafficEngine& PredictiveTrafficEngine::operator=(PredictiveTrafficEngine&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_historicalDb = std::move(other.m_historicalDb);
        m_liveSpeedHistory = std::move(other.m_liveSpeedHistory);
        m_latestSpeed = std::move(other.m_latestSpeed);
        m_cache = std::move(other.m_cache);
        m_cacheEnabled = other.m_cacheEnabled;
        m_cacheTtlMs = other.m_cacheTtlMs;
        m_predictionHistory = std::move(other.m_predictionHistory);
        m_statistics = other.m_statistics;
        m_defaultFreeFlowSpeedKmH = other.m_defaultFreeFlowSpeedKmH;
    }
    return *this;
}

uint64_t PredictiveTrafficEngine::CurrentTimeMs() const {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string PredictiveTrafficEngine::MakeHistKey(const std::string& roadId, uint32_t day, uint32_t hour) const {
    return roadId + "_" + std::to_string(day % 7) + "_" + std::to_string(hour % 24);
}

// =============================================================================
// Ingestion APIs
// =============================================================================

bool PredictiveTrafficEngine::UpdateHistoricalRecord(const HistoricalTrafficRecord& record) {
    if (record.roadSegmentId.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string key = MakeHistKey(record.roadSegmentId, record.dayOfWeek, record.hourOfDay);
    HistoricalTrafficRecord rec = record;
    rec.lastUpdatedMs = CurrentTimeMs();
    m_historicalDb[key] = std::move(rec);
    return true;
}

bool PredictiveTrafficEngine::UpdateLiveTraffic(const std::string& roadSegmentId,
                                               double currentSpeedKmH,
                                               uint64_t timestampMs) {
    if (roadSegmentId.empty() || currentSpeedKmH < 0.0) {
        return false;
    }

    const uint64_t now = (timestampMs == 0) ? CurrentTimeMs() : timestampMs;
    std::lock_guard<std::mutex> lock(m_mutex);

    m_latestSpeed[roadSegmentId] = currentSpeedKmH;

    auto& history = m_liveSpeedHistory[roadSegmentId];
    history.push_back(currentSpeedKmH);
    if (history.size() > 10) {
        history.pop_front();
    }

    // Invalidate stale cache entries for this road
    if (m_cacheEnabled) {
        for (auto it = m_cache.begin(); it != m_cache.end();) {
            if (it->first.rfind(roadSegmentId, 0) == 0) {
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    (void)now;
    return true;
}

// =============================================================================
// Prediction APIs
// =============================================================================

TrafficPrediction PredictiveTrafficEngine::PredictRoadCondition(const PredictionInput& input) {
    const uint64_t now = CurrentTimeMs();
    const std::string cacheKey = input.roadSegmentId + "_" +
        std::to_string(input.dayOfWeek % 7) + "_" +
        std::to_string(input.hourOfDay % 24) + "_" +
        std::to_string(static_cast<int>(input.futureLookaheadSeconds)) + "_" +
        std::to_string(static_cast<int>(input.weatherMultiplier * 100.0)) + "_" +
        std::to_string(static_cast<int>(input.eventMultiplier * 100.0));

    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Check cache
    if (m_cacheEnabled) {
        auto cIt = m_cache.find(cacheKey);
        if (cIt != m_cache.end() && now <= cIt->second.expiryTimestampMs) {
            m_statistics.cacheHits++;
            return cIt->second.prediction;
        }
        m_statistics.cacheMisses++;
    }

    // 2. Rule 1: Retrieve historical baseline
    std::string histKey = MakeHistKey(input.roadSegmentId, input.dayOfWeek, input.hourOfDay);
    double histSpeed = input.currentSpeedKmH;
    double freeFlowSpeed = m_defaultFreeFlowSpeedKmH;
    uint32_t sampleCount = 20;
    double variance = 16.0;

    auto hIt = m_historicalDb.find(histKey);
    if (hIt != m_historicalDb.end()) {
        histSpeed = hIt->second.averageSpeedKmH;
        freeFlowSpeed = hIt->second.freeFlowSpeedKmH;
        sampleCount = hIt->second.sampleCount;
        variance = hIt->second.variance;
    } else {
        // Fallback: search any historical entry for this road
        for (const auto& [k, rec] : m_historicalDb) {
            if (rec.roadSegmentId == input.roadSegmentId) {
                freeFlowSpeed = rec.freeFlowSpeedKmH;
                histSpeed = rec.averageSpeedKmH;
                sampleCount = rec.sampleCount;
                variance = rec.variance;
                break;
            }
        }
    }

    // 3. Rule 2 & 3: Moving average and trend
    double movingAvgSpeed = input.currentSpeedKmH;
    double trendRate = 0.0;
    std::string trend = "STABLE";

    auto lIt = m_liveSpeedHistory.find(input.roadSegmentId);
    if (lIt != m_liveSpeedHistory.end() && !lIt->second.empty()) {
        const auto& samples = lIt->second;
        double sum = 0.0;
        for (double s : samples) {
            sum += s;
        }
        movingAvgSpeed = sum / static_cast<double>(samples.size());

        if (samples.size() >= 2) {
            double delta = samples.back() - samples.front();
            trendRate = delta / static_cast<double>(samples.size());
            if (delta < -2.0) {
                trend = "CONGESTING";
            } else if (delta > 2.0) {
                trend = "IMPROVING";
            }
        }
    }

    // 4. Rule 4: Weighted recent samples & horizon blending
    double horizonRatio = std::min(1.0, std::max(0.0, input.futureLookaheadSeconds / 1800.0));
    double effectiveLiveWeight = (1.0 - horizonRatio) * (input.trendWeight + input.recentSamplesWeight);
    double effectiveHistWeight = 1.0 - effectiveLiveWeight;

    double liveComponent = 0.5 * movingAvgSpeed + 0.5 * input.currentSpeedKmH + (trendRate * (input.futureLookaheadSeconds / 60.0));
    double rawPredictedSpeed = effectiveHistWeight * histSpeed + effectiveLiveWeight * liveComponent;

    // 5. Rule 5 & 6: Time-of-day peak and off-peak adjustments
    double todFactor = 1.0;
    uint32_t hour = input.hourOfDay % 24;
    if (hour >= 7 && hour <= 9) {
        // Morning rush
        todFactor = 0.75;
    } else if (hour >= 16 && hour <= 19) {
        // Evening rush
        todFactor = 0.70;
    } else if ((hour >= 22 && hour <= 23) || (hour <= 5)) {
        // Night off-peak: light traffic allowing higher speeds towards free-flow
        todFactor = 1.15;
    }
    rawPredictedSpeed *= todFactor;

    // 6. Rule 7 & 8: Environmental multipliers (weather & event)
    double weatherM = std::max(1.0, input.weatherMultiplier);
    double eventM = std::max(1.0, input.eventMultiplier);
    rawPredictedSpeed /= (weatherM * eventM);

    // Bound speed physically
    double predictedSpeed = std::clamp(rawPredictedSpeed, 5.0, freeFlowSpeed);

    // 7. Congestion factor & Recommendation
    double congestionFactor = std::clamp(1.0 - (predictedSpeed / freeFlowSpeed), 0.0, 1.0);
    std::string recommendation = "Optimal Corridor";
    if (congestionFactor >= 0.45 || predictedSpeed <= 25.0) {
        recommendation = "Use Alternate Corridor";
    } else if (congestionFactor >= 0.25) {
        recommendation = "Caution: Moderate Congestion Ahead";
    }

    // 8. Confidence calculation
    double confidence = CalculateConfidence(input.roadSegmentId, sampleCount, variance, input.futureLookaheadSeconds);

    // 9. Anomaly detection
    bool isAnomaly = DetectTrafficAnomaly(input.roadSegmentId, input.currentSpeedKmH, 2.5);
    if (isAnomaly) {
        m_statistics.anomaliesDetected++;
    }

    TrafficPrediction pred(
        input.roadSegmentId,
        input.currentSpeedKmH,
        predictedSpeed,
        freeFlowSpeed,
        congestionFactor,
        input.futureLookaheadSeconds,
        confidence,
        trend,
        isAnomaly,
        recommendation,
        now);

    // Cache entry
    if (m_cacheEnabled) {
        PredictionCacheEntry entry;
        entry.prediction = pred;
        entry.expiryTimestampMs = now + m_cacheTtlMs;
        m_cache[cacheKey] = std::move(entry);
    }

    m_predictionHistory.push_back(pred);
    m_statistics.totalPredictionsGenerated++;

    return pred;
}

TrafficPrediction PredictiveTrafficEngine::PredictRoadCondition(const std::string& roadSegmentId,
                                                               double futureLookaheadSeconds) {
    double currentSpeed = 50.0;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_latestSpeed.find(roadSegmentId);
        if (it != m_latestSpeed.end()) {
            currentSpeed = it->second;
        }
    }

    PredictionInput input;
    input.roadSegmentId = roadSegmentId;
    input.currentSpeedKmH = currentSpeed;
    input.futureLookaheadSeconds = futureLookaheadSeconds;
    input.dayOfWeek = 1; // Default weekday
    input.hourOfDay = 8; // Default morning
    return PredictRoadCondition(input);
}

// =============================================================================
// Route & Corridor Prediction
// =============================================================================

PredictionResult PredictiveTrafficEngine::PredictRoute(const EmergencyRoute& route,
                                                      double futureLookaheadSeconds,
                                                      double weatherMultiplier,
                                                      double eventMultiplier) {
    const uint64_t now = CurrentTimeMs();
    std::vector<TrafficPrediction> roadPreds;

    if (route.intersections.size() < 2) {
        return PredictionResult(
            route.ambulanceId, route.estimatedArrivalTime, 50.0, 95.0, {}, 0.0, "Optimal Corridor", now);
    }

    roadPreds.reserve(route.intersections.size() - 1);
    double totalPredictedEta = 0.0;
    double speedSum = 0.0;
    double confidenceSum = 0.0;
    double congestionSum = 0.0;

    double cumulativeTimeOffset = 0.0;
    for (size_t i = 0; i + 1 < route.intersections.size(); ++i) {
        std::string roadId = route.intersections[i] + "->" + route.intersections[i + 1];

        // Segment distance estimate
        double distMeters = 500.0;
        if (i < route.timeline.size() && (i + 1) < route.timeline.size()) {
            distMeters = std::max(50.0, route.timeline[i + 1].cumulativeDistanceMeters - route.timeline[i].cumulativeDistanceMeters);
        } else if (route.totalDistance > 0.0) {
            distMeters = route.totalDistance / static_cast<double>(route.intersections.size() - 1);
        }

        // Segment lookahead accounts for progress along route
        double segLookahead = futureLookaheadSeconds + cumulativeTimeOffset;

        PredictionInput in;
        in.roadSegmentId = roadId;
        in.futureLookaheadSeconds = segLookahead;
        in.weatherMultiplier = weatherMultiplier;
        in.eventMultiplier = eventMultiplier;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_latestSpeed.find(roadId);
            in.currentSpeedKmH = (it != m_latestSpeed.end()) ? it->second : 50.0;
        }

        TrafficPrediction pred = PredictRoadCondition(in);
        roadPreds.push_back(pred);

        double speedMS = std::max(1.0, pred.predictedSpeedKmH * (1000.0 / 3600.0));
        double segTravelTime = distMeters / speedMS;
        totalPredictedEta += segTravelTime;
        cumulativeTimeOffset += segTravelTime;

        speedSum += pred.predictedSpeedKmH;
        confidenceSum += pred.confidenceScore;
        congestionSum += pred.congestionFactor;
    }

    size_t count = roadPreds.size();
    double avgSpeed = count > 0 ? (speedSum / static_cast<double>(count)) : 50.0;
    double avgConf = count > 0 ? (confidenceSum / static_cast<double>(count)) : 95.0;
    double avgCongestion = count > 0 ? (congestionSum / static_cast<double>(count)) : 0.0;

    // Rule 10: Corridor scoring formula: composite cost
    double confRatio = std::max(0.5, avgConf / 100.0);
    double corridorScore = totalPredictedEta * (1.0 + avgCongestion) / confRatio;

    std::string rec = "Optimal Corridor";
    if (avgCongestion >= 0.50) {
        rec = "Use Alternate Corridor";
    } else if (avgCongestion >= 0.30) {
        rec = "Caution: Moderate Congestion Ahead";
    }

    return PredictionResult(
        route.ambulanceId,
        totalPredictedEta,
        avgSpeed,
        avgConf,
        std::move(roadPreds),
        corridorScore,
        rec,
        now);
}

double PredictiveTrafficEngine::PredictETA(const EmergencyRoute& route,
                                          double weatherMultiplier,
                                          double eventMultiplier) {
    auto res = PredictRoute(route, 300.0, weatherMultiplier, eventMultiplier);
    return res.predictedTotalEtaSeconds;
}

PredictionResult PredictiveTrafficEngine::PredictBestCorridor(
    const std::vector<EmergencyRoute>& candidateRoutes,
    double weatherMultiplier,
    double eventMultiplier) {
    auto ranked = RankCorridors(candidateRoutes, weatherMultiplier, eventMultiplier);
    if (!ranked.empty()) {
        return ranked.front();
    }
    return PredictionResult();
}

std::vector<PredictionResult> PredictiveTrafficEngine::RankCorridors(
    const std::vector<EmergencyRoute>& candidateRoutes,
    double weatherMultiplier,
    double eventMultiplier) {
    std::vector<PredictionResult> results;
    results.reserve(candidateRoutes.size());

    for (const auto& route : candidateRoutes) {
        results.push_back(PredictRoute(route, 300.0, weatherMultiplier, eventMultiplier));
    }

    // Sort ascending by corridor score (lowest score is best)
    std::sort(results.begin(), results.end(),
        [](const PredictionResult& a, const PredictionResult& b) {
            return a.corridorScore < b.corridorScore;
        });

    return results;
}

// =============================================================================
// Analytics & Detection APIs
// =============================================================================

double PredictiveTrafficEngine::CalculateConfidence(const std::string& roadSegmentId,
                                                    uint32_t sampleCount,
                                                    double variance,
                                                    double timeHorizonSeconds) const {
    (void)roadSegmentId;
    // Base confidence from sample density: 20+ samples yields 90%+
    double sampleWeight = std::min(1.0, static_cast<double>(sampleCount) / 20.0);
    double baseConf = 75.0 + (sampleWeight * 20.0); // 75% to 95%

    // Variance penalty
    double varPenalty = std::min(15.0, std::max(0.0, (variance - 9.0) * 0.5));
    baseConf -= varPenalty;

    // Horizon degradation: farther into future slightly degrades confidence
    double horizonDecay = std::min(15.0, (timeHorizonSeconds / 3600.0) * 15.0);
    baseConf -= horizonDecay;

    return std::clamp(std::round(baseConf), 50.0, 99.0);
}

std::string PredictiveTrafficEngine::DetectTrafficTrend(const std::string& roadSegmentId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_liveSpeedHistory.find(roadSegmentId);
    if (it == m_liveSpeedHistory.end() || it->second.size() < 2) {
        return "STABLE";
    }

    double delta = it->second.back() - it->second.front();
    if (delta < -2.0) {
        return "CONGESTING";
    } else if (delta > 2.0) {
        return "IMPROVING";
    }
    return "STABLE";
}

bool PredictiveTrafficEngine::DetectTrafficAnomaly(const std::string& roadSegmentId,
                                                  double currentSpeedKmH,
                                                  double zScoreThreshold) const {
    // Check historical mean and variance for road
    double histMean = 50.0;
    double histVar = 16.0;
    bool found = false;

    for (const auto& [k, rec] : m_historicalDb) {
        if (rec.roadSegmentId == roadSegmentId) {
            histMean = rec.averageSpeedKmH;
            histVar = rec.variance;
            found = true;
            break;
        }
    }

    if (!found) {
        return false;
    }

    double stdDev = std::sqrt(std::max(1.0, histVar));
    double zScore = std::abs(currentSpeedKmH - histMean) / stdDev;
    return zScore >= zScoreThreshold;
}

// =============================================================================
// Reporting & History APIs
// =============================================================================

std::string PredictiveTrafficEngine::GeneratePredictionReport(const PredictionResult& result) const {
    std::ostringstream oss;
    oss << "==========================================================\n";
    oss << " SMART AEGIS: Corridor Traffic Prediction Report\n";
    oss << "==========================================================\n";
    oss << "Corridor: " << result.corridorId << "\n";
    oss << std::fixed << std::setprecision(1);
    oss << "Predicted ETA: " << result.predictedTotalEtaSeconds << " sec\n";
    oss << "Average Predicted Speed: " << result.averagePredictedSpeedKmH << " km/h\n";
    oss << "Confidence: " << result.compositeConfidenceScore << "%\n";
    oss << "Recommendation: " << result.recommendation << "\n";
    oss << "Corridor Cost Score: " << result.corridorScore << "\n";
    oss << "Segments Analyzed: " << result.roadPredictions.size() << "\n";
    oss << "==========================================================\n";
    return oss.str();
}

std::vector<TrafficPrediction> PredictiveTrafficEngine::GetPredictionHistory() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_predictionHistory;
}

void PredictiveTrafficEngine::ClearHistory() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_predictionHistory.clear();
}

void PredictiveTrafficEngine::ClearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_historicalDb.clear();
    m_liveSpeedHistory.clear();
    m_latestSpeed.clear();
    m_cache.clear();
    m_predictionHistory.clear();
    m_statistics = PredictionStatistics{};
}

// =============================================================================
// Audit Logging
// =============================================================================

std::string PredictiveTrafficEngine::FormatAuditLog(const TrafficPrediction& pred) {
    std::ostringstream oss;
    oss << "[PREDICTIVE TRAFFIC]\n\n";
    oss << "Road:\n" << pred.roadSegmentId << "\n\n";
    oss << "Current Speed:\n" << static_cast<int>(std::round(pred.currentSpeedKmH)) << " km/h\n\n";
    oss << "Predicted Speed:\n" << static_cast<int>(std::round(pred.predictedSpeedKmH)) << " km/h\n\n";
    oss << "Prediction Window:\n" << static_cast<int>(std::round(pred.predictionWindowSeconds)) << " sec\n\n";
    oss << "Confidence:\n" << static_cast<int>(std::round(pred.confidenceScore)) << "%\n\n";
    oss << "Recommendation:\n" << pred.recommendation;
    return oss.str();
}

// =============================================================================
// Configuration & Telemetry
// =============================================================================

void PredictiveTrafficEngine::SetCacheEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cacheEnabled = enabled;
}

bool PredictiveTrafficEngine::IsCacheEnabled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cacheEnabled;
}

void PredictiveTrafficEngine::ClearCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

PredictionStatistics PredictiveTrafficEngine::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_statistics;
}

size_t PredictiveTrafficEngine::GetHistoricalRecordCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_historicalDb.size();
}

size_t PredictiveTrafficEngine::GetLiveRoadCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latestSpeed.size();
}

void PredictiveTrafficEngine::SetDefaultFreeFlowSpeed(double speedKmH) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_defaultFreeFlowSpeedKmH = speedKmH;
}

double PredictiveTrafficEngine::GetDefaultFreeFlowSpeed() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_defaultFreeFlowSpeedKmH;
}

} // namespace predictive_traffic
} // namespace aegis
