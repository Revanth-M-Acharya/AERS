#include "aegis/core/coordinator.hpp"

namespace aegis {
namespace core {
namespace coordinator {

CentralCoordinator::CentralCoordinator() = default;
CentralCoordinator::~CentralCoordinator() = default;

bool CentralCoordinator::Start() {
    // TODO: Bootstrap call handler, route engine, and preemption listeners
    return true;
}

void CentralCoordinator::Stop() {
    // TODO: Gracefully flush pending assignments
}

} // namespace coordinator
} // namespace core
} // namespace aegis
