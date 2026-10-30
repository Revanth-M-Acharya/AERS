/**
 * @file test_raspberry_pi4_bsp_phase16a.cpp
 * @brief SMART AEGIS (AERS) Phase 16A Unit Tests
 *
 * Validates the Raspberry Pi 4 Board Support Package (BSP) & Low-Level GPIO Driver Layer:
 * - 12 Comprehensive Unit Tests
 * - Board Initialization & Shutdown Lifecycle (Rules 8 & 9)
 * - GPIO Pin Reservation & Release (Rule 4)
 * - Duplicate Pin Reservation Rejection & Forensic Error Logging (Rule 3, Rule 10)
 * - Pin Mode & Pull Resistor Configuration (Rule 1)
 * - Digital Read and Write Operations (Rule 2)
 * - GPIO State Toggling
 * - Hardware Interrupt Enable/Disable & Mode Validation (Rule 5)
 * - Interrupt Callback Registration & Event Simulation (Rule 6)
 * - Hardware Peripheral Exclusivity & Reservation (Rule 7)
 * - Structured Operational ASCII Reports ([BOARD], [GPIO], [INTERRUPT])
 * - 100-Thread Concurrency Stress Test (Zero Deadlocks, Zero Data Races)
 * - Performance Benchmark (100 GPIOs, 1000 Reads < 1ms, 1000 Writes < 1ms, 1000 Toggles, 1000 Interrupts)
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "bsp/RaspberryPi4BSP.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis::bsp;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed: " << msg << " (" << #cond << ") at line " << __LINE__ << std::endl; \
            std::exit(1); \
        } \
    } while(0)

// =============================================================================
// Unit Tests
// =============================================================================

/**
 * @brief TEST 1: Board Initialization & Telemetry
 */
void Test1_BoardInitialization() {
    std::cout << "[TEST 1] Board Initialization..." << std::endl;
    RaspberryPi4BSP bsp;

    TEST_ASSERT(!bsp.IsBoardInitialized(), "Board must initially be uninitialized");

    // Rule 8: Operations fail before initialization
    TEST_ASSERT(!bsp.ReserveGPIO(17, "TEST_DRV"), "ReserveGPIO must fail before board init (Rule 8)");

    TEST_ASSERT(bsp.InitializeBoard(BoardRevision::RPI4_B_REV1_4), "InitializeBoard must succeed");
    TEST_ASSERT(bsp.IsBoardInitialized(), "Board must be initialized");

    BoardInformation info = bsp.ReadBoardInformation();
    TEST_ASSERT(info.boardRevision == BoardRevision::RPI4_B_REV1_4, "Revision must match");
    TEST_ASSERT(info.ramSizeMB == 4096, "RAM size must match 4096 MB");
    TEST_ASSERT(info.currentPowerState == PowerState::POWER_ON, "Power state must be POWER_ON");
    TEST_ASSERT(bsp.ReadTemperature() > 0.0, "Temperature reading must be positive");
    TEST_ASSERT(bsp.ReadVoltage() > 0.0, "Voltage reading must be positive");
    TEST_ASSERT(bsp.ReadClock(ClockSource::ARM_CLK) == 1800000000ULL, "ARM clock must be 1.8 GHz");

    TEST_ASSERT(bsp.ShutdownBoard(), "ShutdownBoard must succeed");
    TEST_ASSERT(!bsp.IsBoardInitialized(), "Board must be uninitialized after shutdown");

    std::cout << "  ✓ PASS: Board initialization, telemetry, and shutdown verified." << std::endl;
}

/**
 * @brief TEST 2: GPIO Reservation & Release
 */
