#ifndef AEGIS_CORE_DYNAMIC_GREEN_CORRIDOR_HPP
#define AEGIS_CORE_DYNAMIC_GREEN_CORRIDOR_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace core {
namespace coordinator {

/**
 * @brief Computes preemption envelopes and issues dynamic green corridor signals.
 * TODO: Implement arrival estimation and sequential intersection preemption dispatch.
 */
class DynamicGreenCorridor {
public:
    DynamicGreenCorridor();
    ~DynamicGreenCorridor();

    void TriggerPreemption(const std::string& intersectionId, const std::string& unitId);
    void ReleaseCorridor(const std::string& intersectionId);
};

} // namespace coordinator
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_DYNAMIC_GREEN_CORRIDOR_HPP
