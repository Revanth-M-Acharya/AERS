# AERS MASTER SOFTWARE SPECIFICATION

**Document Version:** 1.0.0  
**Project Name:** AERS (Autonomous Emergency Response System)  
**Classification:** Mission-Critical Cyber-Physical System (CPS) Specification  
**Baseline Repository:** `https://github.com/Revanth-M-Acharya/AERS`  
**Current Branch:** `develop`  
**Target Environments:** QNX Neutrino RTOS 7.1+, Embedded Linux (Raspberry Pi OS 64-bit / Debian), POSIX.1-2017  
**Language Standard:** C++17 (ISO/IEC 14882:2017)  
**Compiler Suite:** Clang++ (LLVM 10.0+ / Apple Clang 14.0+) / GCC 9.3+  
**Authority Directive:** This document serves as the permanent, authoritative, single source of truth for the AERS project. All future engineering and AI-assisted sessions MUST reference and synchronize with this document.

---

## TABLE OF CONTENTS

1. [Project Overview](#1-project-overview)
2. [Repository Structure](#2-repository-structure)
3. [Build System](#3-build-system)
4. [Complete Software Architecture](#4-complete-software-architecture)
5. [Development History](#5-development-history)
6. [Module Documentation](#6-module-documentation)
7. [Class Documentation](#7-class-documentation)
8. [Configuration System](#8-configuration-system)
9. [Communication System](#9-communication-system)
10. [Coordinator](#10-coordinator)
11. [Intersection Nodes](#11-intersection-nodes)
12. [Scheduler](#12-scheduler)
13. [Medical Module](#13-medical-module)
14. [Logging](#14-logging)
15. [Deployment](#15-deployment)
16. [Testing](#16-testing)
17. [Repository Statistics](#17-repository-statistics)
18. [Current Capabilities](#18-current-capabilities)
19. [Known Limitations](#19-known-limitations)
20. [Future Development](#20-future-development)
21. [Development Rules](#21-development-rules)
22. [AI Context Section](#22-ai-context-section)

---

# 1 Project Overview

### 1.1 Vision
The **Autonomous Emergency Response System (AERS)** is an intelligent, distributed cyber-physical system designed to eliminate transit delays for emergency vehicles navigating dense urban traffic grids. By orchestrating dynamic green corridors ahead of emergency units, evaluating clinical patient urgency in real time, and resolving multi-vehicle intersection conflicts deterministically, AERS minimizes time-to-treatment for critical medical emergencies while preserving civilian traffic safety and grid stability.

### 1.2 Purpose
Urban traffic congestion remains one of the primary contributors to preventable pre-hospital mortality in acute medical emergencies such as cardiac arrest, massive hemorrhage, and ischemic stroke. Traditional emergency transit relies on acoustic sirens and visual beacons, which require civilian drivers to perceive, react, and yield manually under congested conditions. AERS transitions emergency transit from passive yielding to active, autonomous grid control:
* Interlocking traffic signals dynamically along the emergency route.
* Creating rolling "Green Corridors" that clear civilian queues before the ambulance arrives.
* Arbitrating between multiple converging ambulances using a multi-factor clinical triage priority engine.
* Providing emergency operations centers with real-time auditability and manual override capabilities.

### 1.3 Objectives
1. **Dynamic Green Wave Generation:** Dynamically preempt traffic signals along the active route of an emergency vehicle, clearing downstream intersections to provide a non-stop green corridor.
2. **Clinical Urgency-Based Priority:** Dynamically calculate triage priority scores based on physiological vitals (heart rate, blood pressure, respiration rate, SpO2, blood loss, pain level, consciousness) and incident elapsed time across 12 emergency categories.
3. **Deterministic Conflict Resolution:** Detect spatial and temporal contention when two or more ambulances approach the same intersection within a configurable conflict time window (default: 10 seconds), and resolve contention safely without deadlocks or simultaneous green signals.
4. **Hard Fail-Safe Traffic Control:** Guarantee physical traffic signal invariants (minimum clearance intervals, mandatory yellow transitions $\ge 3.0$s, all-red buffers $\ge 1.0$s, and fail-safe flashing yellow / all-red reversion upon communication loss or watchdog trip).
5. **Distributed Hardware Deployment:** Execute edge signal control directly on physical Raspberry Pi nodes communicating with a Central Coordinator over standard POSIX networking (Wi-Fi / Ethernet LAN).

### 1.4 Scope
* **Central Coordination:** Central Coordinator software executing on Raspberry Pi 1 (QNX RTOS / Linux).
* **Edge Intersections:** Traffic signal controllers executing directly on Raspberry Pi 2 (Intersection A), Raspberry Pi 3 (Intersection B), and Raspberry Pi 4 (Intersection C).
* **Fleet Telemetry:** Ambulance tracking, simulated GPS kinematics, clinical vitals ingestion, and peer-to-peer collision awareness.
* **Control Center Operations:** Emergency operator consoles, manual priority overrides, emergency stop functionality, and forensic event logging.

### 1.5 Current Implementation Status
The project has successfully implemented **Phases 1 through 8**:
* **Phase 1:** Common Foundation, Math, Time, Geo Utilities, Configuration, Thread-Safe Queues.
* **Phase 2:** Network Layer, Transport Abstractions, Serializer, MQTT Broker Integration.
* **Phase 3:** Coordinator Decision Engine, Event Queue, Intersection Registry, Corridor Manager.
* **Phase 4:** Ambulance Node, GPS Kinematics, Vitals Stream, V2V Safety Module.
* **Phase 5A:** Intersection Node Edge Engine, Signal State Machine, 4-Node Raspberry Pi Deployment.
* **Phase 5B:** MQTT Topics Schema, Network Reconnect, Edge Fail-Safe Recovery.
* **Phase 6 & 7:** Hospital and Traffic Management Dashboard Backends.
* **Phase 8:** Multi-Ambulance Conflict Detection, Intelligent Priority Scheduling, Clinical Patient Model, Automated Conflict Resolution Engine, and Operator Override System.

All 18 integrated test suites pass with 100% success on both macOS (Darwin) and Linux development environments.

### 1.6 Development Maturity
AERS is written in modern, standard C++17. The codebase adheres strictly to RAII (Resource Acquisition Is Initialization), smart-pointer lifetime semantics (`std::unique_ptr`, `std::shared_ptr`), thread-safety primitives (`std::mutex`, `std::unique_lock`, `std::condition_variable`), and zero external third-party library dependencies for core networking, JSON serialization, or math.

### 1.7 Mandatory Project Facts & Architecture Constraints
1. **Project Name:** The official name of this project is **AERS**. It must never be renamed. (Any reference to "SMART AEGIS" in older files, commit logs, or test banners represents legacy development naming and is authoritative only where referring to historical namespaces like `aegis::`).
2. **Hardware Topology:** The system is engineered to deploy across **FOUR Raspberry Pi boards**:
   * **Raspberry Pi 1:** Central Coordinator (`192.168.1.101:9000`)
   * **Raspberry Pi 2:** Intersection Node A / `INT_A` (`192.168.1.102:9001`)
   * **Raspberry Pi 3:** Intersection Node B / `INT_B` (`192.168.1.103:9002`)
   * **Raspberry Pi 4:** Intersection Node C / `INT_C` (`192.168.1.104:9003`)
3. **Direct Signal Execution:** Traffic signal state machines, clearance timing, and fail-safe logic execute **directly on the Raspberry Pi edge nodes**.
4. **NO ESP32 / Arduino Microcontrollers:** There are **NO** ESP32 microcontrollers, Arduino devices, or intermediate 8/32-bit auxiliary boards responsible for traffic signal logic in the current architecture. Any historical mention of ESP32 or Arduino in older markdown documents is strictly legacy documentation.
5. **Codebase Authority:** The repository source code as it exists on the `develop` branch is the sole authoritative reference.

---

# 2 Repository Structure

The AERS repository is organized logically by architectural layer and functional responsibility. Below is the complete directory inventory:

```
AERS/
├── CMakeLists.txt                 # Top-level CMake build orchestration
├── Makefile                       # High-performance universal Makefile for POSIX builds
├── AI_RULEBOOK.md                 # Guidelines and invariants for AI assistants
├── CHANGELOG.md                   # Chronological development log
├── DEPLOYMENT_GUIDE.md            # Hardware setup and cluster deployment instructions
├── DEVELOPMENT_PLAN.md            # 12-Phase engineering roadmap and SRS traceability
├── ENGINEERING_AUDIT_REPORT.md    # Multi-phase verification and compliance audit
├── HARDWARE_ABSTRACTION_DOCUMENT.md# GPIO interface and hardware abstraction specs
├── INTERFACE_SPECIFICATION.md     # Interface Control Document (ICD) and schemas
├── LICENSE                        # MIT License
├── RASPBERRY_PI_PIN_MAPPING.md    # Physical GPIO pin assignment guide for RPi 1-4
├── README.md                      # Repository landing page and quickstart guide
├── SOFTWARE_DESIGN_DOCUMENT.md    # Detailed software design descriptions
├── SYSTEM_ARCHITECTURE.md         # Foundational architecture specification
├── assets/                        # Diagrams, demonstration videos, and media
│   ├── images/                    # Architectural diagrams and schematics
│   ├── presentation/              # Slide decks and technical posters
│   └── videos/                    # Hardware demonstration and simulation captures
├── build/                         # Build artifacts directory (ignored by git)
│   ├── bin/                       # Output binaries and test executables
│   ├── lib/                       # Output static archive libaegis.a
│   └── obj/                       # Compiled translation unit object files (.o)
├── cmake/                         # Toolchain files for cross-compilation
│   └── qnx_toolchain.cmake        # QNX SDP 7.1 Neutrino cross-compiler toolchain
├── config/                        # Runtime JSON configuration profiles
│   ├── default.json               # Baseline system default configuration
│   ├── development.json           # Local simulation configuration profile
│   ├── production.json            # Full-scale physical deployment configuration
│   └── deployment/                # Multi-Pi hardware deployment configurations
│       ├── rpi1_coordinator.json     # Raspberry Pi 1 Coordinator config
│       ├── rpi2_intersection_a.json  # Raspberry Pi 2 INT_A config
│       ├── rpi3_intersection_b.json  # Raspberry Pi 3 INT_B config
│       └── rpi4_intersection_c.json  # Raspberry Pi 4 INT_C config
├── docs/                          # In-depth architectural and design documentation
│   ├── architecture/              # Subsystem architecture descriptions
│   ├── design/                    # Component-level design specifications
│   ├── generated/                 # Automated documentation outputs
│   └── meeting_notes/             # Engineering design decision notes
├── hardware/                      # Schematics and electrical pin mappings
│   ├── gps/                       # GPS receiver module specifications
│   ├── led_controller/            # Traffic lamp driver schematics
│   ├── raspberry_pi/              # Raspberry Pi 4 Model B board specifications
│   ├── relay/                     # Relay board opto-isolation schematics
│   └── sensors/                   # Vehicle detection sensor specifications
├── include/                       # Public system header files
│   └── aegis/                     # Root namespace headers
│       ├── common/                # Core utilities, circular buffer, math, types
│       ├── config/                # Configuration management interface
│       ├── core/                  # Coordinator, dispatch engine, route planner
│       ├── dashboard/             # Hospital and traffic management consoles
│       ├── edge/                  # Traffic controller, signal FSM, GPIO, GPS
│       ├── logger/                # Synchronous system logger interface
│       ├── network/               # POSIX sockets, MQTT client, serializer
│       └── safety/                # Watchdog timer and safety monitor interfaces
├── scripts/                       # Operational build, deployment, and test scripts
│   ├── build.sh                   # Unified build automation script (CMake/Make)
│   ├── deploy_pi.sh               # Remote SSH deployment script for Raspberry Pis
│   ├── flash_qnx.sh               # QNX IFS boot image flasher script
│   ├── run_simulation.sh          # End-to-end multi-node simulation launcher
│   ├── start_all_nodes.sh         # Local multi-process cluster launcher (RPi 1-4)
│   ├── start_rpi1_coordinator.sh  # RPi 1 Coordinator standalone runner
│   ├── start_rpi2_int_a.sh        # RPi 2 Intersection Node A runner
│   ├── start_rpi3_int_b.sh        # RPi 3 Intersection Node B runner
│   └── start_rpi4_int_c.sh        # RPi 4 Intersection Node C runner
├── src/                           # Complete implementation source files
│   ├── ambulance/                 # Ambulance profile, registry, node, kinematics
│   ├── apps/                      # Entrypoint main functions for daemon binaries
│   ├── common/                    # Implementation of common foundation utilities
│   ├── communication/             # TCP SocketServer, SocketClient, Message
│   ├── config/                    # NodeConfigLoader, ConfigManager
│   ├── control/                   # ManualOverride, EmergencyOperatorConsole
│   ├── coordinator/               # CoordinatorManager, GreenCorridor, EventQueue
│   ├── hospital_dashboard/        # Hospital backend service
│   ├── intersection/              # IntersectionNode, SignalController, Heartbeat
│   ├── logger/                    # Logger, EventLogger, SimulationRecorder
│   ├── medical/                   # PatientProfile, PriorityEngine, MedicalDatabase
│   ├── network/                   # NetworkManager, Serializer, MQTT, Transports
│   ├── priority_scheduler/        # Baseline priority scheduler
│   ├── route_planner/             # Graph-based routing and Dijkstra navigation
│   ├── safety/                    # Watchdog and safety monitoring implementation
│   ├── scheduler/                 # Phase 8 Conflict Engine, Intelligent Scheduler
│   ├── system/                    # SystemManager lifecycle orchestrator
│   ├── traffic_controller/        # Signal FSM, preemption, recovery, GPIO
│   └── traffic_dashboard/         # Traffic management center backend service
├── tests/                         # Automated test suites (Unit, Integration, Sim)
│   ├── hardware/                  # Physical GPIO loopback and timing tests
│   ├── integration/               # System integration and deployment test suites
│   ├── simulation/                # Full mission scenario simulations
│   └── unit/                      # 17 focused unit test suites covering all modules
└── tools/                         # Offline utility tools
    └── simulator/                 # Traffic and vehicle kinetic simulator utilities
```

### Detailed Folder Responsibilities
* **`src/apps/`:** Contains the daemon entry points (`main()` functions):
  * `coordinator_main.cpp`: Executable entry point for the Central Coordinator daemon (`aegis_coordinator`).
  * `intersection_main.cpp`: Executable entry point for the edge Intersection Node daemon (`aegis_intersection_node`).
  * `ambulance_main.cpp`: Executable entry point for the vehicle On-Board Unit daemon (`aegis_ambulance`).
  * `traffic_main.cpp`: Executable entry point for the standalone Traffic Controller daemon (`aegis_traffic_controller`).
* **`src/common/`:** High-performance, memory-safe foundation components providing mathematical calculation, UUID generation, ISO 8601 time string formatting, thread-safe queuing, circular ring buffering, and zero-dependency JSON extraction.
* **`src/communication/`:** Lightweight POSIX stream socket networking providing direct TCP client/server communication between the Central Coordinator and edge Intersection Nodes over IP networks.
* **`src/coordinator/`:** Central intelligence core maintaining the global registry of intersections, tracking vehicle dispatch, evaluating green wave preemption, and queueing asynchronous system events.
* **`src/intersection/`:** Edge intelligence executing directly on Raspberry Pi nodes, encapsulating the intersection state machine, command dispatching, and heartbeat generation.
* **`src/scheduler/`:** Multi-ambulance conflict detection and intelligent priority arbitration engine (Phase 8), resolving intersection contention deterministically.
* **`src/medical/`:** Clinical triage evaluation engine implementing a 14-attribute patient profile, calculating emergency scores across 12 diagnostic categories and physiological shock metrics.
* **`src/logger/`:** Structured telemetry recording 13 distinct event categories with timestamped forensic context and JSON export capability.
* **`config/deployment/`:** Production configuration files for all 4 Raspberry Pi hardware targets.

---

# 3 Build System

AERS provides two fully supported, production-grade build systems: **CMake** (for standard CI/CD, IDEs, and cross-compilation) and a **Universal Native Makefile** (for standalone edge compilation on Raspberry Pi OS without external build dependencies).

### 3.1 Compiler Requirements
* **Language Standard:** C++17 (`-std=c++17`).
* **Supported Compilers:**
  * Apple Clang 14.0+ / LLVM Clang 10.0+
  * GNU Compiler Collection (GCC) 9.3+
  * QNX QCC Compiler (targeting QNX Neutrino RTOS 7.1)
* **Mandatory Warning Flags:**
  ```text
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
  ```
  The entire codebase compiles with **zero warnings** under these strict flags.

### 3.2 Build Configurations
1. **Debug (`BUILD_TYPE=Debug`):**
   * Flags: `-g3 -O0 -DAEGIS_DEBUG`
   * Symbols: Complete debug symbol tables enabled; asserts active.
2. **Release (`BUILD_TYPE=Release`):**
   * Flags: `-O3 -DNDEBUG`
   * Optimizations: Aggressive link-time optimization and inlining.

### 3.3 Target Artifacts

#### Static Libraries
* **`build/lib/libaegis.a`:** The consolidated static archive containing all compiled translation units across all subsystems (`common`, `config`, `logger`, `safety`, `network`, `communication`, `coordinator`, `ambulance`, `traffic_controller`, `intersection`, `system`, `medical`, `scheduler`, `control`, `route_planner`, `priority_scheduler`, `hospital_dashboard`, `traffic_dashboard`).

#### Daemon Executables (`build/bin/`)
1. `aegis_coordinator`: Central Coordinator daemon executable.
2. `aegis_intersection_node`: Edge Intersection Node daemon executable.
3. `aegis_ambulance`: Ambulance On-Board Unit simulator daemon executable.
4. `aegis_traffic_controller`: Standalone Traffic Controller edge daemon executable.

#### Test Executables (`build/bin/`)
1. `test_build_smoke`
2. `test_common_foundation`
3. `test_communication_infrastructure`
4. `test_communication_module`
5. `test_coordinator_core`
6. `test_coordinator_foundation`
7. `test_green_corridor`
8. `test_ambulance_node`
9. `test_traffic_signal_node`
10. `test_intersection_node`
11. `test_system_integration`
12. `test_rpi_deployment`
13. `test_medical_priority`
14. `test_ambulance_registry`
15. `test_conflict_detection`
16. `test_manual_override`
17. `test_event_logger`
18. `test_phase8_intelligent_scheduler`

### 3.4 Build System Dependency Graph

```
                   [Source Files (*.cpp)]
                             │
            ┌────────────────┴────────────────┐
            ▼                                 ▼
      [Native Makefile]               [CMakeLists.txt]
            │                                 │
            ├───────────► [Compilation] ◄─────┤
            │         (-std=c++17 -Wall)      │
            ▼                                 ▼
     [Object Files (.o)]             [Object Files (.o)]
            │                                 │
            └────────────────┬────────────────┘
                             ▼
                 [libaegis.a Static Archive]
                             │
            ┌────────────────┴────────────────┐
            ▼                                 ▼
     [Daemon Binaries]                 [Test Binaries]
   - aegis_coordinator               - test_build_smoke
   - aegis_intersection_node         - test_system_integration
   - aegis_ambulance                 - test_rpi_deployment
   - aegis_traffic_controller        - test_phase8_intelligent_scheduler
                                     - (14 additional suites...)
```

### 3.5 Build Commands

```bash
# 1. Unified Script Runner (Auto-detects CMake or Native Make)
./scripts/build.sh Debug          # Build Debug configuration
./scripts/build.sh Release        # Build Release configuration
./scripts/build.sh test           # Build and execute all test suites

# 2. Native Make Direct Workflow
make -j4 all                      # Compile static library and daemon binaries
make test                         # Run all 18 test suites sequentially
make clean                        # Purge build directory and generated binaries

# 3. CMake Direct Workflow
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
ctest --output-on-failure
```

---

# 4 Complete Software Architecture

AERS is structured as a multi-tier, distributed cyber-physical system. It bridges edge-level real-time traffic signal actuation with central fleet routing and clinical triage arbitration.

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                CENTRAL COORDINATOR (RPi 1)                             │
│                                                                                        │
│   ┌────────────────────────────────────────────────────────────────────────────────┐   │
│   │                         Intelligent Priority Scheduler                         │   │
│   │   ┌───────────────────────────┐         ┌──────────────────────────────────┐   │   │
│   │   │  ConflictDetectionEngine  │         │    ConflictResolutionManager     │   │   │
│   │   └─────────────┬─────────────┘         └──────────────────┬───────────────┘   │   │
│   │                 │                                          │                   │   │
│   │                 ▼                                          ▼                   │   │
│   │   ┌───────────────────────────┐         ┌──────────────────────────────────┐   │   │
│   │   │   PriorityEngine (Med)    │         │      ManualOverrideManager       │   │   │
│   │   └─────────────┬─────────────┘         └──────────────────┬───────────────┘   │   │
│   │                 │                                          │                   │   │
│   │                 └───────────────────┬──────────────────────┘                   │   │
│   │                                     ▼                                          │   │
│   │                         IntelligentPriorityScheduler                           │   │
│   └─────────────────────────────────────┬──────────────────────────────────────────┘   │
│                                         │                                              │
│   ┌─────────────────────────────────────▼──────────────────────────────────────────┐   │
│   │                              CoordinatorManager                                │   │
│   │   ┌────────────────────────┐  ┌─────────────────────┐  ┌───────────────────┐   │   │
│   │   │      GreenCorridor     │  │ IntersectionRegistry│  │TrafficCommandMgr  │   │   │
│   │   └────────────────────────┘  └─────────────────────┘  └───────────────────┘   │   │
│   │   ┌────────────────────────┐  ┌─────────────────────┐  ┌───────────────────┐   │   │
│   │   │       EventQueue       │  │    EventLogger      │  │SimulationRecorder │   │   │
│   │   └────────────────────────┘  └─────────────────────┘  └───────────────────┘   │   │
│   └─────────────────────────────────────┬──────────────────────────────────────────┘   │
│                                         │                                              │
│   ┌─────────────────────────────────────▼──────────────────────────────────────────┐   │
│   │                         Network Communication Layer                            │   │
│   │        [SocketServer (TCP:9000)]        │        [MQTT Transport Engine]       │   │
│   └─────────────────────────────────────────┼──────────────────────────────────────┘   │
└─────────────────────────────────────────────┼──────────────────────────────────────────┘
                                              │ POSIX Sockets (TCP/IP) & MQTT
                      ┌───────────────────────┼───────────────────────┐
                      ▼                       ▼                       ▼
       ┌────────────────────────┐┌────────────────────────┐┌────────────────────────┐
       │ INTERSECTION A (RPi 2) ││ INTERSECTION B (RPi 3) ││ INTERSECTION C (RPi 4) │
       │  [IntersectionNode]    ││  [IntersectionNode]    ││  [IntersectionNode]    │
       │  [SignalController]    ││  [SignalController]    ││  [SignalController]    │
       │  [HeartbeatManager]    ││  [HeartbeatManager]    ││  [HeartbeatManager]    │
       │  [SocketClient (TCP)]  ││  [SocketClient (TCP)]  ││  [SocketClient (TCP)]  │
       │  [Simulated/Live GPIO] ││  [Simulated/Live GPIO] ││  [Simulated/Live GPIO] │
       └────────────────────────┘└────────────────────────┘└────────────────────────┘
```

### 4.1 Subsystem Relationships
1. **Edge-to-Coordinator Link:** Each physical Intersection Node (RPi 2, 3, 4) executes `IntersectionNode`. At startup, it loads its node identity from JSON, connects over TCP to the Coordinator (RPi 1) on port 9000, sends a `REGISTRATION` packet, receives a `REGISTRATION_ACK`, and launches a 1 Hz keepalive thread (`HeartbeatManager`).
2. **Coordinator Preemption Dispatch:** When an ambulance mission is initiated, `CoordinatorManager` constructs a `GreenCorridor` mapping the sequence of intersections (`INT_A` $\to$ `INT_B` $\to$ `INT_C`). As the ambulance approaches, `TrafficCommandManager` generates preemption packets (`PREEMPT`, `FORCE_GREEN`) routed through `SocketServer` to the designated intersection node.
3. **Clinical Urgency Arbitration:** Patient vital signs collected by the ambulance are evaluated by `PriorityEngine`, calculating a dynamic 0–100 severity score.
4. **Multi-Ambulance Conflict Detection:** `ConflictDetectionEngine` monitors all active units. When arrival times at any shared intersection are within 10 seconds of each other, `IntelligentPriorityScheduler` executes deterministic multi-factor arbitration, and `ConflictResolutionManager` dispatches safe `PROCEED` and `DELAY` commands.
5. **Fail-Safe Operation:** If network connectivity between an Intersection Node and the Coordinator fails for longer than 3 missed heartbeats, `SignalController` automatically aborts preemption and transitions to a fail-safe state (isolated `FLASHING_YELLOW` or local fixed-time cycle).

---

# 5 Development History

The AERS system was developed across eight methodical phases, establishing foundational mechanics before layering complex scheduling and edge hardware deployment.

| Phase | Title | Primary Objective | Key Classes & Files Created | Status |
|---|---|---|---|---|
| **Phase 1** | Foundation & Configuration | High-performance utilities, thread-safe primitives, JSON parser, and configuration management. | `geo_utils`, `time_utils`, `uuid_utils`, `validator`, `json_helper`, `ThreadSafeQueue`, `CircularBuffer`, `NodeConfig` | **100% Verified** |
| **Phase 2** | Communication Infrastructure | Network transport layers, serialization of 12 ICD message schemas, POSIX sockets, and MQTT client. | `NetworkManager`, `Serializer`, `MessageRouter`, `AuthenticationManager`, `SocketClient`, `SocketServer`, `MosquittoTransport` | **100% Verified** |
| **Phase 3** | Coordinator Core Engine | Central decision engine, event queue, intersection registry, and dynamic green corridor tracking. | `CoordinatorManager`, `EventQueue`, `GreenCorridor`, `IntersectionRegistry`, `TrafficCommandManager`, `IntersectionState` | **100% Verified** |
| **Phase 4** | Ambulance Node & Kinematics | On-Board Unit (OBU), GPS kinematics filtering, patient vitals monitoring, and ad-hoc V2V safety broadcasts. | `AmbulanceNode`, `GpsTracker`, `VitalsMonitor`, `V2vModule`, `AmbulanceMissionSimulator` | **100% Verified** |
| **Phase 5A** | Traffic Signal & Pi Deployment | Direct edge signal controller FSM, fail-safe recovery, 4-node deployment configs, and startup scripts. | `SignalController`, `IntersectionNode`, `HeartbeatManager`, `NodeConfigLoader`, `rpi1`–`rpi4` deployment configs | **100% Verified** |
| **Phase 5B** | MQTT Integration & Edge Resilience | MQTT pub/sub integration for traffic nodes, reconnect resilience, and duplicate publication suppression. | `TrafficSignalNode`, `TrafficController`, `SignalStateMachine`, `PreemptionManager`, `RecoveryManager` | **100% Verified** |
| **Phase 6 & 7** | Dashboard Backend Services | Ingestion pipelines for hospital emergency dashboards and traffic management center consoles. | `HospitalDashboardBackend`, `TrafficDashboardBackend` | **100% Verified** |
| **Phase 8** | Multi-Ambulance Conflict Resolution | Intelligent multi-factor priority scheduling, conflict detection, clinical patient model, and operator override. | `ConflictDetectionEngine`, `IntelligentPriorityScheduler`, `ConflictResolutionManager`, `PatientProfile`, `MedicalDatabase`, `ManualOverrideManager`, `AmbulanceAwarenessService`, `MultiAmbulanceSimulation` | **100% Verified** |

---

# 6 Module Documentation

The codebase is partitioned into 19 self-contained modules located under `src/`:

### 6.1 `common` (`src/common/`, `include/aegis/common/`)
* **Purpose:** Core algorithms, mathematical calculations, and foundational data structures.
* **Responsibilities:**
  * Great-circle geodesic distance and bearing calculations (`geo_utils.cpp`).
  * High-resolution timestamps, elapsed time stopwatches, and ISO 8601 formatting (`time_utils.cpp`).
  * RFC 4122 v4 UUID generation (`uuid_utils.cpp`).
  * Zero-dependency string tokenization and JSON extraction (`json_helper.cpp`).
  * Data sanitization and range validation (`validator.cpp`).
  * Lock-free circular ring buffering (`circular_buffer.hpp`) and blocking thread-safe queues (`thread_safe_queue.hpp`).
* **Implementation Status:** Fully operational; verified by `test_common_foundation`.

### 6.2 `config` (`src/config/`, `include/aegis/config/`)
* **Purpose:** Static and dynamic system configuration parsing and validation.
* **Responsibilities:**
  * Defines `NodeConfig` containing network bindings, roles, and timeouts.
  * `NodeConfigLoader` parses deployment JSON files and performs path resolution fallbacks (supporting both root and `build/` working directories).
* **Implementation Status:** Fully operational; verified by `test_rpi_deployment`.

### 6.3 `communication` (`src/communication/`)
* **Purpose:** High-throughput, low-latency POSIX stream socket IPC over TCP/IP.
* **Responsibilities:**
  * `SocketServer`: Multi-threaded TCP server binding to `0.0.0.0` or specified IP, managing active client sessions.
  * `SocketClient`: Resilient TCP client with automatic reconnection and non-blocking I/O.
  * `Message`: Newline-delimited JSON packet framing containing sender, receiver, message type, timestamp, and payload.
* **Implementation Status:** Fully operational; verified by `test_communication_module`.

### 6.4 `network` (`src/network/`, `include/aegis/network/`)
* **Purpose:** Advanced transport layer supporting MQTT pub/sub, serialization, and connection lifecycle management.
* **Responsibilities:**
  * `NetworkManager`: Unified facade abstracting underlying transport implementations.
  * `Serializer`: Bidirectional JSON serialization for all 12 system message types.
  * `MessageRouter`: Hierarchical topic matching supporting MQTT wildcards (`+`, `#`).
  * `AuthenticationManager`: Token-based RBAC, HMAC verification, and mutual authentication.
  * `SimulationTransport`: In-memory zero-latency bus for regression tests without open network ports.
* **Implementation Status:** Fully operational; verified by `test_communication_infrastructure`.

### 6.5 `coordinator` (`src/coordinator/`, `include/aegis/core/`)
* **Purpose:** Central intelligence, state tracking, and corridor preemption orchestration.
* **Responsibilities:**
  * `CoordinatorManager`: Central engine driving mission lifecycles and signal overrides.
  * `EventQueue`: Thread-safe FIFO event queue with condition variable blocking for event-driven processing.
  * `IntersectionRegistry`: In-memory thread-safe database of all connected intersection edge nodes.
  * `GreenCorridor`: Tracking step-by-step progress of green wave routes across multiple intersections.
  * `TrafficCommandManager`: Factory for preemption and release command packets.
* **Implementation Status:** Fully operational; verified by `test_coordinator_core`, `test_coordinator_foundation`, `test_green_corridor`.

### 6.6 `intersection` (`src/intersection/`)
* **Purpose:** Edge intersection intelligence executing directly on Raspberry Pi nodes.
* **Responsibilities:**
  * `IntersectionNode`: Edge application daemon managing coordinator connectivity, command reception, and state dispatch.
  * `SignalController`: Physical signal state machine enforcing minimum green, yellow clearance, and all-red intervals.
  * `HeartbeatManager`: 1 Hz keepalive beacon emitter detecting link failure.
* **Implementation Status:** Fully operational; verified by `test_intersection_node`.

### 6.7 `medical` (`src/medical/`)
* **Purpose:** Clinical urgency modeling and physiological triage evaluation.
* **Responsibilities:**
  * `PatientProfile`: 14-attribute clinical entity modeling patient identity, diagnosis, and physiological vitals.
  * `PriorityEngine`: Algorithmic evaluation engine producing 0–100 emergency severity scores based on shock index, hypoxemia, vitals crisis thresholds, and incident delay.
  * `MedicalDatabase`: In-memory registry containing 16 realistic patient profiles covering all 12 diagnostic categories.
* **Implementation Status:** Fully operational; verified by `test_medical_priority`.

### 6.8 `scheduler` (`src/scheduler/`)
* **Purpose:** Multi-ambulance conflict detection and intelligent priority arbitration (Phase 8).
* **Responsibilities:**
  * `ConflictDetectionEngine`: Contention detection within configurable arrival time windows ($\Delta t \le 10$s).
  * `IntelligentPriorityScheduler`: Deterministic multi-factor scoring combining medical urgency (40%), emergency category (25%), ETA (20%), distance remaining (10%), and manual override bonus (5%).
  * `ConflictResolutionManager`: Generation of conflict actions (`PROCEED`, `HOLD`, `DELAY`, `RESUME`, `ALTERNATE_ROUTE`).
  * `ManualOverrideManager`: Operator intervention console supporting priority changes and emergency stops.
  * `AmbulanceAwarenessService`: Peer-to-peer proximity warning broadcaster.
  * `MultiAmbulanceSimulation`: Comprehensive scenario simulator testing 8 simultaneous emergencies.
* **Implementation Status:** Fully operational; verified by `test_conflict_detection` and `test_phase8_intelligent_scheduler`.

### 6.9 `logger` (`src/logger/`, `include/aegis/logger/`)
* **Purpose:** Comprehensive event recording and simulation telemetry capture.
* **Responsibilities:**
  * `Logger`: Thread-safe console/file logger with 6 log levels and microsecond timestamps.
  * `EventLogger`: Structured event recorder tracking 13 significant system event types with multi-attribute filtering and JSON export.
  * `SimulationRecorder`: Mission timeline playback and frame capture engine.
* **Implementation Status:** Fully operational; verified by `test_event_logger`.

### 6.10 `ambulance` (`src/ambulance/`, `include/aegis/edge/`)
* **Purpose:** Vehicle on-board unit telemetry, kinematics simulation, and registry management.
* **Responsibilities:**
  * `AmbulanceProfile`: Vehicle telemetry representation (speed, location, assigned patient, corridor ID, state).
  * `AmbulanceRegistry`: Thread-safe registry tracking all active emergency fleet units.
  * `GpsTracker`: Kinematics Kalman filter and dead-reckoning extrapolation.
  * `VitalsMonitor`: Telemetry collector for patient vital signs with automated alarm emission.
  * `V2vModule`: Direct ad-hoc vehicle-to-vehicle Basic Safety Message (BSM) broadcaster.
* **Implementation Status:** Fully operational; verified by `test_ambulance_node` and `test_ambulance_registry`.

### 6.11 `traffic_controller` (`src/traffic_controller/`, `include/aegis/edge/`)
* **Purpose:** Low-level signal state machine, preemption management, and GPIO abstraction.
* **Responsibilities:**
  * `SignalStateMachine`: Hardware-safe transitions (`RED`, `YELLOW`, `GREEN`, `ALL_RED`, `FLASHING_YELLOW`).
  * `PreemptionManager`: Emergency preemption entry and exit sequences.
  * `RecoveryManager`: Smooth restoration of civilian signal coordination post-emergency.
  * `GpioInterface`: Abstraction layer for Raspberry Pi physical GPIO pins (with full software simulation mode).
  * `TrafficSignalNode`: MQTT-integrated edge node daemon.
* **Implementation Status:** Fully operational; verified by `test_traffic_signal_node`.

### 6.12 `control` (`src/control/`)
* **Purpose:** Control Center manual intervention and operator console.
* **Responsibilities:**
  * `ManualOverride`: Record structure tracking dispatcher actions with cryptographic/audit timestamps.
  * `EmergencyOperatorConsole`: Interactive console for manually forcing green/red phases, pausing/resuming corridors, or executing a global emergency stop.
* **Implementation Status:** Fully operational; verified by `test_manual_override`.

### 6.13 `system` (`src/system/`, `include/aegis/safety/`)
* **Purpose:** System-level health monitoring, watchdog timers, and process lifecycle management.
* **Responsibilities:**
  * `SystemManager`: Central process manager aggregating node health reports.
  * `Watchdog`: Hardware and software watchdog servicing with 500ms timeout bounds.
  * `SafetyMonitor`: Runtime invariant validation and fail-safe trigger.
* **Implementation Status:** Fully operational; verified by `test_system_integration`.

### 6.14 `route_planner` (`src/route_planner/`, `include/aegis/core/`)
* **Purpose:** Urban road network graph modeling and dynamic path computation.
* **Responsibilities:**
  * `RoutePlanner`: Dijkstra-based shortest-path and minimum-transit-time path computation across road network vertices.
* **Implementation Status:** Fully operational.

### 6.15 `priority_scheduler` (`src/priority_scheduler/`, `include/aegis/core/`)
* **Purpose:** Legacy baseline priority scheduler (superseded by Phase 8 `IntelligentPriorityScheduler`).
* **Responsibilities:**
  * Basic triage-based ambulance arbitration.
* **Implementation Status:** Fully operational.

### 6.16 `hospital_dashboard` & `traffic_dashboard` (`src/*/`, `include/aegis/dashboard/`)
* **Purpose:** Backend data aggregation services for external web or graphical user interfaces.
* **Responsibilities:**
  * Ingesting live ETA countdowns, patient vitals, and intersection phase states for operator visualization.
* **Implementation Status:** Fully operational.

---

# 7 Class Documentation

### 7.1 `CoordinatorManager`
* **Namespace:** `aegis::coordinator`
* **Header:** `src/coordinator/CoordinatorManager.h`
* **Source:** `src/coordinator/CoordinatorManager.cpp`
* **Responsibilities:** Core coordination engine for the Central Coordinator (RPi 1). Manages intersection registrations, maintains an in-memory table of connected nodes, provides live console observability (`[CONNECT]`, `[REGISTER]`, `[HEARTBEAT]`, `[DISCONNECT]`, `[RECONNECT]`), constructs dynamic green corridors, processes incoming telemetry, and executes signal preemption commands.
* **Key Structures & Types:**
  * `struct ConnectedNodeInfo`: In-memory tracking record containing `nodeId`, `role`, `ipAddress`, `port`, `connectionStatus` (`"ONLINE"`/`"OFFLINE"`), `lastHeartbeatTimestamp`, `clientId`, and `everRegistered`.
* **Key Methods:**
  * `bool Start(uint16_t port, const std::string& host)`: Initializes `SocketServer` on the specified TCP port and host interface, starts the event worker thread.
  * `void Stop()`: Halts server listeners, clears active corridors, transitions in-memory nodes to `OFFLINE`, and disconnects clients cleanly.
  * `std::vector<ConnectedNodeInfo> GetConnectedNodes() const`: Returns ordered snapshot of tracked network nodes (`INT_A`, `INT_B`, `INT_C`, `AMB_01`, `HOSPITAL`, etc.).
  * `bool GetConnectedNodeInfo(const std::string& nodeId, ConnectedNodeInfo& outInfo) const`: Queries telemetry record for a specific node.
  * `std::string GetNetworkStatusString() const`: Generates formatted network state table.
  * `void PrintNetworkStatus() const`: Prints the formatted network state table to standard output.
  * `void PrintNetworkStatus(const CoordinatorManager& coordinator)`: Free helper function for network observability.
  * `bool CreateCorridor(const std::string& corridorId, const std::vector<std::string>& intersections)`: Instantiates a new green wave corridor.
  * `bool ActivateCorridor()`: Generates preemption green commands for active intersection and red commands for down-path intersections.
  * `bool AdvanceCorridor()`: Preempts the next intersection along the route and resets previous to normal or red.
  * `bool CompleteCorridor()` / `CancelCorridor()`: Concludes or cancels corridor, restoring normal phasing across all route junctions.
* **Lifecycle:** Instantiated at daemon startup; persists throughout coordinator execution.

### 7.2 `IntersectionNode`
* **Namespace:** `aegis::intersection`
* **Header:** `src/intersection/IntersectionNode.h`
* **Source:** `src/intersection/IntersectionNode.cpp`
* **Responsibilities:** Edge application daemon executing on Raspberry Pis 2, 3, and 4. Connects to the Coordinator, handles local signal timing, executes preemption commands, and emits heartbeats.
* **Key Methods:**
  * `bool Initialize(const config::NodeConfig& config)`: Configures node identifiers, network targets, and signal timings.
  * `bool Start()`: Establishes TCP connection to Coordinator, registers node, and starts `HeartbeatManager`.
  * `void Stop()`: Restores signals to normal cycling, stops heartbeat emission, and closes sockets.
  * `void HandleCommand(const std::string& commandPayload)`: Executes preemption instructions (`PREEMPT`, `FORCE_GREEN`, `RELEASE`).
  * `SignalController& GetSignalController()`: Returns reference to internal signal state machine.
* **Lifecycle:** Edge daemon lifetime; automatically attempts reconnection upon link loss.

### 7.3 `SignalController`
* **Namespace:** `aegis::intersection`
* **Header:** `src/intersection/SignalController.h`
* **Source:** `src/intersection/SignalController.cpp`
* **Responsibilities:** Direct traffic signal phase control and physical safety invariant enforcement.
* **Key Methods:**
  * `void SetState(SignalState newState)`: Safely transitions signal phase (`RED`, `YELLOW`, `GREEN`, `ALL_RED`, `FLASHING_YELLOW`).
  * `SignalState GetState() const`: Returns current active phase.
  * `bool RequestPreemption(const std::string& approach, uint32_t durationSec)`: Initiates emergency preemption sequence, honoring yellow clearance intervals.
  * `void ClearPreemption()`: Restores civilian signal timing cycle.
* **Invariants:** Mandatory $\ge 3.0$s yellow transition before red; strictly prevents simultaneous conflicting greens.

### 7.4 `PatientProfile`
* **Namespace:** `aegis::medical`
* **Header:** `src/medical/PatientProfile.h`
* **Source:** `src/medical/PatientProfile.cpp`
* **Responsibilities:** Comprehensive patient data model containing 14 clinical and physiological attributes.
* **Attributes:** `patientId`, `name`, `age`, `gender`, `disease` (`EmergencyType`), `heartRate`, `bloodPressure` (`systolic`/`diastolic`), `respirationRate`, `spo2`, `bloodLossPercent`, `painLevel`, `consciousness` (`ConsciousnessLevel`), `estimatedTimeSinceIncidentMinutes`.
* **Key Methods:** Full suite of getters and fluent setters, `CalculateShockIndex()`, `IsShockIndexCritical()`, `ToJson()`.

### 7.5 `PriorityEngine`
* **Namespace:** `aegis::medical`
* **Header:** `src/medical/PriorityEngine.h`
* **Source:** `src/medical/PriorityEngine.cpp`
* **Responsibilities:** Clinical triage evaluation engine producing deterministic 0–100 priority scores.
* **Algorithm Breakdown:**
  * Base Category Weights: Cardiac Arrest (95), Respiratory Failure (90), Stroke (85), Heart Attack (80), Internal Bleeding (75), Trauma (70), Organ Transport (65), Major Accident (60), Pediatric (55), Poisoning (50), Burn Injury (45), Pregnancy (35 routine / 85 crisis).
  * Physiological Penalties: Heart rate anomalies (+5 to +15), blood pressure crisis (+10 to +15), respiratory distress (+10 to +15), hypoxemia ($SpO_2 < 90\%$: +15), shock index $> 1.0$ (+10 to +20), Class III/IV blood loss (+15 to +20), unconsciousness (+15).
  * Time Escalation: $+1.0$ point per 5 minutes of incident delay.
  * Classification: `CRITICAL` ($\ge 75$), `HIGH` ($55–74$), `MEDIUM` ($35–54$), `LOW` ($< 35$).

### 7.6 `ConflictDetectionEngine`
* **Namespace:** `aegis::scheduler`
* **Header:** `src/scheduler/ConflictDetectionEngine.h`
* **Source:** `src/scheduler/ConflictDetectionEngine.cpp`
* **Responsibilities:** Temporal and spatial trajectory conflict detection across active ambulances.
* **Key Methods:**
  * `std::vector<ConflictEvent> DetectConflicts(const std::vector<AmbulanceProfile>& ambulances, double timeWindowSeconds)`: Scans all pairwise ambulance trajectories. If two or more units target the same intersection and $|ETA_1 - ETA_2| \le \text{timeWindowSeconds}$ (default: 10s), a `ConflictEvent` is generated.

### 7.7 `IntelligentPriorityScheduler`
* **Namespace:** `aegis::scheduler`
* **Header:** `src/scheduler/IntelligentPriorityScheduler.h`
* **Source:** `src/scheduler/IntelligentPriorityScheduler.cpp`
* **Responsibilities:** Multi-factor deterministic arbitration between conflicting emergency vehicles.
* **Scoring Formula:**
  $$\text{TotalScore} = (S_{\text{med}} \times 0.40) + (W_{\text{cat}} \times 0.25) + (W_{\text{ETA}} \times 0.20) + (W_{\text{dist}} \times 0.10) + (B_{\text{override}} \times 0.05)$$
  * Deterministic Tie-Breaking: If total scores are identical, ties are resolved deterministically by lower ETA, closer Euclidean distance, and lexicographical ambulance ID.

### 7.8 `ConflictResolutionManager`
* **Namespace:** `aegis::scheduler`
* **Header:** `src/scheduler/ConflictResolutionManager.h`
* **Source:** `src/scheduler/ConflictResolutionManager.cpp`
* **Responsibilities:** Generates actionable traffic command directives to execute arbitration decisions.
* **Action Types:**
  * `PROCEED`: Grants immediate green preemption to the winning ambulance.
  * `HOLD`: Holds conflicting approach signals red.
  * `DELAY`: Instructs secondary ambulance to reduce speed or holds approach until winner clears (staggered delay $\approx 15$s).
  * `RESUME`: Restores green corridor for delayed ambulance once intersection is clear.
  * `ALTERNATE_ROUTE`: Emits route recalculation advisory if delay exceeds clinical threshold.

### 7.9 `ManualOverrideManager`
* **Namespace:** `aegis::scheduler`
* **Header:** `src/scheduler/ManualOverrideManager.h`
* **Source:** `src/scheduler/ManualOverrideManager.cpp`
* **Responsibilities:** Control center dispatcher intervention interface.
* **Key Features:** Operator priority escalation/de-escalation, forced phase control (`FORCE_GREEN`, `FORCE_RED`), corridor cancellation, network-wide `EMERGENCY_STOP`, and immediate scheduler recalculation dispatch.

### 7.10 `EventLogger`
* **Namespace:** `aegis::logger`
* **Header:** `src/logger/EventLogger.h`
* **Source:** `src/logger/EventLogger.cpp`
* **Responsibilities:** Thread-safe structured event recording for forensic auditability.
* **Key Features:** Logs 13 event types with microsecond timestamps, multi-attribute querying (`GetEventsBySeverity`, `GetEventsByAmbulance`, `GetEventsByType`), JSON export, and synchronous event listener callbacks.

---

# 8 Configuration System

AERS implements a robust, schema-validated configuration subsystem capable of parsing and generating deployment files without third-party dependencies.

### 8.1 `NodeConfig` Structure
```cpp
namespace aegis::config {
    enum class NodeRole { COORDINATOR, INTERSECTION, AMBULANCE, TRAFFIC_CONTROLLER, UNKNOWN };

    struct NodeConfig {
        std::string nodeId;              // Unique node identifier (e.g., "COORDINATOR", "INT_A")
        NodeRole role;                   // Operational role
        std::string ipAddress;           // Local network binding IP
        uint16_t port;                   // Local listening port
        std::string coordinatorIp;       // Central Coordinator IP
        uint16_t coordinatorPort;        // Central Coordinator port (default: 9000)
        uint32_t heartbeatIntervalMs;    // Keepalive interval (default: 1000ms)
        uint32_t connectTimeoutMs;       // Connection timeout (default: 3000ms)
        uint32_t ackTimeoutMs;           // Registration ACK timeout (default: 3000ms)
        bool autoReconnect;              // Automatic reconnection flag (default: true)
        
        bool IsValid() const;
    };
}
```

### 8.2 Deployment Configuration Files (`config/deployment/`)
The repository includes dedicated configuration files for all four Raspberry Pi boards:

1. **`rpi1_coordinator.json` (Central Coordinator):**
   ```json
   {
     "nodeId": "COORDINATOR",
     "role": "Coordinator",
     "ipAddress": "192.168.1.101",
     "port": 9000,
     "coordinatorIp": "192.168.1.101",
     "coordinatorPort": 9000,
     "heartbeatIntervalMs": 1000,
     "connectTimeoutMs": 3000,
     "ackTimeoutMs": 3000,
     "autoReconnect": true
   }
   ```
2. **`rpi2_intersection_a.json` (Intersection Node A / `INT_A`):**
   ```json
   {
     "nodeId": "INT_A",
     "role": "Intersection",
     "ipAddress": "192.168.1.102",
     "port": 9001,
     "coordinatorIp": "192.168.1.101",
     "coordinatorPort": 9000,
     "heartbeatIntervalMs": 1000,
     "connectTimeoutMs": 3000,
     "ackTimeoutMs": 3000,
     "autoReconnect": true
   }
   ```
3. **`rpi3_intersection_b.json` (Intersection Node B / `INT_B`):**
   ```json
   {
     "nodeId": "INT_B",
     "role": "Intersection",
     "ipAddress": "192.168.1.103",
     "port": 9002,
     "coordinatorIp": "192.168.1.101",
     "coordinatorPort": 9000,
     "heartbeatIntervalMs": 1000,
     "connectTimeoutMs": 3000,
     "ackTimeoutMs": 3000,
     "autoReconnect": true
   }
   ```
4. **`rpi4_intersection_c.json` (Intersection Node C / `INT_C`):**
   ```json
   {
     "nodeId": "INT_C",
     "role": "Intersection",
     "ipAddress": "192.168.1.104",
     "port": 9003,
     "coordinatorIp": "192.168.1.101",
     "coordinatorPort": 9000,
     "heartbeatIntervalMs": 1000,
     "connectTimeoutMs": 3000,
     "ackTimeoutMs": 3000,
     "autoReconnect": true
   }
   ```

### 8.3 Path Resolution Fallback
`NodeConfigLoader::LoadFromFile` implements dual-path resolution:
* If the caller supplies a path starting with `../` (e.g. from within `build/`), but execution is at the repository root, the loader strips `../` and successfully opens the file.
* If the caller supplies a path starting with `config/` from within `build/`, the loader prepends `../` and loads the configuration without error.

---

# 9 Communication System

AERS features dual communication subsystems: a low-latency direct POSIX Stream Socket (TCP) module used for core Raspberry Pi cluster orchestration (Phase 5A), and an asynchronous MQTT Pub/Sub messaging backbone (Phase 2 & 5B).

### 9.1 TCP Socket Framing & Message Protocol
TCP streams are framed using standard newline (`\n`) delimiters. Every packet encapsulates a serialized `Message` object:
```json
{
  "type": "REGISTRATION",
  "senderId": "INT_A",
  "receiverId": "COORDINATOR",
  "payload": "{\"role\":\"Intersection\",\"ip\":\"192.168.1.102\",\"port\":9001}",
  "timestamp": 1700000000000
}
```

### 9.2 Handshake and Preemption Sequence

```
  Intersection Node (RPi 2/3/4)                      Coordinator (RPi 1)
               │                                              │
               │────────── TCP Connect (Port 9000) ──────────>│
               │                                              │
               │──────────── REGISTRATION Packet ────────────>│
               │                                              │ (Store in Registry)
               │<─────────── REGISTRATION_ACK ────────────────│
               │                                              │
               │ [Start Heartbeat Thread (1 Hz)]              │
               │                                              │
               │────────────── HEARTBEAT Packet ─────────────>│
               │                                              │ (Update LastSeen timestamp)
               │                                              │
               │                                              │ [Ambulance Dispatched]
               │<───────── PREEMPT (Approach: North) ─────────│
               │                                              │
               │ [Signal FSM: Yellow -> All-Red -> Green]     │
               │                                              │
               │───────────── STATUS (Phase: Green) ─────────>│
               │                                              │
               │                                              │ [Ambulance Clears Node]
               │<────────── RELEASE (Resume Normal) ──────────│
               │                                              │
               │ [Signal FSM: Resumes Fixed Cycle]            │
```

### 9.3 MQTT Topics Schema (Phase 5B)
* `aegis/ambulance/{id}/gps`: High-frequency kinematics broadcasts (10 Hz).
* `aegis/ambulance/{id}/vitals`: Patient physiological vitals telemetry (1 Hz).
* `aegis/intersection/{id}/commands`: Coordinator preemption directives (`PREEMPT`, `FORCE_GREEN`, `RELEASE`).
* `aegis/intersection/{id}/status`: Edge intersection phase and operational status reports.
* `aegis/coordinator/events`: System-wide audit event broadcasts.

---

# 10 Coordinator

The **Central Coordinator** executes as a centralized daemon on Raspberry Pi 1, providing global oversight, route progression, and command distribution.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        COORDINATOR MANAGER                             │
│                                                                        │
│  ┌───────────────────────────┐        ┌─────────────────────────────┐  │
│  │    IntersectionRegistry   │        │        GreenCorridor        │  │
│  │ - Registered Nodes        │        │ - Active Route              │  │
│  │ - IP & Port Mappings      │        │ - Current Corridor Step     │  │
│  │ - Heartbeat Freshness     │        │ - Step Progression Engine   │  │
│  └─────────────┬─────────────┘        └──────────────┬──────────────┘  │
│                │                                     │                 │
│                ▼                                     ▼                 │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                         Event Queue                              │  │
│  │   - Thread-Safe FIFO Buffer                                      │  │
│  │   - Event Dispatching Worker Loop                                │  │
│  └──────────────────────────────────┬───────────────────────────────┘  │
│                                     │                                  │
│                                     ▼                                  │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    TrafficCommandManager                         │  │
│  │   - Generates PREEMPT, FORCE_GREEN, FORCE_RED, RELEASE packets   │  │
│  └──────────────────────────────────┬───────────────────────────────┘  │
│                                     │                                  │
│                                     ▼                                  │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                     SocketServer (Port 9000)                     │  │
│  │   - Broadcasts Preemption Commands to RPi 2, 3, 4                │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────┘
```

### 10.1 Mission Lifecycle Management
1. **Creation:** Coordinator receives emergency incident dispatch specifying origin, destination, and intermediate intersection sequence (`INT_A` $\to$ `INT_B` $\to$ `INT_C`).
2. **Corridor Activation:** `GreenCorridor` locks the initial intersection approach into preemptive green, locking conflicting approaches into red.
3. **Step Progression:** As the vehicle's GPS kinematics cross the preemption threshold of downstream nodes, the corridor advances dynamically.
4. **Corridor Completion:** When the ambulance traverses the final intersection, the corridor terminates and normal civilian timing resumes across all nodes.

---

# 11 Intersection Nodes

Each edge Intersection Node (RPi 2, RPi 3, RPi 4) executes autonomously while accepting higher-priority overrides from the Coordinator.

### 11.1 Signal State Machine Transitions
The `SignalController` enforces deterministic state transitions:

```
              ┌───────────────────────────────┐
              │             RED               │◄──────────────────────────┐
              └───────────────┬───────────────┘                           │
                              │ (Timer Expired / Normal Cycle)            │
                              ▼                                           │
              ┌───────────────────────────────┐                           │
              │            GREEN              │                           │
              └───────────────┬───────────────┘                           │
                              │ (Cycle End OR Preemption Trigger)         │
                              ▼                                           │
              ┌───────────────────────────────┐                           │
              │            YELLOW             │ (Mandatory >= 3.0s)       │
              └───────────────┬───────────────┘                           │
                              │                                           │
                              ▼                                           │
              ┌───────────────────────────────┐                           │
              │           ALL_RED             │ (Mandatory >= 1.0s)       │
              └───────────────┬───────────────┘                           │
                              │                                           │
                              └───────────────────────────────────────────┘
```

### 11.2 Edge Local Decision Making & Fail-Safe
* If communication with the Central Coordinator drops for more than 3 consecutive heartbeat cycles ($> 3000$ms), the node enters degraded mode.
* In degraded mode, emergency preemption is immediately suspended, and the signal controller enters **FLASHING_YELLOW** caution or fixed-time local coordination.
* When connectivity is re-established, the node automatically re-registers and resumes normal preemption readiness without requiring a manual restart.

---

# 12 Scheduler

The **Phase 8 Intelligent Scheduling and Conflict Resolution Subsystem** guarantees conflict-free passage through urban intersections when multiple ambulances operate simultaneously.

### 12.1 Scheduling Workflow
```
     [Active Fleet Kinematics & Patient Vitals]
                         │
                         ▼
          ┌─────────────────────────────┐
          │   ConflictDetectionEngine   │
          │  (Time Window: Delta <= 10s)│
          └──────────────┬──────────────┘
                         │
          ┌──────────────┴──────────────┐
          ▼                             ▼
   [No Conflict]                [Conflict Detected]
   Standard Corridor                    │
   Progression                          ▼
                         ┌─────────────────────────────┐
                         │ IntelligentPriorityScheduler│
                         │ - Clinical Urgency (40%)    │
                         │ - Category Severity (25%)   │
                         │ - ETA Weight (20%)          │
                         │ - Distance Remaining (10%)  │
                         │ - Dispatcher Override (5%)  │
                         └──────────────┬──────────────┘
                                        │
                                        ▼
                         ┌─────────────────────────────┐
                         │  ConflictResolutionManager  │
                         │  - PROCEED -> Winner Unit   │
                         │  - HOLD    -> Conflicting   │
                         │  - DELAY   -> Secondary Unit│
                         └─────────────────────────────┘
```

### 12.2 Strict Mutual Exclusion Guarantee
The system enforces an unbreakable invariant: **Under no circumstances shall two conflicting intersection approaches receive simultaneous GREEN signal grants.** If two ambulances arrive simultaneously, the higher-priority unit is granted `PROCEED`, and the secondary unit is assigned `DELAY` with a mandatory clearance buffer ($\ge 15$s).

---

# 13 Medical Module

The medical subsystem grounds traffic preemption in clinical emergency medicine.

### 13.1 Emergency Categories and Severity Weights
The system evaluates 12 distinct medical conditions:
1. **Cardiac Arrest:** Weight 95 (Immediate cerebral anoxia risk)
2. **Respiratory Failure:** Weight 90 (Acute ventilatory compromise)
3. **Acute Stroke:** Weight 85 (Time-critical thrombolytic / thrombectomy window)
4. **Heart Attack (STEMI):** Weight 80 (Myocardial infarction necrosis risk)
5. **Internal Bleeding:** Weight 75 (Hemorrhagic shock Class III/IV)
6. **Severe Trauma:** Weight 70 (Polytrauma resuscitation)
7. **Organ Transport:** Weight 65 (Cold ischemia time expiration)
8. **Major Road Accident:** Weight 60 (High-energy vehicular collision)
9. **Pediatric Emergency:** Weight 55 (Rapid pediatric physiological decompensation)
10. **Acute Poisoning:** Weight 50 (Toxic ingestion / metabolic shock)
11. **Severe Burn Injury:** Weight 45 (Inhalation injury & plasma loss)
12. **Pregnancy Emergency:** Weight 35 (Routine labor) escalating to 85+ (Eclampsia / uterine rupture)

### 13.2 Decision Flow
```
[Patient Vitals Sampled] 
       │
       ▼
[Calculate Shock Index: Heart Rate / Systolic BP]
       │
       ▼
[Check Vitals Thresholds (SpO2 < 90%, HR > 120 / < 50, Systolic BP > 180 / < 90)]
       │
       ▼
[Apply Delay Penalty: +1.0 Point per 5 Min Elapsed]
       │
       ▼
[Compute Composite Priority Score (0 - 100)] ───► Classified: CRITICAL / HIGH / MED / LOW
```

---

# 14 Logging

AERS features a unified logging and forensic audit infrastructure across all nodes.

### 14.1 Logger Subsystems
1. **`Logger` (`src/logger/logger.cpp`):**
   * Multi-level synchronous logging (`TRACE`, `LOG_DEBUG`, `INFO`, `WARN`, `ERR`, `CRITICAL`).
   * Microsecond-precision ISO 8601 timestamps.
   * Colorized console output and synchronized thread-safe file output.
2. **`EventLogger` (`src/logger/EventLogger.cpp`):**
   * Captures 13 distinct tactical system events:
     `PATIENT_CREATED`, `AMBULANCE_REGISTERED`, `PRIORITY_CALCULATED`, `CONFLICT_DETECTED`, `CONFLICT_RESOLVED`, `GREEN_CORRIDOR_ACTIVATED`, `GREEN_CORRIDOR_COMPLETED`, `MANUAL_OVERRIDE`, `NODE_FAILURE`, `NODE_RECOVERY`, `HEARTBEAT`, `REGISTRATION`, `TRAFFIC_COMMAND`.
   * Multi-attribute filtering and structured JSON export.
3. **`SimulationRecorder` (`src/logger/SimulationRecorder.cpp`):**
   * Session-based mission playback recorder capturing timeline snapshots of fleet positions and intersection signal states.

---

# 15 Deployment

AERS is deployed across a 4-node physical Raspberry Pi hardware cluster communicating over standard Wi-Fi or Ethernet LAN.

### 15.1 Physical Cluster Topology

| Node Identity | Hardware Target | Role | Static IP Address | Listening Port | Operating System |
|---|---|---|---|---|---|
| **Raspberry Pi 1** | Raspberry Pi 4 Model B (4GB/8GB) | Central Coordinator | `192.168.1.101` | `9000` | QNX Neutrino RTOS 7.1 / Linux |
| **Raspberry Pi 2** | Raspberry Pi 4 Model B (2GB/4GB) | Intersection Node A (`INT_A`) | `192.168.1.102` | `9001` | Raspberry Pi OS 64-bit (Debian) |
| **Raspberry Pi 3** | Raspberry Pi 4 Model B (2GB/4GB) | Intersection Node B (`INT_B`) | `192.168.1.103` | `9002` | Raspberry Pi OS 64-bit (Debian) |
| **Raspberry Pi 4** | Raspberry Pi 4 Model B (2GB/4GB) | Intersection Node C (`INT_C`) | `192.168.1.104` | `9003` | Raspberry Pi OS 64-bit (Debian) |

### 15.2 Deployment Scripts
* **`scripts/start_all_nodes.sh`:** Orchestrates local multi-process execution of all four nodes with automated startup delays and graceful signal trapping (`SIGINT`, `SIGTERM`).
* **`scripts/start_rpi1_coordinator.sh`:** Standalone launch script for Raspberry Pi 1.
* **`scripts/start_rpi2_int_a.sh`:** Standalone launch script for Raspberry Pi 2.
* **`scripts/start_rpi3_int_b.sh`:** Standalone launch script for Raspberry Pi 3.
* **`scripts/start_rpi4_int_c.sh`:** Standalone launch script for Raspberry Pi 4.
* **`scripts/deploy_pi.sh`:** Automated SSH deployment script compiling and distributing binaries to remote Raspberry Pis.

---

# 16 Testing

The repository maintains an extensive test suite verifying mathematical utilities, communication framing, coordinator decision trees, edge signal states, multi-node deployment, and conflict resolution.

### 16.1 Test Suites Summary

| Test Binary | Category | Target Subsystem | Scope & Key Assertions | Status |
|---|---|---|---|---|
| `test_build_smoke` | Unit | Build Smoke | Validates memory allocations, basic math, and compiler sanity. | **PASS** |
| `test_common_foundation` | Unit | Foundation Utilities | Validates UUID, ISO 8601 time, Geo haversine distance, and JSON parsing. | **PASS** |
| `test_communication_infrastructure` | Unit | Network Layer | Validates 12 ICD schemas, topic routing, mTLS/HMAC tokens, and transports. | **PASS** |
| `test_communication_module` | Unit | POSIX Sockets | Validates TCP SocketServer, SocketClient, and message framing. | **PASS** |
| `test_coordinator_core` | Unit | Decision Engine | Validates intersection status, corridor advancement, and preemption packets. | **PASS** |
| `test_coordinator_foundation` | Unit | Coordinator Base | Validates event queues, intersection registry, and dispatch engines. | **PASS** |
| `test_green_corridor` | Unit | Green Wave Engine | Validates multi-intersection corridor activation, progression, and release. | **PASS** |
| `test_ambulance_node` | Unit | Ambulance Edge | Validates GPS kinematics filtering, vitals monitoring, and V2V broadcasts. | **PASS** |
| `test_traffic_signal_node` | Unit | Signal Node Edge | Validates 15 test cases covering FSM transitions, fail-safe recovery, and MQTT. | **PASS** |
| `test_intersection_node` | Unit | Intersection Node | Validates registration handshakes, ACK confirmation, and heartbeats. | **PASS** |
| `test_system_integration` | Integration | End-to-End System | Validates 3-node connection, corridor preemption, and node failover. | **PASS** |
| `test_rpi_deployment` | Integration | Pi Deployment | Validates JSON config loading for RPi 1-4, startup sequencing, and preemption. | **PASS** |
| `test_medical_priority` | Unit | Medical Triage | Validates 12 disease models, shock index, and priority classifications. | **PASS** |
| `test_ambulance_registry` | Unit | Fleet Registry | Validates vehicle registration, mutators, search, and thread-safe queries. | **PASS** |
| `test_conflict_detection` | Unit | Conflict Engine | Validates contention between 2, 3, and 4 ambulances with strict mutual exclusion. | **PASS** |
| `test_manual_override` | Unit | Operator Console | Validates dispatcher priority changes, forced green/red, and emergency stop. | **PASS** |
| `test_event_logger` | Unit | Telemetry & Audit | Validates 13 event types, filtering, multi-threaded logging, and JSON export. | **PASS** |
| `test_phase8_intelligent_scheduler` | Unit | Phase 8 Scheduler | Validates 8 integrated suites covering multi-factor arbitration and simulation. | **PASS** |

### 16.2 Test Execution Notes
* **Latest Linux Build Passes:** The complete test suite compiles and passes 100% on both Linux x86_64/ARM64 and macOS.
* **Deployment Test Fixes:** Deployment configuration tests (`test_rpi_deployment`) incorporate dual-path fallback resolution to ensure tests pass whether invoked directly from the repository root or within the `build/` directory.

---

# 17 Repository Statistics

* **C++ Source Files (`.cpp`):** 104 files (~22,376 lines of code)
* **C++ Header Files (`.h`, `.hpp`):** 90 files (~8,559 lines of code)
* **CMake & Makefile Scripts:** 29 files (~899 lines of code)
* **JSON Configuration Files:** 7 files (~138 lines)
* **Shell Scripts (`.sh`):** 9 files (~313 lines)
* **Engineering Markdown Documentation:** 48 files (~11,615 lines)
* **Static Libraries:** 1 (`libaegis.a`)
* **Daemon Executables:** 4 (`aegis_coordinator`, `aegis_intersection_node`, `aegis_ambulance`, `aegis_traffic_controller`)
* **Test Suites:** 18 executable test binaries (100% passing)
* **Total Project Size:** ~43,900 lines across 287 tracked repository files.

---

# 18 Current Capabilities

1. **Autonomous Green Wave Progression:** Creation and advancement of multi-intersection green corridors without manual intervention.
2. **Clinical Urgency Evaluation:** Dynamic calculation of patient priority scores (0–100) based on 12 medical conditions and real-time physiological vitals.
3. **Multi-Ambulance Conflict Arbitration:** Deterministic resolution of intersection contention across multiple converging ambulances within a 10-second conflict window.
4. **Strict Signal Mutual Exclusion:** Mathematical and architectural guarantee that two conflicting green phases can never be granted simultaneously.
5. **Fail-Safe Signal Degradation:** Automatic edge reversion to flashing yellow or local coordination upon communication timeout.
6. **Manual Dispatcher Override:** Full control center override suite (priority change, force green, force red, emergency stop) with immediate scheduler recalculation.
7. **Distributed Raspberry Pi Deployment:** Turnkey multi-node configuration and automated startup scripts for a 4-Raspberry Pi cluster.
8. **Forensic Audit Logging:** Tamper-evident, structured event logging covering 13 tactical system events with JSON export.
9. **Zero Third-Party Core Dependencies:** Standalone POSIX socket networking, custom JSON parsing, thread-safe queues, and standard C++17 library foundations.

---

# 19 Known Limitations

1. **Simulated Hardware GPIO:** Traffic signal lamp actuation currently utilizes `SimulationGpioInterface` (tracking state in memory) pending connection to physical relay boards.
2. **Simulated GPS Kinematics:** Vehicle positions are generated via internal route kinematics simulators rather than physical NMEA-0183 UART serial GPS receivers.
3. **In-Memory Fleet Registries:** Intersection and ambulance registries are maintained in memory; they do not currently persist to an external database across coordinator restarts.
4. **Single-Subnet Networking:** Default deployment configuration files assume a flat IPv4 `/24` local network (`192.168.1.0/24`).
5. **Plaintext Socket Communication:** POSIX stream socket connections run standard TCP; production deployments across public WANs require an underlying VPN, WireGuard, or SSH tunnel.

---

# 20 Future Development

The following placeholders represent planned future phases. **These are architectural placeholders only and must not be implemented until formally scheduled.**

### 20.1 Phase 9: Hardware Interfacing, Physical Relay Drivers & NMEA GPS Integration
* *Objective:* Transition from simulated GPIO to physical Raspberry Pi 40-pin GPIO headers, optocoupled relay modules, and serial GPS hardware receivers.

### 20.2 Phase 10: Cellular C-V2X / 5G Sidelink Ad-Hoc Interoperability & Cloud Telemetry
* *Objective:* Implement direct ad-hoc PC5/Sidelink V2V and V2I communication channels alongside cloud telemetry replication.

### 20.3 Phase 11: AI-Powered Predictive Congestion Forecasting & Multi-City Grid Scaling
* *Objective:* Integrate machine-learning traffic flow models to clear traffic backlogs minutes before emergency vehicle arrival.

---

# 21 Development Rules

All developers and AI assistants contributing to AERS must adhere strictly to these engineering commandments:

1. **Update This Specification:** Whenever new code, classes, configurations, or modules are added or modified, this document (`AERS_MASTER_SOFTWARE_SPECIFICATION.md`) must be updated concurrently.
2. **Never Rename the Architecture:** The project name is **AERS**. Core namespaces (`aegis::`) and architectural layers must remain invariant.
3. **Source Code Is Authoritative:** If any existing documentation conflicts with the repository source code on the `develop` branch, the source code is the sole source of truth.
4. **Never Assume Features Exist:** Do not assume the existence of features, drivers, or modules unless verified in the repository.
5. **Maintain Raspberry Pi Compatibility:** All code must compile and execute cleanly on ARM64/ARMv7 Raspberry Pi hardware.
6. **Maintain POSIX / Linux / QNX Compatibility:** Avoid platform-specific extensions that break POSIX.1-2017, Linux, or QNX Neutrino RTOS compilation.
7. **Keep Deployment Scripts Synchronized:** Any change to ports, node IDs, or configuration schemas must be immediately reflected in `config/deployment/` and `scripts/`.
8. **Zero Regression Tolerance:** All 18 test suites must continue to pass with 100% success before any branch merge.
9. **Zero Compiler Warnings:** All code must compile cleanly under `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor`.
10. **Strict Signal Mutual Exclusion:** No code modification may permit conflicting green signal indications at any intersection.

---

# 22 AI Context Section

### 22.1 Project Identity & Philosophy
AERS is a mission-critical cyber-physical system. In safety-critical software engineering, determinism, clarity, and bounded execution times take precedence over clever abstractions. The codebase favors explicit, readable C++17 with RAII memory management, minimal macro usage, and zero unnecessary dependencies.

### 22.2 Naming Conventions
* **Namespaces:** Lowercase nested namespaces under `aegis::` (e.g., `aegis::coordinator`, `aegis::intersection`, `aegis::scheduler`, `aegis::medical`, `aegis::common`).
* **Classes & Structs:** UpperCamelCase (e.g., `CoordinatorManager`, `IntelligentPriorityScheduler`, `PatientProfile`).
* **Methods:** UpperCamelCase (e.g., `RegisterIntersection()`, `CalculateScore()`, `DetectConflicts()`).
* **Variables & Members:** lowerCamelCase with no trailing underscores (e.g., `nodeId`, `heartbeatIntervalMs`, `conflictWindowSeconds`).
* **Constants & Enums:** UPPER_SNAKE_CASE (e.g., `SignalState::ALL_RED`, `EmergencyType::CARDIAC_ARREST`).

### 22.3 Coding Standards
* C++17 standard compliance across all translation units.
* Smart pointers (`std::unique_ptr`, `std::shared_ptr`) for heap memory; never use raw `new` and `delete`.
* Thread safety via `std::mutex` and `std::lock_guard` / `std::unique_lock`.
* Condition variables (`std::condition_variable`) for event synchronization; never use busy-wait polling loops.
* Strong typing via enum classes (`enum class SignalState`, `enum class EmergencyType`).

### 22.4 Future AI Assistant Instructions
Before proposing or writing any code in this repository:
1. Read this document (`AERS_MASTER_SOFTWARE_SPECIFICATION.md`) completely to ground your understanding.
2. Verify that the task aligns with the 4-Raspberry Pi architecture (RPi 1: Coordinator, RPi 2-4: Intersections).
3. Do not introduce microcontrollers (ESP32/Arduino); all edge logic runs directly on Raspberry Pis.
4. Execute `make test` or `./scripts/build.sh test` to verify that all 18 existing test suites pass before and after your edits.
5. Conclude your session by updating this specification to document any newly created classes, methods, or configurations.
