# SMART AEGIS: Software Development Plan & Engineering Roadmap
**Document Identifier:** SDP-AEGIS-2026-V1.0  
**Classification:** Mission-Critical Cyber-Physical System (CPS)  
**Applicable Standards:** POSIX.1-2017, ISO 26262 (ASIL-B Principles), IEC 62304, IEEE 802.11p/C-V2X  
**Architecture Baseline Reference:** [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md)

---

## 1. Development Philosophy

The development of **SMART AEGIS** follows a disciplined, safety-oriented embedded and distributed systems engineering approach. In mission-critical cyber-physical systems where human lives and traffic safety intersect, software reliability cannot be retrofitted—it must be engineered from the first component.

### 1.1 Module-by-Module Construction
Development progresses strictly on an incremental, module-by-module basis. No monolithic or "all-at-once" development is permitted. Each functional unit defined in `SYSTEM_ARCHITECTURE.md` is developed in isolation as an autonomous component before being scheduled for integration.

### 1.2 Independent Testability
Every module must be accompanied by standalone harness drivers, mock interfaces, and boundary-condition assertions. A module is considered incomplete if it cannot be compiled, executed, and verified in an isolated environment detached from the broader network or hardware layer.

### 1.3 Architectural Invariance
The architectural contracts defined in `SYSTEM_ARCHITECTURE.md` are binding. No implementation may:
* Violate single-responsibility boundaries.
* Introduce undocumented inter-module couplings.
* Bypass data sanitization, authentication, or safety monitoring layers.
* Modify protocol payload schemas unilaterally without formal updates to the architecture specification.

### 1.4 Safety-Critical Determinism & Real-Time Constraints
Core actuation routines (traffic signal phase changes, preemption interlocks, collision avoidance warnings) are treated as hard/firm real-time tasks. Execution jitter on QNX Neutrino RTOS microkernel tasks must remain bounded within strict microsecond tolerances. Real-time safety paths must never depend on non-deterministic external calls or dynamic memory allocations during time-critical loops.

---

## 2. System Requirements Specification (SRS Traceability Baseline)

To establish complete traceability from conception through verification, the system is governed by the following formal System Requirements:

| Requirement ID | Category | Requirement Description |
|---|---|---|
| **REQ-001** | Ingestion | System shall ingest emergency call metadata and categorize incidents with standardized triage codes (Red, Amber, Green) within 500ms of entry. |
| **REQ-002** | Fleet Mgmt | System shall maintain dynamic state machines for all fleet ambulances, preventing double-dispatch under concurrent requests via atomic reservation locks. |
| **REQ-003** | Navigation | System shall compute optimal, lowest-transit-time paths incorporating live traffic density and signalized intersection nodes within 50ms of dispatch. |
| **REQ-004** | Kinematics | Ambulance On-Board Unit (OBU) shall sample, filter, and transmit GPS kinematics (position, speed, bearing) at a rate of 10 Hz with covariance validation. |
| **REQ-005** | Dead-Reckoning| OBU shall execute Inertial Measurement Unit (IMU) dead-reckoning extrapolation during GNSS outages, bounding drift to < 10m over 30 seconds. |
| **REQ-006** | Preemption | Dynamic Green Corridor shall compute dynamic preemption arrival envelopes and issue preemption triggers to downstream traffic controllers. |
| **REQ-007** | Signal Safety | Traffic Signal Controller shall enforce physical non-conflicting green interlocks, mandatory yellow clearance (≥ 3.0s), and all-red clearance (≥ 1.0s). |
| **REQ-008** | Fail-Safe Signal| Traffic Signal Controller shall revert to isolated fail-safe flashing amber within 50ms upon software fault, communication timeout, or watchdog trip. |
| **REQ-009** | Vitals Stream | Patient Vitals Module shall ingest, normalize, and stream physiological metrics (ECG, SpO2, NIBP, HR) at 1 Hz with sub-second transmission latency. |
| **REQ-010** | Vitals Alarm | Patient Vitals Module shall detect acute physiological deterioration thresholds and emit high-priority clinical alarms within 100ms of detection. |
| **REQ-011** | Hospital HMI | Hospital Dashboard shall render dynamic ETA countdowns, live vitals waveforms, and support bidirectional clinical advice transmission. |
| **REQ-012** | Traffic HMI | Traffic Dashboard shall render real-time intersection phase states, active green corridors, and support authenticated manual preemption overrides. |
| **REQ-013** | Fleet HMI | Route Dashboard shall display global fleet locations, active trajectories, off-route alerts, and route recalculation prompts. |
| **REQ-014** | Arbitration | Priority Scheduler shall arbitrate concurrent resource demands (crossing corridors, hospital docks) based on triage level and dynamic vitals severity. |
| **REQ-015** | Deconfliction| Multi Ambulance Coordinator shall detect trajectory convergence between multiple units ≥ 15s before arrival and issue speed/stagger advisories. |
| **REQ-016** | V2V Ad-hoc | Vehicles shall broadcast low-latency ad-hoc Basic Safety Messages (BSM) over direct wireless media at 10 Hz without cellular dependence. |
| **REQ-017** | V2I Fallback | Roadside traffic controllers shall accept and verify direct local V2I preemption beacons when central WAN connectivity is lost. |
| **REQ-018** | Police Alert | System shall generate automated CAD-interoperable perimeter clearance notices to police dispatch consoles upon corridor creation. |
| **REQ-019** | Audit Logging | System shall record all tactical, operational, and safety events with monotonic microsecond timestamps into tamper-evident storage. |
| **REQ-020** | Auth & RBAC | System shall enforce mutual TLS (mTLS) for edge nodes and role-based access control (RBAC) with cryptographic JWT tokens for human operators. |
| **REQ-021** | Security Guard| System shall reject replayed, expired (> 5s skew), or unauthenticated commands across all network and IPC interfaces. |
| **REQ-022** | Watchdog | Safety Monitor shall service the physical Hardware Watchdog Timer (WDT) and execute fail-safe system reboot if heartbeats cease > 500ms. |
| **REQ-023** | Network Comm | Network Communication layer shall provide transparent store-and-forward telemetry buffering during cellular dropouts with zero data loss. |
| **REQ-024** | Dynamic Config| Configuration Manager shall validate configuration schemas at startup and distribute dynamic parameters without requiring process restarts. |
| **REQ-025** | Determinism | Critical QNX real-time threads shall operate under deterministic priority scheduling with priority inheritance enabled to prevent inversion. |

---

## 3. Development Phases (Enhanced with Traceability)

```
┌────────────────────────────────────────────────────────────────────────┐
│ Phase 1: Project Initialization & Configuration Management             │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
┌───────────────────────────────────▼────────────────────────────────────┐
│ Phase 2: Communication Infrastructure & Authentication Backbone        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
          ┌─────────────────────────┴─────────────────────────┐
          ▼                                                   ▼
┌───────────────────────────────────┐   ┌────────────────────────────────┐
│ Phase 3: Coordinator Core         │   │ Phase 12: Logging Service      │
└─────────────────┬─────────────────┘   └────────────────┬───────────────┘
                  │                                      │
          ┌───────┴───────────────┬──────────────────────┤
          ▼                       ▼                      │
┌──────────────────┐    ┌──────────────────┐             │
│ Phase 4:         │    │ Phase 5:         │             │
│ Ambulance Node   │    │ Traffic Signal   │             │
└─────────┬────────┘    └─────────┬────────┘             │
          │                       │                      │
          ├───────────────────────┤                      │
          ▼                       ▼                      │
┌──────────────────┐    ┌──────────────────┐             │
│ Phase 6:         │    │ Phase 7:         │             │
│ Hospital Dash    │    │ Traffic Dash     │             │
└─────────┬────────┘    └─────────┬────────┘             │
          │                       │                      │
          └───────────────┬───────┘                      │
                          ▼                              │
┌──────────────────────────────────────────────────┐     │
│ Phase 8: Priority Scheduling                     │     │
└─────────────────────────┬────────────────────────┘     │
                          │                              │
┌─────────────────────────▼────────────────────────┐     │
│ Phase 9: Multi Ambulance Coordination            │     │
└─────────────────────────┬────────────────────────┘     │
                          │                              │
┌─────────────────────────▼────────────────────────┐     │
│ Phase 10: V2V & V2I Ad-hoc Mesh                  │     │
└─────────────────────────┬────────────────────────┘     │
                          │                              │
┌─────────────────────────▼────────────────────────┐     │
│ Phase 11: Safety Monitoring (Watchdogs/Fail-Safe)│◄────┘
└─────────────────────────┬────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────┐
│ Phase 13: System Integration                     │
└─────────────────────────┬────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────┐
│ Phase 14: Rigorous Multi-Tier Verification       │
└─────────────────────────┬────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────┐
│ Phase 15: Final Live Demonstration Packaging     │
└──────────────────────────────────────────────────┘
```

---

### Phase 1: Project Initialization & Configuration Management
* **Purpose:** Establish the foundational workspace, standardized build environments, toolchains, cross-compilation configurations for QNX and Linux/Raspberry Pi, and unified serialization schemas.
* **Requirements Satisfied:** `REQ-024`.
* **Architecture Modules Implemented:** `Configuration Manager` (Module 19).
* **Interfaces Exposed:**
  * `IConfigService`: `GetParam(key)`, `SetParam(key, val)`, `SubscribeConfigChange(callback)`.
  * CLI configuration validator utility.
* **Interfaces Consumed:** Underlying POSIX / QNX filesystem APIs.
* **Downstream Phase Dependencies:** Directly prerequisite to **all subsequent phases (Phases 2 through 15)**.
* **Expected Deliverables:**
  * CMake / QNX `qcc` toolchain definitions.
  * Standardized directory layout (`src/`, `include/`, `tests/`, `config/`, `scripts/`).
  * Unified JSON / Protocol Buffer message schemas.
  * Schema-validated baseline configuration files (`config.default.json`).
