/**
 * @file CityModel.cpp
 * @brief Implementation of CityModel digital twin container.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "digital_twin/CityModel.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace aegis::digital_twin {

CityModel::CityModel() {
    InitializeDefaultCityLayout();
}

void CityModel::InitializeDefaultCityLayout() {
    std::lock_guard<std::mutex> lock(mutex_);
    roadSegments_.clear();
    intersections_.clear();
    ambulances_.clear();
    hospitals_.clear();
    cameras_.clear();
    roadConditions_.clear();

    // 1. Intersections
    IntersectionNode intA;
    intA.intersectionId = "INT_A";
    intA.name = "Market & 1st Street";
    intA.latitude = 37.7749;
    intA.longitude = -122.4194;
    intA.ipAddress = "192.168.1.10";
    intA.port = 9001;
    intA.signalState = "GREEN";
    intersections_["INT_A"] = intA;

    IntersectionNode intB;
    intB.intersectionId = "INT_B";
    intB.name = "Mission & 2nd Street";
    intB.latitude = 37.7800;
    intB.longitude = -122.4150;
    intB.ipAddress = "192.168.1.11";
    intB.port = 9002;
    intB.signalState = "RED";
    intersections_["INT_B"] = intB;

    IntersectionNode intC;
    intC.intersectionId = "INT_C";
    intC.name = "Howard & 3rd Street";
    intC.latitude = 37.7850;
    intC.longitude = -122.4100;
    intC.ipAddress = "192.168.1.12";
    intC.port = 9003;
    intC.signalState = "RED";
    intersections_["INT_C"] = intC;

    // 2. Road Segments
    RoadSegment segAB;
    segAB.segmentId = "SEG_INTA_INTB";
    segAB.fromIntersectionId = "INT_A";
    segAB.toIntersectionId = "INT_B";
    segAB.lengthMeters = 600.0;
    segAB.speedLimitKmh = 50.0;
    segAB.laneCount = 2;
    segAB.currentTrafficDensity = 0.20;
    segAB.currentAverageSpeedKmh = 45.0;
    segAB.suitabilityScore = 92.0;
    segAB.suitabilityRating = "OPTIMAL";
    roadSegments_["SEG_INTA_INTB"] = segAB;

    RoadSegment segBC;
    segBC.segmentId = "SEG_INTB_INTC";
    segBC.fromIntersectionId = "INT_B";
    segBC.toIntersectionId = "INT_C";
    segBC.lengthMeters = 750.0;
    segBC.speedLimitKmh = 50.0;
    segBC.laneCount = 2;
    segBC.currentTrafficDensity = 0.25;
    segBC.currentAverageSpeedKmh = 42.0;
    segBC.suitabilityScore = 88.0;
    segBC.suitabilityRating = "OPTIMAL";
    roadSegments_["SEG_INTB_INTC"] = segBC;

    RoadSegment segAC;
    segAC.segmentId = "SEG_INTA_INTC";
    segAC.fromIntersectionId = "INT_A";
    segAC.toIntersectionId = "INT_C";
    segAC.lengthMeters = 1200.0;
    segAC.speedLimitKmh = 60.0;
    segAC.laneCount = 3;
    segAC.currentTrafficDensity = 0.15;
    segAC.currentAverageSpeedKmh = 55.0;
    segAC.suitabilityScore = 95.0;
    segAC.suitabilityRating = "OPTIMAL";
    roadSegments_["SEG_INTA_INTC"] = segAC;

    // 3. Hospitals
    HospitalFacility h1;
    h1.hospitalId = "HOSP_CITY_TRAUMA";
    h1.name = "City General Trauma Center";
    h1.latitude = 37.7900;
    h1.longitude = -122.4050;
    h1.traumaLevel = 1;
    h1.totalBeds = 150;
    h1.availableBeds = 32;
    h1.totalIcuBeds = 25;
    h1.availableIcuBeds = 6;
    h1.erCapacityPercent = 70.0;
    h1.triageReadinessScore = 94.0;
    hospitals_["HOSP_CITY_TRAUMA"] = h1;

    HospitalFacility h2;
    h2.hospitalId = "HOSP_ST_JUDE";
    h2.name = "St. Jude Emergency Hospital";
    h2.latitude = 37.7650;
    h2.longitude = -122.4250;
    h2.traumaLevel = 2;
    h2.totalBeds = 90;
    h2.availableBeds = 20;
    h2.totalIcuBeds = 12;
    h2.availableIcuBeds = 3;
    h2.erCapacityPercent = 65.0;
    h2.triageReadinessScore = 88.0;
    hospitals_["HOSP_ST_JUDE"] = h2;

    // 4. Cameras
    CameraEntity c1;
    c1.cameraId = "CAM_INTA_01";
    c1.intersectionId = "INT_A";
    c1.coveredSegmentIds = {"SEG_INTA_INTB", "SEG_INTA_INTC"};
    c1.latitude = 37.7749;
    c1.longitude = -122.4194;
    c1.status = "ACTIVE";
    c1.healthScore = 0.98;
    c1.currentFps = 30.0;
    cameras_["CAM_INTA_01"] = c1;

    CameraEntity c2;
    c2.cameraId = "CAM_INTB_01";
    c2.intersectionId = "INT_B";
    c2.coveredSegmentIds = {"SEG_INTA_INTB", "SEG_INTB_INTC"};
    c2.latitude = 37.7800;
    c2.longitude = -122.4150;
    c2.status = "ACTIVE";
    c2.healthScore = 0.96;
    c2.currentFps = 30.0;
    cameras_["CAM_INTB_01"] = c2;

    CameraEntity c3;
    c3.cameraId = "CAM_INTC_01";
    c3.intersectionId = "INT_C";
    c3.coveredSegmentIds = {"SEG_INTB_INTC", "SEG_INTA_INTC"};
    c3.latitude = 37.7850;
    c3.longitude = -122.4100;
    c3.status = "ACTIVE";
    c3.healthScore = 0.95;
    c3.currentFps = 30.0;
    cameras_["CAM_INTC_01"] = c3;

    // 5. Default Ambulance
    AmbulanceEntity amb;
    amb.ambulanceId = "AMB_01";
    amb.vehicleType = "ALS";
    amb.callSign = "MEDIC-1";
    amb.latitude = 37.7730;
    amb.longitude = -122.4200;
    amb.speedKmh = 0.0;
    amb.priorityLevel = 90;
    amb.status = "IDLE";
    amb.heartRate = 72;
    amb.systolicBp = 120;
    amb.diastolicBp = 80;
    amb.spO2 = 99;
    ambulances_["AMB_01"] = amb;
}

// Road Segments
void CityModel::AddRoadSegment(const RoadSegment& segment) {
    std::lock_guard<std::mutex> lock(mutex_);
    roadSegments_[segment.segmentId] = segment;
}

RoadSegment CityModel::GetRoadSegment(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = roadSegments_.find(segmentId);
    if (it != roadSegments_.end()) return it->second;
    RoadSegment def;
    def.segmentId = segmentId;
    return def;
}

std::vector<RoadSegment> CityModel::GetAllRoadSegments() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadSegment> list;
    for (const auto& [_, s] : roadSegments_) list.push_back(s);
    return list;
}

void CityModel::UpdateSegmentTraffic(const std::string& segmentId, double density, double avgSpeedKmh, double suitability, const std::string& rating, bool isBlocked) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = roadSegments_.find(segmentId);
    if (it != roadSegments_.end()) {
        it->second.currentTrafficDensity = density;
        it->second.currentAverageSpeedKmh = avgSpeedKmh;
        it->second.suitabilityScore = suitability;
        it->second.suitabilityRating = rating;
        it->second.isBlocked = isBlocked;
    }
}

// Intersections
void CityModel::AddIntersection(const IntersectionNode& intersection) {
    std::lock_guard<std::mutex> lock(mutex_);
    intersections_[intersection.intersectionId] = intersection;
}

IntersectionNode CityModel::GetIntersection(const std::string& intersectionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) return it->second;
    IntersectionNode def;
    def.intersectionId = intersectionId;
    return def;
}

std::vector<IntersectionNode> CityModel::GetAllIntersections() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<IntersectionNode> list;
    for (const auto& [_, i] : intersections_) list.push_back(i);
    return list;
}

void CityModel::UpdateIntersectionSignal(const std::string& intersectionId, const std::string& state, const std::string& preemption, const std::string& corridorId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) {
        it->second.signalState = state;
        it->second.preemptionState = preemption;
        it->second.activeCorridorId = corridorId;
    }
}

void CityModel::UpdateIntersectionHeartbeat(const std::string& intersectionId, uint64_t nowMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = intersections_.find(intersectionId);
    if (it != intersections_.end()) {
        it->second.lastHeartbeatMonoMs = nowMonoMs;
        it->second.isConnected = true;
    }
}

// Ambulances
void CityModel::AddAmbulance(const AmbulanceEntity& ambulance) {
    std::lock_guard<std::mutex> lock(mutex_);
    ambulances_[ambulance.ambulanceId] = ambulance;
}

AmbulanceEntity CityModel::GetAmbulance(const std::string& ambulanceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ambulances_.find(ambulanceId);
    if (it != ambulances_.end()) return it->second;
    AmbulanceEntity def;
    def.ambulanceId = ambulanceId;
    return def;
}

std::vector<AmbulanceEntity> CityModel::GetAllAmbulances() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AmbulanceEntity> list;
    for (const auto& [_, a] : ambulances_) list.push_back(a);
    return list;
}

void CityModel::UpdateAmbulanceTelemetry(const std::string& ambulanceId, double lat, double lon, double speedKmh, const std::string& segmentId, double etaSec, const std::string& status) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ambulances_.find(ambulanceId);
    if (it != ambulances_.end()) {
        it->second.latitude = lat;
        it->second.longitude = lon;
        it->second.speedKmh = speedKmh;
        it->second.currentSegmentId = segmentId;
        it->second.etaSeconds = etaSec;
        it->second.status = status;
    }
}

void CityModel::UpdateAmbulanceVitals(const std::string& ambulanceId, int hr, int sbp, int dbp, int spo2) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = ambulances_.find(ambulanceId);
    if (it != ambulances_.end()) {
        it->second.heartRate = hr;
        it->second.systolicBp = sbp;
        it->second.diastolicBp = dbp;
        it->second.spO2 = spo2;
    }
}

// Hospitals
void CityModel::AddHospital(const HospitalFacility& hospital) {
    std::lock_guard<std::mutex> lock(mutex_);
    hospitals_[hospital.hospitalId] = hospital;
}

HospitalFacility CityModel::GetHospital(const std::string& hospitalId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) return it->second;
    HospitalFacility def;
    def.hospitalId = hospitalId;
    return def;
}

std::vector<HospitalFacility> CityModel::GetAllHospitals() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<HospitalFacility> list;
    for (const auto& [_, h] : hospitals_) list.push_back(h);
    return list;
}

void CityModel::UpdateHospitalCapacity(const std::string& hospitalId, uint32_t availBeds, uint32_t availIcu, double erCap, bool diversion) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) {
        it->second.availableBeds = availBeds;
        it->second.availableIcuBeds = availIcu;
        it->second.erCapacityPercent = erCap;
        it->second.isDiversionActive = diversion;
    }
}

void CityModel::AssignAmbulanceToHospital(const std::string& hospitalId, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hospitals_.find(hospitalId);
    if (it != hospitals_.end()) {
        if (std::find(it->second.assignedAmbulanceIds.begin(), it->second.assignedAmbulanceIds.end(), ambulanceId) == it->second.assignedAmbulanceIds.end()) {
            it->second.assignedAmbulanceIds.push_back(ambulanceId);
            it->second.incomingPatientsCount++;
        }
    }
}

// Cameras
void CityModel::AddCamera(const CameraEntity& camera) {
    std::lock_guard<std::mutex> lock(mutex_);
    cameras_[camera.cameraId] = camera;
}

CameraEntity CityModel::GetCamera(const std::string& cameraId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it != cameras_.end()) return it->second;
    CameraEntity def;
    def.cameraId = cameraId;
    return def;
}

std::vector<CameraEntity> CityModel::GetAllCameras() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<CameraEntity> list;
    for (const auto& [_, c] : cameras_) list.push_back(c);
    return list;
}

void CityModel::UpdateCameraStatus(const std::string& cameraId, const std::string& status, double healthScore, double fps, uint64_t drops, bool isStreaming) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(cameraId);
    if (it != cameras_.end()) {
        it->second.status = status;
        it->second.healthScore = healthScore;
        it->second.currentFps = fps;
        it->second.droppedFrames = drops;
        it->second.isCorridorFeedActive = isStreaming;
    }
}

// Road Conditions
void CityModel::AddOrUpdateRoadCondition(const RoadConditionEntity& condition) {
    std::lock_guard<std::mutex> lock(mutex_);
    roadConditions_[condition.eventId] = condition;
}

void CityModel::RemoveRoadCondition(const std::string& eventId) {
    std::lock_guard<std::mutex> lock(mutex_);
    roadConditions_.erase(eventId);
}

std::vector<RoadConditionEntity> CityModel::GetAllRoadConditions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadConditionEntity> list;
    for (const auto& [_, rc] : roadConditions_) list.push_back(rc);
    return list;
}

std::vector<RoadConditionEntity> CityModel::GetConditionsForSegment(const std::string& segmentId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<RoadConditionEntity> list;
    for (const auto& [_, rc] : roadConditions_) {
        if (rc.roadSegment == segmentId) list.push_back(rc);
    }
    return list;
}

void CityModel::Reset() {
    InitializeDefaultCityLayout();
}

std::string CityModel::ToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\"intersections\":[";
    size_t i = 0;
    for (const auto& [id, node] : intersections_) {
        ss << "{\"id\":\"" << node.intersectionId << "\",\"signal\":\"" << node.signalState
           << "\",\"preemption\":\"" << node.preemptionState << "\",\"corridor\":\"" << node.activeCorridorId << "\"}"
           << (i + 1 < intersections_.size() ? "," : "");
        ++i;
    }
    ss << "],\"roadSegments\":[";
    i = 0;
    for (const auto& [id, seg] : roadSegments_) {
        ss << "{\"id\":\"" << seg.segmentId << "\",\"suitability\":" << std::fixed << std::setprecision(1) << seg.suitabilityScore
           << ",\"rating\":\"" << seg.suitabilityRating << "\",\"blocked\":" << (seg.isBlocked ? "true" : "false") << "}"
           << (i + 1 < roadSegments_.size() ? "," : "");
        ++i;
    }
    ss << "],\"ambulances\":[";
    i = 0;
    for (const auto& [id, amb] : ambulances_) {
        ss << "{\"id\":\"" << amb.ambulanceId << "\",\"status\":\"" << amb.status
           << "\",\"speed\":" << std::fixed << std::setprecision(1) << amb.speedKmh
           << ",\"hr\":" << amb.heartRate << ",\"bp\":\"" << amb.systolicBp << "/" << amb.diastolicBp << "\"}"
           << (i + 1 < ambulances_.size() ? "," : "");
        ++i;
    }
    ss << "],\"hospitals\":[";
    i = 0;
    for (const auto& [id, h] : hospitals_) {
        ss << "{\"id\":\"" << h.hospitalId << "\",\"availBeds\":" << h.availableBeds
           << ",\"availIcu\":" << h.availableIcuBeds << ",\"erCap\":" << std::fixed << std::setprecision(1) << h.erCapacityPercent << "}"
           << (i + 1 < hospitals_.size() ? "," : "");
        ++i;
    }
    ss << "]}";
    return ss.str();
}

std::string CityModel::ToSummaryString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "City Model Summary: Intersections=" << intersections_.size()
       << ", Road Segments=" << roadSegments_.size()
       << ", Ambulances=" << ambulances_.size()
       << ", Hospitals=" << hospitals_.size()
       << ", Cameras=" << cameras_.size()
       << ", Active Conditions=" << roadConditions_.size();
    return ss.str();
}

} // namespace aegis::digital_twin

