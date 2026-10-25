# SMART AEGIS: Software Design Document (SDD)
**Document Identifier:** SDD-AEGIS-2026-V1.0  
**Classification:** Mission-Critical Cyber-Physical System (CPS)  
**Applicable Standards:** IEEE 1016-2009 (Software Design Descriptions), ISO 26262 (ASIL-B), POSIX.1-2017, MISRA C++:2008 / AUTOSAR C++14 Guidelines  
**Source Baseline References:**  
* [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md)  
* [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md)  
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md)  
* [`INTERFACE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/INTERFACE_SPECIFICATION.md)  

---

## 1. Introduction

### 1.1 Purpose
This Software Design Document (SDD) provides the complete technical design and implementation blueprint for **SMART AEGIS** (Automated Emergency Grid & Intelligent Shield). It translates the functional requirements, architectural models, and interface constraints established in previous specifications into concrete object models, thread schedules, memory layout policies, and execution sequences.

This document serves as the authoritative implementation guide for embedded engineers, distributed systems developers, and quality assurance personnel. **Deviation from the architectural designs, class structures, or threading models outlined herein is strictly prohibited.**

### 1.2 Scope
The scope of this document encompasses all software components comprising SMART AEGIS:
* **The Deterministic Edge Layer:** QNX Neutrino RTOS / POSIX microkernel services executing on physical Raspberry Pi SBCs for vehicle On-Board Units (OBU) and Roadside Units (RSU).
* **The Supervisory Coordination Plane:** Central call intake, A* graph routing, multi-criteria priority arbitration, and dynamic green corridor calculators.
* **The Distributed Communications Backbone:** High-throughput MQTT over TLS 1.3, decentralized direct wireless ad-hoc UDP beacons for V2X, and QNX synchronous message-passing primitives.
* **The Human-Machine Interfaces (HMI):** Hospital Emergency Intake, Municipal Traffic Supervision, and Tactical Route Dashboards.

### 1.3 Definitions, Acronyms, and Abbreviations
* **ACN:** Automated Crash Notification
* **ALS / BLS:** Advanced Life Support / Basic Life Support
* **BSM:** Basic Safety Message (SAE J2735 compliant V2X beacon)
* **C-ITS:** Cooperative Intelligent Transport Systems
* **CPS:** Cyber-Physical System
* **EKF:** Extended Kalman Filter
* **FSM:** Finite State Machine
* **HAM:** High Availability Manager (QNX)
* **HIL:** Hardware-in-the-Loop
* **ICD:** Interface Control Document
* **OBU:** On-Board Unit (Ambulance In-Vehicle Telematics Node)
* **RAII:** Resource Acquisition Is Initialization
* **resmgr:** QNX POSIX Resource Manager
* **RSU:** Roadside Unit (Traffic Signal Controller Node)
* **RTOS:** Real-Time Operating System
* **V2I:** Vehicle-to-Infrastructure
* **V2V:** Vehicle-to-Vehicle
* **WDT:** Watchdog Timer

### 1.4 References
1. IEEE Std 1016-2009: *IEEE Standard for Information Technology—Systems Design—Software Design Descriptions*.
2. ISO 26262-6:2018: *Road vehicles — Functional safety — Part 6: Product development at the software level*.
3. POSIX.1-2017: *IEEE Standard for Information Technology—Portable Operating System Interface (POSIX(R))*.
4. QNX Neutrino RTOS 7.1/8.0: *System Architecture Manual and Programmer's Guide*.

---

## 2. Overall Software Design

SMART AEGIS employs a **hierarchical layered architecture** combined with a **reactive event-driven pipeline** at the supervisory tier and a **deterministic cyclic executive** at the edge tier.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        TIER 4: SUPERVISORY & HMI                       │
│     [Hospital Dashboard]    [Traffic Dashboard]    [Route Dashboard]   │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ WebSockets / JSON-RPC
┌───────────────────────────────────▼────────────────────────────────────┐
│                    TIER 3: COORDINATION & ARBITRATION                  │
│  [Emergency Call] ──► [Priority Scheduler] ◄──► [Ambulance Management] │
│          │                    │                         │              │
│          ▼                    ▼                         ▼              │
│  [Route Planner] ◄──── [Multi-Ambulance] ────► [Dynamic Corridor]      │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ MQTT (TLS 1.3) / TCP
┌───────────────────────────────────▼────────────────────────────────────┐
│                TIER 2: DISTRIBUTED TRANSPORT & SECURITY                │
│    [Network Communication Manager]     [Authentication & Crypto]       │
└───────────────────┬───────────────────────────────┬────────────────────┘
                    │                               │
        WAN Wireless (LTE/5G)             WAN Wireless (LTE/5G)
                    │                               │
┌───────────────────▼───────────────┐   ┌───────────▼────────────────────┐
│   TIER 1A: AMBULANCE OBU (EDGE)   │   │   TIER 1B: TRAFFIC RSU (EDGE)  │
│ • [GPS Tracking & EKF Engine]     │   │ • [Traffic Signal Controller]  │
│ • [Patient Vitals Monitor]        │   │ • [Deterministic Phase FSM]    │
│ • [Local Store-Forward Queue]     │   │ • [Hardware Relay Resmgr]      │
│ • [V2V Ad-hoc Beacon Transmitter] │   │ • [Fail-Safe Amber Interlock]  │
└───────────────────▲───────────────┘   └───────────▲────────────────────┘
                    │     DIRECT WIRELESS AD-HOC    │
                    └──────── (UDP Port 8998) ──────┘
```

### 2.1 Architectural Invariants
1. **Safety Decoupling:** The edge safety layer (Tier 1B) operates autonomously. If Tier 2, Tier 3, or Tier 4 fails entirely, Tier 1B enforces legal phase transitions, all-red intervals, and fails safe into flashing amber.
2. **Determinism at the Edge:** Real-time threads on QNX nodes execute with bounded latencies, pre-allocated memory buffers, and priority inheritance mutexes.
3. **Data Freshness Over Buffering in Control Loops:** Kinematics and ad-hoc V2X frames favor the newest packet; stale queue items are dropped immediately to prevent dead-reckoning lag.

---

## 3. Namespace Organization

To prevent identifier collisions, clarify component ownership, and enforce clean encapsulation, the C++ codebase is partitioned into hierarchical namespaces under the master root namespace `aegis`:

```
aegis::
├── common::             # Fundamental primitives, units, and mathematical types
│   ├── types            # Structs, coordinates, kinematics, vitals
│   ├── error_codes      # 16-bit standardized error enumerations
│   ├── constants        # Physical, temporal, and port constants
│   └── math             # Haversine, bearing, matrix EKF routines
│
├── config::             # Configuration management and schema validation
│
├── logger::             # Logging abstractions and slogger2 backends
│
├── safety::             # Watchdog kickers, heartbeat supervisors, fail-safe FSM
│
├── network::            # Network transport, MQTT wrappers, UDP sockets
│   ├── mqtt             # Paho/Mosquitto client abstraction
│   ├── udp              # Broadcast/multicast ad-hoc sockets
│   └── auth             # JWT verification, HMAC signing, mTLS adapters
│
├── core::               # Central dispatch and planning engines
│   ├── dispatch         # Emergency call intake and fleet asset tracking
│   ├── routing          # A* graph navigation and intersection passage
│   ├── scheduler        # Priority evaluation and contention arbitration
│   └── corridor         # Preemption calculation and green corridor sequencing
│
└── edge::               # In-vehicle and roadside embedded hardware components
    ├── ambulance        # OBU node, GPS NMEA parser, vitals monitor, V2V
    └── traffic          # RSU node, traffic phase FSM, GPIO relay drivers
```

---

## 4. Folder Mapping

Every one of the 19 architecture modules maps deterministically to a designated repository location, ensuring strict header/source separation and single-responsibility isolation:

| Module Number & Name | Interface Header Directory | Implementation Source Directory | Configuration / Assets |
|---|---|---|---|
| **1. Emergency Call Module** | `include/aegis/core/` | `src/coordinator/` | `config/default.json` |
| **2. Ambulance Management** | `include/aegis/core/` | `src/coordinator/` | `config/default.json` |
| **3. Route Planning** | `include/aegis/core/` | `src/route_planner/` | `config/default.json` |
| **4. GPS Tracking** | `include/aegis/edge/` | `src/ambulance/` | `hardware/gps/` |
| **5. Dynamic Green Corridor** | `include/aegis/core/` | `src/coordinator/` | `config/default.json` |
| **6. Traffic Signal Controller**| `include/aegis/edge/` | `src/traffic_controller/` | `hardware/relay/`, `hardware/led_controller/` |
| **7. Hospital Dashboard** | N/A (UI Client) | `src/hospital_dashboard/` | `src/hospital_dashboard/index.html` |
| **8. Traffic Dashboard** | N/A (UI Client) | `src/traffic_dashboard/` | `src/traffic_dashboard/index.html` |
| **9. Route Dashboard** | N/A (UI Client) | `src/route_planner/` | `dashboards/route/` |
| **10. Patient Vitals Module** | `include/aegis/edge/` | `src/ambulance/` | `hardware/sensors/` |
| **11. Priority Scheduler** | `include/aegis/core/` | `src/priority_scheduler/` | `config/default.json` |
| **12. Multi Ambulance Coord** | `include/aegis/core/` | `src/coordinator/` | `config/default.json` |
| **13. V2V Communication** | `include/aegis/edge/` | `src/ambulance/` | `config/default.json` |
| **14. Police Notification** | `include/aegis/core/` | `src/coordinator/` | `config/default.json` |
| **15. Logging** | `include/aegis/logger/` | `src/logger/` | `config/default.json` |
| **16. Authentication** | `include/aegis/network/`| `src/network/` | `config/production.json` (certs) |
| **17. Safety Monitor** | `include/aegis/safety/` | `src/safety/` | `hardware/raspberry_pi/` |
| **18. Network Communication** | `include/aegis/network/`| `src/network/` | `config/default.json` |
| **19. Configuration Manager** | `include/aegis/config/` | `src/config/` | `config/` (all JSON files) |

---

## 5. Module Breakdown (All 19 Modules)

```
┌────────────────────────────────────────────────────────────────────────┐
│                    MODULE SPECIFICATION METRIC MATRIX                  │
│                                                                        │
│  19 Modules Defined Across:                                            │
│  • Core Dispatch & Planning (Modules 1, 2, 3, 5, 11, 12, 14)           │
│  • Vehicle Edge Telematics (Modules 4, 10, 13)                         │
│  • Roadside Traffic Control (Module 6)                                 │
│  • Supervisory HMI Consoles (Modules 7, 8, 9)                          │
│  • Cross-Cutting Infrastructure (Modules 15, 16, 17, 18, 19)           │
└────────────────────────────────────────────────────────────────────────┘
```

### Module 1: Emergency Call Module
* **Responsibility:** Ingest incoming 911 distress requests and automated crash notifications (ACN), validate geospatial limits, and assign baseline triage category.
* **Public Interfaces:** `ICallIntake` (`IngestCall()`, `GetActiveIncident()`).
* **Internal Components:** Call sanitization filter, geofence validator, triage rule engine.
* **Dependencies:** `aegis_common`, `aegis_config`, `aegis_logger`.
* **Thread Ownership:** Executes within the Coordinator Main Thread.
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** Synchronous in-process C++ calls; emits `EmergencyRequest` via MQTT QoS 2.
* **Error Handling Strategy:** Rejects invalid coordinates with `ERR_INVALID_MESSAGE`; returns `std::nullopt`.
* **Configuration Files Used:** `config/*.json` (`system.geofence`).
* **Logging Strategy:** Logs call intake at `INFO`; logs boundary rejections at `WARN`.

### Module 2: Ambulance Management
* **Responsibility:** Track fleet lifecycle states (`AVAILABLE` -> `DISPATCHED` -> `EN_ROUTE_PATIENT` -> `PATIENT_PICKED` -> `EN_ROUTE_HOSPITAL` -> `ARRIVED`), manage capability profiles (ALS vs BLS), and enforce atomic reservation locks.
* **Public Interfaces:** `IFleetManager` (`ReserveUnit()`, `UpdateUnitState()`, `GetUnitProfile()`).
* **Internal Components:** Thread-safe vehicle registry, state transition validator, assignment mutex lock table.
* **Dependencies:** `aegis_common`, `aegis_logger`.
* **Thread Ownership:** Shared across coordinator worker pool; state protected by mutex.
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** In-process mutex-locked queries; publishes `AmbulanceStateUpdate` via MQTT QoS 2.
* **Error Handling Strategy:** Race conditions during reservation return `false`; caller re-queries.
* **Configuration Files Used:** Fleet registry definitions in `config/*.json`.
* **Logging Strategy:** Logs all vehicle state transitions at `INFO` with monotonic timestamps.

### Module 3: Route Planning
* **Responsibility:** Compute optimal shortest-time paths across municipal road graphs, factoring dynamic traffic impedance and signalized intersection sequences.
* **Public Interfaces:** `IRouteEngine` (`CalculateRoute()`, `RecalculateOnDiversion()`).
* **Internal Components:** Directed graph storage, A* heuristic pathfinder, intersection traversal serializer.
* **Dependencies:** `aegis_common`, `aegis_config`.
* **Thread Ownership:** Spawns asynchronous calculation worker threads (`std::async`) to avoid blocking dispatch.
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** Direct C++ struct returns (`NavigationRoute`).
* **Error Handling Strategy:** Graph disconnectivity raises `ERR_ROUTE_BLOCKED`; falls back to straight-line azimuth.
* **Configuration Files Used:** Road network topology profiles.
* **Logging Strategy:** Logs path generation time and waypoint count at `DEBUG`.

### Module 4: GPS Tracking
* **Responsibility:** Interface with physical GNSS hardware via UART, validate NMEA sentences, execute Extended Kalman Filter (EKF) dead-reckoning during GPS dropouts, and broadcast 10 Hz kinematics.
* **Public Interfaces:** `ITelematicsFeed` (`GetLatestKinematics()`, `RegisterKinematicsCallback()`).
* **Internal Components:** Serial port driver / QNX resmgr client, NMEA checksum validator, 6-DOF IMU EKF filter.
* **Dependencies:** `aegis_common`, `aegis_safety`.
* **Thread Ownership:** Dedicated high-priority edge thread (`GPS_Reader_Thread`).
* **Process Ownership:** `aegis_ambulance_node` process on Raspberry Pi/QNX.
* **Communication Method:** Reads `/dev/gps_nmea` or `/dev/ttyS0`; publishes `GPSUpdate` via MQTT QoS 0.
* **Error Handling Strategy:** NMEA frame loss > 1000ms raises `ERR_GPS_TIMEOUT`; switches to dead-reckoning.
* **Configuration Files Used:** `config/*.json` (`ambulance.gpsSampleRateHz`).
* **Logging Strategy:** Logs GPS fix loss and re-acquisition at `WARN`/`INFO`.

### Module 5: Dynamic Green Corridor
* **Responsibility:** Project ambulance arrival envelopes to upcoming intersections, calculate phase preemption trigger moments, issue preemption commands, and release corridors upon vehicle exit.
* **Public Interfaces:** `ICorridorService` (`EvaluateApproachingVehicles()`, `TriggerPreemption()`, `ReleaseCorridor()`).
* **Internal Components:** Spatial geofence monitor, preemption timing calculator, corridor lock registry.
* **Dependencies:** `aegis_common`, `aegis_network`, `aegis_route_planner`.
* **Thread Ownership:** Coordinator preemption evaluation loop (20 Hz periodic timer).
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** Publishes `TrafficPreemptionRequest` via MQTT QoS 2.
* **Error Handling Strategy:** Unresponsive intersection triggers secondary retry and alerts driver.
* **Configuration Files Used:** `config/*.json` (`traffic.yellowClearanceMinMs`, `traffic.maxPreemptionHoldMs`).
* **Logging Strategy:** Logs preemption requests and releases at `INFO` with corridor UUIDs.

### Module 6: Traffic Signal Controller
* **Responsibility:** Control physical traffic signal indicators deterministically, enforce legal phase transitions, guarantee minimum-yellow and all-red clearance intervals, and drop to flashing amber on failure.
* **Public Interfaces:** `ISignalController` (`RequestPreemption()`, `GetPhaseState()`, `FallbackToFailSafe()`).
* **Internal Components:** Deterministic phase FSM, hardware GPIO relay driver, safety conflict monitor.
* **Dependencies:** `aegis_common`, `aegis_safety`, `aegis_logger`.
* **Thread Ownership:** Dedicated real-time thread (`Traffic_FSM_Thread`) with high QNX priority (Priority 150).
* **Process Ownership:** `aegis_traffic_controller` process on Raspberry Pi / QNX.
* **Communication Method:** GPIO register access or `/dev/traffic_controller` resmgr; MQTT QoS 1 for telemetry.
* **Error Handling Strategy:** Conflicting green detected by interlock immediately forces `ERR_DUAL_GREEN_ABORT` and drops hardware relays to flashing amber.
* **Configuration Files Used:** `config/*.json` (`traffic.*`).
* **Logging Strategy:** Logs every phase transition and preemption trigger at `INFO`; faults at `FATAL`.

### Module 7: Hospital Dashboard
* **Responsibility:** Render real-time incoming ambulance telemetry, live vital waveforms (ECG/SpO2/BP), dynamic ETA countdowns, and transmit physician pre-arrival orders back to the ambulance.
* **Public Interfaces:** Web / Qt GUI client; consumes MQTT WebSockets.
* **Internal Components:** Waveform renderer, countdown timer, clinical order dispatcher.
* **Dependencies:** WebSockets client, charting library (Chart.js / Qt QML).
* **Thread Ownership:** Browser UI rendering thread or Qt GUI main event loop.
* **Process Ownership:** Independent client process on hospital workstation.
* **Communication Method:** Subscribes to `aegis/fleet/+/vitals` and `aegis/fleet/+/alarms` via WebSockets.
* **Error Handling Strategy:** WebSocket severance displays visual reconnect banner; backfills upon reconnection.
* **Configuration Files Used:** Web client environment configurations.
* **Logging Strategy:** Logs user interactions and clinical orders to central audit trail.

### Module 8: Traffic Dashboard
* **Responsibility:** Display citywide map of all signalized intersections, active preemption corridors, and enable authenticated manual phase holds/overrides.
* **Public Interfaces:** Web / Qt GUI client; consumes MQTT WebSockets.
* **Internal Components:** GIS map canvas (Leaflet / Mapbox), signal state indicator, override token validator.
* **Dependencies:** WebSockets client, geospatial rendering engine.
* **Thread Ownership:** Browser UI rendering thread or Qt GUI main event loop.
* **Process Ownership:** Independent client process on traffic authority console.
* **Communication Method:** Subscribes to `aegis/traffic/+/status`; publishes manual overrides via MQTT QoS 2.
* **Error Handling Strategy:** Displays warning banner if intersection fails to acknowledge override.
* **Configuration Files Used:** GIS map tile endpoints and intersection layout profiles.
* **Logging Strategy:** All manual overrides audited with operator ID and timestamp.

### Module 9: Route Dashboard
* **Responsibility:** Provide dispatchers and in-vehicle Mobile Data Terminals (MDT) with visual route navigation, live traffic congestion layers, and off-route recalculation notifications.
* **Public Interfaces:** Web / Qt GUI client; consumes MQTT WebSockets.
* **Internal Components:** Vector map renderer, breadcrumb tracking layer, route deviation detector.
* **Dependencies:** Geospatial rendering engine, vector tiles.
* **Thread Ownership:** Browser UI rendering thread or Qt GUI main event loop.
* **Process Ownership:** Independent client process on dispatch workstation / MDT.
* **Communication Method:** Subscribes to `aegis/fleet/+/location` and route topics.
* **Error Handling Strategy:** Lost tile server switches to offline vector cache.
* **Configuration Files Used:** Vector map caching profiles.
* **Logging Strategy:** Logs map viewport changes and recalculation clicks at `DEBUG`.

### Module 10: Patient Vitals Module
* **Responsibility:** Ingest continuous physiological vitals from biomedical monitoring equipment (or mock generator), detect acute deterioration events, and format streams.
* **Public Interfaces:** `IPatientMonitor` (`SampleVitals()`, `RegisterAlarmCallback()`).
* **Internal Components:** Sensor serial/BLE parser, threshold anomaly detector, ring-buffer cache.
* **Dependencies:** `aegis_common`, `aegis_logger`.
* **Thread Ownership:** Dedicated periodic sampling thread (`Vitals_Sampling_Thread`, 1 Hz).
* **Process Ownership:** `aegis_ambulance_node` process.
* **Communication Method:** Emits `VitalsUpdate` (1 Hz, QoS 1) and `HospitalAlert` (QoS 2) via MQTT.
* **Error Handling Strategy:** Zero-lead detection raises `ERR_VITALS_FAILURE`; alerts paramedics.
* **Configuration Files Used:** `config/*.json` (`ambulance.vitalsSampleRateHz`).
* **Logging Strategy:** Logs vitals alarms at `CRITICAL`; logs baseline stream statistics at `DEBUG`.

### Module 11: Priority Scheduler
* **Responsibility:** Dynamically calculate multi-factorial priority scores (triage category, vitals deterioration, transit distance) and arbitrate resource contention (corridors, hospital docks).
* **Public Interfaces:** `IPriorityArbiter` (`EvaluatePriority()`, `ArbitrateContention()`).
* **Internal Components:** Multi-criteria weighted decision matrix, priority queue, tie-breaking arbiter.
* **Dependencies:** `aegis_common`, `aegis_config`.
* **Thread Ownership:** In-process synchronous calls from Coordinator loop.
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** C++ method calls returning `PriorityDecision` structs.
* **Error Handling Strategy:** Missing vitals fall back to baseline triage category score.
* **Configuration Files Used:** Scoring weights defined in `config/*.json`.
* **Logging Strategy:** Logs every arbitration outcome and mathematical score breakdown at `INFO`.

### Module 12: Multi Ambulance Coordinator
* **Responsibility:** Detect spatial convergence between multiple active emergency units, predict trajectory intersections, and compute speed/stagger advisories to prevent collisions.
* **Public Interfaces:** `ICoordinationService` (`RegisterTrajectory()`, `DetectConflicts()`, `GetAdvisories()`).
* **Internal Components:** Spatio-temporal bounding box index, trajectory crossing predictor, stagger calculator.
* **Dependencies:** `aegis_common`, `aegis_priority_scheduler`, `aegis_route_planner`.
* **Thread Ownership:** Coordinator spatial evaluation thread (10 Hz periodic timer).
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** Emits advisories to `aegis/fleet/+/advisories` via MQTT QoS 2.
* **Error Handling Strategy:** Out-of-order GPS telemetry triggers timestamp sorting before conflict evaluation.
* **Configuration Files Used:** Proximity threshold parameters in `config/*.json`.
* **Logging Strategy:** Logs conflict detections and speed advisories at `WARN`.

### Module 13: V2V Communication
* **Responsibility:** Broadcast autonomous 10 Hz Basic Safety Messages (BSM) over direct ad-hoc wireless sockets and transmit localized V2I preemption beacons directly to roadside units during cellular WAN outages.
* **Public Interfaces:** `IV2XBeacon` (`BroadcastSafetyBeacon()`, `SendDirectPreemption()`).
* **Internal Components:** POSIX UDP broadcast socket manager, neighbor proximity table, fallback trigger.
* **Dependencies:** `aegis_common`, `aegis_safety`.
* **Thread Ownership:** Dedicated low-latency communication thread (`V2X_Socket_Thread`).
* **Process Ownership:** `aegis_ambulance_node` process.
* **Communication Method:** Raw UDP datagrams on Port 8999 (V2V) and Port 8998 (V2I).
* **Error Handling Strategy:** Socket send failure raises network warning; continues retrying periodically.
* **Configuration Files Used:** `config/*.json` (`network.v2vUdpPort`, `network.v2iUdpPort`).
* **Logging Strategy:** Logs peer beacon reception statistics at `DEBUG`.

### Module 14: Police Notification
* **Responsibility:** Format and dispatch real-time perimeter clearance requests and route schedules to municipal law enforcement Computer-Aided Dispatch (CAD) systems upon corridor formation.
* **Public Interfaces:** `IPoliceAlertService` (`DispatchPerimeterNotice()`).
* **Internal Components:** Inter-agency JSON/CAP serializer, webhook client.
* **Dependencies:** `aegis_common`, `aegis_network`.
* **Thread Ownership:** Asynchronous worker task spawned on corridor activation.
* **Process Ownership:** `aegis_coordinator` process.
* **Communication Method:** HTTPS POST / REST webhooks.
* **Error Handling Strategy:** Remote CAD gateway failure retries up to 3 times before logging alert.
* **Configuration Files Used:** Law enforcement endpoint configurations.
* **Logging Strategy:** Logs all dispatched notices and gateway acknowledgments at `INFO`.

### Module 15: Logging
* **Responsibility:** Provide centralized, high-throughput, non-blocking structured audit logging across native host environments and QNX target systems (`slogger2`).
* **Public Interfaces:** `ILogger` (`Log()`, `LogTrace()`, `LogInfo()`, `LogWarn()`, `LogError()`, `LogFatal()`).
* **Internal Components:** Lock-free circular ring buffer, background file writer, QNX `slogger2` wrapper.
* **Dependencies:** `aegis_common`.
* **Thread Ownership:** Dedicated background flushing thread (`Logger_Worker_Thread`).
* **Process Ownership:** Linked as a core library into all system processes.
* **Communication Method:** Non-blocking in-memory queue pushes; writes to disk / shared memory.
* **Error Handling Strategy:** Storage capacity > 90% triggers FIFO log drop of oldest non-critical logs.
* **Configuration Files Used:** `config/*.json` (`system.logLevel`).
* **Logging Strategy:** Self-contained logging service.

### Module 16: Authentication
* **Responsibility:** Validate node cryptographic certificates (mTLS), issue and verify HMAC signatures for V2X payloads, and enforce Role-Based Access Control (RBAC) tokens (JWT) on supervisory dashboards.
* **Public Interfaces:** `IAuthService` (`ValidateToken()`, `SignPayload()`, `VerifySignature()`).
* **Internal Components:** OpenSSL/mbedTLS cryptographic wrapper, JWT parser, HMAC-SHA256 validator.
* **Dependencies:** `aegis_common`, `aegis_config`.
* **Thread Ownership:** Synchronous in-process calls at communication ingress boundaries.
* **Process Ownership:** Central server and edge network layers.
* **Communication Method:** Cryptographic verification of packet metadata.
* **Error Handling Strategy:** Invalid signatures or expired tokens immediately reject request (`ERR_AUTH_TOKEN_EXPIRED`).
* **Configuration Files Used:** `config/production.json` (certificate and key paths).
* **Logging Strategy:** Logs all authentication failures and suspected replay attacks at `CRITICAL`.

### Module 17: Safety Monitor
* **Responsibility:** Monitor process heartbeats, service the physical Hardware Watchdog Timer (WDT) line, detect execution starvation, and initiate controlled fail-safe degradation routines.
* **Public Interfaces:** `ISafetySupervisor` (`RegisterHeartbeat()`, `KickWatchdog()`, `AssertFailSafe()`).
* **Internal Components:** Multi-process pulse listener, POSIX watchdog driver (`/dev/watchdog`), HAM bridge.
* **Dependencies:** `aegis_common`, `aegis_logger`.
* **Thread Ownership:** Dedicated highest-priority real-time thread (`Safety_Watchdog_Thread`, QNX Priority 200).
* **Process Ownership:** `aegis_safety_daemon` or integrated into edge controller processes.
* **Communication Method:** QNX Pulse channels (`0x01` WDT kick); `/dev/watchdog` file descriptor writes.
* **Error Handling Strategy:** If heartbeats cease > 500ms, watchdog driver times out, forcing hardware reset.
* **Configuration Files Used:** `config/*.json` (`system.heartbeatIntervalMs`, `system.watchdogTimeoutMs`).
* **Logging Strategy:** Logs keep-alive failures at `FATAL`.

### Module 18: Network Communication
* **Responsibility:** Abstract underlying network media, manage MQTT client connections with automated exponential backoff reconnects, and provide transparent store-and-forward buffering.
* **Public Interfaces:** `INetworkManager` (`Publish()`, `Subscribe()`, `IsConnected()`, `GetStoreForwardQueue()`).
* **Internal Components:** Paho/Mosquitto C++ wrapper, memory-backed ring buffer queue, reconnect state machine.
* **Dependencies:** `aegis_common`, `aegis_logger`, `aegis_config`.
* **Thread Ownership:** Dedicated network I/O loop thread (`Network_Event_Thread`).
* **Process Ownership:** Integrated into all communicating daemons.
* **Communication Method:** MQTT over TCP/TLS; POSIX TCP/IP networking stack.
* **Error Handling Strategy:** Connection loss transitions state to `DISCONNECTED`; routes packets to ring buffer.
* **Configuration Files Used:** `config/*.json` (`network.*`).
* **Logging Strategy:** Logs connection drops and reconnect events at `WARN`/`INFO`.

### Module 19: Configuration Manager
* **Responsibility:** Parse, validate against schema, and provide thread-safe cached access to static and dynamic configuration parameters across all nodes.
* **Public Interfaces:** `IConfigManager` (`LoadProfile()`, `GetInt()`, `GetString()`, `GetBool()`, `GetDouble()`).
* **Internal Components:** JSON schema parser, in-memory read-only configuration map, thread-safe cache.
* **Dependencies:** `aegis_common`.
* **Thread Ownership:** Loaded during bootstrap on main thread; read concurrently via const references.
* **Process Ownership:** Linked statically into all system processes.
* **Communication Method:** Direct C++ in-memory API.
* **Error Handling Strategy:** Missing or malformed config files abort process startup with descriptive console output.
* **Configuration Files Used:** `config/default.json`, `config/development.json`, `config/production.json`.
* **Logging Strategy:** Logs loaded profile name and active parameter overrides at `INFO`.

---

## 6. Class Design (Exhaustive C++ Declarations)

```
┌────────────────────────────────────────────────────────────────────────┐
│                        CLASS DESIGN ARCHITECTURE                       │
│                                                                        │
│   • Strict C++17 conformance                                           │
│   • RAII resource encapsulation (No raw pointers)                      │
│   • Explicit method signatures with const-correctness                  │
│   • Pimpl (Pointer-to-Implementation) idiom for driver decoupling      │
└────────────────────────────────────────────────────────────────────────┘
```

Below are the complete architectural class and struct definitions for core modules:

### 6.1 Common Types & Primitives (`aegis::common`)
```cpp
namespace aegis {
namespace common {

struct GeoCoordinate {
    double latitude;
    double longitude;
    double altitude;
};

struct VehicleKinematics {
    GeoCoordinate position;
    float speedKmH;
    float bearingDeg;
    float hdop;
    uint8_t fixQuality;
    uint64_t timestampMs;
};

struct PatientVitals {
    uint16_t heartRate;
    uint8_t spO2;
    uint16_t systolicBP;
    uint16_t diastolicBP;
    uint8_t respirationRate;
    uint8_t gcsScore;
    uint64_t timestampMs;
};

enum class AmbulanceState : uint8_t {
    AVAILABLE = 0,
    DISPATCHED,
    EN_ROUTE_PATIENT,
    PATIENT_PICKED,
    EN_ROUTE_HOSPITAL,
    ARRIVED,
    OFF_DUTY
};

enum class SignalPhase : uint8_t {
    RED = 0,
    YELLOW,
    GREEN,
    FLASHING_AMBER
};

enum class TriageCategory : uint8_t {
    CODE_GREEN = 0,
    CODE_AMBER,
    CODE_RED
};

} // namespace common
} // namespace aegis
```

### 6.2 Traffic Signal Controller (`aegis::edge::traffic`)
```cpp
namespace aegis {
namespace edge {
namespace traffic {

enum class ControllerFsmState : uint8_t {
    NORMAL_CYCLE = 0,
    PREEMPTION_PREPARE,
    GREEN_CORRIDOR,
    RESTORE,
    FAILSAFE_FLASHING_AMBER
};

class GpioInterface {
public:
    GpioInterface();
    ~GpioInterface();

    bool Initialize();
    void SetLampState(common::SignalPhase northSouth, common::SignalPhase eastWest);
    bool VerifyHardwareInterlock() const;

private:
    int m_gpioFd{-1};
    bool m_hardwareHealthy{false};
};

class TrafficController {
public:
    TrafficController();
    ~TrafficController();

    bool Initialize(const std::string& intersectionId, uint32_t yellowClearanceMs);
    void Start();
    void Stop();
    
    bool HandlePreemptionRequest(const std::string& unitId, common::SignalPhase requestedPhase, uint32_t holdDurationSec);
    void ReleasePreemption();
    void FallbackToFailSafe();
    
    ControllerFsmState GetCurrentState() const;
    common::SignalPhase GetNorthSouthPhase() const;
    common::SignalPhase GetEastWestPhase() const;

private:
    void ExecutePhaseCycle();
    void TransitionTo(ControllerFsmState newState);

    std::string m_intersectionId;
    uint32_t m_yellowClearanceMs{3000};
    uint32_t m_allRedClearanceMs{1000};
    ControllerFsmState m_state{ControllerFsmState::NORMAL_CYCLE};
    common::SignalPhase m_nsPhase{common::SignalPhase::RED};
    common::SignalPhase m_ewPhase{common::SignalPhase::RED};
    
    std::string m_activePreemptionUnit;
    uint64_t m_preemptionHoldDeadlineMs{0};
    
    GpioInterface m_gpio;
    mutable std::mutex m_stateMutex;
    std::atomic<bool> m_isRunning{false};
};

} // namespace traffic
} // namespace edge
} // namespace aegis
```

### 6.3 GPS Tracking & Telematics (`aegis::edge::ambulance`)
```cpp
namespace aegis {
namespace edge {
namespace ambulance {

class GpsTracker {
public:
    GpsTracker();
    ~GpsTracker();

    bool Initialize(const std::string& serialPortPath, uint32_t baudRate);
    void Start();
    void Stop();

    common::VehicleKinematics GetLatestKinematics() const;
    bool ParseNmeaSentence(std::string_view nmeaSentence);

private:
    void ReadLoop();
    void UpdateDeadReckoning();

    int m_serialFd{-1};
    common::VehicleKinematics m_latestKinematics;
    uint64_t m_lastGpsFixTimestampMs{0};
    
    mutable std::mutex m_telemetryMutex;
    std::atomic<bool> m_isRunning{false};
    std::thread m_workerThread;
};

class VitalsMonitor {
public:
    VitalsMonitor();
    ~VitalsMonitor();

    bool Initialize(const std::string& sensorDevicePath);
    void Start();
    void Stop();

    common::PatientVitals GetLatestVitals() const;
    bool DetectAcuteDeterioration(const common::PatientVitals& vitals) const;

private:
    void SamplingLoop();

    common::PatientVitals m_currentVitals;
    mutable std::mutex m_vitalsMutex;
    std::atomic<bool> m_isRunning{false};
    std::thread m_samplingThread;
};

class V2vModule {
public:
    V2vModule();
    ~V2vModule();

    bool Initialize(uint16_t udpPort);
    void Start();
    void Stop();

    bool BroadcastSafetyBeacon(const common::VehicleKinematics& kinematics, common::AmbulanceState state);
    bool SendDirectPreemptionBeacon(const std::string& intersectionId, const std::string& unitId);

private:
    void ReceiveLoop();

    int m_socketFd{-1};
    uint16_t m_port{8999};
    std::atomic<bool> m_isRunning{false};
    std::thread m_rxThread;
};

class AmbulanceNode {
public:
    AmbulanceNode();
    ~AmbulanceNode();

    bool Initialize(const std::string& unitId, const std::string& configPath);
    void Start();
    void Stop();

private:
    void TelemetrySyncLoop();

    std::string m_unitId;
    common::AmbulanceState m_state{common::AmbulanceState::AVAILABLE};
    
    GpsTracker m_gpsTracker;
    VitalsMonitor m_vitalsMonitor;
    V2vModule m_v2vModule;
    
    std::atomic<bool> m_isRunning{false};
    std::thread m_syncThread;
};

} // namespace ambulance
} // namespace edge
} // namespace aegis
```

### 6.4 Priority Scheduler (`aegis::core::scheduler`)
```cpp
namespace aegis {
namespace core {
namespace scheduler {

struct TriageEvaluationInput {
    common::TriageCategory category;
    uint8_t spO2;
    uint16_t systolicBP;
    double distanceKm;
    uint32_t trafficDelaySec;
};

class PriorityScheduler {
public:
    PriorityScheduler();
    ~PriorityScheduler();

    float CalculatePriorityScore(const TriageEvaluationInput& input) const;
    std::string ArbitrateContention(const std::string& unitA, float scoreA,
                                   const std::string& unitB, float scoreB) const;

private:
    float m_weightCategory{0.40f};
    float m_weightVitals{0.35f};
    float m_weightDistance{0.15f};
    float m_weightDelay{0.10f};
};

} // namespace scheduler
} // namespace core
} // namespace aegis
```

### 6.5 Route Planning Engine (`aegis::core::routing`)
```cpp
namespace aegis {
namespace core {
namespace routing {

struct NavigationRoute {
    std::string routeId;
    std::vector<common::GeoCoordinate> waypoints;
    std::vector<std::string> intersectionIds;
    double estimatedTimeSec{0.0};
    double totalDistanceMeters{0.0};
};

class RoutePlanner {
public:
    RoutePlanner();
    ~RoutePlanner();

    bool LoadGraphTopology(const std::string& mapTopologyFile);
    NavigationRoute CalculateRoute(const common::GeoCoordinate& origin,
                                  const common::GeoCoordinate& destination) const;

private:
    class GraphImpl;
    std::unique_ptr<GraphImpl> m_graph;
};

} // namespace routing
} // namespace core
} // namespace aegis
```

### 6.6 Safety Monitor & Watchdog (`aegis::safety`)
```cpp
namespace aegis {
namespace safety {

class Watchdog {
public:
    Watchdog();
    ~Watchdog();

    bool Initialize(uint32_t timeoutMs);
    void Kick();

private:
    int m_wdtFd{-1};
    uint32_t m_timeoutMs{500};
};

class SafetyMonitor {
public:
    SafetyMonitor();
    ~SafetyMonitor();

    bool Initialize(uint32_t heartbeatIntervalMs, uint32_t watchdogTimeoutMs);
    void Start();
    void Stop();

    void RegisterHeartbeat(std::string_view moduleName);
    bool IsHealthy() const;

private:
    void SupervisorLoop();

    Watchdog m_watchdog;
    uint32_t m_intervalMs{200};
    std::unordered_map<std::string, uint64_t> m_heartbeatTimestamps;
    
    mutable std::mutex m_mutex;
    std::atomic<bool> m_isRunning{false};
    std::thread m_supervisorThread;
};

} // namespace safety
} // namespace aegis
```

---

## 7. Thread Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                        THREAD EXECUTION TOPOLOGY                       │
│                                                                        │
│  Thread Name             Priority   Policy        Periodicity  Stack   │
│  ───────────────────────────────────────────────────────────────────── │
│  Watchdog_Thread         200 (Max)  SCHED_FIFO    200 ms       32 KB   │
│  Traffic_FSM_Thread      150        SCHED_FIFO    10 ms (Firm) 64 KB   │
│  GPS_Reader_Thread       100        SCHED_FIFO    100 ms (10Hz)64 KB   │
│  V2X_Socket_Thread       90         SCHED_FIFO    10 ms        64 KB   │
│  Vitals_Sampling_Thread  50         SCHED_RR      1000 ms (1Hz)48 KB   │
│  Coordinator_Dispatch    30         SCHED_RR      Asynchronous 128 KB  │
│  Network_Event_Thread    20         SCHED_RR      Event Loop   128 KB  │
│  Logger_Worker_Thread    10 (Min)   SCHED_OTHER   Background   64 KB   │
└────────────────────────────────────────────────────────────────────────┘
```

### 7.1 Thread Specifications

1. **`Watchdog_Thread`:**
   * **Role:** Top-level supervisor. Checks pulse tables and pets `/dev/watchdog` register every 200ms.
   * **Scheduling:** Hard real-time (`SCHED_FIFO`), Priority 200. Cannot be preempted by application code.
2. **`Traffic_FSM_Thread`:**
   * **Role:** Advances the intersection phase state machine, monitors yellow-clearance timers, and drives physical relay lines.
   * **Scheduling:** Hard real-time (`SCHED_FIFO`), Priority 150. Executes on 10ms tick.
3. **`GPS_Reader_Thread`:**
   * **Role:** Reads serial NMEA stream from GNSS UART buffer, computes EKF updates, and updates vehicle kinematics cache.
   * **Scheduling:** Real-time (`SCHED_FIFO`), Priority 100. Executes on 100ms cycle (10 Hz).
4. **`V2X_Socket_Thread`:**
   * **Role:** Broadcasts and receives ad-hoc UDP datagrams on Ports 8998 and 8999.
   * **Scheduling:** Real-time (`SCHED_FIFO`), Priority 90. Non-blocking `select()` / `epoll()` loop.
5. **`Vitals_Sampling_Thread`:**
   * **Role:** Reads biomedical telemetry, executes threshold checking, and flags clinical alarms.
   * **Scheduling:** Soft real-time (`SCHED_RR`), Priority 50. Executes on 1,000ms cycle (1 Hz).
6. **`Coordinator_Dispatch_Thread`:**
   * **Role:** Ingests calls, triggers A* path calculations, and resolves priority arbitration.
   * **Scheduling:** Non-real-time (`SCHED_RR`), Priority 30. Event-driven queue processor.
7. **`Network_Event_Thread`:**
   * **Role:** Handles MQTT keep-alives, socket polling, TLS handshakes, and store-and-forward queue flushing.
   * **Scheduling:** Standard priority (`SCHED_RR`), Priority 20. Event-driven via Paho MQTT background thread.
8. **`Logger_Worker_Thread`:**
   * **Role:** Drains asynchronous in-memory ring buffers and flushes logs to local flash or `slogger2`.
   * **Scheduling:** Low-priority background (`SCHED_OTHER`), Priority 10. Runs during idle CPU slices.

---

## 8. Process Architecture

To achieve process-level fault isolation, SMART AEGIS is partitioned across discrete OS processes communicating via IPC and network interfaces:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        PROCESS ISOLATION TOPOLOGY                      │
│                                                                        │
│   HOST / DISPATCH NODE:                                                │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │ PID 101: aegis_coordinator                                      │   │
│   │   • Call Handler, Route Planner, Priority Scheduler, Corridor  │   │
│   └────────────────────────────────────────────────────────────────┘   │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │ PID 102: aegis_mqtt_broker (Mosquitto Daemon)                  │   │
│   └────────────────────────────────────────────────────────────────┘   │
│                                                                        │
│   AMBULANCE OBU (Raspberry Pi / QNX):                                  │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │ PID 201: aegis_ambulance_node                                  │   │
│   │   • GPS Tracker, Vitals Monitor, V2V Transceiver, Local Cache  │   │
│   └────────────────────────────────────────────────────────────────┘   │
│                                                                        │
│   TRAFFIC RSU (Raspberry Pi / QNX):                                    │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │ PID 301: aegis_traffic_controller                              │   │
│   │   • Phase FSM, Hardware Relays, Safety Interlock               │   │
│   └────────────────────────────────────────────────────────────────┘   │
│   ┌────────────────────────────────────────────────────────────────┐   │
│   │ PID 302: aegis_safety_daemon                                   │   │
│   │   • Watchdog Kicker, Process Supervisor, HAM Monitor           │   │
│   └────────────────────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────────────────────┘
```

### 8.1 Process Lifecycles & Crash Containment
* **Isolated Memory Spaces:** If `aegis_ambulance_node` suffers an unexpected crash, the physical `aegis_traffic_controller` is completely unaffected.
* **Process Watchdogs:** On QNX targets, the High Availability Manager (HAM) monitors PID 301. If the process terminates abnormally, HAM immediately executes the recovery rule, asserting flashing amber and restarting the process within 50ms.
* **Process Orchestration:** Daemons are managed via standard systemd unit files on Linux or `/etc/rc.sysinit` startup scripts on QNX Neutrino.

---

## 9. Design Patterns

The implementation leverages proven software engineering design patterns to address specific functional constraints:

| Design Pattern | Target Module / Component | Rationale & Architectural Application |
|---|---|---|
| **Singleton** | `Logger`, `ConfigManager` | Ensures a single authoritative source of configuration and log serialization per process. |
| **Factory** | `NetworkManager`, `SensorFactory` | Decouples creation of physical vs mock sensor drivers and TLS vs plaintext MQTT clients. |
| **Observer** | `EmergencyCallModule`, `VitalsMonitor` | Allows dashboard and logging sinks to subscribe to asynchronous alarm triggers. |
| **Publisher/Subscriber**| Distributed WAN (`NetworkManager`) | MQTT topic topology decouples publishers (OBU, RSU) from arbitrary subscribers (Dashboards). |
| **Strategy** | `RoutePlanner`, `PriorityScheduler` | Allows swapping A* algorithms with alternative routing heuristics or changing triage scoring formulas. |
| **State Machine** | `TrafficController`, `AmbulanceNode` | Deterministic modeling of operational phases with formal entry, exit, and guard assertions. |
| **Dependency Injection**| Unit Test Harnesses (`tests/*`) | Injects mock UART serial devices and mock network transports into controllers during testing. |
| **Repository** | `AmbulanceManagement` | Encapsulates storage, retrieval, and atomic locking of vehicle asset states. |
| **Adapter** | `GpsTracker`, `GpioInterface` | Adapts heterogeneous hardware interfaces (POSIX UART vs QNX resmgr) to unified C++ APIs. |
| **Command** | `TrafficPreemptionRequest`, `HospitalCommand` | Encapsulates actionable instructions as serializable, verifiable payload objects. |

---

## 10. Memory Management & Zero-Allocation Strategy

In mission-critical embedded systems, dynamic heap allocation is a primary source of non-deterministic latency (heap fragmentation, allocation locks).

```
┌────────────────────────────────────────────────────────────────────────┐
│                        MEMORY MANAGEMENT STRATEGY                      │
│                                                                        │
│   Non-Real-Time Path (Coordinator / Tools):                            │
│     • Modern C++ RAII (std::unique_ptr, std::shared_ptr)               │
│     • Standard containers (std::vector, std::string)                   │
│                                                                        │
│   Hard Real-Time Path (Traffic Controller / Safety Monitor / OBU):     │
│     • ZERO heap allocation during operational loops                    │
│     • Pre-allocated static object pools & circular ring buffers        │
│     • Stack allocation with bounded maximum stack frames (< 64 KB)     │
│     • Lock-free atomic shared memory for telemetry caches              │
└────────────────────────────────────────────────────────────────────────┘
```

### 10.1 Ownership Rules
* **Single Ownership:** Implemented via `std::unique_ptr<T>`. Transfer of ownership requires explicit `std::move()`.
* **Shared Ownership:** Restricted to multi-threaded telemetry sinks via `std::shared_ptr<const T>`. Pointers are passed as const references to prevent reference count churn.
* **Prohibited Idioms:** `malloc()`, `free()`, raw `new`, raw `delete`, and owning raw pointers (`T*`) are strictly forbidden.

### 10.2 Real-Time Pre-Allocation
In `TrafficController`, `GpsTracker`, and `SafetyMonitor`:
* All message buffers, NMEA sentence parsing buffers, and EKF state vectors are allocated during process `Initialize()`.
* Real-time execution loops operate exclusively over fixed-capacity memory blocks (e.g., `std::array<char, 256>`).

---

## 11. Exception and Error Propagation

To maintain deterministic execution across embedded RTOS targets, the system enforces a **Zero-Exception Policy** in edge modules.

```
┌────────────────────────────────────────────────────────────────────────┐
│                        ERROR PROPAGATION FLOW                          │
│                                                                        │
│   Operation Fails (e.g., UART Checksum Mismatch)                       │
│        │                                                               │
│        ▼                                                               │
│   Return Result<T, ErrorCode>::Err(ERR_001_GPS_TIMEOUT)                │
│        │                                                               │
│        ▼                                                               │
│   Caller Inspects ErrorCode via if (!result.IsOk())                    │
│        │                                                               │
│        ├──────────────────────────────┬────────────────────────────┐   │
│        ▼                              ▼                            ▼   │
│   Log to slogger2            Execute Local Fallback        Report to Safety│
│   (Level: WARN)              (IMU Dead-Reckoning)          Monitor Pulse   │
└────────────────────────────────────────────────────────────────────────┘
```

### 11.1 The `Result<T, ErrorCode>` Idiom
All fallible functions return an explicit, non-throwing `Result` structure:
```cpp
template <typename T, typename E>
class Result {
public:
    static Result Ok(T value);
    static Result Err(E error);

    bool IsOk() const;
    const T& Value() const;
    const E& Error() const;
};
```

### 11.2 Graceful Degradation Tiers
1. **Tier 1 (Transient Failure):** Packet loss over WAN. Action: Queue message in local ring buffer, retry via exponential backoff.
2. **Tier 2 (Sensor Degradation):** GPS lock lost. Action: Activate 6-DOF IMU dead-reckoning extrapolation.
3. **Tier 3 (Subsystem Crash):** Traffic coordinator unreachable. Action: Activate localized direct V2I RF beacon.
4. **Tier 4 (Critical Safety Trip):** Conflicting green command detected or WDT timeout. Action: Hardware relays de-energize to flashing amber.

---

## 12. Logging Architecture

SMART AEGIS implements an asynchronous, structured logging service that decouples log generation from physical I/O.

### 12.1 Log Severity Levels
1. `TRACE (0)`: High-frequency loop iterations, raw byte dumps.
2. `DEBUG (1)`: Algorithmic milestones, parsed NMEA fields.
3. `INFO  (2)`: State transitions, call intake, preemption triggers.
4. `WARN  (3)`: Recoverable anomalies, dead-reckoning fallbacks.
5. `ERR   (4)`: Unrecoverable module errors, dropped network links.
6. `FATAL (5)`: Safety interlock trips, watchdog timeouts, system aborts.

### 12.2 Structured Log Schema
Every log entry adheres to the following fixed-width binary / JSON structure:
```
[Timestamp: UTC Microseconds] [Level] [ModuleID] [ThreadID] Message
Example:
[1792790401100234] [INFO] [TRAFFIC_FSM] [TID:1002] Phase transition: NORTH_SOUTH_GREEN -> YELLOW_CLEARANCE
```

### 12.3 QNX `slogger2` Integration
On QNX targets, the logger writes directly into `slogger2` shared memory buffers, completely eliminating disk write latency from real-time threads.

---

## 13. Configuration Architecture

Configuration data is managed hierarchically across three profiles (`default.json`, `development.json`, `production.json`).

```
┌────────────────────────────────────────────────────────────────────────┐
│                      CONFIGURATION HIERARCHY                           │
│                                                                        │
│   Compile-Time Defaults (constexpr constants.hpp)                      │
│             │                                                          │
│             ▼                                                          │
│   Baseline Profile (config/default.json)                               │
│             │                                                          │
│             ▼                                                          │
│   Environment Overrides (config/development.json OR production.json)   │
│             │                                                          │
│             ▼                                                          │
│   Validated In-Memory ConfigManager Cache (Thread-Safe Access)         │
└────────────────────────────────────────────────────────────────────────┘
```

* **Schema Validation:** At startup, `ConfigManager` parses the target JSON file and validates required fields against strict data types.
* **Immutability Post-Init:** Once loaded, configuration values are treated as read-only const references, preventing race conditions.

---

## 14. Build Dependency Graph

The project builds via modular CMake targets, enforcing directed acyclic dependencies (DAG):

```
                       ┌──────────────────────┐
                       │     aegis_common     │
                       └──────────▲───────────┘
                                  │
         ┌────────────────────────┼────────────────────────┐
         │                        │                        │
┌────────┴────────┐      ┌────────┴────────┐      ┌────────┴────────┐
│  aegis_logger   │      │  aegis_config   │      │  aegis_safety   │
└────────▲────────┘      └────────▲────────┘      └────────▲────────┘
         │                        │                        │
         ├────────────────────────┴────────────────────────┤
         ▼                                                 ▼
┌─────────────────┐                               ┌─────────────────┐
│  aegis_network  │                               │ aegis_traffic_  │
└────────▲────────┘                               │   controller    │
         │                                        └─────────────────┘
         ├────────────────────────┬────────────────────────┐
         ▼                        ▼                        ▼
┌─────────────────┐      ┌─────────────────┐      ┌─────────────────┐
│aegis_coordinator│      │ aegis_route_    │      │ aegis_ambulance │
│                 │      │    planner      │      │      node       │
└─────────────────┘      └─────────────────┘      └─────────────────┘
```

---

## 15. UML-Style ASCII Architecture Diagrams

### 15.1 Component Diagram: Full Distributed System
```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                                CENTRAL SUPERVISORY PLANE                               │
│                                                                                        │
│  ┌───────────────────────┐       ┌───────────────────────┐       ┌──────────────────┐  │
│  │ EmergencyCallHandler  │──────►│   PriorityScheduler   │◄─────►│ AmbulanceManager │  │
│  └───────────┬───────────┘       └───────────┬───────────┘       └────────┬─────────┘  │
│              │                               │                            │            │
│              ▼                               ▼                            │            │
│  ┌───────────────────────┐       ┌───────────────────────┐                │            │
│  │     RoutePlanner      │──────►│ DynamicGreenCorridor  │                │            │
│  └───────────────────────┘       └───────────┬───────────┘                │            │
│                                              │                            │            │
│  ════════════════════════════════════════════╪════════════════════════════╪══════════  │
│                       CENTRAL MQTT BROKER (TLS Port 8883)                              │
│  ════════════════════════════════════════════╪════════════════════════════╪══════════  │
│         ▲                                    │                            │            │
│         │                                    │                            │            │
└─────────┼────────────────────────────────────┼────────────────────────────┼────────────┘
          │ (10Hz Kinematics)                  │ (Preemption Cmd)           │ (State)
          │                                    │                            │
┌─────────┴─────────────────────────────┐   ┌──┴────────────────────────────▼────────────┐
│      AMBULANCE OBU (Raspberry Pi)     │   │      TRAFFIC RSU (Raspberry Pi / QNX)      │
│                                       │   │                                            │
│  ┌────────────┐     ┌──────────────┐  │   │  ┌──────────────────────┐  ┌────────────┐  │
│  │ GpsTracker │     │VitalsMonitor │  │   │  │  TrafficController   │  │SafetyMon   │  │
│  └─────┬──────┘     └──────┬───────┘  │   │  │  (Deterministic FSM) │  │(Watchdog)  │  │
│        │                   │          │   │  └──────────┬───────────┘  └─────┬──────┘  │
│        ▼                   ▼          │   │             │                    │         │
│  ┌─────────────────────────────────┐  │   │             ▼                    ▼         │
│  │     V2V Communication Module    │  │   │  ┌──────────────────────┐  ┌────────────┐  │
│  └─────────────────┬───────────────┘  │   │  │ GpioInterface / Relays│  │/dev/watch- │  │
│                    │                  │   │  │ (R/Y/G Traffic Lamps)│  │   dog      │  │
└────────────────────┼──────────────────┘   │  └──────────────────────┘  └────────────┘  │
                     │                      └────────────────────────────────────────────┘
                     │ (Direct Ad-hoc UDP: 8998)
                     └─────────────────────────────────────────────────────────────┘
```

### 15.2 Sequence Diagram: Emergency Call to Green Corridor Activation
```
Caller            EmergencyCall      PrioritySched     RoutePlanner     DynamicCorridor   TrafficController
  │                     │                  │                 │                 │                 │
  │── SubmitCall() ────►│                  │                 │                 │                 │
  │                     │── Evaluate() ───►│                 │                 │                 │
  │                     │                  │── CalcRoute() ─►│                 │                 │
  │                     │                  │                 │── Waypoints ───►│                 │
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │    (Ambulance Approaches Zone)    │
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │                 │── PreemptCmd ──►│
  │                     │                  │                 │                 │   (Hold: 20s)   │
  │                     │                  │                 │                 │                 │ Enforce Yellow
  │                     │                  │                 │                 │                 │ Clearance (3s)
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │                 │                 │ Enforce All-Red
  │                     │                  │                 │                 │                 │ Clearance (1s)
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │                 │                 │ Illuminate GREEN
  │                     │                  │                 │                 │◄── StatusAck ───│ (Hold Phase)
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │    (Ambulance Clears Geofence)    │
  │                     │                  │                 │                 │                 │
  │                     │                  │                 │                 │── ReleaseCmd ──►│
  │                     │                  │                 │                 │                 │ Restore Normal
  │                     │                  │                 │                 │                 │ Phase Cycles
```

---

## 16. Complete File Inventory & Future Source Mapping

The following table provides the exhaustive directory-to-source-file mapping for all C++, Python, and Web files across the SMART AEGIS project:

| Source File Path | Header File Path | Primary Responsibility |
|---|---|---|
| `src/common/geo_utils.cpp` | `include/aegis/common/geo_utils.hpp` | Haversine distance, bearing, and geospatial math. |
| `src/config/config_manager.cpp` | `include/aegis/config/config_manager.hpp` | JSON schema parsing and configuration lookup. |
| `src/logger/logger.cpp` | `include/aegis/logger/logger.hpp` | Asynchronous structured log serialization. |
| `src/safety/watchdog.cpp` | `include/aegis/safety/watchdog.hpp` | Hardware watchdog timer (`/dev/watchdog`) driver. |
| `src/safety/safety_monitor.cpp`| `include/aegis/safety/safety_monitor.hpp` | Multi-module heartbeat monitor and supervisor. |
| `src/network/mqtt_client.cpp` | `include/aegis/network/mqtt_client.hpp` | Paho MQTT C++ wrapper with TLS and auto-reconnect. |
| `src/network/network_manager.cpp`| `include/aegis/network/network_manager.hpp` | Unified network manager and store-forward queue. |
| `src/priority_scheduler/priority_scheduler.cpp` | `include/aegis/core/priority_scheduler.hpp` | Multi-criteria triage ranking and arbitration. |
| `src/route_planner/route_planner.cpp` | `include/aegis/core/route_planner.hpp` | A* road graph pathfinder and intersection vectors. |
| `src/coordinator/emergency_call_handler.cpp` | `include/aegis/core/emergency_call_handler.hpp` | 911 call intake and incident qualification. |
| `src/coordinator/dynamic_green_corridor.cpp` | `include/aegis/core/dynamic_green_corridor.hpp` | Green corridor preemption calculator and releases. |
| `src/coordinator/coordinator.cpp` | `include/aegis/core/coordinator.hpp` | Central coordinator daemon main runtime loop. |
| `src/ambulance/gps_tracker.cpp` | `include/aegis/edge/gps_tracker.hpp` | NMEA parser and 6-DOF IMU dead-reckoning engine. |
| `src/ambulance/vitals_monitor.cpp` | `include/aegis/edge/vitals_monitor.hpp` | Biomedical vitals sampler and anomaly detector. |
| `src/ambulance/v2v_module.cpp` | `include/aegis/edge/v2v_module.hpp` | Ad-hoc UDP BSM broadcast and direct V2I beacon. |
| `src/ambulance/ambulance_node.cpp`| `include/aegis/edge/ambulance_node.hpp` | In-vehicle OBU main daemon runtime loop. |
| `src/traffic_controller/gpio_interface.cpp` | `include/aegis/edge/gpio_interface.hpp` | Relay hardware driver and interlock checker. |
| `src/traffic_controller/traffic_controller.cpp` | `include/aegis/edge/traffic_controller.hpp` | Deterministic signal phase FSM and preemption. |
| `src/hospital_dashboard/index.html` | N/A | Hospital ER admissions terminal and vitals chart. |
| `src/traffic_dashboard/index.html` | N/A | Municipal traffic command and manual override map. |
| `tools/simulator/gps_simulator/gps_simulator.py` | N/A | Virtual NMEA GNSS serial emitter over PTY. |
| `tools/simulator/traffic_simulator/traffic_simulator.py` | N/A | Array of virtual traffic signal controllers. |
| `tools/simulator/vitals_simulator/vitals_simulator.py` | N/A | Synthetic ECG/BP/SpO2 waveform generator. |
| `tools/simulator/dispatcher_simulator/dispatcher_simulator.py`| N/A | Mock 911 emergency call generator CLI. |
| `tools/simulator/ambulance_simulator/ambulance_simulator.py` | N/A | Virtual ambulance kinematics and path follower. |
| `tests/unit/test_common_geo.cpp` | N/A | Unit test for Haversine and bearing math. |
| `tests/integration/test_ipc_pipeline.cpp` | N/A | Integration test for IPC data pipelines. |
| `tests/simulation/test_sim_mission.cpp` | N/A | Headless end-to-end mission test harness. |
| `tests/hardware/test_gpio_timing.cpp` | N/A | Oscilloscope/logic analyzer timing test for relays. |

---

## 17. Coding & Engineering Standards

### 17.1 C++17 Standards
* Standard: Strict **ISO/IEC 14882:2017 (C++17)**.
* Compilation Flags: `-std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`.
* No non-standard extensions (`-fno-gnu-extensions`).

### 17.2 Naming Conventions
* **Types / Classes / Structs:** UpperCamelCase (e.g., `TrafficController`, `GeoCoordinate`).
* **Functions / Methods:** UpperCamelCase for public methods (e.g., `RequestPreemption()`), camelCase for internal helpers (e.g., `executeCycle()`).
* **Variables / Parameters:** camelCase (e.g., `unitId`, `holdDurationSec`).
* **Member Variables:** camelCase with `m_` prefix (e.g., `m_state`, `m_yellowClearanceMs`).
* **Constants / Enum Values:** ALL_CAPS with underscores (e.g., `HEARTBEAT_INTERVAL_MS`, `CODE_RED`).

### 17.3 Documentation Style (Doxygen)
All public classes, methods, and types must feature Doxygen blocks:
```cpp
/**
 * @brief Requests immediate traffic signal preemption along a specified approach.
 * @param unitId Unique identifier of requesting emergency vehicle.
 * @param requestedPhase Directional movement requested (e.g., NORTH_SOUTH_GREEN).
 * @param holdDurationSec Maximum duration in seconds to hold preemption.
 * @return True if preemption was safely scheduled; false if rejected by conflict interlock.
 * @pre unitId must not be empty; holdDurationSec must be between 5 and 60 seconds.
 * @post Traffic controller enters PREEMPTION_PREPARE state.
 */
bool RequestPreemption(const std::string& unitId, common::SignalPhase requestedPhase, uint32_t holdDurationSec);
```

---

## 18. Future Extension Hooks

The SMART AEGIS architecture reserves explicit decoupling hooks for future municipal expansions:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        RESERVED EXTENSION HOOKS                        │
│                                                                        │
│  Hook Identifier             Target Subsystem     Interface Protocol   │
│  ───────────────────────────────────────────────────────────────────── │
│  HOOK_POLICE_DISPATCH        PoliceNotification   REST / CAP Webhook   │
│  HOOK_FIRE_MUTUAL_AID        EmergencyCallModule  MQTT QoS 2 (Mutual)  │
│  HOOK_AI_TRAFFIC_PREDICTION  RoutePlanning        gRPC Stream (Port 51)│
│  HOOK_5G_CV2X_SIDELINK       V2vModule            IEEE 1609.2 / PC5    │
│  HOOK_AUTONOMOUS_VEHICLE     AmbulanceNode        ROS 2 / DDS Bridge   │
└────────────────────────────────────────────────────────────────────────┘
```

1. **Police & Fire Mutual Aid:** The `PoliceNotification` module includes abstract sink interfaces enabling instant integration of Fire Department structural response teams.
2. **AI Traffic Flow Prediction:** Route Planning supports pluggable cost estimators, allowing a machine-learning queue prediction engine to feed dynamic road weights via gRPC.
3. **5G NR C-V2X Sidelink (PC5):** `V2vModule` abstracts the underlying physical layer, permitting seamless upgrade from UDP Wi-Fi ad-hoc broadcast to 3GPP Release 16 PC5 Sidelink hardware.
4. **Autonomous Emergency Vehicles:** The kinematics and corridor preemption loops expose standard ROS 2 / DDS message formats for integration with Level 4 autonomous vehicle drive-by-wire platforms.

---
**END OF SOFTWARE DESIGN DOCUMENT**
