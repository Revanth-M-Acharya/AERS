#ifndef AEGIS_CONFIG_CONFIG_MANAGER_HPP
#define AEGIS_CONFIG_CONFIG_MANAGER_HPP

#include <string>
#include <memory>
#include "aegis/common/error_codes.hpp"

namespace aegis {
namespace config {

/**
 * @brief Configuration Manager responsible for loading and querying JSON profiles.
 * TODO: Implement JSON schema validation and thread-safe parameter queries.
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    common::ErrorCode LoadProfile(const std::string& configFilePath);
    int GetInt(const std::string& key, int defaultValue = 0) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;
    double GetDouble(const std::string& key, double defaultValue = 0.0) const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace config
} // namespace aegis

#endif // AEGIS_CONFIG_CONFIG_MANAGER_HPP
