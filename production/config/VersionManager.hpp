/**
 * @file VersionManager.hpp
 * @brief System versioning, build metadata, and protocol compatibility verification.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include <string>
#include <cstdint>

namespace aegis::production {

struct BuildMetadata {
    std::string majorVersion{"1"};
    std::string minorVersion{"0"};
    std::string patchVersion{"0"};
    std::string releaseTag{"v1.0-hackathon-core"};
    std::string phaseTag{"Phase 9 Production Readiness"};
    std::string targetPlatform{"Raspberry Pi 4 Model B (4 GB)"};
    std::string targetOs{"QNX Neutrino RTOS 8.0 Starter Kit (aarch64le)"};
    std::string compilerVersion{"ISO C++20 / Clang / GCC"};
    std::string buildTimestamp{"2026-10-31 02:45:00 UTC"};
    uint32_t protocolVersionMajor{1};
    uint32_t protocolVersionMinor{0};
};

class VersionManager {
public:
    static BuildMetadata GetBuildMetadata();
    static std::string GetFullVersionString();
    static bool IsProtocolCompatible(uint32_t peerMajor, uint32_t peerMinor);
    static std::string ToJson();
};

} // namespace aegis::production