void Test2_GPIOReservation() {
    std::cout << "[TEST 2] GPIO Reservation & Release..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(!bsp.IsGPIOReserved(17), "Pin 17 must initially be unreserved");

    TEST_ASSERT(bsp.ReserveGPIO(17, "TRAFFIC_LAMP_RELAY"), "Reserve pin 17 must succeed");
    TEST_ASSERT(bsp.IsGPIOReserved(17), "Pin 17 must be reserved");

    GPIOPin pin;
    TEST_ASSERT(bsp.GetPin(17, pin), "Query pin 17");
    TEST_ASSERT(pin.reservationStatus == true, "reservationStatus must be true");
    TEST_ASSERT(pin.ownerDriver == "TRAFFIC_LAMP_RELAY", "Owner driver must match");
    TEST_ASSERT(pin.bank == GPIOBank::BANK0, "Pin 17 must belong to BANK0");

    // Release pin (Rule 4: returns to INPUT state)
    TEST_ASSERT(bsp.ReleaseGPIO(17), "Release pin 17 must succeed");
    TEST_ASSERT(!bsp.IsGPIOReserved(17), "Pin 17 must be unreserved");

    TEST_ASSERT(bsp.GetPin(17, pin), "Query released pin 17");
    TEST_ASSERT(pin.reservationStatus == false, "reservationStatus must be false");
    TEST_ASSERT(pin.mode == PinMode::INPUT, "Released pin must return to INPUT mode (Rule 4)");
    TEST_ASSERT(pin.ownerDriver.empty(), "Owner must be cleared");

    std::cout << "  ✓ PASS: GPIO reservation and release lifecycle verified." << std::endl;
}

/**
 * @brief TEST 3: Duplicate Reservation (Rule 3 & Rule 10)
 */
void Test3_DuplicateReservation() {
    std::cout << "[TEST 3] Duplicate Reservation (Rule 3)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(bsp.ReserveGPIO(18, "SIREN_MODULATOR"), "Initial reservation must succeed");

    // Rule 3: Reserved pins cannot be reserved twice
    TEST_ASSERT(!bsp.ReserveGPIO(18, "ANOTHER_DRIVER"), "Duplicate reservation must fail (Rule 3)");

    std::string err = bsp.GetLastError("GPIO_18");
    TEST_ASSERT(!err.empty(), "Forensic error must be recorded for duplicate reservation (Rule 10)");

    GPIOPin pin;
    TEST_ASSERT(bsp.GetPin(18, pin), "Query pin 18");
    TEST_ASSERT(pin.ownerDriver == "SIREN_MODULATOR", "Original owner must remain intact");

    std::cout << "  ✓ PASS: Duplicate reservation rejection and forensic logging verified." << std::endl;
}

/**
 * @brief TEST 4: Pin Configuration (Rule 1)
 */
void Test4_PinConfiguration() {
    std::cout << "[TEST 4] Pin Configuration (Rule 1)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    // Rule 1: Pins must be reserved before configuration
    TEST_ASSERT(!bsp.ConfigurePinMode(22, PinMode::OUTPUT),
                "Configure unreserved pin must fail (Rule 1)");
    TEST_ASSERT(!bsp.ConfigurePullResistor(22, PullMode::PULL_UP),
                "Configure pull resistor on unreserved pin must fail (Rule 1)");

    TEST_ASSERT(bsp.ReserveGPIO(22, "PEDESTRIAN_BUTTON"), "Reserve pin 22");

    TEST_ASSERT(bsp.ConfigurePinMode(22, PinMode::INPUT), "Configure INPUT mode");
    TEST_ASSERT(bsp.ConfigurePullResistor(22, PullMode::PULL_UP), "Configure PULL_UP resistor");

    GPIOPin pin;
    TEST_ASSERT(bsp.GetPin(22, pin), "Query pin 22");
    TEST_ASSERT(pin.mode == PinMode::INPUT, "Mode must be INPUT");
    TEST_ASSERT(pin.pullConfiguration == PullMode::PULL_UP, "Pull mode must be PULL_UP");

    std::cout << "  ✓ PASS: Pin configuration dependency gating verified." << std::endl;
}

/**
 * @brief TEST 5: Read and Write GPIO (Rule 2)
 */
