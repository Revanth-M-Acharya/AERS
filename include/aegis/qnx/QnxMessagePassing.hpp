/**
 * @file QnxMessagePassing.hpp
 * @brief SMART AEGIS (AERS) Phase 4 - Native QNX Neutrino Message Passing IPC.
 *
 * Implements native QNX synchronous message passing:
 * - ChannelCreate(), ChannelDestroy()
 * - ConnectAttach(), ConnectDetach()
 * - MsgSend(), MsgReceive(), MsgReply()
 *
 * Provides dedicated message schemas for Coordinator <-> Intersection Node IPC:
 * - Heartbeat
 * - Registration
 * - Corridor Requests
 * - Signal Commands
 * - Status Updates
 * - Recovery Messages
 *
 * Synchronous rendezvous semantics: client blocks until server replies.
 * Full dual-mode support: Native QNX Neutrino syscalls on QNX SDP 8.0,
 * and high-fidelity POSIX simulation for host automated test execution.
 *
 * Platform: Raspberry Pi 4 Model B | QNX SDP 8.0 (aarch64le) | Neutrino RTOS
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#ifndef AEGIS_QNX_MESSAGE_PASSING_HPP
#define AEGIS_QNX_MESSAGE_PASSING_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#include "aegis/common/time_utils.hpp"
#include "aegis/common/qnx_compat.hpp"

#if defined(__QNX__) || defined(__QNXNTO__)
    #include <sys/neutrino.h>
    #include <sys/netmgr.h>
#endif

namespace aegis {
namespace qnx {

/**
 * @brief Categorization of QNX IPC message types.
 */
enum class QnxMsgType : uint32_t {
    HEARTBEAT         = 0x1001,
    REGISTRATION      = 0x1002,
    CORRIDOR_REQUEST  = 0x1003,
    SIGNAL_COMMAND    = 0x1004,
    STATUS_UPDATE     = 0x1005,
    RECOVERY_MESSAGE  = 0x1006,
    IPC_ACK           = 0x2001,
    IPC_NACK          = 0x2002
};

inline const char* QnxMsgTypeToString(QnxMsgType type) noexcept {
    switch (type) {
        case QnxMsgType::HEARTBEAT:        return "HEARTBEAT";
        case QnxMsgType::REGISTRATION:     return "REGISTRATION";
        case QnxMsgType::CORRIDOR_REQUEST: return "CORRIDOR_REQUEST";
        case QnxMsgType::SIGNAL_COMMAND:   return "SIGNAL_COMMAND";
        case QnxMsgType::STATUS_UPDATE:    return "STATUS_UPDATE";
        case QnxMsgType::RECOVERY_MESSAGE: return "RECOVERY_MESSAGE";
        case QnxMsgType::IPC_ACK:          return "IPC_ACK";
        case QnxMsgType::IPC_NACK:         return "IPC_NACK";
        default:                           return "UNKNOWN";
    }
}

#pragma pack(push, 1)

/**
 * @brief Standard QNX message header prepended to every IPC packet.
 */
struct QnxMsgHeader {
    QnxMsgType type{QnxMsgType::HEARTBEAT};
    char senderId[32]{0};
    char receiverId[32]{0};
    uint32_t payloadSize{0};
    uint64_t timestampUs{0};
    uint32_t sequenceNumber{0};
};

/**
 * @brief Structured reply message returned via MsgReply().
 */
struct QnxReplyMsg {
    QnxMsgType type{QnxMsgType::IPC_ACK};
    int32_t status{0};                 ///< 0 = success, negative = error
    char statusMessage[64]{0};
    uint64_t replyTimestampUs{0};
};

// =============================================================================
// Message Payloads
// =============================================================================

struct QnxHeartbeatPayload {
    uint64_t heartbeatTimeUs;
    double cpuTemp;
    double cpuLoad;
};

struct QnxRegistrationPayload {
    char nodeId[32];
    char role[32];
    char ipAddress[32];
    char macAddress[32];
    uint32_t swVersion;
};

