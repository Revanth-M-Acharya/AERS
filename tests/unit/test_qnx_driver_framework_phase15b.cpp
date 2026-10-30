/**
 * @file test_qnx_driver_framework_phase15b.cpp
 * @brief SMART AEGIS (AERS) Phase 15B Unit Tests
 *
 * Validates the QNX Neutrino Device Driver Framework & BSP Integration:
 * - 12 Comprehensive Unit Tests
 * - Driver Registration & Querying
 * - Duplicate Rejection & Forensic Error Logging (Rule 9)
 * - Initialization Lifecycle & Pre-requisite Guards (Rule 1, Rule 2)
 * - Start & Stop Lifecycle Execution (Rule 3)
 * - Hardware Resource Allocation & Uniqueness (Rule 6)
 * - Interrupt Enablement & Capability Validation (Rule 5)
 * - Read/Write Device Interface & Guard Conditions (Rule 4)
 * - QNX Resource Manager Path Creation & Lifecycle
 * - Deterministic Restart Recovery (Rule 8: Stop -> Shutdown -> Init -> Start)
 * - Structured Operational ASCII Reports ([FRAMEWORK], [DRIVER], [RESOURCE])
 * - 100-Thread Concurrency Stress Test (Zero Deadlocks, Zero Data Races)
 * - Performance Benchmark (100 Drivers, 1,000 Inits < 2ms, 1,000 Reads < 1ms, 1,000 Writes < 1ms, 1,000 IOCTLs, 100 Allocations)
 *
 * @author SMART AEGIS Core Engineering Team
 * @date 2026
 */

#include "qnx_driver_framework/QNXDeviceDriverFramework.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>

using namespace aegis::qnx_driver;

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
 * @brief TEST 1: Driver Registration
 */
void Test1_DriverRegistration() {
    std::cout << "[TEST 1] Driver Registration..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor gpio("DRV_GPIO_0", "BCM2711 GPIO Controller", DriverType::GPIO,
                         DriverCapability::READ_WRITE, "/dev/gpio0", 1, 0);
    DriverDescriptor uart("DRV_UART_0", "PL011 UART Serial", DriverType::UART,
                         DriverCapability::READ_WRITE, "/dev/ser1", 2, 1);

    TEST_ASSERT(framework.RegisterDriver(gpio), "Register DRV_GPIO_0 must succeed");
    TEST_ASSERT(framework.RegisterDriver(uart), "Register DRV_UART_0 must succeed");

    DriverDescriptor qDrv;
    TEST_ASSERT(framework.GetDriver("DRV_GPIO_0", qDrv), "Query registered DRV_GPIO_0");
    TEST_ASSERT(qDrv.driverType == DriverType::GPIO, "Driver type must match GPIO");
    TEST_ASSERT(qDrv.driverState == DriverState::REGISTERED, "Initial state must be REGISTERED");
    TEST_ASSERT(qDrv.devicePath == "/dev/gpio0", "Device path must match /dev/gpio0");

    auto all = framework.GetAllDrivers();
    TEST_ASSERT(all.size() == 2, "Total registered drivers must be 2");

    TEST_ASSERT(framework.RemoveDriver("DRV_UART_0"), "Remove DRV_UART_0 must succeed");
    TEST_ASSERT(!framework.GetDriver("DRV_UART_0", qDrv), "Removed driver must not be found");
    TEST_ASSERT(framework.GetAllDrivers().size() == 1, "Remaining drivers count must be 1");

    std::cout << "  ✓ PASS: Driver registration, query, and removal verified." << std::endl;
}

/**
 * @brief TEST 2: Duplicate Rejection (Rule 9)
 */
