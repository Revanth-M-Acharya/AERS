# SMART AEGIS (AERS) — Build, Run & Demonstration Guide

================================================================================
SMART AEGIS: Automated Emergency Routing & Smart Traffic Corridor Preemption System
Production Engineering Handbook — Target: QNX SDP 8.0 & Cross-Platform POSIX
================================================================================

This document provides complete, definitive instructions for building, running, simulating, testing, and deploying the SMART AEGIS system on both cross-platform development workstations (macOS Apple Clang, Linux GCC, MSVC) and physical Raspberry Pi 4 clusters running QNX Neutrino RTOS 8.0.

---

## 1. System Architecture & Cluster Specification

SMART AEGIS coordinates emergency vehicle corridors across roadside intersection nodes using low-latency TCP/IP sockets and direct BCM2711 GPIO hardware control.

### Cluster Topology

```
                  ┌────────────────────────────────────────────────────────┐
                  │               Raspberry Pi 1: COORDINATOR              │
                  │               Static IP: 10.204.179.4                  │
                  │   OS: QNX SDP 8.0 / Linux / macOS (Pure Software)      │
                  │   Role: TCP Client Engine, Green Corridor Scheduler    │
                  │   Hardware: ZERO GPIO ACCESS (Pure Software Master)    │
                  └───────────────────────────┬────────────────────────────┘
                                              │
                      TCP Socket Control Mesh │ Persistent Connections (2.0s retry)
                      ISO 8601 / CRC16-CCITT  │ Periodic PING (5.0s interval)
                                              │
        ┌─────────────────────────────────────┼─────────────────────────────────────┐
        │                                     │                                     │
        ▼                                     ▼                                     ▼
┌──────────────────────────────┐ ┌──────────────────────────────┐ ┌──────────────────────────────┐
│  Raspberry Pi 2 (Node 1)     │ │  Raspberry Pi 3 (Node 2)     │ │  Raspberry Pi 4 (Node 3)     │
│  IP: 10.204.179.245          │ │  IP: 10.204.179.130          │ │  IP: 10.204.179.144          │
│  TCP Port: 5001              │ │  TCP Port: 5002              │ │  TCP Port: 5003              │
│  Role: TCP Server Daemon     │ │  Role: TCP Server Daemon     │ │  Role: TCP Server Daemon     │
│  LEDs:                       │ │  LEDs:                       │ │  LEDs:                       │
│  • Red:  BCM 5  (Pin 29)     │ │  • Red:  BCM 5  (Pin 29)     │ │  • Red:  NONE (HW Shortage)  │
│  • Blue: BCM 6  (Pin 31)     │ │  • Blue: BCM 6  (Pin 31)     │ │  • Blue: BCM 6  (Pin 31)     │
│  Resistors: 330Ω Series      │ │  Resistors: 330Ω Series      │ │  Resistors: 330Ω Series      │
└──────────────────────────────┘ └──────────────────────────────┘ └──────────────────────────────┘
```

### Hardware Constraints & Invariants

1. **Relay Modules Completely Eliminated:** All 12V DC relays and auxiliary power boards have been replaced by direct GPIO control through 330Ω current-limiting resistors connected directly to breadboard LEDs.
2. **Direction of Connection:** The Coordinator acts as a **TCP Client** initiating outgoing persistent connections to all intersection nodes. Intersection nodes act as **TCP Servers** listening on ports `5001`, `5002`, and `5003`.
3. **Node 3 Hardware Shortage Handling:** Raspberry Pi 4 (Node 3) is physically equipped with only a Blue LED (BCM 6 / Pin 31). In the software stack (`QnxHardwareGpioController`), Red LED commands directed to Node 3 are handled safely via a virtualized no-op that logs the operation and returns success without error.
4. **Coordinator Isolation:** Raspberry Pi 1 has no physical LEDs connected and performs pure software coordination with zero GPIO register interaction.

---

## 2. Directory Structure & Key Files

