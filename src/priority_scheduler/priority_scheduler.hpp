#ifndef AEGIS_PRIORITY_SCHEDULER_HPP
#define AEGIS_PRIORITY_SCHEDULER_HPP

#include <string>
#include "aegis/common/types.hpp"

namespace aegis {
namespace scheduler {

struct TriageEvaluationInput {
    common::TriageCategory category;
    uint8_t spO2;
    uint16_t systolicBP;
    double distanceKm;
};

class PriorityScheduler {
public:
    PriorityScheduler() = default;
    ~PriorityScheduler() = default;

    float CalculatePriorityScore(const TriageEvaluationInput& input);
    std::string ArbitrateContention(const std::string& unitA, float scoreA,
                                   const std::string& unitB, float scoreB);
};

} // namespace scheduler
} // namespace aegis

#endif // AEGIS_PRIORITY_SCHEDULER_HPP