* **Completion Criteria:**
  * Clean compilation across both native host (x86_64 Linux/macOS) and target (ARMv7/ARMv8 QNX SDP & Raspberry Pi).
  * 100% pass rate on configuration schema validation test suite.
* **Dependencies:** None.

---

### Phase 2: Communication Infrastructure & Authentication Backbone
* **Purpose:** Construct the resilient network and IPC messaging backbone that allows modules to exchange events securely across processes, hosts, and physical boards.
* **Requirements Satisfied:** `REQ-020`, `REQ-021`, `REQ-023`.
* **Architecture Modules Implemented:** `Network Communication` (Module 18), `Authentication` (Module 16).
* **Interfaces Exposed:**
  * `INetworkTransport`: `Publish(topic, payload, qos)`, `Subscribe(topic, callback)`, `SendP2P(dest, payload)`.
  * `IAuthService`: `AuthenticateNode(cert)`, `ValidateToken(jwt)`, `RevokeToken(id)`.
* **Interfaces Consumed:** `IConfigService` (from Phase 1), OS socket libraries, OpenSSL/mbedTLS.
* **Downstream Phase Dependencies:** Unlocks **Phases 3, 4, 5, 6, 7, 10, 11, 12**.
* **Expected Deliverables:**
  * Transport abstraction layer (MQTT client, POSIX/QNX socket wrappers, WebSockets adapter).
  * Automated reconnect and exponential back-off engine.
  * mTLS handshake and JWT token verification interceptors.
* **Completion Criteria:**
  * Verified end-to-end publish/subscribe loop across simulated network partitions.
  * Immediate rejection of expired, unsigned, or replayed packets.
  * Reconnection latency verified under 1.5 seconds following link restoration.
* **Dependencies:** Phase 1.

---

### Phase 3: Coordinator Core
* **Purpose:** Establish central intelligence for emergency call handling, unit state tracking, navigation computation, and corridor triggering.
* **Requirements Satisfied:** `REQ-001`, `REQ-002`, `REQ-003`, `REQ-006`.
* **Architecture Modules Implemented:** `Emergency Call Module` (Module 1), `Ambulance Management` (Module 2), `Route Planning` (Module 3), `Dynamic Green Corridor` (Module 5).
* **Interfaces Exposed:**
  * `ICallIntake`: `SubmitEmergencyCall(CallPayload) -> IncidentID`.
  * `IFleetManager`: `ReserveUnit(UnitID, IncidentID)`, `UpdateUnitState(UnitID, State)`.
  * `IRouteEngine`: `CalculateRoute(StartCoord, EndCoord) -> RouteVector`.
  * `ICorridorService`: `TriggerCorridor(RouteVector, VehicleTelemetry)`, `ReleaseCorridor(IntersectionID)`.
* **Interfaces Consumed:** `INetworkTransport`, `IAuthService`, `IConfigService`.
* **Downstream Phase Dependencies:** Prerequisite to **Phases 6, 7, 8, 9, 13**.
* **Expected Deliverables:**
  * Triage categorization logic with boundary validation.
  * In-memory fleet state registry with atomic mutex reservation locks.
  * Graph-based A* path planning engine with intersection passage vector outputs.
  * Dynamic green corridor spatial calculation and preemption command engine.
* **Completion Criteria:**
  * Emergency call ingestion to validated incident event within 50ms.
  * A* shortest-time path calculation completes in < 50ms on a 500-node graph.
  * Dynamic preemption trigger windows generated accurately against simulated speeds.
* **Dependencies:** Phase 1, Phase 2.

---

### Phase 4: Ambulance Node
* **Purpose:** Implement the vehicle On-Board Unit (OBU) executing on embedded hardware (Raspberry Pi / QNX) to ingest telemetry, process vitals, and maintain connection to dispatch.
* **Requirements Satisfied:** `REQ-004`, `REQ-005`, `REQ-009`, `REQ-010`, `REQ-023`.
* **Architecture Modules Implemented:** `GPS Tracking` (Module 4), `Patient Vitals Module` (Module 10).
* **Interfaces Exposed:**
  * `ITelematicsFeed`: `GetKinematics() -> KinematicsPacket`.
  * `IPatientMonitor`: `StreamVitals() -> VitalsPacket`, `RegisterAlarmCallback(callback)`.
* **Interfaces Consumed:** `INetworkTransport`, `IConfigService`, physical/mock serial UART drivers.
* **Downstream Phase Dependencies:** Prerequisite to **Phases 6, 8, 10, 13**.
* **Expected Deliverables:**
  * Serial NMEA GPS parser and Extended Kalman Filter (EKF) dead-reckoning module.
  * Physiological vitals ingestion engine with anomaly threshold detection.
  * Ring-buffer store-and-forward caching queue for network blackouts.
* **Completion Criteria:**
  * GPS feed streams kinematic packets at 10 Hz with valid NMEA checksums.
  * Vitals simulator detects acute drop (e.g., blood pressure < 80 mmHg) and emits alarm in < 100ms.
  * Zero telemetry loss across a simulated 30-second WAN disconnect.
* **Dependencies:** Phase 1, Phase 2.

---

### Phase 5: Traffic Signal Node
* **Purpose:** Build the deterministic, fail-safe Roadside Unit (RSU) intersection controller interfacing with physical signal indicators and safety interlocks.
* **Requirements Satisfied:** `REQ-007`, `REQ-008`, `REQ-022`.
* **Architecture Modules Implemented:** `Traffic Signal Controller` (Module 6).
* **Interfaces Exposed:**
  * `ISignalController`: `RequestPreemption(PreemptCmd) -> Ack`, `GetPhaseState() -> PhaseState`.
* **Interfaces Consumed:** `INetworkTransport`, `IConfigService`, GPIO / Relay driver lines.
* **Downstream Phase Dependencies:** Prerequisite to **Phases 7, 10, 11, 13**.
* **Expected Deliverables:**
  * Finite State Machine (FSM) enforcing Red, Amber, Green cycles.
  * Hardware GPIO/Relay driver abstraction layer.
  * Preemption handler with minimum-green, yellow-clearance, and all-red guarantees.
  * Hardware/Software conflict interlock preventing conflicting green states.
* **Completion Criteria:**
  * Signal phases cycle deterministically.
  * Preemption command triggers safe yellow clearance (≥ 3.0s) prior to corridor green.
  * Under simulated process crash, outputs drop to fail-safe flashing amber within 50ms.
* **Dependencies:** Phase 1, Phase 2.

---

### Phase 6: Hospital Dashboard
* **Purpose:** Provide emergency ward physicians and triage teams with real-time incoming patient telematics, live vitals waveforms, and ETA projections.
* **Requirements Satisfied:** `REQ-011`.
* **Architecture Modules Implemented:** `Hospital Dashboard` (Module 7).
* **Interfaces Exposed:**
  * Web / Qt user interface for emergency admissions.
  * Clinical instruction endpoint: `SendClinicalAdvisory(AdvisoryPayload)`.
* **Interfaces Consumed:** `INetworkTransport` (subscribing to vitals and ETA topics), `IAuthService`.
* **Downstream Phase Dependencies:** Feeds into **Phases 13, 15**.
* **Expected Deliverables:**
  * Emergency intake dashboard interface.
  * Dynamic ETA countdown tracker with distance progress bar.
  * Streaming physiological vitals chart component (ECG, HR, SpO2, NIBP).
  * Two-way clinical advice transmission console.
* **Completion Criteria:**
  * Telemetry updates render without UI frame drops (< 16ms render budget).
  * Critical vitals alarm triggers prominent visual and audible UI states.
  * Paramedic instructions dispatched from dashboard are acknowledged by vehicle node.
* **Dependencies:** Phase 2, Phase 3, Phase 4.

---

### Phase 7: Traffic Dashboard
* **Purpose:** Equip municipal traffic operators with a comprehensive spatial console to supervise city intersections, active preemption corridors, and manual overrides.
* **Requirements Satisfied:** `REQ-012`, `REQ-013`.
* **Architecture Modules Implemented:** `Traffic Dashboard` (Module 8), `Route Dashboard` (Module 9).
* **Interfaces Exposed:**
  * Municipal supervisory GIS console.
  * Manual override interface: `TriggerManualOverride(IntersectionID, PhaseHold)`.
* **Interfaces Consumed:** `INetworkTransport`, `IAuthService`.
* **Downstream Phase Dependencies:** Feeds into **Phases 13, 15**.
* **Expected Deliverables:**
  * Map-centric GIS monitoring console displaying all signals and active corridors.
  * Real-time signal phase visualization (Red/Yellow/Green indicators).
  * Secure manual override console for emergency signal holding or clearing.
  * Fleet position tracking map with route history and deviation alerts.
* **Completion Criteria:**
  * Map displays active green corridors synchronously with vehicle transit.
  * Manual override securely takes precedence over automatic cycles within 200ms.
  * Reroute event displays immediate alternative visual paths.
* **Dependencies:** Phase 2, Phase 3, Phase 5.

---

### Phase 8: Priority Scheduling
* **Purpose:** Implement multi-criteria decision algorithms to rank incidents, resolve resource competition, and arbitrate crossing priorities.
* **Requirements Satisfied:** `REQ-014`, `REQ-025`.
* **Architecture Modules Implemented:** `Priority Scheduler` (Module 11).
* **Interfaces Exposed:**
  * `IPriorityArbiter`: `EvaluateTriagePriority(IncidentData, VitalsData) -> PriorityScore`, `ResolveContention(UnitA, UnitB) -> YieldDecision`.
* **Interfaces Consumed:** `ICallIntake`, `IPatientMonitor`, `INetworkTransport`.
* **Downstream Phase Dependencies:** Prerequisite to **Phases 9, 13**.
* **Expected Deliverables:**
  * Algorithmic triage weighting engine (evaluating vitals, incident category, distance).
  * Automated vehicle allocation optimizer.
  * Preemption rights arbiter for contested corridors.
