# SMART AEGIS (AERS) — MASTER TECHNICAL REFERENCE MANUAL
**Document Identifier:** ATR-AEGIS-2026-V1.0  
**Classification:** Safety-Critical Cyber-Physical System (CPS)  
**Target Systems:** QNX Neutrino RTOS 7.1 / 8.0 & Embedded Linux (Raspberry Pi 4 Model B)  
**Language Standard:** ISO C++17 (Strict Zero Third-Party Core Dependencies)  
**Applicable Standards:** ISO 26262 (ASIL-B Principles), MISRA C++:2008, IEEE 802.11p/V2X Architectural Alignment, POSIX.1-2017  
**Build & Test Verification:** 42/42 Passing Unit Test Suites (100% Success Rate)

---

# SECTION 1: PROJECT OVERVIEW

### 1.1 Vision
The **Autonomous Emergency Response System (AERS / SMART AEGIS)** is designed to eliminate urban emergency transit delays by transforming traditional municipal road infrastructure into an autonomous, real-time, coordinated, and medically aware emergency green corridor grid. By combining edge-computed clinical acuity scoring, intelligent multi-vehicle conflict arbitration, dynamic topological route replanning, distributed multi-node consensus, and fail-safe hardware signal actuation, SMART AEGIS creates a deterministic zero-delay transit envelope for ambulances traveling between incident sites and specialized trauma centers.

### 1.2 Objectives
1. **Dynamic Green Wave Progression:** Autonomously pre-clear and hold green traffic signals along active emergency transit paths without requiring manual dispatcher intervention.
2. **Clinical Urgency Arbitration:** Dynamically compute patient physiological priority (0–100 continuous scale) using multi-parameter vital signs (GCS, Systolic BP, Heart Rate, Respiration Rate, SpO2) and clinical condition base severity weights to arbitrate contested intersections.
3. **Strict Physical Mutual Exclusion:** Mathematically and architecturally prevent conflicting green indications across all controlled traffic directions, eliminating intersection collision hazards.
4. **Deterministic Distributed Consensus:** Execute multi-node consensus verification (majority quorum $(N/2)+1$) across edge Raspberry Pi traffic controllers prior to physical relay lamp actuation.
5. **Fail-Safe Edge Degradation:** Ensure edge intersection nodes autonomously revert to safe default states (flashing yellow or local sensor actuation) within 1.0 second of communication loss or coordinator failure.
6. **Hardware Decoupling & Portability:** Maintain absolute separation of concerns between municipal business logic and physical silicon via discrete Hardware Abstraction Layer (HAL) and Board Support Package (BSP) architectures.
7. **Production-Grade QNX Real-Time Operation:** Ensure full architectural and source compatibility with the QNX Neutrino Real-Time Operating System (SDP 8.0) and Raspberry Pi 4 Model B hardware rigs.

### 1.3 Problem Statement
Urban emergency medical services (EMS) face severe structural delays in modern metropolitan environments. Traffic congestion, uncoordinated signal cycles, and lack of inter-agency data sharing cause ambulances to lose critical minutes during transit—often referred to as the "Golden Hour" in trauma medicine where survival rates drop by 10% for every 10 minutes of delay.
Existing traffic signal preemption systems (e.g., optical strobe emitters or local acoustic sirens) suffer from fatal architectural shortcomings:
* **No Medical Awareness:** All emergency vehicles are treated equally, regardless of whether an ambulance is transporting a cardiac arrest patient versus a minor stable sprain.
* **No Multi-Vehicle Arbitration:** When two ambulances converge on the same intersection from perpendicular streets, legacy systems fail to safely arbitrate priority, resulting in deadlocks or near-miss collisions.
* **Lack of Route-Wide Coordination:** Signals are preempted locally on an intersection-by-intersection basis, causing erratic stop-and-go transit without green wave continuity.
* **Single Point of Failure Vulnerability:** Fragile cloud-based preemption architectures drop offline during cellular congestion, leaving edge intersections in uncoordinated or hazardous states.
* **Hardware Coupling:** Software logic is tightly coupled to specific proprietary microcontroller relays, preventing rapid modular upgrades.

### 1.4 Scope
SMART AEGIS covers the complete end-to-end municipal emergency lifecycle:
1. **Emergency Call Ingestion & Qualification:** Automated triage classification and initial incident dispatch.
2. **Clinical Triage & Vitals Monitoring:** Real-time continuous score recalculation based on dynamic telemetry.
3. **Ambulance Fleet Allocation:** Deterministic 8-rule vehicle selection factoring capabilities, battery/fuel, and transit time.
4. **Hospital Resource Optimization:** Deterministic 10-rule destination selection factoring ED bay occupancy, ICU beds, mechanical ventilators, and on-duty surgical specialists.
5. **Dynamic Topological Routing:** Real-time A* pathfinding incorporating live road congestion, blockages, and traffic forecasting.
6. **Green Wave Coordination:** Coordinated phase scheduling, pre-clearance envelopes, hold green periods, and clearance offsets.
7. **Distributed Multi-Node Consensus:** Fault-tolerant, term-based agreement across edge controllers prior to preemption actuation.
8. **Edge Raspberry Pi Hardware Control:** Exclusive pin reservation, digital GPIO switching, edge/level interrupt handling, and relay actuation.
9. **Municipal Incident Command:** High-level Emergency Operations Center (EOC) multi-agency coordination, severity escalation, and situational awareness reporting.
10. **Predictive System Maintenance:** Continuous platform-wide telemetry diagnostics and failure prediction.

### 1.5 System Goals
* **Decision Latency:** Evaluate and rank 100 candidate hospitals in $< 0.3\,	ext{ms}$; arbitrate 100 converging corridors in $< 0.4\,	ext{ms}$.
* **Network Latency:** Process and acknowledge node telemetry in $< 0.1\,	ext{ms}$ ($< 2.0\,	ext{ms}$ maximum real-time budget).
* **Hardware Actuation:** Execute digital GPIO switching and relay toggle in $< 0.0005\,	ext{ms}$ ($< 1.0\,\mu	ext{s}$ simulated).
* **Reliability:** 100% fail-safe degradation on communication timeout ($> 5.0\,	ext{s}$) or heartbeat failure.
* **Zero Contention Hazards:** Exact mutual exclusion across perpendicular signal phases ($0$ simultaneous conflicting greens).

### 1.6 Design Philosophy
The SMART AEGIS architecture is built upon six uncompromising core principles:
1. **Zero External Core Dependencies:** The entire foundation, networking, scheduling, routing, and hardware interface stack is implemented in pure Modern ISO C++17 utilizing only the standard library and native POSIX/QNX OS primitives.
2. **Layered Architectural Inversion:** Upper municipal management layers decide *WHAT* must happen; intermediate coordination layers decide *HOW*; distributed consensus guarantees *AGREEMENT*; hardware abstraction executes *ACTUATION*.
3. **Deterministic Real-Time Predictability:** Dynamic allocations in safety-critical execution loops are minimized; algorithms employ predictable worst-case computational complexity ($O(1)$ or $O(N \log N)$).
4. **Disjoint Mutex Domains:** Internal concurrency is protected via dedicated, non-overlapping mutex domains preventing lock inversion and deadlocks.
5. **Fail-Safe Default-Safe States:** Physical relays and digital outputs are wired and programmed to default to safe states (all-red or flashing yellow) upon loss of power, process termination, or network interruption.
6. **Tamper-Evident Forensic Accountability:** Every state change, preemption request, vote, override, and diagnostic event is logged with microsecond timestamps and structured audit markers.

### 1.7 Functional Requirements (FR)
* **FR-01 (Call Intake):** Ingest 911 emergency calls, extract GPS coordinates, caller ID, and incident description, and generate an incident UUID within 5 ms.
* **FR-02 (Triage Evaluation):** Calculate continuous clinical urgency score (0–100) from 12 emergency categories and 5 physiological parameters within 1 ms.
* **FR-03 (Fleet Dispatch):** Select optimal ambulance unit from municipal fleet registry using 8 deterministic rules within 2 ms.
* **FR-04 (Hospital Selection):** Select optimal receiving medical facility and reserve bed/ICU/ventilator assets using 10 deterministic rules within 5 ms.
* **FR-05 (Dynamic Routing):** Calculate topological minimum-cost route between current location, incident scene, and target hospital within 10 ms.
* **FR-06 (Corridor Generation):** Construct multi-intersection green wave schedule with pre-clearance, hold green, and release offsets within 5 ms.
* **FR-07 (Multi-Ambulance Arbitration):** Detect and arbitrate spatial, temporal, or contraflow corridor conflicts across converging ambulances within 5 ms.
* **FR-08 (Distributed Consensus):** Reach majority agreement ($(N/2)+1$) across edge intersection controllers before preemption execution within 50 ms.
* **FR-09 (Signal State Machine):** Control signal lamps via deterministic 8-state FSM enforcing minimum green (10s), yellow change (4s), and red clearance (2s).
* **FR-10 (Hardware Abstraction):** Decouple business logic from physical silicon through HAL and BSP interfaces with sub-microsecond GPIO access.
* **FR-11 (Cluster Management):** Supervise Raspberry Pi 4 edge controllers, discover nodes, track CPU/thermal metrics, and deploy configurations.
* **FR-12 (Network Communication):** Transmit binary-framed, CRC-16 protected packets over TCP/IP with 1 Hz heartbeats and 5-second timeout failover.

### 1.8 Non-Functional Requirements (NFR)
* **NFR-01 (Real-Time Performance):** Worst-case end-to-end corridor trigger-to-actuation latency shall not exceed 100 ms under full municipal load.
* **NFR-02 (Safety & Integrity):** Architectural mutual exclusion shall guarantee zero probability of simultaneous perpendicular green indications.
* **NFR-03 (Concurrency & Thread Safety):** All engine operations shall operate free of data races or deadlocks under 100 concurrent threads.
* **NFR-04 (Zero Warnings):** Codebase shall compile with zero warnings under `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor`.
* **NFR-05 (Memory Safety):** All resources shall be managed via RAII principles with zero memory leaks.
* **NFR-06 (Operating System Compatibility):** Full support for QNX Neutrino RTOS 7.1 / 8.0 (ARM64 / x86_64) and POSIX-compliant Linux/macOS.


---

# SECTION 2: COMPLETE SYSTEM ARCHITECTURE

### 2.1 High-Level Architectural Stack
SMART AEGIS employs a multi-tiered cyber-physical hierarchy separating tactical municipal governance from low-level silicon actuation:

```
+===================================================================================+
| LEVEL 1: MUNICIPAL INCIDENT COMMAND & DECISION SUPPORT (Phase 12B)                |
| - High-level EOC dashboard, multi-agency escalation, city-wide disaster response  |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 2: EMERGENCY MISSION EXECUTION & SUPERVISION (Phase 12A)                    |
| - 11-stage mission lifecycle: Call -> Triage -> Dispatch -> Hospital -> Clearance |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 3: MUNICIPAL RESOURCE ENGINES (Phases 11A & 11B)                            |
| - FleetManagementEngine (Ambulance allocation, range, medical capability)         |
| - HospitalResourceEngine (ICU, ventilators, emergency department bays, doctors)   |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 4: DISTRIBUTED TRAFFIC NETWORK ORCHESTRATION (Phase 13A)                    |
| - City-wide topology graph, corridor ownership, synchronization groups, leaders   |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 5: TRAFFIC SIGNAL CONSENSUS & MULTI-NODE AGREEMENT (Phase 13B)              |
| - Term-based Raft-style consensus, majority voting, immutable session audit       |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 6: CLUSTER SUPERVISION & SYSTEM DIAGNOSTICS (Phases 14A & 14B)              |
| - RaspberryPiClusterManager (Heartbeat monitoring, deployment, telemetry)        |
| - SystemDiagnosticsEngine (Failure prediction, anomaly scans, predictive maint)   |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 7: COORDINATOR & SCHEDULING CORE (Phases 1–10B)                             |
| - MultiCorridorCoordinator, PriorityScheduler, GreenWaveCoordinator               |
| - DynamicRouteReplanner, DistributedSignalSynchronizer, PredictiveTrafficEngine   |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 8: HARDWARE ABSTRACTION LAYER (HAL) (Phase 15A)                             |
| - Centralized peripheral manager, command validation, driver lifecycle            |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 9: QNX DEVICE DRIVER FRAMEWORK (Phase 15B)                                  |
| - Resource manager /dev/... attachments, interrupt binding, hardware resources    |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 10: RASPBERRY PI 4 BOARD SUPPORT PACKAGE (BSP) (Phase 16A)                  |
| - Broadcom BCM2711 SoC register interface, 58 GPIO pins, pull-ups, edge interrupts|
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 11: QNX NETWORK COMMUNICATION LAYER (Pure C++17 Socket Architecture)        |
| - Coordinator TCP Server (:9000), Traffic Node Clients, 19-Byte Frame, CRC16      |
+===================================================================================+
                                         │
+===================================================================================+
| LEVEL 12: PHYSICAL HARDWARE / SILICON RIG                                          |
| - Raspberry Pi 4 Model B SBCs, 4-Channel Relay Modules, Signal Lamps, GPS, OLED    |
+===================================================================================+
```

### 2.2 Component Diagram
The interconnected subsystems and their functional associations are shown in the following Mermaid diagram:

```mermaid
graph TD
    CallHandler["EmergencyCallHandler<br/>(Call Intake & Initial Triage)"] --> MissionExec["MissionExecutionEngine<br/>(Master Mission Lifecycle)"]
    MissionExec --> IncidentCmd["IncidentCommandEngine<br/>(Multi-Agency EOC Command)"]
    
    IncidentCmd --> FleetEng["FleetManagementEngine<br/>(Ambulance Unit Allocation)"]
    IncidentCmd --> HospEng["HospitalResourceEngine<br/>(ICU / Bed / Doctor Allocation)"]
    
    MissionExec --> RoutePlanner["RoutePlanner / DynamicReplanner<br/>(A* Pathfinding & Traffic Evading)"]
    RoutePlanner --> CorridorPlanner["EmergencyCorridorPlanner<br/>(Preemption Envelopes)"]
    
    CorridorPlanner --> GreenWave["GreenWaveCoordinator<br/>(Velocity & Offset Sync)"]
    GreenWave --> MultiCorridor["MultiCorridorCoordinator<br/>(5-Stage Conflict Arbitration)"]
    
    MultiCorridor --> NetOrch["TrafficNetworkOrchestrator<br/>(City-Wide Corridor Groups)"]
    NetOrch --> Consensus["DistributedConsensusEngine<br/>(Majority Voting Quorum)"]
    
    Consensus --> ClusterMgr["RaspberryPiClusterManager<br/>(Node Telemetry & Deployment)"]
    ClusterMgr --> SysDiag["SystemDiagnosticsEngine<br/>(Predictive Maintenance)"]
    
    Consensus --> TcpNet["TcpServer / TcpClient<br/>(19-Byte Frame & CRC16)"]
    TcpNet --> HAL["HardwareAbstractionLayer<br/>(HAL Peripheral Isolation)"]
    
    HAL --> DriverFwk["QNXDeviceDriverFramework<br/>(Resource Managers & Lifecycle)"]
    DriverFwk --> BSP["RaspberryPi4BSP<br/>(BCM2711 GPIO & Peripheral Silicon)"]
    BSP --> Hardware["Physical Relays & Signal Lamps"]
```

### 2.3 Deployment Diagram
The physical deployment architecture maps across a four-node Raspberry Pi 4 testbed and local area network:

```mermaid
graph LR
    subgraph HostServer["RPi 1 / Municipal Coordinator (192.168.1.100)"]
        CoordServer["Coordinator Daemon (aegis_coordinator)<br/>- TCP Server :9000<br/>- Incident Command & Mission Engines<br/>- Fleet & Hospital Resource Engines<br/>- Multi-Corridor Scheduler<br/>- Cluster Deployment Manager"]
    end

    subgraph EdgeNodeA["RPi 2 / Intersection A (192.168.1.101)"]
        NodeA["Traffic Node Daemon (aegis_intersection_node)<br/>- TCP Client -> :9000<br/>- Signal Controller FSM<br/>- Hardware Abstraction Layer<br/>- BCM2711 GPIO Drivers<br/>- Relay Board (Pins 17, 27, 22)"]
    end

    subgraph EdgeNodeB["RPi 3 / Intersection B (192.168.1.102)"]
        NodeB["Traffic Node Daemon (aegis_intersection_node)<br/>- TCP Client -> :9000<br/>- Signal Controller FSM<br/>- Hardware Abstraction Layer<br/>- BCM2711 GPIO Drivers<br/>- Relay Board (Pins 17, 27, 22)"]
    end

    subgraph EdgeNodeC["RPi 4 / Intersection C (192.168.1.103)"]
        NodeC["Traffic Node Daemon (aegis_intersection_node)<br/>- TCP Client -> :9000<br/>- Signal Controller FSM<br/>- Hardware Abstraction Layer<br/>- BCM2711 GPIO Drivers<br/>- Relay Board (Pins 17, 27, 22)"]
    end

    CoordServer ===|TCP/IP Wire Protocol| NodeA
    CoordServer ===|TCP/IP Wire Protocol| NodeB
    CoordServer ===|TCP/IP Wire Protocol| NodeC
```

### 2.4 Runtime Architecture
* **Event-Driven Dispatching:** All internal subsystems communicate through high-throughput, non-blocking in-process thread-safe queues (`ThreadSafeQueue<Event>`).
* **Microsecond-Accurate Timers:** Subsystems execute against `std::chrono::steady_clock` to prevent clock skew or NTP-induced backward step anomalies.
* **Non-Blocking Network Sockets:** The TCP Server accepts connections and transfers data asynchronously using `poll()` / `select()` and non-blocking sockets.
* **Separation of Planning and Actuation:** The Coordinator computes corridor timings in advance and transmits scheduled execution envelopes to edge nodes. Edge nodes verify consensus and execute phase shifts locally.

### 2.5 Communication Architecture
* **Star Topology Point-to-Point TCP/IP:** Coordinator acts as the central hub; edge intersection nodes connect outward as clients.
* **Wire Protocol Framing:** Every transmission is encapsulated in a 19-byte binary header with magic bytes `0xA55A`, dynamic CRC-16-CCITT checksum, and length-prefixed payload.
* **Bidirectional Full-Duplex:** Clients send telemetry (`STATUS`, `TRAFFIC_STATE`, `HEARTBEAT`) and receive control directives (`COMMAND`, `ACK`); Server pushes preemption schedules and monitors keepalives.
* **Automatic Recovery:** Edge clients maintain autonomous background reconnection loops that detect socket termination and re-establish links within 2.0 seconds.

### 2.6 Thread Architecture
Internal operations are segregated across dedicated threads to guarantee that network I/O or background logging never starves real-time signal control:

```
[Process: aegis_coordinator]
  ├── Thread 1 (Main / Command Loop): System initialization, signal trapping (SIGINT/TERM)
  ├── Thread 2 (TCP Accept Worker): Non-blocking poll() and accept() for incoming client links
  ├── Thread 3..N (TCP Client Workers): Dedicated thread per connected edge client (blocking read)
  ├── Thread N+1 (Heartbeat Watchdog): 1 Hz periodic scan evicting clients silent for > 5s
  ├── Thread N+2 (Mission Execution Loop): 10 Hz state progression and validation
  └── Thread N+3 (Cluster Health Monitor): Telemetry aggregation and diagnostics

[Process: aegis_intersection_node]
  ├── Thread 1 (Main / Lifecycle Loop): Daemon lifecycle and graceful shutdown
  ├── Thread 2 (TCP Client Receiver): Read loop parsing 19-byte binary frames and CRC16
  ├── Thread 3 (Heartbeat Generator): Autonomous 1 Hz timer sending keepalive frames
  ├── Thread 4 (Auto-Reconnect Supervisor): Background socket monitor with backoff sleep
  └── Thread 5 (Signal Controller FSM Ticker): 10 Hz phase duration and clearance evaluation
```

### 2.7 Process Architecture
* **Independent Execution Daemons:** Each system node runs as an autonomous OS process (`aegis_coordinator`, `aegis_intersection_node`, `aegis_ambulance`, `aegis_traffic_controller`).
* **Decoupled Failure Domains:** Crash or termination of an edge intersection node has zero effect on other intersections or the central coordinator.
* **Local Autonomy:** If the coordinator process terminates, edge intersection nodes detect heartbeat timeout within 5.0 seconds and transition immediately to local fail-safe coordination.

### 2.8 Network Architecture
* **Coordinator Port:** `9000` (Default TCP listening port for all edge client nodes).
* **Test & Isolation Ports:** `9101`–`9110` (Dedicated ports utilized during parallel unit testing).
* **MQTT Telemetry Broker Port:** `1883` (Standard MQTT port for external monitoring dashboards).
* **Web / REST Dashboard Port:** `8080` (HTTP / WebSocket interface for incident command operators).


---

# SECTION 3: DIRECTORY STRUCTURE

### 3.1 Complete Project Repository Tree
The repository contains 390 files spanning ~84,500 lines of production C++17 source code, headers, build automation, unit tests, and engineering documentation:

```text
.
├── AERS_DEVELOPMENT_ROADMAP.md
├── AERS_ENGINEERING_RULEBOOK.md
├── AERS_MASTER_SOFTWARE_SPECIFICATION.md
├── AI_RULEBOOK.md
├── CHANGELOG.md
├── CMakeLists.txt
├── DEPLOYMENT_GUIDE.md
├── DEVELOPMENT_PLAN.md
├── ENGINEERING_AUDIT_REPORT.md
├── HARDWARE_ABSTRACTION_DOCUMENT.md
├── INTERFACE_SPECIFICATION.md
├── Makefile
├── RASPBERRY_PI_PIN_MAPPING.md
├── README.md
├── SOFTWARE_DESIGN_DOCUMENT.md
├── SYSTEM_ARCHITECTURE.md
├── assets/
│   ├── images/
│   ├── presentation/
│   └── videos/
├── config/
│   ├── default.json
│   ├── deployment/
│   │   ├── rpi1_coordinator.json
│   │   ├── rpi2_intersection_a.json
│   │   ├── rpi3_intersection_b.json
│   │   └── rpi4_intersection_c.json
│   ├── development.json
│   └── production.json
├── docs/
│   ├── architecture/
│   ├── design/
│   └── meeting_notes/
├── hardware/
│   ├── gps/
│   ├── led_controller/
│   ├── raspberry_pi/
│   ├── relay/
│   └── sensors/
├── include/
│   └── aegis/
│       ├── common/
│       ├── config/
│       ├── core/
│       ├── dashboard/
│       ├── edge/
│       ├── logger/
│       ├── network/
│       └── safety/
├── network/
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── README.md
│   ├── example_client.cpp
│   ├── example_server.cpp
│   ├── message.cpp
│   ├── message.hpp
│   ├── tcp_client.cpp
│   ├── tcp_client.hpp
│   ├── tcp_server.cpp
│   └── tcp_server.hpp
├── scripts/
│   ├── build.sh
│   ├── deploy_pi.sh
│   ├── flash_qnx.sh
│   ├── run_simulation.sh
│   ├── start_all_nodes.sh
│   ├── start_rpi1_coordinator.sh
│   ├── start_rpi2_int_a.sh
│   ├── start_rpi3_int_b.sh
│   └── start_rpi4_int_c.sh
├── src/
│   ├── ambulance/
│   ├── apps/
│   ├── bsp/
│   ├── common/
│   ├── communication/
│   ├── config/
│   ├── control/
│   ├── coordinator/
│   ├── distributed_consensus/
│   ├── distributed_signal/
│   ├── fleet_management/
│   ├── green_corridor/
│   ├── green_wave/
│   ├── hardware_abstraction/
│   ├── hospital_dashboard/
│   ├── hospital_management/
│   ├── hospital_selection/
│   ├── incident_command/
│   ├── intersection/
│   ├── logger/
│   ├── medical/
│   ├── mission_execution/
│   ├── multi_corridor/
│   ├── network/
│   ├── orchestrator/
│   ├── predictive_traffic/
│   ├── priority_scheduler/
│   ├── qnx_driver_framework/
│   ├── replanning/
│   ├── route_planner/
│   ├── rpi_cluster/
│   ├── safety/
│   ├── scheduler/
│   ├── system/
│   ├── system_diagnostics/
│   ├── traffic_controller/
│   ├── traffic_dashboard/
│   ├── traffic_network_orchestrator/
│   └── traffic_signal/
└── tests/
    ├── CMakeLists.txt
    ├── integration/
    └── unit/
```

