#include "aegis/core/coordinator.hpp"
#include "aegis/common/uuid_utils.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/network/serializer.hpp"
#include "aegis/network/topics.hpp"

namespace aegis {
namespace core {
namespace coordinator {

static std::string TriageToString(common::TriageCategory triage) {
    switch (triage) {
        case common::TriageCategory::CODE_RED: return "CODE_RED";
        case common::TriageCategory::CODE_AMBER: return "CODE_AMBER";
        case common::TriageCategory::CODE_GREEN: return "CODE_GREEN";
        default: return "CODE_GREEN";
    }
}

CentralCoordinator::CentralCoordinator()
    : m_network(std::make_shared<network::NetworkManager>()),
      m_ownsNetwork(true) {}

CentralCoordinator::CentralCoordinator(std::shared_ptr<network::NetworkManager> netManager)
    : m_network(std::move(netManager)),
      m_ownsNetwork(false) {}

CentralCoordinator::~CentralCoordinator() {
    Stop();
}

bool CentralCoordinator::Start() {
    if (m_running) return true;

    if (m_ownsNetwork && !m_network->IsConnected()) {
        m_network->Initialize("127.0.0.1", 1883, false, "COORD-CENTRAL");
    }

    m_fleetManager.PopulateSimulatedFleet();
    m_routePlanner.InitializeDefaultCityGraph();

    SetupNetworkSubscriptions();
    SetupCorridorHandlers();

    m_running = true;
    return true;
}

void CentralCoordinator::Stop() {
    if (!m_running) return;
    m_running = false;
    if (m_ownsNetwork && m_network) {
        m_network->Shutdown();
    }
}

bool CentralCoordinator::IsRunning() const noexcept {
    return m_running;
}

void CentralCoordinator::SetupCorridorHandlers() {
    m_corridorEngine.SetTriggerHandler([this](const std::string& intersectionId,
                                              const network::TrafficPreemptionRequestMsg& req) {
        // Standardized command topic & schema
        network::IntersectionCommandMsg cmd;
        cmd.command = "PREEMPT";
        cmd.intersectionId = intersectionId;
        cmd.timestamp = common::GetUtcMilliseconds();
        std::string payload = network::Serializer::Serialize(cmd);
        std::string topic = network::topics::IntersectionCommands(intersectionId);
        m_network->Publish(topic, payload, 2);

        // Backward compatibility for legacy listeners
        std::string legacyPayload = network::Serializer::Serialize(req);
        m_network->Publish("aegis/traffic/" + intersectionId + "/preempt", legacyPayload, 2);
    });

    m_corridorEngine.SetReleaseHandler([this](const std::string& intersectionId,
                                              const std::string& unitId) {
        // Standardized command topic & schema
        network::IntersectionCommandMsg cmd;
        cmd.command = "CLEAR";
        cmd.intersectionId = intersectionId;
        cmd.timestamp = common::GetUtcMilliseconds();
        std::string payload = network::Serializer::Serialize(cmd);
        std::string topic = network::topics::IntersectionCommands(intersectionId);
        m_network->Publish(topic, payload, 1);

        // Backward compatibility for legacy listeners
        std::string legacyPayload = "{\"intersectionId\":\"" + intersectionId + "\",\"unitId\":\"" + unitId + "\"}";
        m_network->Publish("aegis/traffic/" + intersectionId + "/release", legacyPayload, 1);
    });
}

void CentralCoordinator::SetupNetworkSubscriptions() {
    // 1. Subscribe to vehicle kinematics updates (canonical & legacy alias)
    auto gpsHandler = [this](const std::string& /*topic*/, const std::string& payload) {
        auto res = network::Serializer::DeserializeGPSUpdate(payload);
        if (res.IsOk()) {
            const auto& gps = res.Value();
            ProcessGpsUpdate(gps.unitId, {gps.latitude, gps.longitude, 0.0}, gps.speed, gps.bearing);
        }
    };
    m_network->Subscribe(std::string(network::topics::ALL_FLEET_LOCATION), gpsHandler);
    m_network->Subscribe(std::string(network::topics::LEGACY_AMBULANCE_GPS), gpsHandler);

    // 2. Subscribe to incoming 911 emergency requests
    m_network->Subscribe(std::string(network::topics::EMERGENCY_REQUEST), [this](const std::string& /*topic*/, const std::string& payload) {
        auto res = network::Serializer::DeserializeEmergencyRequest(payload);
        if (res.IsOk()) {
            const auto& req = res.Value();
            EmergencyCallInput input;
            input.callerName = req.callerContact;
            input.callerContact = req.callerContact;
            input.location = {req.latitude, req.longitude, 0.0};
            input.description = req.incidentType;
            input.incidentTypeHint = req.incidentType;
            input.requestedCapabilities = req.requiredCapabilities;
            SubmitEmergencyCall(input);
        }
    });

    // 3. Subscribe to intersection status & telemetry
    m_network->Subscribe(std::string(network::topics::ALL_INTERSECTION_STATUS), [](const std::string& /*topic*/, const std::string& /*payload*/) {
        // Intersection monitoring / telemetry ingestion
    });
}

void CentralCoordinator::ProcessGpsUpdate(const std::string& unitId,
                                          const common::GeoCoordinate& location,
                                          float speed, float bearing) {
    m_fleetManager.UpdateTelemetry(unitId, location, speed, bearing);
    m_corridorEngine.EvaluateVehicleProgress(unitId, location, speed, bearing);
}

common::Result<IncidentDispatchResult> CentralCoordinator::SubmitEmergencyCall(const EmergencyCallInput& input) {
    // 1. Qualify and validate call intake
    auto qualRes = m_callHandler.QualifyCall(input);
    if (qualRes.IsErr()) {
        return common::Result<IncidentDispatchResult>::Err(qualRes.Error());
    }

    const auto& report = qualRes.Value();

    // 2. Register incident
    auto createRes = m_incidentManager.CreateIncident(report);
    if (createRes.IsErr()) {
        return common::Result<IncidentDispatchResult>::Err(createRes.Error());
    }

    auto incidentRec = m_incidentManager.GetIncident(report.incidentId).Value();

    // 3. Automated dispatch selection
    auto dispatchRes = m_dispatchEngine.SelectBestAmbulance(incidentRec, m_fleetManager);
    if (dispatchRes.IsErr()) {
        IncidentDispatchResult failResult;
        failResult.incidentId = report.incidentId;
        failResult.success = false;
        failResult.failureReason = "NO_MATCHING_AMBULANCE_AVAILABLE";
        return common::Result<IncidentDispatchResult>::Ok(failResult);
    }

    const auto& decision = dispatchRes.Value();
    std::string unitId = decision.unitId;

    // 4. Atomic unit reservation lock
    if (!m_fleetManager.ReserveUnit(unitId, report.incidentId)) {
        IncidentDispatchResult failResult;
        failResult.incidentId = report.incidentId;
        failResult.success = false;
        failResult.failureReason = "FLEET_RESERVATION_CONFLICT";
        return common::Result<IncidentDispatchResult>::Ok(failResult);
    }

    // 5. Compute optimal navigation route
    auto ambulanceRec = m_fleetManager.GetUnit(unitId).Value();
    auto route = m_routePlanner.CalculateRoute(ambulanceRec.location, report.location);

    // 6. Register dynamic green corridor
    std::string corridorId = "CORR-" + common::GenerateUuidV4().substr(0, 8);
    m_corridorEngine.RegisterCorridor(corridorId, report.incidentId, unitId, route.intersectionIds);

    // 7. Update incident record
    m_incidentManager.AssignAmbulance(report.incidentId, unitId, route.routeId);

    // 8. Publish dispatch notification over MQTT
    network::EmergencyRequestMsg emMsg;
    emMsg.incidentId = report.incidentId;
    emMsg.timestamp = report.timestamp;
    emMsg.callerContact = report.callerContact;
    emMsg.latitude = report.location.latitude;
    emMsg.longitude = report.location.longitude;
    emMsg.triageCategory = TriageToString(report.triage);
    emMsg.incidentType = report.incidentType;
    emMsg.requiredCapabilities = report.requiredCapabilities;

    std::string emPayload = network::Serializer::Serialize(emMsg);
    m_network->Publish("aegis/incident/" + report.incidentId + "/dispatched", emPayload, 2);

    IncidentDispatchResult res;
    res.incidentId = report.incidentId;
    res.assignedUnitId = unitId;
    res.route = route;
    res.corridorId = corridorId;
    res.success = true;

    return common::Result<IncidentDispatchResult>::Ok(res);
}

} // namespace coordinator
} // namespace core
} // namespace aegis
