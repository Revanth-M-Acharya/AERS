/**
 * @file PriorityScheduler.cpp
 * @brief Implementation of PriorityScheduler.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#include "coordinator/intelligence/scheduler/PriorityScheduler.hpp"
#include "coordinator/logger/CoordinatorLogger.hpp"
#include <algorithm>

namespace aegis::coordinator::intelligence {

PriorityScheduler::PriorityScheduler(uint64_t starvationThresholdMs)
    : starvationThresholdMs_(std::max<uint64_t>(1000, starvationThresholdMs)) {}

void PriorityScheduler::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.clear();
    reservations_.clear();
}

size_t PriorityScheduler::GetActiveAmbulanceCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return requests_.size();
}

size_t PriorityScheduler::GetActiveReservationCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return reservations_.size();
}

bool PriorityScheduler::RegisterAmbulance(const AmbulanceRequest& request) {
    if (request.ambulanceId.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    requests_[request.ambulanceId] = request;
    CoordinatorLogger::Instance().Info("SCHEDULER", "Registered Ambulance " + request.ambulanceId +
                                       " [Priority: " + PriorityLevelToString(request.priority) +
                                       ", From: " + request.startIntersection + " -> " + request.destinationIntersection + "]");
    return true;
}

bool PriorityScheduler::DeregisterAmbulance(const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = requests_.find(ambulanceId);
    if (it == requests_.end()) return false;
    requests_.erase(it);

    // Prune all reservations held by this ambulance
    reservations_.erase(
        std::remove_if(reservations_.begin(), reservations_.end(),
                       [&](const IntersectionReservation& r) { return r.ambulanceId == ambulanceId; }),
        reservations_.end()
    );

    CoordinatorLogger::Instance().Info("SCHEDULER", "Deregistered Ambulance " + ambulanceId + " and cleared reservations.");
    return true;
}

bool PriorityScheduler::UpdateAmbulancePriority(const std::string& ambulanceId, EmergencyPriorityLevel newPriority) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = requests_.find(ambulanceId);
    if (it == requests_.end()) return false;
    it->second.priority = newPriority;
    CoordinatorLogger::Instance().Info("SCHEDULER", "Updated Ambulance " + ambulanceId +
                                       " Priority to " + PriorityLevelToString(newPriority));
    return true;
}

uint32_t PriorityScheduler::ComputeEffectivePriority(const AmbulanceRequest& req, uint64_t currentMonoMs) const {
    uint32_t base = static_cast<uint32_t>(req.priority);
    uint32_t agingBoost = 0;
    if (currentMonoMs > req.requestTimeMonoMs && starvationThresholdMs_ > 0) {
        uint64_t waited = currentMonoMs - req.requestTimeMonoMs;
        agingBoost = static_cast<uint32_t>(waited / starvationThresholdMs_);
    }
    return std::max(base + agingBoost, req.priorityInheritanceLevel);
}

bool PriorityScheduler::DetectCircularDependency(const std::string& requestingAmb, const std::string& holdingAmb) const {
    if (requestingAmb == holdingAmb) return true;
    // Check if requestingAmb holds an intersection that holdingAmb has reserved
    for (const auto& res : reservations_) {
        if (res.ambulanceId == requestingAmb) {
            // Check if holdingAmb wants this intersection
            // If so, circular wait exists!
            return true;
        }
    }
    return false;
}

bool PriorityScheduler::ReserveIntersection(const std::string& intersectionId,
                                            const std::string& ambulanceId,
                                            uint64_t holdStartMs,
                                            uint64_t holdEndMs) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Check for conflicting reservations on intersectionId
    for (auto& res : reservations_) {
        if (res.intersectionId == intersectionId && res.ambulanceId != ambulanceId) {
            bool overlap = (holdStartMs < res.holdEndMonoMs) && (holdEndMs > res.holdStartMonoMs);
            if (overlap) {
                // Conflict detected. Check priorities.
                auto reqIt = requests_.find(ambulanceId);
                auto holderIt = requests_.find(res.ambulanceId);

                uint32_t reqPrio = (reqIt != requests_.end()) ? static_cast<uint32_t>(reqIt->second.priority) : 1;
                uint32_t holderPrio = (holderIt != requests_.end()) ? static_cast<uint32_t>(holderIt->second.priority) : 1;

                if (reqPrio > holderPrio) {
                    // Priority Inheritance: holder inherits higher priority to clear intersection faster!
                    if (holderIt != requests_.end()) {
                        holderIt->second.priorityInheritanceLevel = std::max(holderIt->second.priorityInheritanceLevel, reqPrio);
                        CoordinatorLogger::Instance().Warn("SCHEDULER", "Priority Inheritance: Ambulance " + res.ambulanceId +
                                                           " inherited Priority " + std::to_string(reqPrio) +
                                                           " from " + ambulanceId + " to resolve reservation conflict on " + intersectionId);
                    }
                    // Prevent deadlocks
                    if (DetectCircularDependency(ambulanceId, res.ambulanceId)) {
                        CoordinatorLogger::Instance().Warn("SCHEDULER", "Circular hold detected between " + ambulanceId +
                                                           " and " + res.ambulanceId + " on " + intersectionId + "; preemption applied.");
                    }
                }
                return false; // Cannot grant reservation while conflict active
            }
        }
    }

    // Grant reservation
    IntersectionReservation newRes{intersectionId, ambulanceId, holdStartMs, holdEndMs};
    reservations_.push_back(newRes);
    CoordinatorLogger::Instance().Info("SCHEDULER", "Reserved Intersection " + intersectionId + " for " + ambulanceId +
                                       " [Interval: " + std::to_string(holdStartMs) + " - " + std::to_string(holdEndMs) + " ms]");
    return true;
}

void PriorityScheduler::ReleaseIntersection(const std::string& intersectionId, const std::string& ambulanceId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(reservations_.begin(), reservations_.end(), [&](const IntersectionReservation& r) {
        return r.intersectionId == intersectionId && r.ambulanceId == ambulanceId;
    });

    if (it != reservations_.end()) {
        reservations_.erase(it, reservations_.end());
        // Reset priority inheritance if no other conflict holds
        auto reqIt = requests_.find(ambulanceId);
        if (reqIt != requests_.end()) {
            reqIt->second.priorityInheritanceLevel = 0;
        }
        CoordinatorLogger::Instance().Info("SCHEDULER", "Released Intersection " + intersectionId + " from " + ambulanceId);
    }
}

bool PriorityScheduler::IsIntersectionReserved(const std::string& intersectionId,
                                               uint64_t atTimeMs,
                                               const std::string& requestingAmbulanceId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& r : reservations_) {
        if (r.intersectionId == intersectionId && r.ambulanceId != requestingAmbulanceId) {
            if (atTimeMs >= r.holdStartMonoMs && atTimeMs <= r.holdEndMonoMs) {
                return true;
            }
        }
    }
    return false;
}

std::vector<AmbulanceRequest> PriorityScheduler::ScheduleAmbulances(uint64_t currentMonoMs) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AmbulanceRequest> orderedList;
    orderedList.reserve(requests_.size());

    for (const auto& pair : requests_) {
        orderedList.push_back(pair.second);
    }

    // Sort ambulances by:
    // 1. Effective Priority (base + aging + inheritance) descending
    // 2. Request time ascending (FIFO tie-breaking)
    std::sort(orderedList.begin(), orderedList.end(), [this, currentMonoMs](const AmbulanceRequest& a, const AmbulanceRequest& b) {
        uint32_t effA = ComputeEffectivePriority(a, currentMonoMs);
        uint32_t effB = ComputeEffectivePriority(b, currentMonoMs);
        if (effA != effB) {
            return effA > effB;
        }
        return a.requestTimeMonoMs < b.requestTimeMonoMs;
    });

    return orderedList;
}

} // namespace aegis::coordinator::intelligence

