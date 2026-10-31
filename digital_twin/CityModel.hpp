/**
 * @file CityModel.hpp
 * @brief Municipal digital twin state model representing roads, intersections, ambulances, hospitals, and cameras.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <cstdint>

namespace aegis::digital_twin {

struct RoadSegment {
    std::string segmentId;
    std::string fromIntersectionId;
    std::string toIntersectionId;
    double lengthMeters{500.0};
    double speedLimitKmh{50.0};
    uint32_t laneCount{2};
    double currentTrafficDensity{0.1}; // 0.0 - 1.0
    double currentAverageSpeedKmh{48.0};
    double suitabilityScore{95.0};     // 0.0 - 100.0
    std::string suitabilityRating{"OPTIMAL"};
    bool isBlocked{false};
    std::string activeCorridorId;
};

struct IntersectionNode {
    std::string intersectionId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    std::string ipAddress{"127.0.0.1"};
    uint16_t port{9000};
    std::string hardwareType{"Raspberry Pi 4 Model B"};
    std::string signalState{"RED"};       // "RED", "YELLOW", "GREEN", "ALL_RED"
    std::string preemptionState{"NONE"}; // "NONE", "PREEMPTED", "CLEARING", "RESTORED"
    std::string activeCorridorId;
    uint64_t lastHeartbeatMonoMs{0};
    bool isConnected{true};
    bool esp32ControllerConnected{true};
    std::vector<bool> relayStates{false, false, false, false}; // 4 channels
};

struct AmbulanceEntity {
    std::string ambulanceId;
    std::string vehicleType{"ALS"};      // "ALS", "BLS"
    std::string callSign{"MEDIC-1"};
    double latitude{0.0};
    double longitude{0.0};
    double headingDeg{0.0};
    double speedKmh{0.0};
    uint32_t priorityLevel{80};          // 1 - 100
    std::string activeEmergencyId;
    std::string assignedHospitalId;
    std::string currentSegmentId;
    std::string targetIntersectionId;
    std::string status{"IDLE"};          // "IDLE", "DISPATCHED", "EN_ROUTE", "ON_SCENE", "TO_HOSPITAL", "HANDOVER", "CLEAR"
    double etaSeconds{0.0};
    // Patient vitals
    int heartRate{75};
    int systolicBp{120};
    int diastolicBp{80};
    int spO2{98};
};

struct HospitalFacility {
    std::string hospitalId;
    std::string name;
    double latitude{0.0};
    double longitude{0.0};
    uint32_t traumaLevel{1};             // 1 = Comprehensive, 2 = Major, etc.
    uint32_t totalBeds{100};
    uint32_t availableBeds{25};
    uint32_t totalIcuBeds{20};
    uint32_t availableIcuBeds{5};
    double erCapacityPercent{75.0};
    double triageReadinessScore{90.0};   // 0.0 - 100.0
    std::vector<std::string> assignedAmbulanceIds;
    uint32_t incomingPatientsCount{0};
    bool isDiversionActive{false};
};

struct CameraEntity {
    std::string cameraId;
    std::string intersectionId;
    std::vector<std::string> coveredSegmentIds;
    double latitude{0.0};
    double longitude{0.0};
    std::string status{"ACTIVE"};        // "ACTIVE", "STREAMING", "DEGRADED", "OFFLINE"
    double healthScore{0.95};            // 0.0 - 1.0
    double currentFps{30.0};
    uint64_t droppedFrames{0};
    bool isCorridorFeedActive{false};
};

struct RoadConditionEntity {
    std::string eventId;
    std::string incidentId;
    std::string type;                    // "ACCIDENT", "FLOODING", "ROAD_CLOSED", etc.
    std::string roadSegment;
    std::string severity;                // "INFO", "LOW", "MEDIUM", "HIGH", "CRITICAL"
    double confidence{0.90};
    std::string lifecycleState{"ACTIVE"};// "DETECTED", "CONFIRMED", "ACTIVE", "RESOLVING", "CLEARED", "ARCHIVED"
    bool isBlocking{false};
    std::string description;
    uint64_t timestampMonoMs{0};
};

class CityModel {
public:
    CityModel();
    ~CityModel() = default;

    // Road Segments
    void AddRoadSegment(const RoadSegment& segment);
    RoadSegment GetRoadSegment(const std::string& segmentId) const;
    std::vector<RoadSegment> GetAllRoadSegments() const;
    void UpdateSegmentTraffic(const std::string& segmentId, double density, double avgSpeedKmh, double suitability, const std::string& rating, bool isBlocked = false);

    // Intersections
    void AddIntersection(const IntersectionNode& intersection);
    IntersectionNode GetIntersection(const std::string& intersectionId) const;
    std::vector<IntersectionNode> GetAllIntersections() const;
    void UpdateIntersectionSignal(const std::string& intersectionId, const std::string& state, const std::string& preemption = "NONE", const std::string& corridorId = "");
    void UpdateIntersectionHeartbeat(const std::string& intersectionId, uint64_t nowMonoMs);

    // Ambulances
    void AddAmbulance(const AmbulanceEntity& ambulance);
    AmbulanceEntity GetAmbulance(const std::string& ambulanceId) const;
    std::vector<AmbulanceEntity> GetAllAmbulances() const;
    void UpdateAmbulanceTelemetry(const std::string& ambulanceId, double lat, double lon, double speedKmh, const std::string& segmentId, double etaSec, const std::string& status);
    void UpdateAmbulanceVitals(const std::string& ambulanceId, int hr, int sbp, int dbp, int spo2);

    // Hospitals
    void AddHospital(const HospitalFacility& hospital);
    HospitalFacility GetHospital(const std::string& hospitalId) const;
    std::vector<HospitalFacility> GetAllHospitals() const;
    void UpdateHospitalCapacity(const std::string& hospitalId, uint32_t availBeds, uint32_t availIcu, double erCap, bool diversion);
    void AssignAmbulanceToHospital(const std::string& hospitalId, const std::string& ambulanceId);

    // Cameras
    void AddCamera(const CameraEntity& camera);
    CameraEntity GetCamera(const std::string& cameraId) const;
    std::vector<CameraEntity> GetAllCameras() const;
    void UpdateCameraStatus(const std::string& cameraId, const std::string& status, double healthScore, double fps, uint64_t drops, bool isStreaming);

    // Road Conditions / AI Detections
    void AddOrUpdateRoadCondition(const RoadConditionEntity& condition);
    void RemoveRoadCondition(const std::string& eventId);
    std::vector<RoadConditionEntity> GetAllRoadConditions() const;
    std::vector<RoadConditionEntity> GetConditionsForSegment(const std::string& segmentId) const;

    // Reset & Initialization
    void Reset();
    void InitializeDefaultCityLayout();

    // Serialization
    std::string ToJson() const;
    std::string ToSummaryString() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, RoadSegment> roadSegments_;
    std::unordered_map<std::string, IntersectionNode> intersections_;
    std::unordered_map<std::string, AmbulanceEntity> ambulances_;
    std::unordered_map<std::string, HospitalFacility> hospitals_;
    std::unordered_map<std::string, CameraEntity> cameras_;
    std::unordered_map<std::string, RoadConditionEntity> roadConditions_;
};

} // namespace aegis::digital_twin

