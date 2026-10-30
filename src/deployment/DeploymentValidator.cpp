/**
 * @file DeploymentValidator.cpp
 * @brief SMART AEGIS (AERS) Phase 16D - Deployment Validator Implementation.
 */

#include "DeploymentValidator.h"
#include <set>

namespace aegis {
namespace deployment {

ValidationResult DeploymentValidator::ValidateNode(const NodeConfiguration& config) {
    ValidationResult res;

    std::string err;
    if (!config.Validate(&err)) {
        res.AddError(err);
    }

    if (config.port < 1024) {
        res.AddWarning("Port " + std::to_string(config.port) + " is in privileged port range (< 1024)");
    }

    if (config.role == NodeRole::TRAFFIC_NODE && config.coordinatorPort < 1024) {
        res.AddWarning("Coordinator port " + std::to_string(config.coordinatorPort) + " is in privileged port range (< 1024)");
    }

    if (config.heartbeatIntervalMs > 5000) {
        res.AddWarning("Heartbeat interval " + std::to_string(config.heartbeatIntervalMs) + " ms may lead to slow fault detection");
    }

    if (config.gpioEnabled && config.simulationEnabled) {
        res.AddWarning("Both GPIO and simulation are enabled; simulated drivers will be used on non-RPi hosts");
    }

    return res;
}

ValidationResult DeploymentValidator::ValidateCluster(const NodeConfiguration& coordinatorConfig,
                                                      const std::vector<NodeConfiguration>& edgeNodeConfigs) {
    ValidationResult res;

    // Validate Coordinator itself
    ValidationResult coordRes = ValidateNode(coordinatorConfig);
    if (!coordRes.valid) {
        for (const auto& err : coordRes.errors) res.AddError("Coordinator: " + err);
    }
    for (const auto& w : coordRes.warnings) res.AddWarning("Coordinator: " + w);

    if (coordinatorConfig.role != NodeRole::COORDINATOR) {
        res.AddError("Coordinator configuration has invalid role: " + NodeRoleToString(coordinatorConfig.role));
    }

    std::set<std::string> registeredNodeIds;
    std::set<std::string> registeredAddresses;

    registeredNodeIds.insert(coordinatorConfig.nodeId);
    registeredAddresses.insert(coordinatorConfig.ipAddress + ":" + std::to_string(coordinatorConfig.port));

    for (size_t i = 0; i < edgeNodeConfigs.size(); ++i) {
        const auto& node = edgeNodeConfigs[i];
        std::string prefix = "Node [" + node.nodeId + "]: ";

        ValidationResult nodeRes = ValidateNode(node);
        if (!nodeRes.valid) {
            for (const auto& err : nodeRes.errors) res.AddError(prefix + err);
        }
        for (const auto& w : nodeRes.warnings) res.AddWarning(prefix + w);

        if (node.role != NodeRole::TRAFFIC_NODE) {
            res.AddError(prefix + "Role must be TRAFFIC_NODE");
        }

        // Unique Node ID check
        if (registeredNodeIds.find(node.nodeId) != registeredNodeIds.end()) {
            res.AddError(prefix + "Duplicate Node ID detected: " + node.nodeId);
        } else {
            registeredNodeIds.insert(node.nodeId);
        }

        // Host:Port collision check
        std::string addressEndpoint = node.ipAddress + ":" + std::to_string(node.port);
        if (registeredAddresses.find(addressEndpoint) != registeredAddresses.end()) {
            res.AddWarning(prefix + "Endpoint " + addressEndpoint + " is shared with another node (safe only if on different physical Raspberry Pi hosts)");
        } else {
            registeredAddresses.insert(addressEndpoint);
        }

        // Coordinator endpoint consistency check
        if (node.coordinatorPort != coordinatorConfig.port) {
            res.AddError(prefix + "Configured coordinator port " + std::to_string(node.coordinatorPort) +
                         " does not match coordinator listening port " + std::to_string(coordinatorConfig.port));
        }
    }

    // Check that expectedNodes defined by coordinator are covered
    for (const auto& expectedId : coordinatorConfig.expectedNodes) {
        if (registeredNodeIds.find(expectedId) == registeredNodeIds.end()) {
            res.AddError("Coordinator expects node '" + expectedId + "', but no matching configuration exists in cluster");
        }
    }

    return res;
}

} // namespace deployment
} // namespace aegis