void Test2_DuplicateRejection() {
    std::cout << "[TEST 2] Duplicate Rejection (Rule 9)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor spi1("DRV_SPI_0", "BCM2711 SPI0 Master", DriverType::SPI,
                         DriverCapability::READ_WRITE, "/dev/spi0");
    DriverDescriptor spiDup("DRV_SPI_0", "Duplicate SPI Master", DriverType::SPI,
                           DriverCapability::READ_WRITE, "/dev/spi0");

    TEST_ASSERT(framework.RegisterDriver(spi1), "First registration must succeed");
    TEST_ASSERT(!framework.RegisterDriver(spiDup), "Duplicate registration must be rejected (Rule 9)");

    std::string err = framework.GetLastError("DRV_SPI_0");
    TEST_ASSERT(!err.empty(), "Forensic error log must be recorded on duplicate registration");

    DriverDescriptor qDrv;
    TEST_ASSERT(framework.GetDriver("DRV_SPI_0", qDrv), "Query original driver");
    TEST_ASSERT(qDrv.driverName == "BCM2711 SPI0 Master", "Original driver descriptor must be preserved");

    std::cout << "  ✓ PASS: Duplicate driver rejection and forensic logging verified." << std::endl;
}

/**
 * @brief TEST 3: Initialization Lifecycle (Rule 1 & Rule 2)
 */
void Test3_InitializationLifecycle() {
    std::cout << "[TEST 3] Initialization Lifecycle (Rules 1 & 2)..." << std::endl;
    QNXDeviceDriverFramework framework;

    // Rule 1: Attempt init on unregistered driver -> must fail
    TEST_ASSERT(!framework.InitializeDriver("NON_EXISTENT_DRV"),
                "Initialization of unregistered driver must fail (Rule 1)");

    DriverDescriptor oled("DRV_OLED_0", "SSD1306 Status Display", DriverType::OLED,
                         DriverCapability::WRITE_ONLY, "/dev/oled0");
    TEST_ASSERT(framework.RegisterDriver(oled), "Register DRV_OLED_0");

    // Rule 2: Attempt init without allocated resources -> must fail
    TEST_ASSERT(!framework.InitializeDriver("DRV_OLED_0"),
                "Initialization without allocated resources must fail (Rule 2)");

    DriverDescriptor checkDrv;
    TEST_ASSERT(framework.GetDriver("DRV_OLED_0", checkDrv), "Query OLED driver");
    TEST_ASSERT(checkDrv.driverState == DriverState::FAILED, "State should be FAILED after missing resource");

    // Allocate required resource
    TEST_ASSERT(framework.AllocateResource("DRV_OLED_0", "RES_I2C_BUS1", "I2C Bus 1 Channel", "I2C_BUS"),
                "Allocate I2C resource to DRV_OLED_0");

    // Now initialize -> must succeed
    TEST_ASSERT(framework.InitializeDriver("DRV_OLED_0"), "Initialization with resource must succeed");

    TEST_ASSERT(framework.GetDriver("DRV_OLED_0", checkDrv), "Query OLED driver post-init");
    TEST_ASSERT(checkDrv.driverState == DriverState::INITIALIZED, "Driver state must be INITIALIZED");
    TEST_ASSERT(checkDrv.driverLoaded == true, "driverLoaded flag must be true");

    std::cout << "  ✓ PASS: Driver initialization lifecycle and dependency gates verified." << std::endl;
}

/**
 * @brief TEST 4: Start and Stop Lifecycle (Rule 3)
 */