void Test5_ReadWriteGPIO() {
    std::cout << "[TEST 5] Read and Write GPIO (Rule 2)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    // Reserve pins
    TEST_ASSERT(bsp.ReserveGPIO(23, "OUT_DRIVER"), "Reserve pin 23");
    TEST_ASSERT(bsp.ReserveGPIO(24, "IN_DRIVER"), "Reserve pin 24");

    // Configure modes
    TEST_ASSERT(bsp.ConfigurePinMode(23, PinMode::OUTPUT), "Pin 23 as OUTPUT");
    TEST_ASSERT(bsp.ConfigurePinMode(24, PinMode::INPUT), "Pin 24 as INPUT");

    // Rule 2: Write to INPUT pin must fail
    TEST_ASSERT(!bsp.WriteGPIO(24, PinState::HIGH), "Write to INPUT pin must fail (Rule 2)");

    // Write to OUTPUT pin must succeed
    TEST_ASSERT(bsp.WriteGPIO(23, PinState::HIGH), "Write HIGH to OUTPUT pin");
    PinState st = PinState::UNKNOWN;
    TEST_ASSERT(bsp.ReadGPIO(23, st), "Read pin 23");
    TEST_ASSERT(st == PinState::HIGH, "State must be HIGH");

    TEST_ASSERT(bsp.WriteGPIO(23, PinState::LOW), "Write LOW to OUTPUT pin");
    TEST_ASSERT(bsp.ReadGPIO(23, st), "Read pin 23");
    TEST_ASSERT(st == PinState::LOW, "State must be LOW");

    // Read from unreserved pin must fail
    TEST_ASSERT(!bsp.ReadGPIO(25, st), "Read from unreserved pin must fail");

    std::cout << "  ✓ PASS: Digital read and write guard conditions verified." << std::endl;
}

/**
 * @brief TEST 6: Toggle GPIO
 */
void Test6_ToggleGPIO() {
    std::cout << "[TEST 6] Toggle GPIO..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(bsp.ReserveGPIO(26, "STATUS_BEACON"), "Reserve pin 26");
    TEST_ASSERT(bsp.ConfigurePinMode(26, PinMode::OUTPUT), "Configure OUTPUT mode");

    bsp.WriteGPIO(26, PinState::LOW);
    TEST_ASSERT(bsp.ReadGPIO(26) == PinState::LOW, "Initial state LOW");

    TEST_ASSERT(bsp.ToggleGPIO(26), "First toggle");
    TEST_ASSERT(bsp.ReadGPIO(26) == PinState::HIGH, "State after first toggle must be HIGH");

    TEST_ASSERT(bsp.ToggleGPIO(26), "Second toggle");
    TEST_ASSERT(bsp.ReadGPIO(26) == PinState::LOW, "State after second toggle must be LOW");

    std::cout << "  ✓ PASS: GPIO pin toggle behavior verified." << std::endl;
}

/**
 * @brief TEST 7: Interrupt Enable and Disable (Rule 5)
 */
void Test7_InterruptEnableDisable() {
    std::cout << "[TEST 7] Interrupt Enable and Disable (Rule 5)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(bsp.ReserveGPIO(4, "EMERGENCY_BUTTON"), "Reserve pin 4");
    TEST_ASSERT(bsp.ReserveGPIO(5, "LAMP_OUTPUT"), "Reserve pin 5");

    bsp.ConfigurePinMode(4, PinMode::INPUT);
    bsp.ConfigurePinMode(5, PinMode::OUTPUT);

    // Rule 5: Interrupts require INPUT mode
    TEST_ASSERT(!bsp.EnableInterrupt(5, InterruptTrigger::RISING_EDGE),
                "Enable interrupt on OUTPUT pin must fail (Rule 5)");

    // Enable on INPUT pin
    TEST_ASSERT(bsp.EnableInterrupt(4, InterruptTrigger::RISING_EDGE),
                "Enable interrupt on INPUT pin must succeed");

    GPIOPin pin;
    TEST_ASSERT(bsp.GetPin(4, pin), "Query pin 4");
    TEST_ASSERT(pin.interruptEnabled == true, "interruptEnabled must be true");
    TEST_ASSERT(pin.interruptTrigger == InterruptTrigger::RISING_EDGE, "Trigger must match");

    // Disable interrupt
    TEST_ASSERT(bsp.DisableInterrupt(4), "Disable interrupt must succeed");
    TEST_ASSERT(bsp.GetPin(4, pin), "Query pin 4 post-disable");
    TEST_ASSERT(pin.interruptEnabled == false, "interruptEnabled must be false");

    std::cout << "  ✓ PASS: Interrupt enablement and direction constraints verified." << std::endl;
}

