/**
 * @file LampController.hpp
 * @brief Intermediate hardware actuation layer translating optical aspects to relay activations.
 * Contains ZERO TCP / networking logic.
 * @copyright (c) 2026 SMART AEGIS Project. All rights reserved.
 */

#pragma once

#include "traffic_node/relay_controller/RelayController.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace aegis::traffic {

class LampController {
public:
    explicit LampController(std::shared_ptr<RelayController> relayController);
    ~LampController() = default;

    // Prevent copying
    LampController(const LampController&) = delete;
    LampController& operator=(const LampController&) = delete;

    bool Initialize(uint32_t switchDelayMs = 50);
    void Shutdown();

    // High-Level Lamp Displays
    void DisplayRed();
    void DisplayYellow();
    void DisplayGreen();
    void DisplayAllRed();
    void DisplayFlashingYellow();
    void DisplayDark();

    // Lamp State Inquiries
    bool IsRedLit() const;
    bool IsYellowLit() const;
    bool IsGreenLit() const;
    bool IsAuxLit() const;

    std::shared_ptr<RelayController> GetRelayController() const { return relayController_; }

private:
    std::shared_ptr<RelayController> relayController_;
    uint32_t switchDelayMs_{50};
    mutable std::mutex mutex_;
    std::atomic<bool> initialized_{false};
};

} // namespace aegis::traffic

