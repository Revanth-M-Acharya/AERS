/**
 * @file gpio_interface.hpp
 * @brief Hardware-independent GPIO abstraction interface and simulation implementation.
 */

#ifndef AEGIS_EDGE_GPIO_INTERFACE_HPP
#define AEGIS_EDGE_GPIO_INTERFACE_HPP

#include <string>
#include <memory>
#include <mutex>

namespace aegis {
namespace edge {
namespace traffic {

/**
 * @brief Abstract interface for traffic signal lamp actuation.
 * Decouples controller logic completely from physical SBC GPIO / relay hardware.
 */
class IGpioInterface {
public:
    virtual ~IGpioInterface() = default;

    virtual bool Initialize() = 0;
    virtual void SetRed() = 0;
    virtual void SetYellow() = 0;
    virtual void SetGreen() = 0;
    virtual void FlashYellow() = 0;
    virtual void FlashRed() = 0;
    virtual void AllOff() = 0;
    virtual void Shutdown() = 0;
};

/**
 * @brief Hardware-independent simulation GPIO driver logging all lamp state modifications.
 */
class SimulationGpioInterface : public IGpioInterface {
public:
    SimulationGpioInterface();
    ~SimulationGpioInterface() override;

    bool Initialize() override;
    void SetRed() override;
    void SetYellow() override;
    void SetGreen() override;
    void FlashYellow() override;
    void FlashRed() override;
    void AllOff() override;
    void Shutdown() override;

    std::string GetLastAction() const;
    bool IsInitialized() const noexcept;

private:
    mutable std::mutex m_mutex;
    std::string m_lastAction{"ALL_OFF"};
    bool m_isInitialized{false};
};

/**
 * @brief Factory function creating the simulation GPIO driver.
 */
std::unique_ptr<IGpioInterface> CreateSimulationGpio();

} // namespace traffic
} // namespace edge
} // namespace aegis

#endif // AEGIS_EDGE_GPIO_INTERFACE_HPP