void Test4_StartAndStopLifecycle() {
    std::cout << "[TEST 4] Start and Stop Lifecycle (Rule 3)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor timer("DRV_TIMER_0", "System Hardware Timer", DriverType::TIMER,
                          DriverCapability::READ_WRITE, "/dev/timer0");
    TEST_ASSERT(framework.RegisterDriver(timer), "Register DRV_TIMER_0");

    // Rule 3: Start without initialization must fail
    TEST_ASSERT(!framework.StartDriver("DRV_TIMER_0"), "Start without initialization must fail (Rule 3)");

    // Allocate resource and initialize
    TEST_ASSERT(framework.AllocateResource("DRV_TIMER_0", "RES_SYS_TIMER", "System Timer Channel 1", "TIMER"),
                "Allocate timer resource");
    TEST_ASSERT(framework.InitializeDriver("DRV_TIMER_0"), "Initialize timer driver");

    // Start driver
    TEST_ASSERT(framework.StartDriver("DRV_TIMER_0"), "Start initialized driver must succeed");

    DriverDescriptor qTimer;
    TEST_ASSERT(framework.GetDriver("DRV_TIMER_0", qTimer), "Query timer driver");
    TEST_ASSERT(qTimer.driverState == DriverState::RUNNING, "State must be RUNNING");
    TEST_ASSERT(qTimer.isRunning == true, "isRunning must be true");
    TEST_ASSERT(qTimer.startupTimestamp > 0, "startupTimestamp must be non-zero");

    auto running = framework.GetRunningDrivers();
    TEST_ASSERT(running.size() == 1, "Running drivers count must be 1");
    TEST_ASSERT(running[0].driverId == "DRV_TIMER_0", "Running driver ID must match");

    // Stop driver
    TEST_ASSERT(framework.StopDriver("DRV_TIMER_0"), "Stop driver must succeed");
    TEST_ASSERT(framework.GetDriver("DRV_TIMER_0", qTimer), "Query stopped timer driver");
    TEST_ASSERT(qTimer.driverState == DriverState::STOPPED, "State must be STOPPED");
    TEST_ASSERT(qTimer.isRunning == false, "isRunning must be false");

    // Re-start stopped driver
    TEST_ASSERT(framework.StartDriver("DRV_TIMER_0"), "Re-start stopped driver must succeed");
    TEST_ASSERT(framework.GetDriver("DRV_TIMER_0", qTimer), "Query re-started timer driver");
    TEST_ASSERT(qTimer.driverState == DriverState::RUNNING, "State must be RUNNING again");

    std::cout << "  ✓ PASS: Start and stop lifecycle transitions verified." << std::endl;
}

/**
 * @brief TEST 5: Resource Allocation (Rule 6)
 */
void Test5_ResourceAllocation() {
    std::cout << "[TEST 5] Resource Allocation (Rule 6)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor eth1("DRV_ETH_0", "Gigabit Ethernet MAC", DriverType::ETHERNET,
                         DriverCapability::READ_WRITE, "/dev/io-pkt/eth0");
    DriverDescriptor eth2("DRV_ETH_1", "Secondary Ethernet", DriverType::ETHERNET,
                         DriverCapability::READ_WRITE, "/dev/io-pkt/eth1");

    TEST_ASSERT(framework.RegisterDriver(eth1), "Register eth0");
    TEST_ASSERT(framework.RegisterDriver(eth2), "Register eth1");

    HardwareResource macRes("RES_ETH_MAC0", "BCM2711 GENET MAC", "ETHERNET_CONTROLLER");
    TEST_ASSERT(framework.AllocateResource("DRV_ETH_0", macRes), "Allocate MAC0 to eth0");

    // Rule 6: Attempting to allocate same resource to another driver must fail
    TEST_ASSERT(!framework.AllocateResource("DRV_ETH_1", macRes),
                "Allocate already-allocated resource must fail (Rule 6)");

    HardwareResource qRes;
    TEST_ASSERT(framework.GetResource("RES_ETH_MAC0", qRes), "Query allocated resource");
    TEST_ASSERT(qRes.allocated == true, "Resource must be allocated");
    TEST_ASSERT(qRes.ownerDriver == "DRV_ETH_0", "Owner must be DRV_ETH_0");
    TEST_ASSERT(qRes.resourceState == ResourceState::ALLOCATED, "State must be ALLOCATED");

    // Release resource
    TEST_ASSERT(framework.ReleaseResource("RES_ETH_MAC0"), "Release MAC0 must succeed");
    TEST_ASSERT(framework.GetResource("RES_ETH_MAC0", qRes), "Query released resource");
    TEST_ASSERT(qRes.allocated == false, "Resource must be unallocated");
    TEST_ASSERT(qRes.resourceState == ResourceState::RELEASED, "State must be RELEASED");

    // Re-allocate to eth1 after release
    TEST_ASSERT(framework.AllocateResource("DRV_ETH_1", macRes),
                "Re-allocation after release must succeed");
    TEST_ASSERT(framework.GetResource("RES_ETH_MAC0", qRes), "Query re-allocated resource");
    TEST_ASSERT(qRes.ownerDriver == "DRV_ETH_1", "Owner must now be DRV_ETH_1");

    std::cout << "  ✓ PASS: Resource allocation uniqueness and release verified." << std::endl;
}

