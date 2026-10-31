/**
 * @file ProductionConfigManager.hpp
 * @brief Configuration manager supporting Dev/Test/Prod profiles, schema validation, and runtime reload.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <cstdint>

namespace aegis::production {

enum class DeploymentProfile {
    DEVELOPMENT,
    TEST,
    PRODUCTION
};

inline std::string DeploymentProfileToString(DeploymentProfile profile) {
    switch (profile) {
        case DeploymentProfile::DEVELOPMENT: return "DEVELOPMENT";
        case DeploymentProfile::TEST:        return "TEST";
        case DeploymentProfile::PRODUCTION:  return "PRODUCTION";
        default:                             return "UNKNOWN";
    }
}

inline DeploymentProfile StringToDeploymentProfile(const std::string& str) {
    if (str == "DEVELOPMENT" || str == "dev")  return DeploymentProfile::DEVELOPMENT;
    if (str == "TEST" || str == "test")        return DeploymentProfile::TEST;
    if (str == "PRODUCTION" || str == "prod")  return DeploymentProfile::PRODUCTION;
    return DeploymentProfile::PRODUCTION;
}

struct ConfigValidationResult {
    bool isValid{true};
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

class ProductionConfigManager {
public:
    explicit ProductionConfigManager(DeploymentProfile profile = DeploymentProfile::PRODUCTION);
    ~ProductionConfigManager() = default;

    void SetProfile(DeploymentProfile profile);
    DeploymentProfile GetProfile() const;

    bool LoadFromString(const std::string& configData);
    bool LoadDefaults();

    void SetString(const std::string& key, const std::string& value);
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    void SetInt(const std::string& key, int64_t value);
    int64_t GetInt(const std::string& key, int64_t defaultValue = 0) const;

    void SetDouble(const std::string& key, double value);
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;

    void SetBool(const std::string& key, bool value);
    bool GetBool(const std::string& key, bool defaultValue = false) const;

    /**
     * @brief Validate configuration parameters against safety schemas.
     */
    ConfigValidationResult ValidateSchema() const;

    std::string ExportJson() const;

private:
    mutable std::recursive_mutex mutex_;
    DeploymentProfile profile_{DeploymentProfile::PRODUCTION};
    std::unordered_map<std::string, std::string> entries_;
};

} // namespace aegis::production
