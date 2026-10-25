/**
 * @file recovery_manager.hpp
 * @brief Discrete state manager tracking post-preemption recovery cycle status.
 */

#ifndef AEGIS_EDGE_RECOVERY_MANAGER_HPP
#define AEGIS_EDGE_RECOVERY_MANAGER_HPP

#include <mutex>

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Manages recovery transition state after emergency preemption clearance.
 * Only stores and manages recovery state without coupling to timing or GPIO.
 */
class RecoveryManager {
public:
    RecoveryManager();
    ~RecoveryManager() = default;

    void StartRecovery();
    void RecoveryFinished();
    void Reset();

    bool IsRecoveryActive() const noexcept;

private:
    mutable std::mutex m_mutex;
    bool m_recoveryActive{false};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_RECOVERY_MANAGER_HPP
