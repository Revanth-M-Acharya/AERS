/**
 * @file preemption_manager.hpp
 * @brief Discrete state manager tracking preemption activation status.
 */

#ifndef AEGIS_EDGE_PREEMPTION_MANAGER_HPP
#define AEGIS_EDGE_PREEMPTION_MANAGER_HPP

#include <mutex>

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Manages emergency preemption status.
 * Encapsulates emergency activation state without coupling to timing or GPIO.
 */
class PreemptionManager {
public:
    PreemptionManager();
    ~PreemptionManager() = default;

    void StartEmergency();
    void CancelEmergency();
    bool IsEmergencyActive() const noexcept;

private:
    mutable std::mutex m_mutex;
    bool m_emergencyActive{false};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_PREEMPTION_MANAGER_HPP