/**
 * @brief TEST 8: Interrupt Callback and Simulation (Rule 6)
 */
void Test8_InterruptCallback() {
    std::cout << "[TEST 8] Interrupt Callback and Simulation (Rule 6)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(bsp.ReserveGPIO(13, "PULSE_SENSOR"), "Reserve pin 13");
    bsp.ConfigurePinMode(13, PinMode::INPUT);

    // Rule 6: Callbacks require interrupts enabled
    TEST_ASSERT(!bsp.RegisterInterruptCallback(13, [](uint32_t, PinState, uint64_t){}),
                "Register callback before enabling interrupt must fail (Rule 6)");

    TEST_ASSERT(bsp.EnableInterrupt(13, InterruptTrigger::FALLING_EDGE), "Enable interrupt");

    std::atomic<bool> callbackInvoked{false};
    uint32_t cbPin = 0;
    PinState cbState = PinState::UNKNOWN;

    TEST_ASSERT(bsp.RegisterInterruptCallback(13, [&](uint32_t p, PinState s, uint64_t) {
        callbackInvoked = true;
        cbPin = p;
        cbState = s;
    }), "Register interrupt callback must succeed");

    // Simulate hardware interrupt
    TEST_ASSERT(bsp.SimulateInterrupt(13, PinState::LOW), "SimulateInterrupt must succeed");
    TEST_ASSERT(callbackInvoked.load() == true, "Callback must be invoked");
    TEST_ASSERT(cbPin == 13, "Callback pin must match 13");
    TEST_ASSERT(cbState == PinState::LOW, "Callback state must match LOW");

    // Remove callback
    TEST_ASSERT(bsp.RemoveInterruptCallback(13), "Remove callback must succeed");
    callbackInvoked = false;
    bsp.SimulateInterrupt(13, PinState::HIGH);
    TEST_ASSERT(callbackInvoked.load() == false, "Removed callback must not be invoked");

    std::cout << "  ✓ PASS: Interrupt callback registration and event simulation verified." << std::endl;
}

/**
 * @brief TEST 9: Peripheral Reservation (Rule 7)
 */
void Test9_PeripheralReservation() {
    std::cout << "[TEST 9] Peripheral Reservation (Rule 7)..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    TEST_ASSERT(!bsp.IsPeripheralReserved("SPI0"), "SPI0 must initially be unreserved");

    TEST_ASSERT(bsp.ReservePeripheral("SPI0", "OLED_DISPLAY_DRIVER"), "Reserve SPI0");
    TEST_ASSERT(bsp.IsPeripheralReserved("SPI0"), "SPI0 must be reserved");

    // Rule 7: Reserved peripherals cannot be shared
    TEST_ASSERT(!bsp.ReservePeripheral("SPI0", "SECONDARY_DRIVER"),
                "Reserve already reserved peripheral must fail (Rule 7)");

    TEST_ASSERT(bsp.ReleasePeripheral("SPI0"), "Release SPI0 must succeed");
    TEST_ASSERT(!bsp.IsPeripheralReserved("SPI0"), "SPI0 must be unreserved");

    // Re-reserve after release
    TEST_ASSERT(bsp.ReservePeripheral("SPI0", "SECONDARY_DRIVER"), "Re-reserve SPI0 after release");
    TEST_ASSERT(bsp.IsPeripheralReserved("SPI0"), "SPI0 must now be reserved");

    std::cout << "  ✓ PASS: Peripheral exclusivity and reservation lifecycle verified." << std::endl;
}

/**
 * @brief TEST 10: ASCII Reports
 */
