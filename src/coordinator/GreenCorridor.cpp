/**
 * @file GreenCorridor.cpp
 * @brief Implementation of GreenCorridor.
 */

#include "GreenCorridor.h"

namespace aegis {
namespace coordinator {

const char* CorridorStatusToString(CorridorStatus status) noexcept {
    switch (status) {
        case CorridorStatus::CREATED:   return "CREATED";
        case CorridorStatus::ACTIVE:    return "ACTIVE";
        case CorridorStatus::COMPLETED: return "COMPLETED";
        case CorridorStatus::CANCELLED: return "CANCELLED";
        default:                        return "UNKNOWN";
    }
}

CorridorStatus StringToCorridorStatus(const std::string& str) noexcept {
    if (str == "CREATED")   return CorridorStatus::CREATED;
    if (str == "ACTIVE")    return CorridorStatus::ACTIVE;
    if (str == "COMPLETED") return CorridorStatus::COMPLETED;
    if (str == "CANCELLED") return CorridorStatus::CANCELLED;
    return CorridorStatus::CREATED;
}

GreenCorridor::GreenCorridor()
    : m_corridorId("")
    , m_intersections()
    , m_currentIndex(0)
    , m_status(CorridorStatus::CREATED) {
}

GreenCorridor::GreenCorridor(std::string corridorId, std::vector<std::string> intersectionIds)
    : m_corridorId(std::move(corridorId))
    , m_intersections(std::move(intersectionIds))
    , m_currentIndex(0)
    , m_status(CorridorStatus::CREATED) {
}

GreenCorridor::GreenCorridor(GreenCorridor&& other) noexcept {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    m_corridorId = std::move(other.m_corridorId);
    m_intersections = std::move(other.m_intersections);
    m_currentIndex = other.m_currentIndex;
    m_status = other.m_status;
}

GreenCorridor& GreenCorridor::operator=(GreenCorridor&& other) noexcept {
    if (this != &other) {
        std::scoped_lock lock(m_mutex, other.m_mutex);
        m_corridorId = std::move(other.m_corridorId);
        m_intersections = std::move(other.m_intersections);
        m_currentIndex = other.m_currentIndex;
        m_status = other.m_status;
    }
    return *this;
}

bool GreenCorridor::CreateCorridor(const std::string& corridorId, const std::vector<std::string>& intersectionIds) {
    if (corridorId.empty() || intersectionIds.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_corridorId = corridorId;
    m_intersections = intersectionIds;
    m_currentIndex = 0;
    m_status = CorridorStatus::CREATED;
    return true;
}

bool GreenCorridor::Activate() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != CorridorStatus::CREATED || m_intersections.empty()) {
        return false;
    }

    m_status = CorridorStatus::ACTIVE;
    m_currentIndex = 0;
    return true;
}

bool GreenCorridor::AdvanceToNextIntersection() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != CorridorStatus::ACTIVE) {
        return false;
    }

    if (m_currentIndex + 1 < m_intersections.size()) {
        m_currentIndex++;
        return true;
    } else {
        // Reached end of corridor, transition to COMPLETED
        m_status = CorridorStatus::COMPLETED;
        return false;
    }
}

bool GreenCorridor::Complete() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != CorridorStatus::ACTIVE) {
        return false;
    }

    m_status = CorridorStatus::COMPLETED;
    return true;
}

bool GreenCorridor::Cancel() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status == CorridorStatus::COMPLETED || m_status == CorridorStatus::CANCELLED) {
        return false;
    }

    m_status = CorridorStatus::CANCELLED;
    return true;
}

std::string GreenCorridor::CurrentIntersection() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentIndex < m_intersections.size()) {
        return m_intersections[m_currentIndex];
    }
    return "";
}

std::string GreenCorridor::NextIntersection() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentIndex + 1 < m_intersections.size()) {
        return m_intersections[m_currentIndex + 1];
    }
    return "";
}

std::string GreenCorridor::GetCorridorId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_corridorId;
}

CorridorStatus GreenCorridor::GetStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

std::vector<std::string> GreenCorridor::GetIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections;
}

size_t GreenCorridor::GetCurrentIndex() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentIndex;
}

size_t GreenCorridor::GetTotalIntersections() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_intersections.size();
}

bool GreenCorridor::IsActive() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status == CorridorStatus::ACTIVE;
}

bool GreenCorridor::IsCompleted() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status == CorridorStatus::COMPLETED;
}

bool GreenCorridor::IsCancelled() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status == CorridorStatus::CANCELLED;
}

bool GreenCorridor::IsCreated() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status == CorridorStatus::CREATED;
}

void GreenCorridor::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_corridorId.clear();
    m_intersections.clear();
    m_currentIndex = 0;
    m_status = CorridorStatus::CREATED;
}

} // namespace coordinator
} // namespace aegis
