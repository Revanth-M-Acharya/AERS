#include "aegis/config/config_manager.hpp"

namespace aegis {
namespace config {

class ConfigManager::Impl {
public:
    // TODO: Ingest JSON profile into an in-memory key-value cache
};

ConfigManager::ConfigManager() : m_impl(std::make_unique<Impl>()) {}
ConfigManager::~ConfigManager() = default;

common::ErrorCode ConfigManager::LoadProfile(const std::string& /*configFilePath*/) {
    // TODO: Validate JSON schema against configuration model
    return common::ErrorCode::SUCCESS;
}

int ConfigManager::GetInt(const std::string& /*key*/, int defaultValue) const {
    return defaultValue;
}

std::string ConfigManager::GetString(const std::string& /*key*/, const std::string& defaultValue) const {
    return defaultValue;
}

bool ConfigManager::GetBool(const std::string& /*key*/, bool defaultValue) const {
    return defaultValue;
}

double ConfigManager::GetDouble(const std::string& /*key*/, double defaultValue) const {
    return defaultValue;
}

} // namespace config
} // namespace aegis
