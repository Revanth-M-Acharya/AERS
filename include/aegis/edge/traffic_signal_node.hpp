/**
 * @file traffic_signal_node.hpp
 * @brief High-level edge node coordinating the traffic controller lifecycle and execution loop.
 */

#ifndef AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP
#define AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP

#include <memory>
#include <atomic>
#include <cstdint>

#include "aegis/edge/traffic_controller.hpp"

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Roadside traffic signal node encapsulating controller lifecycle, deterministic stepping, and execution.
 */
class TrafficSignalNode {
public:
    TrafficSignalNode();
    explicit TrafficSignalNode(std::unique_ptr<TrafficController> controller);
    explicit TrafficSignalNode(const TrafficTimingConfig& config);
    ~TrafficSignalNode();

    // Non-copyable, non-movable daemon node
    TrafficSignalNode(const TrafficSignalNode&) = delete;
    TrafficSignalNode& operator=(const TrafficSignalNode&) = delete;
    TrafficSignalNode(TrafficSignalNode&&) = delete;
    TrafficSignalNode& operator=(TrafficSignalNode&&) = delete;

    /**
     * @brief Starts the underlying traffic controller and arms the execution state.
     * @return true if started successfully.
     */
    bool Start();

    /**
     * @brief Stops the traffic controller and execution loop.
     */
    void Stop();

    /**
     * @brief Executes the signal node cycle.
     * @param iterations Number of iterations to execute (0 = run continuously until Stop() is called).
     * @param stepMs Delta time in milliseconds advanced per tick step.
     */
    void Run(uint32_t iterations = 0, uint32_t stepMs = 100);

    /**
     * @brief Advances the traffic controller by a single time increment.
     * @param deltaMs Elapsed duration in milliseconds.
     */
    void Tick(uint32_t deltaMs = 100);

    /**
     * @brief Checks if the node is currently running.
     */
    bool IsRunning() const noexcept;

    /**
     * @brief Gets current active signal state.
     */
    SignalState GetCurrentState() const;

    /**
     * @brief Access underlying traffic controller.
     */
    TrafficController& GetController() noexcept;
    const TrafficController& GetController() const noexcept;

private:
    std::unique_ptr<TrafficController> m_controller;
    std::atomic<bool> m_running{false};
};

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_TRAFFIC_SIGNAL_NODE_HPP
