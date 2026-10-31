/**
 * @file IntelligenceConfig.hpp
 * @brief Configuration model for Coordinator Intelligence Subsystem.
 * All timing parameters, thresholds, and routing policies are dynamically configurable.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::coordinator::intelligence {

class IntelligenceConfig {
public:
    IntelligenceConfig();

    std::string roadNetworkConfigFile{"RoadNetwork.conf"};
    double defaultSpeedLimitKmh{50.0};
    uint32_t greenLeadTimeMs{5000};
    uint32_t greenHoldDurationMs{15000};
    uint64_t starvationAgingThresholdMs{10000};
    double etaEmaAlpha{0.25};
    uint32_t minSafetyHoldDurationMs{2000};
    uint32_t maxSafetyHoldDurationMs{60000};

    bool LoadFromFile(const std::string& filePath);
    bool Validate() const;
    std::string ToString() const;
};

} // namespace aegis::coordinator::intelligence

