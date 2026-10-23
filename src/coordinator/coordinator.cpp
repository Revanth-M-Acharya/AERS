#include "coordinator.hpp"

namespace aegis {
namespace coordinator {

CentralCoordinator::CentralCoordinator() = default;
CentralCoordinator::~CentralCoordinator() = default;

bool CentralCoordinator::Start() {
    return true;
}

void CentralCoordinator::Stop() {}

} // namespace coordinator
} // namespace aegis
