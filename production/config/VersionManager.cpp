/**
 * @file VersionManager.cpp
 * @brief Implementation of build metadata and version manager.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "production/config/VersionManager.hpp"
#include <sstream>

namespace aegis::production {

BuildMetadata VersionManager::GetBuildMetadata() {
    return BuildMetadata{};
}

std::string VersionManager::GetFullVersionString() {
    BuildMetadata meta = GetBuildMetadata();
    return "SMART AEGIS " + meta.releaseTag + " (" + meta.phaseTag + ") on " + meta.targetOs;
}

bool VersionManager::IsProtocolCompatible(uint32_t peerMajor, uint32_t peerMinor) {
    BuildMetadata meta = GetBuildMetadata();
    // Major version must strictly match; minor version backward compatible
    if (peerMajor != meta.protocolVersionMajor) {
        return false;
    }
    return peerMinor <= meta.protocolVersionMinor;
}

std::string VersionManager::ToJson() {
    BuildMetadata m = GetBuildMetadata();
    std::ostringstream ss;
    ss << "{\n"
       << "  \"version\": \"" << m.majorVersion << "." << m.minorVersion << "." << m.patchVersion << "\",\n"
       << "  \"release_tag\": \"" << m.releaseTag << "\",\n"
       << "  \"phase\": \"" << m.phaseTag << "\",\n"
       << "  \"target_platform\": \"" << m.targetPlatform << "\",\n"
       << "  \"target_os\": \"" << m.targetOs << "\",\n"
       << "  \"compiler\": \"" << m.compilerVersion << "\",\n"
       << "  \"build_timestamp\": \"" << m.buildTimestamp << "\",\n"
       << "  \"protocol_version\": \"" << m.protocolVersionMajor << "." << m.protocolVersionMinor << "\"\n"
       << "}";
    return ss.str();
}

} // namespace aegis::production

