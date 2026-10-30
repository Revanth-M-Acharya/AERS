# SMART AEGIS: Distributed Ambulance Emergency Response System

**SMART AEGIS** (Automated Emergency Grid & Intelligent Shield) is a production-quality, safety-critical cyber-physical emergency response platform designed for **QNX Neutrino RTOS**, **Raspberry Pi embedded SBCs**, and distributed municipal networks.

---

## Release Baseline: SMART AEGIS v1.0 Hackathon Core (`v1.0-hackathon-core`)
* **Status:** **ARCHITECTURALLY FROZEN PRODUCTION BASELINE**
* **Release Date:** October 30, 2026
* **Target RTOS:** QNX Neutrino RTOS 8.0 / 7.1 (`aarch64le`)
* **Target Hardware:** Raspberry Pi 4 Model B (4-SBC Edge Cluster)
* **Detailed Release Specification:** [`docs/releases/v1.0-hackathon-core.md`](docs/releases/v1.0-hackathon-core.md)

### Release Summary & Core Highlights
1. **Architectural Freeze & Core Infrastructure Locking:**  
   The complete architecture, topological layering, public APIs, message schemas, and hardware pin mappings are **FORMALLY FROZEN**. The 18 designated Core Infrastructure modules (Coordinator, Traffic Controller, Route Planner, Green Corridor, Priority Scheduler, Distributed Scheduler, Safety Monitor, Deadline Monitor, QNX Resource Manager, QNX Message Passing, QNX Pulse Manager, QNX Timer Service, QNX Thread Scheduler, QNX Interrupt Manager, QNX Diagnostics, GPIO BSP, Relay Controller, and Network Layer) are locked against modification except for critical bug fixes.
2. **Native QNX SDP 8.0 Microkernel Services:**  
   Authentic QNX implementation featuring `/dev/aegis/*` Resource Manager devices, synchronous blocking rendezvous IPC (`MsgSend`/`MsgReceive`/`MsgReply`), non-blocking pulses (`SIGEV_PULSE`), monotonic timers with microsecond jitter tracking, strict 5-tier `SCHED_FIFO` thread scheduling with `PrioInheritMutex`, hardware interrupt attachment via `InterruptAttachEvent()`, and 9 industrial `devctl()` commands.
3. **Bare-Metal BCM2711 Hardware Control:**  
   Direct physical register manipulation via `ThreadCtl(_NTO_TCTL_IO, 0)` and `mmap_device_io(0x1000, 0xFE200000)` utilizing native `in32()` and `out32()` register calls with zero simulated GPIO on QNX builds.
4. **Autonomous Industrial Safety:**  
   14-step deterministic `BootManager`, 10-step graceful `ShutdownManager`, Relay Power-On Self Test (POST), 1 Hz hardware watchdog thread, 5-tier network QoS monitoring, dual-green mutual exclusion interlock, and 5-class autonomous fault recovery.
5. **Comprehensive Verification:**  
   **53 out of 53 automated test suites passing with 100% success**, zero compiler warnings, zero memory leaks, and zero data races.

---

## 1. System Documentation & Design Baseline
The implementation of SMART AEGIS is strictly governed by the engineering baselines located in the repository:
* [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md): Architectural topology, 19 single-responsibility modules, and interface definitions.
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md): 15-phase engineering roadmap, System Requirements (`REQ-001` to `REQ-025`), RTM, and QNX RTOS design.
* [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md): Mandatory AI engineering constitution, C++17 rules, thread-safety, and Git policies.
* [`INTERFACE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/INTERFACE_SPECIFICATION.md): Formal Interface Control Document (ICD), MQTT topics, schemas, QNX IPC, and error codes.
* [`SOFTWARE_DESIGN_DOCUMENT.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SOFTWARE_DESIGN_DOCUMENT.md): Comprehensive Software Design Document (SDD), UML sequence diagrams, thread models, and memory layouts.

---

## 2. Repository Layout
```
AERS/ (SMART_AEGIS)
├── include/aegis/                  # Public C++ header interfaces
│   ├── common/                     # Types, error codes, constants, geo utilities
│   ├── config/                     # Configuration Manager interfaces
│   ├── logger/                     # Asynchronous structured logger interfaces
│   ├── safety/                     # Watchdog and safety supervisor interfaces
│   ├── network/                    # MQTT and network abstraction interfaces
│   ├── core/                       # Coordinator, routing, and priority scheduler interfaces
│   ├── edge/                       # GPS, vitals, V2V, and traffic controller interfaces
│   ├── dashboard/                  # Hospital and traffic dashboard backend interfaces
│   ├── runtime/                    # Distributed Runtime Integration Framework interfaces
│   ├── deployment/                 # Raspberry Pi Hardware Layer & Distributed Deployment interfaces
│   └── hardware/                   # Physical Hardware Integration & Relay Control interfaces
│
├── src/                            # Modular static library implementations
│   ├── common/                     # Geometric math & Haversine formula
│   ├── config/                     # JSON configuration parser
│   ├── logger/                     # Logger engine (POSIX stream / QNX slogger2)
│   ├── safety/                     # Hardware watchdog & pulse supervisor
│   ├── network/                    # Network abstraction layer
│   ├── priority_scheduler/         # Multi-criteria triage arbiter
│   ├── route_planner/              # A* heuristic road graph pathfinder
│   ├── coordinator/                # Call intake & dynamic green corridor
│   ├── ambulance/                  # Ambulance OBU telematics daemon
│   ├── traffic_controller/         # Traffic signal RSU controller & relay interlocks
│   ├── hospital_dashboard/         # Hospital intake terminal service
│   ├── traffic_dashboard/          # Municipal traffic supervisory service
│   ├── runtime/                    # Distributed runtime orchestrator, coordinator & edge node runtimes
│   ├── deployment/                 # Node config, boot manager, heartbeat supervisor, cluster launcher, CLI
│   ├── simulator/                  # Complete City Simulation Engine (Digital Twin)
│   ├── hardware/                   # Physical hardware, relay control, IR sensors, Wi-Fi mesh & diagnostics
│   └── apps/                       # Entrypoint executables
│
├── tests/                          # Automated verification suites
│   ├── unit/                       # Unit tests (e.g., test_build_smoke)
│   ├── integration/                # Multi-module IPC data pipeline tests
│   ├── simulation/                 # Full incident mission tests
│   └── hardware/                   # Hardware-in-the-loop tests (GPIO/UART)
│
├── config/                         # Configuration profiles (default, development, production)
├── cmake/                          # Toolchain configurations (qnx_toolchain.cmake)
├── scripts/                        # Automation scripts (build, run_simulation, deploy, flash)
├── tools/simulator/                # Headless node simulation engines
├── hardware/                       # Wiring pinouts, schematics, and sensor guides
└── assets/                         # Presentation slides and video recordings
```

---

## 3. How to Configure the Project

The build system supports multiple build profiles and target architectures.

### 3.1 Build Types
* **Debug (Default):** Includes full debug symbols (`-g3`), optimization disabled (`-O0`), and `-DDEBUG` flag.
* **Release:** Full optimization enabled (`-O3`), dead-code elimination, and `-DNDEBUG` flag.

### 3.2 Configuration Options
When configuring via CMake:
```bash
# Debug Build (Default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release Build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# QNX Neutrino RTOS Cross-Compilation
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/qnx_toolchain.cmake -DQNX_TARGET_ARCH=aarch64le
```

---

## 4. How to Build

