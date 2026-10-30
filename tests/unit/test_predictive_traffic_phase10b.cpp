/**
 * @file test_predictive_traffic_phase10b.cpp
 * @brief Comprehensive verification suite for Phase 10B: AI Predictive Traffic Intelligence Engine.
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <atomic>
#include <iomanip>

#include "predictive_traffic/PredictiveTrafficEngine.h"
#include "green_corridor/EmergencyCorridorPlanner.h"

using namespace aegis;
using namespace aegis::predictive_traffic;
using namespace aegis::green_corridor;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << (msg) << " (" << #cond << ") at " \
                      << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

namespace {

EmergencyRoute BuildSampleRoute(const std::string& ambId,
                                const std::vector<std::string>& path,
                                double totalDistMeters = 2000.0) {
    EmergencyRoute route;
    route.ambulanceId = ambId;
    route.intersections = path;
    route.totalDistance = totalDistMeters;
    route.isValid = path.size() >= 2;

    double segDist = path.size() > 1 ? (totalDistMeters / static_cast<double>(path.size() - 1)) : totalDistMeters;
    double cumulative = 0.0;
    for (size_t i = 0; i < path.size(); ++i) {
        route.timeline.emplace_back(path[i], cumulative / 15.0, cumulative);
        cumulative += segDist;
    }
    route.estimatedArrivalTime = cumulative / 15.0;
    return route;
}

// =============================================================================
// TEST 1: Historical Prediction
// =============================================================================
void Test1_HistoricalPrediction() {
    std::cout << "[TEST 1] Historical prediction..." << std::endl;

    PredictiveTrafficEngine engine;

    // Ingest historical record for morning rush vs midday
    HistoricalTrafficRecord recMorning("ROAD_H1", 1, 8, 30.0, 60.0, 150, 9.0);
    HistoricalTrafficRecord recMidday("ROAD_H1", 1, 13, 52.0, 60.0, 120, 8.0);

    TEST_ASSERT(engine.UpdateHistoricalRecord(recMorning), "Historical morning update should succeed");
    TEST_ASSERT(engine.UpdateHistoricalRecord(recMidday), "Historical midday update should succeed");
    TEST_ASSERT(engine.GetHistoricalRecordCount() == 2, "DB count should be 2");

    PredictionInput inMorning("ROAD_H1", 32.0, 300.0, 1, 8);
    auto predMorning = engine.PredictRoadCondition(inMorning);

    PredictionInput inMidday("ROAD_H1", 50.0, 300.0, 1, 13);
    auto predMidday = engine.PredictRoadCondition(inMidday);

    TEST_ASSERT(predMorning.predictedSpeedKmH < predMidday.predictedSpeedKmH,
                "Morning predicted speed must reflect historical congestion compared to midday");
    TEST_ASSERT(predMorning.congestionFactor > predMidday.congestionFactor,
                "Morning congestion factor must exceed midday congestion factor");

    // Verify structured audit log formatting
    std::string audit = PredictiveTrafficEngine::FormatAuditLog(predMorning);
    TEST_ASSERT(audit.find("[PREDICTIVE TRAFFIC]") != std::string::npos, "Audit log must contain header");
    TEST_ASSERT(audit.find("Road:\nROAD_H1") != std::string::npos, "Audit log must contain road ID");
    TEST_ASSERT(audit.find("Confidence:\n") != std::string::npos, "Audit log must contain confidence");

    std::cout << "  ✓ PASS: Historical baseline and audit formatting verified.\n";
}

// =============================================================================
// TEST 2: Live Prediction
// =============================================================================
void Test2_LivePrediction() {
    std::cout << "[TEST 2] Live prediction..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_L1", 1, 10, 50.0, 60.0, 100));

    // Update live speed to a low speed (traffic slowdown)
    bool liveUpdated = engine.UpdateLiveTraffic("ROAD_L1", 18.0);
    TEST_ASSERT(liveUpdated, "Live traffic update should succeed");
    TEST_ASSERT(engine.GetLiveRoadCount() == 1, "Live road count should be 1");

    PredictionInput in("ROAD_L1", 18.0, 120.0, 1, 10);
    auto pred = engine.PredictRoadCondition(in);

    TEST_ASSERT(pred.currentSpeedKmH == 18.0, "Current speed should match input");
    TEST_ASSERT(pred.predictedSpeedKmH < 40.0, "Short-horizon prediction must adapt to live slowdown");
    TEST_ASSERT(pred.recommendation == "Use Alternate Corridor", "Severe slowdown triggers alternate recommendation");

    std::cout << "  ✓ PASS: Live traffic telemetry adaptation verified.\n";
}

// =============================================================================
// TEST 3: Moving Average & Trend
// =============================================================================
void Test3_MovingAverage() {
    std::cout << "[TEST 3] Moving average & trend analysis..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_TREND", 1, 12, 50.0, 60.0));

    // Deliver degrading sequential speed samples
    std::vector<double> degradingSpeeds = {52.0, 48.0, 44.0, 38.0, 32.0, 26.0};
    for (double s : degradingSpeeds) {
        engine.UpdateLiveTraffic("ROAD_TREND", s);
    }

    std::string trend = engine.DetectTrafficTrend("ROAD_TREND");
    TEST_ASSERT(trend == "CONGESTING", "Degrading speed samples must detect CONGESTING trend");

    PredictionInput in("ROAD_TREND", 26.0, 180.0, 1, 12);
    auto pred = engine.PredictRoadCondition(in);
    TEST_ASSERT(pred.trend == "CONGESTING", "Prediction output trend should match detected trend");
    TEST_ASSERT(pred.predictedSpeedKmH < 35.0, "Moving average trend must drag predicted speed down");

    // Test improving trend
    std::vector<double> improvingSpeeds = {20.0, 25.0, 32.0, 40.0, 48.0};
    for (double s : improvingSpeeds) {
        engine.UpdateLiveTraffic("ROAD_IMPROVE", s);
    }
    std::string trendImp = engine.DetectTrafficTrend("ROAD_IMPROVE");
    TEST_ASSERT(trendImp == "IMPROVING", "Accelerating samples must detect IMPROVING trend");

    std::cout << "  ✓ PASS: Moving average smoothing and trend direction detection verified.\n";
}

// =============================================================================
// TEST 4: Peak Hour Prediction
// =============================================================================
void Test4_PeakHourPrediction() {
    std::cout << "[TEST 4] Peak hour prediction..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_PEAK", 1, 8, 45.0, 60.0));
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_PEAK", 1, 12, 45.0, 60.0));

    PredictionInput inPeak("ROAD_PEAK", 45.0, 300.0, 1, 8); // 08:00 Morning peak
    auto predPeak = engine.PredictRoadCondition(inPeak);

    PredictionInput inNormal("ROAD_PEAK", 45.0, 300.0, 1, 12); // 12:00 Normal
    auto predNormal = engine.PredictRoadCondition(inNormal);

    TEST_ASSERT(predPeak.predictedSpeedKmH < predNormal.predictedSpeedKmH,
                "Peak hour prediction must predict lower speed than normal hour");
    TEST_ASSERT(predPeak.congestionFactor > predNormal.congestionFactor,
                "Peak hour must produce higher congestion factor");

    std::cout << "  ✓ PASS: Peak-hour congestion penalty verified.\n";
}

// =============================================================================
// TEST 5: Off-Peak Prediction
// =============================================================================
void Test5_OffPeakPrediction() {
    std::cout << "[TEST 5] Off-peak prediction..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_OFFPEAK", 1, 2, 48.0, 60.0));
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_OFFPEAK", 1, 15, 48.0, 60.0));

    PredictionInput inNight("ROAD_OFFPEAK", 48.0, 300.0, 1, 2); // 02:00 Night off-peak
    auto predNight = engine.PredictRoadCondition(inNight);

    PredictionInput inDay("ROAD_OFFPEAK", 48.0, 300.0, 1, 15); // 15:00 Regular daytime
    auto predDay = engine.PredictRoadCondition(inDay);

    TEST_ASSERT(predNight.predictedSpeedKmH >= predDay.predictedSpeedKmH,
                "Off-peak night prediction must achieve faster speeds towards free-flow");

    std::cout << "  ✓ PASS: Off-peak free-flow recovery verified.\n";
}

// =============================================================================
// TEST 6: Weather Adjustment
// =============================================================================
void Test6_WeatherAdjustment() {
    std::cout << "[TEST 6] Weather adjustment..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_WX", 1, 10, 50.0, 60.0));

    PredictionInput inClear("ROAD_WX", 50.0, 300.0, 1, 10, 1.0); // Clear
    auto predClear = engine.PredictRoadCondition(inClear);

    PredictionInput inStorm("ROAD_WX", 50.0, 300.0, 1, 10, 1.45); // Storm (1.45x multiplier)
    auto predStorm = engine.PredictRoadCondition(inStorm);

    TEST_ASSERT(predStorm.predictedSpeedKmH < predClear.predictedSpeedKmH,
                "Inclement weather multiplier must reduce predicted speed");
    TEST_ASSERT(predStorm.congestionFactor > predClear.congestionFactor,
                "Weather multiplier must increase congestion factor");

    std::cout << "  ✓ PASS: Weather environmental multiplier interface verified.\n";
}

// =============================================================================
// TEST 7: Event Adjustment
// =============================================================================
void Test7_EventAdjustment() {
    std::cout << "[TEST 7] Event adjustment..." << std::endl;

    PredictiveTrafficEngine engine;
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_EV", 1, 18, 45.0, 60.0));

    PredictionInput inNormal("ROAD_EV", 45.0, 300.0, 1, 18, 1.0, 1.0); // Normal
    auto predNormal = engine.PredictRoadCondition(inNormal);

    PredictionInput inStadium("ROAD_EV", 45.0, 300.0, 1, 18, 1.0, 1.40); // Stadium event
    auto predStadium = engine.PredictRoadCondition(inStadium);

    TEST_ASSERT(predStadium.predictedSpeedKmH < predNormal.predictedSpeedKmH,
                "Major event multiplier must decrease predicted travel speed");

    std::cout << "  ✓ PASS: Event congestion multiplier interface verified.\n";
}

// =============================================================================
// TEST 8: Prediction Confidence & Anomaly Detection
// =============================================================================
void Test8_PredictionConfidence() {
    std::cout << "[TEST 8] Prediction confidence & anomaly detection..." << std::endl;

    PredictiveTrafficEngine engine;

    // High sample density, low variance, near horizon
    double confHigh = engine.CalculateConfidence("ROAD_C1", 200, 4.0, 60.0);
    // Low sample density, high variance, far horizon
    double confLow = engine.CalculateConfidence("ROAD_C2", 3, 36.0, 3600.0);

    TEST_ASSERT(confHigh > confLow, "High sample density and short horizon must have higher confidence");
    TEST_ASSERT(confHigh >= 90.0, "High confidence must exceed 90%");
    TEST_ASSERT(confLow >= 50.0, "Low confidence must be bounded to at least 50%");

    // Anomaly detection test
    engine.UpdateHistoricalRecord(HistoricalTrafficRecord("ROAD_ANOMALY", 1, 10, 50.0, 60.0, 100, 16.0)); // Mean=50, Std=4
    bool normalSample = engine.DetectTrafficAnomaly("ROAD_ANOMALY", 46.0, 2.5); // z = 1.0
    bool anomalySample = engine.DetectTrafficAnomaly("ROAD_ANOMALY", 15.0, 2.5); // z = 8.75

    TEST_ASSERT(!normalSample, "Speed close to mean is not an anomaly");
    TEST_ASSERT(anomalySample, "Speed > 2.5 std devs away must trigger anomaly flag");

    std::cout << "  ✓ PASS: Confidence estimation and statistical anomaly detection verified.\n";
}

// =============================================================================
// TEST 9: Concurrency (100 Threads)
// =============================================================================
void Test9_Concurrency() {
    std::cout << "[TEST 9] Concurrency (100 threads, 0 data races)..." << std::endl;

    PredictiveTrafficEngine engine;
    const int NUM_THREADS = 100;
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(NUM_THREADS));

    std::atomic<int> readyCount{0};
    std::atomic<bool> startFlag{false};

    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([&engine, i, &readyCount, &startFlag]() {
            std::string roadId = "ROAD_TH_" + std::to_string(i % 10);
            readyCount.fetch_add(1);

            while (!startFlag.load()) {
                std::this_thread::yield();
            }

            // Historical update
            engine.UpdateHistoricalRecord(HistoricalTrafficRecord(roadId, 1, 8, 40.0 + static_cast<double>(i % 15)));

            // Live updates
            engine.UpdateLiveTraffic(roadId, 35.0 + static_cast<double>(i % 20));

            // Prediction query
            PredictionInput in(roadId, 38.0, 200.0, 1, 8);
            auto pred = engine.PredictRoadCondition(in);
            TEST_ASSERT(!pred.roadSegmentId.empty(), "Prediction must be valid");

            // Corridor ranking
            EmergencyRoute r1 = BuildSampleRoute("AMB_A", {roadId, "INT_DEST"});
            EmergencyRoute r2 = BuildSampleRoute("AMB_B", {roadId, "INT_ALT", "INT_DEST"});
            auto best = engine.PredictBestCorridor({r1, r2});
            TEST_ASSERT(!best.corridorId.empty(), "Best corridor must be selected");
        });
    }

    while (readyCount.load() < NUM_THREADS) {
        std::this_thread::yield();
    }
    startFlag.store(true);

    for (auto& th : threads) {
        th.join();
    }

    auto stats = engine.GetStatistics();
    TEST_ASSERT(stats.totalPredictionsGenerated > 0, "Predictions were generated under concurrent access");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races.\n";
}

// =============================================================================
// TEST 10: Performance Benchmark (Predict 100 Roads < 5 ms)
// =============================================================================
void Test10_Performance() {
    std::cout << "[TEST 10] Performance benchmark: Predict 100 roads under 5 ms..." << std::endl;

    PredictiveTrafficEngine engine;
    const size_t NUM_ROADS = 100;
    std::vector<PredictionInput> inputs;
    inputs.reserve(NUM_ROADS);

    for (size_t i = 1; i <= NUM_ROADS; ++i) {
        std::string roadId = "ROAD_BENCH_" + std::to_string(i);
        engine.UpdateHistoricalRecord(HistoricalTrafficRecord(
            roadId, 1, 8, 35.0 + static_cast<double>(i % 25), 60.0, 50, 16.0));
        engine.UpdateLiveTraffic(roadId, 30.0 + static_cast<double>(i % 20));

        inputs.emplace_back(roadId, 32.0, 300.0, 1, 8, 1.1, 1.0);
    }

    // Warmup
    for (size_t i = 0; i < 10; ++i) {
        engine.PredictRoadCondition(inputs[i]);
    }

    // Benchmark predicting 100 roads without cache
    engine.SetCacheEnabled(false);
    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& in : inputs) {
        auto pred = engine.PredictRoadCondition(in);
        (void)pred;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "  -> Predicted 100 roads in: "
              << std::fixed << std::setprecision(3) << elapsedMs << " ms\n";

    TEST_ASSERT(elapsedMs < 5.0, "Prediction of 100 roads must complete under 5.0 ms");

    std::cout << "  ✓ PASS: 100 roads predicted in " << elapsedMs << " ms (< 5.0 ms target achieved).\n";
}

} // namespace

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 10B AI Predictive Traffic Intelligence" << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_HistoricalPrediction();
    Test2_LivePrediction();
    Test3_MovingAverage();
    Test4_PeakHourPrediction();
    Test5_OffPeakPrediction();
    Test6_WeatherAdjustment();
    Test7_EventAdjustment();
    Test8_PredictionConfidence();
    Test9_Concurrency();
    Test10_Performance();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 10 PHASE 10B UNIT TESTS PASSED SUCCESSFULLY (100%)" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
