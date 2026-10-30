/**
 * @file ConfigurationLoader.h
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Configuration Loader.
 *
 * Implements deterministic JSON parsing and saving for node configuration files
 * with zero third-party dependencies.
 */

#ifndef AEGIS_DEPLOYMENT_CONFIGURATION_LOADER_H
#define AEGIS_DEPLOYMENT_CONFIGURATION_LOADER_H

#include "NodeConfiguration.h"
#include <string>

namespace aegis {
namespace deployment {

class ConfigurationLoader {
public:
    /**
     * @brief Load and parse a NodeConfiguration from a JSON file.
     * @param filePath Path to JSON configuration file.
     * @param outConfig Destination configuration object to populate.
     * @param outError Optional string receiving error description on failure.
     * @return true on success, false if file read or JSON parse fails.
     */
    static bool LoadFromFile(const std::string& filePath,
                             NodeConfiguration& outConfig,
                             std::string* outError = nullptr);

    /**
     * @brief Parse a NodeConfiguration directly from a raw JSON string buffer.
     * @param jsonStr String buffer containing the JSON document.
     * @param outConfig Destination configuration object to populate.
     * @param outError Optional string receiving error description on failure.
     * @return true on success, false if JSON parse fails.
     */
    static bool LoadFromString(const std::string& jsonStr,
                               NodeConfiguration& outConfig,
                               std::string* outError = nullptr);

    /**
     * @brief Serialize and write a NodeConfiguration to a JSON file.
     * @param filePath Path to target file.
     * @param config Configuration object to write.
     * @param outError Optional string receiving error description on failure.
     * @return true on success, false on write failure.
     */
    static bool SaveToFile(const std::string& filePath,
                           const NodeConfiguration& config,
                           std::string* outError = nullptr);

    /**
     * @brief Search standard deployment paths to discover default configuration file.
     * Checks: config/node.json, config/deployment/rpi1_coordinator.json, /etc/aegis/node.json.
     * @return Path to first discovered configuration file or empty string if none found.
     */
    static std::string FindDefaultConfigFile();
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_CONFIGURATION_LOADER_H