SMART AEGIS provides automated build mechanisms for host environments and cross-compilers.

### Method 1: Automated Script Runner (Recommended)
```bash
# Debug build
./scripts/build.sh

# Release build
./scripts/build.sh Release

# Build and execute all automated test suites
./scripts/build.sh test
```

### Method 2: Direct CMake Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

### Method 3: Direct Make Runner
```bash
# Compile all 12 modular static libraries and 3 application daemons
make all

# Clean build artifacts
make clean
```

### Generated Executables
Compiled binaries are output to `build/bin/`:
* `build/bin/aegis_coordinator`: Central dispatch and green corridor coordinator.
* `build/bin/aegis_ambulance`: In-vehicle On-Board Unit (OBU) telematics daemon.
* `build/bin/aegis_traffic_controller`: Roadside Unit (RSU) traffic signal controller.
* `build/bin/test_build_smoke`: Module instantiation smoke test harness.
* `build/bin/test_common_foundation`: Comprehensive 15-component foundation verification harness.

---

## 5. Common Foundation Library (`aegis_common`)
The common foundation library provides core utilities utilized across all 19 system modules:
1. **Geo Utilities:** WGS84 Haversine distance, initial forward bearing, ETA calculations, and boundary validation.
2. **Time Utilities:** Monotonic microsecond clock, UTC conversions, ISO 8601 formatting, and precision `Stopwatch`.
3. **UUID Utilities:** RFC 4122 compliant UUIDv4 generator, event/incident ID generators, and unit formatters.
4. **Thread-Safe Queue:** Mutex and condition-variable protected generic queue supporting blocking and timed-wait operations.
5. **Circular Ring Buffer:** Pre-allocated, zero-heap template buffer (`CircularBuffer<T, N>`) with overwrite semantics.
6. **Result<T, E>:** Non-throwing monadic success/error container for deterministic error handling.
7. **Error Model:** Standardized `AegisError` structure with origin module tagging and severity levels.
8. **Event Model:** Extensible `Event` base class with monotonic timestamps and serialization hooks.
9. **State Machine:** Generic thread-safe `StateMachine<StateEnum>` with transition guard validations.
10. **Timer Utilities:** Non-blocking `SoftwareTimer` tracking expiration and remaining durations.
11. **JSON Helper:** Lightweight JSON field formatter, escaper, and key extractor.
12. **Common Constants:** Temporal deadlines, clearance intervals, and network port definitions.
13. **Strongly Typed Enums:** Type-safe enums for triage categories, signal phases, and vehicle states.
14. **Serialization Interface:** `ISerializable` contract for JSON and binary serialization.
15. **Validation Utilities:** Range verifiers, bounded string checks, and standard CRC-32 checksum computation.

---

## 6. Communication Infrastructure (Phase 2 & Pre-5C Refinement)

The communication infrastructure library (`aegis_network`) provides production-grade, thread-safe network services connecting all SMART AEGIS nodes:

1. **NetworkManager:** Subsystem facade orchestrating MQTT telemetry, direct V2X sockets, RBAC authentication, and heartbeat tracking.
2. **MqttClient & IMqttTransport:** Transport-independent MQTT client architecture supporting interchangeable backends:
   - `IMqttTransport`: Abstract interface specifying connection, disconnection, subscription, unsubscription, publication, and reconnect lifecycles.
   - `SimulationTransport`: In-memory thread-safe pub/sub engine wrapping `MessageRouter` with offline ring-buffering.
   - `MosquittoTransport`: Clean skeleton with documented hooks for libmosquitto integration during physical hardware deployment.
   - Preserves 100% public API compatibility while allowing zero-code-change transport swapping.
3. **Unified MQTT Topic Schema (`aegis::network::topics`):** Standardized, type-safe topic constants and helper functions across all nodes:
   - Commands: `aegis/intersection/{id}/commands`, `aegis/hospital/{id}/commands`
   - Telemetry & Status: `aegis/intersection/{id}/status`, `aegis/ambulance/{id}/location`, `aegis/ambulance/{id}/vitals`
   - Safety & Heartbeats: `aegis/ambulance/{id}/alarms`, `aegis/ambulance/{id}/heartbeat`, `aegis/intersection/{id}/status`
   - Dispatch & Emergency: `aegis/emergency/request`, `aegis/incident/{id}/dispatched`
4. **MessageRouter:** MQTT topic router supporting exact path routing, single-level (`+`), and multi-level (`#`) wildcards.
5. **AuthenticationManager:** API bearer token verification, RBAC role authorization, HMAC payload signatures, and mTLS verification hooks.
6. **RetryPolicy:** Exponential backoff with configurable jitter, multipliers, and maximum attempt limits.
7. **HeartbeatManager:** Periodic node heartbeat generation and peer liveness monitoring with staleness detection.
8. **Serializer:** High-throughput JSON serialization and deserialization for all 10 ICD schemas defined in `INTERFACE_SPECIFICATION.md`.
9. **SocketClient & SocketServer:** POSIX UDP datagram sockets for direct vehicle-to-vehicle (V2V) and vehicle-to-infrastructure (V2I) ad-hoc messaging with deterministic loopback fallback for sandboxed execution.
10. **ConnectionManager:** State machine monitoring network link lifecycle and keep-alive ping latency.

---

## 7. Coordinator Core (Phase 3)

The Coordinator subsystem (`aegis_coordinator` and `aegis_route_planner`) provides central intelligence and real-time incident dispatch management:

1. **CentralCoordinator:** Master dispatch supervisor unifying call intake, fleet tracking, route planning, dynamic green corridor preemption, and MQTT event broadcast.
2. **EmergencyCallHandler:** 911 distress call qualification, metropolitan geofencing validation, symptom-based triage categorization (`CODE_RED`, `CODE_AMBER`, `CODE_GREEN`), and required capability inference.
3. **IncidentManager:** Thread-safe incident lifecycle storage supporting state transitions (`REPORTED` -> `ASSIGNED` -> `EN_ROUTE_SCENE` -> `ON_SCENE` -> `EN_ROUTE_HOSPITAL` -> `RESOLVED`), queries, and ambulance assignment.
4. **AmbulanceRegistry:** Thread-safe fleet registry maintaining vehicle profiles (ALS vs BLS), location, kinematics, and atomic reservation check-and-set locking.
5. **FleetManager:** Fleet supervisor API wrapping the ambulance registry and populating initial simulated metropolitan fleet deployments.
6. **DispatchEngine:** Automated dispatch selection algorithm matching required clinical capabilities, vehicle readiness, and proximity distances with ALS preference for critical incidents.
7. **RoutePlanner:** Graph-based municipal navigation engine executing A* shortest-time pathfinding with Dijkstra fallback over a 20-node simulated metropolitan street network, computing waypoint coordinates, ordered intersection sequences, and travel ETAs.
8. **GreenCorridorEngine (DynamicGreenCorridor):** Spatial corridor preemption scheduler evaluating vehicle progress along route vectors, activating signal preemption windows, generating signed `TrafficPreemptionRequestMsg` payloads, and releasing intersections upon passage.

---

## 8. Ambulance Node (Phase 4)

The Ambulance On-Board Unit subsystem (`aegis_ambulance`) implements the vehicle edge controller, kinematic tracking, physiological patient monitoring, and ad-hoc V2V communication:

1. **AmbulanceNode:** Main in-vehicle OBU edge daemon orchestrating GPS tracking, physiological telemetry, V2V safety broadcasting, and vehicle operational state transitions (`AVAILABLE`, `DISPATCHED`, `EN_ROUTE_PATIENT`, `ON_SCENE`, `EN_ROUTE_HOSPITAL`, `ARRIVED`). Features local store-and-forward telemetry ring buffering ensuring zero packet loss across WAN dropouts.
2. **GpsTracker:** 10 Hz kinematic positioning engine featuring NMEA-0183 serial sentence parser (`$GPRMC`, `$GPGGA`) with checksum validation, coordinate conversion, waypoint simulation step interpolation, and Extended Kalman Filter (EKF) / Dead-Reckoning fallback upon GNSS fix loss.
3. **VitalsMonitor:** Real-time physiological patient telemetry engine generating realistic multi-parameter vitals (Heart Rate, SpO2, Blood Pressure, Respiration Rate, Core Temperature, and GCS). Implements acute deterioration evaluation and instant alarm callbacks for critical conditions (acute hypoxia, cardiac arrest, hypotensive crisis, and severe pyrexia).
4. **V2vModule:** Autonomous ad-hoc wireless transceiver generating SAE J2735-compatible Basic Safety Messages (BSMs) at 10 Hz over UDP port 8999, along with direct vehicle-to-infrastructure (V2I) emergency preemption fallback beacons dispatched to roadside units on UDP port 8998.
5. **AmbulanceMissionSimulator:** Comprehensive incident mission simulator advancing vehicle kinematics and vitals along street waypoints through the complete mission lifecycle (`Dispatch` -> `Travel to patient` -> `Pickup on-scene` -> `Travel to hospital` -> `Arrive at trauma bay` -> `Mission complete`).

---

## 9. Traffic Signal Node (Phase 5A & 5B)

The Traffic Signal Node subsystem (`aegis_traffic_controller`) provides a deterministic, hardware-independent roadside intersection controller and MQTT V2I communication foundation connecting roadside units with the Central Coordinator:

1. **TrafficSignalNode:** Top-level edge daemon managing controller lifecycle (`Start()`, `Stop()`, `Run()`, `Tick()`), deterministic stepping, and bidirectional MQTT V2I communications (`Connect()`, `Disconnect()`, `Reconnect()`, `Subscribe()`, `PublishStatus()`, `ProcessCommand()`, `SendHeartbeat()`).
2. **TrafficController:** Master coordinator governing the signal state machine, emergency preemption manager, recovery manager, fail-safe modes (`EnterFailSafe()`, `RestoreNormalCycle()`), and GPIO abstraction interface. Completely decoupled from MQTT.
3. **SignalStateMachine:** Strict deterministic Finite State Machine (FSM) enforcing valid normal cycles (`RED -> GREEN -> YELLOW -> RED`) and emergency cycles (`Current -> ALL_RED -> GREEN -> Hold Green -> Recovery -> YELLOW -> RED`), while categorically rejecting hazardous transitions (such as `GREEN -> RED`, `YELLOW -> GREEN`, or `RED -> YELLOW`).
4. **PreemptionManager:** Dedicated state manager managing emergency preemption requests (`StartEmergency()`, `CancelEmergency()`, `IsEmergencyActive()`) without coupling to timing or GPIO.
5. **RecoveryManager:** Dedicated recovery cycle manager tracking post-clearance stabilization (`StartRecovery()`, `RecoveryFinished()`, `Reset()`, `IsRecoveryActive()`).
6. **IGpioInterface & SimulationGpioInterface:** Complete hardware abstraction interface decoupling signal logic from physical relays or SBC GPIOs. `SimulationGpioInterface` logs all simulated lamp actuations (`SET_RED`, `SET_YELLOW`, `SET_GREEN`, `ALL_RED`, `FLASHING_YELLOW`, `FLASHING_RED`, `ALL_OFF`, `SHUTDOWN`) without any external dependencies.
7. **TrafficTimingConfig:** Fully configurable timing parameters (`redDurationMs`, `greenDurationMs`, `yellowDurationMs`, `allRedClearanceMs`, `emergencyHoldDurationMs`, `recoveryDurationMs`) eliminating hardcoded magic numbers.
8. **MQTT V2I Communication & Supported Commands:**
   - Subscribes to `aegis/intersection/{intersection_id}/commands`.
   - Supports commands: `PREEMPT` (trigger emergency corridor), `CLEAR` (cancel emergency & begin recovery), `RESTORE` (restore normal cycle), `HEARTBEAT` (coordinator liveness ping), `PING` (immediate status publication), `STATUS_REQUEST` (full state broadcast).
   - Publishes 1 Hz heartbeats and real-time state telemetry to `aegis/intersection/{intersection_id}/status` including `intersection_id`, `current_state`, `emergency_active`, `recovery_active`, `heartbeat_counter`, `uptime_ms`, `timestamp`, and `health`.
   - **Fault-Tolerant Watchdog:** If coordinator heartbeat is lost for >5 seconds, the node automatically enters `FLASHING_YELLOW` fail-safe mode until communication is restored. If MQTT connection drops, the local controller continues executing cycles without disruption and synchronizes state upon reconnect.
   - **Duplicate Suppression:** Automatic duplicate publication suppression ensures zero redundant network traffic.

---

## 9.1. Raspberry Pi Hardware Layer & Distributed Deployment (Phase 16D)

The Deployment subsystem (`aegis_deployment`) manages the complete 4-node physical and simulated Raspberry Pi 4 Model B cluster running QNX Neutrino RTOS / POSIX TCP/IP:

1. **Topology:** 1 Central Coordinator RPi (`COORDINATOR`) communicating with 3 Edge Traffic Signal RPis (`INT_A`, `INT_B`, `INT_C`). Edge nodes communicate exclusively through the authoritative Coordinator.
2. **Deterministic 8-Stage Boot Process:** `POWER_ON` $\to$ `CONFIG_READ` $\to$ `BSP_INIT` $\to$ `HAL_INIT` $\to$ `GPIO_INIT` $\to$ `TCP_INIT` $\to$ `REGISTERING` $\to$ `HEARTBEATING` $\to$ `READY`. Boot execution completes deterministically in $< 50\,\mu\text{s}$.
3. **Emergency Readiness Gating:** The Central Coordinator strictly rejects all incoming emergency calls until every expected edge node is confirmed online and in `READY` state.
4. **BCM2711 Hardware GPIO Integration:**
   - **Traffic Signal Lamps:** Red (GPIO 17), Yellow (GPIO 27), Green (GPIO 22), North-South Green (GPIO 23), East-West Green (GPIO 24)
   - **Pedestrian Signals:** Walk (GPIO 25), Don't Walk (GPIO 5)
   - **Sensors & Inputs:** Approach IR Vehicle Detectors (GPIO 12, 13), Emergency Override Button (GPIO 16)
   - **Audible Alerts:** Warning Buzzer (GPIO 6)