struct QnxCorridorRequestPayload {
    char corridorId[32];
    char ambulanceId[32];
    char targetIntersection[32];
    double etaSec;
    double requestedGreenDurationSec;
};

struct QnxSignalCommandPayload {
    char command[32];
    char targetIntersection[32];
    double durationSec;
    uint32_t preemptionCode;
};

struct QnxStatusUpdatePayload {
    char nodeId[32];
    char currentPhase[32];
    bool isPreempted;
    uint32_t passedVehicleCount;
};

struct QnxRecoveryMsgPayload {
    char faultType[32];
    char targetSubsystem[32];
    uint32_t actionCode;
};

/**
 * @brief Generic envelope supporting up to 512-byte payload.
 */
struct QnxEnvelope {
    QnxMsgHeader header;
    uint8_t payload[512];
};

#pragma pack(pop)

// Forward declarations for simulation
struct SimulatedChannel;

/**
 * @class QnxChannelServer
 * @brief Native QNX IPC Channel Server (Coordinator / Intersection Endpoint).
 */
class QnxChannelServer {
public:
    explicit QnxChannelServer(const std::string& serverName);
    ~QnxChannelServer();

    QnxChannelServer(const QnxChannelServer&) = delete;
    QnxChannelServer& operator=(const QnxChannelServer&) = delete;

    /**
     * @brief Creates the native QNX channel.
     * @return Channel ID (chid >= 0), or -1 on error.
     */
    int CreateChannel();

    /**
     * @brief Destroys the QNX channel.
     */
    void DestroyChannel();

    /**
     * @brief Receives a message (blocks until client calls MsgSend).
     * @param outHeader Message header.
     * @param outPayload Buffer for payload bytes.
     * @param maxPayloadBytes Capacity of outPayload.
     * @param outRcvid QNX receive ID used to reply.
     * @return Number of payload bytes received, or -1 on error.
     */
    int Receive(QnxMsgHeader& outHeader, void* outPayload, size_t maxPayloadBytes, int& outRcvid);

    /**
     * @brief Replies to client unblocking MsgSend().
     */
    int Reply(int rcvid, const QnxReplyMsg& reply);

    int GetChannelId() const noexcept { return m_chid; }
    std::string GetServerName() const { return m_serverName; }
    uint64_t GetMessagesReceived() const noexcept { return m_msgCount; }

private:
    std::string m_serverName;
    int m_chid{-1};
    uint64_t m_msgCount{0};

#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::shared_ptr<SimulatedChannel> m_simChannel;
#endif
};

/**
 * @class QnxChannelClient
 * @brief Native QNX IPC Channel Client (ConnectAttach & MsgSend).
 */
class QnxChannelClient {
public:
    explicit QnxChannelClient(const std::string& clientName);
    ~QnxChannelClient();

    QnxChannelClient(const QnxChannelClient&) = delete;
    QnxChannelClient& operator=(const QnxChannelClient&) = delete;

    /**
     * @brief Attaches connection to target channel.
     */
    int Attach(int targetChid, const std::string& serverName = "");

    /**
     * @brief Detaches connection.
     */
    void Detach();

    /**
     * @brief Synchronous MsgSend: sends request and blocks until server replies.
     */
    int Send(const QnxMsgHeader& header, const void* payload, size_t payloadSize, QnxReplyMsg& outReply);

    int GetConnectionId() const noexcept { return m_coid; }
    std::string GetClientName() const { return m_clientName; }
    uint64_t GetMessagesSent() const noexcept { return m_sendCount; }
    double GetLastLatencyUs() const noexcept { return m_lastLatencyUs; }

private:
    std::string m_clientName;
    int m_coid{-1};
    int m_targetChid{-1};
    uint64_t m_sendCount{0};
    double m_lastLatencyUs{0.0};

#if !defined(__QNX__) && !defined(__QNXNTO__)
    std::shared_ptr<SimulatedChannel> m_simChannel;
#endif
};

} // namespace qnx
} // namespace aegis

#endif // AEGIS_QNX_MESSAGE_PASSING_HPP