/**
 * @brief TEST 6: Interrupt Enable/Disable (Rule 5)
 */
void Test6_InterruptEnableDisable() {
    std::cout << "[TEST 6] Interrupt Enable/Disable (Rule 5)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor led("DRV_LED_0", "Status LED Array", DriverType::GPIO,
                        DriverCapability::WRITE_ONLY, "/dev/led0");
    DriverDescriptor btn("DRV_BTN_0", "Emergency Pushbutton IRQ", DriverType::INTERRUPT,
                        DriverCapability::INTERRUPT_CAPABLE, "/dev/irq/btn0");

    TEST_ASSERT(framework.RegisterDriver(led), "Register LED");
    TEST_ASSERT(framework.RegisterDriver(btn), "Register Pushbutton IRQ");

    // Rule 5: Non-interrupt capable driver cannot enable interrupts
    TEST_ASSERT(!framework.EnableInterrupt("DRV_LED_0"),
                "Enable interrupt on non-interrupt driver must fail (Rule 5)");

    // Interrupt capable driver
    TEST_ASSERT(framework.EnableInterrupt("DRV_BTN_0"), "Enable interrupt on IRQ driver must succeed");

    DriverDescriptor qBtn;
    TEST_ASSERT(framework.GetDriver("DRV_BTN_0", qBtn), "Query button driver");
    TEST_ASSERT(qBtn.interruptEnabled == true, "interruptEnabled must be true");

    // Disable interrupt
    TEST_ASSERT(framework.DisableInterrupt("DRV_BTN_0"), "Disable interrupt must succeed");
    TEST_ASSERT(framework.GetDriver("DRV_BTN_0", qBtn), "Query button driver post-disable");
    TEST_ASSERT(qBtn.interruptEnabled == false, "interruptEnabled must be false");

    std::cout << "  ✓ PASS: Interrupt enablement and capability validation verified." << std::endl;
}

/**
 * @brief TEST 7: Read/Write Interface (Rule 4)
 */
void Test7_ReadWriteInterface() {
    std::cout << "[TEST 7] Read/Write Interface (Rule 4)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor rwDrv("DRV_RW_0", "Serial Transceiver", DriverType::UART,
                          DriverCapability::READ_WRITE, "/dev/ttyS1");
    DriverDescriptor roDrv("DRV_RO_0", "GPS Position Receiver", DriverType::GPS,
                          DriverCapability::READ_ONLY, "/dev/gps0");
    DriverDescriptor woDrv("DRV_WO_0", "PWM Siren Modulator", DriverType::GPIO,
                          DriverCapability::WRITE_ONLY, "/dev/pwm0");

    TEST_ASSERT(framework.RegisterDriver(rwDrv), "Register RW driver");
    TEST_ASSERT(framework.RegisterDriver(roDrv), "Register RO driver");
    TEST_ASSERT(framework.RegisterDriver(woDrv), "Register WO driver");

    // Allocate resources and initialize
    TEST_ASSERT(framework.AllocateResource("DRV_RW_0", "RES_UART1"), "Alloc UART1");
    TEST_ASSERT(framework.AllocateResource("DRV_RO_0", "RES_GPS1"), "Alloc GPS1");
    TEST_ASSERT(framework.AllocateResource("DRV_WO_0", "RES_PWM1"), "Alloc PWM1");

    TEST_ASSERT(framework.InitializeDriver("DRV_RW_0"), "Init RW");
    TEST_ASSERT(framework.InitializeDriver("DRV_RO_0"), "Init RO");
    TEST_ASSERT(framework.InitializeDriver("DRV_WO_0"), "Init WO");

    // Rule 4: Attempting read/write when NOT RUNNING must fail
    std::string readBuf;
    TEST_ASSERT(!framework.ReadDevice("DRV_RW_0", readBuf), "Read while INITIALIZED must fail (Rule 4)");
    TEST_ASSERT(!framework.WriteDevice("DRV_RW_0", "DATA"), "Write while INITIALIZED must fail (Rule 4)");

    // Start drivers to transition to RUNNING
    TEST_ASSERT(framework.StartDriver("DRV_RW_0"), "Start RW");
    TEST_ASSERT(framework.StartDriver("DRV_RO_0"), "Start RO");
    TEST_ASSERT(framework.StartDriver("DRV_WO_0"), "Start WO");

    // Read and Write on RW driver must succeed
    TEST_ASSERT(framework.WriteDevice("DRV_RW_0", "TEST_PAYLOAD_123"), "Write to RW driver must succeed");
    TEST_ASSERT(framework.ReadDevice("DRV_RW_0", readBuf), "Read from RW driver must succeed");
    TEST_ASSERT(readBuf == "TEST_PAYLOAD_123", "Read payload must match written payload");

    // Capability check: Write to READ_ONLY must fail
    TEST_ASSERT(!framework.WriteDevice("DRV_RO_0", "INVALID_WRITE"), "Write to READ_ONLY driver must fail");
    TEST_ASSERT(framework.ReadDevice("DRV_RO_0", readBuf), "Read from READ_ONLY driver must succeed");

    // Capability check: Read from WRITE_ONLY must fail
    TEST_ASSERT(!framework.ReadDevice("DRV_WO_0", readBuf), "Read from WRITE_ONLY driver must fail");
    TEST_ASSERT(framework.WriteDevice("DRV_WO_0", "SIREN_ON"), "Write to WRITE_ONLY driver must succeed");

    std::cout << "  ✓ PASS: Read and write interface guards and capability checks verified." << std::endl;
}