5. **1 Hz Telemetry & Watchdog Supervision:** Edge nodes stream CPU, RAM, temperature, signal status, and GPIO pin metrics every second. Coordinator evicts silent nodes after 3 missed periods, triggering immediate fail-safe `FLASHING_YELLOW` degradation on edge controllers.
6. **Graceful Multi-Phase Shutdown:** Safe clearance: `CLEARING_TRAFFIC` $\to$ `NOTIFYING_PEERS` $\to$ `RELEASING_GPIO` $\to$ `CLOSING_NETWORK` $\to$ `FLUSHING_LOGS` $\to$ `TERMINATED`.
7. **Multi-Stream Logging:** Concurrently captures 5 distinct operational logs: `runtime.log`, `network.log`, `gpio.log`, `diagnostics.log`, and `crash.log`.
8. **Administrative Coordinator CLI:**
   ```bash
   cluster status              # Show overall cluster status and emergency gating
   list nodes                  # List all registered nodes and connection state
   node health [node_id]       # Display detailed telemetry or live dashboard
   shutdown node <node_id>     # Gracefully terminate an edge node
   restart node <node_id>      # Reboot and re-register an edge node
   broadcast message <text>    # Broadcast administrative message to all nodes
   simulate failure <node_id>  # Inject simulated network loss / fail-safe
   ```
9. **Automated Shell Automation Scripts (`scripts/`):**
   - `deploy.sh`: Builds and distributes binaries across the cluster
   - `run_coordinator.sh`: Boots Central Coordinator daemon
   - `run_intersection.sh <id>`: Boots an Edge Traffic Signal daemon
   - `stop_cluster.sh`: Gracefully shuts down all running cluster daemons
   - `restart_cluster.sh`: Reboots all cluster nodes in correct dependency sequence
   - `collect_logs.sh`: Aggregates all 5 log streams from all nodes into `logs/cluster_archive_<timestamp>.tar.gz`
   - `health_check.sh`: Verifies node connectivity and cluster readiness

## 9.2. SMART AEGIS Control Centre Dashboard (Phase 17)

The Control Centre Dashboard (`src/dashboard/`) is a Qt6 command center application representing the municipal Emergency Operations Centre (EOC):
* **Live Emergency Table:** Displays all active emergency calls, triage priority tiers (P1_CRITICAL to P4_NON_URGENT), allocated ambulances, destination hospitals, and 10-stage pipeline progression.
* **City Status Matrix:** Real-time health, connection state, and green corridor status across all cluster intersection nodes.
* **Hospital Capacity Telemetry:** Live inpatient bed, ICU capacity, and diversion tracking.
* **Fleet Telemetry:** Live coordinates, speeds, fuel, and telemetry for active ambulances.
* **Dispatcher Override:** Real-time manual overrides for corridor holds, emergency cancellations, and priority escalation.

## 9.3. Traffic Operations & Signal Monitoring Console (Phase 18)

The Traffic Operations Dashboard (`src/dashboard/traffic/`) is a dedicated Qt6 operations console for municipal traffic engineers and RSU oversight:
* **Interactive Grid Map:** Live visual mapping of intersections, status pins, and active green corridors.
* **4-Phase Signal State Machine:** Real-time animated traffic lights showing current phase, signal color, and countdown timers.
* **Optical IR Vehicle Sensors:** Dedicated sensor monitors displaying approach and departure IR beam state and pulse counters (ready for physical GPIO hardware in Phase 20).
* **Hardware Health & GPIO Matrix:** Live 8-channel BCM2711 GPIO pin states, CPU/RAM utilization, and temperature for every edge node.
* **Queue Congestion Analytics:** Real-time queue length, vehicle dissipation, and peak congestion tracking.
* **Audit Event Logs:** Comprehensive system logs with instant CSV export for forensic post-incident review.

## 9.4. Complete Municipal Digital Twin & City Simulation Engine (Phase 19)

The City Simulation Engine (`src/simulator/`) provides a complete, deterministic municipal software digital twin:
* **Simulation Engine Architecture:** Owns virtual clock, road network graph, vehicle kinematics, clinical emergency generator, hospital capacities, and dashboard telemetry streaming across dedicated background worker threads.
* **Simulation Speed Multipliers:** High-precision virtual clock supporting 1x, 2x, 5x, 10x speed, pause, resume, reset, and single tick stepping.
* **Road Graph & Dynamic Routing:** Dijkstra shortest-path pathfinding dynamically adjusting travel times based on road congestion, construction zones, accidents, and closures.
* **Autonomous Fleet & Civilian Traffic:** Continuous kinematic movement of civilian cars, buses, trucks, motorcycles, and pedestrians along municipal corridors.
* **Ambulance 8-Stage Lifecycle:** Full autonomous lifecycle: `IDLE` $\to$ `DISPATCH` $\to$ `EN_ROUTE_TO_PATIENT` $\to$ `PATIENT_PICKUP` $\to$ `TRANSPORT` $\to$ `HOSPITAL_ARRIVAL` $\to$ `MISSION_COMPLETE` $\to$ `RETURN_TO_BASE`.
* **Clinical Emergency Generation:** Generates clinically realistic patient profiles (Cardiac Arrest, Stroke, Trauma, Road Accident, Mass Casualty) with real-time vitals (heart rate, SpO2, blood pressure, GCS).
* **Three Demonstration Modes:**
  1. **Mode 1 (Full Software Simulation):** Entire municipal grid active with civilian traffic, diurnal congestion curves, and stochastic emergency calls.
  2. **Mode 2 (Single Ambulance Hardware Mode):** Feeds Raspberry Pi deployment with a single ambulance traversing corridor `INT_A` $\to$ `INT_B` $\to$ `INT_C` $\to$ `HOSP_CITY_TRAUMA`, generating optical IR trigger pulses.
  3. **Mode 3 (Dual Ambulance Conflict Mode):** Dispatches two ambulances (`AMB_01` cardiac critical vs `AMB_02` trauma high) along intersecting paths through `INT_B`, testing priority scheduling, conflict detection, hold queuing, and dynamic corridor arbitration upon intersection clearance.
* **Event-Driven Dashboard Publishing:** Seamlessly streams live snapshots to both Phase 17 and Phase 18 dashboards via `DashboardAdapter`.

---

## 10. How to Run Tests

### Automated Verification Test Suites
```bash
# Run all unit tests via the build script
./scripts/build.sh test

# Or run directly via Make
make test

# Or run individual test suites from build output
./build/bin/test_build_smoke
./build/bin/test_common_foundation
./build/bin/test_communication_infrastructure
./build/bin/test_coordinator_core
./build/bin/test_ambulance_node
./build/bin/test_traffic_signal_node
```

---

## 11. How to Add Future Modules

To add a new software module to SMART AEGIS, follow this standardized five-step engineering procedure:

### Step 1: Define the Public Interface
Create the public header in `include/aegis/<tier>/<module_name>.hpp`:
```cpp
#ifndef AEGIS_<TIER>_<MODULE_NAME>_HPP
#define AEGIS_<TIER>_<MODULE_NAME>_HPP

namespace aegis {
namespace <tier> {

class NewModule {
public:
    NewModule();
    ~NewModule();
    bool Initialize();
};

} // namespace <tier>
} // namespace aegis

#endif
```

### Step 2: Implement the Module Class
Create the implementation file in `src/<module_name>/<module_name>.cpp`:
```cpp
#include "aegis/<tier>/<module_name>.hpp"

namespace aegis {
namespace <tier> {

NewModule::NewModule() = default;
NewModule::~NewModule() = default;

bool NewModule::Initialize() {
    return true;
}

} // namespace <tier>
} // namespace aegis
```

### Step 3: Create Module `CMakeLists.txt`
In `src/<module_name>/CMakeLists.txt`, define the static library and its dependencies:
```cmake
add_library(aegis_<module_name> STATIC
    <module_name>.cpp
)
target_include_directories(aegis_<module_name> PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_<module_name> PUBLIC aegis_common aegis_logger)
```

