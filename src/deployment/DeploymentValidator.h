/**
 * @file DeploymentValidator.h
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Validator.
 *
 * Validates individual node configurations and complete multi-node cluster
 * deployment setups before system initialization and booting.
 */

#ifndef AEGIS_DEPLOYMENT_DEPLOYMENT_VALIDATOR_H
#define AEGIS_DEPLOYMENT_DEPLOYMENT_VALIDATOR_H

#include "NodeConfiguration.h"
#include <string>
#include <vector>

namespace aegis {
namespace deployment {

struct ValidationResult {
    bool valid{true};
    std::vector<std::string> errors{};
    std::vector<std::string> warnings{};

    void AddError(const std::string& err) {
        errors.push_back(err);
        valid = false;
    }

    void AddWarning(const std::string& warn) {
        warnings.push_back(warn);
    }
};

class DeploymentValidator {
public:
    /**
     * @brief Validate a single node configuration.
     * @param config The node configuration to evaluate.
     * @return ValidationResult containing valid status, errors, and warnings.
     */
    static ValidationResult ValidateNode(const NodeConfiguration& config);

    /**
     * @brief Validate an entire multi-node cluster deployment (Coordinator + Edge Nodes).
     * @param coordinatorConfig Central Coordinator configuration.
     * @param edgeNodeConfigs List of Roadside Intersection Node configurations.
     * @return ValidationResult covering cluster-wide consistency.
     */
    static ValidationResult ValidateCluster(const NodeConfiguration& coordinatorConfig,
                                            const std::vector<NodeConfiguration>& edgeNodeConfigs);
};

} // namespace deployment
} // namespace aegis

#endif // AEGIS_DEPLOYMENT_DEPLOYMENT_VALIDATOR_H