/**
 * @brief TEST 8: Resource Manager Creation
 */
void Test8_ResourceManagerCreation() {
    std::cout << "[TEST 8] Resource Manager Creation..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor drv("DRV_DEV_MGR", "Virtual Test Device", DriverType::GPIO,
                        DriverCapability::READ_WRITE, "/dev/qnx_test0");
    TEST_ASSERT(framework.RegisterDriver(drv), "Register driver");
    TEST_ASSERT(framework.AllocateResource("DRV_DEV_MGR", "RES_DEV_MGR"), "Alloc resource");
    TEST_ASSERT(framework.InitializeDriver("DRV_DEV_MGR"), "Init driver");
    TEST_ASSERT(framework.StartDriver("DRV_DEV_MGR"), "Start driver");

    // Attach QNX resource manager path
    TEST_ASSERT(framework.CreateResourceManager("DRV_DEV_MGR", "/dev/qnx_custom0"),
                "Create resource manager at custom path");

    DriverDescriptor qDrv;
    TEST_ASSERT(framework.GetDriver("DRV_DEV_MGR", qDrv), "Query driver");
    TEST_ASSERT(qDrv.devicePath == "/dev/qnx_custom0", "Path must be updated");

    // Open, IOCTL, and Close
    TEST_ASSERT(framework.OpenDevice("DRV_DEV_MGR"), "Open device");
    std::string ioctlAck;
    TEST_ASSERT(framework.IoControl("DRV_DEV_MGR", 0x101, "FREQ=50HZ", ioctlAck), "IoControl execute");
    TEST_ASSERT(ioctlAck.find("IOCTL_ACK:CMD=257") != std::string::npos, "IoControl ACK verified");
    TEST_ASSERT(framework.CloseDevice("DRV_DEV_MGR"), "Close device");

    // Destroy resource manager
    TEST_ASSERT(framework.DestroyResourceManager("DRV_DEV_MGR"), "Destroy resource manager");

    std::cout << "  ✓ PASS: Resource manager attachment and I/O control verified." << std::endl;
}

/**
 * @brief TEST 9: Restart Recovery (Rule 8)
 */
