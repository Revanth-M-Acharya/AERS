/**
 * @file ProductionConfigManager.cpp
 * @brief Implementation of production configuration manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/config/ProductionConfigManager.hpp"
#include <sstream>

namespace aegis::production {

ProductionConfigManager::ProductionConfigManager(DeploymentProfile profile)
    : profile_(profile) {
    LoadDefaults();
}

void ProductionConfigManager::SetProfile(DeploymentProfile profile) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    profile_ = profile;
    LoadDefaults();
}

DeploymentProfile ProductionConfigManager::GetProfile() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return profile_;
}

bool ProductionConfigManager::LoadDefaults() {
    entries_.clear();
    if (profile_ == DeploymentProfile::PRODUCTION) {
        entries_["server.listen_port"] = "9090";
        entries_["server.max_clients"] = "64";
        entries_["watchdog.timeout_ms"] = "2000";
        entries_["heartbeat.interval_ms"] = "1000";
        entries_["heartbeat.max_misses"] = "3";
        entries_["security.require_auth"] = "true";
        entries_["security.max_skew_ms"] = "5000";
        entries_["reliability.queue_size"] = "2000";
    } else if (profile_ == DeploymentProfile::TEST) {
        entries_["server.listen_port"] = "9090";
        entries_["server.max_clients"] = "16";
        entries_["watchdog.timeout_ms"] = "5000";
        entries_["heartbeat.interval_ms"] = "500";
        entries_["heartbeat.max_misses"] = "5";
        entries_["security.require_auth"] = "false";
        entries_["security.max_skew_ms"] = "30000";
        entries_["reliability.queue_size"] = "500";
    } else { // DEVELOPMENT
        entries_["server.listen_port"] = "9090";
        entries_["server.max_clients"] = "8";
        entries_["watchdog.timeout_ms"] = "10000";
        entries_["heartbeat.interval_ms"] = "2000";
        entries_["heartbeat.max_misses"] = "10";
        entries_["security.require_auth"] = "false";
        entries_["security.max_skew_ms"] = "60000";
        entries_["reliability.queue_size"] = "200";
    }
    return true;
}

bool ProductionConfigManager::LoadFromString(const std::string& configData) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::istringstream stream(configData);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        auto eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            std::string key = line.substr(0, eqPos);
            std::string val = line.substr(eqPos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            val.erase(0, val.find_first_not_of(" \t\r\n"));
            val.erase(val.find_last_not_of(" \t\r\n") + 1);

            if (!key.empty()) {
                entries_[key] = val;
            }
        }
    }
    return true;
}

void ProductionConfigManager::SetString(const std::string& key, const std::string& value) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    entries_[key] = value;
}

std::string ProductionConfigManager::GetString(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto it = entries_.find(key);
    return (it != entries_.end()) ? it->second : defaultValue;
}

void ProductionConfigManager::SetInt(const std::string& key, int64_t value) {
    SetString(key, std::to_string(value));
}

int64_t ProductionConfigManager::GetInt(const std::string& key, int64_t defaultValue) const {
    std::string str = GetString(key);
    if (str.empty()) return defaultValue;
    try {
        return std::stoll(str);
    } catch (...) {
        return defaultValue;
    }
}

void ProductionConfigManager::SetDouble(const std::string& key, double value) {
    SetString(key, std::to_string(value));
}

double ProductionConfigManager::GetDouble(const std::string& key, double defaultValue) const {
    std::string str = GetString(key);
    if (str.empty()) return defaultValue;
    try {
        return std::stod(str);
    } catch (...) {
        return defaultValue;
    }
}

void ProductionConfigManager::SetBool(const std::string& key, bool value) {
    SetString(key, value ? "true" : "false");
}

bool ProductionConfigManager::GetBool(const std::string& key, bool defaultValue) const {
    std::string str = GetString(key);
    if (str.empty()) return defaultValue;
    return (str == "true" || str == "1" || str == "yes");
}

ConfigValidationResult ProductionConfigManager::ValidateSchema() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ConfigValidationResult result;

    int64_t port = GetInt("server.listen_port", 0);
    if (port <= 1024 || port > 65535) {
        result.isValid = false;
        result.errors.push_back("server.listen_port must be in range 1025-65535, found " + std::to_string(port));
    }

    int64_t watchdog = GetInt("watchdog.timeout_ms", 0);
    if (watchdog < 100 || watchdog > 60000) {
        result.isValid = false;
        result.errors.push_back("watchdog.timeout_ms must be between 100ms and 60000ms");
    }

    int64_t hbInterval = GetInt("heartbeat.interval_ms", 0);
    if (hbInterval < 100 || hbInterval > 10000) {
        result.isValid = false;
        result.errors.push_back("heartbeat.interval_ms must be between 100ms and 10000ms");
    }

    int64_t hbMisses = GetInt("heartbeat.max_misses", 0);
    if (hbMisses < 1 || hbMisses > 20) {
        result.isValid = false;
        result.errors.push_back("heartbeat.max_misses must be between 1 and 20");
    }

    return result;
}

std::string ProductionConfigManager::ExportJson() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::ostringstream ss;
    ss << "{\n"
       << "  \"profile\": \"" << DeploymentProfileToString(profile_) << "\",\n"
       << "  \"entries\": {\n";

    bool first = true;
    for (const auto& [k, v] : entries_) {
        if (!first) ss << ",\n";
        first = false;
        ss << "    \"" << k << "\": \"" << v << "\"";
    }
    ss << "\n  }\n}";
    return ss.str();
}

} // namespace aegis::production