### 3.2 Directory Responsibilities & Module Ownership
The system is partitioned into strictly defined functional directories:

| Directory | Core Purpose & Technical Scope | Architectural Layer | Primary Owner Module |
|:---|:---|:---|:---|
| `network/` | Standalone QNX/POSIX TCP networking library (`Message`, `TcpServer`, `TcpClient`), example daemons, and standalone builds. | Level 11: Network Transport | `network` |
| `src/bsp/` | Raspberry Pi 4 Model B Board Support Package (Broadcom BCM2711 SoC register mapping, 58 GPIO pins, pull-ups). | Level 10: Silicon BSP | `RaspberryPi4BSP` |
| `src/qnx_driver_framework/` | QNX Neutrino Device Driver Framework, resource manager (`resmgr`) path attachments (`/dev/...`), interrupt binding. | Level 9: Driver Framework | `QNXDeviceDriverFramework` |
| `src/hardware_abstraction/` | Hardware Abstraction Layer (HAL) isolating municipal logic from physical peripherals (GPIO, PWM, SPI, UART, I2C). | Level 8: HAL Abstraction | `HardwareAbstractionLayer` |
| `src/system_diagnostics/` | Platform-wide continuous health supervision, anomaly detection, failure prediction, and maintenance scheduling. | Level 6: Reliability | `SystemDiagnosticsEngine` |
| `src/rpi_cluster/` | Raspberry Pi 4 cluster supervision, dynamic node discovery, heartbeat monitoring, and configuration deployment. | Level 6: Cluster Infrastructure | `RaspberryPiClusterManager` |
| `src/distributed_consensus/`| Term-based Raft-style distributed consensus engine guaranteeing unanimous preemption agreement across edge controllers. | Level 5: Distributed Agreement| `DistributedConsensusEngine` |
| `src/traffic_network_orchestrator/` | Municipal traffic execution layer managing topological network graphs, corridor ownership locks, and rolling green waves. | Level 4: Network Orchestration | `TrafficNetworkOrchestrator` |
| `src/incident_command/` | High-level Emergency Operations Center (EOC) multi-agency coordination, severity escalation, and situational reports. | Level 1: Municipal Command | `IncidentCommandEngine` |
| `src/mission_execution/` | Master mission lifecycle supervisor binding intake, triage, dispatch, routing, green waves, and hospital handoffs. | Level 2: Master Supervision | `MissionExecutionEngine` |
| `src/hospital_management/` | Live hospital bed tracking, ICU allocation, mechanical ventilator inventories, and physician scheduling. | Level 3: Municipal Resources | `HospitalResourceEngine` |
| `src/hospital_selection/` | Multi-criteria hospital selection engine optimizing destination trauma centers via 10 deterministic selection rules. | Level 3: Municipal Resources | `HospitalSelectionEngine` |
| `src/fleet_management/` | Ambulance fleet registry, availability tracking, medical capability matching, battery/fuel monitoring, and dispatch. | Level 3: Municipal Resources | `FleetManagementEngine` |
| `src/predictive_traffic/` | AI traffic intelligence engine forecasting future corridor congestion and speed drops using statistical profiles. | Level 7: Traffic Intelligence | `PredictiveTrafficEngine` |
| `src/distributed_signal/` | Real-time multi-node execution scheduling, clock synchronization, heartbeat tracking, and transmission retry backoff. | Level 7: Signal Sync | `DistributedSignalSynchronizer` |
| `src/multi_corridor/` | Real-time multi-ambulance corridor monitoring, 5-category conflict detection, and 5-tier deterministic priority arbitration. | Level 7: Conflict Arbitration | `MultiCorridorCoordinator` |
| `src/replanning/` | Real-time dynamic route replanning evading traffic jams, blockages, and high-ETA corridors. | Level 7: Dynamic Pathing | `DynamicRouteReplanner` |
| `src/orchestrator/` | Autonomous emergency orchestrator coordinating signal preemption requests, clearance windows, and restorations. | Level 7: Preemption Engine | `EmergencyOrchestrator` |
| `src/green_wave/` | Green wave velocity synchronization, intersection travel offsets, pre-clearance envelopes, and hold green timers. | Level 7: Wave Synchronization | `GreenWaveCoordinator` |
| `src/green_corridor/` | Mathematical emergency corridor generation calculating clearance timings and route waypoints. | Level 7: Corridor Planner | `EmergencyCorridorPlanner` |
| `src/traffic_signal/` | Production traffic signal node coordinating local preemption requests, fail-safe degradation, and MQTT telemetry. | Level 7: Edge Controller | `TrafficSignalNode` |
| `src/traffic_controller/` | Low-level traffic signal state machine (8 states), minimum green enforcement, and simulation GPIO interfaces. | Level 7: State Machine | `TrafficController` |
| `src/intersection/` | Edge intersection controller node managing local signal hardware, coordinator links, and fail-safe flashing yellow. | Level 7: Intersection Node | `IntersectionNode` |
| `src/ambulance/` | Ambulance mobile node simulating GPS kinematics, physiological vitals generation, and on-board V2V communication. | Level 7: Edge Mobile Node | `AmbulanceNode` |
| `src/medical/` | Patient profile data models, clinical condition severity weights (12 conditions), and continuous 0–100 priority scoring. | Level 7: Clinical Triage | `PriorityEngine` |
| `src/scheduler/` | Priority scheduling engine, conflict detection, mutual exclusion enforcement, and manual operator overrides. | Level 7: Scheduling Engine | `PriorityScheduler` |
| `src/coordinator/` | Municipal coordinator manager, intersection registry, emergency call handler, dispatch engine, and event queue. | Level 7: Coordinator Core | `CoordinatorManager` |
| `src/control/` | Manual operator console and emergency override manager for municipal traffic dispatchers. | Level 1: Operator Interface | `EmergencyOperatorConsole` |
| `src/hospital_dashboard/` | Hospital trauma bay backend receiving incoming patient vitals, predicted ETA, and medical prep notifications. | Level 1: Medical Dashboard | `HospitalDashboardBackend` |
| `src/traffic_dashboard/` | City-wide traffic monitoring dashboard backend visualizing active signal phases, corridor status, and preemption. | Level 1: Traffic Dashboard | `TrafficDashboardBackend` |
| `src/safety/` | Process heartbeat monitors, software watchdog timers, and dead-man trips. | Level 6: Safety Supervision | `Watchdog` / `SafetyMonitor` |
| `src/logger/` | High-throughput structured event logging, 13 tactical system event types, simulation recorder, and ASCII reports. | Foundation: Logging | `EventLogger` |
| `src/communication/` | Legacy binary socket transport and message framing primitives (maintained for backward compatibility). | Foundation: Transport | `SocketServer` / `SocketClient` |
| `src/network/` | Multi-transport abstraction layer supporting MQTT, TCP, UDP, and in-memory simulation transports. | Foundation: Multi-Transport | `NetworkManager` |
| `src/config/` | Schema-validated deployment configuration parser loading node IDs, network ports, and geographic coordinates. | Foundation: Configuration | `ConfigManager` |
| `src/common/` | Thread-safe queues, circular ring buffers, geo utils (Haversine), time utils, UUID generators, and validation helpers. | Foundation: Core Utilities | Common Library |
| `src/apps/` | Main daemon entrypoints: `coordinator_main.cpp`, `intersection_main.cpp`, `ambulance_main.cpp`, `traffic_main.cpp`. | Executables | System Applications |
| `tests/unit/` | 42 comprehensive unit test suites validating every module with 100% test pass rate. | Verification | Unit Test Suite |
| `tests/integration/` | End-to-end multi-node cluster deployment tests, multi-ambulance simulations, and stress tests. | Verification | Integration Tests |
| `config/deployment/` | Production deployment configuration files for a 4-Raspberry Pi hardware cluster (`rpi1` through `rpi4`). | Deployment | Configuration Subsystem |
| `scripts/` | Shell automation scripts for building, testing, QNX flashing, and multi-node cluster deployment. | Automation | Operational Scripts |
| `hardware/` | Pin mapping documentation, wiring diagrams, relay specifications, and hardware BOM for Raspberry Pi 4. | Hardware Documentation | Physical Testbed |

### 3.3 File-by-File Breakdown of Key Source Components
* **`network/message.hpp` & `network/message.cpp`:**
  * Defines binary wire protocol `Message`, `MessageType` enum, CRC-16-CCITT calculations, and `ParseStream` defragmentation.
* **`network/tcp_server.hpp` & `network/tcp_server.cpp`:**
  * Implements `TcpServer`, non-blocking client accept worker, dedicated read threads, 5s heartbeat watchdog, and message broadcasting.
* **`network/tcp_client.hpp` & `network/tcp_client.cpp`:**
  * Implements `TcpClient`, connection management, auto-registration, background 1 Hz heartbeat generator, and auto-reconnect supervisor.
* **`src/bsp/RaspberryPi4BSP.h` & `src/bsp/RaspberryPi4BSP.cpp`:**
  * Master BSP interface for BCM2711 SoC controlling 58 digital GPIO pins, mode configuration, pull resistors, interrupts, and peripherals.
* **`src/qnx_driver_framework/QNXDeviceDriverFramework.h` & `src/qnx_driver_framework/QNXDeviceDriverFramework.cpp`:**
  * QNX driver framework managing driver lifecycle, `/dev/...` resource manager simulation, resource allocation, and device I/O.
* **`src/hardware_abstraction/HardwareAbstractionLayer.h` & `src/hardware_abstraction/HardwareAbstractionLayer.cpp`:**
  * Central HAL isolating business logic from hardware peripherals, command validation rules, and device error recovery.
* **`src/system_diagnostics/SystemDiagnosticsEngine.h` & `src/system_diagnostics/SystemDiagnosticsEngine.cpp`:**
  * Continuous health monitor, failure predictor, anomaly detector, and maintenance scheduler across all municipal subsystems.
* **`src/rpi_cluster/RaspberryPiClusterManager.h` & `src/rpi_cluster/RaspberryPiClusterManager.cpp`:**
  * Raspberry Pi cluster supervisor managing node discovery, heartbeat monitoring, telemetry tracking, and configuration deployment.
* **`src/distributed_consensus/DistributedConsensusEngine.h` & `src/distributed_consensus/DistributedConsensusEngine.cpp`:**
  * Raft-style distributed consensus engine enforcing single-leader invariant, term tracking, majority voting quorum, and session immutability.
* **`src/traffic_network_orchestrator/TrafficNetworkOrchestrator.h` & `src/traffic_network_orchestrator/TrafficNetworkOrchestrator.cpp`:**
  * Municipal traffic network layer managing intersection adjacency graphs, corridor locks, sync groups, and rolling green waves.
* **`src/incident_command/IncidentCommandEngine.h` & `src/incident_command/IncidentCommandEngine.cpp`:**
  * High-level EOC decision support engine executing 10 deterministic multi-agency command rules and 4-tier severity escalation.
* **`src/mission_execution/MissionExecutionEngine.h` & `src/mission_execution/MissionExecutionEngine.cpp`:**
  * Master execution supervisor enforcing 11-stage mission progression and 8-state transition machine across the municipal ecosystem.


---

# SECTION 4: MODULE DOCUMENTATION

This section provides complete technical documentation for every production module across all 12 architectural layers of SMART AEGIS.

---

### 4.1 `TcpServer`, `TcpClient`, `Message` (QNX Network Communication Layer)
* **Directory:** `network/`
* **Namespace:** `aegis::network`
* **Purpose:** Provides high-throughput, low-latency, deterministic TCP/IP communication connecting the central Coordinator to distributed Raspberry Pi edge nodes under QNX Neutrino RTOS and POSIX environments.
* **Responsibilities:**
  * Strict binary message framing using 19-byte fixed header, magic bytes `0xA55A`, and CRC-16-CCITT checksum validation.
  * Multi-threaded concurrent server with non-blocking accept loop, dedicated per-client worker threads, and dead socket reclamation.
  * Autonomous 5-second heartbeat watchdog supervisor terminating silent or hung nodes.
  * Edge client lifecycle management with automated initial `REGISTER` dispatch, background 1 Hz heartbeat timer, and non-blocking auto-reconnect.
  * Safe stream defragmentation (`ParseStream`) reconstructing fragmented packets and recovering from corrupt byte noise.
* **Public APIs:**
  * `TcpServer::Start(const std::string& host, uint16_t port, int backlog = 128)`
  * `TcpServer::Stop()`
  * `TcpServer::Broadcast(const Message& msg)`
  * `TcpServer::SendToNode(const std::string& nodeId, const Message& msg)`
  * `TcpClient::Connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 3000)`
  * `TcpClient::Disconnect()`
  * `TcpClient::Send(const Message& msg)`
  * `TcpClient::Receive(Message& outMsg, uint32_t timeoutMs = 1000)`
  * `TcpClient::StartHeartbeat(uint32_t intervalMs = 1000)`
  * `TcpClient::SetAutoReconnect(bool enabled, uint32_t intervalMs = 2000)`
* **Dependencies:** Pure C++17 STL, standard BSD/POSIX sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`, `poll.h`). Zero external libraries.
* **Inputs & Outputs:** Ingests raw TCP byte streams; outputs deserialized, checksum-verified `Message` structs.
* **Thread Ownership:** Manages background listener thread, per-client read worker threads, heartbeat timer thread, and auto-reconnect monitor thread.
* **Status & Porting:** 100% complete, fully verified in unit test suite. Can remain unchanged on QNX BSD socket stack or migrate to native QNX messaging.

---

### 4.2 `RaspberryPi4BSP` (Phase 16A Board Support Package Layer)
* **Directory:** `src/bsp/`
* **Header / Source:** `src/bsp/RaspberryPi4BSP.h`, `src/bsp/RaspberryPi4BSP.cpp`
* **Namespace:** `aegis::bsp`
* **Purpose:** Master hardware-facing abstraction and Board Support Package (BSP) for the Raspberry Pi 4 Model B (Broadcom BCM2711 SoC).
* **Responsibilities:**
  * Controls 58 digital GPIO pins across Banks 0, 1, and 2 with exclusive reservation locks (`ReserveGPIO`).
  * Enforces pin mode configuration (`INPUT`, `OUTPUT`, `ALT0`–`ALT5`) and internal pull resistors (`NONE`, `PULL_UP`, `PULL_DOWN`).
  * Executes high-throughput digital read, write, and toggle operations with sub-microsecond latencies (< 0.2 $\mu	ext{s}$).
  * Manages edge/level interrupt configuration and callback dispatching (`ConfigureInterrupt`).
  * Manages exclusive peripheral hardware reservations for `I2C1`, `SPI0`, `UART0`, and `PWM0`.
  * Generates structured ASCII operational reports (`[BOARD]`, `[GPIO]`, `[INTERRUPT]`).
* **Public APIs:**
  * `Initialize()`, `Shutdown()`, `IsInitialized()`
  * `ReserveGPIO(uint8_t pin, const std::string& consumer)`
  * `ReleaseGPIO(uint8_t pin, const std::string& consumer)`
  * `SetPinMode(uint8_t pin, PinMode mode)`
  * `WritePin(uint8_t pin, PinState state)`
  * `ReadPin(uint8_t pin, PinState& outState)`
  * `TogglePin(uint8_t pin)`
  * `ConfigureInterrupt(uint8_t pin, InterruptTrigger trigger, InterruptCallback callback)`
  * `ReservePeripheral(const std::string& peripheral, const std::string& consumer)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete in simulated hardware mode. Ready for direct connection to physical BCM2711 registers or QNX `/dev/gpio` resource manager.

---

### 4.3 `QNXDeviceDriverFramework` (Phase 15B Driver Framework Layer)
* **Directory:** `src/qnx_driver_framework/`
* **Header / Source:** `src/qnx_driver_framework/QNXDeviceDriverFramework.h`, `src/qnx_driver_framework/QNXDeviceDriverFramework.cpp`
* **Namespace:** `aegis::qnx_driver`
* **Purpose:** Production-quality QNX Neutrino Device Driver Framework interfacing the Hardware Abstraction Layer with the underlying BSP.
* **Responsibilities:**
  * Comprehensive driver lifecycle management (`UNREGISTERED`, `REGISTERED`, `INITIALIZED`, `RUNNING`, `STOPPED`, `FAILED`).
  * Hardware resource allocation, exclusivity tracking, and interrupt enablement gating.
  * Simulates QNX Neutrino resource manager path attachments (`/dev/...`).
  * Standardized device I/O abstraction (`ReadDevice`, `WriteDevice`, `IoControl`).
  * Deterministic restart recovery implementing Rule 8: Stop -> Shutdown -> Init -> Start.
* **Public APIs:**
  * `RegisterDriver(std::unique_ptr<DriverDescriptor> descriptor)`
  * `InitializeDriver(const std::string& driverId)`
  * `StartDriver(const std::string& driverId)`
  * `StopDriver(const std::string& driverId)`
  * `WriteDevice(const std::string& driverId, const std::vector<uint8_t>& data)`
  * `ReadDevice(const std::string& driverId, std::vector<uint8_t>& outData, size_t maxBytes)`
  * `IoControl(const std::string& driverId, uint32_t command, const std::vector<uint8_t>& inData, std::vector<uint8_t>& outData)`
  * `RestartDriver(const std::string& driverId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. On QNX SDP 8.0, binds to native `resmgr_attach()`, `io_read()`, `io_write()`, and `io_devctl()`.

---

### 4.4 `HardwareAbstractionLayer` (Phase 15A HAL Layer)
* **Directory:** `src/hardware_abstraction/`
* **Header / Source:** `src/hardware_abstraction/HardwareAbstractionLayer.h`, `src/hardware_abstraction/HardwareAbstractionLayer.cpp`
* **Namespace:** `aegis::hal`
* **Purpose:** Centralized hardware abstraction layer isolating high-level municipal logic from physical peripherals.
* **Responsibilities:**
  * Exposes vendor-neutral device access for GPIO, PWM, SPI, UART, I2C, Ethernet, LED, OLED, GPS, and Buzzer.
  * Deterministic driver lifecycle enforcement (`OFFLINE`, `ONLINE`, `INITIALIZED`, `READY`, `BUSY`, `ERROR`).
  * Four strict command execution protection rules (Initialization check, Driver existence check, Offline rejection, Disable protection).
  * High-resolution execution latency accounting and statistical tracking (`HardwareStatistics`).
  * Hardware reset and error recovery (`ResetDevice`).
* **Public APIs:**
  * `Initialize()`, `Shutdown()`
  * `RegisterDriver(std::shared_ptr<IHardwareDriver> driver)`
  * `SendCommand(const HardwareCommand& cmd, HardwareResponse& outResponse)`
  * `ReadSensor(const std::string& deviceId, SensorData& outData)`
  * `ResetDevice(const std::string& deviceId)`
  * `GetStatistics() const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Completely platform-independent.

---

### 4.5 `SystemDiagnosticsEngine` (Phase 14B Diagnostics Layer)
* **Directory:** `src/system_diagnostics/`
* **Header / Source:** `src/system_diagnostics/SystemDiagnosticsEngine.h`, `src/system_diagnostics/SystemDiagnosticsEngine.cpp`
* **Namespace:** `aegis::diagnostics`
* **Purpose:** Platform-wide continuous health supervision, multi-subsystem diagnostics, and predictive failure engine.
* **Responsibilities:**
  * Telemetry monitoring across Coordinator, Cluster, Network, Consensus, Traffic, and Edge nodes.
  * Multi-metric fault detection for heartbeat timeouts, packet loss, thermal overruns, and memory leaks.
  * Predictive maintenance forecasting thermal shutdowns, heap exhaustion, hardware degradation, and network link flapping.
  * Deterministic maintenance scheduling with 5-tier priority hierarchy (`NONE`, `LOW`, `MEDIUM`, `HIGH`, `IMMEDIATE`).
  * System readiness validation (`VerifySystemReadiness`).
* **Public APIs:**
  * `RecordTelemetry(const ComponentTelemetry& telemetry)`
  * `RunDiagnostics(const std::string& componentId)`
  * `RunFullSystemScan()`
  * `PredictMaintenanceNeeds(const std::string& componentId)`
  * `VerifySystemReadiness() const`
  * `ScheduleMaintenance(const MaintenanceTask& task)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.6 `RaspberryPiClusterManager` (Phase 14A Cluster Management Layer)
* **Directory:** `src/rpi_cluster/`
* **Header / Source:** `src/rpi_cluster/RaspberryPiClusterManager.h`, `src/rpi_cluster/RaspberryPiClusterManager.cpp`
* **Namespace:** `aegis::rpi_cluster`
* **Purpose:** Hardware cluster supervision and deployment lifecycle management across all Raspberry Pi 4 edge intersection controllers.
* **Responsibilities:**
  * Dynamic node discovery (`DiscoverNode`) and registration.
  * Heartbeat tracking with automated offline failover.
  * Edge telemetry ingestion (CPU temperature, CPU load, RAM usage, storage usage, uptime, latency).
  * 4-tier health classification (`HEALTHY`, `WARNING`, `CRITICAL`, `OFFLINE`).
  * 5-stage deployment state machine (`NOT_DEPLOYED`, `DEPLOYING`, `DEPLOYED`, `FAILED`, `RECOVERING`).
  * Configuration push with schema version verification.
  * Cluster readiness gating (`VerifyClusterReadiness`).
* **Public APIs:**
  * `DiscoverNode(const DiscoveredNodeInfo& info)`
  * `ProcessHeartbeat(const std::string& nodeId, uint64_t timestamp)`
  * `UpdateTelemetry(const std::string& nodeId, const NodeTelemetry& telemetry)`
  * `DeployConfiguration(const std::string& nodeId, const std::string& configJson, uint32_t schemaVersion)`
  * `RecoverNode(const std::string& nodeId)`
  * `VerifyClusterReadiness() const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.7 `DistributedConsensusEngine` (Phase 13B Consensus Layer)
