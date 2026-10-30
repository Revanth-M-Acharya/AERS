# SMART AEGIS (AERS) - QNX Raspberry Pi 4 Deployment & Platform Compatibility Guide

**Target RTOS:** QNX Neutrino RTOS (QNX SDP 8.0 / 7.1)  
**Target Hardware:** Raspberry Pi 4 Model B (Broadcom BCM2711 SoC, 4x Cortex-A72, `aarch64le`)  
**Network Architecture:** Wi-Fi TCP/IP Mesh with Static IPv4 Addressing  
**Hardware Topology:** 1 Coordinator SBC + 3 Roadside Intersection SBCs driving 4-Channel Optocoupled Relays and Complementary Dual-Road Traffic Signals.

---

## 1. Executive Overview

This guide provides the complete, production-grade instructions to compile, package, deploy, run, and debug **SMART AEGIS** on Raspberry Pi 4 Model B boards using the **QNX SDP 8.0 Raspberry Pi 4 Quick Start Image**.

SMART AEGIS executes across four physical single-board computers (SBCs):
1. **Coordinator Raspberry Pi 4 (`192.168.1.100:9000`):** Central decision engine, Emergency Operations Centre (EOC) dashboard backend, dynamic A* route replanning, green corridor synchronization, and fleet management.
2. **Intersection Node A (`192.168.1.101:9001`):** Roadside unit managing Road A & Road B complementary signals, active-low relay bank, optical IR vehicle intrusion sensors, and acoustic preemption warnings.
3. **Intersection Node B (`192.168.1.102:9002`):** Central urban intersection unit arbitrating multi-corridor convergence and emergency preemption.
4. **Intersection Node C (`192.168.1.103:9003`):** Hospital access intersection unit clearing inbound ambulances approaching trauma centers.

---

## 2. Hardware Bill of Materials (BOM) & Wiring Reference

### 2.1 Per-Intersection Hardware Stack
* 1x Raspberry Pi 4 Model B (2GB, 4GB, or 8GB RAM) running QNX SDP 8.0 Quick Start Image
* 1x 4-Channel Optocoupled Relay Module (5V VCC, 3.3V logic compatible, Active-Low trigger)
* 2x Traffic Signal LED Modules (Road A: Red, Yellow, Green; Road B: Red, Yellow, Green)
* 1x Pedestrian Signal LED Indicator (Green Walk)
* 1x High-Intensity Optical Emergency Strobe LED
* 1x 5V Active Acoustic Warning Buzzer
* 2x Active Optical Infrared (IR) Proximity Sensors (Approach IR & Departure IR)
* 1x 5V/3A USB-C Power Supply

### 2.2 Pinout & Signal Mapping (Broadcom BCM2711 40-Pin Header)

| Functional Subsystem | Hardware Component | BCM GPIO | Physical Header Pin | Signal Logic |
| :--- | :--- | :--- | :--- | :--- |
| **Road A Primary Signal** | Road A Red Lamp | **GPIO 17** | Pin 11 | Active-Low Relay Ch 1 (0V = ON) |
| | Road A Yellow Lamp | **GPIO 27** | Pin 13 | Active-Low Relay Ch 2 (0V = ON) |
| | Road A Green Lamp | **GPIO 22** | Pin 15 | Active-Low Relay Ch 3 (0V = ON) |
| **Road B Secondary Signal** | Road B Red Lamp | **GPIO 23** | Pin 16 | Direct Output (3.3V High-drive) |
| | Road B Yellow Lamp | **GPIO 24** | Pin 18 | Direct Output (3.3V High-drive) |
| | Road B Green Lamp | **GPIO 25** | Pin 22 | Active-Low Relay Ch 4 (0V = ON) |
| **Pedestrian Crossing** | Pedestrian Walk LED | **GPIO 5** | Pin 29 | Direct Output (3.3V High-drive) |
| **Emergency Strobe & Audio** | Optical Strobe LED | **GPIO 26** | Pin 37 | Direct Output (3.3V High-drive) |
| | Acoustic Alert Buzzer | **GPIO 6** | Pin 31 | Direct Output (3.3V High-drive) |
| **Optical IR Detection** | Approach IR Sensor | **GPIO 12** | Pin 32 | Digital Input (Internal Pull-Up) |
| | Departure IR Sensor | **GPIO 13** | Pin 33 | Digital Input (Internal Pull-Up) |
| **Power & Ground** | Logic Supply 3.3V | 3V3 | Pin 1 | 3.3V DC (Sensors) |
| | Coil Supply 5.0V | 5V | Pin 2 / 4 | 5.0V DC (Relay VCC) |
| | Common Ground | GND | Pin 6 / 9 / 14 / 20 / 39 | Ground Return |