* **Completion Criteria:**
  * Higher-criticality incident preempts lower-priority unit allocation deterministically.
  * Contested intersection grants green corridor to vehicle with the highest priority score.
* **Dependencies:** Phase 3, Phase 4.

---

### Phase 9: Multi Ambulance Coordination
* **Purpose:** Coordinate spatial interactions between multiple responding ambulances to prevent trajectory conflicts, intersection collisions, and hospital bottlenecks.
* **Requirements Satisfied:** `REQ-015`.
* **Architecture Modules Implemented:** `Multi Ambulance Coordinator` (Module 12).
* **Interfaces Exposed:**
  * `ICoordinationService`: `RegisterTrajectory(UnitID, RouteVector)`, `DetectConflicts() -> ConflictReport`.
* **Interfaces Consumed:** `IPriorityArbiter`, `ITelematicsFeed`, `IRouteEngine`.
* **Downstream Phase Dependencies:** Feeds into **Phases 10, 13**.
* **Expected Deliverables:**
  * Spatial trajectory intersection prediction engine.
  * Speed advisory generator to stagger conflicting arrivals at shared intersections.
  * Staging and hospital dock distribution coordinator.
* **Completion Criteria:**
  * Detects converging trajectory conflicts at least 15 seconds before intersection entry.
  * Issues speed adjustments or yield orders that resolve crossing hazards cleanly.
* **Dependencies:** Phase 3, Phase 8.

---

### Phase 10: V2V Communication & Ad-Hoc Mesh
* **Purpose:** Equip vehicles and roadside units with decentralized, ultra-low-latency direct wireless ad-hoc communications for proximity safety and localized preemption.
* **Requirements Satisfied:** `REQ-016`, `REQ-017`.
* **Architecture Modules Implemented:** `V2V Communication` (Module 13).
* **Interfaces Exposed:**
  * `IV2XBeacon`: `BroadcastSafetyBeacon(BSMPacket)`, `ReceiveBeacons() -> NeighborTable`.
  * `IV2IFallback`: `SendDirectPreemption(IntersectionID, PreemptBeacon)`.
* **Interfaces Consumed:** `ITelematicsFeed`, raw POSIX / QNX broadcast socket interfaces.
* **Downstream Phase Dependencies:** Feeds into **Phases 11, 13**.
* **Expected Deliverables:**
  * Ad-hoc UDP broadcast beacon transmitter and receiver (BSM schema).
  * Dynamic peer neighbor table maintaining proximity states.
  * Direct V2I fallback preemption trigger (bypassing central broker).
* **Completion Criteria:**
  * Latency between peer nodes under 20ms over local wireless medium.
  * Traffic signal switches to green corridor via direct V2I beacon during WAN blackout.
* **Dependencies:** Phase 4, Phase 5.

---

### Phase 11: Safety Monitoring
* **Purpose:** Provide supervisory watchdog monitoring, invariant enforcement, fail-safe degradation, and system heartbeat tracking.
* **Requirements Satisfied:** `REQ-008`, `REQ-022`, `REQ-025`.
* **Architecture Modules Implemented:** `Safety Monitor` (Module 17).
* **Interfaces Exposed:**
  * `IWatchdogSupervisor`: `RegisterHeartbeat(ModuleID, TimeoutMs)`, `KickWatchdog(ModuleID)`.
  * `IFailSafeManager`: `TriggerDegradedMode(SubsystemID, Reason)`.
* **Interfaces Consumed:** Underlying QNX High Availability Manager (HAM) or Linux WDT device `/dev/watchdog`.
* **Downstream Phase Dependencies:** Prerequisite to **Phases 13, 14**.
* **Expected Deliverables:**
  * Multi-process heartbeat monitor with pulse intervals.
  * Hardware Watchdog Timer (WDT) service driver.
  * Fail-safe degradation state machine (flashing amber, manual control release).
* **Completion Criteria:**
  * Process crash or unresponsiveness triggers hardware/software recovery within 500ms.
  * Broken heartbeats cause traffic signal to revert to isolated fail-safe mode safely.
* **Dependencies:** Phase 2, Phase 5, Phase 10.

---

### Phase 12: Logging & Police Notification
* **Purpose:** Deploy high-throughput, non-blocking, tamper-evident audit logging across all nodes and automate law enforcement clearance feeds.
* **Requirements Satisfied:** `REQ-018`, `REQ-019`.
* **Architecture Modules Implemented:** `Logging` (Module 15), `Police Notification` (Module 14).
* **Interfaces Exposed:**
  * `ILogger`: `LogTrace(ModuleID, Msg)`, `LogWarn(...)`, `LogCritical(...)`, `AuditSecurity(...)`.
  * `IPoliceAlertService`: `DispatchPerimeterNotice(CorridorID, RouteCoords)`.
* **Interfaces Consumed:** QNX `slogger2` backend, inter-agency REST/CAP API endpoints.
* **Downstream Phase Dependencies:** Feeds into **Phases 13, 14, 15**.
* **Expected Deliverables:**
  * Asynchronous zero-copy logging backend with monotonic microsecond timestamps.
  * Automated police dispatch notification engine with CAD-compatible JSON output.
* **Completion Criteria:**
  * Logging overhead introduces < 1% CPU footprint during peak loads.
  * Complete audit trail recoverable after sudden node power interruption.
* **Dependencies:** Phase 1, Phase 2.

---

### Phase 13: System Integration
* **Purpose:** Execute progressive system interconnection, validating end-to-end data pipelines from emergency call ingestion down to physical intersection relay toggling and hospital dashboard rendering.
* **Requirements Satisfied:** All requirements (`REQ-001` through `REQ-025`).
* **Architecture Modules Implemented:** All 19 modules integrated.
* **Expected Deliverables:**
  * Fully unified multi-node system running across target hardware and host workstations.
  * Integrated launch scripts and daemon management configurations (`systemd` / QNX startup scripts).
* **Completion Criteria:**
  * Complete mission sequence executes end-to-end without manual intervention.
* **Dependencies:** Phases 1 through 12.

---

### Phase 14: Rigorous Multi-Tier Verification
* **Purpose:** Subject the entire integrated system to formal stress, fault-injection, network degradation, and compliance testing.
* **Requirements Satisfied:** Verification of `REQ-001` through `REQ-025`.
* **Modules Involved:** All 19 modules.
* **Expected Deliverables:**
  * Automated regression test suite.
  * Chaos engineering and network-drop test scripts.
  * Hardware-in-the-loop (HIL) verification reports.
* **Completion Criteria:**
  * 100% pass rate on functional regression tests.
  * Deterministic recovery demonstrated across all fault-injection scenarios.
* **Dependencies:** Phase 13.

---

### Phase 15: Final Live Demonstration Packaging
* **Purpose:** Package the project for presentation, configure hardware demonstration rigs, create live visualization tools, and verify showcase scenarios.
* **Requirements Satisfied:** Full operational presentation readiness.
* **Modules Involved:** All modules, Dashboards, Hardware Prototype Rigs.
* **Expected Deliverables:**
  * Polished live demonstration testbed with visual signal LEDs and moving ambulance indicators.
  * Recorded fallback demonstration video.
  * Judge-ready presentation slide deck and architectural one-pagers.
* **Completion Criteria:**
  * Flawless execution across all 6 live presentation scenarios.
* **Dependencies:** Phase 14.

---

## 4. Major Milestones

```
M0: Architecture Complete ──────► M1: Coordinator Working ──────► M2: GPS & Telematics Working
                                                                               │
M5: Multi-Ambulance Ready ◄───── M4: Green Corridor Working ◄─── M3: Dashboards Operational
            │
            ▼
M6: Safety & Fail-Safe Validated ──► M7: Hackathon Ready (Golden Master)
```

| Milestone | Gate Criteria & Verification Deliverable | Target Timeline |
|---|---|---|
| **M0: Architecture Complete** | `SYSTEM_ARCHITECTURE.md` and `DEVELOPMENT_PLAN.md` ratified with zero ambiguity. | Day 1 |
| **M1: Coordinator Working** | Call intake, routing calculation, and unit state transitions operating in headless mode. | Day 2 |
| **M2: GPS & Telematics Working** | Vehicle node transmitting 10 Hz kinematics and vitals telemetry with store-and-forward caching. | Day 2 |
| **M3: Dashboards Operational** | Hospital, Traffic, and Route dashboards rendering live data feeds via Web/Qt. | Day 3 |
| **M4: Green Corridor Working** | Live preemption sequence verified on physical traffic signal hardware with all-red clearances. | Day 4 |
| **M5: Multi-Ambulance Ready** | Simultaneous dispatch of 2+ units with algorithmic priority resolution and deconfliction. | Day 5 |
| **M6: Safety & Fail-Safe Validated** | Hardware watchdog reboot, network blackout survival, and graceful degradation proven. | Day 6 |
| **M7: Hackathon Ready** | End-to-end physical hardware demonstration, polished UI, documentation, and video ready. | Day 7 |

---

## 5. Integration Order & Topological Assembly

To minimize integration risk, modules are assembled in strict topological order. **Under no circumstances are all modules integrated simultaneously.**

