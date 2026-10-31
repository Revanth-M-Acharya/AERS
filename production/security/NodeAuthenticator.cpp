/**
 * @file NodeAuthenticator.cpp
 * @brief Implementation of node authentication and authorization.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/security/NodeAuthenticator.hpp"
#include "common/utils/TimeUtils.hpp"

namespace aegis::production {

NodeAuthenticator::NodeAuthenticator() = default;

void NodeAuthenticator::RegisterAuthorizedNode(const std::string& nodeId, const std::string& role, const std::string& authToken) {
    std::lock_guard<std::mutex> lock(mutex_);
    AuthorizedNodeCredentials cred;
    cred.nodeId = nodeId;
    cred.role = role;
    cred.authToken = authToken;
    cred.registeredTimestampMs = common::TimeUtils::GetCurrentTimestampMs();
    cred.isRevoked = false;
    credentials_[nodeId] = cred;
}

void NodeAuthenticator::RevokeNode(const std::string& nodeId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = credentials_.find(nodeId);
    if (it != credentials_.end()) {
        it->second.isRevoked = true;
    }
}

bool NodeAuthenticator::Authenticate(const std::string& nodeId, const std::string& authToken) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = credentials_.find(nodeId);
    if (it == credentials_.end()) {
        rejectedAttempts_++;
        return false;
    }

    if (it->second.isRevoked) {
        rejectedAttempts_++;
        return false;
    }

    if (it->second.authToken != authToken) {
        rejectedAttempts_++;
        return false;
    }

    return true;
}

bool NodeAuthenticator::IsNodeAuthorized(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = credentials_.find(nodeId);
    return (it != credentials_.end() && !it->second.isRevoked);
}

bool NodeAuthenticator::IsNodeRevoked(const std::string& nodeId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = credentials_.find(nodeId);
    if (it != credentials_.end()) {
        return it->second.isRevoked;
    }
    return false;
}

size_t NodeAuthenticator::GetAuthorizedCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [k, v] : credentials_) {
        if (!v.isRevoked) count++;
    }
    return count;
}

uint32_t NodeAuthenticator::GetRejectedAuthAttempts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return rejectedAttempts_;
}

void NodeAuthenticator::Reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    credentials_.clear();
    rejectedAttempts_ = 0;
}

} // namespace aegis::production