> [!IMPORTANT]
> **Active-Low Relay Logic:** The 4-channel optocoupled relay module coils energize when their trigger inputs are pulled to Ground (`0V` / `PinState::LOW`). Writing `3.3V` (`PinState::HIGH`) de-energizes the coil. The SMART AEGIS `RelayController` handles this inversion transparently and guarantees fail-safe de-energization upon node shutdown.

---

## 3. QNX SDP 8.0 Host Environment Setup

### 3.1 Sourcing QNX Environment Variables
On your Linux or macOS host development machine:
```bash
# For QNX SDP 8.0 on Linux x86_64:
source ~/qnx800/qnxsdp-env.sh

# Or export manually:
export QNX_HOST="$HOME/qnx800/host/linux/x86_64"
export QNX_TARGET="$HOME/qnx800/target/qnx"
export QNX_CONFIGURATION="$HOME/.qnx"
export PATH="$QNX_HOST/usr/bin:$PATH"
```

Verify the compiler version:
```bash
q++ -V
# Expected: QCC 8.0 or GCC 12+ targeted to QNX Neutrino RTOS
```

---

## 4. Cross-Compilation Workflows (`aarch64le`)

SMART AEGIS provides three primary compilation workflows for QNX SDP 8.0:

### Method A: Automated QNX Script
```bash
# Debug build for Raspberry Pi 4 (aarch64le)
./scripts/qnx/build_qnx.sh Debug aarch64le

# Release build with full optimizations (-O3)
./scripts/qnx/build_qnx.sh Release aarch64le

# Profile build with profiling symbols (-O2 -g -p)
./scripts/qnx/build_qnx.sh Profile aarch64le
```

### Method B: Dedicated QNX Makefile (`Makefile.qnx`)
```bash
# Debug build
make -f Makefile.qnx ARCH=aarch64le BUILD_TYPE=Debug

# Release build
make -f Makefile.qnx ARCH=aarch64le BUILD_TYPE=Release

# Profile build
make -f Makefile.qnx ARCH=aarch64le BUILD_TYPE=Profile
```

### Method C: CMake Toolchain
```bash
# Configure out-of-tree QNX build
cmake -B build-qnx \
      -DCMAKE_TOOLCHAIN_FILE=cmake/qnx_toolchain.cmake \
      -DQNX_TARGET_ARCH=aarch64le \
      -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build-qnx -j4
```

---

## 5. QNX Momentics IDE Integration

### 5.1 Importing the Project
1. Open **QNX Momentics IDE** (Eclipse-based).
2. Go to **File -> Import... -> General -> Existing Projects into Workspace**.
3. Select the `AERS` root repository directory.
4. Ensure `SMART_AEGIS` is checked and click **Finish**.
5. Momentics detects the included `.project` and `.cproject` files and configures the `aarch64le` QDE environment.

### 5.2 Build Configurations
Under the **Project -> Build Configurations** menu, three configurations are pre-defined:
* `Debug-aarch64le`: Compiles with debug symbols (`-g -O0 -DAEGIS_DEBUG`).
* `Release-aarch64le`: Compiles with maximum performance optimizations (`-O3 -DNDEBUG`).
* `Profile-aarch64le`: Compiles with execution profiling support (`-O2 -g -p -DPROFILE`).