```
Step 1: Configuration & Logging Foundations
        [Configuration Manager] + [Logging]
                        │
                        ▼
Step 2: Core Messaging & Authentication
        [Network Communication] + [Authentication]
                        │
                        ▼
Step 3: Central Dispatch Logic
        [Emergency Call Module] + [Ambulance Management] + [Route Planning]
                        │
                        ▼
Step 4: Edge Sensor & Actuator Nodes
        [GPS Tracking] + [Patient Vitals]  ───►  [Ambulance OBU Node]
        [Traffic Signal Controller]        ───►  [Traffic RSU Node]
                        │
                        ▼
Step 5: Preemption Loop Closure
        [Ambulance OBU Node] + [Route Planning] + [Dynamic Green Corridor] + [Traffic RSU Node]
                        │
                        ▼
Step 6: Supervisory Visualization
        [Hospital Dashboard] + [Traffic Dashboard] + [Route Dashboard]
                        │
                        ▼
Step 7: Advanced Arbitration & Optimization
        [Priority Scheduler] + [Multi Ambulance Coordinator]
                        │
                        ▼
Step 8: Decentralized Mesh & Inter-Agency
        [V2V Communication] + [Police Notification]
                        │
                        ▼
Step 9: Safety Supervisor Integration
        [Safety Monitor] integrated across all active nodes
```

---

## 6. QNX Real-Time Operating System (RTOS) Architecture & Engineering

### 6.1 Microkernel Architecture & Message Passing
SMART AEGIS leverages QNX Neutrino’s microkernel architecture where all non-kernel services (network stacks, filesystem drivers, hardware resource managers) execute in memory-protected user space.
* **Synchronous IPC (`MsgSendv`, `MsgReceivev`, `MsgReplyv`):** Critical transactions between the edge safety monitor and the traffic controller use synchronous IPC channels, guaranteeing zero copy operations and bounded execution times.
* **Pulse Channels (`MsgSendPulse`):** Asynchronous, non-blocking 32-bit notifications used for high-frequency watchdog heartbeats, sensor arrival strobes, and timer expiration events. Pulses do not block the sender and carry a small, deterministic payload.

### 6.2 POSIX Resource Managers (`resmgr`)
Hardware devices on the Raspberry Pi / QNX embedded controller are exposed via custom POSIX Resource Managers:
* `/dev/traffic_controller`: Intercepts POSIX `read()`, `write()`, and `devctl()` calls, translating abstract phase commands into physical GPIO register writes.
* `/dev/gps_nmea`: Streams filtered kinematics data from UART hardware to consumer modules without exposing low-level serial registers.

### 6.3 Deterministic Priority Scheduling & Inversion Prevention
QNX Neutrino provides 256 priority levels. Real-time tasks in SMART AEGIS are configured as follows:
* **Priority 200–255 (Highest):** Hardware interrupt handlers and Watchdog kicker threads.
* **Priority 100–199:** Traffic Signal Controller FSM, Preemption Interlock, and V2I direct beacon listener.
* **Priority 50–99:** GPS kinematics processing, EKF dead-reckoning, and vitals anomaly detector.
* **Priority 1–49 (Lowest):** Background MQTT telemetry transmission, remote logging, and UI synchronization.
* **Priority Inheritance:** All mutexes guarding shared state (such as the intersection phase table) have `PTHREAD_PRIO_INHERIT` enabled to guarantee that lower-priority logging threads never indefinitely block high-priority safety interlocks.

### 6.4 Adaptive Partitioning
On multi-core edge hardware, QNX Adaptive Partitioning is configured to divide CPU budgets into hard safety partitions:
* **Safety Partition (Guaranteed 50% CPU):** Traffic Signal Controller, Watchdog kicker, Hardware interlock.
* **Telemetry Partition (Guaranteed 30% CPU):** GPS Tracking, Vitals module, Network client.
* **Best-Effort Partition (Remaining 20% CPU):** Background diagnostics and log rotation.
During a denial-of-service or network flood, the Safety Partition cannot be starved of CPU cycles.

### 6.5 High Availability Manager (HAM) & Watchdog Timers
* QNX High Availability Manager monitors critical edge processes. If the Traffic Signal Controller process experiences a fatal crash (`SIGSEGV`, `SIGBUS`), HAM immediately reconstructs the process context within 30ms or asserts the hardware fail-safe line.
* A hardware watchdog service runs as a dedicated daemon, petting the Raspberry Pi hardware watchdog register (`/dev/watchdog`) every 200ms. If heartbeats stop for > 500ms, the board automatically power-cycles.

### 6.6 Asynchronous High-Speed Logging (`slogger2`)
All logging calls on QNX targets use `slogger2`, which writes binary log events directly into shared-memory circular ring buffers. This prevents disk I/O bottlenecks from inducing latency spikes into real-time control loops.

### 6.7 Cross-Compilation & Target Deployment
* Build toolchain utilizes the QNX Software Development Platform (`qcc` compiler targeting `aarch64le` for Raspberry Pi 4B/5).
* Target deployment is automated via TFTP/NFS boot scripts and secure SSH deployment keys during CI runs.

---

## 7. Simulation & Hardware-in-the-Loop (HIL) Strategy