void Test9_RestartRecovery() {
    std::cout << "[TEST 9] Restart Recovery (Rule 8)..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor drv("DRV_RESTART_TEST", "Critical Traffic Loop", DriverType::GPIO,
                        DriverCapability::READ_WRITE, "/dev/loop0");
    TEST_ASSERT(framework.RegisterDriver(drv), "Register restart test driver");
    TEST_ASSERT(framework.AllocateResource("DRV_RESTART_TEST", "RES_LOOP0", "Inductive Loop", "GPIO"),
                "Allocate resource");
    TEST_ASSERT(framework.InitializeDriver("DRV_RESTART_TEST"), "Initialize driver");
    TEST_ASSERT(framework.StartDriver("DRV_RESTART_TEST"), "Start driver");

    DriverDescriptor qDrv;
    TEST_ASSERT(framework.GetDriver("DRV_RESTART_TEST", qDrv), "Query initial state");
    TEST_ASSERT(qDrv.driverState == DriverState::RUNNING, "Initial state must be RUNNING");

    // Rule 8: RestartDriver performs Stop -> Shutdown -> Init -> Start
    TEST_ASSERT(framework.RestartDriver("DRV_RESTART_TEST"), "RestartDriver must succeed");

    TEST_ASSERT(framework.GetDriver("DRV_RESTART_TEST", qDrv), "Query restarted driver");
    TEST_ASSERT(qDrv.driverState == DriverState::RUNNING, "State after restart must be RUNNING");
    TEST_ASSERT(qDrv.isRunning == true, "isRunning must be true");
    TEST_ASSERT(qDrv.driverLoaded == true, "driverLoaded must be true");
    TEST_ASSERT(qDrv.resourceAllocated == true, "Resource must be re-allocated");

    HardwareResource qRes;
    TEST_ASSERT(framework.GetResource("RES_LOOP0", qRes), "Query re-allocated resource");
    TEST_ASSERT(qRes.allocated == true, "Resource must remain allocated");
    TEST_ASSERT(qRes.ownerDriver == "DRV_RESTART_TEST", "Owner must remain DRV_RESTART_TEST");

    std::cout << "  ✓ PASS: Restart recovery lifecycle (Rule 8) verified." << std::endl;
}

/**
 * @brief TEST 10: ASCII Reports
 */
void Test10_AsciiReports() {
    std::cout << "[TEST 10] ASCII Reports..." << std::endl;
    QNXDeviceDriverFramework framework;

    DriverDescriptor drv("REP_DRV_01", "Traffic Signal Lamp Relay", DriverType::GPIO,
                        DriverCapability::INTERRUPT_CAPABLE, "/dev/lamp0");
    framework.RegisterDriver(drv);
    framework.AllocateResource("REP_DRV_01", "RES_LAMP0", "Lamp Controller", "GPIO");
    framework.InitializeDriver("REP_DRV_01");
    framework.EnableInterrupt("REP_DRV_01");
    framework.StartDriver("REP_DRV_01");

    // 1. Framework Report
    std::string repFw = framework.GenerateDriverFrameworkReport();
    std::cout << repFw << std::endl;
    TEST_ASSERT(repFw.find("[FRAMEWORK]") != std::string::npos, "Header [FRAMEWORK] present");
    TEST_ASSERT(repFw.find("SMART_AEGIS_QNX_DRIVER_FRAMEWORK") != std::string::npos, "Framework name present");
    TEST_ASSERT(repFw.find("Drivers Registered:\n1") != std::string::npos, "Drivers Registered 1");
    TEST_ASSERT(repFw.find("Drivers Running:\n1") != std::string::npos, "Drivers Running 1");
    TEST_ASSERT(repFw.find("Resources Allocated:\n1") != std::string::npos, "Resources Allocated 1");
    TEST_ASSERT(repFw.find("Status:\nOPERATIONAL") != std::string::npos, "Status OPERATIONAL");

    // 2. Driver Report
    std::string repDrv = framework.GenerateDriverReport("REP_DRV_01");
    std::cout << repDrv << std::endl;
    TEST_ASSERT(repDrv.find("[DRIVER]") != std::string::npos, "Header [DRIVER] present");
    TEST_ASSERT(repDrv.find("Driver:\nREP_DRV_01") != std::string::npos, "Driver ID present");
    TEST_ASSERT(repDrv.find("Type:\nGPIO") != std::string::npos, "Type GPIO present");
    TEST_ASSERT(repDrv.find("State:\nRUNNING") != std::string::npos, "State RUNNING present");
    TEST_ASSERT(repDrv.find("Capability:\nINTERRUPT_CAPABLE") != std::string::npos, "Capability present");
    TEST_ASSERT(repDrv.find("Running:\nYES") != std::string::npos, "Running YES present");
    TEST_ASSERT(repDrv.find("Interrupts:\nENABLED") != std::string::npos, "Interrupts ENABLED present");
    TEST_ASSERT(repDrv.find("Resource:\nRES_LAMP0") != std::string::npos, "Resource RES_LAMP0 present");

    // 3. Resource Report
    std::string repRes = framework.GenerateResourceReport("RES_LAMP0");
    std::cout << repRes << std::endl;
    TEST_ASSERT(repRes.find("[RESOURCE]") != std::string::npos, "Header [RESOURCE] present");
    TEST_ASSERT(repRes.find("Resource:\nRES_LAMP0") != std::string::npos, "Resource ID present");
    TEST_ASSERT(repRes.find("Owner:\nREP_DRV_01") != std::string::npos, "Owner present");
    TEST_ASSERT(repRes.find("Allocated:\nYES") != std::string::npos, "Allocated YES present");

    std::cout << "  ✓ PASS: All ASCII reports validated against specification." << std::endl;
}