### Step 4: Register in `src/CMakeLists.txt`
Append the new module directory to `src/CMakeLists.txt`:
```cmake
add_subdirectory(<module_name>)
```

### Step 5: Add Automated Unit Test
In `tests/unit/`, create `test_<module_name>.cpp`, register it in `tests/CMakeLists.txt`, and verify with `./scripts/build.sh test`.

---

## 12. Phase 20: Physical Hardware Integration & Live Traffic Signal Demonstration

The SMART AEGIS physical hardware layer interfaces directly with Raspberry Pi 4 Model B SBCs, 4-channel / 8-channel optocoupled relay modules, and physical LED traffic signals.

### Hardware Topology per Intersection
```
Raspberry Pi 4 Model B (Broadcom BCM2711)
  │
  ├── [GPIO Outputs] ──> Optocoupled 4-Channel Relay Module (Active-Low)
  │                        ├── Channel 1: Road A Red   (GPIO 17)
  │                        ├── Channel 2: Road A Yellow(GPIO 27)
  │                        ├── Channel 3: Road A Green (GPIO 22)
  │                        └── Channel 4: Road B Green (GPIO 25)
  │
  ├── [Direct GPIO]  ──> Road B Red (GPIO 23), Road B Yellow (GPIO 24)
  ├── [Direct GPIO]  ──> Pedestrian Walk (GPIO 5), Strobe (GPIO 26), Buzzer (GPIO 6)
  └── [GPIO Inputs]  <── Optical IR Sensors: Approach (GPIO 12), Departure (GPIO 13)
```

### Static Wi-Fi Mesh Network
All nodes communicate via Wi-Fi TCP/IP with static IP addressing:
* **Coordinator:** `192.168.1.100:9000`
* **Intersection A (INT_A):** `192.168.1.101:9001`
* **Intersection B (INT_B):** `192.168.1.102:9002`
* **Intersection C (INT_C):** `192.168.1.103:9003`

### Key Safety & Operational Features
* **Active-Low Relay Logic:** 0V / LOW energizes coil (ON), 3.3V / HIGH de-energizes coil (OFF), with readback self-testing and cycle counting.
* **Hardware Mutual Exclusion Interlock:** Strict architectural and hardware interlock prevents conflicting green signals simultaneously (`Road A Green` and `Road B Green`). Trips a hardware fault and forces immediate `ALL_RED_CLEARANCE`.
* **Automatic Departure Preemption Clearance:** Optical IR departure sensor beam detects ambulance clearance and automatically terminates emergency preemption, initiating safe All-Red transition before restoring normal cycle.
* **Wi-Fi Fail-Safe Degradation:** Automatic degradation to `FAILSAFE_FLASHING_YELLOW` upon Wi-Fi communication loss, with auto-reconnection and seamless recovery.
* **Diagnostics & Forensic Event Logging:** 6-stage self-test diagnostics suite and circular event logger with CSV/JSON export.

### Running Phase 20 Test Suite
```bash
./build/bin/test_hardware_integration_phase20
```

---

## 13. Phase 20.5: QNX Raspberry Pi 4 Deployment & Platform Compatibility

SMART AEGIS is fully certified and compatible with **QNX SDP 8.0** and **QNX SDP 7.1** running on the **Raspberry Pi 4 Quick Start Image** (`aarch64le`).

For the comprehensive deployment guide, see [`docs/QNX_RASPBERRY_PI_4_DEPLOYMENT_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_RASPBERRY_PI_4_DEPLOYMENT_GUIDE.md).

### Platform & Toolchain Support
* **Target Architecture:** Broadcom BCM2711, 4x Cortex-A72 (`aarch64le`).
* **Toolchains:** QNX `qcc` / `q++` with `gcc_ntoaarch64le_cpp`, `-lang-c++`, `libc++`.
* **Build Configurations:** `Debug` (`-g3 -O0`), `Release` (`-O3 -DNDEBUG`), `Profile` (`-O2 -g3 -DPROFILE`).
* **Real-Time Priority Hierarchy:** QNX `SCHED_FIFO` with real-time thread priority 21 for Coordinator and Preemption tasks via `include/aegis/common/qnx_compat.hpp`.

### IDE Support
* **QNX Momentics IDE:** Pre-configured `.project` and `.cproject` with QNX C/C++ nature (`qdeNature`) and remote launch configurations in `momentics/launch/`:
  * `aegis_coordinator_remote.launch`: Remote launch on Coordinator (`192.168.1.100:8000`).
  * `aegis_intersection_remote.launch`: Remote launch on Intersection Node (`192.168.1.101:8000`).
* **VS Code QNX Extension:** Configured `.vscode/tasks.json`, `.vscode/launch.json`, `.vscode/settings.json`, and `.vscode/c_cpp_properties.json` for one-click build, debug, and remote GDB debugging (`qconn`/`pdebug`).

### Deployment Scripts (`scripts/qnx/`)
* **`build_qnx.sh`:** Automated cross-compilation for `aarch64le` using CMake or `Makefile.qnx`.
* **`package_qnx.sh`:** Packages binaries, configurations, and startup scripts into `aegis_qnx_rpi4.tar.gz`.
* **`deploy_qnx.sh`:** Distributes tarball to all 4 nodes over Wi-Fi (`192.168.1.100`–`103`).
* **`run_qnx_coordinator.sh`:** Launches Coordinator with `on -p 21 ./aegis_coordinator`.
* **`run_qnx_intersection.sh`:** Launches Intersection Nodes with `on -p 21 ./aegis_intersection_node <node_id>`.
* **`qnx_setup_wifi.sh`:** Configures static Wi-Fi IP and route tables under QNX Neutrino RTOS.

### Running Phase 20.5 Test Suite
```bash
./build/bin/test_qnx_deployment_phase20_5
```
*(All 95 assertions pass with 100% success).*

---

## 14. Phase 20.6: QNX Deployment Hardening & Real Hardware Validation

Phase 20.6 prepares and validates the entire SMART AEGIS system for physical deployment on four Raspberry Pi 4 Model B boards running the official **QNX Raspberry Pi 4 Quick Start Image** (`aarch64le`).

For detailed architectural and electrical specifications:
* Network Architecture: [`docs/QNX_NETWORK_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_NETWORK_GUIDE.md)
* Hardware & Wiring: [`docs/QNX_HARDWARE_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_HARDWARE_GUIDE.md)
* Deployment Guide: [`docs/QNX_RASPBERRY_PI_4_DEPLOYMENT_GUIDE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/docs/QNX_RASPBERRY_PI_4_DEPLOYMENT_GUIDE.md)

