#ifndef AEGIS_COORDINATOR_DYNAMIC_GREEN_CORRIDOR_HPP
#define AEGIS_COORDINATOR_DYNAMIC_GREEN_CORRIDOR_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace coordinator {

class DynamicGreenCorridor {
public:
    DynamicGreenCorridor() = default;
    ~DynamicGreenCorridor() = default;

    void TriggerPreemption(const std::string& intersectionId, const std::string& unitId);
    void ReleaseCorridor(const std::string& intersectionId);
};

} // namespace coordinator
} // namespace aegis

#endif // AEGIS_COORDINATOR_DYNAMIC_GREEN_CORRIDOR_HPP
