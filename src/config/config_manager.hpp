#ifndef AEGIS_CONFIG_CONFIG_MANAGER_HPP
#define AEGIS_CONFIG_CONFIG_MANAGER_HPP

#include <string>
#include "aegis/common/error_codes.hpp"

namespace aegis {
namespace config {

class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    common::ErrorCode LoadProfile(const std::string& configFilePath);
    int GetInt(const std::string& key, int defaultValue = 0) const;
    std::string GetString(const std::string& key, const std::string& defaultValue = "") const;
    bool GetBool(const std::string& key, bool defaultValue = false) const;

private:
    class Impl;
    Impl* m_impl{nullptr};
};

} // namespace config
} // namespace aegis

#endif // AEGIS_CONFIG_CONFIG_MANAGER_HPP