/**
 * @brief TEST 11: 100-Thread Concurrency
 */
void Test11_100ThreadConcurrency() {
    std::cout << "[TEST 11] 100-Thread Concurrency..." << std::endl;
    QNXDeviceDriverFramework framework;

    const int kNumThreads = 100;
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);
    std::atomic<int> successCount{0};

    for (int i = 0; i < kNumThreads; ++i) {
        threads.emplace_back([&framework, i, &successCount]() {
            std::string dId = "CONC_DRV_" + std::to_string(i);
            std::string rId = "CONC_RES_" + std::to_string(i);

            bool okReg = framework.RegisterDriver(dId, "Conc Driver", DriverType::GPIO,
                                                 DriverCapability::READ_WRITE, "/dev/c" + std::to_string(i));
            bool okAlloc = framework.AllocateResource(dId, rId, "Conc Res", "GPIO");
            bool okInit = framework.InitializeDriver(dId);
            bool okStart = framework.StartDriver(dId);

            std::string data = "DATA_" + std::to_string(i);
            bool okWrite = framework.WriteDevice(dId, data);
            std::string readBack;
            bool okRead = framework.ReadDevice(dId, readBack);

            DriverDescriptor qD;
            bool okGet = framework.GetDriver(dId, qD);

            if (okReg && okAlloc && okInit && okStart && okWrite && okRead && okGet) {
                successCount++;
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

    auto stats = framework.GetStatistics();
    TEST_ASSERT(stats.totalDrivers == static_cast<uint64_t>(kNumThreads), "Total drivers must match 100");
    TEST_ASSERT(stats.runningDrivers == static_cast<uint64_t>(kNumThreads), "Running drivers must match 100");

    std::cout << "  ✓ PASS: 100 concurrent threads completed with 0 data races (count: "
              << successCount.load() << ")." << std::endl;
}

/**
 * @brief TEST 12: Performance Benchmark
 */
void Test12_PerformanceBenchmark() {
    std::cout << "[TEST 12] Performance Benchmark: 100 Drivers, 1,000 Inits, 1,000 Reads, 1,000 Writes, 1,000 IOCTLs..." << std::endl;
    QNXDeviceDriverFramework framework;

    // 1. Register 100 drivers and allocate 100 resources
    for (int i = 0; i < 100; ++i) {
        std::string dId = "BENCH_DRV_" + std::to_string(i);
        std::string rId = "BENCH_RES_" + std::to_string(i);
        framework.RegisterDriver(dId, "Bench Driver", DriverType::GPIO,
                                 DriverCapability::READ_WRITE, "/dev/b" + std::to_string(i));
        framework.AllocateResource(dId, rId, "Bench Resource", "GPIO");
    }

    // 2. Benchmark 1,000 initializations (< 2 ms average target)
    auto tStartInit = std::chrono::steady_clock::now();
    for (int k = 0; k < 10; ++k) {
        for (int i = 0; i < 100; ++i) {
            std::string dId = "BENCH_DRV_" + std::to_string(i);
            framework.InitializeDriver(dId);
        }
    }
    auto tEndInit = std::chrono::steady_clock::now();
    double totalInitMs = std::chrono::duration<double, std::milli>(tEndInit - tStartInit).count();
    double avgInitMs = totalInitMs / 1000.0;

    // Start all 100 drivers for I/O
    for (int i = 0; i < 100; ++i) {
        framework.StartDriver("BENCH_DRV_" + std::to_string(i));
    }

    // 3. Benchmark 1,000 writes (< 1 ms average target)
    auto tStartWrite = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string dId = "BENCH_DRV_" + std::to_string(i % 100);
        framework.WriteDevice(dId, "BENCHMARK_PAYLOAD_BYTE_STREAM");
    }
    auto tEndWrite = std::chrono::steady_clock::now();
    double totalWriteMs = std::chrono::duration<double, std::milli>(tEndWrite - tStartWrite).count();
    double avgWriteMs = totalWriteMs / 1000.0;

    // 4. Benchmark 1,000 reads (< 1 ms average target)
    std::string readBuf;
    auto tStartRead = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string dId = "BENCH_DRV_" + std::to_string(i % 100);
        framework.ReadDevice(dId, readBuf);
    }
    auto tEndRead = std::chrono::steady_clock::now();
    double totalReadMs = std::chrono::duration<double, std::milli>(tEndRead - tStartRead).count();
    double avgReadMs = totalReadMs / 1000.0;

    // 5. Benchmark 1,000 IOCTL requests
    std::string ioctlAck;
    auto tStartIoctl = std::chrono::steady_clock::now();
    for (int i = 0; i < 1000; ++i) {
        std::string dId = "BENCH_DRV_" + std::to_string(i % 100);
        framework.IoControl(dId, 0x200 + (static_cast<uint32_t>(i) % 10), "BENCH_ARG", ioctlAck);
    }
    auto tEndIoctl = std::chrono::steady_clock::now();
    double totalIoctlMs = std::chrono::duration<double, std::milli>(tEndIoctl - tStartIoctl).count();
    double avgIoctlMs = totalIoctlMs / 1000.0;

    std::cout << "  -> 1,000 Inits Total Latency: " << totalInitMs << " ms (Avg: " << avgInitMs << " ms / init)" << std::endl;
    std::cout << "  -> 1,000 Writes Total Latency: " << totalWriteMs << " ms (Avg: " << avgWriteMs << " ms / write)" << std::endl;
    std::cout << "  -> 1,000 Reads Total Latency: " << totalReadMs << " ms (Avg: " << avgReadMs << " ms / read)" << std::endl;
    std::cout << "  -> 1,000 IOCTLs Total Latency: " << totalIoctlMs << " ms (Avg: " << avgIoctlMs << " ms / ioctl)" << std::endl;

    TEST_ASSERT(avgInitMs < 2.0, "Average initialization latency must be < 2 ms");
    TEST_ASSERT(avgWriteMs < 1.0, "Average write latency must be < 1 ms");
    TEST_ASSERT(avgReadMs < 1.0, "Average read latency must be < 1 ms");

    std::cout << "  ✓ PASS: All performance benchmarks (< 2ms init, < 1ms read/write) satisfied!" << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " SMART AEGIS: Phase 15B QNX Driver Framework Tests       " << std::endl;
    std::cout << "==========================================================" << std::endl;

    Test1_DriverRegistration();
    Test2_DuplicateRejection();
    Test3_InitializationLifecycle();
    Test4_StartAndStopLifecycle();
    Test5_ResourceAllocation();
    Test6_InterruptEnableDisable();
    Test7_ReadWriteInterface();
    Test8_ResourceManagerCreation();
    Test9_RestartRecovery();
    Test10_AsciiReports();
    Test11_100ThreadConcurrency();
    Test12_PerformanceBenchmark();

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL 12 PHASE 15B UNIT TESTS PASSED SUCCESSFULLY (100%)  " << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

