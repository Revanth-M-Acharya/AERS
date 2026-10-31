/**
 * @file IntersectionsConfig.hpp
 * @brief Configuration model and parser for config/intersections.json.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace aegis::config {

struct GpioPinConfig {
    int redPin{5};
    int bluePin{6};
    bool hasRed{true};
    bool hasBlue{true};
    int physicalRedPin{29};
    int physicalBluePin{31};
    int groundPin{30};
};

struct IntersectionNodeConfig {
    int id{1};
    std::string name{"INT_1"};
    std::string ip{"10.204.179.245"};
    uint16_t port{5001};
    GpioPinConfig gpio;
    std::string description;
};

struct CoordinatorNodeConfig {
    std::string id{"COORDINATOR"};
    std::string ip{"10.204.179.4"};
    bool hasLeds{false};
    std::string description;
};

struct SystemSettings {
    std::string name{"SMART_AEGIS"};
    std::string version{"2.5.0"};
    uint32_t reconnectIntervalMs{2000};
    uint32_t pingIntervalMs{5000};
    uint32_t commandTimeoutMs{1000};
    uint32_t maxRetries{3};
    CoordinatorNodeConfig coordinator;
};

class IntersectionsConfig {
public:
    IntersectionsConfig() = default;
    ~IntersectionsConfig() = default;

    static std::shared_ptr<IntersectionsConfig> LoadFromFile(const std::string& filePath = "config/intersections.json");
    static std::shared_ptr<IntersectionsConfig> LoadFromJsonString(const std::string& jsonContent);
    static std::shared_ptr<IntersectionsConfig> CreateDefaultCluster();

    bool IsValid() const { return valid_; }
    const SystemSettings& GetSystemSettings() const { return system_; }
    const std::vector<IntersectionNodeConfig>& GetIntersections() const { return intersections_; }

    std::optional<IntersectionNodeConfig> GetIntersectionById(int id) const;
    std::optional<IntersectionNodeConfig> GetIntersectionByName(const std::string& name) const;

private:
    bool valid_{false};
    SystemSettings system_;
    std::vector<IntersectionNodeConfig> intersections_;
};

} // namespace aegis::config