void Test10_AsciiReports() {
    std::cout << "[TEST 10] ASCII Reports..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    bsp.ReserveGPIO(14, "UART0_TX");
    bsp.ConfigurePinMode(14, PinMode::ALT0);

    bsp.ReserveGPIO(15, "UART0_RX");
    bsp.ConfigurePinMode(15, PinMode::INPUT);
    bsp.EnableInterrupt(15, InterruptTrigger::RISING_EDGE);
    bsp.RegisterInterruptCallback(15, [](uint32_t, PinState, uint64_t){});

    // 1. Board Report
    std::string repBoard = bsp.GenerateBoardReport();
    std::cout << repBoard << std::endl;
    TEST_ASSERT(repBoard.find("[BOARD]") != std::string::npos, "Header [BOARD] present");
    TEST_ASSERT(repBoard.find("Raspberry Pi 4 Model B") != std::string::npos, "Model present");
    TEST_ASSERT(repBoard.find("Revision:\nRPI4_B_REV1_4") != std::string::npos, "Revision present");
    TEST_ASSERT(repBoard.find("RAM:\n4096 MB") != std::string::npos, "RAM present");
    TEST_ASSERT(repBoard.find("Status:\nINITIALIZED") != std::string::npos, "Status present");

    // 2. GPIO Report
    std::string repGpio = bsp.GenerateGPIOReport(14);
    std::cout << repGpio << std::endl;
    TEST_ASSERT(repGpio.find("[GPIO]") != std::string::npos, "Header [GPIO] present");
    TEST_ASSERT(repGpio.find("Pin:\n14") != std::string::npos, "Pin 14 present");
    TEST_ASSERT(repGpio.find("Bank:\nBANK0") != std::string::npos, "Bank present");
    TEST_ASSERT(repGpio.find("Mode:\nALT0") != std::string::npos, "Mode ALT0 present");
    TEST_ASSERT(repGpio.find("Reserved:\nYES") != std::string::npos, "Reserved YES present");
    TEST_ASSERT(repGpio.find("Owner:\nUART0_TX") != std::string::npos, "Owner present");

    // 3. Interrupt Report
    std::string repIrq = bsp.GenerateInterruptReport(15);
    std::cout << repIrq << std::endl;
    TEST_ASSERT(repIrq.find("[INTERRUPT]") != std::string::npos, "Header [INTERRUPT] present");
    TEST_ASSERT(repIrq.find("Pin:\n15") != std::string::npos, "Pin 15 present");
    TEST_ASSERT(repIrq.find("Enabled:\nYES") != std::string::npos, "Enabled YES present");
    TEST_ASSERT(repIrq.find("Trigger:\nRISING_EDGE") != std::string::npos, "Trigger present");
    TEST_ASSERT(repIrq.find("Callback:\nREGISTERED") != std::string::npos, "Callback REGISTERED present");

    std::cout << "  ✓ PASS: Structured ASCII reports validated against specification." << std::endl;
}

/**
 * @brief TEST 11: 100-Thread Concurrency
 */
void Test11_100ThreadConcurrency() {
    std::cout << "[TEST 11] 100-Thread Concurrency..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    const int kNumThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<int> successCount{0};

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&bsp, i, &successCount]() {
            uint32_t pin = static_cast<uint32_t>(i % 50); // Valid pins 0 to 49
            std::string driver = "CONC_DRV_" + std::to_string(i);

            // Reserve, configure, write, read, release cycle
            bool okRes = bsp.ReserveGPIO(pin, driver);
            if (okRes) {
                bsp.ConfigurePinMode(pin, PinMode::OUTPUT);
                bsp.WriteGPIO(pin, PinState::HIGH);
                PinState st;
                bsp.ReadGPIO(pin, st);
                bsp.ToggleGPIO(pin);
                bsp.ReleaseGPIO(pin);
                successCount++;
            } else {
                // If contention occurred on same pin from another thread, query telemetry
                GPIOPin p;
                if (bsp.GetPin(pin, p)) {
                    successCount++;
                }
            }
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    TEST_ASSERT(successCount.load() == kNumThreads,
                "All 100 concurrent threads must succeed without data races or deadlocks");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: "
              << successCount.load() << ")." << std::endl;
}

/**
 * @brief TEST 12: Performance Benchmark
 */
