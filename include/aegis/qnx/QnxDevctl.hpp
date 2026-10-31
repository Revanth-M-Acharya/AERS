/**
 * @file QnxDevctl.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Industrial QNX devctl() Commands & Data Structures.
 *
 * Implements native QNX devctl() commands for roadside hardware and municipal coordinator control:
 * - GET_SYSTEM_STATUS
 * - GET_NODE_STATUS
 * - START_CORRIDOR
 * - STOP_CORRIDOR
 * - RESET_NODE
 * - READ_TELEMETRY
 * - READ_LOGS
 * - ENABLE_DIAGNOSTICS
 * - RUN_SELF_TEST
 *
 * Uses native QNX command packing macros (__DIOT, __DIOW, __DIOWR, __DIOF) with fallback definitions
 * for host automated testing.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_DEVCTL_HPP
#define AEGIS_QNX_DEVCTL_HPP

#include <cstdint>
#include <cstring>
#include <string>

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <devctl.h>
    #ifndef _POSIX_DEVDIR_NONE
        #define _POSIX_DEVDIR_NONE   0x00000000U
    #endif
    #ifndef _POSIX_DEVDIR_TO
        #define _POSIX_DEVDIR_TO     0x80000000U
    #endif
    #ifndef _POSIX_DEVDIR_FROM
        #define _POSIX_DEVDIR_FROM   0x40000000U
    #endif
    #ifndef _POSIX_DEVDIR_TOFROM
        #define _POSIX_DEVDIR_TOFROM (_POSIX_DEVDIR_TO | _POSIX_DEVDIR_FROM)
    #endif
    #ifndef __DION
        #define __DION(class, cmd) (((class) << 8) | (cmd))
    #endif
    #ifndef __DIOT
        #define __DIOT(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_TO) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOF
        #define __DIOF(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_FROM) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOTF
        #define __DIOTF(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_TOFROM) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOW
        #define __DIOW(class, cmd, data) __DIOT(class, cmd, data)
    #endif
    #ifndef __DIOWR
        #define __DIOWR(class, cmd, data) __DIOTF(class, cmd, data)
    #endif
#else
    // Native-compatible QNX devctl macro definitions for POSIX / host testing
    #ifndef _POSIX_DEVDIR_NONE
        #define _POSIX_DEVDIR_NONE   0x00000000U
    #endif
    #ifndef _POSIX_DEVDIR_TO
        #define _POSIX_DEVDIR_TO     0x80000000U
    #endif
    #ifndef _POSIX_DEVDIR_FROM
        #define _POSIX_DEVDIR_FROM   0x40000000U
    #endif
    #ifndef _POSIX_DEVDIR_TOFROM
        #define _POSIX_DEVDIR_TOFROM (_POSIX_DEVDIR_TO | _POSIX_DEVDIR_FROM)
    #endif
    #ifndef __DION
        #define __DION(class, cmd) (((class) << 8) | (cmd))
    #endif
    #ifndef __DIOT
        #define __DIOT(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_TO) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOF
        #define __DIOF(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_FROM) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOTF
        #define __DIOTF(class, cmd, data) \
            (static_cast<uint32_t>((_POSIX_DEVDIR_TOFROM) | ((sizeof(data) & 0xFFF) << 16) | (((class) & 0xFF) << 8) | ((cmd) & 0xFF)))
    #endif
    #ifndef __DIOW
        #define __DIOW(class, cmd, data) __DIOT(class, cmd, data)
    #endif
    #ifndef __DIOWR
        #define __DIOWR(class, cmd, data) __DIOTF(class, cmd, data)
    #endif
#endif

namespace aegis {
namespace qnx {

// Magic class identifier for SMART AEGIS devctl commands ('A' = 0x41)
constexpr int AEGIS_DEVCTL_CLASS = 'A';

#pragma pack(push, 1)

/**
 * @brief Payload for GET_SYSTEM_STATUS
 */