To enable comprehensive verification prior to physical board availability, SMART AEGIS implements a strict "Simulation-First" development methodology.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        SIMULATION TESTBED                              │
│                                                                        │
│  [Virtual GPS Generator] ────► [Ambulance OBU Logic] ────► [Vitals Gen]│
│            │                                                    │      │
│            ▼                                                    ▼      │
│  [Network Emulation: NetEm (Latency/Jitter/Drops)] ◄────────────┘      │
│            │                                                           │
│            ▼                                                           │
│  [Central Dispatch & Corridor Engine]                                  │
│            │                                                           │
│            ▼                                                           │
│  [Virtual Traffic Intersection Array (Simulated Relays & FSM)]         │
│            │                                                           │
│            ▼                                                           │
│  [Fault Injector: Broker Kill, Sensor Detach, Watchdog Starvation]     │
└────────────────────────────────────────────────────────────────────────┘
```

### 7.1 Virtual GPS & Kinematics Generator
* A software module emulates a physical u-blox GNSS receiver, transmitting valid NMEA-0183 (`$GPRMC`, `$GPGGA`) sentences over virtual pseudo-terminal (`pty`) pairs.
* Supports trajectory playback from predefined GeoJSON routes, simulating realistic vehicle speeds, acceleration curves, and satellite signal-to-noise ratio degradation.

### 7.2 Virtual Traffic Signal Intersections
* A software simulator instantiates an array of 20+ virtual signalized intersections matching real-world municipal spacing.
* Replicates the complete FSM, yellow clearance intervals, and relay outputs, rendering intersection states visually in real time.

### 7.3 Mock Hospital Telemetry Sink
* Emulates receiving hospital emergency departments, auto-acknowledging inbound transfers, and simulating ER physician orders.

### 7.4 Network Emulation & WAN NetEm Simulation
* Utilizes Linux `tc / netem` to simulate hostile network environments:
  * 3G/4G cellular latency (50ms – 250ms normal distribution).
  * Packet loss bursts (up to 15% random drop).
  * Total connection blackout periods (tunnels, cellular dead zones).

### 7.5 Fault Injection Framework
An automated chaos-testing script injects precise failure conditions:
* **Broker Kill:** Abruptly terminates the central MQTT broker during active preemption.
* **Sensor Detach:** Simulates sudden physical disconnection of the SpO2 / ECG sensors.
* **Corrupted NMEA:** Injects malformed checksums and out-of-order coordinate jumps.
* **Thread Starvation:** Induces 100% CPU load on non-real-time cores to verify priority isolation.

---

## 8. Resource Planning

### 8.1 Hardware Resource Allocation Matrix

| Phase | Development Hardware Required | Target Deployment Hardware | Testing & Validation Equipment |
|---|---|---|---|
| **Phase 1** | Developer Workstation (x86_64) | None (Host builds) | Native compilation runners |
| **Phase 2** | Workstation, Local LAN Router | Raspberry Pi 4B (Linux / QNX) | Wireshark, Network Packet Analyzer |
| **Phase 3** | Workstation (Multi-core) | Central Server / Host | High-concurrency load tester |
| **Phase 4** | Workstation, USB-to-UART Adapter | Raspberry Pi 4B + GPS HAT + IMU | GNSS Simulator, Serial Terminal |
| **Phase 5** | Workstation, GPIO Breakout | Raspberry Pi 4B / QNX + 4-Ch Relay Board | Digital Multimeter, 12V LED Signal Rigs |
| **Phase 6** | Dual-Monitor Workstation | Web Tablet / Hospital Terminal | Chrome DevTools, Frame profiler |
| **Phase 7** | Multi-Monitor Workstation | Traffic Operation Console | WebGL / GPU Benchmarks |
| **Phase 8** | High-Performance Workstation | Central Server | Concurrency stress simulator |
| **Phase 9** | High-Performance Workstation | Central Server | Spatial collision testbed |
| **Phase 10** | 2x Laptops with Wi-Fi Ad-hoc | 2x Raspberry Pi 4B (Ad-hoc 802.11p) | RF Signal Analyzer, UDP packet generator |
| **Phase 11** | Workstation + Logic Analyzer | Raspberry Pi with WDT enabled | Physical power interrupt switch |
| **Phase 12** | Workstation, High-Speed SSD | Industrial eMMC on Raspberry Pi | Flash wear-out / write benchmark tools |
| **Phase 13** | Full Dev Rig (All Laptops) | 3x Raspberry Pi (2x OBU, 1x RSU) | Complete physical grid demonstration board |
| **Phase 14** | Complete Testbed | Full Distributed Rig | Automated Chaos Runner, Power Brownout Rig |
| **Phase 15** | Presentation Rig, Projector | Final Assembled Demonstration Rig | Backup Video Rig, Live Display Monitors |

### 8.2 Software Dependencies & Toolchains

| Dependency | Purpose | Target Version / License | Target Platform |
|---|---|---|---|
| **QNX SDP** | Microkernel RTOS & Cross-Compiler | QNX SDP 7.1 / 8.0 (Commercial) | Raspberry Pi 4B / Target SBC |
| **GCC / Clang** | Host Compilation & Tooling | C++17 compliant (GPL/LLVM) | Host Linux / macOS |
| **CMake** | Unified Build System | ≥ 3.22 (BSD) | Host & Cross-compilation |
| **Eclipse Mosquitto** | MQTT Telemetry Broker & Client | 2.0.x (EPL/EDL) | Central Server & Edge Nodes |
| **OpenSSL / mbedTLS** | TLS 1.3 & Cryptographic Services | OpenSSL 3.0.x / mbedTLS 3.x | All nodes |
| **Qt Framework** | Embedded & Desktop Dashboards | Qt 6.5 LTS (LGPLv3) | Dispatch & Hospital Workstations |
| **Node.js / React** | Web Dashboard Interface | Node v20 LTS / React 18 | Web Clients |
| **SQLite / libsqlite3** | Edge Persistent Buffering | 3.42+ (Public Domain) | Edge Nodes (OBU / RSU) |
| **Google Test (gtest)** | C++ Unit Testing & Mocking | 1.14.0 (BSD-3) | Test Runners |
| **PyTest** | Python Integration & Test Drivers | 7.4+ (MIT) | Integration Test Rig |

---

## 9. Configuration Management & Operational Profiles

### 9.1 Network Ports & IP Topology

```
┌────────────────────────────────────────────────────────┐
│               NETWORK TOPOLOGY SPECIFICATION           │
│                                                        │
│  Central Broker / Server:   192.168.10.100             │
│    • Port 8883:   MQTT over TLS (M2M Telemetry)        │
│    • Port 9001:   MQTT over WebSockets (Dashboards)    │
│    • Port 443:    HTTPS / REST API & Auth Service      │
│                                                        │
│  Ambulance OBU Node 1:      192.168.10.101             │
│  Ambulance OBU Node 2:      192.168.10.102             │
│    • Port 50051:  gRPC / Internal Control Interface    │
│    • Port 8999:   V2V / Ad-hoc UDP Broadcast           │
│                                                        │
│  Traffic RSU Node 1:        192.168.10.201             │
│  Traffic RSU Node 2:        192.168.10.202             │
│    • Port 8998:   V2I Direct Preemption UDP Receiver   │
│    • Port 50052:  RSU Maintenance Interface           │
└────────────────────────────────────────────────────────┘
```

### 9.2 MQTT Topic Namespace Specification
* `aegis/fleet/<unit_id>/kinematics` (QoS 0): 10 Hz GPS telemetry feed.
* `aegis/fleet/<unit_id>/vitals` (QoS 1): 1 Hz patient physiological data stream.
* `aegis/fleet/<unit_id>/alarms` (QoS 2): High-priority medical deterioration alarms.
* `aegis/corridor/<intersection_id>/preempt` (QoS 2): Preemption execution commands.
* `aegis/corridor/<intersection_id>/status` (QoS 1): Current signal phase broadcasts.
* `aegis/dispatch/incidents` (QoS 2): Emergency call dispatch orders.
* `aegis/system/heartbeat/<node_id>` (QoS 0): Health keep-alive pulses.

### 9.3 Timing Constants, Timeouts & Deadlines
* **Watchdog Service Interval:** 200ms.
* **Watchdog Hardware Timeout:** 500ms.
* **GPS Telemetry Sampling Period:** 100ms (10 Hz).
* **Vitals Telemetry Sampling Period:** 1000ms (1 Hz).
* **Preemption Yellow Clearance Interval:** 3000ms (Minimum mandatory).
* **Preemption All-Red Clearance Interval:** 1000ms (Minimum mandatory).
* **Network Disconnect Reconnect Backoff:** Exponential, base 500ms, max 5000ms.
* **Token Expiration Duration:** 3600 seconds (1 hour).

### 9.4 Build Profiles
* **Debug (`-DCMAKE_BUILD_TYPE=Debug`):** Optimization `-O0`, debug symbols `-g3`, verbose console logging enabled, address sanitizer (`ASan`) linked.
* **Release (`-DCMAKE_BUILD_TYPE=Release`):** Optimization `-O2`, symbols stripped, binary `slogger2` output only, security flags (`-fstack-protector-strong`, `-D_FORTIFY_SOURCE=2`).
* **Simulation (`-DAEGIS_SIMULATION=ON`):** Hooks physical GPIO and serial calls to virtual software simulators for headless testing.

---

## 10. Performance Targets & Resource Budgets

To prevent resource contention, every software module is constrained by rigid operational budgets:

| Module Name | Max CPU Load (%) | Max RAM Footprint (MB) | Latency Budget (ms) | Target Throughput (msg/s) | Max Acceptable Response Time (ms) | Max Packet Size (bytes) | Max Reconnect Time (s) | Target Uptime (%) |
|---|---|---|---|---|---|---|---|---|
| **Emergency Call Module** | < 2% | < 32 MB | < 20 ms | 50 calls/s | 100 ms | 4,096 B | 2.0 s | 99.99% |
| **Ambulance Management** | < 3% | < 48 MB | < 10 ms | 100 state/s | 50 ms | 1,024 B | 2.0 s | 99.99% |
| **Route Planning** | < 15% | < 128 MB | < 50 ms | 10 routes/s | 200 ms | 32,768 B | 2.0 s | 99.95% |
| **GPS Tracking** | < 5% | < 16 MB | < 5 ms | 20 pkts/s | 10 ms | 512 B | 1.0 s | 99.999% |
| **Dynamic Green Corridor** | < 5% | < 32 MB | < 10 ms | 50 cmds/s | 25 ms | 1,024 B | 1.0 s | 99.999% |
| **Traffic Signal Controller**| < 4% | < 16 MB | < 2 ms | 50 cmds/s | 10 ms | 512 B | 0.5 s | 99.999% |
| **Hospital Dashboard** | < 10% | < 96 MB | < 16 ms | 30 fps UI | 100 ms | 8,192 B | 3.0 s | 99.9% |
| **Traffic Dashboard** | < 10% | < 96 MB | < 16 ms | 30 fps UI | 100 ms | 16,384 B | 3.0 s | 99.9% |
| **Route Dashboard** | < 10% | < 96 MB | < 16 ms | 30 fps UI | 100 ms | 16,384 B | 3.0 s | 99.9% |
| **Patient Vitals Module** | < 4% | < 24 MB | < 10 ms | 10 pkts/s | 50 ms | 2,048 B | 1.0 s | 99.99% |
| **Priority Scheduler** | < 5% | < 32 MB | < 15 ms | 100 evts/s | 50 ms | 2,048 B | 2.0 s | 99.99% |
| **Multi Ambulance Coord** | < 8% | < 64 MB | < 20 ms | 50 evts/s | 50 ms | 4,096 B | 2.0 s | 99.99% |
| **V2V Communication** | < 5% | < 16 MB | < 2 ms | 20 pkts/s | 5 ms | 256 B | 0.5 s | 99.999% |
| **Police Notification** | < 2% | < 16 MB | < 50 ms | 10 pkts/s | 200 ms | 4,096 B | 5.0 s | 99.9% |
| **Logging Service** | < 2% | < 32 MB | < 1 ms | 1,000 logs/s| 10 ms | 1,024 B | 1.0 s | 99.99% |
| **Authentication Service** | < 3% | < 32 MB | < 5 ms | 100 tokens/s| 20 ms | 1,024 B | 2.0 s | 99.99% |
| **Safety Monitor** | < 2% | < 8 MB | < 1 ms | 100 pulses/s| 5 ms | 256 B | 0.2 s | 99.999% |
| **Network Communication** | < 6% | < 48 MB | < 5 ms | 500 msgs/s | 15 ms | 65,536 B | 1.0 s | 99.99% |
| **Configuration Manager** | < 1% | < 16 MB | < 5 ms | 10 queries/s| 10 ms | 8,192 B | 1.0 s | 99.99% |

---

## 11. Security & Information Assurance Plan

```
┌────────────────────────────────────────────────────────────────────────┐
│                        SECURITY ARCHITECTURE                           │
│                                                                        │
│   Mutual TLS 1.3 (mTLS) ──► Strict RBAC Tokens ──► Monotonic Nonces   │
│   (Node-to-Broker)          (Operator Claims)      (Anti-Replay)       │
│                                                          │             │
│   Zero-Trust IPC Boundaries ◄── Sanitized Inputs ◄───────┘             │
└────────────────────────────────────────────────────────────────────────┘
```

### 11.1 Identity & Authentication (mTLS & Device Keys)
* All distributed nodes (Ambulance OBU, Traffic RSU, Central Dispatch) authenticate using Mutual TLS (mTLS) over TLS 1.3.
* Node private keys are stored with restricted permissions (`chmod 0400`) and accessed exclusively by the local network daemon.
* Server certificates use a private root Certificate Authority (CA) created specifically for the municipal emergency grid.

### 11.2 Role-Based Access Control (RBAC) & Claims
Human operators and dashboard sessions authenticate via JSON Web Tokens (JWT) signed using ECDSA (P-256):
* `Role: Dispatcher`: Permitted to ingest calls, assign units, and alter global routes. Forbidden from overriding traffic signals directly.
* `Role: TrafficOperator`: Permitted to issue manual traffic preemption overrides. Forbidden from accessing patient clinical vitals.
* `Role: Paramedic`: Permitted to stream vitals and view navigation. Forbidden from altering signal timings.
* `Role: Physician`: Permitted to view vitals and issue clinical instructions.

### 11.3 Cryptographic Standards
* **Data in Transit:** TLS 1.3 with AES-256-GCM cipher suites.
* **V2V Ad-hoc Wireless:** Payloads signed with HMAC-SHA256 pre-shared key credentials for ultra-fast verification without certificate chain traversal overhead.
* **Stored Vitals:** HIPAA-compliant encryption at rest on edge storage.

### 11.4 Replay Attack Prevention & Integrity
* Every packet carries a monotonic 64-bit sequence counter and a UTC microsecond timestamp.
* Packets received with timestamps skewed > 5000ms relative to system time, or with duplicate sequence IDs, are dropped immediately and trigger a security audit alarm.

### 11.5 Input Validation & Boundary Sanitization
* All incoming payloads are validated against strict JSON/protobuf schemas prior to memory deserialization.
* String fields (caller names, descriptions) are sanitized against buffer overflow attacks and SQL/command injection vectors.
* Geospatial coordinates are clamped to valid metropolitan bounding box limits.

---

## 12. Comprehensive Risk Analysis & Mitigation Matrix

| Risk ID | Failure Description | Type | Likelihood | Impact | Severity | Mitigation Strategy | Owner |
|---|---|---|---|---|---|---|---|
| **RSK-001** | Central MQTT broker crashes or becomes unreachable. | SW/Net | Medium | Critical | High | Edge nodes switch immediately to local store-and-forward caching and activate localized V2I ad-hoc beacons. | Network Lead |
| **RSK-002** | GPS antenna failure or signal loss in urban canyon. | HW/Sens | High | High | High | EKF dead-reckoning uses 6-DOF IMU accelerometer/gyroscope to bridge location gaps up to 30s. | Embedded Lead |
| **RSK-003** | Traffic Signal Controller crashes or suffers memory fault. | SW | Low | Critical | High | QNX HAM / Hardware Watchdog resets board within 500ms; physical hardware drops to flashing amber. | RTOS Lead |
| **RSK-004** | Sudden power interruption to edge Raspberry Pi node. | HW | Medium | High | High | Read-only root filesystem with journaling write partitions prevents OS corruption; quick cold boot (< 10s). | Embedded Lead |
| **RSK-005** | Wireless network packet loss during preemption trigger. | Net | High | High | High | Preemption uses MQTT QoS 2 (four-step handshake) with automated retry and local RF beacon redundancy. | Network Lead |
| **RSK-006** | Race condition during simultaneous ambulance dispatch. | SW | Low | High | Medium | Atomic mutex reservation locks on in-memory vehicle objects ensure sequential asset reservation. | Backend Lead |
| **RSK-007** | Priority inversion in RTOS locking traffic interlocks. | SW/RT | Low | Critical | High | All POSIX mutexes configured with `PTHREAD_PRIO_INHERIT` to prevent lower-priority tasks from starving interlocks. | RTOS Lead |
| **RSK-008** | Clock drift across distributed nodes breaks timestamps. | Sys | Medium | Medium | Medium | Local NTP/PTP daemons synchronize clocks to within 5ms of central GPS reference time. | Systems Lead |
| **RSK-009** | Physical relay contact welding or driver failure on RSU. | HW | Low | Critical | High | Opto-isolated relay board with physical hardware interlock line physically preventing dual greens. | HW Lead |
| **RSK-010** | Biomedical sensor detachment during transit. | HW/Sens | High | Low | Low | Anomaly detector distinguishes zero-lead detachment from physiological drop, alerting paramedics immediately. | Telematics Lead |
| **RSK-011** | Deadlock during crossing path priority resolution. | SW | Low | High | Medium | Strict total ordering on vehicle IDs and deterministic score tie-breakers prevent cyclic deadlock. | Alg Lead |
| **RSK-012** | Memory leak causing crash during multi-day continuous run. | SW | Medium | High | High | Strict RAII smart pointer ownership, zero raw `new`/`malloc`, and Valgrind/ASan CI leak verification gates. | Quality Lead |

---

## 13. Testing Strategy & Verification Protocols

### 13.1 Testing Levels
1. **Unit Testing:** Algorithmic verification of graph search, checksum calculation, triage weighting, and schema validators.
2. **Module Testing:** Verification of individual modules running in isolated testbeds with mock inputs.
3. **Communication Testing:** Benchmark of MQTT/IPC throughput, latency jitter, and reconnection handling under partition.
4. **Hardware Testing:** Oscilloscope verification of relay switching speeds, GPIO stability, and serial UART timings.
5. **System Testing:** End-to-end execution of complete operational workflows across distributed hardware nodes.
6. **Chaos Testing:** Deliberate failure injection (process death, cable disconnection, thread starvation).
7. **Performance Testing:** Verification against the formal budgets in Section 10.

### 13.2 Formal Test Coverage Matrix

| Requirement | Module Name | Unit Test | Integration Test | Hardware Test | Chaos Test | Performance Test | Acceptance Test |
|---|---|---|---|---|---|---|---|
| **REQ-001** | Emergency Call Module | `UT-CALL-01` | `IT-CALL-01` | N/A | `CT-CALL-01` | `PT-CALL-01` | `AT-SCENARIO-2` |
| **REQ-002** | Ambulance Management | `UT-AMB-01` | `IT-AMB-01` | N/A | `CT-AMB-01` | `PT-AMB-01` | `AT-SCENARIO-2` |
| **REQ-003** | Route Planning | `UT-ROUTE-01`| `IT-ROUTE-01`| N/A | `CT-ROUTE-01`| `PT-ROUTE-01`| `AT-SCENARIO-2` |
| **REQ-004** | GPS Tracking | `UT-GPS-01`  | `IT-GPS-01`  | `HT-GPS-01` | `CT-GPS-01`  | `PT-GPS-01`  | `AT-SCENARIO-4` |
| **REQ-005** | GPS Tracking (EKF) | `UT-GPS-02`  | `IT-GPS-02`  | `HT-GPS-02` | `CT-GPS-02`  | `PT-GPS-02`  | `AT-SCENARIO-4` |
| **REQ-006** | Dynamic Green Corridor| `UT-CORR-01` | `IT-CORR-01` | `HT-CORR-01`| `CT-CORR-01` | `PT-CORR-01` | `AT-SCENARIO-4` |
| **REQ-007** | Traffic Signal Controller| `UT-SIG-01`| `IT-SIG-01`  | `HT-SIG-01` | `CT-SIG-01`  | `PT-SIG-01`  | `AT-SCENARIO-4` |
| **REQ-008** | Traffic Signal (Fail-Safe)|`UT-SIG-02`| `IT-SIG-02`  | `HT-SIG-02` | `CT-SIG-02`  | `PT-SIG-02`  | `AT-SCENARIO-6` |
| **REQ-009** | Patient Vitals Module | `UT-VIT-01`  | `IT-VIT-01`  | `HT-VIT-01` | `CT-VIT-01`  | `PT-VIT-01`  | `AT-SCENARIO-3` |
| **REQ-010** | Patient Vitals (Alarms)| `UT-VIT-02` | `IT-VIT-02`  | `HT-VIT-02` | `CT-VIT-02`  | `PT-VIT-02`  | `AT-SCENARIO-3` |
| **REQ-011** | Hospital Dashboard | `UT-HOSP-01` | `IT-HOSP-01` | N/A | `CT-HOSP-01` | `PT-HOSP-01` | `AT-SCENARIO-3` |
| **REQ-012** | Traffic Dashboard | `UT-TRF-01`  | `IT-TRF-01`  | N/A | `CT-TRF-01`  | `PT-TRF-01`  | `AT-SCENARIO-1` |
| **REQ-013** | Route Dashboard | `UT-RDSH-01` | `IT-RDSH-01` | N/A | `CT-RDSH-01` | `PT-RDSH-01` | `AT-SCENARIO-2` |
| **REQ-014** | Priority Scheduler | `UT-PRI-01`  | `IT-PRI-01`  | N/A | `CT-PRI-01`  | `PT-PRI-01`  | `AT-SCENARIO-5` |
| **REQ-015** | Multi Ambulance Coord | `UT-MAC-01`  | `IT-MAC-01`  | N/A | `CT-MAC-01`  | `PT-MAC-01`  | `AT-SCENARIO-5` |
| **REQ-016** | V2V Communication | `UT-V2V-01`  | `IT-V2V-01`  | `HT-V2V-01` | `CT-V2V-01`  | `PT-V2V-01`  | `AT-SCENARIO-6` |
| **REQ-017** | V2I Fallback Preemption| `UT-V2I-01` | `IT-V2I-01`  | `HT-V2I-01` | `CT-V2I-01`  | `PT-V2I-01`  | `AT-SCENARIO-6` |
| **REQ-018** | Police Notification | `UT-POL-01`  | `IT-POL-01`  | N/A | `CT-POL-01`  | `PT-POL-01`  | `AT-SCENARIO-4` |
| **REQ-019** | Logging Service | `UT-LOG-01`  | `IT-LOG-01`  | `HT-LOG-01` | `CT-LOG-01`  | `PT-LOG-01`  | `AT-SCENARIO-1` |
| **REQ-020** | Authentication | `UT-SEC-01`  | `IT-SEC-01`  | N/A | `CT-SEC-01`  | `PT-SEC-01`  | `AT-SCENARIO-1` |
| **REQ-021** | Security Guard (Replay)|`UT-SEC-02` | `IT-SEC-02`  | N/A | `CT-SEC-02`  | `PT-SEC-02`  | `AT-SCENARIO-1` |
| **REQ-022** | Safety Monitor (WDT) | `UT-SAF-01`  | `IT-SAF-01`  | `HT-SAF-01` | `CT-SAF-01`  | `PT-SAF-01`  | `AT-SCENARIO-6` |
| **REQ-023** | Network Comm (Buffer) | `UT-NET-01`  | `IT-NET-01`  | `HT-NET-01` | `CT-NET-01`  | `PT-NET-01`  | `AT-SCENARIO-6` |
| **REQ-024** | Configuration Manager | `UT-CFG-01`  | `IT-CFG-01`  | N/A | `CT-CFG-01`  | `PT-CFG-01`  | `AT-SCENARIO-1` |
| **REQ-025** | RTOS Priority Sched | `UT-RTOS-01` | `IT-RTOS-01` | `HT-RTOS-01`| `CT-RTOS-01` | `PT-RTOS-01` | `AT-SCENARIO-4` |

---

## 14. Continuous Integration & Continuous Deployment (CI/CD)

```
┌────────────────────────────────────────────────────────────────────────┐
│                        GITHUB ACTIONS CI PIPELINE                      │
│                                                                        │
│   [Git Push / PR] ──► [Format & Lint: clang-format / cpplint]          │
│                              │                                         │
│                              ▼                                         │
│                       [Static Analysis]                                │
│                     (clang-tidy / Cppcheck)                            │
│                              │                                         │
│                              ▼                                         │
│                    [Multi-Target Build]                                │
│              (Host x86_64 + QNX SDP aarch64le)                         │
│                              │                                         │
│                              ▼                                         │
│                 [Automated Unit Tests & ASan]                          │
│                              │                                         │
│                              ▼                                         │
│                 [Code Coverage Report (> 85%)]                         │
│                              │                                         │
│                              ▼                                         │
│                 [Artifact Packaging & Release]                         │
└────────────────────────────────────────────────────────────────────────┘
```

### 14.1 Pipeline Stage Definitions
1. **Lint & Code Format:** Enforces `clang-format` (Google C++ style) and `flake8` for Python scripts.
2. **Static Analysis:** Executes `clang-tidy` with bug-finding checks and `Cppcheck` to catch null pointer dereferences, uninitialized variables, and resource leaks.
3. **Multi-Target Cross-Compilation:**
   * Build 1: Native x86_64 host (GCC 12 / Clang 15) with AddressSanitizer (`-fsanitize=address,undefined`).
   * Build 2: Target ARMv8 cross-compilation via QNX SDP `qcc` toolchain.
4. **Automated Unit & Module Tests:** Executes all `gtest` and `pytest` targets in parallel.
5. **Coverage Gate:** Generates `gcov / lcov` reports. Pull requests failing to achieve ≥ 85% branch coverage on core logic are automatically blocked.
6. **Artifact Packaging:** Generates tarballs containing compiled binaries, default configuration files, and launch scripts, published as GitHub Release artifacts.

---

## 15. Coding Standards & Engineering Guidelines

### 15.1 C++17 Guidelines & Modern Idioms
* Standard: Strict **C++17** standard conformance (`-std=c++17`).
* Use `std::string_view` for non-owning string arguments to eliminate heap allocations.
* Use `std::optional` and `std::variant` instead of sentinel error values or raw union types.

### 15.2 Memory Safety & Ownership Rules
* **Zero Raw `new` / `delete`:** All memory allocation must utilize RAII smart pointers (`std::unique_ptr` for single ownership, `std::shared_ptr` for shared ownership).
* **Deterministic Allocation:** In safety-critical QNX threads (Traffic Controller, Watchdog), all buffers and data structures must be pre-allocated during process initialization. **Zero dynamic memory allocation (`malloc`, `new`) during real-time loops.**

### 15.3 Concurrency & Real-Time Thread Safety
* All shared state access must be guarded by `std::mutex` or `std::shared_mutex` via `std::lock_guard` / `std::unique_lock`.
* On QNX targets, mutexes guarding time-critical data must be initialized with `PTHREAD_PRIO_INHERIT`.
* Prefer lock-free atomic primitives (`std::atomic<bool>`, `std::atomic<uint64_t>`) for high-frequency telemetry counters.

### 15.4 Error Handling & Zero-Exception Policy
* **Zero Exceptions in Real-Time Code:** To maintain deterministic latency and comply with real-time standards, exceptions are disabled (`-fno-exceptions`) in embedded edge modules.
* Error propagation utilizes explicit `Result<T, ErrorCode>` structures (similar to `std::expected` idioms).

### 15.5 Naming Conventions & Project Structure
* **Namespaces:** `aegis::core`, `aegis::edge`, `aegis::telemetry`, `aegis::network`.
* **Types / Classes:** UpperCamelCase (e.g., `SignalController`, `KinematicsPacket`).
* **Functions / Methods:** UpperCamelCase for public methods (e.g., `RequestPreemption`), camelCase for internal helpers.
* **Variables / Members:** camelCase with `m_` prefix for member variables (e.g., `m_currentPhase`, `m_vehicleSpeed`).
* **Constants / Enums:** ALL_CAPS with underscores (e.g., `MAX_RECONNECT_ATTEMPTS`).
* **Documentation:** All classes and public interfaces must include Doxygen-compliant documentation comments (`/** ... */`).

---

## 16. Demonstration Plan & Showcase Scenarios

The demonstration provides judges with an intuitive, multi-screen, hardware-backed showcase demonstrating SMART AEGIS operating across normal, emergency, and catastrophic failure modes.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        JUDGES' DEMONSTRATION RIG                       │
│                                                                        │
│  [Screen 1: Dispatch & Route Map]     [Screen 2: Hospital Emergency ER]│
│  [Screen 3: Traffic Authority Desk]   [Physical Model: Mini-Grid LEDs] │
│                                                                        │
│  Physical Hardware:                                                    │
│  • Node A: Raspberry Pi (Ambulance OBU + GPS + Vitals Simulator)       │
│  • Node B: Raspberry Pi / QNX (Intersection RSU + Physical Relays/LEDs)│
└────────────────────────────────────────────────────────────────────────┘
```

