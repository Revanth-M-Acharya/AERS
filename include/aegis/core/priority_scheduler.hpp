#ifndef AEGIS_CORE_PRIORITY_SCHEDULER_HPP
#define AEGIS_CORE_PRIORITY_SCHEDULER_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace core {
namespace scheduler {

struct TriageEvaluationInput {
    common::TriageCategory category{common::TriageCategory::CODE_GREEN};
    uint8_t spO2{98};
    uint16_t systolicBP{120};
    double distanceKm{0.0};
    uint32_t trafficDelaySec{0};
};

/**
 * @brief Multi-criteria decision engine for incident prioritization and contention arbitration.
 * TODO: Implement weighted multi-factorial ranking formula.
 */
class PriorityScheduler {
public:
    PriorityScheduler();
    ~PriorityScheduler();

    float CalculatePriorityScore(const TriageEvaluationInput& input) const;
    std::string ArbitrateContention(const std::string& unitA, float scoreA,
                                   const std::string& unitB, float scoreB) const;
};

} // namespace scheduler
} // namespace core
} // namespace aegis

#endif // AEGIS_CORE_PRIORITY_SCHEDULER_HPP