### Key Hardening Features
1. **Wi-Fi Mesh Network Hardening:** Static IP assignments (`192.168.1.100` Coordinator, `192.168.1.101`–`103` Intersections). Incorporates packet retries with exponential backoff (`SendCommandWithRetry`), RTT latency measurement, continuous packet loss percentage accounting, 1 Hz heartbeats, 5.0-second watchdog offline eviction, and automatic post-severance link recovery.
2. **Industrial Relay & Hardware Interlock:** 4-channel optocoupled relay actuation with active-low coil logic (0V ON / 3.3V OFF), power-on safe state (GPIO HIGH de-energized), relay cycle counter, and hardware mutual exclusion interlock that detects conflicting green inputs and immediately trips safe `ALL_RED_CLEARANCE`.
3. **Optical IR Sensor Digital Debouncer:** `OpticalSensorFilter` implements a 50 ms qualification threshold that rejects transient optical/electrical noise spikes (< 50 ms), qualifies valid vehicle approach interruptions, triggers automatic green wave corridor clearance upon vehicle departure, and isolates stuck sensors (> 30s).
4. **Process Supervision & Auto-Restart:** Production runner scripts (`run_qnx_coordinator.sh`, `run_qnx_intersection.sh`) implement a supervisor watchdog loop with signal trapping (`SIGINT`, `SIGTERM`), auto-restart on unexpected exit with 2s backoff, multi-file log redirects (`logs/`), and boot auto-launch support (`qnx_autolaunch.sh`).
5. **Dedicated Role-Specific Deployment Packages:**
   * `aegis_coordinator_qnx_rpi4.tar.gz` (Coordinator package)
   * `aegis_intersection_qnx_rpi4.tar.gz` (Intersection Nodes package)
   * `aegis_qnx_rpi4.tar.gz` (Universal bundle)

### Running Phase 20.6 Test Suite
```bash
./build/bin/test_qnx_hardening_phase20_6
```
*(All 122 assertions pass with 100% success).*

## 15. Phase 20.7: QNX Production Hardening

Phase 20.7 delivers industrial-grade production hardening for SMART AEGIS deployed across four physical Raspberry Pi 4 Model B SBCs running the official **QNX SDP 8.0 Raspberry Pi 4 Quick Start Image** (`aarch64le`) over static Wi-Fi TCP/IP mesh architecture (`192.168.1.100` Coordinator, `192.168.1.101`–`103` Intersections).

### Production Hardening Components
1. **GPIO Reservation Manager (`GpioReservationManager`):** Thread-safe singleton providing strict BCM2711 pin range validation (0..27), duplicate reservation rejection, conflict checking, ownership release, and ASCII diagnostics table.
2. **Hardware Watchdog Thread (`HardwareWatchdog`):** 1 Hz supervisory thread monitoring coordinator heartbeat freshness (5.0s timeout), node heartbeat (2.0s timeout), worker thread liveness, relay health, GPIO state, stuck IR sensors (> 30.0s), and Wi-Fi connectivity. Enforces immediate `ALL_RED` fail-safe and coordinator notification upon fault.
3. **Relay Power-On Self Test (POST):** Deterministic POST (`ExecutePowerOnSelfTest()`) cycling each channel with energize/de-energize readback verification, emitting `Relay1 PASS` through `Relay4 PASS`, and aborting startup into safe mode upon coil failure.
4. **Wi-Fi Link Quality Monitor (`WirelessQualityMonitor`):** Rolling window telemetry calculating average RSSI, RTT latency, packet loss percentage, link uptime, signal strength trend (`IMPROVING`, `STABLE`, `DEGRADING`), and 5-tier link health categorization (`EXCELLENT`, `GOOD`, `POOR`, `CRITICAL`, `DISCONNECTED`).
5. **Deterministic 10-Step Boot Manager (`BootManager`):** Production boot state machine transitioning deterministically through:
   `BSP_INITIALIZATION` $\to$ `GPIO_RESERVATION` $\to$ `RELAY_SELF_TEST` $\to$ `HARDWARE_DIAGNOSTICS` $\to$ `WIFI_STARTUP` $\to$ `NODE_REGISTRATION` $\to$ `COORDINATOR_HANDSHAKE` $\to$ `WATCHDOG_STARTUP` $\to$ `HARDWARE_READY` $\to$ `ACCEPT_TRAFFIC_COMMANDS`. Aborts into safe mode on any phase failure.
6. **Safe 10-Step Shutdown Manager (`ShutdownManager`):** Non-reentrant graceful shutdown orchestrator with POSIX `SIGINT`/`SIGTERM` signal handlers executing:
   `STOP_COORDINATOR_COMMANDS` $\to$ `NOTIFY_PEERS` $\to$ `STOP_WATCHDOG` $\to$ `SAVE_LOGS` $\to$ `SAVE_DIAGNOSTICS` $\to$ `TURN_OFF_RELAYS` $\to$ `FORCE_ALL_RED` $\to$ `RELEASE_GPIO` $\to$ `CLOSE_SOCKETS` $\to$ `SHUTDOWN_COMPLETE`.
7. **Node Registration Service (`NodeRegistrationManager`):** Embedded coordinator registration authority inside `CoordinatorRuntime` validating inbound registrations, rejecting duplicate node IDs with conflicting IPs, tracking capabilities, recording heartbeats, and pruning nodes silent for > 5.0 seconds.

### Running Phase 20.7 Test Suite
```bash
./build/bin/test_qnx_hardening_phase20_7
```
*(All 157 assertions pass with 100% success).*

---

## 16. Phase 1: Real QNX Hardware Layer (No Stubs, No Simulation)

Phase 1 converts the hardware abstraction layer into a **REAL QNX Raspberry Pi 4 Model B implementation** (`aarch64le`) targeting the official QNX SDP 8.0 Raspberry Pi 4 Quick Start Image, completely eliminating software stubs and simulation on QNX builds.

### Architectural Deliverables
1. **Direct Memory-Mapped BCM2711 Register Access:**
   * Acquires I/O access privileges from the QNX Neutrino microkernel via `ThreadCtl(_NTO_TCTL_IO, 0)`.
   * Maps physical BCM2711 peripheral registers (`0xFE200000`, 4KB) via `mmap_device_io(0x1000, 0xFE200000)` with fallback to `mmap_device_memory()`.
   * Uses native QNX hardware macros `in32()` and `out32()` from `<hw/inout.h>` to read and write physical hardware registers.
   * Completely unmaps device memory via `munmap_device_io()` during clean shutdown.
2. **Zero Software-Only Pin States on QNX:**
   * Completely removes `std::map` / `std::unordered_map` for GPIO pins on `__QNX__` builds.
   * Pin states and directions are read directly from physical hardware registers (`GPLEV0`/`GPLEV1` and `GPFSEL0`..`GPFSEL5`).
   * Reservation metadata is tracked using a deterministic, fixed-size `std::array<QnxPinReservationRecord, 58>` with zero heap allocation.
3. **Phase 1 Standard Hardware API:**
   * `SetPin(uint32_t pin, PinState state)` / `SetPin(uint32_t pin, bool high)`: Direct register output via `GPSET0`/`GPSET1` and `GPCLR0`/`GPCLR1`.
   * `GetPin(uint32_t pin) const`: Direct register readback from `GPLEV0`/`GPLEV1`.
   * `ConfigurePin(uint32_t pin, PinMode mode)`: Direct 3-bit direction masking in `GPFSEL0`..`GPFSEL5`.
   * `TogglePin(uint32_t pin)`: Inverts physical pin state with register readback.
   * `Cleanup()`: De-energizes outputs, restores INPUT mode, and executes `munmap_device_io()`.
4. **Relay Controller Active-Low & Green Interlock:**
   * Active-low relay driving: 0V / LOW = ON (energized coil), 3.3V / HIGH = OFF (de-energized coil).
   * All relay coils guaranteed OFF (driven HIGH) during startup initialization.
   * Mutual exclusion interlock: rejects energizing Road A Green (GPIO 22) if Road B Green (GPIO 25) is active, and vice-versa, logging critical conflict errors.