### 5.3 Launch Configurations
Pre-configured QNX Remote Application launch configurations are provided in `momentics/launch/`:
* `aegis_coordinator_remote.launch`: Deploys and debugs Coordinator on `192.168.1.100:8000` via `qconn`.
* `aegis_intersection_remote.launch`: Deploys and debugs Intersection Node on `192.168.1.101:8000`.

To launch:
1. Ensure `qconn` is running on target: `/usr/sbin/qconn &`
2. In Momentics, click **Run -> Debug Configurations...** -> Select **aegis_coordinator_remote**.
3. Momentics transfers the binary to `/opt/aegis/bin/` and attaches GDB.

---

## 6. VS Code QNX Extension Integration

Complete configuration files are available in `.vscode/`:

### 6.1 VS Code Tasks (`.vscode/tasks.json`)
* **Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on macOS):** Runs `QNX Build (Debug - aarch64le)`.
* **Run Task `Host Run Tests`:** Executes full test regression (`make test`).
* **Run Task `Deploy to Coordinator (192.168.1.100)`:** Deploys software bundle over Wi-Fi.

### 6.2 VS Code Debugging (`.vscode/launch.json`)
* `QNX Remote Debug - Coordinator (192.168.1.100)`: Connects GDB to `192.168.1.100:8000`.
* `Local Host Debug - Phase 20.5 QNX Deployment Test`: Debugs `test_qnx_deployment_phase20_5` on host.

---

## 7. Packaging & Deployment to Raspberry Pi 4 Boards

### 7.1 Assemble Deployment Bundle
Execute the packager:
```bash
./scripts/qnx/package_qnx.sh
```
This produces `aegis_qnx_rpi4.tar.gz` containing:
```
./
├── bin/
│   ├── aegis_coordinator
│   ├── aegis_intersection_node
│   ├── aegis_ambulance
│   └── aegis_traffic_controller
├── config/
│   ├── hardware.json
│   ├── node.json
│   └── deployment/
│       ├── rpi1_coordinator.json
│       ├── rpi2_intersection_a.json
│       ├── rpi3_intersection_b.json
│       └── rpi4_intersection_c.json
└── scripts/
    ├── qnx_setup_wifi.sh
    ├── run_qnx_coordinator.sh
    └── run_qnx_intersection.sh
```

### 7.2 Remote Wi-Fi Deployment
Deploy to each Raspberry Pi node using static IPs:
```bash
# 1. Coordinator (RPi 1)
./scripts/qnx/deploy_qnx.sh 192.168.1.100 COORDINATOR

# 2. Intersection A (RPi 2)
./scripts/qnx/deploy_qnx.sh 192.168.1.101 INT_A

# 3. Intersection B (RPi 3)
./scripts/qnx/deploy_qnx.sh 192.168.1.102 INT_B

# 4. Intersection C (RPi 4)
./scripts/qnx/deploy_qnx.sh 192.168.1.103 INT_C
```

---

## 8. Target Execution & Runtime Verification

### 8.1 On Coordinator Raspberry Pi (`192.168.1.100`)
```bash
cd /opt/aegis
./scripts/run_qnx_coordinator.sh
```
* Binds TCP server on `192.168.1.100:9000` at QNX Real-Time Priority 21 (`SCHED_FIFO`).
* Initiates dynamic route replanner, hospital bed monitoring, and 1 Hz cluster heartbeat supervisor.

### 8.2 On Roadside Intersection Raspberry Pis (`192.168.1.101`–`103`)
```bash
# On Intersection Node A:
cd /opt/aegis
./scripts/run_qnx_intersection.sh INT_A

# On Intersection Node B:
cd /opt/aegis
./scripts/run_qnx_intersection.sh INT_B

# On Intersection Node C:
cd /opt/aegis
./scripts/run_qnx_intersection.sh INT_C
```