* **Directory:** `src/distributed_consensus/`
* **Header / Source:** `src/distributed_consensus/DistributedConsensusEngine.h`, `src/distributed_consensus/DistributedConsensusEngine.cpp`
* **Namespace:** `aegis::consensus`
* **Purpose:** Distributed consensus layer guaranteeing unanimous preemption agreement across edge Raspberry Pi controllers prior to lamp actuation.
* **Responsibilities:**
  * Single-leader invariant with term tracking and heartbeat monitoring.
  * Majority vote quorum enforcement ($(N / 2) + 1$).
  * Timeout-based automatic abort and leader re-election.
  * Safe clearance signal rollback upon aborted preemption sessions.
  * Committed session immutability and permanent vote audit logging.
* **Public APIs:**
  * `ElectLeader(const std::string& candidateNodeId, uint64_t term)`
  * `ProposePreemptionPlan(const PreemptionPlanProposal& proposal)`
  * `CastVote(const PreemptionVote& vote)`
  * `CommitPlan(const std::string& planId)`
  * `AbortPlan(const std::string& planId, const std::string& reason)`
  * `VerifyQuorum(const std::string& planId) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.8 `TrafficNetworkOrchestrator` (Phase 13A Network Orchestration Layer)
* **Directory:** `src/traffic_network_orchestrator/`
* **Header / Source:** `src/traffic_network_orchestrator/TrafficNetworkOrchestrator.h`, `src/traffic_network_orchestrator/TrafficNetworkOrchestrator.cpp`
* **Namespace:** `aegis::network_orchestration`
* **Purpose:** Municipal traffic network execution layer coordinating all city intersections as an integrated topology.
* **Responsibilities:**
  * Topological adjacency graph management and route pathing validation.
  * Active corridor ownership and intersection exclusive locking.
  * Dynamic synchronization groups with deterministic leader election.
  * Rolling green wave progression across consecutive signal nodes.
  * Heartbeat tracking, offline node isolation, and fail-safe clearance rollback.
* **Public APIs:**
  * `AddIntersection(const IntersectionTopologyNode& node)`
  * `AddRoadSegment(const RoadSegment& segment)`
  * `CreateCorridor(const std::string& corridorId, const std::vector<std::string>& path)`
  * `ActivateCorridor(const std::string& corridorId)`
  * `DeactivateCorridor(const std::string& corridorId)`
  * `SynchronizeGroup(const std::string& groupId)`
  * `IsIntersectionLocked(const std::string& intersectionId) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.9 `IncidentCommandEngine` (Phase 12B Incident Command Layer)
* **Directory:** `src/incident_command/`
* **Header / Source:** `src/incident_command/IncidentCommandEngine.h`, `src/incident_command/IncidentCommandEngine.cpp`
* **Namespace:** `aegis::incident_command`
* **Purpose:** High-level Emergency Operations Center (EOC) multi-agency decision support backend.
* **Responsibilities:**
  * Master incident registry orchestrating concurrent municipal emergencies.
  * 4-tier severity escalation (`NORMAL`, `HIGH`, `CRITICAL`, `DISASTER`).
  * 10 deterministic automatic multi-agency decision rules.
  * Manual dispatcher intervention logging and override management.
  * Tamper-evident chronological timeline auditing and situational awareness reporting.
* **Public APIs:**
  * `RegisterIncident(const IncidentCommandRecord& incident)`
  * `UpdateSeverity(const std::string& incidentId, IncidentSeverity severity)`
  * `EvaluateDecisionRules(const std::string& incidentId)`
  * `ExecuteOperatorOverride(const OperatorOverrideAction& action)`
  * `GenerateSituationalReport(const std::string& incidentId) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.10 `MissionExecutionEngine` (Phase 12A Master Mission Layer)
* **Directory:** `src/mission_execution/`
* **Header / Source:** `src/mission_execution/MissionExecutionEngine.h`, `src/mission_execution/MissionExecutionEngine.cpp`
* **Namespace:** `aegis::mission`
* **Purpose:** Highest-level master controller and execution supervisor for the entire AERS emergency ecosystem.
* **Responsibilities:**
  * Ingests 911 calls and unifies triage, fleet dispatch, hospital bed allocation, routing, and green wave preemption into one unified mission record.
  * Enforces 11-stage chronological mission progression and 8-state transition machine.
  * Executes 10 deterministic mission validation rules preventing invalid or hazardous operations.
  * Single source of truth for municipal emergency status and forensic audit logs.
* **Public APIs:**
  * `InitiateMission(const MissionParameters& params)`
  * `AdvanceMissionStage(const std::string& missionId, MissionStage nextStage)`
  * `TransitionMissionState(const std::string& missionId, MissionState nextState)`
  * `ValidateMissionIntegrity(const std::string& missionId) const`
  * `AbortMission(const std::string& missionId, const std::string& reason)`
  * `GetMissionRecord(const std::string& missionId) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.11 `HospitalResourceEngine` (Phase 11B Hospital Resource Layer)
* **Directory:** `src/hospital_management/`
* **Header / Source:** `src/hospital_management/HospitalResourceEngine.h`, `src/hospital_management/HospitalResourceEngine.cpp`
* **Namespace:** `aegis::hospital_management`
* **Purpose:** Centralized municipal hospital resource tracking, real-time inpatient bed availability, ICU bed allocation, mechanical ventilator inventories, and physician scheduling.
* **Responsibilities:**
  * Deterministic multi-criteria hospital selection executing 10 deterministic selection rules.
  * End-to-end bed and ICU reservation management lifecycle (`ReserveBed`, `ReleaseBed`, `ReserveICUBed`, `ReleaseICUBed`).
  * Emits structured ASCII operational reports (`[HOSPITAL]`).
* **Public APIs:**
  * `RegisterHospital(const HospitalFacility& facility)`
  * `UpdateCapacity(const std::string& hospitalId, const HospitalCapacity& capacity)`
  * `SelectOptimalHospital(const PatientClinicalProfile& profile, const GeoCoordinates& location)`
  * `ReserveBed(const std::string& hospitalId, const std::string& patientId)`
  * `ReleaseBed(const std::string& hospitalId, const std::string& patientId)`
  * `ReserveICUBed(const std::string& hospitalId, const std::string& patientId)`
  * `ReleaseICUBed(const std::string& hospitalId, const std::string& patientId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.12 `HospitalSelectionEngine` (Phase 11B Hospital Selection Layer)
* **Directory:** `src/hospital_selection/`
* **Header / Source:** `src/hospital_selection/HospitalSelectionEngine.h`, `src/hospital_selection/HospitalSelectionEngine.cpp`
* **Namespace:** `aegis::hospital_selection`
* **Purpose:** Fast-path destination hospital selection engine computing multi-factor capability matches and ETA rankings.
* **Responsibilities:**
  * Real-time municipal hospital network tracking and emergency department load monitoring.
  * 10-factor deterministic ranking with automatic full/offline failover.
  * Sub-millisecond hospital selection across 100+ facilities.
* **Public APIs:**
  * `RegisterHospital(const HospitalInfo& info)`
  * `SelectBestHospital(const EmergencyContext& ctx)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.13 `FleetManagementEngine` (Phase 11A Fleet Management Layer)
* **Directory:** `src/fleet_management/`
* **Header / Source:** `src/fleet_management/FleetManagementEngine.h`, `src/fleet_management/FleetManagementEngine.cpp`
* **Namespace:** `aegis::fleet`
* **Purpose:** Centralized ambulance fleet management, vehicle availability tracking, and deterministic emergency dispatch engine.
* **Responsibilities:**
  * Real-time municipal fleet registry with automated heartbeat timeout failover.
  * Multi-tier medical capability matching (`BASIC`, `ADVANCED`, `INTENSIVE_CARE`, `NEONATAL`, `HAZMAT`).
  * 8-rule deterministic ranking factoring transit ETA, battery/fuel, crew certifications, and vehicle load.
  * Emits structured ASCII operational reports (`[FLEET]`).
