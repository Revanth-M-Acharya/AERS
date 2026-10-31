/**
 * @file OrchestrationConfig.hpp
 * @brief Dynamic configuration parameters for Coordinator Orchestration & Distributed Runtime.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::coordinator::orchestration {

class OrchestrationConfig {
public:
    OrchestrationConfig();

    uint32_t heartbeatTimeoutMs{5000};
    uint32_t retryCount{3};
    uint32_t retryDelayMs{500};
    uint32_t dispatchTimeoutMs{2000};
    uint32_t clockDriftThresholdMs{500};
    uint32_t maxNodeDelayMs{3000};
    uint32_t recoveryTimeoutMs{4000};
    uint32_t fsmTimeoutMs{10000};
    uint32_t eventQueueSize{256};
    uint32_t threadPoolSize{4};

    bool LoadFromFile(const std::string& filePath);
    bool Validate() const;
    std::string ToString() const;
};

} // namespace aegis::coordinator::orchestration

