#include "aegis/network/authentication_manager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/common/validator.hpp"
#include <sstream>
#include <iomanip>

namespace aegis {
namespace network {

AuthenticationManager::AuthenticationManager() = default;

void AuthenticationManager::SetSharedHmacSecret(std::string_view secret) {
    m_hmacSecret = secret;
}

void AuthenticationManager::RegisterApiToken(std::string_view token, UserRole role, uint64_t validDurationSec) {
    uint64_t now = common::GetUtcMilliseconds();
    TokenRecord rec;
    rec.token = token;
    rec.role = role;
    rec.expiryMs = now + (validDurationSec * 1000);
    m_registeredTokens.push_back(rec);
}

common::Result<AuthTokenClaims> AuthenticationManager::ValidateToken(std::string_view bearerToken) const {
    std::string_view actualToken = bearerToken;
    if (actualToken.rfind("Bearer ", 0) == 0) {
        actualToken.remove_prefix(7);
    }

    uint64_t now = common::GetUtcMilliseconds();
    for (const auto& rec : m_registeredTokens) {
        if (rec.token == actualToken) {
            AuthTokenClaims claims;
            claims.subject = "operator";
            claims.role = rec.role;
            claims.issuedAtMs = now;
            claims.expiresAtMs = rec.expiryMs;
            claims.isExpired = (now >= rec.expiryMs);

            if (claims.isExpired) {
                return common::Result<AuthTokenClaims>::Err(common::ErrorCode::ERR_AUTH_TOKEN_EXPIRED);
            }
            return common::Result<AuthTokenClaims>::Ok(claims);
        }
    }

    return common::Result<AuthTokenClaims>::Err(common::ErrorCode::ERR_INVALID_MESSAGE);
}

bool AuthenticationManager::AuthorizeRole(const AuthTokenClaims& claims, UserRole requiredRole) const noexcept {
    if (claims.isExpired) {
        return false;
    }
    if (claims.role == UserRole::SYSTEM_ADMIN) {
        return true; // Admin has universal access
    }
    return (claims.role == requiredRole);
}

std::string AuthenticationManager::SignPayload(std::string_view payload) const {
    uint32_t crc = common::Validator::ComputeCrc32(std::string(payload) + m_hmacSecret);
    std::ostringstream oss;
    oss << "sig_" << std::hex << std::setfill('0') << std::setw(8) << crc;
    return oss.str();
}

bool AuthenticationManager::VerifySignature(std::string_view payload, std::string_view expectedSignature) const {
    std::string computed = SignPayload(payload);
    return (computed == expectedSignature);
}

bool AuthenticationManager::VerifyNodeCertificate(std::string_view certPem, std::string_view caPem) const {
    // Future-ready mTLS interface: checks non-empty PEM boundaries
    return (!certPem.empty() && !caPem.empty());
}

} // namespace network
} // namespace aegis
