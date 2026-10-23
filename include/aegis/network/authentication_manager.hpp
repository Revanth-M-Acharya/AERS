/**
 * @file authentication_manager.hpp
 * @brief Token validation, RBAC verification, HMAC signatures, and mTLS hooks.
 */

#ifndef AEGIS_NETWORK_AUTHENTICATION_MANAGER_HPP
#define AEGIS_NETWORK_AUTHENTICATION_MANAGER_HPP

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include "aegis/common/error_codes.hpp"
#include "aegis/common/result.hpp"

namespace aegis {
namespace network {

enum class UserRole : uint8_t {
    UNKNOWN = 0,
    DISPATCHER,
    TRAFFIC_OPERATOR,
    PARAMEDIC,
    PHYSICIAN,
    SYSTEM_ADMIN
};

struct AuthTokenClaims {
    std::string subject;
    UserRole role{UserRole::UNKNOWN};
    uint64_t issuedAtMs{0};
    uint64_t expiresAtMs{0};
    bool isExpired{true};
};

class AuthenticationManager {
public:
    AuthenticationManager();
    ~AuthenticationManager() = default;

    void SetSharedHmacSecret(std::string_view secret);
    void RegisterApiToken(std::string_view token, UserRole role, uint64_t validDurationSec);

    common::Result<AuthTokenClaims> ValidateToken(std::string_view bearerToken) const;
    bool AuthorizeRole(const AuthTokenClaims& claims, UserRole requiredRole) const noexcept;

    std::string SignPayload(std::string_view payload) const;
    bool VerifySignature(std::string_view payload, std::string_view expectedSignature) const;

    // Future-ready mTLS interface
    bool VerifyNodeCertificate(std::string_view certPem, std::string_view caPem) const;

private:
    struct TokenRecord {
        std::string token;
        UserRole role;
        uint64_t expiryMs;
    };

    std::string m_hmacSecret{"SMART_AEGIS_SHARED_KEY_2026"};
    std::vector<TokenRecord> m_registeredTokens;
};

} // namespace network
} // namespace aegis

#endif // AEGIS_NETWORK_AUTHENTICATION_MANAGER_HPP