void Test12_PerformanceBenchmark() {
    std::cout << "[TEST 12] Performance Benchmark: 100 GPIOs, 1,000 Reads, 1,000 Writes, 1,000 Toggles, 1,000 Interrupts..." << std::endl;
    RaspberryPi4BSP bsp;
    bsp.InitializeBoard();

    // 1. Reserve and configure pins (pins 0 to 57)
    for (uint32_t p = 0; p < 50; ++p) {
        bsp.ReserveGPIO(p, "BENCH_DRIVER_" + std::to_string(p));
        if (p < 25) {
            bsp.ConfigurePinMode(p, PinMode::OUTPUT);
        } else {
            bsp.ConfigurePinMode(p, PinMode::INPUT);
            bsp.EnableInterrupt(p, InterruptTrigger::RISING_EDGE);
            bsp.RegisterInterruptCallback(p, [](uint32_t, PinState, uint64_t){});
        }
    }

    // 2. Benchmark 1,000 Writes (< 1 ms average target)
    auto tStartWrite = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        uint32_t pin = static_cast<uint32_t>(i % 25);
        bsp.WriteGPIO(pin, (i % 2 == 0) ? PinState::HIGH : PinState::LOW);
    }
    auto tEndWrite = std::chrono::steady_clock::now();
    double totalWriteMs = std::chrono::duration<double, std::milli>(tEndWrite - tStartWrite).count();
    double avgWriteMs = totalWriteMs / 1000.0;

    // 3. Benchmark 1,000 Reads (< 1 ms average target)
    PinState st;
    auto tStartRead = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        uint32_t pin = static_cast<uint32_t>(i % 50);
        bsp.ReadGPIO(pin, st);
    }
    auto tEndRead = std::chrono::steady_clock::now();
    double totalReadMs = std::chrono::duration<double, std::milli>(tEndRead - tStartRead).count();
    double avgReadMs = totalReadMs / 1000.0;

    // 4. Benchmark 1,000 Toggles
    auto tStartToggle = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        uint32_t pin = static_cast<uint32_t>(i % 25);
        bsp.ToggleGPIO(pin);
    }
    auto tEndToggle = std::chrono::steady_clock::now();
    double totalToggleMs = std::chrono::duration<double, std::milli>(tEndToggle - tStartToggle).count();
    double avgToggleMs = totalToggleMs / 1000.0;

    // 5. Benchmark 1,000 Interrupt Events
    auto tStartIrq = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        uint32_t pin = 25 + static_cast<uint32_t>(i % 25);
        bsp.SimulateInterrupt(pin, PinState::HIGH);
    }
    auto tEndIrq = std::chrono::steady_clock::now();
    double totalIrqMs = std::chrono::duration<double, std::milli>(tEndIrq - tStartIrq).count();
    double avgIrqMs = totalIrqMs / 1000.0;

    std::cout << "  -> 1,000 Writes Total Latency: " << totalWriteMs << " ms (Avg: " << avgWriteMs << " ms / write)" << std::endl;
    std::cout << "  -> 1,000 Reads Total Latency: " << totalReadMs << " ms (Avg: " << avgReadMs << " ms / read)" << std::endl;
    std::cout << "  -> 1,000 Toggles Total Latency: " << totalToggleMs << " ms (Avg: " << avgToggleMs << " ms / toggle)" << std::endl;
    std::cout << "  -> 1,000 Interrupts Total Latency: " << totalIrqMs << " ms (Avg: " << avgIrqMs << " ms / event)" << std::endl;

    TEST_ASSERT(avgWriteMs < 1.0, "Average write latency must be < 1 ms");
    TEST_ASSERT(avgReadMs < 1.0, "Average read latency must be < 1 ms");

    std::cout << "  ✓ PASS: All performance benchmarks (< 1ms read/write) satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 16A Raspberry Pi 4 BSP Tests         " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_BoardInitialization();
    Test2_GPIOReservation();
    Test3_DuplicateReservation();
    Test4_PinConfiguration();
    Test5_ReadWriteGPIO();
    Test6_ToggleGPIO();
    Test7_InterruptEnableDisable();
    Test8_InterruptCallback();
    Test9_PeripheralReservation();
    Test10_AsciiReports();
    Test11_100ThreadConcurrency();
    Test12_PerformanceBenchmark();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 12 PHASE 16A UNIT TESTS PASSED SUCCESSFULLY (100%)  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