---

### Scenario 1: Normal Traffic Baseline
* **Action:** System runs with no active emergency calls.
* **System Behavior:**
  * Traffic signal controllers cycle through standard fixed/adaptive timing phases (North-South Green -> Yellow -> Red; East-West Green...).
  * Traffic Dashboard shows green/red status reflecting reality.
  * Ambulances appear on the Fleet Dashboard in `AVAILABLE` status parked at their stations.
* **Judge Takeaway:** The system behaves as standard civil traffic infrastructure during non-emergency periods.

---

### Scenario 2: Emergency Call & Intelligent Dispatch
* **Action:** Operator triggers a simulated high-severity emergency call (Code Red: Acute Myocardial Infarction).
* **System Behavior:**
  * Emergency Call Module parses coordinates and calculates triage priority.
  * Priority Scheduler selects the closest capable ALS ambulance.
  * Ambulance terminal displays route and responds: state changes to `DISPATCHED` -> `EN_ROUTE_SCENE`.
  * Route Dashboard highlights calculated trajectory.
* **Judge Takeaway:** Dynamic unit assignment based on proximity and vehicle capability.

---

### Scenario 3: Patient Intake & Hospital Live Vitals Streaming
* **Action:** Ambulance reaches patient, crew begins transit to trauma center (state: `EN_ROUTE_HOSPITAL`), streaming physiological vitals.
* **System Behavior:**
  * Hospital Dashboard receives automated alert: "Inbound Code Red - ETA 4 mins".
  * Real-time ECG, heart rate, and blood pressure graphs render live on the hospital screen.
  * Patient vitals simulator triggers a sudden hypotensive crisis; hospital dashboard emits an immediate visual alarm.
  * ER doctor clicks "Prepare Cath Lab & Administer Epinephrine", which appears immediately on the ambulance terminal.