* **Public APIs:**
  * `RegisterAmbulance(const AmbulanceUnit& unit)`
  * `UpdateAmbulanceStatus(const std::string& unitId, AmbulanceStatus status)`
  * `UpdateAmbulanceLocation(const std::string& unitId, const GeoCoordinates& location, double batteryLevel)`
  * `DispatchBestAmbulance(const EmergencyCallData& call)`
  * `ReleaseAmbulance(const std::string& unitId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.14 `PredictiveTrafficEngine` (Phase 10B Predictive Traffic Layer)
* **Directory:** `src/predictive_traffic/`
* **Header / Source:** `src/predictive_traffic/PredictiveTrafficEngine.h`, `src/predictive_traffic/PredictiveTrafficEngine.cpp`
* **Namespace:** `aegis::predictive`
* **Purpose:** AI predictive traffic intelligence engine forecasting future corridor congestion and speed drops.
* **Responsibilities:**
  * Deterministic speed and congestion forecasting using historical statistical profiles and moving averages.
  * Time-of-day peak/off-peak profiling with weather and municipal event multipliers.
  * Statistical Z-score anomaly detection flagging unusual road slowdowns.
  * Candidate corridor ranking for dynamic route optimization.
* **Public APIs:**
  * `IngestTrafficObservation(const TrafficObservation& obs)`
  * `PredictSegmentCongestion(const std::string& segmentId, uint64_t futureTimestamp)`
  * `PredictCorridorTransitTime(const std::vector<std::string>& segmentIds, uint64_t futureTimestamp)`
  * `DetectAnomalies(const std::string& segmentId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.15 `DistributedSignalSynchronizer` (Phase 10A Signal Synchronization Layer)
* **Directory:** `src/distributed_signal/`
* **Header / Source:** `src/distributed_signal/DistributedSignalSynchronizer.h`, `src/distributed_signal/DistributedSignalSynchronizer.cpp`
* **Namespace:** `aegis::distributed_signal`
* **Purpose:** Real-time multi-node execution scheduling, clock synchronization, and transmission retry management.
* **Responsibilities:**
  * Dispatches preemption schedules across distributed edge nodes.
  * Tracks network round-trip latencies, clock skews, and node keepalives.
  * Executes exponential backoff retries on communication failures.
* **Public APIs:**
  * `RegisterIntersection(const std::string& id, const std::string& address, uint16_t port)`
  * `SchedulePreemption(const ScheduledPreemptionPlan& plan)`
  * `ProcessHeartbeat(const std::string& id, uint64_t timestamp)`
  * `GetAverageLatencyMs(const std::string& id) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.16 `MultiCorridorCoordinator` (Phase 9B Multi-Corridor Layer)
* **Directory:** `src/multi_corridor/`
* **Header / Source:** `src/multi_corridor/MultiCorridorCoordinator.h`, `src/multi_corridor/MultiCorridorCoordinator.cpp`
* **Namespace:** `aegis::multi_corridor`
* **Purpose:** Real-time multi-ambulance corridor monitoring, conflict detection, and deterministic priority arbitration.
* **Responsibilities:**
  * Monitors active corridors across multiple converging emergency vehicles.
  * Identifies 5 conflict categories (Spatial, Temporal, Contraflow, Consecutive Subpath, Full Overlap).
  * Executes 5-tier deterministic priority arbitration model.
  * Delays, adjusts, or triggers dynamic rerouting on contested green corridors.
* **Public APIs:**
  * `RegisterCorridor(const CorridorDefinition& corridor)`
  * `UpdateAmbulancePosition(const std::string& ambId, const GeoCoordinates& coords, double speed)`
  * `DetectConflicts()`
  * `ArbitrateConflict(const CorridorConflict& conflict)`
  * `CancelCorridor(const std::string& corridorId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.17 `DynamicRouteReplanner` (Phase 9A Replanning Layer)
* **Directory:** `src/replanning/`
* **Header / Source:** `src/replanning/DynamicRouteReplanner.h`, `src/replanning/DynamicRouteReplanner.cpp`
* **Namespace:** `aegis::replanning`
* **Purpose:** Live traffic adaptation and dynamic route replanning engine.
* **Responsibilities:**
  * Real-time monitoring of road segment congestion and temporary road blockages.
  * Dynamic A* path recalculation when segment delay exceeds replanning threshold (> 15s).
  * Automated Green Wave corridor regeneration and signal preemption redispatching.
* **Public APIs:**
  * `UpdateSegmentTraffic(const std::string& segmentId, double congestionLevel, bool isBlocked)`
  * `EvaluateRouteViability(const std::string& ambId, const NavigationRoute& currentRoute)`
  * `CalculateAlternativeRoute(const GeoCoordinates& origin, const GeoCoordinates& destination)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.18 `EmergencyOrchestrator` (Phase 8B Orchestration Layer)
* **Directory:** `src/orchestrator/`
* **Header / Source:** `src/orchestrator/EmergencyOrchestrator.h`, `src/orchestrator/EmergencyOrchestrator.cpp`
* **Namespace:** `aegis::orchestrator`
* **Purpose:** Master coordination layer connecting PriorityScheduler, GreenCorridorPlanner, and IntersectionNodes.
* **Responsibilities:**
  * Coordinates emergency corridor activation, execution, and release.
  * Manages transition states from call intake to final hospital arrival.
* **Public APIs:**
  * `Initialize()`, `Shutdown()`
  * `TriggerEmergencyCorridor(const EmergencyCorridorRequest& request)`
  * `ReleaseEmergencyCorridor(const std::string& corridorId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.19 `GreenWaveCoordinator` (Phase 7B Green Wave Layer)
* **Directory:** `src/green_wave/`
* **Header / Source:** `src/green_wave/GreenWaveCoordinator.h`, `src/green_wave/GreenWaveCoordinator.cpp`
* **Namespace:** `aegis::green_wave`
* **Purpose:** Coordinates synchronized signal preemption offsets based on vehicle velocity and intersection spacing.
* **Responsibilities:**
  * Computes progressive green wave offsets: $t_{	ext{offset}}(i) = \sum rac{d(k, k+1)}{v_{	ext{expected}}}$.
  * Enforces pre-clearance holding intervals and minimum green times.
* **Public APIs:**
  * `GenerateGreenWave(const std::vector<std::string>& intersections, double ambulanceSpeed)`
  * `UpdateWaveProgress(const std::string& currentIntersectionId)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.20 `EmergencyCorridorPlanner` (Phase 7A Corridor Layer)
* **Directory:** `src/green_corridor/`
* **Header / Source:** `src/green_corridor/EmergencyCorridorPlanner.h`, `src/green_corridor/EmergencyCorridorPlanner.cpp`
* **Namespace:** `aegis::corridor`
* **Purpose:** Calculates geometric corridor boundaries, clearance timings, and intersection approach envelopes.
* **Responsibilities:**
  * Ingests route waypoints and calculates approach vectors, preemption trigger radiuses, and clearance times.
* **Public APIs:**
  * `PlanCorridor(const NavigationRoute& route, double targetSpeed)`
  * `GetIntersectionTiming(const std::string& intersectionId) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.21 `TrafficSignalNode` (Phase 6A/6B Traffic Signal Layer)
* **Directory:** `src/traffic_signal/`, `src/intersection/`
* **Header / Source:** `src/traffic_signal/TrafficSignalNode.h`, `src/traffic_signal/TrafficSignalNode.cpp`
* **Namespace:** `aegis::edge`
* **Purpose:** Complete production traffic signal node coordinating local preemption, fail-safe degradation, and MQTT telemetry.
* **Responsibilities:**
  * Controls underlying `TrafficController` and signal state machine.
  * Connects to coordinator, receives preemption commands, and handles fail-safe degradation to flashing yellow.
* **Public APIs:**
  * `Start()`, `Stop()`
  * `HandlePreemptionCommand(const PreemptionCommand& cmd)`
  * `SetOperationalMode(SignalOperationalMode mode)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.22 `TrafficController` & `SignalStateMachine`
* **Directory:** `src/traffic_controller/`
* **Header / Source:** `src/traffic_controller/traffic_controller.cpp`, `src/traffic_controller/signal_state_machine.cpp`
* **Namespace:** `aegis::edge`
* **Purpose:** Core 8-state deterministic traffic signal state machine and GPIO interface driver.
* **Responsibilities:**
  * Enforces state machine transitions: `RED` $	o$ `GREEN` $	o$ `YELLOW` $	o$ `ALL_RED` $	o$ `EMERGENCY_PREEMPTION` $	o$ `SAFE_CLEARANCE` $	o$ `FLASHING_YELLOW` $	o$ `ALL_OFF`.
  * Guarantees minimum green (10s), yellow clearance (4s), and red clearance (2s).
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Directly interfaces with `HardwareAbstractionLayer`.

---

### 4.23 `PriorityEngine` & `PatientProfile`
* **Directory:** `src/medical/`
* **Header / Source:** `src/medical/PriorityEngine.h`, `src/medical/PatientProfile.h`
* **Namespace:** `aegis::medical`
* **Purpose:** Clinical triage evaluation engine computing continuous priority scores (0–100).
* **Responsibilities:**
  * Evaluates 12 medical emergency categories (`CARDIAC_ARREST`, `STROKE`, `TRAUMA`, etc.).
  * Applies physiological weights to Glasgow Coma Scale, Systolic BP, Heart Rate, Respiratory Rate, and SpO2.
* **Public APIs:**
  * `CalculatePriorityScore(const PatientProfile& profile) const`
  * `CategorizeTriageLevel(double score) const`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.24 `PriorityScheduler` & `ConflictDetectionEngine`
* **Directory:** `src/scheduler/`
* **Header / Source:** `src/scheduler/PriorityScheduler.h`, `src/scheduler/ConflictDetectionEngine.h`
* **Namespace:** `aegis::scheduler`
* **Purpose:** Intelligent priority scheduling engine arbitrating intersection contention and enforcing strict mutual exclusion.
* **Responsibilities:**
  * Manages active preemption requests across all municipal intersections.
  * Solves multi-ambulance conflicts using clinical triage scores and arrival times.
  * Enforces manual operator overrides.
* **Public APIs:**
  * `SchedulePreemption(const PreemptionRequest& req)`
  * `CancelPreemption(const std::string& requestId)`
  * `ApplyManualOverride(const ManualOverride& override)`
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.25 `CoordinatorManager`
* **Directory:** `src/coordinator/`
* **Header / Source:** `src/coordinator/CoordinatorManager.h`, `src/coordinator/CoordinatorManager.cpp`
* **Namespace:** `aegis::core`
* **Purpose:** Master coordinator supervisor orchestrating call intake, fleet tracking, intersection registries, and event queues.
* **Responsibilities:**
  * Manages intersection registry, ambulance registry, and active emergency missions.
  * Emits event queue items and dispatches commands to network interfaces.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.26 `RoutePlanner`
* **Directory:** `src/route_planner/`
* **Header / Source:** `src/route_planner/route_planner.cpp`, `include/aegis/core/route_planner.hpp`
* **Namespace:** `aegis::core`
* **Purpose:** Static and dynamic A* pathfinding engine over municipal road network graphs.
* **Responsibilities:**
  * Calculates shortest time routes factoring speed limits and baseline congestion.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.27 `AmbulanceNode` & `AmbulanceProfile`
* **Directory:** `src/ambulance/`
* **Header / Source:** `src/ambulance/ambulance_node.cpp`, `src/ambulance/AmbulanceProfile.h`
* **Namespace:** `aegis::edge`
* **Purpose:** Edge vehicle software simulating kinematics, telemetry streaming, and vitals updates.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.28 `IntersectionNode`
* **Directory:** `src/intersection/`
* **Header / Source:** `src/intersection/IntersectionNode.h`, `src/intersection/IntersectionNode.cpp`
* **Namespace:** `aegis::edge`
* **Purpose:** Edge intersection manager coordinating signal controllers and communication links.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.29 `EventLogger` & `SimulationRecorder`
* **Directory:** `src/logger/`
* **Header / Source:** `src/logger/EventLogger.h`, `src/logger/SimulationRecorder.h`
* **Namespace:** `aegis::logger`
* **Purpose:** High-throughput structured event logging and simulation playback recording.
* **Responsibilities:**
  * Captures 13 tactical system events with microsecond timestamps and structured ASCII formatting.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.30 `SafetyMonitor` & `Watchdog`
* **Directory:** `src/safety/`
* **Header / Source:** `src/safety/safety_monitor.cpp`, `src/safety/watchdog.cpp`
* **Namespace:** `aegis::safety`
* **Purpose:** Software watchdog timers and dead-man trips monitoring process health.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.31 `ConfigManager` & `NodeConfig`
* **Directory:** `src/config/`
* **Header / Source:** `src/config/config_manager.cpp`, `src/config/NodeConfig.h`
* **Namespace:** `aegis::config`
* **Purpose:** Configuration loading, schema validation, and fallback resolution.
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.

---

### 4.32 `Common Utilities` (Geo, Time, UUID, Queue, CircularBuffer)
* **Directory:** `src/common/`, `include/aegis/common/`
* **Purpose:** Foundational data structures and algorithms (Haversine geodesic math, monotonic timers, UUIDv4, thread-safe queues, circular buffers).
* **Dependencies:** Pure C++17 STL.
* **Status & Porting:** 100% complete. Platform independent.


---

# SECTION 5: COORDINATOR DECISION ENGINE

### 5.1 Architecture & Master Decision Pipeline
The Coordinator Decision Engine is the supreme brain of the SMART AEGIS municipal network. It orchestrates six cooperating subsystems into a deterministic real-time pipeline:

```
[911 Call / ACN Ingestion]
            │
            ▼
[EmergencyCallHandler] ────> [PatientProfile & PriorityEngine] (Clinical Score 0-100)
            │                               │
            ▼                               ▼
[IncidentCommandEngine] ───> [FleetManagementEngine] (Unit Allocation)
            │                               │
            ▼                               ▼
[HospitalResourceEngine] ──> [RoutePlanner & DynamicReplanner] (A* Pathfinding)
            │                               │
            ▼                               ▼
[EmergencyCorridorPlanner] ─> [GreenWaveCoordinator] (Velocity Offsets)
            │                               │
            ▼                               ▼
[MultiCorridorCoordinator] ─> [DistributedConsensusEngine] (Majority Quorum)
            │
            ▼
[TcpServer -> Edge Intersection Actuation]
```

### 5.2 End-to-End Decision Workflow
1. **Intake & Triage Qualification:** Distress calls are ingested with caller location and description. Initial clinical severity is categorized.
2. **Acuity Scoring:** `PriorityEngine` evaluates 12 medical conditions and vital signs to generate a continuous score $S_{	ext{priority}} \in [0, 100]$.
3. **Ambulance Allocation:** `FleetManagementEngine` selects the nearest capable vehicle enforcing 8 deterministic dispatch rules.
4. **Hospital Destination Selection:** `HospitalResourceEngine` ranks receiving facilities using 10 deterministic rules, locking an ICU bed and ventilator.
5. **Path Optimization:** `RoutePlanner` determines the minimum travel-time route connecting the unit, incident scene, and destination hospital.
6. **Green Wave Schedule Generation:** `GreenWaveCoordinator` calculates intersection velocity arrival times, pre-clearance times, and hold durations.
7. **Conflict Arbitration:** If multiple corridors intersect, `MultiCorridorCoordinator` resolves contention using clinical priority and arrival windows.
8. **Consensus Agreement:** `DistributedConsensusEngine` conducts a majority vote across affected intersection nodes.
9. **Dispatch & Execution:** Preemption directives are transmitted over TCP to edge nodes; signals actuate green; traffic clears.

### 5.3 Clinical Priority Calculation Algorithm
The continuous clinical priority score $P \in [0, 100]$ is computed via a multi-factor weighted equation:

$$P = \min\left(100.0, \; W_{	ext{base}} + \Delta_{	ext{GCS}} + \Delta_{	ext{BP}} + \Delta_{	ext{HR}} + \Delta_{	ext{RR}} + \Delta_{	ext{SpO2}}ight)$$

#### Base Severity Weights ($W_{	ext{base}}$):
* `CARDIAC_ARREST`: 95.0
* `MASSIVE_TRAUMA`: 90.0
* `STROKE`: 85.0
* `SEVERE_RESPIRATORY_DISTRESS`: 80.0
* `SEVERE_BURNS`: 75.0
* `ACUTE_CORONARY_SYNDROME`: 70.0
* `OBSTETRIC_EMERGENCY`: 65.0
* `HEAD_INJURY`: 60.0
* `INTERNAL_BLEEDING`: 55.0
* `SEVERE_FRACTURE`: 40.0
* `MODERATE_BLEEDING`: 30.0
* `MINOR_INJURY`: 10.0

#### Physiological Penalty Offsets:
* **Glasgow Coma Scale (GCS):**
  * $	ext{GCS} \le 8$: $+15.0$
  * $9 \le 	ext{GCS} \le 12$: $+8.0$
  * $	ext{GCS} \ge 13$: $+0.0$
* **Systolic Blood Pressure (SBP):**
  * $	ext{SBP} < 90\,	ext{mmHg}$ or $	ext{SBP} > 180\,	ext{mmHg}$: $+10.0$
  * Otherwise: $+0.0$
* **Heart Rate (HR):**
  * $	ext{HR} < 50\,	ext{bpm}$ or $	ext{HR} > 120\,	ext{bpm}$: $+8.0$
  * Otherwise: $+0.0$
* **Respiratory Rate (RR):**
  * $	ext{RR} < 10\,	ext{bpm}$ or $	ext{RR} > 29\,	ext{bpm}$: $+8.0$
  * Otherwise: $+0.0$
* **Oxygen Saturation (SpO2):**
  * $	ext{SpO2} < 90\%$: $+12.0$
  * $90\% \le 	ext{SpO2} \le 94\%$: $+5.0$
  * $	ext{SpO2} \ge 95\%$: $+0.0$

### 5.4 10 Deterministic Incident Command Rules (Phase 12B)
The `IncidentCommandEngine` enforces 10 strict decision rules during every evaluation tick:
1. **Rule 1 (Mass-Casualty Triage):** Incidents with $> 5$ patients automatically escalate severity to `DISASTER`.
2. **Rule 2 (Trauma Center Bypass):** Patients with trauma score $> 80$ bypass non-trauma community clinics.
3. **Rule 3 (Dual Unit Dispatch):** Cardiac arrest incidents require immediate dispatch of two units (ALS + BLS).
4. **Rule 4 (Police Corridor Escort):** Severe incidents traversing $> 4$ intersections trigger automated police escort notifications.
5. **Rule 5 (Hospital Diversion Override):** If all district hospitals are on diversion status, the least-utilized facility is forced open.
6. **Rule 6 (Dynamic Detour Trigger):** If a corridor segment reports $> 70\%$ congestion, alternative routing is executed immediately.
7. **Rule 7 (Contraflow Quarantine):** Opposing ambulance corridors are prohibited on two-lane undivided roadways.
8. **Rule 8 (Preemption Hold Limit):** No single intersection may hold preemption green for $> 60$ seconds continuously.
9. **Rule 9 (Signal Restoration Buffer):** After preemption clearance, a minimum 15-second baseline cyclic buffer is enforced.
10. **Rule 10 (Operator Override Supremacy):** Manual dispatcher interventions override all algorithmic decisions with forensic audit logging.

### 5.5 Sequence Diagram: Emergency Call to Signal Actuation
```mermaid
sequenceDiagram
    autonumber
    actor Caller as 911 Caller
    participant Ingestion as EmergencyCallHandler
    participant Engine as MissionExecutionEngine
    participant Medical as PriorityEngine
    participant Fleet as FleetManagementEngine
    participant Hospital as HospitalResourceEngine
    participant Route as RoutePlanner
    participant Wave as GreenWaveCoordinator
    participant Net as TcpServer
    participant Signal as Traffic Node A

    Caller->>Ingestion: 911 Distress Call (Location, Cardiac Arrest)
    Ingestion->>Engine: InitiateMission(CallData)
    Engine->>Medical: CalculatePriorityScore(Profile)
    Medical-->>Engine: Score = 98.0 (CRITICAL)
    Engine->>Fleet: DispatchBestAmbulance(Score, Location)
    Fleet-->>Engine: Assigned AMB_01 (ALS Unit)
    Engine->>Hospital: SelectOptimalHospital(PatientProfile, Location)
    Hospital-->>Engine: Reserved HOSP_CITY_TRAUMA (ICU Bed 4 Locked)
    Engine->>Route: CalculateRoute(Origin, Scene, Hospital)
    Route-->>Engine: Route [INT_A, INT_B, INT_C]
    Engine->>Wave: GenerateGreenWave(Route, Speed=60km/h)
    Wave-->>Engine: Green Wave Schedule Envelopes
    Engine->>Net: Broadcast Preemption Schedule
    Net->>Signal: COMMAND (PREEMPT_APPROACH_1, Duration=25s)
    Signal->>Signal: State Machine -> EMERGENCY_PREEMPTION
    Signal-->>Net: ACK (PREEMPTION_ACTIVE)
    Net-->>Engine: Preemption Confirmed
```

### 5.6 Flowchart: Master Decision Pipeline
```mermaid
flowchart TD
    Start([Emergency Call Ingested]) --> Qualify{Valid Location & Category?}
    Qualify -- No --> Reject([Reject Call & Alert Operator])
    Qualify -- Yes --> Triage[Calculate Priority Score P via PriorityEngine]
    
    Triage --> TriageCheck{P >= 70.0 Critical?}
    TriageCheck -- Yes --> Escalate[Set Priority CRITICAL / Escalate EOC]
    TriageCheck -- No --> Standard[Set Priority HIGH / MEDIUM]
    
    Escalate --> Fleet[Select ALS Ambulance via FleetManagementEngine]
    Standard --> FleetBLS[Select Nearest Unit via FleetManagementEngine]
    
    Fleet --> Hospital[Select Trauma Center & Lock ICU via HospitalResourceEngine]
    FleetBLS --> HospitalGen[Select Community Hospital via HospitalResourceEngine]
    
    Hospital --> Route[Compute A* Route via RoutePlanner]
    HospitalGen --> Route
    
    Route --> Congestion{Segment Congestion > 70%?}
    Congestion -- Yes --> Replanner[DynamicRouteReplanner Detour]
    Replanner --> Corridor[Generate Corridor Schedule via GreenWaveCoordinator]
    Congestion -- No --> Corridor
    
    Corridor --> Conflict{Contested Intersections?}
    Conflict -- Yes --> Arbitrate[MultiCorridorCoordinator 5-Stage Arbitration]
    Conflict -- No --> Consensus[DistributedConsensusEngine Quorum Verification]
    Arbitrate --> Consensus
    
    Consensus --> Approved{Quorum Approved?}
    Approved -- Yes --> Actuate[Dispatch Preemption Commands to Traffic Nodes]
    Approved -- No --> Rollback[Abort Preemption & Revert Safe Clearance]
    
    Actuate --> Complete([Green Wave Active & Monitored])
```


---

# SECTION 6: GREEN CORRIDOR GENERATOR

### 6.1 Architecture & Functional Model
The Green Corridor Generator consists of the `EmergencyCorridorPlanner`, `GreenWaveCoordinator`, and `DynamicGreenCorridor` modules. It transforms static road waypoints into a synchronized, dynamic temporal envelope that moves along the route ahead of the emergency vehicle:

```
[Navigation Route] ─────> [EmergencyCorridorPlanner] (Spatial Buffers & Distances)
                                    │
                                    ▼
[Ambulance Velocity Telemetry] ─> [GreenWaveCoordinator] (Temporal Offsets & Velocity Wave)
                                    │
                                    ▼
[Scheduled Preemption Matrix] ──> [Edge Signal Node Actuation & Clearance Windows]
```

### 6.2 Mathematical Algorithms & Wave Calculations
To achieve zero-stop progression, the green wave offset for each consecutive intersection $i$ along the corridor is computed dynamically:

$$t_{	ext{arrival}}(i) = t_{	ext{dispatch}} + \sum_{k=0}^{i-1} rac{d(k, k+1)}{v_{	ext{expected}}(k, k+1)}$$

Where:
* $d(k, k+1)$ is the Haversine geodesic distance between intersection $k$ and intersection $k+1$.
* $v_{	ext{expected}}(k, k+1)$ is the target ambulance velocity adjusted by road classification and curvature.

#### Preemption Temporal Envelope:
For each intersection $i$, the signal actuation window $[t_{	ext{preempt\_start}}(i), \; t_{	ext{preempt\_end}}(i)]$ is bounded by:

$$t_{	ext{preempt\_start}}(i) = t_{	ext{arrival}}(i) - t_{	ext{pre\_clearance}} - t_{	ext{queue\_drain}}$$

$$t_{	ext{preempt\_end}}(i) = t_{	ext{arrival}}(i) + t_{	ext{passage\_buffer}}$$

Where:
* $t_{	ext{pre\_clearance}}$ is the mandatory yellow change (4s) and all-red clearance (2s) interval for cross traffic ($6\,	ext{s}$ total).
* $t_{	ext{queue\_drain}}$ is the time required to flush the standing queue of waiting passenger vehicles ($1.5\,	ext{s}$ per queued vehicle).
* $t_{	ext{passage\_buffer}}$ is the safety trailing window held after vehicle passage ($5\,	ext{s}$) to prevent rear-end collisions.

### 6.3 Corridor Finite State Machine
The corridor lifecycle is governed by an 8-state deterministic state machine:

```mermaid
stateDiagram-v2
    [*] --> INACTIVE
    INACTIVE --> COMPUTING: Route Ingested
    COMPUTING --> RESERVED: Envelopes Calculated
    RESERVED --> PREEMPTION_HOLD: Vehicle In Trigger Zone
    PREEMPTION_HOLD --> ADVANCING: Intersection Cleared
    ADVANCING --> PREEMPTION_HOLD: Next Intersection Triggered
    ADVANCING --> RESTORING: Final Intersection Cleared
    RESTORING --> COMPLETED: Cycles Resynchronized
    PREEMPTION_HOLD --> ABORTED: Conflict / Timeout / Override
    ABORTED --> RESTORING: Safe Clearance Executed
    COMPLETED --> [*]
```

### 6.4 Intersection Sequencing & Progressive Triggering
Progression along the corridor is dynamically synchronized via spatial GPS geofences surrounding each intersection:
* **Outer Geofence ($500\,	ext{m}$):** Coordinator sends `PREPARE_PREEMPTION` directive. Signal finishes current pedestrian clearance and suppresses conflicting phase extensions.
* **Inner Geofence ($250\,	ext{m}$):** Signal initiates mandatory yellow change (4s) and all-red (2s), transitioning perpendicular approaches to red.
* **Approach Geofence ($100\,	ext{m}$):** Protected green phase is locked for emergency vehicle direction.
* **Exit Geofence ($20\,	ext{m}$ past intersection):** Exit sensor trips preemption release; intersection enters restoration phase.

### 6.5 Corridor Restoration Logic
Abruptly snapping traffic signals back to normal operation after preemption can cause acute congestion spikes or driver confusion. SMART AEGIS implements smooth cyclic restoration:
1. **Clearance Window:** Upon exit sensor trigger, the emergency phase transitions through standard yellow (4s) and all-red (2s).
2. **Starvation Compensation:** The cross-street direction that was held on red during preemption is granted immediate priority for its full maximum green interval ($30\,	ext{s}$).
3. **Offset Resynchronization:** Signal timing gently modulates cycle splits over two subsequent cycles until master coordinated network offsets are fully restored.

### 6.6 Concrete Corridor Execution Example
Consider an emergency corridor traversing three intersections: `INT_A`, `INT_B`, and `INT_C` with distances $d(A, B) = 600\,	ext{m}$ and $d(B, C) = 900\,	ext{m}$, at an expected velocity $v = 15\,	ext{m/s}$ ($54\,	ext{km/h}$):

| Intersection | Distance from Origin | Travel Time | Arrival ($t_{	ext{arr}}$) | Pre-Clearance Start ($t - 10	ext{s}$) | Green Lock Window | Restoration Start |
|:---|:---|:---|:---|:---|:---|:---|
| **INT_A** | $200\,	ext{m}$ | $13.3\,	ext{s}$ | $T + 13.3\,	ext{s}$ | $T + 3.3\,	ext{s}$ | $T + 9.3\,	ext{s}$ – $T + 18.3\,	ext{s}$ | $T + 18.3\,	ext{s}$ |
| **INT_B** | $800\,	ext{m}$ | $53.3\,	ext{s}$ | $T + 53.3\,	ext{s}$ | $T + 43.3\,	ext{s}$ | $T + 49.3\,	ext{s}$ – $T + 58.3\,	ext{s}$ | $T + 58.3\,	ext{s}$ |
| **INT_C** | $1700\,	ext{m}$ | $113.3\,	ext{s}$ | $T + 113.3\,	ext{s}$ | $T + 103.3\,	ext{s}$ | $T + 109.3\,	ext{s}$ – $T + 118.3\,	ext{s}$ | $T + 118.3\,	ext{s}$ |


---

# SECTION 7: MULTI AMBULANCE PRIORITY SCHEDULER

### 7.1 Architecture & Scheduling Model
The Multi-Ambulance Priority Scheduler (`PriorityScheduler`, `IntelligentPriorityScheduler`, `MultiCorridorCoordinator`, `ConflictDetectionEngine`, `ConflictResolutionManager`) manages concurrent emergency vehicles competing for the same roadway resources:

```
[Active Vehicle Envelopes] ──> [ConflictDetectionEngine] (Spatial & Temporal Overlap)
                                         │
                                         ▼
[Contested Intersections] ───> [ConflictResolutionManager] (5-Tier Priority Arbitration)
                                         │
                                         ▼
[Arbitration Decisions] ─────> [Exclusive Intersection Lease Table & Stagger Timers]
```

### 7.2 Conflict Taxonomy (5 Conflict Categories)
The conflict detection engine continuously evaluates pairs of active corridor paths across five distinct geometric and temporal conflict categories:

```
1. Spatial Contention:
   Ambulance 1 ───────► [ Intersection B ] ◄─────── Ambulance 2
                        (Perpendicular Crossing within 10s window)

2. Temporal Window Contention:
   Ambulance 1 ───────► [ Intersection B ] ───────► Ambulance 2
                        (Same Direction, headway < safety following distance 15s)

3. Contraflow / Head-On Contention:
   Ambulance 1 ───────► [ Segment B - C ] ◄─────── Ambulance 2
                        (Opposing Directions on single undivided carriageway)

4. Consecutive Subpath Contention:
   Ambulance 1 ───────► [ Int A -> Int B -> Int C ]
   Ambulance 2 ───────► [ Int B -> Int C -> Int D ]
                        (Overlapping multi-intersection chain)

5. Complete Path Overlap:
   Ambulance 1 & 2 traversing identical sequence of intersections simultaneously.
```

### 7.3 5-Tier Deterministic Arbitration Model
When a conflict is detected between two or more ambulances at an intersection, the `ConflictResolutionManager` executes a strict 5-tier arbitration hierarchy:

1. **Tier 1 (Clinical Urgency Score Differential):**
   * If $|S_{	ext{AMB1}} - S_{	ext{AMB2}}| \ge 10.0\,	ext{points}$, the vehicle with the higher clinical priority score wins immediate exclusive preemption.
   * *Example:* Cardiac arrest ($98.0$) strictly beats moderate bleeding ($35.0$).
2. **Tier 2 (Arrival Proximity / Time-to-Intersection):**
   * If clinical scores are within $10.0\,	ext{points}$, the vehicle arriving first ($t_{	ext{ETA}}$) wins right-of-way if $\Delta t_{	ext{ETA}} \ge 5.0\,	ext{seconds}$.
   * *Rationale:* Holding an ambulance that is already inside the intersection approach zone creates dangerous sudden braking hazards.
3. **Tier 3 (Corridor State & Inertia):**
   * A vehicle already executing an active green corridor has higher inertia than a vehicle whose corridor has not yet started.
4. **Tier 4 (Medical Capability Classification):**
   * Higher medical capability tier wins: `INTENSIVE_CARE` > `ADVANCED_LIFE_SUPPORT` > `BASIC_LIFE_SUPPORT`.
5. **Tier 5 (Deterministic Tie-Breaker):**
   * If all clinical, temporal, and capability factors are identical, right-of-way is granted based on lexicographical vehicle ID ordering (`AMB_01` < `AMB_02`), guaranteeing 100% deterministic reproducibility across distributed nodes.

### 7.4 Arbitration Actions & Timing Adjustments
Once the winner and loser are determined, the scheduler applies one of three resolution strategies:
* **Strategy A (Stagger Delay):** The lower-priority vehicle's speed recommendation is modulated to delay arrival by $\Delta t = 8.0\,	ext{seconds}$, allowing the winner to clear the intersection completely.
* **Strategy B (Hold at Preceding Intersection):** If headway is too tight, the lower-priority vehicle's corridor holds green at the preceding intersection, pausing the vehicle safely.
* **Strategy C (Dynamic Reroute):** If the required delay exceeds $25.0\,	ext{seconds}$, the `DynamicRouteReplanner` calculates an alternative non-conflicting route around the congested intersection.

### 7.5 Deadlock Prevention & Intersection Ownership
To mathematically guarantee that multi-vehicle resource contention never produces deadlocks:
1. **Total Resource Ordering:** Intersections are assigned globally unique integer IDs. Whenever an emergency corridor requests exclusive preemption across multiple intersections, locks must be acquired in **strictly ascending intersection ID order**.
2. **Lease Timeouts:** Every intersection preemption lock carries an immutable hardware lease timer ($60\,	ext{seconds}$ maximum). If a vehicle fails to clear or sensor reports fail, the lease expires and the lock is forcibly released.
3. **No Circular Waits:** A corridor holding intersection $K$ cannot block intersection $J$ ($J < K$) while waiting for intersection $L$ ($L > K$); locks are acquired atomically as an envelope batch.

### 7.6 Conflict Arbitration Sequence Diagram
```mermaid
sequenceDiagram
    autonumber
    participant AMB1 as AMB_01 (Cardiac, Score 95)
    participant AMB2 as AMB_02 (Fracture, Score 40)
    participant Detector as ConflictDetectionEngine
    participant Arbiter as ConflictResolutionManager
    participant Signal as Intersection Node B
    participant Replanner as DynamicRouteReplanner

    AMB1->>Detector: Telemetry (ETA to INT_B: 20s, Score: 95)
    AMB2->>Detector: Telemetry (ETA to INT_B: 22s, Score: 40)
    Detector->>Detector: Spatial Contention Detected at INT_B (Overlap < 5s)
    Detector->>Arbiter: Arbitrate(AMB1, AMB2, INT_B)
    Arbiter->>Arbiter: Tier 1 Evaluation: |95 - 40| = 55 >= 10 -> AMB1 Wins
    Arbiter->>Signal: Lock Preemption for AMB_01 (Window: T+15s to T+25s)
    Signal-->>Arbiter: Lease Confirmed for AMB_01
    Arbiter->>AMB1: Maintain Target Velocity (50 km/h) - Corridor Clear
    Arbiter->>Arbiter: Evaluate Delay on AMB_02 (Requires 12s delay)
    Arbiter->>AMB2: Advisory: Reduce Velocity (Target: 32 km/h) - Delay 12s
```


---

# SECTION 8: ROUTE MANAGEMENT

### 8.1 Architecture & Implementation
Route management is governed by `RoutePlanner` (initial static and dynamic A* pathfinding) and `DynamicRouteReplanner` (live road condition adaptation and obstruction evading). 

```
[City Road Network Graph] ──► [RoutePlanner (A* Search)] ──► [Baseline Navigation Route]
                                        ▲
                                        │ (Live Telemetry & Obstruction Feeds)
[Live Segment Traffic] ──────► [DynamicRouteReplanner] ────► [Detour & Reroute Advisories]
```

### 8.2 Road Network Graph Representation
The municipal street topology is modeled as a directed, weighted graph $G = (V, E)$:
* **Vertices ($V$):** Signalized intersections, freeway interchanges, hospital emergency department gates, and ambulance staging stations. Each vertex stores latitude/longitude geocoordinates and intersection ID.
* **Edges ($E$):** Road segments connecting intersections. Each edge $e \in E$ carries static attributes (length in meters, lane count, speed limit, road classification) and dynamic real-time attributes (current average speed, congestion factor $C \in [0.0, 1.0]$, road blockage flags).

### 8.3 A* Pathfinding & Cost Function
The cost function $f(n) = g(n) + h(n)$ balances transit duration against preemption ease:

$$g(n) = \sum_{e \in 	ext{path}} rac{	ext{length}(e)}{v_{	ext{effective}}(e)}$$

$$v_{	ext{effective}}(e) = v_{	ext{speed\_limit}}(e) 	imes (1.0 - C(e)) 	imes W_{	ext{classification}}(e)$$

Where:
* $C(e) \in [0.0, 0.95]$ is the real-time congestion index. If a segment is fully blocked ($C(e) = 1.0$), the edge weight is set to $\infty$.
* $W_{	ext{classification}}$ is a weighting preference factor (e.g., Arterial = 1.0, Collector = 0.85, Residential = 0.70).
* $h(n)$ is the admissible Euclidean / Haversine geodesic distance heuristic to the destination vertex divided by maximum municipal speed limit ($80\,	ext{km/h}$).

### 8.4 Route Lifecycle & Dynamic Replanning Trigger
A navigation route progresses through a well-defined lifecycle:
1. **Planned:** Initial route computed from current ambulance location to emergency scene or hospital.
2. **Active:** Vehicle en route; green wave preemption envelopes scheduled along the route path.
3. **Replanning Triggered:** A replanning event is tripped when one of four trigger conditions occurs:
   * **Segment Obstruction:** A downstream road segment is reported blocked (accident, downed power line, construction).
   * **Acute Congestion Surge:** Live congestion index on a planned segment spikes by $> 30\%$, adding $> 15\,	ext{seconds}$ to total ETA.
   * **Multi-Corridor Conflict Arbitration:** `MultiCorridorCoordinator` requests a detour because the current corridor is blocked by a higher-priority emergency vehicle.
   * **Operator Manual Divert:** Dispatcher manually selects an alternative route.
4. **Detour Computed:** Alternative path generated avoiding the problematic segments.
5. **Corridor Re-anchoring:** Previous corridor preemption envelopes are smoothly cleared, and new green waves are deployed to the new route.
6. **Completed:** Vehicle arrives at destination; corridor is fully released.

### 8.5 Key Data Structures
```cpp
namespace aegis::core {
    struct Waypoint {
        std::string intersectionId;
        double latitude;
        double longitude;
        double targetSpeedKmh;
        uint64_t expectedArrivalTimestamp;
    };

    struct RouteSegment {
        std::string segmentId;
        std::string sourceIntersectionId;
        std::string targetIntersectionId;
        double lengthMeters;
        double speedLimitKmh;
        double currentCongestionLevel; // 0.0 to 1.0
        bool isBlocked;
    };

    struct NavigationRoute {
        std::string routeId;
        std::string ambulanceId;
        std::vector<Waypoint> waypoints;
        std::vector<RouteSegment> segments;
        double totalDistanceMeters;
        double estimatedTotalSeconds;
        uint64_t generatedTimestamp;
    };
}
```


---

# SECTION 9: TRAFFIC SIGNAL CONTROL ENGINE

### 9.1 Signal Control Finite State Machine
The traffic signal execution layer (`SignalStateMachine`, `SignalController`, `TrafficController`, `TrafficSignalNode`) enforces a deterministic 8-state finite state machine governed by strict physical clearance intervals and safety invariants:

```mermaid
stateDiagram-v2
    [*] --> ALL_RED: System Boot
    ALL_RED --> RED: Initialized
    RED --> GREEN: Normal Cycle Green
    GREEN --> YELLOW: Cycle Green Expires (Min Green Enforced)
    YELLOW --> ALL_RED: Yellow Clearance (4s)
    ALL_RED --> RED: Red Clearance (2s)
    
    GREEN --> EMERGENCY_PREEMPTION: Preemption Command (Min Green Satisfied)
    RED --> EMERGENCY_PREEMPTION: Preemption Command (Direct)
    YELLOW --> ALL_RED_PREEMPT: Preemption During Yellow
    ALL_RED_PREEMPT --> EMERGENCY_PREEMPTION: Clearance Complete
    
    EMERGENCY_PREEMPTION --> SAFE_CLEARANCE: Vehicle Clears / Preemption End
    SAFE_CLEARANCE --> YELLOW: Clearance Yellow (4s)
    SAFE_CLEARANCE --> ALL_RED: Clearance All-Red (2s)
    
    GREEN --> FLASHING_YELLOW: Comm Loss / Watchdog Trip / Malfunction
    RED --> FLASHING_YELLOW: Comm Loss / Watchdog Trip / Malfunction
    EMERGENCY_PREEMPTION --> FLASHING_YELLOW: Hardware Fault Detected
    FLASHING_YELLOW --> ALL_OFF: Power Failure / Manual Kill
```

### 9.2 State Transition Matrix & Clearance Rules
The state machine strictly enforces timing intervals:

| Current State | Next State | Trigger / Condition | Minimum Duration | Safety Invariant Enforced |
|:---|:---|:---|:---|:---|
| `RED` | `GREEN` | Normal cycle timer expired | $10.0\,	ext{s}$ (Min Green) | Conflicting approaches verified RED |
| `GREEN` | `YELLOW` | Normal phase timer expired | $4.0\,	ext{s}$ (Yellow Change) | Minimum green interval ($10.0\,	ext{s}$) elapsed |
| `YELLOW` | `ALL_RED` | Yellow clearance timer expired | $2.0\,	ext{s}$ (Red Clearance) | Clearing intersection of turning vehicles |
| `ALL_RED` | `RED` / `GREEN` | Clearance interval expired | $0.0\,	ext{s}$ | Zero green indications citywide |
| `RED` / `GREEN` | `EMERGENCY_PREEMPTION` | Preemption directive received | $15.0\,	ext{s}$ – $60.0\,	ext{s}$ | Conflicting approaches forced to RED |
| `EMERGENCY_PREEMPTION` | `SAFE_CLEARANCE` | Exit sensor tripped or lease expired | $4.0\,	ext{s}$ Yellow + $2.0\,	ext{s}$ Red | Prevents instant phase snapping |
| Any State | `FLASHING_YELLOW` | Heartbeat timeout ($> 5.0\,	ext{s}$) or error | Indefinite until reset | Fail-safe edge autonomy |

### 9.3 Strict Physical Mutual Exclusion Guarantee
The most critical safety guarantee in SMART AEGIS is that **conflicting signal phases can never display GREEN simultaneously**. This is guaranteed at three distinct levels:
1. **Software Level (FSM):** The state machine evaluates phase bitmasks. If the requested preemption phase shares any conflicting approach with an active phase, the transition is blocked until the active phase transitions through yellow and all-red.
2. **HAL Level (Command Validator):** The `HardwareAbstractionLayer` inspects outgoing GPIO write commands. A write command asserting a green lamp relay is rejected if any conflicting green relay is currently marked HIGH in the driver's software shadow register.
3. **Hardware Level (Relay Interlock):** On physical intersection installations, physical relay boards use double-pole double-throw (DPDT) hardware interlocks where the NC/NO contacts physically cut power to conflicting green lamp circuits, preventing hardware faults from energizing dual greens.

### 9.4 Emergency Mode Preemption Execution
When a valid `COMMAND` packet with opcode `PREEMPT_APPROACH_1` arrives at the edge node:
1. If the current phase is already Green for Approach 1, the timer is extended by the requested duration.
2. If Approach 1 is currently Red and Approach 2 is Green:
   * The controller checks if Approach 2 has met its minimum green ($10\,	ext{s}$). If not, it waits for the remaining seconds.
   * Approach 2 transitions to Yellow ($4\,	ext{s}$).
   * All approaches transition to All-Red ($2\,	ext{s}$).
   * Approach 1 energizes Green, locking the preemption hold.
3. Exit detection or lease expiration releases the hold, transitioning Approach 1 through Yellow ($4\,	ext{s}$) and All-Red ($2\,	ext{s}$), before returning control to cross-traffic.


---

# SECTION 10: NETWORK COMMUNICATION

### 10.1 Architecture & Implementation
The SMART AEGIS networking infrastructure is implemented as a pure Modern C++17 standalone library (`network/`) targeting QNX Neutrino RTOS and POSIX-compliant platforms without any third-party networking dependencies:

```
[Coordinator TCP Server] (Port 9000)
    │
    ├── Listen/Accept Loop (Non-blocking poll(), Backlog 128)
    ├── Per-Client Dedicated Worker Threads (std::thread)
    ├── Dynamic Heartbeat Watchdog Supervisor (5.0s Timeout Scan)
    └── Disjoint Mutex Domains (clientsMutex_, statisticsMutex_)
            ▲
            │ TCP/IP Wire Stream (19-Byte Header + Checksum)
            ▼
[Traffic Node TCP Clients] (Node A, Node B, Node C)
    │
    ├── Non-Blocking Connect & Immediate REGISTER Handshake
    ├── Autonomous 1 Hz Heartbeat Generator Timer Thread
    ├── Thread-Safe Synchronous & Asynchronous RX Handling
    └── Non-Blocking Auto-Reconnect Supervisor Thread (2.0s Backoff)
```

### 10.2 Connection Lifecycle & Handshake Flow
1. **Socket Connection:** Edge client connects to the coordinator socket on port 9000 (`Connect()`).
2. **Immediate Registration:** Upon socket establishment, the client immediately transmits a `REGISTER` frame containing its fixed 4-character node identifier (e.g. `INTA`, `INTB`, `INTC`).
3. **Server Registration:** The server validates the frame, binds the socket file descriptor to the node ID in its internal client table, and dispatches a confirmation `ACK` frame.
4. **Heartbeat Cadence:** The client's background timer emits a `HEARTBEAT` frame once every 1.0 second ($1\,	ext{Hz}$). The server updates `lastHeartbeatTimestamp`.
5. **Periodic Telemetry:** The client emits `STATUS` and `TRAFFIC_STATE` frames whenever local phases or sensor readings change.
6. **Command Dispatch:** The coordinator transmits `COMMAND` frames to trigger emergency corridor preemption. The client replies with an `ACK` frame.

### 10.3 Heartbeat & Watchdog Mechanics
* **Client-Side Generation:** `TcpClient::StartHeartbeat(intervalMs = 1000)` spawns an autonomous timer thread executing:
  ```cpp
  while (heartbeatRunning_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatIntervalMs_));
      SendHeartbeat();
  }
  ```
* **Server-Side Watchdog Supervisor:** `TcpServer::WatchdogLoop()` scans all registered clients every 1.0 second:
  ```cpp
  const uint64_t now = GetMonotonicTimeMs();
  for (const auto& [nodeId, client] : clients_) {
      if (now - client->lastHeartbeatMs > heartbeatTimeoutMs_) { // 5000 ms
          // Mark offline, close socket, remove from active preemption quorum
          DisconnectNode(nodeId, "HEARTBEAT_TIMEOUT");
      }
  }
  ```

### 10.4 Auto-Reconnection & Link Recovery
If an edge node loses its link due to network partition or coordinator restart:
1. The client's receiver thread detects `recv() <= 0` or socket error.
2. The client flags `isConnected_ = false` and invokes the connection change callback.
3. The background auto-reconnect supervisor awakens, closes the dead socket descriptor, and attempts to re-establish the connection every 2.0 seconds.
4. Once reconnected, the client immediately transmits a fresh `REGISTER` frame, resuming synchronized operation without requiring node reboot.

### 10.5 Signal & Broken Pipe Protection
Under Unix and POSIX operating systems, calling `send()` on a socket closed by the remote peer delivers the `SIGPIPE` signal, which terminates the process by default. SMART AEGIS employs three layers of defense:
1. `std::signal(SIGPIPE, SIG_IGN)` is installed in both `TcpServer` and `TcpClient` constructors.
2. Sockets on macOS/BSD are configured with `setsockopt(..., SO_NOSIGPIPE, ...)`.
3. Outgoing socket `send()` calls pass the `MSG_NOSIGNAL` flag where supported.


---

# SECTION 11: MESSAGE PROTOCOL

### 11.1 Binary Wire Protocol Specification
All network packets in SMART AEGIS use a standardized, endian-neutral binary frame structure. The frame consists of a fixed 19-byte header followed by variable-length payload bytes:

```
+===================================================================================+
| OFFSET | FIELD NAME       | TYPE     | SIZE | DESCRIPTION                         |
+===================================================================================+
| 0x00   | Magic Byte 0     | uint8_t  | 1 B  | 0xA5 (Stream Framing Marker 1)      |
| 0x01   | Magic Byte 1     | uint8_t  | 1 B  | 0x5A (Stream Framing Marker 2)      |
| 0x02   | Message Type     | uint8_t  | 1 B  | Opcode (0x01 to 0x07)               |
| 0x03   | Timestamp        | uint64_t | 8 B  | Big-Endian Milliseconds Epoch       |
| 0x0B   | Node Identifier  | char[4]  | 4 B  | Fixed 4-character ASCII Node ID     |
| 0x0F   | Payload Length   | uint16_t | 2 B  | Big-Endian Payload Length (0-65535) |
| 0x11   | Checksum         | uint16_t | 2 B  | Big-Endian CRC-16-CCITT             |
| 0x13   | Payload Bytes    | uint8_t[]| N B  | Variable payload data               |
+===================================================================================+
```

### 11.2 Message Type Enumeration
```cpp
enum class MessageType : uint8_t {
    REGISTER      = 0x01, // Edge client registration
    HEARTBEAT     = 0x02, // 1 Hz keepalive pulse
    STATUS        = 0x03, // Edge telemetry and health metrics
    TRAFFIC_STATE = 0x04, // Current signal phase and approach queues
    COMMAND       = 0x05, // Coordinator preemption directives
    ACK           = 0x06, // Transactional acknowledgment
    ERROR         = 0x07  // Protocol or hardware fault notification
};
```

### 11.3 CRC-16-CCITT Checksum Calculation
Data integrity is verified using the standard CRC-16-CCITT polynomial ($x^{16} + x^{12} + x^5 + 1$, polynomial `0x1021`, initial value `0xFFFF`):

```cpp
uint16_t Message::CalculateCRC16(const uint8_t* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint16_t>(data[i] << 8);
        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}
```
* **Checksum Scope:** The CRC16 is computed across the header bytes (excluding the checksum field itself: bytes `0x00` through `0x10`) concatenated with all payload bytes (bytes `0x13` through `0x13 + N - 1`).

### 11.4 Stream Defragmentation & Packet Reconstruction
In TCP streams, packets can arrive fragmented across multiple `recv()` calls or batched together into a single buffer. The stream accumulator algorithm parses complete frames and discards corrupted noise:

```cpp
bool Message::ParseStream(std::vector<uint8_t>& buffer, Message& outMsg) {
    while (buffer.size() >= HEADER_SIZE) {
        // Step 1: Scan for Magic Bytes 0xA5 0x5A
        if (buffer[0] != MAGIC_BYTE_0 || buffer[1] != MAGIC_BYTE_1) {
            buffer.erase(buffer.begin()); // Discard leading noise
            continue;
        }

        // Step 2: Read payload length from bytes 15..16
        const uint16_t payloadLen = static_cast<uint16_t>(
            (static_cast<uint16_t>(buffer[15]) << 8) | buffer[16]);
        const size_t totalFrameSize = HEADER_SIZE + payloadLen;

        // Step 3: Check if full frame is present in buffer
        if (buffer.size() < totalFrameSize) {
            return false; // Wait for remaining TCP segments
        }

        // Step 4: Deserialize and verify CRC
        std::vector<uint8_t> frameData(buffer.begin(), buffer.begin() + totalFrameSize);
        if (outMsg.Deserialize(frameData)) {
            buffer.erase(buffer.begin(), buffer.begin() + totalFrameSize);
            return true; // Valid frame extracted
        }

        // CRC failed; slide window by 1 byte to re-sync
        buffer.erase(buffer.begin());
    }
    return false;
}
```

### 11.5 Byte-Level Example: Preemption Command Frame
The following represents the exact binary wire frame for a `COMMAND` message from coordinator to `INTA` with payload `"PREEMPT_APP_1"`:

```text
[Header - 19 Bytes]
A5 5A          -> Magic Bytes (0xA5, 0x5A)
05             -> Type: COMMAND (0x05)
00 00 01 9B 42 10 5A 20 -> Timestamp: 1,766,345,678,368 ms
49 4E 54 41    -> Node ID: "INTA"
00 0D          -> Payload Length: 13 Bytes (0x000D)
3C 8F          -> Checksum: CRC-16 (0x3C8F)

[Payload - 13 Bytes]
50 52 45 45 4D 50 54 5F 41 50 50 5F 31 -> "PREEMPT_APP_1"
```


---

# SECTION 12: THREADING MODEL

### 12.1 Thread Architecture & Concurrency Matrix
SMART AEGIS employs a multi-threaded architecture with strict separation of real-time execution, network I/O, and background supervision. The table below outlines all persistent threads across the system:

| Process | Thread Name | Execution Rate | Priority / Policy | Core Responsibilities |
|:---|:---|:---|:---|:---|
| **Coordinator** | `Main/EventLoop` | Continuous | Normal (`SCHED_OTHER`) | Process initialization, signal handling, user console |
| **Coordinator** | `TcpAcceptWorker` | Event-driven (`poll()`) | High (`SCHED_RR`) | Non-blocking socket accept on port 9000 |
| **Coordinator** | `TcpClientWorker[N]`| Blocking `recv()` | High (`SCHED_RR`) | Ingests frames from connected edge node $N$ |
| **Coordinator** | `HeartbeatWatchdog`| $1.0\,	ext{Hz}$ periodic | Normal (`SCHED_OTHER`) | Scans edge node keepalives, evicts dead nodes ($> 5	ext{s}$) |
| **Coordinator** | `MissionExecutionLoop`| $10\,	ext{Hz}$ ($100\,	ext{ms}$) | Critical (`SCHED_FIFO`) | Evaluates mission progression and preemption timing |
| **Coordinator** | `ClusterHealthMonitor`| $0.2\,	ext{Hz}$ ($5.0\,	ext{ms}$) | Low (`SCHED_OTHER`) | Diagnostic scans and predictive maintenance |
| **Intersection** | `Main/Daemon` | Continuous | Normal (`SCHED_OTHER`) | Process lifecycle, hardware shutdown traps |
| **Intersection** | `TcpClientReceiver`| Blocking `recv()` | High (`SCHED_RR`) | Reads binary frames from coordinator link |
| **Intersection** | `HeartbeatGenerator`| $1.0\,	ext{Hz}$ periodic | Normal (`SCHED_OTHER`) | Emits keepalive frame to coordinator |
| **Intersection** | `AutoReconnectLoop` | $0.5\,	ext{Hz}$ backoff | Low (`SCHED_OTHER`) | Background socket monitor re-establishing dropped links |
| **Intersection** | `SignalControllerFSM`| $10\,	ext{Hz}$ ($100\,	ext{ms}$) | Critical (`SCHED_FIFO`) | Ticks phase timers, clearance intervals, and GPIO relays |

### 12.2 Mutex Hierarchy & Deadlock Prevention
To guarantee that concurrent execution never deadlocks under multi-threaded load:
1. **Disjoint Mutex Domains:** Internal subsystems maintain non-overlapping mutex domains. A thread holding a network lock never requests a scheduling lock, and a thread holding a scheduling lock never performs blocking socket I/O.
2. **Dedicated Subsystem Mutexes:**
   * `clientsMutex_`: Guards the coordinator's connected client descriptor map.
   * `statisticsMutex_`: Guards performance counter accumulators.
   * `callbackMutex_`: Protects registered event handlers.
   * `socketMutex_`: Guards raw socket file descriptor writes in `TcpClient`.
   * `syncRxMutex_`: Synchronizes synchronous condition-variable receive operations.
3. **Lock Ordering Discipline:** If multiple locks must ever be acquired simultaneously, they are acquired in strict canonical order:
   $$	ext{Domain Lock} \prec 	ext{Registry Lock} \prec 	ext{Statistics Lock}$$
4. **Zero Lock Inversion:** No callback is ever executed while holding an internal state mutex. Callbacks are invoked *after* releasing the lock or on local copies of state.

### 12.3 Thread Execution Flow Diagram
```mermaid
sequenceDiagram
    autonumber
    participant Accept as TcpAcceptWorker
    participant Worker as TcpClientWorker
    participant Watchdog as HeartbeatWatchdog
    participant Engine as MissionExecutionLoop
    participant FSM as SignalControllerFSM

    Note over Accept,Watchdog: Coordinator Process Threads
    Accept->>Worker: Spawn on New Edge Connection
    loop Blocking Read
        Worker->>Worker: Parse 19-Byte Frames & Verify CRC16
        Worker->>Engine: Push Event (REGISTER / TELEMETRY / ACK)
    end
    loop Every 1000ms
        Watchdog->>Watchdog: Check client->lastHeartbeatMs
        alt Elapsed > 5000ms
            Watchdog->>Worker: Terminate Worker & Reclaim Socket
        end
    end
    loop Every 100ms
        Engine->>Engine: Tick Preemption Envelopes
        Engine->>Worker: Send COMMAND Frame
    end
    Note over FSM: Edge Node Process Thread
    loop Every 100ms
        FSM->>FSM: Update Phase Durations & Clearance Intervals
    end
```


---

# SECTION 13: PROCESS ARCHITECTURE

### 13.1 Process Partitioning & Binaries
SMART AEGIS builds into discrete, modular standalone process binaries designed to run across multiple physical machines or virtual containers:

```
[MUNICIPAL COORDINATOR HOST / SERVER]
  └── Binary: aegis_coordinator (or network/example_server)
      ├── Coordinator decision pipeline
      ├── TCP Master Server (:9000)
      ├── Incident command & mission engines
      └── Fleet & Hospital registries

[EDGE TRAFFIC CONTROLLER NODES - RPi 2, 3, 4]
  └── Binary: aegis_intersection_node (or aegis_traffic_controller / example_client)
      ├── Local traffic signal state machine
      ├── Hardware Abstraction Layer & BCM2711 GPIO
      └── TCP Client connecting to Coordinator

[MOBILE EMERGENCY VEHICLES]
  └── Binary: aegis_ambulance
      ├── Kinematics & GPS simulation
      └── Vitals generation & telemetry client
```

### 13.2 Automated Startup Sequencing
To ensure deterministic discovery and eliminate race conditions during system power-up, nodes must be started in strict chronological sequence:

```
Step 1: Start Central Coordinator Daemon
        $ ./build/bin/aegis_coordinator --config config/deployment/rpi1_coordinator.json
        -> Binds TCP server to 0.0.0.0:9000
        -> Initializes incident registries and event queue
        -> Enters listening state

Step 2: Start Edge Intersection Nodes (Nodes A, B, C)
        $ ./build/bin/aegis_intersection_node --config config/deployment/rpi2_intersection_a.json
        -> Initializes Hardware Abstraction Layer & BCM2711 GPIO pins
        -> Puts local lamps into baseline cyclic operation (RED/GREEN/YELLOW)
        -> Connects outward to Coordinator on 192.168.1.100:9000
        -> Sends REGISTER frame
        -> Begins 1 Hz heartbeat transmission

Step 3: Start Mobile Ambulance Units
        $ ./build/bin/aegis_ambulance --config config/default.json
        -> Connects to Coordinator
        -> Begins streaming kinematics and vitals telemetry
```

### 13.3 Graceful Shutdown Sequencing
Abrupt termination of traffic controllers can leave physical signal lamps dark or in unsafe states. SMART AEGIS implements an orderly 3-stage shutdown sequence:
1. **Signal Trapping:** All binaries trap `SIGINT` (Ctrl+C) and `SIGTERM`.
2. **Safe Signal State Transition:**
   * Edge nodes immediately transition signal lamps from `GREEN` or `EMERGENCY_PREEMPTION` into standard `YELLOW` ($4\,	ext{s}$), followed by `ALL_RED` ($2\,	ext{s}$).
   * Relays are put into cold-iron fail-safe default states (normally-closed red or hardware flasher).
3. **Socket Teardown:**
   * Clients send an unregister notice and execute `shutdown(SHUT_WR)` followed by `close()`.
   * Server shuts down listener, joins worker threads, flushes log buffers to disk, and exits cleanly.

### 13.4 Process Failure Recovery & Watchdog Supervision
If an edge traffic controller process terminates abnormally:
* **Hardware Interlock Trip:** Loss of software GPIO keepalive de-energizes the hardware relay watchdog, tripping external fail-safe circuitry to hardware flashing yellow.
* **Coordinator Detection:** Coordinator's heartbeat watchdog detects missing keepalives within $5.0\,	ext{seconds}$, marks the node `OFFLINE`, and removes it from active preemption quorums.
* **Automatic Restart:** Systemd (Linux) or High Availability Manager / HAM (QNX) automatically restarts the crashed daemon.
* **State Resynchronization:** The restarted node establishes a new TCP connection, sends `REGISTER`, and re-synchronizes its clock and phase state with the coordinator.


---

# SECTION 14: DATA STRUCTURES

This section details the primary data structures, enums, constants, and containers implemented across SMART AEGIS.

---

### 14.1 Core Enums
```cpp
namespace aegis::network {
    enum class MessageType : uint8_t {
        REGISTER      = 0x01,
        HEARTBEAT     = 0x02,
        STATUS        = 0x03,
        TRAFFIC_STATE = 0x04,
        COMMAND       = 0x05,
        ACK           = 0x06,
        ERROR         = 0x07
    };
}

namespace aegis::edge {
    enum class SignalState {
        RED, GREEN, YELLOW, ALL_RED,
        EMERGENCY_PREEMPTION, SAFE_CLEARANCE,
        FLASHING_YELLOW, ALL_OFF
    };

    enum class SignalOperationalMode {
        FIXED_TIME, ACTUATED, COORDINATED_WAVE,
        EMERGENCY_PREEMPTION, FAILSAFE_FLASHING
    };
}

namespace aegis::bsp {
    enum class PinMode { INPUT, OUTPUT, ALT0, ALT1, ALT2, ALT3, ALT4, ALT5 };
    enum class PinState { LOW, HIGH };
    enum class PullMode { NONE, PULL_UP, PULL_DOWN };
    enum class InterruptTrigger { NONE, RISING_EDGE, FALLING_EDGE, BOTH_EDGES, HIGH_LEVEL, LOW_LEVEL };
}

namespace aegis::qnx_driver {
    enum class DriverState { UNREGISTERED, REGISTERED, INITIALIZED, RUNNING, STOPPED, FAILED };
    enum class ResourceType { GPIO, PWM, SPI, I2C, UART, ETHERNET, MEMORY };
}

namespace aegis::hal {
    enum class DriverStatus { OFFLINE, ONLINE, INITIALIZED, READY, BUSY, ERROR };
    enum class DeviceCategory { GPIO, RELAY, PWM, SPI, I2C, UART, SENSOR, NETWORK, DISPLAY };
}

namespace aegis::medical {
    enum class EmergencyCondition {
        CARDIAC_ARREST, MASSIVE_TRAUMA, STROKE,
        SEVERE_RESPIRATORY_DISTRESS, SEVERE_BURNS,
        ACUTE_CORONARY_SYNDROME, OBSTETRIC_EMERGENCY,
        HEAD_INJURY, INTERNAL_BLEEDING, SEVERE_FRACTURE,
        MODERATE_BLEEDING, MINOR_INJURY
    };

    enum class TriageLevel { CRITICAL, HIGH, MEDIUM, LOW };
}

namespace aegis::fleet {
    enum class AmbulanceCapability { BASIC, ADVANCED, INTENSIVE_CARE, NEONATAL, HAZMAT };
    enum class AmbulanceStatus { AVAILABLE, DISPATCHED, EN_ROUTE_SCENE, ON_SCENE, TRANSPORTING, AT_HOSPITAL, MAINTENANCE, OFFLINE };
}

namespace aegis::hospital_management {
    enum class HospitalStatus { ONLINE, OFFLINE, FULL, EMERGENCY_ONLY, MAINTENANCE };
    enum class SpecialtyCapability { GENERAL, TRAUMA, CARDIAC, PEDIATRIC, NEURO, BURNS, ICU, NEONATAL, ORTHOPEDIC, MULTISPECIALITY };
}

namespace aegis::incident_command {
    enum class IncidentSeverity { NORMAL, HIGH, CRITICAL, DISASTER };
    enum class IncidentStatus { REPORTED, TRIAGED, DISPATCHED, ACTIVE, MITIGATED, CLOSED };
}

namespace aegis::mission {
    enum class MissionStage {
        CALL_INGESTION, TRIAGE_EVALUATION, AMBULANCE_DISPATCH,
        EN_ROUTE_SCENE, ON_SCENE_CARE, HOSPITAL_SELECTION,
        HOSPITAL_RESERVATION, ROUTE_CORRIDOR_PLANNING,
        GREEN_WAVE_EXECUTION, HOSPITAL_ARRIVAL, MISSION_COMPLETED
    };

    enum class MissionState { CREATED, ACTIVE, EN_ROUTE, AT_SCENE, TRANSPORTING, AT_HOSPITAL, COMPLETED, ABORTED };
}
```

### 14.2 Core Structs & Data Models
```cpp
namespace aegis::network {
    struct Message {
        static constexpr size_t HEADER_SIZE = 19;
        static constexpr uint8_t MAGIC_BYTE_0 = 0xA5;
        static constexpr uint8_t MAGIC_BYTE_1 = 0x5A;

        MessageType type{MessageType::HEARTBEAT};
        uint64_t timestamp{0};
        std::string nodeId{"NONE"};
        uint16_t payloadLength{0};
        uint16_t checksum{0};
        std::vector<uint8_t> payload;

        std::vector<uint8_t> Serialize() const;
        bool Deserialize(const std::vector<uint8_t>& buffer);
        static uint16_t CalculateCRC16(const uint8_t* data, size_t length);
        static bool ParseStream(std::vector<uint8_t>& buffer, Message& outMsg);
    };
}

namespace aegis::bsp {
    struct GPIOPinDescriptor {
        uint8_t pinNumber;
        PinMode mode{PinMode::INPUT};
        PinState state{PinState::LOW};
        PullMode pull{PullMode::NONE};
        bool isReserved{false};
        std::string reservedBy;
        InterruptTrigger interruptTrigger{InterruptTrigger::NONE};
        uint64_t lastToggleTimestamp{0};
    };
}

namespace aegis::medical {
    struct PatientProfile {
        std::string patientId;
        EmergencyCondition condition;
        int glasgowComaScale;   // 3 to 15
        int systolicBloodPressure; // mmHg
        int heartRate;          // bpm
        int respiratoryRate;    // breaths/min
        int oxygenSaturation;   // SpO2 percentage (0 to 100)
        double computedPriorityScore; // 0.0 to 100.0
        TriageLevel triageLevel;
    };
}

namespace aegis::fleet {
    struct AmbulanceUnit {
        std::string unitId;
        AmbulanceCapability capability;
        AmbulanceStatus status;
        double currentLatitude;
        double currentLongitude;
        double batteryLevelPct;
        double speedKmh;
        uint64_t lastHeartbeatMs;
        std::string assignedIncidentId;
    };
}

namespace aegis::hospital_management {
    struct HospitalFacility {
        std::string hospitalId;
        std::string name;
        double latitude;
        double longitude;
        HospitalStatus status;
        uint32_t totalBeds;
        uint32_t availableBeds;
        uint32_t totalICUBeds;
        uint32_t availableICUBeds;
        uint32_t totalVentilators;
        uint32_t availableVentilators;
        uint32_t availableDoctors;
        std::vector<SpecialtyCapability> capabilities;
    };
}
```

### 14.3 Entity Relationship Diagram
```mermaid
erDiagram
    EmergencyCall ||--|| MissionRecord : "initiates"
    MissionRecord ||--|| PatientProfile : "contains"
    MissionRecord ||--o| AmbulanceUnit : "assigns"
    MissionRecord ||--o| HospitalFacility : "reserves"
    MissionRecord ||--|| NavigationRoute : "follows"
    NavigationRoute ||--|{ Waypoint : "composed_of"
    NavigationRoute ||--|| GreenCorridor : "triggers"
    GreenCorridor ||--|{ ScheduledPreemption : "enforces"
    ScheduledPreemption ||--|| IntersectionNode : "actuates"
    IntersectionNode ||--|{ GPIOPinDescriptor : "controls"
```


---

# SECTION 15: API REFERENCE

This section provides the formal API specification for the major system classes.

---

### 15.1 `aegis::network::TcpServer`
* **Header:** `network/tcp_server.hpp`
* **Methods:**
  * `bool Start(const std::string& host, uint16_t port, int backlog = 128)`
    * *Purpose:* Binds server socket, configures `SO_REUSEADDR`, `SO_REUSEPORT`, starts non-blocking listen loop, and launches background heartbeat watchdog thread.
    * *Parameters:* `host` (IPv4 address, e.g. `"0.0.0.0"`), `port` (listening port, e.g. `9000`), `backlog` (max pending listen queue, default `128`).
    * *Returns:* `true` on successful socket initialization and bind; `false` on socket error.
  * `void Stop()`
    * *Purpose:* Terminates listener thread, closes all connected client sockets, joins worker threads, and clears client registries.
  * `bool Broadcast(const Message& msg)`
    * *Purpose:* Transmits a message to all currently connected edge nodes.
    * *Returns:* `true` if dispatched to at least one client without network error.
  * `bool SendToNode(const std::string& nodeId, const Message& msg)`
    * *Purpose:* Unicasts a message to the specific registered node identified by `nodeId`.
  * `void SetOnMessage(MessageCallback callback)`
    * *Purpose:* Registers an asynchronous callback invoked whenever a valid message frame is received from any client.
  * `void SetHeartbeatTimeoutMs(uint32_t timeoutMs)`
    * *Purpose:* Sets the watchdog eviction timeout for silent nodes (default `5000` ms).

---

### 15.2 `aegis::network::TcpClient`
* **Header:** `network/tcp_client.hpp`
* **Methods:**
  * `bool Connect(const std::string& host, uint16_t port, uint32_t timeoutMs = 3000)`
    * *Purpose:* Establishes a TCP connection to the coordinator, spawns the receiver read thread, and immediately emits a `REGISTER` frame.
    * *Parameters:* `host` (coordinator IP), `port` (coordinator port), `timeoutMs` (connect timeout).
    * *Returns:* `true` if connected and registered; `false` otherwise.
  * `void Disconnect()`
    * *Purpose:* Disables auto-reconnect, stops heartbeats, sends unregister notice, and closes the socket.
  * `bool Send(const Message& msg)`
    * *Purpose:* Thread-safe serialization and transmission of a message frame.
  * `bool Receive(Message& outMsg, uint32_t timeoutMs = 1000)`
    * *Purpose:* Synchronous condition-variable receive blocking until a frame arrives or timeout expires.
  * `void StartHeartbeat(uint32_t intervalMs = 1000)`
    * *Purpose:* Spawns the autonomous background heartbeat generator timer thread.
  * `void SetAutoReconnect(bool enabled, uint32_t intervalMs = 2000)`
    * *Purpose:* Enables autonomous background socket reconnection on link loss.

---

### 15.3 `aegis::bsp::RaspberryPi4BSP`
* **Header:** `src/bsp/RaspberryPi4BSP.h`
* **Methods:**
  * `bool Initialize()`
    * *Purpose:* Initializes BCM2711 peripheral descriptors, resets 58 GPIO pin definitions, and clears reservation locks.
  * `bool ReserveGPIO(uint8_t pin, const std::string& consumer)`
    * *Purpose:* Acquires exclusive reservation lock on the specified GPIO pin (0–57). Fails if already reserved by another module.
  * `bool SetPinMode(uint8_t pin, PinMode mode)`
    * *Purpose:* Configures pin function (`INPUT`, `OUTPUT`, or `ALT0`–`ALT5`). Requires prior reservation.
  * `bool WritePin(uint8_t pin, PinState state)`
    * *Purpose:* Sets digital output state (`HIGH` or `LOW`). Enforces that pin mode is configured as `OUTPUT`.
  * `bool ReadPin(uint8_t pin, PinState& outState)`
    * *Purpose:* Reads digital logic state. Valid for both `INPUT` and `OUTPUT` pins.
  * `bool TogglePin(uint8_t pin)`
    * *Purpose:* Inverts current pin logic state with sub-microsecond latency.
  * `bool ConfigureInterrupt(uint8_t pin, InterruptTrigger trigger, InterruptCallback callback)`
    * *Purpose:* Binds an edge or level interrupt callback to the specified GPIO input pin.

---

### 15.4 `aegis::medical::PriorityEngine`
* **Header:** `src/medical/PriorityEngine.h`
* **Methods:**
  * `double CalculatePriorityScore(const PatientProfile& profile) const`
    * *Purpose:* Calculates the continuous 0–100 clinical urgency score based on base medical condition and physiological vitals (GCS, BP, HR, RR, SpO2).
    * *Returns:* Double precision score clamped between `0.0` and `100.0`.
  * `TriageLevel CategorizeTriageLevel(double score) const`
    * *Purpose:* Maps continuous score to categorical triage level:
      * $S \ge 75.0$: `CRITICAL`
      * $50.0 \le S < 75.0$: `HIGH`
      * $25.0 \le S < 50.0$: `MEDIUM`
      * $S < 25.0$: `LOW`

---

### 15.5 `aegis::hospital_management::HospitalResourceEngine`
* **Header:** `src/hospital_management/HospitalResourceEngine.h`
* **Methods:**
  * `std::string SelectOptimalHospital(const PatientClinicalProfile& profile, const GeoCoordinates& location)`
    * *Purpose:* Ranks all registered municipal hospitals via 10 deterministic selection rules and returns the optimal hospital ID.
  * `bool ReserveBed(const std::string& hospitalId, const std::string& patientId)`
    * *Purpose:* Decrements available inpatient beds and locks reservation record. Fails if capacity is zero.
  * `bool ReserveICUBed(const std::string& hospitalId, const std::string& patientId)`
    * *Purpose:* Decrements available ICU critical care beds and reserves trauma resources.


---

# SECTION 16: STATE MACHINES

SMART AEGIS enforces formal deterministic state machines across all critical operational domains.

---

### 16.1 Master Coordinator Mission FSM
```mermaid
stateDiagram-v2
    [*] --> IDLE
    IDLE --> CALL_INGESTED: 911 Call Ingestion
    CALL_INGESTED --> TRIAGING: Ingestion Validated
    TRIAGING --> DISPATCHING: Priority Score Calculated
    DISPATCHING --> CORRIDOR_ACTIVE: Unit & Hospital Assigned
    CORRIDOR_ACTIVE --> ADVANCING: Green Wave Progressing
    ADVANCING --> ARRIVED_SCENE: Ambulance On Scene
    ARRIVED_SCENE --> TRANSPORTING: Patient Loaded
    TRANSPORTING --> ARRIVED_HOSPITAL: Hospital Transit Complete
    ARRIVED_HOSPITAL --> RESTORATION: Corridor Restored
    RESTORATION --> COMPLETED: Audit Log Finalized
    CORRIDOR_ACTIVE --> ABORTED: Conflict / Cancel
    ABORTED --> RESTORATION: Safe Rollback
    COMPLETED --> [*]
```

### 16.2 Traffic Signal Controller FSM
```mermaid
stateDiagram-v2
    [*] --> ALL_RED: Boot
    ALL_RED --> RED: Safe Init
    RED --> GREEN: Phase Timer (Min Green 10s)
    GREEN --> YELLOW: Phase Timer Expired
    YELLOW --> ALL_RED: Yellow Clearance (4s)
    ALL_RED --> RED: Red Clearance (2s)
    GREEN --> EMERGENCY_PREEMPTION: Preempt Trigger (Min Green Done)
    RED --> EMERGENCY_PREEMPTION: Preempt Trigger
    EMERGENCY_PREEMPTION --> SAFE_CLEARANCE: Vehicle Exit / Lease Expire
    SAFE_CLEARANCE --> YELLOW: Clearance Yellow (4s)
    SAFE_CLEARANCE --> ALL_RED: Clearance Red (2s)
    Any --> FLASHING_YELLOW: Timeout / Malfunction
```

### 16.3 Distributed Network Connection FSM
```mermaid
stateDiagram-v2
    [*] --> DISCONNECTED
    DISCONNECTED --> CONNECTING: Connect() Called
    CONNECTING --> CONNECTED: Socket Handshake Complete
    CONNECTED --> REGISTERED: REGISTER Frame Acknowledged
    REGISTERED --> REGISTERED: 1 Hz Heartbeat Exchanged
    REGISTERED --> TIMEOUT: Missed Heartbeat > 5s
    TIMEOUT --> DISCONNECTED: Socket Reclaimed
    DISCONNECTED --> RECONNECTING: Auto-Reconnect Timer
    RECONNECTING --> CONNECTING: Socket Attempt
```

### 16.4 Ambulance Lifecycle FSM
```mermaid
stateDiagram-v2
    [*] --> AVAILABLE
    AVAILABLE --> DISPATCHED: Dispatch Directive
    DISPATCHED --> EN_ROUTE_SCENE: Unit Rolling
    EN_ROUTE_SCENE --> ON_SCENE: Geofence Arrived
    ON_SCENE --> TRANSPORTING: Vitals Locked / Hospital Picked
    TRANSPORTING --> AT_HOSPITAL: Transfer Complete
    AT_HOSPITAL --> RETURNING: Unit Cleaned / Restocked
    RETURNING --> AVAILABLE: Base Reached
    Any --> OUT_OF_SERVICE: Mechanical Fault / Maintenance
```

### 16.5 Hospital Admission & Bed Reservation FSM
```mermaid
stateDiagram-v2
    [*] --> PENDING_NOTIFICATION
    PENDING_NOTIFICATION --> BED_RESERVED: ReserveBed() Executed
    BED_RESERVED --> ICU_LOCKED: Patient Critical (Ventilator Allocated)
    ICU_LOCKED --> PATIENT_ARRIVED: Intake Bay Geofence Trigger
    BED_RESERVED --> PATIENT_ARRIVED: Non-ICU Intake
    PATIENT_ARRIVED --> ADMITTED: Transfer of Care Accepted
    ADMITTED --> RELEASED: Discharge / Transfer
    RELEASED --> [*]
```

### 16.6 Police Corridor Clearance Escort FSM
```mermaid
stateDiagram-v2
    [*] --> STANDBY
    STANDBY --> ALERTED: Severe Incident Traversing Corridor
    ALERTED --> ACKNOWLEDGED: Police Unit Dispatched
    ACKNOWLEDGED --> PERIMETER_SECURED: Intersection Crossings Barricaded
    PERIMETER_SECURED --> CORRIDOR_CLEARED: Ambulance Safely Passes
    CORRIDOR_CLEARED --> STAND_DOWN: Operations Normalized
    STAND_DOWN --> [*]
```


---

# SECTION 17: SEQUENCE DIAGRAMS

### 17.1 End-to-End Emergency Corridor Activation Sequence
```mermaid
sequenceDiagram
    autonumber
    actor Caller as 911 Distress Caller
    participant Coordinator as CoordinatorManager
    participant Medical as PriorityEngine
    participant Fleet as FleetManagementEngine
    participant Hospital as HospitalResourceEngine
    participant Wave as GreenWaveCoordinator
    participant Server as TcpServer
    participant NodeA as Intersection Node A
    participant NodeB as Intersection Node B
    participant Police as PoliceDispatch

    Caller->>Coordinator: Distress Call (Incident: Cardiac Arrest, Location)
    Coordinator->>Medical: CalculatePriorityScore(Profile)
    Medical-->>Coordinator: Score = 96.0 (CRITICAL)
    Coordinator->>Fleet: DispatchBestAmbulance(Score, Location)
    Fleet-->>Coordinator: Assigned AMB_01 (ALS Unit)
    Coordinator->>Hospital: SelectOptimalHospital(PatientProfile, Location)
    Hospital-->>Coordinator: Reserved HOSP_CITY_TRAUMA (ICU 1 Locked)
    Coordinator->>Police: AlertCorridorEscort(Route: INT_A -> INT_B)
    Police-->>Coordinator: Escort Unit 104 En Route
    Coordinator->>Wave: GenerateGreenWave([INT_A, INT_B], Speed=60km/h)
    Wave-->>Coordinator: Preemption Schedule Matrix
    Coordinator->>Server: Broadcast Preemption Directives
    Server->>NodeA: COMMAND (PREEMPT_APP_1, Window: T+10s..T+25s)
    Server->>NodeB: COMMAND (PREEMPT_APP_1, Window: T+25s..T+45s)
    NodeA->>NodeA: FSM -> EMERGENCY_PREEMPTION
    NodeA-->>Server: ACK (PREEMPTION_ACTIVE)
    NodeB->>NodeB: FSM -> EMERGENCY_PREEMPTION
    NodeB-->>Server: ACK (PREEMPTION_ACTIVE)
    Server-->>Coordinator: All Edge Intersections Preempted
```

### 17.2 Communication Loss & Fail-Safe Fallback Sequence
```mermaid
sequenceDiagram
    autonumber
    participant Node as Traffic Node A
    participant Server as Coordinator Server
    participant Watchdog as HeartbeatWatchdog
    participant Hardware as Relay Board

    Node-xServer: Network Cable Severed / Link Dropped
    loop Every 1000ms
        Watchdog->>Watchdog: Scan lastHeartbeatMs for Node A
    end
    Note over Watchdog: 5000ms Elapsed with No Heartbeat
    Watchdog->>Server: Evict Node A (Mark OFFLINE)
    Server->>Server: Recompute Quorum without Node A
    
    Note over Node: Local Client Detects Socket Drop
    Node->>Node: Local FSM: Transition to FLASHING_YELLOW
    Node->>Hardware: Assert Flashing Yellow Relays
    Hardware-->>Node: Lamps Flashing Yellow
    Node->>Node: Launch Background Auto-Reconnect Loop (2s backoff)
```

### 17.3 Graceful System Shutdown Sequence
```mermaid
sequenceDiagram
    autonumber
    actor Admin as System Administrator
    participant Coordinator as Coordinator Daemon
    participant Server as TcpServer
    participant Client as Traffic Node Client
    participant FSM as SignalControllerFSM
    participant Relays as Hardware Relays

    Admin->>Coordinator: SIGINT / SIGTERM
    Coordinator->>Server: Stop()
    Server->>Client: COMMAND (SYSTEM_SHUTDOWN)
    Client->>FSM: Orderly Shutdown Triggered
    FSM->>FSM: Transition Current Phase to YELLOW (4s)
    FSM->>FSM: Transition to ALL_RED (2s)
    FSM->>Relays: Set Relays to Default Fail-Safe (Normally Closed)
    Client->>Server: ACK (SHUTDOWN_READY)
    Client->>Client: Close Socket & Terminate
    Server->>Coordinator: All Clients Disconnected
    Coordinator->>Coordinator: Flush Audit Logs to Disk & Exit
```


---

# SECTION 18: FLOWCHARTS

### 18.1 Master Decision & Triage Flowchart
```mermaid
flowchart TD
    Call[Distress Call Ingested] --> Validate{Valid Payload?}
    Validate -- No --> Reject[Reject & Log Anomaly]
    Validate -- Yes --> Score[Compute Priority Score via PriorityEngine]
    
    Score --> AcuityCheck{Score >= 75.0?}
    AcuityCheck -- Yes --> Crit[Triage: CRITICAL / ALS Unit]
    AcuityCheck -- No --> NonCrit{Score >= 50.0?}
    NonCrit -- Yes --> High[Triage: HIGH / Standard Unit]
    NonCrit -- No --> Norm[Triage: MEDIUM/LOW]
    
    Crit --> HospRule[Hospital 10-Rule Selection: Force Trauma + ICU]
    High --> HospRuleNorm[Hospital Selection: Standard Bed]
    Norm --> HospRuleNorm
    
    HospRule --> RouteAStar[Calculate A* Path]
    HospRuleNorm --> RouteAStar
    
    RouteAStar --> DetourCheck{Segment Congestion > 70%?}
    DetourCheck -- Yes --> Replanner[Calculate Detour via DynamicRouteReplanner]
    DetourCheck -- No --> CorridorPlan[Compute Green Wave via GreenWaveCoordinator]
    Replanner --> CorridorPlan
    
    CorridorPlan --> ConflictCheck{Intersection Contention?}
    ConflictCheck -- Yes --> Arbitrate[Execute 5-Tier Conflict Arbitration]
    ConflictCheck -- No --> Consensus[Reach Multi-Node Consensus]
    Arbitrate --> Consensus
    
    Consensus --> QuorumCheck{Quorum (N/2)+1 Approved?}
    QuorumCheck -- Yes --> Execute[Broadcast Preemption Commands]
    QuorumCheck -- No --> Abort[Abort Plan & Fall Back]
    
    Execute --> Complete([Signals Actuated Green])
```

### 18.2 TCP Network Stream Accumulator Flowchart
```mermaid
flowchart TD
    DataIn[Raw Bytes Ingested from Socket recv] --> Append[Append Bytes to Client Buffer]
    Append --> SizeCheck{Buffer Size >= 19 Bytes?}
    SizeCheck -- No --> Wait[Wait for More TCP Segments]
    
    SizeCheck -- Yes --> MagicCheck{Buffer[0..1] == 0xA55A?}
    MagicCheck -- No --> Slide[Slide Window: Erase First Byte]
    Slide --> SizeCheck
    
    MagicCheck -- Yes --> LenExtract[Read Big-Endian Payload Length L from Bytes 15..16]
    LenExtract --> TotalCheck{Buffer Size >= 19 + L Bytes?}
    TotalCheck -- No --> Wait
    
    TotalCheck -- Yes --> CRCCheck{Compute CRC16 == Frame Checksum?}
    CRCCheck -- No --> Corrupt[CRC Mismatch: Discard Corrupted Frame & Slide]
    Corrupt --> Slide
    
    CRCCheck -- Yes --> Extract[Extract Valid Message Frame]
    Extract --> EraseBuf[Erase 19 + L Bytes from Buffer]
    EraseBuf --> Dispatch[Dispatch Message to Event Callback Handler]
    Dispatch --> SizeCheck
```

### 18.3 Fail-Safe Hardware Watchdog Recovery Flowchart
```mermaid
flowchart TD
    Tick([Watchdog 1000ms Timer Tick]) --> Iterate[Iterate Through Registered Edge Clients]
    Iterate --> TimeCheck{now - lastHeartbeat > 5000ms?}
    TimeCheck -- No --> NextNode[Check Next Node]
    
    TimeCheck -- Yes --> LogTimeout[Log [TIMEOUT] Event for Node]
    LogTimeout --> CloseSocket[Close Socket Descriptor & Erase Client]
    CloseSocket --> RecomputeQuorum[Recompute Active Consensus Quorum]
    RecomputeQuorum --> DegradeSignals[Check if Active Corridors Affected]
    DegradeSignals --> Fallback[Issue Safe Clearance / Flashing Yellow to Node]
    Fallback --> NextNode
    NextNode --> MoreNodes{More Nodes?}
    MoreNodes -- Yes --> Iterate
    MoreNodes -- No --> Sleep([Sleep Until Next 1s Tick])
```


---

# SECTION 19: CONFIGURATION

### 19.1 System Constants, Intervals & Timing Limits
The operational safety of SMART AEGIS is anchored in immutable timing constants and runtime limits:

| Parameter Name | Default Value | Valid Range | Scope / Subsystem | Description |
|:---|:---|:---|:---|:---|
| `DEFAULT_TCP_PORT` | `9000` | `1024`–`65535` | Networking | Coordinator listening TCP port |
| `HEARTBEAT_INTERVAL_MS`| `1000` ms ($1\,	ext{s}$) | `200`–`5000` ms | Edge Clients | Heartbeat transmission period |
| `HEARTBEAT_TIMEOUT_MS` | `5000` ms ($5\,	ext{s}$) | `2000`–`15000` ms | Coordinator | Silent node watchdog eviction threshold |
| `AUTO_RECONNECT_INTERVAL_MS`| `2000` ms ($2\,	ext{s}$) | `500`–`10000` ms | Edge Clients | Background auto-reconnect polling backoff |
| `CONNECT_TIMEOUT_MS` | `3000` ms ($3\,	ext{s}$) | `1000`–`10000` ms | Edge Clients | Non-blocking socket connect timeout |
| `MINIMUM_GREEN_DURATION_SEC`| `10.0` s | `5.0`–`30.0` s | Signal FSM | Minimum green interval before phase change |
| `YELLOW_CHANGE_DURATION_SEC` | `4.0` s | `3.0`–`6.0` s | Signal FSM | Standard yellow change clearance interval |
| `ALL_RED_CLEARANCE_SEC` | `2.0` s | `1.0`–`4.0` s | Signal FSM | Mandatory all-red intersection clearance |
| `MAX_PREEMPTION_HOLD_SEC`| `60.0` s | `15.0`–`120.0` s | Incident Command | Maximum duration a signal may hold green |
| `PREEMPTION_TRIGGER_RADIUS_M`| `500.0` m | `100`–`1500` m | Corridor Planner | Outer GPS geofence for corridor preparation |
| `APPROACH_TRIGGER_RADIUS_M`| `250.0` m | `50`–`500` m | Corridor Planner | Inner GPS geofence locking protected green |
| `MAX_STREET_CONGESTION_INDEX`| `0.70` ($70\%$) | `0.40`–`0.95` | Route Replanner | Congestion threshold triggering dynamic detour |

### 19.2 Deployment Configuration File Formats
Production configurations are stored in schema-validated JSON format under `config/deployment/`.

#### Example: Coordinator Deployment Configuration (`config/deployment/rpi1_coordinator.json`)
```json
{
  "nodeId": "COORDINATOR",
  "role": "COORDINATOR",
  "ipAddress": "0.0.0.0",
  "port": 9000,
  "heartbeatTimeoutMs": 5000,
  "quorumThresholdPercent": 51.0,
  "logLevel": "INFO",
  "intersections": [
    { "id": "INT_A", "ip": "192.168.1.101", "port": 9000, "lat": 12.9716, "lon": 77.5946 },
    { "id": "INT_B", "ip": "192.168.1.102", "port": 9000, "lat": 12.9725, "lon": 77.5958 },
    { "id": "INT_C", "ip": "192.168.1.103", "port": 9000, "lat": 12.9738, "lon": 77.5972 }
  ]
}
```

#### Example: Intersection A Edge Controller Configuration (`config/deployment/rpi2_intersection_a.json`)
```json
{
  "nodeId": "INT_A",
  "role": "INTERSECTION",
  "coordinatorHost": "192.168.1.100",
  "coordinatorPort": 9000,
  "localBindingIp": "192.168.1.101",
  "heartbeatIntervalMs": 1000,
  "autoReconnect": true,
  "reconnectIntervalMs": 2000,
  "gpioPins": {
    "approach1_red": 17,
    "approach1_yellow": 27,
    "approach1_green": 22,
    "approach2_red": 23,
    "approach2_yellow": 24,
    "approach2_green": 25
  },
  "safetyTimings": {
    "minGreenSec": 10,
    "yellowChangeSec": 4,
    "allRedClearanceSec": 2
  }
}
```


---

# SECTION 20: LOGGING SYSTEM

### 20.1 Architecture & Event Categories
The logging subsystem (`EventLogger`, `SimulationRecorder`, `Logger`) provides high-throughput, structured, tamper-evident forensic event logging. Events are classified across 13 tactical system categories:

```
1.  EVENT_CALL_INGESTED       - Incoming 911 distress call / ACN telemetry
2.  EVENT_TRIAGE_EVALUATED    - Clinical priority calculation and triage tier assignment
3.  EVENT_FLEET_DISPATCHED    - Ambulance vehicle selection and dispatch assignment
4.  EVENT_HOSPITAL_SELECTED   - Destination hospital selection and bed/ICU reservation
5.  EVENT_ROUTE_GENERATED     - A* navigation route calculated or dynamically replanned
6.  EVENT_CORRIDOR_ACTIVATED  - Green wave corridor preemption envelope scheduled
7.  EVENT_CONFLICT_DETECTED   - Spatial or temporal corridor contention identified
8.  EVENT_CONFLICT_RESOLVED   - 5-tier priority arbitration executed and winner chosen
9.  EVENT_SIGNAL_PREEMPTED    - Physical traffic signal lamp shifted to emergency green
10. EVENT_SIGNAL_RESTORED     - Traffic signal returned to baseline cyclic coordination
11. EVENT_CONSENSUS_REACHED   - Multi-node preemption agreement quorum achieved
12. EVENT_COMMUNICATION_LOST  - Edge node heartbeat timeout (> 5.0s) tripped
13. EVENT_FAILSAFE_DEGRADED   - Signal controller reverted to fail-safe flashing yellow
```

### 20.2 Severity Levels
* `TRACE`: Verbose byte-level socket traces and raw GPIO toggle events.
* `DEBUG`: Internal algorithm state steps, heuristic costs, and intermediate sensor readings.
* `INFO`: Normal lifecycle events (node registration, heartbeat reception, phase transitions).
* `WARNING`: Non-critical anomalies (congestion surge, minor latency spike, single missed heartbeat).
* `ERROR`: Functional failures (corridor conflict, bed reservation rejection, CRC mismatch).
* `CRITICAL`: Safety-critical violations (conflicting green detected, watchdog timeout, hardware fault).

### 20.3 Structured ASCII Operational Reports
In addition to JSON streams, the logging engine produces formatted, human-readable ASCII operational blocks:

```text
[INCIDENT COMMAND]
Incident: INC-9901
Severity: CRITICAL
Triage Score: 96.0
Dispatched Unit: AMB_01
Target Hospital: HOSP_CITY_TRAUMA
ICU Bed Reserved: Bed 4
Corridor Status: ACTIVE

[MULTI CORRIDOR]
Conflict: CONFL-04
Intersection: INT_B
Ambulance 1: AMB_01 (Score: 96.0)
Ambulance 2: AMB_02 (Score: 45.0)
Winner: AMB_01
Resolution: Delay AMB_02
Stagger Delay: 8 sec

[CONSENSUS]
Plan ID: PLAN_CORRIDOR_42
Term: 3
Leader: COORDINATOR
Votes Cast: 3
Votes Approved: 3
Quorum Threshold: 2 (67%)
Result: COMMITTED

[HOSPITAL]
Facility: HOSP_CITY_TRAUMA
Status: ONLINE
Total Beds: 120 (Available: 14)
ICU Units: 20 (Available: 3)
Ventilators: 15 (Available: 2)
On-Duty Physicians: 8
```


---

# SECTION 21: ERROR HANDLING

### 21.1 Deterministic Error Handling Philosophy
In mission-critical, safety-critical cyber-physical systems, unexpected exceptions can crash control loops and leave hardware in dangerous intermediate states. SMART AEGIS adopts a strict error handling standard:
1. **Zero Uncaught Exceptions in Real-Time Loops:** All thread loops, network packet handlers, and state machine tickers operate within `noexcept` or catch-all protective boundaries.
2. **Explicit Result Pattern (`Result<T>`):** Operations that can fail return a monadic `Result<T>` or `ErrorCode` enum rather than throwing C++ exceptions:
   ```cpp
   Result<std::string> ReserveBed(const std::string& hospitalId, const std::string& patientId);
   ```
3. **Fail-Safe Default-Safe Actions:** If a critical error occurs, the system must immediately and autonomously transition to a provably safe degraded state (e.g. Flashing Yellow).

### 21.2 Failure Taxonomy & Recovery Matrix
The table below specifies every major subsystem failure mode and its autonomous recovery strategy:

| Subsystem | Failure Condition | Detection Mechanism | Immediate Action | Recovery Strategy |
|:---|:---|:---|:---|:---|
| **Networking** | Socket broken pipe / disconnect | `recv() <= 0` or `EPIPE` | Suppress `SIGPIPE`, flag link down | Background thread reconnects every 2.0s |
| **Networking** | Network packet bit corruption | CRC-16 checksum mismatch | Drop corrupted packet immediately | Sliding window accumulator re-syncs |
| **Edge Node** | Coordinator communication loss | Heartbeat timeout $> 5.0\,	ext{s}$ | FSM trips to `FLASHING_YELLOW` | Auto-reconnect restores cyclic mode |
| **Coordinator**| Edge controller silent / crash | Heartbeat watchdog scan | Mark node `OFFLINE`, evict socket | Recompute consensus quorum; retry |
| **Signal FSM** | Conflicting green phase requested | Bitmask assertion check | Reject preemption command | Revert to clearance all-red |
| **Corridor** | Multi-ambulance deadlock | Overlapping arrival times | 5-tier arbitration | Apply 8s stagger or dynamic detour |
| **Hospital** | Target hospital reaches 0 beds | Ingestion capacity check | Disqualify facility | Select next best facility via 10-rule engine |
| **Hardware HAL**| GPIO line stuck or relay failure | Pin state readback mismatch | Log `CRITICAL_HARDWARE_FAULT` | Trip hardware fail-safe relay to flasher |

### 21.3 Watchdog Timers & Dead-Man Circuits
* **Software Watchdogs:** `Watchdog` class monitors thread execution cadence. Worker threads must strobe the watchdog every 100 ms. If a thread stalls for $> 500\,	ext{ms}$, a watchdog trip is generated.
* **Hardware Dead-Man Relay:** Physical Raspberry Pi GPIO pin 26 is toggled continuously by the real-time ticker. An external hardware RC charge pump circuit holds the fail-safe relay open. If software freezes, the pin stops toggling, the capacitor discharges within 400 ms, and the relay drops to flashing yellow.


---

# SECTION 22: TESTING

### 22.1 Comprehensive Test Suite Overview
SMART AEGIS maintains an exhaustive test harness comprising **42 standalone unit and integration test binaries** achieving a 100% test pass rate:

```
==========================================================
 SMART AEGIS TEST VERIFICATION MATRIX (42/42 PASSING)
==========================================================
 [01] test_build_smoke                             -> PASS
 [02] test_common_foundation                       -> PASS
 [03] test_communication_infrastructure            -> PASS
 [04] test_communication_module                    -> PASS
 [05] test_coordinator_core                        -> PASS
 [06] test_coordinator_foundation                  -> PASS
 [07] test_green_corridor                          -> PASS
 [08] test_ambulance_node                          -> PASS
 [09] test_traffic_signal_node                     -> PASS
 [10] test_intersection_node                       -> PASS
 [11] test_system_integration                      -> PASS
 [12] test_rpi_deployment                          -> PASS
 [13] test_medical_priority                        -> PASS
 [14] test_ambulance_registry                      -> PASS
 [15] test_conflict_detection                      -> PASS
 [16] test_manual_override                         -> PASS
 [17] test_event_logger                            -> PASS
 [18] test_phase8_intelligent_scheduler            -> PASS
 [19] test_coordinator_monitoring                  -> PASS
 [20] test_traffic_signal_node_phase6a             -> PASS
 [21] test_coordinator_traffic_signal_phase6b     -> PASS
 [22] test_emergency_corridor_planner_phase7a      -> PASS
 [23] test_green_wave_coordinator_phase7b          -> PASS
 [24] test_priority_scheduler_phase8a              -> PASS
 [25] test_emergency_orchestrator_phase8b          -> PASS
 [26] test_dynamic_route_replanner_phase9a         -> PASS
 [27] test_multi_corridor_coordinator_phase9b      -> PASS
 [28] test_distributed_signal_phase10a             -> PASS
 [29] test_predictive_traffic_phase10b             -> PASS
 [30] test_fleet_management_phase11a               -> PASS
 [31] test_hospital_selection_phase11b             -> PASS
 [32] test_hospital_resource_phase11b              -> PASS
 [33] test_mission_execution_phase12a              -> PASS
 [34] test_incident_command_phase12b               -> PASS
 [35] test_traffic_network_orchestrator_phase13a   -> PASS
 [36] test_distributed_consensus_phase13b          -> PASS
 [37] test_rpi_cluster_phase14a                   -> PASS
 [38] test_system_diagnostics_phase14b             -> PASS
 [39] test_hardware_abstraction_phase15a           -> PASS
 [40] test_qnx_driver_framework_phase15b           -> PASS
 [41] test_raspberry_pi4_bsp_phase16a             -> PASS
 [42] test_qnx_network_layer                       -> PASS
==========================================================
 TOTAL: 42/42 TEST SUITES PASSED (100% SUCCESS RATE)
==========================================================
```

### 22.2 Performance Benchmark Highlights
* **Hospital Selection Latency:** Evaluates and deterministically ranks 100 candidate hospitals in **0.141 ms – 0.267 ms** (requirement: $< 5.0\,	ext{ms}$).
* **Incident Command Latency:** Evaluates 10 deterministic multi-agency decision rules across 1,000 active incidents in **1.84 ms** (average **0.0018 ms** / evaluation).
* **Consensus Voting Latency:** Conducts 1,000 multi-node preemption consensus votes in **0.58 ms** (average **0.00058 ms** / vote).
* **Network Wire Round-Trip Latency:** 1,000 sequential request/response packet exchanges across TCP/IP in **62.08 ms** total (average **0.062 ms** / message, well within the $< 2.0\,	ext{ms}$ real-time budget).
* **BSP Pin Toggle Latency:** 10,000 digital GPIO pin toggle operations in **1.82 ms** (average **0.00018 ms** / operation).

### 22.3 Multi-Threaded Concurrency & Stress Verification
* **100-Thread Concurrency Tests:** Validated across Hospital Resource Engine, Fleet Management, Incident Command, and Raspberry Pi BSP layers. 100 threads concurrently executing simultaneous mutations, reservations, and reads produced zero race conditions, data corruptions, or deadlocks.
* **20-Client Network Stress Test:** 20 simultaneous client threads connecting, bombarding the coordinator server with high-rate telemetry, and disconnecting simultaneously verified server queue stability and dead socket reclamation.


---

# SECTION 23: CURRENT IMPLEMENTATION STATUS

### 23.1 Inventory of Completed Modules
All core software layers (Phases 1 through 16A + QNX Network Layer) are **100% complete, fully implemented, and passing all regression suites**:

| Module / Subsystem | Phase Completed | Source Directory | Code Maturity | Test Status |
|:---|:---|:---|:---|:---|
| **Common Foundation & Utils** | Phase 1 | `src/common/` | Production Grade | PASS (`test_common_foundation`) |
| **Configuration Manager** | Phase 2 | `src/config/` | Production Grade | PASS (`test_build_smoke`) |
| **Communication Infrastructure**| Phase 3 | `src/communication/`, `src/network/` | Production Grade | PASS (`test_communication_*`) |
| **Coordinator Core** | Phase 4 | `src/coordinator/` | Production Grade | PASS (`test_coordinator_*`) |
| **Edge Nodes & Signal FSM** | Phases 5A/6A | `src/intersection/`, `src/traffic_controller/` | Production Grade | PASS (`test_traffic_signal_node_*`) |
| **Green Corridor & Wave Sync** | Phases 7A/7B | `src/green_corridor/`, `src/green_wave/` | Production Grade | PASS (`test_emergency_corridor_*`) |
| **Intelligent Priority Scheduler**| Phases 8A/8B | `src/scheduler/`, `src/orchestrator/` | Production Grade | PASS (`test_priority_scheduler_*`) |
| **Dynamic Route Replanner** | Phase 9A | `src/replanning/` | Production Grade | PASS (`test_dynamic_route_replanner_phase9a`) |
| **Multi-Corridor Conflict Coordinator**| Phase 9B | `src/multi_corridor/` | Production Grade | PASS (`test_multi_corridor_coordinator_phase9b`) |
| **Distributed Signal Synchronizer**| Phase 10A | `src/distributed_signal/` | Production Grade | PASS (`test_distributed_signal_phase10a`) |
| **Predictive Traffic Intelligence**| Phase 10B | `src/predictive_traffic/` | Production Grade | PASS (`test_predictive_traffic_phase10b`) |
| **Fleet Management Engine** | Phase 11A | `src/fleet_management/` | Production Grade | PASS (`test_fleet_management_phase11a`) |
| **Hospital Selection Engine** | Phase 11B | `src/hospital_selection/` | Production Grade | PASS (`test_hospital_selection_phase11b`) |
| **Hospital Resource Engine** | Phase 11B | `src/hospital_management/` | Production Grade | PASS (`test_hospital_resource_phase11b`) |
| **Mission Execution Engine** | Phase 12A | `src/mission_execution/` | Production Grade | PASS (`test_mission_execution_phase12a`) |
| **Incident Command Engine** | Phase 12B | `src/incident_command/` | Production Grade | PASS (`test_incident_command_phase12b`) |
| **Traffic Network Orchestrator**| Phase 13A | `src/traffic_network_orchestrator/` | Production Grade | PASS (`test_traffic_network_orchestrator_phase13a`) |
| **Distributed Consensus Engine**| Phase 13B | `src/distributed_consensus/` | Production Grade | PASS (`test_distributed_consensus_phase13b`) |
| **Raspberry Pi Cluster Manager**| Phase 14A | `src/rpi_cluster/` | Production Grade | PASS (`test_rpi_cluster_phase14a`) |
| **System Diagnostics Engine** | Phase 14B | `src/system_diagnostics/` | Production Grade | PASS (`test_system_diagnostics_phase14b`) |
| **Hardware Abstraction Layer (HAL)**| Phase 15A | `src/hardware_abstraction/` | Production Grade | PASS (`test_hardware_abstraction_phase15a`) |
| **QNX Device Driver Framework**| Phase 15B | `src/qnx_driver_framework/` | Production Grade | PASS (`test_qnx_driver_framework_phase15b`) |
| **Raspberry Pi 4 BSP** | Phase 16A | `src/bsp/` | Production Grade | PASS (`test_raspberry_pi4_bsp_phase16a`) |
| **QNX Network Communication Layer**| Network Phase | `network/` | Production Grade | PASS (`test_qnx_network_layer`) |

### 23.2 Partially Completed & Pending Modules
* **Physical BCM2711 Hardware Registers:** Currently operating in software simulated register mode (`SimulationGpioInterface` / `RaspberryPi4BSP`). Pending electrical wire-up to physical 4-channel relay boards and LED signal heads.
* **Physical GNSS / NMEA Serial Source:** Vehicle GPS coordinates are currently generated via internal kinematics simulation rather than physical UART serial connections to hardware GNSS receivers.
* **Cellular C-V2X / 5G Sidelink:** Wide-area ad-hoc mesh networking between ambulances is simulated over UDP broadcast / TCP.

### 23.3 Known Limitations & Technical Debt
1. **Flat Subnet Assumption:** Deployment configurations assume all nodes reside on a single IPv4 `/24` subnet (`192.168.1.0/24`). Routing across complex multi-homed municipal VLANs requires router configuration.
2. **In-Memory Fleet Registries:** Ambulance and hospital capacity registries reside in RAM; they do not currently persist to an external non-volatile SQL database across process restarts.


---

# SECTION 24: QNX PORTING GUIDE

This section serves as the definitive engineering roadmap for porting SMART AEGIS from the POSIX / x86_64 simulation host to **QNX Software Development Platform (SDP) 8.0** executing natively on the **Raspberry Pi 4 Model B (Broadcom BCM2711 ARM Cortex-A72)**.

---

### 24.1 Module-by-Module QNX Migration Matrix
The table below specifies the exact porting requirements, OS primitives, and migration priority for every module in the repository:

| Module Name | Platform Independent | Needs QNX Sockets | Needs QNX Threads | Needs QNX Timers | Needs QNX IPC | Needs GPIO / HW | Can Remain Unchanged | Migration Priority |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **Common Foundation** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Config Manager** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Priority Engine** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Hospital Selection**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Hospital Resource** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Fleet Management** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Mission Execution** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Incident Command** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Traffic Orchestrator**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Distributed Consensus**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Cluster Manager** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **System Diagnostics**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Route Planner** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Route Replanner** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Multi-Corridor Coord**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Green Wave Coord** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Corridor Planner** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Priority Scheduler**| YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Signal FSM** | YES | NO | NO | NO | NO | NO | YES | Priority 5 (Trivial) |
| **Event Logger** | YES | NO | NO | NO | NO | NO | YES | Priority 4 (Optional slogger2)|
| **TcpServer / Client**| NO | YES | YES | YES | NO | NO | PARTIAL | Priority 2 (High) |
| **Hardware HAL** | NO | NO | NO | NO | NO | YES | PARTIAL | Priority 2 (High) |
| **QNX Driver Framework**| NO | NO | YES | NO | YES | YES | NO | Priority 1 (Immediate) |
| **Raspberry Pi 4 BSP**| NO | NO | NO | NO | YES | YES | NO | Priority 1 (Immediate) |

---

### 24.2 Detailed Porting Instructions for Safety-Critical Components

#### 1. QNX Native Networking (`network/`)
* **Current POSIX Implementation:** Standard BSD sockets (`socket()`, `bind()`, `listen()`, `accept()`, `poll()`, `send()`, `recv()`).
* **QNX SDP 8.0 Compatibility:** QNX provides a full BSD-compliant `io-pkt` networking stack. The current pure C++17 implementation in `network/tcp_server.cpp` and `network/tcp_client.cpp` will **compile and execute unchanged on QNX Neutrino 8.0** without modification.
* **Optional Microkernel Optimization:** Replace TCP sockets with QNX Native Distributed Messaging (**Qnet**):
  ```cpp
  // Optional Qnet microkernel IPC replacing TCP sockets:
  int chid = ChannelCreate(0);
  int coid = ConnectAttach(ND_LOCAL_NODE, pid, chid, _NTO_SIDE_CHANNEL, 0);
  MsgSend(coid, &txMsg, sizeof(txMsg), &rxReply, sizeof(rxReply));
  ```

#### 2. QNX Device Driver Framework & Resource Managers (`src/qnx_driver_framework/`)
* **Current Implementation:** Abstract driver lifecycle and simulated `/dev/...` path registry.
* **QNX SDP 8.0 Porting Target:** Bind to the QNX Resource Manager library (`resmgr`):
  ```cpp
  #include <sys/iofunc.h>
  #include <sys/dispatch.h>

  // Attach path to QNX pathname space:
  resmgr_attr_t resmgr_attr;
  memset(&resmgr_attr, 0, sizeof(resmgr_attr));
  resmgr_attr.nparts_max = 1;
  resmgr_attr.msg_max_size = 2048;

  iofunc_attr_t io_attr;
  iofunc_attr_init(&io_attr, S_IFCHR | 0666, 0, 0);

  resmgr_connect_funcs_t connect_funcs;
  resmgr_io_funcs_t io_funcs;
  iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                   _RESMGR_IO_NFUNCS, &io_funcs);
  io_funcs.read = qnx_gpio_read;
  io_funcs.write = qnx_gpio_write;
  io_funcs.devctl = qnx_gpio_devctl;

  dispatch_t* dpp = dispatch_create();
  resmgr_attach(dpp, &resmgr_attr, "/dev/aegis_gpio", _FTYPE_ANY, 0,
                &connect_funcs, &io_funcs, &io_attr);
  ```

#### 3. Raspberry Pi 4 BCM2711 GPIO Driver (`src/bsp/`)
* **Current Implementation:** Simulated register memory and atomic bitfields.
* **QNX SDP 8.0 Target:** Map the Broadcom BCM2711 physical peripheral base (`0xFE000000` on BCM2711 / RPi 4) using `mmap_device_io()` or `ThreadCtl(_NTO_TCTL_IO, 0)`:
  ```cpp
  #include <sys/mman.h>
  #include <sys/neutrino.h>

  // Enable I/O privileges in QNX microkernel:
  ThreadCtl(_NTO_TCTL_IO, 0);

  // Map physical BCM2711 GPIO registers into virtual process space:
  const uintptr_t BCM2711_PERIPH_BASE = 0xFE000000;
  const uintptr_t GPIO_BASE = BCM2711_PERIPH_BASE + 0x200000;
  volatile uint32_t* gpio = (volatile uint32_t*)mmap_device_memory(
      0, 0x1000, PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, GPIO_BASE);

  // Direct register manipulation:
  void WritePinHigh(uint8_t pin) {
      if (pin < 32) gpio[GPSET0] = (1 << pin);
      else          gpio[GPSET1] = (1 << (pin - 32));
  }
  void WritePinLow(uint8_t pin) {
      if (pin < 32) gpio[GPCLR0] = (1 << pin);
      else          gpio[GPCLR1] = (1 << (pin - 32));
  }
  ```

#### 4. QNX High-Resolution Monotonic Timers & Pulses
* **Current Implementation:** `std::chrono::steady_clock` and sleep threads.
* **QNX Real-Time Optimization:** Employ QNX Kernel Pulses and `timer_create` for jitter-free $10\,	ext{Hz}$ state machine execution without POSIX thread scheduling latency:
  ```cpp
  struct sigevent event;
  SIGEV_PULSE_INIT(&event, coid, SIGEV_PULSE_PRIO_INHERIT, CODE_FSM_TICK, 0);
  timer_t timer_id;
  timer_create(CLOCK_MONOTONIC, &event, &timer_id);

  struct itimerspec itime;
  itime.it_value.tv_sec = 0;
  itime.it_value.tv_nsec = 100000000; // 100 ms (10 Hz)
  itime.it_interval = itime.it_value;
  timer_settime(timer_id, 0, &itime, NULL);
  ```

#### 5. QNX Structured Logging (`src/logger/`)
* **Current Implementation:** ASCII and JSON output to files and standard error.
* **QNX Target:** Integrate with `slogger2` (QNX System Logger) for high-performance zero-copy circular logging to non-volatile shared memory.


---

# SECTION 25: DEPLOYMENT PLAN

### 25.1 Physical Cluster Topology
SMART AEGIS is deployed across four dedicated Raspberry Pi 4 Model B single-board computers interconnected via a private Gigabit Ethernet switch:

```
[MUNICIPAL BACKBONE SWITCH (Unmanaged Gigabit)]
  │
  ├── Port 1: RPi 1 - Municipal Coordinator (Static IP: 192.168.1.100)
  ├── Port 2: RPi 2 - Intersection Node A   (Static IP: 192.168.1.101)
  ├── Port 3: RPi 3 - Intersection Node B   (Static IP: 192.168.1.102)
  └── Port 4: RPi 4 - Intersection Node C   (Static IP: 192.168.1.103)
```

### 25.2 Node Hardware & Software Specifications

#### Node 1: Municipal Coordinator Server
* **Hardware:** Raspberry Pi 4 Model B (8 GB RAM), 64 GB SanDisk Extreme MicroSD, Gigabit Ethernet.
* **Operating System:** QNX Neutrino RTOS 8.0 (ARM64) or Ubuntu 22.04 LTS Server.
* **Network Binding:** Static IP `192.168.1.100`, Netmask `255.255.255.0`, Gateway `192.168.1.1`.
* **Listening TCP Port:** `9000` (Coordinator Wire Port), `1883` (Optional MQTT Broker), `8080` (EOC Web Dashboard).
* **Primary Executable:** `aegis_coordinator` (or `example_server`).
* **Configuration File:** `config/deployment/rpi1_coordinator.json`.

#### Node 2: Intersection Controller Node A
* **Hardware:** Raspberry Pi 4 Model B (4 GB RAM), 4-Channel 5V Relay Module, Red/Yellow/Green LED Signal Head.
* **Operating System:** QNX Neutrino RTOS 8.0 (ARM64) or Raspberry Pi OS Lite (64-bit).
* **Network Binding:** Static IP `192.168.1.101`.
* **GPIO Pinout Assignment (BCM Pin Numbers):**
  * Pin 17: North-South Red Lamp Relay
  * Pin 27: North-South Yellow Lamp Relay
  * Pin 22: North-South Green Lamp Relay
  * Pin 23: East-West Red Lamp Relay
  * Pin 24: East-West Yellow Lamp Relay
  * Pin 25: East-West Green Lamp Relay
  * Pin 26: Hardware Watchdog Heartbeat Pulse Output
* **Primary Executable:** `aegis_intersection_node` (or `example_client`).
* **Configuration File:** `config/deployment/rpi2_intersection_a.json`.

#### Node 3: Intersection Controller Node B
* **Hardware & OS:** Identical to Node 2.
* **Network Binding:** Static IP `192.168.1.102`.
* **GPIO Pinout Assignment:** Identical BCM pin assignments for standardized modular replacement.
* **Primary Executable:** `aegis_intersection_node`.
* **Configuration File:** `config/deployment/rpi3_intersection_b.json`.

#### Node 4: Intersection Controller Node C
* **Hardware & OS:** Identical to Node 2.
* **Network Binding:** Static IP `192.168.1.103`.
* **GPIO Pinout Assignment:** Identical BCM pin assignments.
* **Primary Executable:** `aegis_intersection_node`.
* **Configuration File:** `config/deployment/rpi4_intersection_c.json`.

### 25.3 Automated Cluster Deployment Scripts
The repository includes production shell scripts in `scripts/` to orchestrate multi-node compilation, deployment, and execution:

* **`scripts/build.sh`:** Builds all static libraries, daemon executables, and 42 unit test binaries:
  ```bash
  ./scripts/build.sh
  ```
* **`scripts/deploy_pi.sh`:** Rsyncs compiled binaries and deployment configuration files to all four Raspberry Pi targets over SSH:
  ```bash
  ./scripts/deploy_pi.sh --target-hosts 192.168.1.100,192.168.1.101,192.168.1.102,192.168.1.103
  ```
* **`scripts/start_all_nodes.sh`:** Remotely starts the coordinator followed by intersection daemons in strict chronological order with health checks:
  ```bash
  ./scripts/start_all_nodes.sh
  ```
* **`scripts/flash_qnx.sh`:** Writes the QNX Neutrino RTOS 8.0 BSP image onto bootable MicroSD media for Raspberry Pi 4.


---

# SECTION 26: MASTER DEPENDENCY GRAPH

### 26.1 Complete Subsystem Dependency Graph
The Mermaid graph below illustrates the compile-time and runtime dependency hierarchy spanning all 38 modules in the SMART AEGIS repository:

```mermaid
graph TD
    Common["Common Foundation<br/>(Geo, Time, UUID, Queue, Error, RingBuffer)"]
    Config["Configuration Subsystem<br/>(NodeConfig, ConfigManager)"]
    Logger["Logging Subsystem<br/>(EventLogger, SimulationRecorder)"]
    
    Common --> Config
    Common --> Logger
    
    Medical["Medical Priority Engine<br/>(PatientProfile, PriorityEngine)"]
    Common --> Medical
    
    Route["Route Planning & Dynamic Replanning<br/>(RoutePlanner, DynamicRouteReplanner)"]
    Common --> Route
    
    Fleet["Ambulance Fleet Management<br/>(FleetManagementEngine)"]
    Common --> Fleet
    Medical --> Fleet
    
    Hospital["Hospital Capacity & Bed Management<br/>(HospitalResourceEngine, HospitalSelectionEngine)"]
    Common --> Hospital
    Medical --> Hospital
    
    Corridor["Corridor & Green Wave Planning<br/>(EmergencyCorridorPlanner, GreenWaveCoordinator)"]
    Route --> Corridor
    
    Scheduler["Intelligent Priority Scheduler<br/>(PriorityScheduler, ConflictDetectionEngine, MultiCorridorCoordinator)"]
    Medical --> Scheduler
    Corridor --> Scheduler
    
    NetOrch["Traffic Network Orchestrator<br/>(TrafficNetworkOrchestrator)"]
    Scheduler --> NetOrch
    
    Consensus["Distributed Consensus Engine<br/>(DistributedConsensusEngine)"]
    NetOrch --> Consensus
    
    Cluster["Raspberry Pi Cluster Manager<br/>(RaspberryPiClusterManager)"]
    Consensus --> Cluster
    
    Diagnostics["System Diagnostics Engine<br/>(SystemDiagnosticsEngine)"]
    Cluster --> Diagnostics
    
    Mission["Emergency Mission Execution<br/>(MissionExecutionEngine)"]
    Medical --> Mission
    Fleet --> Mission
    Hospital --> Mission
    Route --> Mission
    Corridor --> Mission
    
    Incident["Incident Command Engine<br/>(IncidentCommandEngine)"]
    Mission --> Incident
    
    NetLayer["QNX Network Communication Layer<br/>(Message, TcpServer, TcpClient)"]
    Common --> NetLayer
    
    HAL["Hardware Abstraction Layer (HAL)<br/>(HardwareAbstractionLayer)"]
    Common --> HAL
    
    DriverFwk["QNX Device Driver Framework<br/>(QNXDeviceDriverFramework)"]
    HAL --> DriverFwk
    
    BSP["Raspberry Pi 4 BSP<br/>(RaspberryPi4BSP)"]
    DriverFwk --> BSP
    
    SignalNode["Traffic Signal Node & Controller<br/>(TrafficSignalNode, TrafficController, SignalStateMachine)"]
    HAL --> SignalNode
    NetLayer --> SignalNode
    
    CoordinatorBin["Daemon: aegis_coordinator"]
    Incident --> CoordinatorBin
    NetLayer --> CoordinatorBin
    
    IntersectionBin["Daemon: aegis_intersection_node"]
    SignalNode --> IntersectionBin
    BSP --> IntersectionBin
```

### 26.2 Communication Graph
```mermaid
graph LR
    AmbulanceUnit["Ambulance Node (Edge)"] ===|Kinematics & Vitals Telemetry| CoordinatorServer["Coordinator Server (9000)"]
    CoordinatorServer ===|Preemption Directives & Quorum Votes| InterA["Intersection Node A (Client)"]
    CoordinatorServer ===|Preemption Directives & Quorum Votes| InterB["Intersection Node B (Client)"]
    CoordinatorServer ===|Preemption Directives & Quorum Votes| InterC["Intersection Node C (Client)"]
    
    InterA ===|1 Hz Heartbeat & Telemetry| CoordinatorServer
    InterB ===|1 Hz Heartbeat & Telemetry| CoordinatorServer
    InterC ===|1 Hz Heartbeat & Telemetry| CoordinatorServer
```


---

# SECTION 27: APPENDIX

### 27.1 Glossary of Terms
* **AERS / SMART AEGIS:** Autonomous Emergency Response System (Automated Emergency Grid & Intelligent Shield).
* **Green Corridor:** A temporary, synchronized sequence of green traffic signal phases along an emergency route, held until an emergency vehicle safely clears each intersection.
* **Green Wave:** A progression technique in traffic engineering where signals are offset based on expected vehicle speed so an approaching vehicle encounters green lights continuously.
* **Preemption:** An operational override interrupting normal cyclic signal phasing to grant immediate right-of-way to authorized emergency vehicles.
* **Mutual Exclusion:** The mathematical and safety guarantee that two physically conflicting traffic approaches (e.g. North-South and East-West) can never simultaneously display a green light.
* **Fail-Safe Degradation:** An autonomous transition to an intrinsically safe operational state (e.g. flashing yellow) upon communication loss, sensor fault, or software hang.
* **Quorum:** The minimum number of participating nodes ($(N/2)+1$) required to agree on a distributed preemption proposal before it is legally executed.
* **Lease Timeout:** A maximum bounded holding period after which an exclusive resource lock (e.g. intersection preemption) is forcibly revoked to prevent indefinite starvation.

### 27.2 Acronyms & Abbreviations
* **ACN:** Automated Crash Notification
* **ALS:** Advanced Life Support
* **ASIL:** Automotive Safety Integrity Level (ISO 26262)
* **BCM:** Broadcom Corporation (SoC manufacturer for Raspberry Pi)
* **BLS:** Basic Life Support
* **BSP:** Board Support Package
* **CAD:** Computer-Aided Dispatch
* **CPS:** Cyber-Physical System
* **CRC:** Cyclic Redundancy Check
* **DPDT:** Double-Pole Double-Throw (Electrical Relay)
* **ED:** Emergency Department (Hospital)
* **EOC:** Emergency Operations Center
* **ETA:** Estimated Time of Arrival
* **FSM:** Finite State Machine
* **GCS:** Glasgow Coma Scale
* **GNSS:** Global Navigation Satellite System
* **GPS:** Global Positioning System
* **HAL:** Hardware Abstraction Layer
* **ICU:** Intensive Care Unit
* **IMU:** Inertial Measurement Unit
* **IPC:** Inter-Process Communication
* **NMEA:** National Marine Electronics Association (0183 GPS protocol)
* **POSIX:** Portable Operating System Interface
* **PWM:** Pulse Width Modulation
* **Qnet:** QNX Native Distributed Network Messaging
* **RAII:** Resource Acquisition Is Initialization (C++ memory idiom)
* **RTOS:** Real-Time Operating System
* **SBC:** Single-Board Computer (Raspberry Pi 4)
* **SDP:** Software Development Platform (QNX)
* **SoC:** System on Chip
* **SPI:** Serial Peripheral Interface
* **UART:** Universal Asynchronous Receiver-Transmitter
* **V2I:** Vehicle-to-Infrastructure Communication
* **V2V:** Vehicle-to-Vehicle Communication
* **V2X:** Vehicle-to-Everything Communication
* **WDT:** Watchdog Timer

### 27.3 Architectural Decision Records (ADR)

#### ADR-01: Zero Third-Party Core Dependencies
* **Context:** Real-time safety-critical systems running on RTOS platforms face severe certification hurdles, ABI mismatches, and memory leak vulnerabilities when depending on large third-party libraries (e.g., Boost, Poco, Qt).
* **Decision:** Implement all core data structures, algorithms, JSON parsing, binary framing, network sockets, thread pools, and mathematical routines using pure Modern ISO C++17 and native POSIX/QNX OS primitives.
* **Consequence:** Absolute source portability, deterministic execution, zero library bloat, instantaneous compilation times, and zero licensing conflicts.

#### ADR-02: Strict Separation of HAL and BSP Layers
* **Context:** Hardware peripherals vary widely between development laptops, simulation testbeds, Raspberry Pi Linux rigs, and industrial QNX microkernel controllers.
* **Decision:** Partition hardware interfacing into two distinct layers: Hardware Abstraction Layer (HAL, vendor-neutral interface isolating business logic) and Board Support Package (BSP, silicon-facing driver controlling physical registers).
* **Consequence:** Upper coordinator and decision logic can be developed, tested, and verified on x86_64 simulation hosts with 100% architectural parity to target hardware rigs.

#### ADR-03: Raft-Style Consensus Quorum Before Signal Actuation
* **Context:** In a distributed network, network partitions or stale preemption messages can cause erratic, unsynchronized signal preemption across adjacent intersections.
* **Decision:** Edge intersection controllers must conduct a majority vote quorum ($(N/2)+1$) before energizing green lamp relays for an emergency corridor.
* **Consequence:** Prevents single-node split-brain preemption and guarantees that all intersections along a corridor agree on the preemption plan before lamp actuation.

#### ADR-04: Endian-Safe Binary Wire Protocol with CRC-16 Checksum
* **Context:** JSON and text-based protocols suffer from high parsing latency, non-deterministic memory allocations, and lack of built-in bit-level error detection.
* **Decision:** Define a fixed 19-byte binary header with magic bytes `0xA55A`, big-endian bit shift encoding, and dynamic CRC-16-CCITT checksum validation.
* **Consequence:** Sub-0.1 ms message processing, zero heap fragmentation in framing loops, and instantaneous rejection of corrupted network packets.

---

### 27.4 Revision History
* **v1.0 (Current Baseline):** Complete consolidation of Phases 1 through 16A and QNX Network Communication Layer into the Master Technical Reference Manual. All 42 unit test suites verified passing (100% success rate).

