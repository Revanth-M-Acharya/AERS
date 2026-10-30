/**
 * @file QnxInterruptManager.cpp
 * @brief Implementation of Phase 4 GPIO Interrupt-Driven Event Management.
 */

#include "aegis/qnx/QnxInterruptManager.hpp"
#include "aegis/qnx/QnxPulseManager.hpp"
#include "aegis/common/time_utils.hpp"
#include "aegis/logger/logger.hpp"

namespace aegis {
namespace qnx {

QnxInterruptManager::QnxInterruptManager() = default;

QnxInterruptManager::~QnxInterruptManager() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_interruptsById) {
#if defined(__QNX__) || defined(__QNXNTO__)
        if (pair.second.state != InterruptState::DETACHED) {
            InterruptDetach(pair.second.interruptId);
        }
#else
        pair.second.state = InterruptState::DETACHED;
#endif
    }
    m_interruptsById.clear();
    m_pinToId.clear();
}

int QnxInterruptManager::AttachGpioInterrupt(int gpioPin, int coid, int8_t pulseCode, int32_t pulseValue, int priority) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // If pin already attached, return existing or error
    if (m_pinToId.find(gpioPin) != m_pinToId.end()) {
        return -1;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    struct sigevent event;
    SIGEV_PULSE_INIT(&event, coid, priority, pulseCode, pulseValue);

    int intrId = InterruptAttachEvent(gpioPin, &event, _NTO_INTR_FLAGS_TRK_MSK);
    if (intrId < 0) {
        return -1;
    }
#else
    int intrId = m_nextInterruptId++;
#endif

    InterruptInfo info;
    info.interruptId = intrId;
    info.gpioPin = gpioPin;
    info.coid = coid;
    info.pulseCode = pulseCode;
    info.pulseValue = pulseValue;
    info.priority = priority;
    info.state = InterruptState::ATTACHED;
    info.triggerCount = 0;
    info.lastTriggerTimestampUs = 0;

    m_interruptsById[intrId] = info;
    m_pinToId[gpioPin] = intrId;

    return intrId;
}

int QnxInterruptManager::DetachInterrupt(int interruptId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_interruptsById.find(interruptId);
    if (it == m_interruptsById.end()) {
        return -1;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    if (InterruptDetach(interruptId) < 0) {
        return -1;
    }
#endif

    m_pinToId.erase(it->second.gpioPin);
    it->second.state = InterruptState::DETACHED;
    m_interruptsById.erase(it);
    return 0;
}

int QnxInterruptManager::MaskInterrupt(int interruptId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_interruptsById.find(interruptId);
    if (it == m_interruptsById.end()) {
        return -1;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    InterruptMask(it->second.gpioPin, interruptId);
#endif
    it->second.state = InterruptState::MASKED;
    return 0;
}

int QnxInterruptManager::UnmaskInterrupt(int interruptId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_interruptsById.find(interruptId);
    if (it == m_interruptsById.end()) {
        return -1;
    }

#if defined(__QNX__) || defined(__QNXNTO__)
    InterruptUnmask(it->second.gpioPin, interruptId);
#endif
    it->second.state = InterruptState::ATTACHED;
    return 0;
}

int QnxInterruptManager::SimulateInterrupt(int gpioPin) {
    InterruptInfo info;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto pinIt = m_pinToId.find(gpioPin);
        if (pinIt == m_pinToId.end()) {
            return -1;
        }

        auto it = m_interruptsById.find(pinIt->second);
        if (it == m_interruptsById.end() || it->second.state != InterruptState::ATTACHED) {
            return -1; // Masked or detached
        }

        it->second.triggerCount++;
        it->second.lastTriggerTimestampUs = aegis::common::GetMonotonicMicroseconds();
        info = it->second;
    }

    // Trigger non-blocking pulse to target coid
    return QnxPulseManager::SendPulse(info.coid, info.priority, info.pulseCode, info.pulseValue);
}

bool QnxInterruptManager::GetInterruptInfo(int interruptId, InterruptInfo& outInfo) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_interruptsById.find(interruptId);
    if (it != m_interruptsById.end()) {
        outInfo = it->second;
        return true;
    }
    return false;
}

bool QnxInterruptManager::GetInterruptInfoByPin(int gpioPin, InterruptInfo& outInfo) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto pinIt = m_pinToId.find(gpioPin);
    if (pinIt != m_pinToId.end()) {
        auto it = m_interruptsById.find(pinIt->second);
        if (it != m_interruptsById.end()) {
            outInfo = it->second;
            return true;
        }
    }
    return false;
}

std::vector<InterruptInfo> QnxInterruptManager::GetAllInterrupts() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<InterruptInfo> result;
    result.reserve(m_interruptsById.size());
    for (const auto& pair : m_interruptsById) {
        result.push_back(pair.second);
    }
    return result;
}

uint64_t QnxInterruptManager::GetTotalTriggerCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t total = 0;
    for (const auto& pair : m_interruptsById) {
        total += pair.second.triggerCount;
    }
    return total;
}

} // namespace qnx
} // namespace aegis