```
AERS/
├── CMakeLists.txt                         # Modern CMake (>= 3.20) build configuration
├── Makefile                               # Legacy GNU Make build pipeline (synchronized)
├── config/
│   └── intersections.json                 # Central production cluster network & pin configuration
├── include/
│   ├── aegis/config/IntersectionsConfig.hpp # RAII configuration parser
│   ├── coordinator_client/                # Coordinator TCP client manager & scheduler
│   ├── gpio/                              # Hardware/Simulation GPIO HAL (BCM2711 & Mock)
│   ├── intersection_service/              # Intersection TCP server daemon
│   ├── protocol/                          # Frame parser, serializer & CRC16 validator
│   └── simulation/                        # Dynamic ANSI simulation dashboard & runner
├── src/
│   ├── apps/
│   │   ├── coordinator_main.cpp           # Coordinator daemon entrypoint
│   │   ├── intersection_main.cpp          # Intersection daemon entrypoint
│   │   ├── simulation_main.cpp            # In-process simulation runner entrypoint
│   │   ├── validator_main.cpp             # 9-vector system pre-flight verification tool
│   │   └── gui_main.cpp                   # Qt6 desktop operator dashboard
│   ├── config/IntersectionsConfig.cpp
│   ├── coordinator_client/
│   ├── gpio/
│   ├── intersection_service/
│   ├── protocol/
│   └── simulation/
├── scripts/
│   ├── start_coordinator.sh               # Launch Coordinator on Pi 1
│   ├── start_intersection1.sh             # Launch Intersection 1 on Pi 2 (port 5001)
│   ├── start_intersection2.sh             # Launch Intersection 2 on Pi 3 (port 5002)
│   ├── start_intersection3.sh             # Launch Intersection 3 on Pi 4 (port 5003)
│   ├── stop_all.sh                        # Kill all running daemons
│   ├── restart_all.sh                     # Orchestrate complete cluster restart
│   ├── health_check.sh                    # Ping & socket verification script
│   └── deploy.sh                          # Automated SSH binary sync & deployment
├── tests/
│   └── unit/
│       └── test_smart_aegis_system.cpp    # 10-point comprehensive verification suite
└── .vscode/
    ├── launch.json                        # VS Code F5 debug configurations
    ├── tasks.json                         # Build & test task definitions
    └── settings.json                      # IntelliSense & CMake environment setup
```

---

## 3. Building SMART AEGIS

### Prerequisites
- **C++ Compiler:** C++17 compliant (Apple Clang 15+, GCC 9+, MSVC 2019+, or QNX QCC for SDP 8.0)
- **CMake:** Version 3.20 or newer
- **Make:** GNU Make (optional alternative build system)
- **Qt6:** (Optional) Required only if building the `gui` desktop application target

### Building with CMake (Recommended)

```bash
# 1. Configure the build directory
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compile all targets with multi-core parallelism
cmake --build build -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# 3. Optional: Install binaries and configuration
cmake --install build --prefix /opt/aegis
```

### Generated Executable Targets

All compiled binaries reside in `build/bin/`:

| Executable | Purpose | Target Platform |
| :--- | :--- | :--- |
| `build/bin/simulation` | Full in-process 5-stage emergency corridor simulation with dynamic ANSI dashboard | Any (POSIX / macOS / Linux) |
| `build/bin/coordinator` | Master Coordinator daemon supporting live network, `--simulate`, and `--demo` modes | Pi 1 or Dev Workstation |
| `build/bin/intersection` | Roadside Intersection daemon supporting `--id <1-3>`, `--port <5001-5003>`, and `--hardware` | Pi 2, Pi 3, Pi 4 |
| `build/bin/validator` | Automated 9-vector pre-flight deployment verification and diagnostics tool | Any |
| `build/bin/gui` | Qt6 cross-platform interactive GUI with animated corridor and LED state visualization | Desktop Workstations |
| `build/bin/test_smart_aegis_system` | Comprehensive 10-point automated unit & integration test runner | Any |

---

## 4. Running the Simulation

The simulation executes the full emergency response lifecycle in-process:
1. Spawns 3 virtual intersection servers on ports 5001, 5002, 5003.
2. Initializes virtual GPIO controllers adhering to all hardware constraints (including Node 3 hardware shortage).
3. Connects the master Coordinator client engine to all virtual nodes.
4. Simulates Ambulance `AMB-101` traveling along the corridor from Station Alpha to City General Hospital.
5. Displays a dynamic, ANSI-formatted terminal dashboard showing live node connection status, current LED states, ambulance coordinates, and event logs.

### Option A: Dedicated Simulation Target (Recommended)

```bash
./build/bin/simulation
```

### Option B: Coordinator Simulation Mode

```bash
./build/bin/coordinator --simulate
```

### Expected Output

```
================================================================================
                    SMART AEGIS — EMERGENCY CORRIDOR SIMULATION
================================================================================
 Time: 2026-10-31T09:30:00Z | Status: ACTIVE_CORRIDOR | Corridor: Node1 -> Node2 -> Node3
--------------------------------------------------------------------------------
 Active Route:
   Stage 1: Preemption Node 1 (IP: 10.204.179.245:5001) -> BLUE ON  (EMERGENCY)
   Stage 2: Preemption Node 2 (IP: 10.204.179.130:5002) -> BLUE ON  (EMERGENCY)
   Stage 3: Preemption Node 3 (IP: 10.204.179.144:5003) -> BLUE ON  (EMERGENCY)
   Stage 4: Ambulance clears Node 1 -> RESTORE ALL
   Stage 5: Ambulance arrives at Hospital -> CORRIDOR COMPLETED
================================================================================
```

---

## 5. Running in Visual Studio Code (F5 Workflow)

Pre-configured launch configurations are provided in `.vscode/launch.json`:

1. Open the project folder in Visual Studio Code.
2. Select the **Run & Debug** panel (`Ctrl+Shift+D` or `Cmd+Shift+D`).
3. Choose one of the following configurations from the dropdown:
   - **`Run SMART AEGIS Simulation (Recommended - F5)`**: Automatically compiles and launches the full simulation.
   - **`Run System Validator`**: Runs pre-flight diagnostics across all network, hardware, and configuration components.
   - **`Run Coordinator (Demo Mode)`**: Runs the master coordinator in standalone demonstration mode.
   - **`Run Coordinator (Live Hardware Network)`**: Runs the coordinator connected to physical Raspberry Pis.
   - **`Run Intersection Node 1 (Port 5001 - Sim)`**: Runs a standalone intersection service daemon.
   - **`Run SMART AEGIS GUI Dashboard`**: Launches the Qt6 desktop interface (if Qt6 is installed).
4. Press **F5** to start debugging or execution.

---

## 6. Pre-Flight Verification & Health Check

Before deploying or running on real hardware, execute the automated validator:

```bash
./build/bin/validator
```

The validator verifies 9 critical operational vectors:
- **Vector 1: Configuration File Integrity** (`config/intersections.json` schema & values).
- **Vector 2: JSON Parser Integrity** (C++ configuration deserialization).
- **Vector 3: TCP Sockets Availability** (Kernel socket creation).
- **Vector 4: Port Bindability** (Local binding tests on 5001, 5002, 5003).
- **Vector 5: Network Reachability** (Optional ping checks to physical Pi IPs).
- **Vector 6: GPIO HAL Factory** (Initialization of simulation/hardware controllers).
- **Vector 7: Hardware Constraint Compliance** (Verification that Node 3 handles missing Red LED safely).
- **Vector 8: Concurrency & Thread Creation** (Worker thread pool allocation).
- **Vector 9: Heap & Dynamic Memory** (Buffer allocations and protocol payload headroom).

You can also run the automated bash health check:
```bash
./scripts/health_check.sh
```

---

## 7. Physical Hardware Deployment on Raspberry Pi 4 Cluster

### Hardware Setup
- **Pi 1 (10.204.179.4):** Coordinator. Pure software, zero GPIO.
- **Pi 2 (10.204.179.245):** Red LED -> Pin 29 (BCM 5) via 330Ω, Blue LED -> Pin 31 (BCM 6) via 330Ω, Cathodes -> Pin 39 (GND).
- **Pi 3 (10.204.179.130):** Red LED -> Pin 29 (BCM 5) via 330Ω, Blue LED -> Pin 31 (BCM 6) via 330Ω, Cathodes -> Pin 39 (GND).
- **Pi 4 (10.204.179.144):** Blue LED -> Pin 31 (BCM 6) via 330Ω, Cathode -> Pin 39 (GND). (No Red LED connected).

### Step-by-Step Launch Sequence

#### 1. Start the Intersection Daemons on Pis 2, 3, and 4 FIRST

**On Pi 2 (`10.204.179.245`):**
```bash
# Using script:
./scripts/start_intersection1.sh
# Or direct command:
./build/bin/intersection --id 1 --port 5001 --hardware
```

**On Pi 3 (`10.204.179.130`):**
```bash
# Using script:
./scripts/start_intersection2.sh
# Or direct command:
./build/bin/intersection --id 2 --port 5002 --hardware
```

**On Pi 4 (`10.204.179.144`):**
```bash
# Using script:
./scripts/start_intersection3.sh
# Or direct command:
./build/bin/intersection --id 3 --port 5003 --hardware
```

#### 2. Start the Master Coordinator on Pi 1

**On Pi 1 (`10.204.179.4`):**
```bash
# Using script:
./scripts/start_coordinator.sh
# Or direct command:
./build/bin/coordinator --hardware
```

The Coordinator will immediately connect to all 3 intersection daemons, log greeting messages, begin 5-second periodic PING health monitoring, and listen for emergency corridor preemption triggers.

#### 3. Stopping the Cluster
To gracefully stop any running daemons on a node:
```bash
./scripts/stop_all.sh
```

To perform a clean restart:
```bash
./scripts/restart_all.sh
```

---

## 8. Automated Test Suite Execution

To run all unit tests and verify the complete software stack:

```bash
# Run unit test binary directly
./build/bin/test_smart_aegis_system

# Or run via CTest
ctest --test-dir build --output-on-failure
```

All 10 test points will execute and verify:
1. Configuration loading & parsing.
2. Hardware shortage handling for Node 3.
3. Simulation GPIO state management.
4. Protocol serialization & CRC16 checksum validation.
5. Protocol deserialization & schema verification.
6. Local TCP loopback transmission.
7. Coordinator client connection & PING health checks.
8. Network fault tolerance & automatic 2.0s reconnection.
9. Green corridor preemption state dispatch.
10. Multi-node concurrent corridor execution.