### 8.3 Live Demonstration Testing
1. **Normal Cyclic Operation:** Intersections cycle complementary green signals (Road A Green $\leftrightarrow$ Road B Red, and vice versa).
2. **Emergency Preemption Trigger:**
   * Vehicle trips Approach IR sensor on GPIO 12 $\to$ node engages green preemption (`PREEMPTION_ROAD_A`).
   * Strobe on GPIO 26 flashes; buzzer on GPIO 6 sounds; pedestrian signal on GPIO 5 forces Don't Walk.
3. **Vehicle Departure & Safe Recovery:**
   * Vehicle trips Departure IR sensor on GPIO 13 $\to$ preemption automatically terminates.
   * Safety interlock commands safe 2.0s All-Red clearance before restoring normal 4-phase cycle.
4. **Communication Fault Degradation:**
   * If Wi-Fi link drops, node automatically degrades to `FAILSAFE_FLASHING_YELLOW` within 3 seconds.
   * Upon Wi-Fi link restoration, node resynchronizes with Coordinator without restart.

---

## 9. Phase 20.6 Deployment Hardening & Hardware Validation

SMART AEGIS Phase 20.6 hardens the deployment workflow for real physical hardware:

### 9.1 Dedicated Node Deployment Packages
The packager (`scripts/qnx/package_qnx.sh`) generates optimized, role-specific deployment archives:
* **`aegis_coordinator_qnx_rpi4.tar.gz`:** Tailored for Central Coordinator SBC (`192.168.1.100`), containing `aegis_coordinator`, `aegis_ambulance`, coordinator configs, supervisor scripts, and deployment instructions.
* **`aegis_intersection_qnx_rpi4.tar.gz`:** Tailored for Roadside Intersection SBCs (`192.168.1.101`–`103`), containing `aegis_intersection_node`, `aegis_traffic_controller`, intersection configs, supervisor scripts, and deployment instructions.
* **`aegis_qnx_rpi4.tar.gz`:** Universal bundle containing all application binaries and configurations.

### 9.2 Production Supervisor & Auto-Restart on Failure
Both `scripts/qnx/run_qnx_coordinator.sh` and `scripts/qnx/run_qnx_intersection.sh` implement an industrial process supervision loop:
* **Failure Recovery:** Automatically restarts crashes with a 2-second backoff up to 10 retry attempts.
* **Signal Trapping:** Traps `SIGINT` and `SIGTERM` to perform clean child process termination and PID file cleanup.
* **Multi-Stream Logging:** Directs stdout and stderr to dedicated log files (`logs/coordinator_stdout.log`, `logs/<node_id>_stdout.log`).
* **Auto-Launch on Boot:** `scripts/qnx/qnx_autolaunch.sh` integrates with `/etc/rc.d/rc.local` to initialize static Wi-Fi and spawn the appropriate node supervisor on system boot.

### 9.3 Companion Engineering Guides
* [`docs/QNX_NETWORK_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_NETWORK_GUIDE.md): Complete Wi-Fi mesh networking, CRC16 protocol framing, heartbeat supervision, packet retry, and QoS latency/loss accounting.
* [`docs/QNX_HARDWARE_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_HARDWARE_GUIDE.md): BCM2711 GPIO pinout, 4-channel active-low relay wiring, optical IR sensor debouncing (`OpticalSensorFilter`), and hardware mutual exclusion interlock.

### 9.4 Running Phase 20.6 Hardening Test Suite
```bash
./build/bin/test_qnx_hardening_phase20_6
```
*(All 122 assertions pass with 100% success).*

---

## 10. Phase 2 Production-Grade Embedded System (Raspberry Pi 4 + QNX SDP 8.0)

Phase 2 establishes full industrial embedded controller behavior across all nodes running on the official QNX SDP 8.0 Raspberry Pi 4 Model B image:

### 10.1 Deterministic 14-Stage Boot Flow
Every node executes the rigorous deterministic startup pipeline:
`Power On` $\to$ `1. Kernel Boot` $\to$ `2. Init BSP` $\to$ `3. Init GPIO` $\to$ `4. Reserve GPIO` $\to$ `5. Init Relay Controller` $\to$ `6. Run Relay POST` $\to$ `7. Init IR Sensors` $\to$ `8. Init Network` $\to$ `9. Verify Link` $\to$ `10. Register Node` $\to$ `11. Start Hardware Watchdog` $\to$ `12. Start Heartbeat` $\to$ `13. Enter Operational Mode` $\to$ `14. READY`.
* **Latency Accounting:** Boot duration is tracked in microseconds ($\mu s$) and milliseconds (ms).
* **Fault Handling:** If any stage fails, the sequence aborts immediately and commands the hardware into Safe Mode (`ALL_RED`).

### 10.2 Relay Power-On Self Test (POST) Flow
Executed at Stage 6 of boot:
1. Sequentially tests Relay 1 (Road A Red, GPIO 17), Relay 2 (Road A Green, GPIO 22), Relay 3 (Road B Red, GPIO 23), Relay 4 (Road B Green, GPIO 25).
2. For each channel: verifies GPIO Output, sets active-low ON state, reads back hardware pin state, sets OFF state, reads back pin state, and records switch latency in microseconds.
3. Tests hardware green conflict interlock: verifies that attempting to energize Road A Green and Road B Green simultaneously is strictly blocked.
4. If any relay readback fails or conflict interlock fails, the boot sequence aborts to Safe Mode.

### 10.3 Dedicated 1 Hz Hardware Watchdog Flow
The dedicated background watchdog thread executes a 1 Hz supervisory loop:
* Verifies Coordinator heartbeat freshness ($< 5.0$s timeout).
* Verifies Node internal keepalive freshness ($< 2.0$s timeout).
* Verifies worker thread liveness ($< 3.0$s timeout).
* Verifies Relay coil health and readback status.
* Verifies GPIO voltage states and pin ownership.
* Verifies optical IR sensors (detects stuck beams $> 30.0$s).
* Verifies Wi-Fi link quality metrics.
* Verifies Traffic Controller state machine and Boot Manager health.
* **On Fault:** Logs failure, dispatches alert to coordinator, moves traffic signals to safe state (`ALL_RED` or `FLASHING_YELLOW`), and executes self-healing auto-recovery once all conditions return to normal.

### 10.4 Wi-Fi / Ethernet Quality Monitoring
Monitors RSSI, packet loss %, RTT latency, reconnect count, and signal strength trends in a rolling window:
* **EXCELLENT:** RSSI $\ge -60$ dBm, Latency $< 20$ ms, Loss $< 1\%$.
* **GOOD:** RSSI $\ge -68$ dBm, Latency $< 60$ ms, Loss $< 3\%$.
* **FAIR:** RSSI $\ge -75$ dBm, Latency $< 150$ ms, Loss $< 10\%$.
* **POOR:** RSSI $\ge -85$ dBm, Latency $< 300$ ms, Loss $< 20\%$.
* **CRITICAL:** RSSI $< -85$ dBm, Latency $\ge 300$ ms, or Loss $\ge 20\%$.
* **DISCONNECTED:** Socket dropped or link down.

### 10.5 Industrial 10-Step Safe Shutdown Flow
Non-reentrant shutdown orchestrator triggered by `SIGINT`, `SIGTERM`, or operator command:
1. `STOP_ACCEPTING_COMMANDS`: Drops inbound coordinator commands.
2. `COMPLETE_ACTIVE_TRANSITION`: Finishes yellow clearance phase.
3. `ALL_RED_CLEARANCE`: Enforces 2.0-second all-red hold.
4. `DEACTIVATE_RELAYS`: Confirms all relay coils are de-energized (active-low HIGH). Coils must NEVER remain energized.
5. `STORE_LOGS`: Flushes event logs to persistent storage.
6. `RELEASE_GPIO`: Releases BCM2711 pin reservations back to OS.
7. `STOP_THREADS`: Joins all background supervisory and worker threads.
8. `CLOSE_SOCKETS`: Gracefully shuts down and closes TCP sockets.
9. `UNMAP_BSP_MEMORY`: Unmaps memory-mapped registers via `munmap_device_io()`.
10. `SHUTDOWN_COMPLETE`: Emits termination diagnostic report.

