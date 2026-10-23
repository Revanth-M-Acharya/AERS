#include "aegis/edge/ambulance_node.hpp"

namespace aegis {
namespace edge {
namespace ambulance {

AmbulanceNode::AmbulanceNode() = default;
AmbulanceNode::~AmbulanceNode() = default;

bool AmbulanceNode::Initialize(const std::string& unitId, const std::string& /*configPath*/) {
    m_unitId = unitId;
    return true;
}

bool AmbulanceNode::Start() {
    // TODO: Start GPS ingestion loop, vitals sampler, and V2V beacon thread
    return true;
}

void AmbulanceNode::Stop() {
    // TODO: Terminate threads and flush local store-forward cache
}

} // namespace ambulance
} // namespace edge
} // namespace aegis
