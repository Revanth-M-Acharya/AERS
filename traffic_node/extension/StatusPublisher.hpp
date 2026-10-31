/**
 * @file StatusPublisher.hpp
 * @brief Extension interface for future AI status reporting.
 * Pure virtual interface — contains ZERO implementation.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>

namespace aegis::traffic::extension {

class IStatusPublisher {
public:
    virtual ~IStatusPublisher() = default;

    /**
     * @brief Publish intersection node status payload to external AI subscribers.
     */
    virtual void PublishStatus(const std::string& statusJson) = 0;
};

} // namespace aegis::traffic::extension