### 10.6 Automatic Node Recovery Flow
* When communication with coordinator is lost, node drops into failsafe mode (`FLASHING_YELLOW` / `ALL_RED`).
* Background auto-reconnection loop retries every $N$ seconds.
* Once link is re-established: node automatically re-registers with coordinator, synchronizes state, clears failsafe, and resumes nominal 4-phase traffic cycle without requiring a system reboot.

### 10.7 Running Phase 2 Embedded System Test Suite
```bash
./build/bin/test_phase2_embedded_production
```
*(All 10 tests pass with 100% success).*

---

## 11. Phase 3 Real-Time Corridor Operations & Fault Recovery

Phase 3 introduces advanced corridor lifecycle supervision and autonomous fault recovery on the **Raspberry Pi 4 QNX SDP 8.0 Quick Start Image** (`aarch64le`):

1. **Corridor Deadline Monitor (`CorridorDeadlineMonitor`):** Tracks corridor timestamps, elapsed intervals, remaining deadlines, and triggers recovery on expiration.
2. **Multi-Domain Emergency Timeout Manager (`EmergencyTimeoutManager`):** Supervises Stalled Ambulance, Coordinator Inactivity, Communication Link, and Intersection ACK timeouts with 3-tier escalation and deadlock-free guarantees.
3. **Dynamic Corridor Release Manager (`DynamicCorridorReleaseManager`):** Deterministic node clearance (`CLEARING_YELLOW` 3.0s $\to$ `ALL_RED_CLEARANCE` 2.0s $\to$ `RELEASED`) with multi-vehicle handoff safety.
4. **Route Progress Tracking & In-Flight Replanning (`RouteProgressTracker`):** Real-time spatial tracking (current, previous, next, remaining hops, completion %) and in-flight bypass calculation around failed downstream intersections.
5. **Deterministic Signal Timeline Generator & Replay Engine (`SignalTimelineGenerator`):** Time-synchronized transition schedule generation with QNX monotonic clock timestamps and offline replay engine.
6. **Raspberry Pi Cluster Health Monitor (`ClusterHealthMonitor`):** Central telemetry aggregator evaluating CPU %, RAM, Temperature, Heartbeats, Network QoS, and computing composite weighted health score ($0.0\% - 100.0\%$).
7. **Autonomous Fault Recovery Engine (`FaultRecoveryManager`):** Recovers across 5 failure classes without node reboot, maintaining Safe Mode strictly as a boundary condition.

### Running Phase 3 Test Suite
```bash
./build/bin/test_phase3_corridor_operations
```

---

## 12. Phase 4 Native QNX Features & Architecture

Phase 4 elevates SMART AEGIS into a genuine **QNX Neutrino RTOS** real-time distributed application utilizing native microkernel primitives:

### 12.1 QNX Resource Manager (`/dev/aegis/*`)
Attached via `resmgr_attach()` to expose roadside hardware and municipal coordination as POSIX character device nodes:
* `/dev/aegis/coordinator`: Central green corridor coordinator status and command interface
* `/dev/aegis/intersectionA`: Intersection A signal phase, GPIO, and relay health
* `/dev/aegis/intersectionB`: Intersection B status
* `/dev/aegis/intersectionC`: Intersection C status
* `/dev/aegis/cluster`: 4-node cluster health metrics and composite score
* `/dev/aegis/system`: System OS version, uptime, and RTOS platform diagnostics

Supports standard `open()`, `read()`, `write()`, and `close()` operations alongside native `devctl()` commands.

