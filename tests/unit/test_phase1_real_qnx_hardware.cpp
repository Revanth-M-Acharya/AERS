/**
 * @file test_phase1_real_qnx_hardware.cpp
 * @brief SMART AEGIS (AERS) Phase 1 - Real QNX Hardware Layer Validation
 *
 * Validates Phase 1 requirements:
 * 1. Real RaspberryPi4BSP API (SetPin, GetPin, ConfigurePin, TogglePin, Cleanup)
 * 2. BCM2711 physical register mappings and constants (0xFE200000 base, GPFSEL, GPSET, GPCLR, GPLEV)
 * 3. GpioReservationManager duplicate pin prevention, invalid pin rejection, and conflict detection
 * 4. RelayController active-low driving (HIGH = OFF, LOW = ON), startup all-OFF guarantee
 * 5. Mutual exclusion interlock between Road A Green (GPIO 22) and Road B Green (GPIO 25)
 * 6. IntersectionHardware deterministic initialization, readiness check, and safe shutdown cleanup
 */

#include "bsp/RaspberryPi4BSP.h"
#include "aegis/hardware/GpioReservationManager.hpp"
#include "hardware/RelayController.h"
#include "hardware/IntersectionHardware.h"

#include <iostream>
#include <cassert>
#include <vector>

using namespace aegis;
using namespace aegis::bsp;
using namespace aegis::hardware;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << msg << " (" << #cond << ") at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while (0)

void Test1_BspPhase1Api() {
    std::cout << "[TEST 1] Testing RaspberryPi4BSP Phase 1 API (SetPin, GetPin, ConfigurePin, TogglePin, Cleanup)..." << std::endl;

    RaspberryPi4BSP bsp;
    TEST_ASSERT(bsp.InitializeBoard(), "Board initialization must succeed");

    // Verify BCM2711 Constants
    TEST_ASSERT(BCM2711_PERIPH_PHYS_BASE == 0xFE000000UL, "BCM2711 peripheral base must be 0xFE000000");
    TEST_ASSERT(BCM2711_GPIO_PHYS_BASE == 0xFE200000UL, "BCM2711 GPIO physical base must be 0xFE200000");
    TEST_ASSERT(BCM2711_GPIO_BLOCK_SIZE == 0x1000UL, "BCM2711 GPIO block size must be 4KB (0x1000)");

    // Reservation & Configuration
    TEST_ASSERT(bsp.ReserveGPIO(17, "TEST_PHASE1"), "Reserve GPIO 17");
    TEST_ASSERT(bsp.ConfigurePin(17, PinMode::OUTPUT), "ConfigurePin(17, OUTPUT) must succeed");

    // SetPin & GetPin
    TEST_ASSERT(bsp.SetPin(17, PinState::HIGH), "SetPin HIGH");
    TEST_ASSERT(bsp.GetPin(17) == PinState::HIGH, "GetPin must return HIGH");

    TEST_ASSERT(bsp.SetPin(17, false), "SetPin(17, false) sets LOW");
    TEST_ASSERT(bsp.GetPin(17) == PinState::LOW, "GetPin must return LOW");

    TEST_ASSERT(bsp.SetPin(17, true), "SetPin(17, true) sets HIGH");
    TEST_ASSERT(bsp.GetPin(17) == PinState::HIGH, "GetPin must return HIGH");

    // TogglePin
    TEST_ASSERT(bsp.TogglePin(17), "TogglePin must succeed");
    TEST_ASSERT(bsp.GetPin(17) == PinState::LOW, "GetPin after toggle must return LOW");

    // Cleanup
    TEST_ASSERT(bsp.Cleanup(), "Cleanup must succeed and unmap hardware resources");
    TEST_ASSERT(!bsp.IsBoardInitialized(), "Board must be uninitialized after Cleanup");

    std::cout << "  ✓ PASS: RaspberryPi4BSP Phase 1 API verified." << std::endl;
}

void Test2_GpioReservationManagerStrictValidation() {
    std::cout << "[TEST 2] Testing GpioReservationManager Strict Validation & Conflict Detection..." << std::endl;

    auto& mgr = GpioReservationManager::Instance();
    mgr.Reset();

    // Valid range is 0..27 on BCM2711 40-pin header
    TEST_ASSERT(mgr.ValidateBcmPin(0), "Pin 0 is valid");
    TEST_ASSERT(mgr.ValidateBcmPin(27), "Pin 27 is valid");
    TEST_ASSERT(!mgr.ValidateBcmPin(28), "Pin 28 exceeds 40-pin header range");
    TEST_ASSERT(!mgr.ValidateBcmPin(57), "Pin 57 exceeds header range");

    // Candidate set duplicate check
    std::string conflictReason;
    std::vector<uint32_t> duplicateSet = {17, 22, 17};
    TEST_ASSERT(!mgr.CanReservePins(duplicateSet, conflictReason), "Candidate set with intra-duplicates must be rejected");
    TEST_ASSERT(!conflictReason.empty(), "Conflict reason must be reported for duplicate pin request");

    // Reserve valid pin
    TEST_ASSERT(mgr.ReservePin(22, "RELAY", "Road A Green", PinMode::OUTPUT), "Reserve GPIO 22");
    TEST_ASSERT(mgr.IsPinReserved(22), "Pin 22 must be reserved");

    // Reject duplicate reservation
    TEST_ASSERT(!mgr.ReservePin(22, "OTHER", "Conflict", PinMode::OUTPUT), "Duplicate reservation must fail");

    // CanReservePins conflict check
    std::vector<uint32_t> testSet = {23, 24, 22};
    TEST_ASSERT(!mgr.CanReservePins(testSet, conflictReason), "Candidate set with already-reserved pin 22 must be rejected");

    mgr.Reset();
    std::cout << "  ✓ PASS: GpioReservationManager strict validation verified." << std::endl;
}

