/**
 * @file NodeAuthenticator.hpp
 * @brief Node identity verification, whitelist enforcement, and token authentication.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>

namespace aegis::production {

struct AuthorizedNodeCredentials {
    std::string nodeId;
    std::string role; // "TRAFFIC_NODE", "AMBULANCE", "HOSPITAL", "CAMERA"
    std::string authToken;
    uint64_t registeredTimestampMs{0};
    bool isRevoked{false};
};

class NodeAuthenticator {
public:
    NodeAuthenticator();
    ~NodeAuthenticator() = default;

    void RegisterAuthorizedNode(const std::string& nodeId, const std::string& role, const std::string& authToken);
    void RevokeNode(const std::string& nodeId);

    /**
     * @brief Authenticate node credentials.
     * @return true if node is authorized, not revoked, and token matches; false otherwise.
     */
    bool Authenticate(const std::string& nodeId, const std::string& authToken);

    bool IsNodeAuthorized(const std::string& nodeId) const;
    bool IsNodeRevoked(const std::string& nodeId) const;

    size_t GetAuthorizedCount() const;
    uint32_t GetRejectedAuthAttempts() const;

    void Reset();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, AuthorizedNodeCredentials> credentials_;
    uint32_t rejectedAttempts_{0};
};

} // namespace aegis::production