struct AegisSystemStatusPayload {
    char state[32];
    uint32_t activeCorridors;
    uint32_t activeNodes;
    uint64_t uptimeSec;
    double clusterScore;
};

/**
 * @brief Payload for GET_NODE_STATUS
 */
struct AegisNodeStatusPayload {
    char nodeId[32];
    char role[32];
    char ipAddress[32];
    char signalPhase[32];
    bool gpioHealthy;
    bool relayHealthy;
    double cpuTemp;
};

/**
 * @brief Payload for START_CORRIDOR
 */
struct AegisStartCorridorPayload {
    char corridorId[32];
    char ambulanceId[32];
    char routeStr[128];
    double etaSec;
    bool success;
};

/**
 * @brief Payload for STOP_CORRIDOR
 */
struct AegisStopCorridorPayload {
    char corridorId[32];
    char reason[64];
    bool success;
};

/**
 * @brief Payload for RESET_NODE
 */
struct AegisResetNodePayload {
    char nodeId[32];
    bool hardReset;
    bool success;
};

/**
 * @brief Payload for READ_TELEMETRY
 */
struct AegisTelemetryPayload {
    double cpuUsage;
    double ramUsageMB;
    double cpuTemp;
    uint32_t rxPackets;
    uint32_t txPackets;
    double networkLatencyMs;
};

/**
 * @brief Payload for READ_LOGS
 */
struct AegisLogsPayload {
    uint32_t logCount;
    char lastLogEntry[256];
};

/**
 * @brief Payload for ENABLE_DIAGNOSTICS
 */
struct AegisEnableDiagPayload {
    bool enable;
    uint32_t verboseLevel;
};

/**
 * @brief Payload for RUN_SELF_TEST
 */
struct AegisSelfTestPayload {
    bool relaysPassed;
    bool gpioPassed;
    bool networkPassed;
    bool overallSuccess;
    char details[128];
};

#pragma pack(pop)

// =============================================================================
// devctl Command Codes
// =============================================================================

#define DCMD_AEGIS_GET_SYSTEM_STATUS   __DIOF (AEGIS_DEVCTL_CLASS, 1, AegisSystemStatusPayload)
#define DCMD_AEGIS_GET_NODE_STATUS     __DIOTF(AEGIS_DEVCTL_CLASS, 2, AegisNodeStatusPayload)
#define DCMD_AEGIS_START_CORRIDOR      __DIOTF(AEGIS_DEVCTL_CLASS, 3, AegisStartCorridorPayload)
#define DCMD_AEGIS_STOP_CORRIDOR       __DIOTF(AEGIS_DEVCTL_CLASS, 4, AegisStopCorridorPayload)
#define DCMD_AEGIS_RESET_NODE          __DIOTF(AEGIS_DEVCTL_CLASS, 5, AegisResetNodePayload)
#define DCMD_AEGIS_READ_TELEMETRY      __DIOF (AEGIS_DEVCTL_CLASS, 6, AegisTelemetryPayload)
#define DCMD_AEGIS_READ_LOGS           __DIOF (AEGIS_DEVCTL_CLASS, 7, AegisLogsPayload)
#define DCMD_AEGIS_ENABLE_DIAGNOSTICS  __DIOT (AEGIS_DEVCTL_CLASS, 8, AegisEnableDiagPayload)
#define DCMD_AEGIS_RUN_SELF_TEST       __DIOTF(AEGIS_DEVCTL_CLASS, 9, AegisSelfTestPayload)

/**
 * @brief Invokes a devctl command on an open device file descriptor.
 * Uses native QNX devctl() when running on Neutrino RTOS, or simulated dispatcher on POSIX host.
 */
int InvokeDevctl(int fd, uint32_t dcmd, void* dataPtr, size_t nBytes, int* infoPtr = nullptr);
inline int InvokeDevctl(int fd, int dcmd, void* dataPtr, size_t nBytes, int* infoPtr = nullptr) {
    return InvokeDevctl(fd, static_cast<uint32_t>(dcmd), dataPtr, nBytes, infoPtr);
}

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_DEVCTL_HPP