* **Judge Takeaway:** Seamless clinical handover and two-way telemedicine collaboration prior to hospital arrival.

---

### Scenario 4: Dynamic Green Corridor in Action
* **Action:** Ambulance approaches a congested signalized intersection on its route to the hospital.
* **System Behavior:**
  * GPS Tracking detects entry into the preemption trigger zone.
  * Dynamic Green Corridor issues a safe preemption command to the intersection controller.
  * The physical traffic signal transitions safely through yellow clearance, enforces all-red pedestrian clearance, and illuminates Green along the ambulance's approach.
  * Opposing directions are held on Red.
  * The moment the ambulance clears the intersection geofence, the signal releases preemption and returns smoothly to normal cycles.
* **Judge Takeaway:** Safe, deterministic, zero-delay intersection clearance with hardware relay switching.

---

### Scenario 5: Multi-Ambulance Convergence & Conflict Arbitration
* **Action:** Two ambulances (Ambulance 1: Code Red, Ambulance 2: Code Yellow) approach the same intersection simultaneously from perpendicular roads.
* **System Behavior:**
  * Multi Ambulance Coordinator detects trajectory conflict 12 seconds prior to arrival.
  * Priority Scheduler evaluates patient vitals and triage codes: Ambulance 1 is granted primary right-of-way.
  * Ambulance 2 receives an in-cab advisory: "Yielding Right of Way - Target Speed 25 km/h".
  * Signal holds green for Ambulance 1, then immediately switches green for Ambulance 2 once cleared.
* **Judge Takeaway:** Algorithmic resolution of multi-unit conflicts without human dispatcher intervention.

---

### Scenario 6: WAN Communication Blackout & V2I Fallback Recovery
* **Action:** While an ambulance is en route, the primary cellular/WAN connection is physically severed (Ethernet cable pulled).
* **System Behavior:**
  * Network Communication detects link drop; Safety Monitor flags communication warning.
  * Ambulance switches transparently to local store-and-forward caching (vitals/GPS buffered in RAM).
  * As the ambulance approaches the next intersection, it issues direct wireless **V2I ad-hoc beacons** (V2V Communication module).
  * The Traffic Signal Controller receives the direct beacon, verifies the cryptographic signature, and executes green preemption locally.
  * Upon WAN cable reconnection, all buffered logs and telemetry backfill to the central server without data loss.
* **Judge Takeaway:** Military-grade resilience and decentralized fail-safe operation during infrastructure outages.

---

## 17. Git Workflow & Version Control Strategy

To guarantee code integrity, traceability, and prevent breaking changes during rapid hackathon development, the following Git branching policy is strictly enforced:

```
[main] ───────────────────────────────────────────● (v1.0 Golden Master Release)
  ▲                                               │
  │ Pull Request (Full Integration Tests Pass)    │
[develop] ──────●───────────────●─────────────────┴───── (Continuous Integration)
  ▲             │               │
  │ PR          │ Branch        │ Branch
[feature/xxx] ──┘               ▼
                          [bugfix/xxx]
```

