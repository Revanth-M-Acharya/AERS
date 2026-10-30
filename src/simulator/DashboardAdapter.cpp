/**
 * @file DashboardAdapter.cpp
 * @brief SMART AEGIS (AERS) Phase 19 - Event-Driven Digital Twin Dashboard Publisher Adapter Implementation.
 */

#include "DashboardAdapter.h"

namespace aegis {
namespace simulator {

DashboardAdapter::DashboardAdapter() = default;

void DashboardAdapter::SetControlCentreDataModel(std::shared_ptr<dashboard::DashboardDataModel> model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_controlCentreModel = std::move(model);
}

void DashboardAdapter::SetTrafficDashboardDataModel(std::shared_ptr<dashboard::traffic::TrafficDashboardDataModel> model) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_trafficModel = std::move(model);
}

void DashboardAdapter::PublishSnapshot(const CityModel& city, double elapsedSimSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Publish to Phase 17 Control Centre Dashboard
    if (m_controlCentreModel) {
        // Intersections
        for (const auto& inter : city.GetAllIntersections()) {
            dashboard::IntersectionRecord rec;
            rec.intersectionId = inter->GetId();
            rec.nodeId = inter->GetId();
            rec.name = inter->GetName();
            rec.latitude = inter->GetLatitude();
            rec.longitude = inter->GetLongitude();
            rec.signalState = inter->GetSignalState();
            rec.currentPhase = SignalPhaseStateToString(inter->GetCurrentPhase());
            rec.countdownSec = inter->GetCountdownSec();
            rec.isPreempted = inter->IsPreempted();
            rec.activeCorridorId = inter->GetActiveCorridorId();
            rec.irSensorActive = inter->IsApproachIrTriggered() || inter->IsDepartureIrTriggered();
            m_controlCentreModel->UpdateIntersection(rec);
        }

        // Fleet Ambulances
        for (const auto& amb : city.GetAllAmbulances()) {
            dashboard::FleetVehicleRecord fRec;
            fRec.vehicleId = amb->GetId();
            fRec.latitude = amb->GetLatitude();
            fRec.longitude = amb->GetLongitude();
            fRec.speedKmh = amb->GetSpeedKmh();
            fRec.headingDeg = amb->GetHeadingDeg();
            fRec.status = AmbulanceLifecycleToString(amb->GetLifecycle());
            fRec.statusStr = fRec.status;
            fRec.assignedHospitalId = amb->GetAssignedHospitalId();
            fRec.assignedIncidentId = amb->GetCurrentIncidentId();
            fRec.transitEtaSec = amb->GetEtaSeconds();
            fRec.hasPatientOnboard = amb->HasPatientOnboard();

            auto v = amb->GetVitals();
            fRec.patientVitals.heartRateBpm = v.heartRateBpm;
            fRec.patientVitals.spo2Pct = v.spo2Pct;
            fRec.patientVitals.bloodPressureSystolic = v.systolicBP;
            fRec.patientVitals.bloodPressureDiastolic = v.diastolicBP;
            fRec.patientVitals.gcsScore = v.gcsScore;
            m_controlCentreModel->UpdateFleetVehicle(fRec);
        }

        // Hospitals
        for (const auto& hosp : city.GetAllHospitals()) {
            dashboard::HospitalCapacityRecord hRec;
            hRec.hospitalId = hosp->GetId();
            hRec.name = hosp->GetName();
            hRec.latitude = hosp->GetLatitude();
            hRec.longitude = hosp->GetLongitude();
            hRec.totalIcuBeds = hosp->GetTotalIcuBeds();
            hRec.availableIcuBeds = hosp->GetAvailableIcuBeds();
            hRec.totalEdBeds = hosp->GetTotalEdBeds();
            hRec.availableEdBeds = hosp->GetAvailableEdBeds();
            hRec.isDiverting = hosp->IsDiverting();
            m_controlCentreModel->UpdateHospital(hRec);
        }

        // City Metrics
        auto metrics = m_controlCentreModel->GetCityMetrics();
        metrics.activeCorridorsCount = 0;
        metrics.preemptedIntersectionsCount = 0;
        for (const auto& inter : city.GetAllIntersections()) {
            if (inter->IsPreempted()) {
                metrics.preemptedIntersectionsCount++;
                metrics.activeCorridorsCount = 1;
            }
        }
        if (metrics.preemptedIntersectionsCount > 0) {
            metrics.cityConditionStr = "CRITICAL";
        } else {
            metrics.cityConditionStr = "NORMAL";
        }
        m_controlCentreModel->SetCityMetrics(metrics);
    }

    // 2. Publish to Phase 18 Traffic Operations Dashboard
    if (m_trafficModel) {
        dashboard::traffic::GreenCorridorSchedule sched;
        sched.activeCorridorId = "";

        for (const auto& inter : city.GetAllIntersections()) {
            dashboard::traffic::IntersectionTelemetry t;
            t.nodeId = inter->GetId();
            t.name = inter->GetName();
            t.latitude = inter->GetLatitude();
            t.longitude = inter->GetLongitude();
            t.signalState = inter->GetSignalState();
            t.currentPhase = SignalPhaseStateToString(inter->GetCurrentPhase());
            t.countdownSec = inter->GetCountdownSec();
            t.isPreempted = inter->IsPreempted();
            t.activeCorridorId = inter->GetActiveCorridorId();
            t.isOnline = true;
            t.isReady = true;
            t.lastHeartbeatSec = 0.1;
            t.latencyMs = 0.4;
            t.packetLossPct = 0.0;
            t.cpuPercent = 14.5;
            t.memPercent = 31.0;
            t.tempCelsius = 41.5;
            t.gpioSignalState = t.signalState;
            t.activeGpioPins = {true, true, false, false, false, false, false, false};
            m_trafficModel->UpdateIntersection(t);

            if (t.isPreempted && sched.activeCorridorId.empty()) {
                sched.activeCorridorId = t.activeCorridorId;
                sched.corridorDirection = "NORTH_BOUND";
                sched.emergencyVehiclePriority = "P1_CRITICAL";
                sched.intersectionSequence = {"INT_A", "INT_B", "INT_C"};
                sched.signalTransitionStage = "PREEMPTION_GREEN";
                sched.preemptionWindowDurationSec = 45.0;
            }

            // Queue
            dashboard::traffic::QueueTelemetry q;
            q.nodeId = inter->GetId();
            q.queueLengthVehicles = inter->GetQueueCount();
            q.estimatedWaitTimeSec = static_cast<double>(q.queueLengthVehicles) * 4.2;
            q.trafficDensityPct = std::min(100.0, static_cast<double>(q.queueLengthVehicles) * 5.0);
            q.laneOccupancyPct = q.trafficDensityPct * 0.85;
            q.currentThroughputVpm = 28;
            q.isPeakLoad = (q.queueLengthVehicles >= 10);
            m_trafficModel->UpdateQueue(q);

            // IR Sensors
            dashboard::traffic::IrSensorTelemetry ir;
            ir.nodeId = inter->GetId();
            ir.approachDetected = inter->IsApproachIrTriggered();
            ir.departureDetected = inter->IsDepartureIrTriggered();
            ir.approachHealth = "HEALTHY";
            ir.departureHealth = "HEALTHY";
            ir.approachCount = 142 + static_cast<uint32_t>(elapsedSimSeconds * 0.1);
            ir.departureCount = 138 + static_cast<uint32_t>(elapsedSimSeconds * 0.1);
            m_trafficModel->UpdateIrSensors(ir);
        }

        m_trafficModel->SetCorridorSchedule(sched);
    }
}

} // namespace simulator
} // namespace aegis