void Test3_RelayControllerActiveLowAndConflictInterlock() {
    std::cout << "[TEST 3] Testing RelayController Active-Low Driving & Green Conflict Prevention..." << std::endl;

    auto bsp = std::make_shared<RaspberryPi4BSP>();
    bsp->InitializeBoard();

    RelayController relayCtrl(bsp);

    // 4-Channel Setup matching intersection hardware
    std::vector<RelayChannelDef> channels = {
        {1, 17, "ROAD_A_RED", true, false},
        {2, 22, "ROAD_A_GREEN", true, false},
        {3, 23, "ROAD_B_RED", true, false},
        {4, 25, "ROAD_B_GREEN", true, false}
    };

    TEST_ASSERT(relayCtrl.Initialize(channels, true), "Relay initialization must succeed");

    // Requirement 4: All relays OFF during startup
    for (uint8_t ch = 1; ch <= 4; ++ch) {
        TEST_ASSERT(!relayCtrl.IsRelayEnergized(ch), "Relay channel must initially be DE-ENERGIZED (OFF)");
    }

    // Active-Low Verification: When OFF (de-energized), GPIO must be HIGH (3.3V)
    TEST_ASSERT(bsp->GetPin(17) == PinState::HIGH, "De-energized active-low relay pin must be HIGH");
    TEST_ASSERT(bsp->GetPin(22) == PinState::HIGH, "De-energized active-low relay pin must be HIGH");

    // Energize Channel 2 (Road A Green, GPIO 22)
    TEST_ASSERT(relayCtrl.SetRelay(2, true), "Energize Road A Green");
    TEST_ASSERT(relayCtrl.IsRelayEnergized(2), "Channel 2 must be energized");
    TEST_ASSERT(bsp->GetPin(22) == PinState::LOW, "Energized active-low relay pin must be LOW (0V)");

    // CRITICAL REQUIREMENT 4: Prevent conflict between Road A Green (GPIO 22) and Road B Green (GPIO 25)
    // Attempt to energize Road B Green while Road A Green is active -> MUST BE STRICTLY REJECTED!
    TEST_ASSERT(!relayCtrl.SetRelay(4, true), "CRITICAL: Energizing Road B Green while Road A Green is active must be REJECTED!");
    TEST_ASSERT(!relayCtrl.IsRelayEnergized(4), "Channel 4 (Road B Green) must NOT be energized");

    // Turn OFF Road A Green
    TEST_ASSERT(relayCtrl.SetRelay(2, false), "De-energize Road A Green");
    TEST_ASSERT(!relayCtrl.IsRelayEnergized(2), "Road A Green must be de-energized");

    // Now energizing Road B Green must succeed
    TEST_ASSERT(relayCtrl.SetRelay(4, true), "Energize Road B Green when Road A Green is off");
    TEST_ASSERT(relayCtrl.IsRelayEnergized(4), "Channel 4 must be energized");

    // Reverse check: Attempt to energize Road A Green while Road B Green is active -> MUST BE REJECTED!
    TEST_ASSERT(!relayCtrl.SetRelay(2, true), "CRITICAL: Energizing Road A Green while Road B Green is active must be REJECTED!");

    // De-energize all
    relayCtrl.DeenergizeAll();
    TEST_ASSERT(!relayCtrl.IsRelayEnergized(4), "All channels de-energized");
    TEST_ASSERT(bsp->GetPin(25) == PinState::HIGH, "De-energized active-low pin must be HIGH");

    relayCtrl.Shutdown();
    bsp->Cleanup();

    std::cout << "  ✓ PASS: Active-low driving and green conflict prevention verified." << std::endl;
}

void Test4_IntersectionHardwareStartupAndSafeShutdown() {
    std::cout << "[TEST 4] Testing IntersectionHardware Startup Verification & Safe Shutdown..." << std::endl;

    auto logger = std::make_shared<HardwareEventLogger>();
    IntersectionHardware hw("INT_PHASE1", logger);

    DualSignalPins pins;
    pins.roadARedPin = 17;
    pins.roadAYellowPin = 27;
    pins.roadAGreenPin = 22;
    pins.roadBRedPin = 23;
    pins.roadBYellowPin = 24;
    pins.roadBGreenPin = 25;

    std::vector<RelayChannelDef> relays = {
        {1, 17, "ROAD_A_RED", true, false},
        {2, 22, "ROAD_A_GREEN", true, false},
        {3, 23, "ROAD_B_RED", true, false},
        {4, 25, "ROAD_B_GREEN", true, false}
    };

    IrSensorParameters ir;
    ir.approachSensorPin = 12;
    ir.departureSensorPin = 13;

    TimingParameters timings;

    TEST_ASSERT(hw.Initialize(pins, relays, ir, timings), "IntersectionHardware Initialize must succeed");
    TEST_ASSERT(hw.GetBsp()->IsBoardInitialized(), "BSP must be initialized");
    TEST_ASSERT(hw.GetRelayController()->IsInitialized(), "RelayController must be initialized");

    // Safe shutdown
    hw.Shutdown();
    TEST_ASSERT(!hw.GetBsp()->IsBoardInitialized(), "BSP Cleanup must unmap hardware memory on shutdown");

    std::cout << "  ✓ PASS: IntersectionHardware startup and safe shutdown verified." << std::endl;
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 1 Real QNX Hardware Layer Unit Tests  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_BspPhase1Api();
    Test2_GpioReservationManagerStrictValidation();
    Test3_RelayControllerActiveLowAndConflictInterlock();
    Test4_IntersectionHardwareStartupAndSafeShutdown();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 4 PHASE 1 HARDWARE TESTS PASSED (100% SUCCESS)       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