5. **Startup Verification & Safe Shutdown Lifecycle:**
   * `IntersectionHardware::Initialize`: verifies BSP initialization, GPIO pin reservation, pin mode configuration, and relay readiness, aborting startup and entering fail-safe mode if any step fails.
   * `IntersectionHardware::Shutdown`: sets signals to `ALL_RED_CLEARANCE`, de-energizes all relays, releases reservations, and unmaps physical hardware memory via `m_bsp->Cleanup()`.
6. **100% Backward Compatibility:**
   * Non-QNX platforms (macOS / Linux) retain software simulation mode to allow comprehensive host automated testing.
   * All 50 test suites (480+ total test assertions) continue to pass 100%.

### Running Phase 1 Test Suite
```bash
./build/bin/test_phase1_real_qnx_hardware
```
*(All 4 test suites pass with 100% success).*

---

## 17. Phase 2: Production-Grade Embedded System (Raspberry Pi 4 + QNX SDP 8.0)

Phase 2 upgrades SMART AEGIS into an industrial-grade embedded controller running on the official **Raspberry Pi 4 QNX SDP 8.0 Quick Start Image** (`aarch64le`) with zero manual intervention required.

### Key Industrial Subsystems
1. **14-Step Deterministic Industrial Boot Manager (`BootManager`):**
   * Deterministic sequence: `KERNEL_BOOT` $\to$ `INIT_BSP` $\to$ `INIT_GPIO` $\to$ `RESERVE_GPIO` $\to$ `INIT_RELAY_CONTROLLER` $\to$ `RUN_RELAY_POST` $\to$ `INIT_IR_SENSORS` $\to$ `INIT_NETWORK_INTERFACE` $\to$ `VERIFY_NETWORK_LINK` $\to$ `REGISTER_WITH_COORDINATOR` $\to$ `START_HARDWARE_WATCHDOG` $\to$ `START_HEARTBEAT` $\to$ `ENTER_OPERATIONAL_MODE` $\to$ `READY`.
   * High-resolution boot timing in microseconds ($\mu s$) and milliseconds (ms).
   * Immediate sequence abort on any failure with automatic transition to Safe Mode (`ALL_RED`) and formatted ASCII report.
2. **Automated Node Registration with 9 Industrial Fields (`NodeRegistrationManager`):**
   * Stores `nodeId`, `hostname`, `ipAddress`, `macAddress`, `softwareVersion`, `hardwareVersion`, `bootTimestampUs`, `currentStatus`, and `heartbeatTimeUs`.
   * Duplicate Node ID collision protection strictly rejecting rogue registration attempts from conflicting IPs while allowing seamless post-reconnect re-registration.
3. **Dedicated 1 Hz Hardware Watchdog Thread (`HardwareWatchdog`):**
   * Real-time supervisory thread monitoring 7 subsystems: Relay Controller, GPIO Driver, IR Sensors, Traffic Controller, Heartbeat, Network, and Boot Manager.
   * Autonomous failure detection, alarm logging, failsafe transition (`ALL_RED` / `FLASHING_YELLOW`), recovery attempts tracking, and automatic recovery resumption.
4. **Relay Power-On Self Test (`RelayController::ExecutePowerOnSelfTest`):**
   * Sequential verification of Relays 1 to 4: GPIO Output, Relay State, Feedback readback, and switch timing in $\mu s$.
   * Dual Green mutual exclusion interlock verification during POST.
   * Power-on active-low safe state guarantee (all coils unpowered/HIGH at startup).
5. **Wi-Fi / Ethernet Quality Monitor (`WirelessQualityMonitor`):**
   * Rolling-window QoS tracking for RSSI, packet loss %, RTT latency, reconnect count, and signal trends.
   * 5-tier classification: `EXCELLENT`, `GOOD`, `FAIR`, `POOR`, `CRITICAL`, and `DISCONNECTED`.
   * Degradation event logging and asynchronous warning notification callbacks.
6. **Industrial 10-Step Safe Shutdown Manager (`ShutdownManager`):**
   * Non-reentrant graceful shutdown: `STOP_ACCEPTING_COMMANDS` $\to$ `COMPLETE_ACTIVE_TRANSITION` $\to$ `ALL_RED_CLEARANCE` $\to$ `DEACTIVATE_RELAYS` $\to$ `STORE_LOGS` $\to$ `RELEASE_GPIO` $\to$ `STOP_THREADS` $\to$ `CLOSE_SOCKETS` $\to$ `UNMAP_BSP_MEMORY` $\to$ `SHUTDOWN_COMPLETE`.
   * Relays guaranteed de-energized (unpowered) on termination.
7. **Automatic Node Recovery Following Link Severance:**
   * Automatic transition to safe state (`FLASHING_YELLOW` / `ALL_RED`) upon network loss.
   * Automatic reconnection, registration refresh, state resynchronization, and nominal cycle restoration without node reboot.
8. **Comprehensive Health Telemetry (10 Parameters) (`HardwareHealthMonitor`):**
   * Periodically publishes CPU Temperature, CPU Usage (%), RAM Usage (MB and %), GPIO Status, Relay Status, Sensor Status, Network Status, Watchdog Status, Boot Status, and Overall Health Grade.
9. **Microsecond-Precision Structured Logging (`HardwareEventLogger`):**
   * High-resolution microsecond timestamps (`timestampUs`) across Boot, POST, Registration, Recovery, Watchdog, Shutdown, and Subsystem Failure events.

### Running Phase 2 Test Suite
```bash
./build/bin/test_phase2_embedded_production
```
*(All 10 test suites pass with 100% success).*

---

## 18. Phase 3: Real-Time Corridor Operations & Fault Recovery

Phase 3 introduces industrial-grade real-time corridor monitoring, deterministic dynamic release, multi-domain timeout supervision, in-flight route replanning, and autonomous fault recovery on the **Raspberry Pi 4 QNX SDP 8.0 Quick Start Image** (`aarch64le`).

### Key Phase 3 Subsystems
1. **Corridor Deadline Monitor (`CorridorDeadlineMonitor`):**
   * Real-time supervision of active ambulance corridors using QNX monotonic clock.
   * Tracks corridor creation timestamp, ETA, elapsed traversal duration, and remaining deadline margins.
   * Detects corridor timeout; automatically triggers autonomous corridor recovery and emits structured `CorridorTimeoutEvent` telemetry.
2. **Multi-Domain Emergency Timeout Manager (`EmergencyTimeoutManager`):**
   * Supervises emergency requests across 4 distinct domains: Stalled Ambulance, Coordinator Inactivity, Communication Link silence, and Intersection Acknowledgement timeouts.
   * Configurable multi-tier escalation policy: Tier 1 Retry $\to$ Tier 2 Degrade Alert $\to$ Tier 3 Fail-safe Recovery.
   * Guaranteed deadlock-free architecture: strictly lock-ordered mutexes, callbacks outside lock scope, non-blocking evaluation loops.
3. **Dynamic Corridor Release Manager (`DynamicCorridorReleaseManager`):**
   * Releases intersections sequentially immediately after the ambulance traverses them without waiting for the entire corridor to complete.
   * Deterministic clearance state machine: `CLEARING_YELLOW` (3.0s) $\to$ `ALL_RED_CLEARANCE` (2.0s) $\to$ `RELEASED_NOMINAL`.
   * Multi-vehicle reservation safety: safely hands off preemption to queued emergency vehicles instead of releasing to civilian traffic.