### 12.2 Native QNX Message Passing (Rendezvous IPC)
Replaces generic sockets with microkernel-native synchronous message passing:
* `ChannelCreate(0)`: Creates private kernel channel on server.
* `ConnectAttach()`: Attaches client connection to target channel ID.
* `MsgSend()`: Sender thread strictly blocks until server calls `MsgReply()`.
* `MsgReceive()`: Server thread receives request header and payload.
* `MsgReply()`: Server unblocks client with structured `QnxReplyMsg`.

Structured schemas: `HEARTBEAT` (`0x1001`), `REGISTRATION` (`0x1002`), `CORRIDOR_REQUEST` (`0x1003`), `SIGNAL_COMMAND` (`0x1004`), `STATUS_UPDATE` (`0x1005`), and `RECOVERY_MESSAGE` (`0x1006`).

### 12.3 QNX Pulse Management (`SIGEV_PULSE`)
Delivers asynchronous, non-blocking kernel events without thread context switches:
* `MsgSendPulse(coid, priority, code, value)`
* Standard application pulse codes:
  - `PULSE_CODE_WATCHDOG = 1`
  - `PULSE_CODE_HEARTBEAT = 2`
  - `PULSE_CODE_CORRIDOR_TIMEOUT = 3`
  - `PULSE_CODE_DEADLINE = 4`
  - `PULSE_CODE_SHUTDOWN = 5`
  - `PULSE_CODE_FAULT_RECOVERY = 6`
  - `PULSE_CODE_INTERRUPT = 7`
* Background pulse dispatcher thread with callback registration.

### 12.4 High-Resolution Monotonic Timers (`CLOCK_MONOTONIC`)
* Configured with `timer_create()` delivering `SIGEV_PULSE` to channel.
* Armed with `timer_settime()` for periodic intervals and one-shot delays.
* Tracks microsecond jitter telemetry ($\mu\text{s}$) and overruns via `timer_getoverrun()`.

### 12.5 Real-Time Thread Scheduling Hierarchy (`SCHED_FIFO`)
Enforces strict 5-tier priority hierarchy:
1. **Priority 30 (Highest):** Coordinator Core Dispatch Thread
2. **Priority 25:** Intersection Controller Threads
3. **Priority 21:** Dynamic Corridor Release & Timeline Engine
4. **Priority 12:** Health Monitor & Deadline Supervision
5. **Priority 5:** Telemetry, Logging & devctl Handlers

Includes `PrioInheritMutex` RAII wrapper utilizing `PTHREAD_PRIO_INHERIT` to prevent unbounded priority inversion across shared resources.

### 12.6 GPIO Interrupt-Driven Event Management (BCM2711)
Attaches hardware lines directly to kernel pulse delivery via `InterruptAttachEvent()`:
* **GPIO 12:** IR Approach Sensor (Inbound vehicle detection)
* **GPIO 13:** IR Departure Sensor (Outbound clearance trigger)
* **GPIO 16:** Emergency Manual Override Button
* **GPIO 24:** Relay Fault Feedback Monitor

### 12.7 Industrial devctl() Commands (Magic Class 'A' = 0x41)
* `DCMD_AEGIS_GET_SYSTEM_STATUS`
* `DCMD_AEGIS_GET_NODE_STATUS`
* `DCMD_AEGIS_START_CORRIDOR`
* `DCMD_AEGIS_STOP_CORRIDOR`
* `DCMD_AEGIS_RESET_NODE`
* `DCMD_AEGIS_READ_TELEMETRY`
* `DCMD_AEGIS_READ_LOGS`
* `DCMD_AEGIS_ENABLE_DIAGNOSTICS`
* `DCMD_AEGIS_RUN_SELF_TEST`

### 12.8 Running Phase 4 Native QNX Test Suite
```bash
./build/bin/test_phase4_native_qnx
```
*(All 10 tests pass with 100% success).*


