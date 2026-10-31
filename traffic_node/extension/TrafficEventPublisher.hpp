/**
 * @file TrafficEventPublisher.hpp
 * @brief Extension interface for future AI traffic event distribution.
 * Pure virtual interface — contains ZERO implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>

namespace aegis::traffic::extension {

class ITrafficEventPublisher {
public:
    virtual ~ITrafficEventPublisher() = default;

    /**
     * @brief Publish structured traffic event notifications (e.g. queue buildup, incident, preemption).
     */
    virtual void PublishTrafficEvent(const std::string& eventType, const std::string& eventData) = 0;
};

} // namespace aegis::traffic::extension