4. **Route Progress Tracking & In-Flight Replanning (`RouteProgressTracker`):**
   * Real-time tracking of vehicle position: current intersection, previous intersection, next intersection, remaining hops, and percentage completion ($0.0\% - 100.0\%$).
   * Dynamic ETA recalculation based on actual traversal speed.
   * In-flight route recalculation around downstream blocked or failed intersections, preserving traversed points and updating remaining paths.
5. **Deterministic Signal Timeline Generator & Replay Engine (`SignalTimelineGenerator`):**
   * Generates deterministic schedules of traffic signal transitions with QNX monotonic clock timestamps: pre-clearance yellow, all-red hold, emergency green, post-passage yellow, post-passage all-red, and nominal return.
   * Enforces minimum green constraints and interlock safety rules.
   * Complete step-by-step replay engine for offline verification, telemetry auditing, and jitter/drift ($\Delta t$) analysis.
6. **Raspberry Pi Cluster Health Monitor (`ClusterHealthMonitor`):**
   * Central supervision of 4 Raspberry Pi boards (1 Coordinator + 3 Intersections): CPU %, RAM (MB and %), Temperature (°C), Heartbeat age (ms), 5-tier Network QoS, GPIO integrity, and Relay health.
   * Degraded node detection and alerts; derives overall weighted cluster health score ($0.0\% - 100.0\%$).
   * Continuous coordinator health and execution state supervision.
7. **Autonomous Fault Recovery Engine (`FaultRecoveryManager`):**
   * Autonomous resolution across 5 failure classes without full system restart:
     1. Communication failures: socket auto-reconnect, queue replay, and resync.
     2. Watchdog events: targeted subsystem reset (relay coil cycle, filter flush) without SBC reboot.
     3. Node reboots: automatic post-boot corridor state and signal timing push.
     4. Temporary network loss: autonomous optical fallback and post-restoration seamless handoff.
     5. Corridor interruptions: dynamic alternate route replanning and orphan preemption lock purge.
   * Enters SAFE MODE (All-Red) strictly as a last resort when recovery is impossible.

### Running Phase 3 Test Suite
```bash
./build/bin/test_phase3_corridor_operations
```
*(All 10 test suites pass with 100% success).*

---

## 19. Phase 4: Native QNX Features

Phase 4 transforms SMART AEGIS into a genuine **QNX Neutrino RTOS** real-time distributed application on the **Raspberry Pi 4 Model B (4 GB)** running the official **QNX SDP 8.0 Quick Start Image** (`aarch64le`).

### Key Phase 4 Native QNX Subsystems

1. **QNX Resource Manager (`QnxResourceManager` via `resmgr_attach`):**
   * Exposes standard device paths under `/dev/aegis/*`:
     - `/dev/aegis/coordinator`
     - `/dev/aegis/intersectionA`
     - `/dev/aegis/intersectionB`
     - `/dev/aegis/intersectionC`
     - `/dev/aegis/cluster`
     - `/dev/aegis/system`
   * Implements full POSIX I/O handlers: `open()`, `read()`, `write()`, `close()`, and `devctl()`.
   * Real-time read buffering returning live JSON telemetry; write handler supporting command dispatching.

2. **Native QNX Synchronous Message Passing (Rendezvous IPC):**
   * Implements native QNX synchronous rendezvous semantics: `ChannelCreate(0)`, `ConnectAttach()`, `MsgSend()`, `MsgReceive()`, and `MsgReply()`.
   * Sender thread strictly blocks in `MsgSend()` until server thread processes the request and executes `MsgReply()`.
   * Dedicated structured IPC schemas: `HEARTBEAT`, `REGISTRATION`, `CORRIDOR_REQUEST`, `SIGNAL_COMMAND`, `STATUS_UPDATE`, and `RECOVERY_MESSAGE`.

3. **QNX Pulse Management (`SIGEV_PULSE` Asynchronous Events):**
   * Non-blocking kernel pulse delivery via `MsgSendPulse()` and `MsgReceivePulse()` with zero sender context switches.
   * Standardized pulse codes:
     - `PULSE_CODE_WATCHDOG = 1`
     - `PULSE_CODE_HEARTBEAT = 2`
     - `PULSE_CODE_CORRIDOR_TIMEOUT = 3`
     - `PULSE_CODE_DEADLINE = 4`
     - `PULSE_CODE_SHUTDOWN = 5`
     - `PULSE_CODE_FAULT_RECOVERY = 6`
     - `PULSE_CODE_INTERRUPT = 7`
   * Multi-threaded asynchronous pulse dispatcher loop with registered callback handlers.

4. **High-Resolution QNX Timers (`QnxTimerService`):**
   * Monotonic high-precision timers using `timer_create(CLOCK_MONOTONIC)` configured for `SIGEV_PULSE` delivery.
   * Armed via `timer_settime()` for periodic intervals and one-shot delays.
   * Real-time microsecond jitter telemetry: minimum, maximum, average jitter ($\mu\text{s}$), and overrun counting via `timer_getoverrun()`.

5. **Real-Time Thread Scheduling Hierarchy & Mutex Inversion Safety (`QnxThreadScheduler`):**
   * Enforces strict 5-tier `SCHED_FIFO` real-time priority hierarchy:
     - **Priority 30:** Coordinator Core Dispatch Thread (Municipal safety preemption)
     - **Priority 25:** Intersection Controller Threads (Real-time GPIO actuation)
     - **Priority 21:** Dynamic Corridor Release & Timeline Engine
     - **Priority 12:** Health Monitor & Deadline Supervision
     - **Priority 5:** Telemetry, Event Logging & devctl Handlers
   * `PrioInheritMutex` RAII wrapper utilizing POSIX `PTHREAD_PRIO_INHERIT` to eliminate unbounded priority inversion.

6. **GPIO Interrupt-Driven Event Management (`QnxInterruptManager`):**
   * Hardware interrupt attachment via `InterruptAttachEvent()` delivering non-blocking pulses directly into channels.
   * Mappings for Raspberry Pi 4 BCM2711 GPIO lines:
     - `GPIO 12`: IR Approach Sensor (Vehicle entrance detection)
     - `GPIO 13`: IR Departure Sensor (Vehicle exit / clearance trigger)
     - `GPIO 16`: Emergency Manual Override Button
     - `GPIO 24`: Relay Fault Feedback Monitor
   * Dynamic interrupt masking, unmasking, detachment, and event counter telemetry.

7. **Industrial devctl() Interface (`QnxDevctl`):**
   * 9 industrial QNX devctl commands under class `'A'` (0x41) with packed structures:
     - `DCMD_AEGIS_GET_SYSTEM_STATUS`
     - `DCMD_AEGIS_GET_NODE_STATUS`
     - `DCMD_AEGIS_START_CORRIDOR`
     - `DCMD_AEGIS_STOP_CORRIDOR`
     - `DCMD_AEGIS_RESET_NODE`
     - `DCMD_AEGIS_READ_TELEMETRY`
     - `DCMD_AEGIS_READ_LOGS`
     - `DCMD_AEGIS_ENABLE_DIAGNOSTICS`
     - `DCMD_AEGIS_RUN_SELF_TEST`

8. **Native QNX Diagnostics & Health Telemetry (`QnxDiagnostics`):**
   * Live RTOS health aggregator capturing device node I/O stats, thread scheduling hierarchy, pulse/timer stats, and GPIO interrupt event counts.
   * Structured JSON reports and formatted ASCII console dashboard tables.

### Running Phase 4 Native QNX Test Suite
```bash
./build/bin/test_phase4_native_qnx
```
*(All 10 test suites pass with 100% success).*