### 17.1 Branch Hierarchy
* `main`: Production-ready, fully tested golden master code only. **Direct commits to `main` are strictly forbidden.**
* `develop`: Active integration branch where completed features are merged and regression tests execute.
* `feature/<module-name>`: Dedicated feature development branches branched off `develop` (e.g., `feature/gps-tracking`, `feature/traffic-controller`).
* `bugfix/<issue-description>`: Focused defect remediation branches branched off `develop` or `main`.

### 17.2 Pull Request (PR) Policy
1. No branch may be merged into `develop` without:
   * Clean compilation across both native and target toolchains.
   * Accompanying unit/module test suites passing with 100% success.
   * Peer review approval by at least one other team engineer.
2. Commits must follow conventional commit naming:
   * `feat: implement NMEA checksum verification in GPS module`
   * `fix: prevent race condition during traffic light preemption hold`
   * `docs: update deployment steps in README`

---

## 18. Documentation Workflow

Engineering documentation is maintained continuously alongside code development. A module is **not considered complete** until its corresponding documentation has been updated and verified.

### 18.1 Continuous Documentation Updates
Upon completion of any module or major bugfix, the following artifacts must be updated in the same pull request:
* `CHANGELOG.md`: Record additions, changes, deprecations, and fixes following Keep a Changelog formatting with semantic versioning references.
* `README.md`: Update operational instructions, build flags, run commands, and dependency listings if new tools or libraries were introduced.
* `SYSTEM_ARCHITECTURE.md`: If any real-world edge case requires an update to interface schemas or communication channels, the architecture document must be formally amended.

### 18.2 API & Code Documentation
* All public functions, classes, and structures must include concise docstrings detailing parameters, return values, thread-safety guarantees, and exception behavior.
* Edge configurations (GPIO pin mappings, network ports, timeout constants) must be documented in centralized header/configuration references.

---

## 19. Estimated Sprint Timeline

The project is structured across a disciplined 7-Day sprint schedule leading directly into the hackathon submission.

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           DEVELOPMENT SPRINT                            │
├─────────┬───────────────────────────────────────────────────────────────┤
│ Day 1   │ Phase 1: Project Setup & Phase 2: Communication Backbone      │
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 2   │ Phase 3: Coordinator Core & Phase 4: Ambulance OBU Node       │
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 3   │ Phase 5: Traffic RSU Node & Phase 6: Hospital Dashboard       │
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 4   │ Phase 7: Traffic Dashboard & Phase 8: Priority Scheduler      │
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 5   │ Phase 9: Multi-Ambulance & Phase 10: V2V / V2I Ad-hoc Mesh    │
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 6   │ Phase 11: Safety Watchdogs & Phase 12: Logging & Phase 13: Int│
├─────────┼───────────────────────────────────────────────────────────────┤
│ Day 7   │ Phase 14: Chaos/HIL Testing & Phase 15: Demo Rig Packaging   │
└─────────┴───────────────────────────────────────────────────────────────┘
```

| Timeframe | Active Phases | Core Engineering Milestone |
|---|---|---|
| **Day 1** | Phase 1, Phase 2 | **M0 & M1:** Build system operational; secure pub/sub and serialization verified. |
| **Day 2** | Phase 3, Phase 4 | **M2:** Ambulance GPS tracking, vitals simulator, and path calculation running. |
| **Day 3** | Phase 5, Phase 6 | **M3:** Traffic light FSM switching physical relays; Hospital Dashboard rendering vitals. |
| **Day 4** | Phase 7, Phase 8 | **M4:** Traffic Dashboard live; Dynamic Green Corridor preemption verified end-to-end. |
| **Day 5** | Phase 9, Phase 10 | **M5:** Priority arbitration and ad-hoc V2V/V2I wireless beacon fallback functional. |
| **Day 6** | Phase 11, Phase 12, Phase 13 | **M6:** Watchdog monitors, fault recovery, structured logging, and full system integration. |
| **Day 7** | Phase 14, Phase 15 | **M7:** 100% test pass, chaos injection verified, demo video recorded, and hackathon packaging complete. |

---

## 20. Final Deliverables Inventory

Prior to hackathon submission, the SMART AEGIS engineering repository will contain the following verified assets:

1. **Production-Quality Source Codebase:**
   * Clean, modular, standards-compliant C++ / Python source code organized strictly by module.
   * Complete absence of monolithic code, memory leaks, or unhandled exceptions.
2. **Command-Line Interface (CLI) Tools:**
   * Autonomous CLI simulation drivers to trigger calls, generate traffic loads, and inject faults without requiring GUI interaction.
3. **Operational Dashboards:**
   * Full-stack Hospital Emergency Room Dashboard (Web/Qt).
   * Municipal Traffic Supervisory Console (Web/Qt).
   * Dispatcher Tactical Fleet & Route Map.
4. **Hardware Prototype Rig:**
   * Raspberry Pi / QNX embedded controller units wired to physical traffic signal LEDs/relays and serial GPS/sensor hardware.
5. **Architectural & Planning Documentation:**
   * Ratified `SYSTEM_ARCHITECTURE.md`.
   * Complete `DEVELOPMENT_PLAN.md`.
   * Updated `README.md` containing single-command setup instructions.
   * Comprehensive `CHANGELOG.md` tracking development history.
6. **Testing & Quality Assurance Report:**
   * Formal test execution logs covering Unit, Module, Hardware, and Chaos/Fault-injection results.
7. **Demonstration Video & Media:**
   * High-definition walkthrough video demonstrating all 6 presentation scenarios running on physical hardware.
8. **Presentation Slide Deck:**
   * Professional technical pitch deck explaining architecture, safety assurances, QNX utilization, and municipal deployment economics.
9. **Clean Git Repository:**
   * Impeccable commit history adhering to semantic commit rules, zero temporary artifacts, and clearly tagged milestone releases (`v1.0-release`).

---

## 21. Requirement Traceability Matrix (RTM)

| Requirement ID | Module Name | Architecture Reference | Dev Phase | Primary Test Method | Milestone |
|---|---|---|---|---|---|
| **REQ-001** | Emergency Call Module | Sec 1 | Phase 3 | `UT-CALL-01`, `IT-CALL-01` | M1 |
| **REQ-002** | Ambulance Management | Sec 2 | Phase 3 | `UT-AMB-01`, `IT-AMB-01` | M1 |
| **REQ-003** | Route Planning | Sec 3 | Phase 3 | `UT-ROUTE-01`, `IT-ROUTE-01` | M1 |
| **REQ-004** | GPS Tracking | Sec 4 | Phase 4 | `UT-GPS-01`, `HT-GPS-01` | M2 |
| **REQ-005** | GPS Tracking (EKF) | Sec 4 | Phase 4 | `UT-GPS-02`, `HT-GPS-02` | M2 |
| **REQ-006** | Dynamic Green Corridor | Sec 5 | Phase 3 | `UT-CORR-01`, `IT-CORR-01` | M4 |
| **REQ-007** | Traffic Signal Controller | Sec 6 | Phase 5 | `UT-SIG-01`, `HT-SIG-01` | M4 |
| **REQ-008** | Traffic Signal Controller | Sec 6 | Phase 5, 11 | `UT-SIG-02`, `HT-SIG-02`, `CT-SIG-02` | M6 |
| **REQ-009** | Patient Vitals Module | Sec 10 | Phase 4 | `UT-VIT-01`, `HT-VIT-01` | M2 |
| **REQ-010** | Patient Vitals Module | Sec 10 | Phase 4 | `UT-VIT-02`, `IT-VIT-02` | M2 |
| **REQ-011** | Hospital Dashboard | Sec 7 | Phase 6 | `UT-HOSP-01`, `IT-HOSP-01` | M3 |
| **REQ-012** | Traffic Dashboard | Sec 8 | Phase 7 | `UT-TRF-01`, `IT-TRF-01` | M3 |
| **REQ-013** | Route Dashboard | Sec 9 | Phase 7 | `UT-RDSH-01`, `IT-RDSH-01` | M3 |
| **REQ-014** | Priority Scheduler | Sec 11 | Phase 8 | `UT-PRI-01`, `IT-PRI-01` | M5 |
| **REQ-015** | Multi Ambulance Coordinator| Sec 12 | Phase 9 | `UT-MAC-01`, `IT-MAC-01` | M5 |
| **REQ-016** | V2V Communication | Sec 13 | Phase 10 | `UT-V2V-01`, `HT-V2V-01` | M5 |
| **REQ-017** | V2V Communication (V2I) | Sec 13 | Phase 10 | `UT-V2I-01`, `HT-V2I-01` | M6 |
| **REQ-018** | Police Notification | Sec 14 | Phase 12 | `UT-POL-01`, `IT-POL-01` | M4 |
| **REQ-019** | Logging | Sec 15 | Phase 12 | `UT-LOG-01`, `PT-LOG-01` | M1 |
| **REQ-020** | Authentication | Sec 16 | Phase 2 | `UT-SEC-01`, `IT-SEC-01` | M1 |
| **REQ-021** | Authentication (Anti-Replay)| Sec 16 | Phase 2 | `UT-SEC-02`, `IT-SEC-02` | M1 |
| **REQ-022** | Safety Monitor (WDT) | Sec 17 | Phase 11 | `UT-SAF-01`, `HT-SAF-01`, `CT-SAF-01` | M6 |
| **REQ-023** | Network Communication | Sec 18 | Phase 2, 4 | `UT-NET-01`, `IT-NET-01`, `CT-NET-01` | M1, M6 |
| **REQ-024** | Configuration Manager | Sec 19 | Phase 1 | `UT-CFG-01`, `IT-CFG-01` | M0, M1 |
| **REQ-025** | RTOS Priority Scheduling | Sec 6, 17 | Phase 5, 11 | `UT-RTOS-01`, `HT-RTOS-01` | M4, M6 |
