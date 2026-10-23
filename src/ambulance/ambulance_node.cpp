#include "ambulance_node.hpp"

namespace aegis {
namespace ambulance {

AmbulanceNode::AmbulanceNode() = default;
AmbulanceNode::~AmbulanceNode() = default;

bool AmbulanceNode::Start() {
    return true;
}

void AmbulanceNode::Stop() {}

} // namespace ambulance
} // namespace aegis
