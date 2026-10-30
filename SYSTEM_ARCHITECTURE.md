# SMART AEGIS: System Architecture Specification

> [!IMPORTANT]
> **ARCHITECTURAL FREEZE DECLARATION (v1.0-hackathon-core):**  
> As of release `v1.0-hackathon-core`, the complete SMART AEGIS system architecture, topological layering, public APIs, message schemas, and hardware pin mappings are **FORMALLY FROZEN**.  
> The following 18 modules are designated as **Core Infrastructure Modules** and must NOT be modified in future commits except for critical bug fixes:
> 1. Coordinator (`src/coordinator/`)
> 2. Traffic Controller (`src/traffic_controller/`, `src/traffic_signal/`)
> 3. Route Planner (`src/route_planner/`, `src/replanning/`)
> 4. Green Corridor (`src/green_corridor/`, `src/green_wave/`)
> 5. Priority Scheduler (`src/priority_scheduler/`, `src/scheduler/`)
> 6. Distributed Scheduler (`src/distributed_signal/`, `src/distributed_consensus/`)
> 7. Safety Monitor (`src/safety/`, `src/hardware/HardwareWatchdog.cpp`)
> 8. Deadline Monitor (`src/corridor/CorridorDeadlineMonitor.cpp`)
> 9. QNX Resource Manager (`src/qnx/QnxResourceManager.cpp`)
> 10. QNX Message Passing (`src/qnx/QnxMessagePassing.cpp`)
> 11. QNX Pulse Manager (`src/qnx/QnxPulseManager.cpp`)
> 12. QNX Timer Service (`src/qnx/QnxTimerService.cpp`)
> 13. QNX Thread Scheduler (`src/qnx/QnxThreadScheduler.cpp`)
> 14. QNX Interrupt Manager (`src/qnx/QnxInterruptManager.cpp`)
> 15. QNX Diagnostics (`src/qnx/QnxDiagnostics.cpp`)
> 16. GPIO BSP (`src/bsp/RaspberryPi4BSP.cpp`)
> 17. Relay Controller (`src/hardware/RelayController.cpp`)
> 18. Network Layer (`network/`, `src/communication/`, `src/network/`)

## Document Overview
* **System Name:** SMART AEGIS (Automated Emergency Grid & Intelligent Shield)
* **Target Domain:** Distributed Ambulance Emergency Response & Preemption System
* **Operating Environments:** QNX Neutrino RTOS (Safety/Edge Nodes), Embedded Linux / Raspberry Pi (Gateway/Field Nodes), Distributed Cloud/Server & Client Workstations (Web/Qt)
* **Standard Conformance:** POSIX.1, ISO 26262 (Functional Safety Principles), IEEE 802.11p/V2X Architectural Alignment

---

## 1. Emergency Call Module

### 1. Purpose
Provides the primary ingestion and qualification interface for incoming public safety emergency requests, converting distress calls and automated alerts into structured emergency incidents.

### 2. Responsibilities
* Capture emergency call metadata (caller identity, reported location, emergency category, incident description).
* Perform initial triage code assignment based on standardized medical dispatch protocols.
* Validate and sanitize incident coordinates against geographic boundaries.
* Dispatch validated emergency events to the downstream assignment and scheduling pipeline.

### 3. Inputs
* Distress call data payloads (Caller Phone, Lat/Long or Address string, Incident Type, Initial Severity).
* Automated Crash Notification (ACN) telemetry packets.
* Dispatcher operator UI inputs.

### 4. Outputs
* Formatted `EmergencyIncidentEvent` containing UUID, timestamp, geocoordinates, triage level, and incident profile.
* Call acknowledgment and receipt status to the originating client/operator.

### 5. Communication with Other Modules
* Emits `EmergencyIncidentEvent` to **Priority Scheduler** and **Ambulance Management**.
* Sends audit records to **Logging**.
* Validates session tokens via **Authentication**.
* Queries active boundary profiles via **Configuration Manager**.

### 6. Hardware Dependencies
* None directly (operates at dispatch server / control station).

### 7. Software Dependencies
* QNX / POSIX Networking Stack.
* JSON / Protocol Buffer serialization libraries.
* Thread synchronization primitives (POSIX pthread / QNX sync).

### 8. Failure Conditions
* Ingestion buffer saturation under mass-casualty surge conditions.
* Malformed input payloads or invalid geocoding strings.
* Database/persistence unavailability during call intake.

### 9. Future Scalability
* Integration with Next-Gen 911 / eCall direct cellular SIP trunks.
* Natural Language Processing (NLP) pipeline integration for automated voice-to-triage translation.

---

## 2. Ambulance Management

### 1. Purpose
Tracks, manages, and maintains the lifecycle, operational readiness, equipment state, and crew capability profiles of all ambulance fleet units.

### 2. Responsibilities
* Maintain an in-memory and persistent registry of all ambulance assets.
* Track dynamic vehicle operational states (`AVAILABLE`, `DISPATCHED`, `EN_ROUTE_SCENE`, `ON_SCENE`, `EN_ROUTE_HOSPITAL`, `OFF_DUTY`, `MAINTENANCE`).
* Match incoming incident requirements with ambulance capability profiles (e.g., Advanced Life Support [ALS] vs. Basic Life Support [BLS], bariatric, neonatal).
* Process ambulance status transition updates and lock assignments to prevent double-dispatch.

### 3. Inputs
* Unit state transition requests from vehicle telematics terminals.
* Dispatch commands from the **Priority Scheduler**.
* Maintenance and manual status overrides from fleet operators.

### 4. Outputs
* Fleet state broadcast messages (`AmbulanceStateUpdate`).
* Unit assignment confirmations and reservation locks.
* Fleet availability metrics for supervisory displays.

### 5. Communication with Other Modules
* Receives target incident assignments from **Priority Scheduler**.
* Sends assigned unit ID and state transitions to **Multi Ambulance Coordinator**, **Route Planning**, and **Logging**.
* Receives connectivity health signals from **Safety Monitor**.
* Uses **Network Communication** for vehicle telemetry synchronization.

### 6. Hardware Dependencies
* None directly on the server; on edge vehicle units, interfaces with the vehicle ignition sensing line.

### 7. Software Dependencies
* Concurrent hash tables / state storage engine with ACID guarantees.
* State machine validation framework.

### 8. Failure Conditions
* Race condition leading to duplicate vehicle assignment.
* Desynchronization between physical vehicle status and database state due to WAN packet loss.
* Deadlock during atomic unit reservation.

### 9. Future Scalability
* Predictive maintenance forecasting using CAN bus telemetry.
* Dynamic multi-region fleet partitioning across metropolitan districts.

---

## 3. Route Planning

### 1. Purpose
Computes, optimizes, and dynamically recalculates navigational trajectories for assigned ambulances, balancing travel time, road geometry, live traffic congestion, and green corridor feasibility.

### 2. Responsibilities
* Ingest current vehicle position, incident coordinates, and target hospital destination.
* Compute the shortest-time path using weighted graph algorithms (e.g., A*, Contraction Hierarchies).
* Factor in dynamic road closures, traffic choke points, and intersection signal capabilities.
* Recalculate routes dynamically upon road blockage detection or dynamic corridor reroutes.

### 3. Inputs
* Origin and destination coordinates (`StartLatLong`, `EndLatLong`).
* High-definition road network topology and signalized intersection maps from **Configuration Manager**.
* Live traffic density indices and road closure feeds.
* Recalculation triggers from **GPS Tracking** (off-route alerts) and **Multi Ambulance Coordinator**.

### 4. Outputs
* Structured `NavigationRoute` containing an ordered waypoint sequence, estimated time of arrival (ETA), and traversed intersection IDs.
* Recalculation advisories upon path diversion.

### 5. Communication with Other Modules
* Delivers computed route vectors to **Dynamic Green Corridor**, **Ambulance Management**, and **Route Dashboard**.
* Emits recalculation notices to **Police Notification** and **Logging**.
* Validates clearance with **Multi Ambulance Coordinator**.

### 6. Hardware Dependencies
* High-throughput CPU/memory subsystem for low-latency graph traversal under high node counts.

### 7. Software Dependencies
* Graph topology engine (e.g., spatial routing library).
* Floating-point geospatial projection libraries (WGS84 / UTM).

### 8. Failure Conditions
* Routing engine timeout during route calculation under heavy graph updates.
* Graph disconnectivity (target node mathematically unreachable due to map constraints).
* Severe divergence between mathematical model and real-world road conditions.

### 9. Future Scalability
* Integration of machine learning-based historical speed prediction profiles.
* Micro-level lane-level routing for connected vehicle networks.

---

## 4. GPS Tracking

### 1. Purpose
Acquires, filters, validates, and propagates real-time geospatial kinematics (position, speed, heading, altitude) from the ambulance On-Board Unit (OBU).

### 2. Responsibilities
* Interface with physical GNSS hardware via standard NMEA-0183 / UBX protocols.
* Apply Kalman filtering / dead-reckoning algorithms to compensate for urban canyon signal reflection or temporary GPS loss.
* Compute kinematic telemetry (instantaneous velocity, bearing, acceleration vectors).
* Detect route divergence (cross-track error) and trigger route recalculation requests.

### 3. Inputs
* Raw GNSS serial bitstreams from physical receiver hardware.
* Inertial Measurement Unit (IMU) accelerometer/gyroscope feeds for dead-reckoning.
* Active trajectory waypoints from **Route Planning**.

### 4. Outputs
* High-confidence, sanitized `VehicleKinematicsPacket` (Latitude, Longitude, Altitude, Bearing, Speed, Covariance, Timestamp).
* Off-route divergence triggers.

### 5. Communication with Other Modules
* Sends kinematics data to **Dynamic Green Corridor**, **Route Planning**, **V2V Communication**, and **Route Dashboard**.
* Streams updates to **Safety Monitor** for geofence validation and speed compliance.
* Emits operational logs to **Logging**.
* Communicates across nodes via **Network Communication**.

### 6. Hardware Dependencies
* GNSS Receiver Module (e.g., u-blox NEO series or Raspberry Pi GPS HAT via UART/USB).
* 6-DOF IMU (e.g., via I2C/SPI) for dead-reckoning.

### 7. Software Dependencies
* QNX Serial Driver (`devc-ser8250` or `devc-serusb`) / POSIX `termios`.
* Mathematical matrix manipulation library for extended Kalman filtering (EKF).

### 8. Failure Conditions
* Complete GNSS antenna disconnect or hardware failure.
* Extended satellite blackout (tunnels, underground parking) exhausting dead-reckoning accuracy.
* Corrupted NMEA frames or buffer overrun in the serial driver.

### 9. Future Scalability
* Integration with Real-Time Kinematic (RTK) positioning for centimeter-level accuracy.
* Multi-constellation GNSS fusion (GPS, Galileo, GLONASS, BeiDou, NavIC).

---

## 5. Dynamic Green Corridor

### 1. Purpose
Coordinates and orchestrates adaptive traffic signal preemption along an ambulance's active path to create an uninterrupted, safe passage corridor while minimizing broader urban traffic gridlock.

### 2. Responsibilities
* Monitor live vehicle position relative to upcoming signalized intersections.
* Calculate dynamic preemption trigger windows based on ambulance velocity, queue dissipation rates, and safe phase transition times.
* Issue sequential `PreemptionRequest` commands to down-path traffic controllers.
* Issue `CorridorRelease` commands immediately as the ambulance clears the intersection detection zone.
* Coordinate corridor adjustments if an ambulance deviates from the planned path.

### 3. Inputs
* Live vehicle kinematics and timestamps from **GPS Tracking**.
* Active navigation waypoints and intersection sequence from **Route Planning**.
* Current signal phase status from **Traffic Signal Controller**.
* Priority resolution overrides from **Priority Scheduler** and **Multi Ambulance Coordinator**.

### 4. Outputs
* Targeted `IntersectionPreemptionCommand` (Intersection ID, Phase ID, Hold Duration, Verification Token).
* Corridor status broadcasts for visualization dashboards.
* Preemption termination signals (`ClearanceVerification`).

### 5. Communication with Other Modules
* Commands **Traffic Signal Controller** nodes.
* Receives prioritization decisions from **Multi Ambulance Coordinator**.
* Streams corridor telemetry to **Traffic Dashboard**, **Route Dashboard**, and **Police Notification**.
* Records state transitions in **Logging**.

### 6. Hardware Dependencies
* None directly (logical coordinator executed at dispatch or edge gateway).

### 7. Software Dependencies
* Precise timer subsystem (QNX POSIX timers / `timer_create` / pulses).
* Geospatial distance and geometric projection algorithms.

### 8. Failure Conditions
* Late preemption trigger due to network delay, causing the ambulance to encounter a red signal or trapped queue.
* Ghost corridor locking due to missed vehicle exit events.
* Contradictory corridor demands from overlapping emergency trajectories.

### 9. Future Scalability
* Cooperative Intelligent Transport Systems (C-ITS) protocol support (SPaT/MAP messages).
* Machine-learned queue clearance algorithms using live traffic camera feeds.

---

## 6. Traffic Signal Controller

### 1. Purpose
Acts as the authoritative, fail-safe edge controller executing signal state changes at individual physical road intersections, ensuring strict adherence to conflict-free phase transitions.

### 2. Responsibilities
* Maintain normal cycle operations (Fixed-time, Actuated, or Adaptive).
* Process authenticated, high-priority preemption requests from **Dynamic Green Corridor** or direct localized **V2V Communication**.
* Enforce hard safety rules: yellow clearance intervals, all-red clearance intervals, and physical interlock rules (forbidding simultaneous green across conflicting movements).
* Revert gracefully to normal or degraded flashing-amber operation upon timeout or communication loss.

### 3. Inputs
* Preemption requests from **Dynamic Green Corridor** via network.
* Direct V2I (Vehicle-to-Infrastructure) emergency beacon signals.
* Physical manual override switch inputs from on-site emergency personnel.
* Cycle timing configurations from **Configuration Manager**.

### 4. Outputs
* Hardware GPIO/Relay actuation signals driving physical traffic lamps (Red, Amber, Green).
* Real-time phase status notifications (`SignalPhaseState`).
* Preemption acknowledgment and conflict alarm alerts.

### 5. Communication with Other Modules
* Responds to **Dynamic Green Corridor** and **Safety Monitor**.
* Telemetry streamed to **Traffic Dashboard** via **Network Communication**.
* Logs hardware faults and phase changes to **Logging**.

### 6. Hardware Dependencies
* Raspberry Pi / QNX embedded controller board.
* Industrial GPIO interface cards / solid-state relay (SSR) modules / opto-isolated drivers.
* Hardware Watchdog Timer (WDT) line.

### 7. Software Dependencies
* QNX Resource Manager (`resmgr`) or Linux sysfs/libgpiod hardware abstraction.
* Real-time deterministic state machine runner with microsecond guarantees.

### 8. Failure Conditions
* Conflicting green command generated by software (must be caught by hardware interlock).
* Stuck preemption hold state due to dropped disconnect message.
* Physical relay welding or blown lamp detection.

### 9. Future Scalability
* NEMA TS2 / 170 / 2070 controller standard protocol support.
* Direct edge camera vision integration for emergency vehicle optical strobe recognition.

---

## 7. Hospital Dashboard

### 1. Purpose
Provides emergency department medical staff, triage nurses, and trauma team leaders with comprehensive pre-arrival intelligence regarding incoming patients and expected arrival schedules.

### 2. Responsibilities
* Present live arrival timelines and dynamic ETA countdowns for assigned ambulances.
* Display streamed patient vital parameters and trauma level trends in real time.
* Allow hospital personnel to acknowledge incoming transfers and assign trauma bays.
* Provide a communication channel for transmitting pre-arrival orders back to the ambulance crew.

### 3. Inputs
* Real-time streaming vitals packets from **Patient Vitals Module**.
* Dynamic ETA and distance updates from **Route Planning** and **GPS Tracking**.
* Incident categorization and notes from **Emergency Call Module**.
* Medical staff input commands (Bay assignment, Acceptance confirmation, Clinical advice).

### 4. Outputs
* Staff acknowledgment confirmations (`HospitalIntakeAck`).
* Trauma team allocation signals.
* Pre-arrival clinical instructions sent to the ambulance crew.

### 5. Communication with Other Modules
* Subscribes to telemetry from **Patient Vitals Module** and **Ambulance Management**.
* Transmits admission readiness status to **Ambulance Management**.
* Validates user permissions via **Authentication**.
* Subscribes to events over **Network Communication**.

### 6. Hardware Dependencies
* Workstation display hardware / Web client terminals / Touchscreen tablets.

### 7. Software Dependencies
* Qt GUI Framework (QML / C++) or Modern Web Client Stack (WebSockets / HTML5 / React).
* Data visualization charting libraries (for real-time ECG / SpO2 / blood pressure rendering).

### 8. Failure Conditions
* WebSocket connection severance leading to stale vital sign rendering.
* UI freeze under high-frequency telemetry ingest.
* Misallocation of patient data across concurrent emergency intake feeds.

### 9. Future Scalability
* Bi-directional WebRTC video conferencing between trauma surgeons and paramedics.
* Automated Electronic Health Record (EHR / HL7 / FHIR) ingestion pipeline.

---

## 8. Traffic Dashboard

### 1. Purpose
Delivers a supervisory situational awareness console for municipal traffic authorities, municipal transit controllers, and system operators.

### 2. Responsibilities
* Render an interactive city-scale map showing all signalized intersections and their live states.
* Highlight active dynamic green corridors and affected intersection groups.
* Alert traffic operators to network anomalies, preemption timeouts, or stuck phases.
* Provide authenticated emergency manual overrides to release or hold specific intersections.

### 3. Inputs
* Intersection state streams from all **Traffic Signal Controller** nodes.
* Active corridor status and path geofences from **Dynamic Green Corridor**.
* Operator override commands and parameter tuning inputs.

### 4. Outputs
* Visual rendering of traffic network states and timing diagrams.
* Manual control commands (`ManualPreemptionOverride`).
* System alert acknowledgments.

### 5. Communication with Other Modules
* Interacts directly with **Dynamic Green Corridor** and **Traffic Signal Controller**.
* Verifies operator clearance via **Authentication**.
* Forwards all operator interactions to **Logging**.
* Relies on **Network Communication** for network-wide state aggregation.

### 6. Hardware Dependencies
* Operator workstation displays and graphics processing units (GPUs).

### 7. Software Dependencies
* Qt (Desktop) or Web-based geospatial GIS rendering canvas (e.g., OpenLayers/Mapbox/Leaflet).
* High-frequency pub/sub client library.

### 8. Failure Conditions
* Rendering latency degradation as the number of monitored intersections scales.
* Transmission of unauthenticated or corrupt override signals.
* Disconnect from the central telemetry broker.

### 9. Future Scalability
* Integration with citywide CCTV camera feeds and automated incident detection systems.
* Automated traffic queue recovery modeling post-corridor collapse.

---

## 9. Route Dashboard

### 1. Purpose
Serves as the tactical navigation, GIS visualization, and fleet tracking interface for central dispatchers, supervisors, and vehicle terminal operators.

### 2. Responsibilities
* Visualize global fleet distribution, incident loci, and hospital coordinates simultaneously.
* Display active, planned, and alternative routes with color-coded traffic congestion overlays.
* Track route progress, off-route warnings, and intersection passage milestones.
* Enable dispatchers to issue manual route reroutes in case of unforeseen blockages.

### 3. Inputs
* Telemetry updates from **GPS Tracking**.
* Active paths and waypoints from **Route Planning**.
* Fleet assignments from **Ambulance Management**.
* Corridor milestones from **Dynamic Green Corridor**.

### 4. Outputs
* Visual UI representation of geospatial tracking data.
* Dispatcher reroute requests and path modification flags.

### 5. Communication with Other Modules
* Consumes data from **GPS Tracking**, **Route Planning**, and **Ambulance Management**.
* Sends manual path modifications to **Route Planning**.
* Authorizes operator credentials via **Authentication**.
* Submits user-initiated events to **Logging**.

### 6. Hardware Dependencies
* Dispatch console workstations / In-vehicle Mobile Data Terminals (MDT).

### 7. Software Dependencies
* Qt QML Location module or WebGL-accelerated geospatial mapping framework.
* Vector tile rendering engine.

### 8. Failure Conditions
* Tile-server unavailability resulting in loss of visual map context.
* Delayed rendering of high-frequency GPS position vectors.
* Memory leaks during extended, multi-day continuous dispatch operations.

### 9. Future Scalability
* 3D digital-twin elevation rendering for complex multi-level highway interchanges.
* Offline vector map caching for disaster resilient in-vehicle operations.

---

## 10. Patient Vitals Module

### 1. Purpose
Acquires, packages, encrypts, and transmits continuous physiological vital signs from in-ambulance patient monitoring equipment to receiving medical facilities.

### 2. Responsibilities
* Ingest streaming vitals (Heart Rate, ECG waveform, SpO2, Non-Invasive Blood Pressure, Respiration Rate, Body Temperature).
* Normalize proprietary biomedical data formats into standard structured payloads.
* Detect threshold anomalies (e.g., severe arrhythmia, catastrophic pressure drop) and flag critical priority alerts.
* Buffer vitals data during cellular network dropouts and execute lossless backfill upon reconnect.

### 3. Inputs
* Serial / Bluetooth / Ethernet data streams from biomedical telemetry sensors or physical patient simulators.
* Manual paramedic diagnostic inputs and Glasgow Coma Scale (GCS) entries.

### 4. Outputs
* High-frequency `PatientVitalsStreamPacket`.
* Critical event alerts (`VitalsAlarmNotification`).
* Locally cached vital audit trails.

### 5. Communication with Other Modules
* Streams formatted packets to **Hospital Dashboard** via **Network Communication**.
* Transmits severity escalation signals to **Priority Scheduler** to upgrade transit priority dynamically.
* Emits clinical data audit points to **Logging** (sanitized of PII).
* Monitored for health and sensor contact integrity by **Safety Monitor**.

### 6. Hardware Dependencies
* In-ambulance edge compute node (Raspberry Pi running QNX or Linux).
* Biomedical sensor interface hardware (UART, USB-to-Serial, Bluetooth Low Energy, or Analog-to-Digital Converters).

### 7. Software Dependencies
* QNX Serial / Bluetooth protocol drivers.
* Cryptographic encryption libraries (TLS 1.3 / AES-256) for HIPAA-compliant data in transit.

### 8. Failure Conditions
* Physical sensor detachment from the patient.
* Hardware serial buffer overrun during burst medical telemetry.
* Local storage corruption on edge flash media during network blackout buffering.

### 9. Future Scalability
* AI-driven on-edge early deterioration detection algorithms (e.g., automated NEWS2 scoring).
* Direct 12-lead ECG real-time telemetry streaming to interventional cardiology cath labs.

---

## 11. Priority Scheduler

### 1. Purpose
Acts as the central algorithmic arbitration engine, dynamically computing and assigning execution priorities to calls, ambulances, and preemption rights across the system.

### 2. Responsibilities
* Evaluate incident criticality using multi-factorial metrics (Triage code, patient vital deterioration, transit time, traffic conditions).
* Rank competing emergency calls and allocate the most appropriate available units.
* Arbitrate resource contention when multiple ambulances require conflicting green corridors or hospital bays.
* Escalate or de-escalate vehicle mission priority levels in real time based on changing clinical conditions.

### 3. Inputs
* Unassigned incident events from **Emergency Call Module**.
* Fleet availability matrices from **Ambulance Management**.
* Clinical severity alarms from **Patient Vitals Module**.
* Traffic network load metrics from **Dynamic Green Corridor**.

### 4. Outputs
* Deterministic unit dispatch decisions (`DispatchOrder`).
* Relative corridor priority scores (`CorridorPriorityRank`).
* Preemption arbitration decisions for crossing paths.

### 5. Communication with Other Modules
* Commands **Ambulance Management** to reserve and deploy resources.
* Instructs **Dynamic Green Corridor** on priority rankings at contested intersections.
* Directs arbitration outcomes to **Multi Ambulance Coordinator**.
* Logs every scheduling decision with mathematical justification to **Logging**.

### 6. Hardware Dependencies
* None directly (server-side scheduling core).

### 7. Software Dependencies
* Deterministic mathematical optimization solver or priority-queue arbitration framework.
* High-resolution system time library.

### 8. Failure Conditions
* Computational deadlock when evaluating circular contention dependencies.
* Starvation of lower-triage incidents during prolonged high-demand emergencies.
* Floating-point precision anomalies in priority weight calculation.

### 9. Future Scalability
* Reinforcement learning integration for city-wide predictive asset pre-positioning.
* Multi-agency prioritization (joint fire, police, and EMS mutual aid arbitration).

---

## 12. Multi Ambulance Coordinator

### 1. Purpose
Manages spatial and temporal inter-dependencies when multiple emergency vehicles operate concurrently in the same geographic sector or converge on the same incident.

### 2. Responsibilities
* Monitor proximity vectors between multiple active ambulances.
* Detect trajectory crossing points and calculate collision / intersection arrival conflicts.
* Enforce spatial convoy intervals during mass-casualty responses to prevent rear-end collisions or intersection clustering.
* Distribute arriving units across multiple staging sectors or hospital receiving docks.

### 3. Inputs
* Real-time kinematics of all active units from **GPS Tracking**.
* Planned trajectories and ETA vectors from **Route Planning**.
* Priority rankings from **Priority Scheduler**.

### 4. Outputs
* Deconfliction directives (`SpeedAdjustmentAdvisory`, `StagingInstruction`, `IntersectionYieldOrder`).
* Convoy spacing constraints.

### 5. Communication with Other Modules
* Interfaces with **Priority Scheduler** to resolve crossing precedence.
* Directs path modifications to **Route Planning**.
* Feeds tactical conflict warnings to **V2V Communication** and **Dynamic Green Corridor**.
* Records tactical decisions in **Logging**.

### 6. Hardware Dependencies
* High-speed compute host for spatial bounding-box / trajectory intersection algorithms.

### 7. Software Dependencies
* Spatial indexing libraries (e.g., R-Tree, Geohash, or Quadtree algorithms).
* Vector mathematics and kinematic trajectory intersection logic.

### 8. Failure Conditions
* Latency-induced trajectory calculation errors leading to false collision alerts.
* Failure to deconflict due to out-of-order GPS updates.
* Unhandled edge cases in circular multi-vehicle arrival bottlenecks.

### 9. Future Scalability
* Autonomous vehicle (AV) cooperative adaptive cruise control (CACC) integration.
* Dynamic air-ambulance (eVTOL / Helicopter) landing zone coordination.

---

## 13. V2V Communication

### 1. Purpose
Provides autonomous, ultra-low-latency vehicle-to-vehicle (and direct vehicle-to-infrastructure) communication between mobile nodes without mandatory dependency on cellular infrastructure.

### 2. Responsibilities
* Broadcast periodic high-frequency safety beacons (Basic Safety Message - BSM equivalent: Location, Heading, Speed, Emergency Status).
* Ingest surrounding peer beacons and maintain a local dynamic tactical neighbor table.
* Detect immediate local proximity hazards, blind-spot convergence, and intersection crossing conflicts.
* Relay localized ad-hoc preemption beacons directly to roadside traffic signal units when out of central network reach.

### 3. Inputs
* Local vehicle kinematics from **GPS Tracking**.
* Emergency mission status from **Ambulance Management**.
* Incoming ad-hoc wireless datagrams from peer vehicles.

### 4. Outputs
* Transmitted wireless broadcast packets.
* Proximity alert warnings to vehicle drivers (`CollisionRiskAlert`).
* Local V2I preemption triggers dispatched to **Traffic Signal Controller**.

### 5. Communication with Other Modules
* Feeds direct peer status to **Multi Ambulance Coordinator**.
* Emits critical alerts to on-board driver terminal via **Route Dashboard**.
* Directly triggers **Traffic Signal Controller** in edge/fallback mode.
* Reports physical RF link degradation to **Safety Monitor**.

### 6. Hardware Dependencies
* Dedicated Short-Range Communications (DSRC) / C-V2X (Cellular V2X) transceiver, or secondary Wi-Fi/Ad-hoc 802.11p module on the Raspberry Pi/QNX edge node.

### 7. Software Dependencies
* QNX raw socket networking drivers / POSIX UDP broadcast/multicast socket interfaces.
* Low-overhead binary packet serialization (e.g., ASN.1 or compact Protocol Buffers).

### 8. Failure Conditions
* Severe radio channel congestion / packet collision in dense multi-vehicle environments.
* RF interference, antenna damage, or localized multipath fading.
* Desynchronization of internal microsecond clocks across independent mobile units.

### 9. Future Scalability
* Direct integration with 5G NR-V2X Sidelink standards (PC5 interface).
* Cryptographic hardware security module (HSM) acceleration for IEEE 1609.2 certificate validation.

---

## 14. Police Notification

### 1. Purpose
Automates secure, real-time coordination with law enforcement agencies for route perimeter clearance, intersection manual support, crowd management, and incident escort.

### 2. Responsibilities
* Generate automated police dispatch notifications upon creation of high-triage corridors or mass-casualty incidents.
* Stream expected route passage schedules to traffic police patrol division consoles.
* Highlight intersections requiring physical officer presence or manual intersection control.
* Accept and integrate law enforcement status acknowledgments and clearance reports.

### 3. Inputs
* Active corridor specifications and schedules from **Dynamic Green Corridor**.
* Incident profiles from **Emergency Call Module**.
* Police unit status and acknowledgment feeds.

### 4. Outputs
* Automated police clearance requests (`PoliceEscortRequest`, `PerimeterSecuringNotice`).
* Real-time CAD (Computer-Aided Dispatch) interoperability payloads.

### 5. Communication with Other Modules
* Receives corridor activation alerts from **Dynamic Green Corridor**.
* Receives incident data from **Emergency Call Module**.
* Transmits confirmation of police clearance back to **Dynamic Green Corridor** and **Safety Monitor**.
* Logs all inter-agency transactions to **Logging**.

### 6. Hardware Dependencies
* Enterprise network gateways / secure inter-agency WAN links.

### 7. Software Dependencies
* Inter-agency integration APIs (REST, Secure WebSockets, or CAP - Common Alerting Protocol).
* Strong encryption modules.

### 8. Failure Conditions
* Third-party police agency CAD gateway downtime or API rate limiting.
* Inter-agency network link disconnection.
* Delayed officer clearance confirmation creating corridor uncertainty.

### 9. Future Scalability
* Automated police patrol vehicle in-car navigation waypoint injection.
* Drone escort automated dispatch and aerial route scouting synchronization.

---

## 15. Logging

### 1. Purpose
Provides a centralized, tamper-evident, deterministic audit logging and event telemetry service spanning all edge and server nodes.

### 2. Responsibilities
* Capture operational, tactical, clinical, and safety-critical log messages across all modules.
* Enforce structured log formats with monotonic timestamps, severity levels, and origin module IDs.
* Persist logs locally during network isolation and stream to centralized persistent storage upon reconnection.
* Maintain an immutable audit trail for post-incident legal investigations and performance debriefs.

### 3. Inputs
* Log messages, error assertions, trace events, and metrics from every module in the system.

### 4. Outputs
* Serialized log streams written to local non-volatile storage.
* Networked log aggregation streams (syslog / central log database).
* Real-time critical error alerts routed to system administrators.

### 5. Communication with Other Modules
* Consumes events from **all 18 other modules** via high-speed, non-blocking IPC/network channels.
* Monitored for storage utilization by **Safety Monitor**.

### 6. Hardware Dependencies
* High-endurance flash storage (e.g., industrial eMMC / SSD) to prevent storage wear-out from continuous writes.

### 7. Software Dependencies
* QNX `slogger2` system logging service or high-performance asynchronous zero-copy logging framework.
* Compression and rotation utilities.

### 8. Failure Conditions
* Local disk exhaustion / out of storage space.
* Blocking I/O causing caller execution threads to miss real-time deadlines.
* Corrupted log files resulting from abrupt power loss during write operations.

### 9. Future Scalability
* Cryptographic append-only blockchain/Merkle-tree ledger integration for legal evidence chain-of-custody.
* Distributed log streaming to cloud analytical warehouses for city-wide emergency metrics.

---

## 16. Authentication

### 1. Purpose
Enforces identity verification, role-based access control (RBAC), and mutual cryptographic trust across all human operators, client applications, and physical edge hardware nodes.

### 2. Responsibilities
* Authenticate human users (dispatchers, paramedics, doctors, traffic operators) using secure credentials and multi-factor authentication.
* Authenticate edge devices (OBU ambulances, RSU traffic lights) using mutual TLS (mTLS) and hardware-backed device certificates.
* Issue, validate, and revoke time-limited authorization tokens (e.g., JWT).
* Enforce access boundaries: ensure medical personnel cannot trigger traffic overrides and traffic operators cannot access patient vitals.

### 3. Inputs
* User login credentials and MFA tokens from UI dashboards.
* Machine certificate exchange packets and API tokens from edge nodes.
* Token validation requests from internal modules.

### 4. Outputs
* Cryptographically signed authorization tokens.
* Identity verification approvals or rejections (`AccessDenied`).
* Token revocation lists (CRL).

### 5. Communication with Other Modules
* Authenticates requests for **Emergency Call Module**, **Hospital Dashboard**, **Traffic Dashboard**, **Route Dashboard**, and **Dynamic Green Corridor**.
* Audits all security events (logins, failed attempts, privilege escalations) to **Logging**.
* Communicates violations to **Safety Monitor**.

### 6. Hardware Dependencies
* Secure Elements / TPM 2.0 / Hardware Security Modules (HSM) where available for hardware key storage on edge nodes.

### 7. Software Dependencies
* Cryptographic libraries (OpenSSL, libsodium, or QNX Cryptographic Library).
* Secure hashing and signature validation implementations.

### 8. Failure Conditions
* Expiration of root CA or device certificates causing network-wide communication denial.
* Clock drift causing valid tokens to be evaluated as premature or expired.
* Authentication service crash blocking new system logins.

### 9. Future Scalability
* Biometric authentication integration for in-ambulance clinical terminals.
* Zero-trust dynamic micro-segmentation across vehicular ad-hoc networks.

---

## 17. Safety Monitor

### 1. Purpose
Acts as the independent, supervisory watchdog and safety assurance module, continuously monitoring system health, enforcing fail-safe boundaries, and executing emergency fallback states.

### 2. Responsibilities
* Execute periodic heartbeat / keep-alive checks against all local and remote software modules.
* Monitor critical invariants (e.g., speed limits, conflicting preemption commands, stale vital telemetry, hardware temperature).
* Detect communication dropouts and initiate controlled, fail-safe degradation routines.
* Trigger hardware watchdog kick pulses on embedded nodes to reset crashed physical nodes.

### 3. Inputs
* Heartbeat pulses and health telemetry from every active module.
* Hardware system metrics (CPU load, memory pressure, storage capacity, operating temperature).
* Watchdog reset pings from the operating system kernel.

### 4. Outputs
* Hardware watchdog refresh strobes.
* System degradation commands (`EnterFailSafeMode`, `RevertToManual`, `EmergencyHalt`).
* Alarms to operational dashboards.

### 5. Communication with Other Modules
* Collects health pulses from all modules.
* Issues override commands to **Traffic Signal Controller** (e.g., command flashing amber upon coordinator death).
* Dispatches critical fault notices to **Logging**, **Hospital Dashboard**, and **Traffic Dashboard**.

### 6. Hardware Dependencies
* Physical Hardware Watchdog Timer (WDT) line on the Raspberry Pi / QNX embedded SBC.
* Temperature sensors and voltage monitors on edge hardware.

### 7. Software Dependencies
* QNX High Availability (HA) framework / Watchdog services / QNX Pulses.
* POSIX monotonic timer APIs.

### 8. Failure Conditions
* Safety monitor thread starvation preventing it from kicking the hardware watchdog (resulting in unexpected reboot).
* Failure to recognize a hung worker thread that is still responding to basic heartbeats (zombie state).
* Misconfiguration of fail-safe thresholds resulting in false-positive emergency shutdowns.

### 9. Future Scalability
* Integration of formal safety run-time verification (Simplex Architecture).
* Automated remote over-the-air (OTA) rollbacks upon recurring boot-loop detection.

---

## 18. Network Communication

### 1. Purpose
Provides the unified, high-reliability, transport-agnostic messaging and serialization backbone connecting distributed servers, edge nodes, and visual interfaces.

### 2. Responsibilities
* Abstract underlying physical networking media (Ethernet, Wi-Fi, 4G/5G Cellular, Satellite).
* Manage connections, automatic reconnects, back-off exponential retries, and keep-alives.
* Provide pub/sub messaging topology for broadcast events and point-to-point RPC for synchronous commands.
* Guarantee message delivery semantics (QoS 0 for transient metrics, QoS 1/2 for safety-critical commands).

### 3. Inputs
* Outgoing message objects from any application module.
* Raw network frames arriving from physical network interfaces (NICs).
* Network interface status events from the OS kernel.

### 4. Outputs
* Structured, deserialized event objects dispatched to destination modules.
* Connection status notifications (`NetworkStateChanged`).
* Packet throughput and link latency metrics.

### 5. Communication with Other Modules
* Acts as the transport layer for **all inter-node communication**.
* Feeds connection state and latency health to **Safety Monitor**.
* Reports network security anomalies to **Authentication** and **Logging**.

### 6. Hardware Dependencies
* Ethernet controllers, 4G/5G cellular modems (via PCIe/USB), and Wi-Fi transceivers.

### 7. Software Dependencies
* MQTT Broker/Client runtime (e.g., Eclipse Mosquitto, Paho).
* TCP/IP & UDP socket libraries, WebSockets server, and TLS 1.3 encryption layers.

### 8. Failure Conditions
* Complete loss of wireless WAN coverage (tunnel, rural dead zone).
* Socket buffer saturation caused by network congestion or high latency.
* Silent socket hang / TCP half-open states without keep-alive detection.

### 9. Future Scalability
* Multi-homed bonding and seamless multipath TCP (MPTCP) failover across dual cellular SIMs and satellite links.
* Transition to OMG DDS (Data Distribution Service) for ultra-low-latency real-time telemetry distribution.

---

## 19. Configuration Manager

### 1. Purpose
Maintains, validates, and distributes global, regional, and node-specific static and dynamic runtime configuration profiles across the distributed ecosystem.

### 2. Responsibilities
* Load, parse, and strictly schema-validate configuration files at startup.
* Store operational parameters (e.g., intersection geometric coordinates, speed limits, preemption hold times, IP endpoints, timeout thresholds).
* Support dynamic runtime reconfiguration without requiring hard application restarts.
* Distribute node-specific configurations to edge units upon authenticated check-in.

### 3. Inputs
* Persistent configuration files (JSON, YAML, or XML formats).
* System administrator update commands via management consoles.
* Parameter query requests from local modules.

### 4. Outputs
* Validated parameter key-value pairs delivered to requesting modules.
* Configuration reload broadcast events (`ConfigReloadNotification`).
* Schema validation error reports.

### 5. Communication with Other Modules
* Queried during startup by **all 18 other modules**.
* Emits update notifications to active modules upon parameter modification.
* Records configuration change histories and author identities to **Logging**.
* Validates administrator credentials via **Authentication**.

### 6. Hardware Dependencies
* Persistent non-volatile storage (NVRAM, EEPROM, or Flash storage partition).

### 7. Software Dependencies
* Schema validation engines (e.g., JSON Schema validator).
* POSIX file system access APIs.

### 8. Failure Conditions
* Syntax or schema errors in configuration files preventing module bootstrap.
* Partial/incomplete configuration write during sudden power cutoff.
* Race condition during dynamic configuration reload leading to inconsistent state.

### 9. Future Scalability
* Centralized cloud configuration synchronization with cryptographic signed configuration bundles.
* Geographically federated configuration distribution across multiple municipal jurisdictions.

---

## Architectural Mapping & Topology

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       SUPERVISORY / CLOUD LAYER                                        │
│                                                                                                        │
│  [1. Emergency Call]        [11. Priority Scheduler]        [2. Ambulance Management]                  │
│          │                             ▲                                ▲                              │
│          ▼                             │                                │                              │
│  [3. Route Planning] ◄─────────────────┴────────────────────────────────┘                              │
│          │                                                                                             │
│          ▼                                                                                             │
│  [5. Dynamic Green Corridor] ◄───► [12. Multi Ambulance Coordinator] ◄───► [14. Police Notification]   │
│                                                                                                        │
│  [7. Hospital Dashboard]    [8. Traffic Dashboard]    [9. Route Dashboard]                             │
│                                                                                                        │
│  Shared Core Services:                                                                                 │
│  [16. Authentication]  •  [19. Configuration Manager]  •  [15. Logging]  •  [17. Safety Monitor]       │
└───────────────────────────────────────────────┬────────────────────────────────────────────────────────┘
                                                │
                          [18. Network Communication: MQTT / TCP / TLS]
                                                │
         ┌──────────────────────────────────────┴──────────────────────────────────────┐
         ▼                                                                             ▼
┌─────────────────────────────────────────────────┐   ┌──────────────────────────────────────────────────┐
│      AMBULANCE OBU NODE (Raspberry Pi/QNX)      │   │      TRAFFIC RSU NODE (Raspberry Pi/QNX)         │
│                                                 │   │                                                  │
│  [4. GPS Tracking]   [10. Patient Vitals]       │   │  [6. Traffic Signal Controller]                  │
│          │                    │                 │   │                  ▲                               │
│          ▼                    ▼                 │   │                  │                               │
│  [13. V2V Communication Module] ────────────────┼───┼──────────────────┘ (Direct V2I Beacon Fallback)  │
│                                                 │   │                                                  │
│  Local Edge Support:                            │   │  Local Edge Support:                             │
│  [15. Logging]  •  [17. Safety Monitor (WDT)]   │   │  [15. Logging]  •  [17. Safety Monitor (WDT)]    │
│  [19. Config]   •  [18. Network Comm]           │   │  [19. Config]   •  [18. Network Comm]            │
└─────────────────────────────────────────────────┘   └──────────────────────────────────────────────────┘
```

---

## 20. Production-Grade Embedded System Architecture (Raspberry Pi 4 + QNX SDP 8.0)

Under Phase 1 and Phase 2, each edge roadside intersection SBC runs an autonomous industrial control loop on the official QNX SDP 8.0 Raspberry Pi 4 Quick Start Image (`aarch64le`):

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│               QNX SDP 8.0 INDUSTRIAL EMBEDDED CONTROLLER ARCHITECTURE                  │
│                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 1. BootManager: Deterministic 14-Stage Startup Pipeline                          │  │
│  │    [Kernel Boot] ➔ [Init BSP] ➔ [Init GPIO] ➔ [Reserve GPIO] ➔ [Init Relays] ➔    │  │
│  │    [Relay POST] ➔ [Init IR] ➔ [Init Net] ➔ [Verify Link] ➔ [Register Node] ➔      │  │
│  │    [Start Watchdog] ➔ [Start Heartbeat] ➔ [Enter Operational] ➔ [READY]          │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 2. HardwareWatchdog (Dedicated 1 Hz Thread)                                      │  │
│  │    Supervises: [Relays] • [GPIO] • [IR Sensors] • [Traffic Controller] •         │  │
│  │                [Heartbeat] • [Network Link] • [Boot Manager]                     │  │
│  │    Action on Fault: ALL_RED / FLASHING_YELLOW ➔ Alarm ➔ Auto-Recovery            │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 3. Relay Controller & Hardware Interlock (Active-Low Driving: 0V ON, 3.3V OFF)    │  │
│  │    Relay 1: Road A Red (GPIO 17)        Relay 3: Road B Red (GPIO 23)            │  │
│  │    Relay 2: Road A Green (GPIO 22)  ◄X► Relay 4: Road B Green (GPIO 25)          │  │
│  │    * Hard mutual exclusion interlock: Road A Green + Road B Green impossible!    │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 4. Wi-Fi / Ethernet Quality Monitor (WirelessQualityMonitor)                     │  │
│  │    5 Tiers: EXCELLENT (>-60dBm) • GOOD (>-68dBm) • FAIR (>-75dBm) •               │  │
│  │             POOR (>-85dBm) • CRITICAL (< -85dBm or >= 20% loss) • DISCONNECTED  │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 5. Safe Shutdown Manager (ShutdownManager: Non-Reentrant 10-Step Pipeline)       │  │
│  │    Stop Commands ➔ Complete Transition ➔ All Red Clearance ➔ Deactivate Relays  │  │
│  │    ➔ Store Logs ➔ Release GPIO ➔ Stop Threads ➔ Close Sockets ➔ Unmap BSP Memory │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 21. Real-Time Corridor Operations & Autonomous Fault Recovery (Phase 3)

Phase 3 implements real-time corridor monitoring, deterministic dynamic release, multi-domain timeout supervision, in-flight route replanning, and autonomous fault recovery on the **Raspberry Pi 4 QNX SDP 8.0 Quick Start Image** (`aarch64le`):

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                   PHASE 3: REAL-TIME CORRIDOR OPERATIONS ARCHITECTURE                  │
│                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 1. CorridorDeadlineMonitor                                                       │  │
│  │    - Monotonic QNX clock deadline accounting (Creation, ETA, Elapsed, Remaining) │  │
│  │    - Timeout detection ➔ Automatic corridor recovery callback & JSON telemetry   │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 2. EmergencyTimeoutManager (Deadlock-Free Supervision)                           │  │
│  │    - Domain 1: Stalled Ambulance (Lack of spatial waypoint progression)          │  │
│  │    - Domain 2: Coordinator Inactivity (Central pulse / dispatch silence)         │  │
│  │    - Domain 3: Communication Link Timeout (Node TCP / heartbeat silence)         │  │
│  │    - Domain 4: Intersection ACK Timeout (Preemption command retransmit/escalate) │  │
│  │    - 3-Tier Escalation: TIER 1 Retry ➔ TIER 2 Alert ➔ TIER 3 Fail-safe Recovery  │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 3. DynamicCorridorReleaseManager                                                 │  │
│  │    - Intersection-by-intersection immediate release upon vehicle passage         │  │
│  │    - Deterministic clearance: CLEARING_YELLOW (3s) ➔ ALL_RED (2s) ➔ RELEASED    │  │
│  │    - Multi-vehicle overlap safety (hands off preemption to queued emergency)     │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 4. RouteProgressTracker & In-Flight Replanner                                    │  │
│  │    - Spatial tracking: Current, Previous, Next, Remaining Hops, Completion %     │  │
│  │    - Dynamic velocity and ETA re-estimation                                      │  │
│  │    - In-flight shortest path recalculation around failed/blocked intersections   │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 5. SignalTimelineGenerator & Replay Engine                                       │  │
│  │    - Deterministic transition schedule with monotonic QNX timestamps             │  │
│  │    - Pre-clearance yellow, all-red hold, green preemption, post-clearance        │  │
│  │    - Replay engine for offline debugging, drift analysis (Δt), and JSON export   │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 6. ClusterHealthMonitor                                                          │  │
│  │    - 4 Nodes: COORD_01 (192.168.1.100), INT_A (.101), INT_B (.102), INT_C (.103) │  │
│  │    - CPU %, RAM (MB/%), Temp (°C), Heartbeat Age (ms), QoS Level, Relay/GPIO     │  │
│  │    - Composite weighted cluster health score (0.0% - 100.0%) & degraded alerts   │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 7. FaultRecoveryManager (Resilient Autonomous Recovery)                          │  │
│  │    - Recovers from Comm Failures (Socket auto-reconnect & queue replay)          │  │
│  │    - Recovers from Watchdog Events (Targeted subsystem reset without reboot)     │  │
│  │    - Recovers from Node Reboot (Automated active corridor state push)            │  │
│  │    - Recovers from Network Loss (Optical sensor fallback & post-restore resync)  │  │
│  │    - Recovers from Corridor Interruption (Dynamic bypass replanning & release)   │  │
│  │    - SAFE MODE Boundary: Enters ALL_RED strictly when recovery is impossible     │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────┘
```

---

## 22. Native QNX Neutrino RTOS Subsystem Architecture (Phase 4)

Phase 4 transforms SMART AEGIS into a genuine QNX Neutrino RTOS real-time distributed application on the **Raspberry Pi 4 Model B (4 GB)** running the official **QNX SDP 8.0 Quick Start Image** (`aarch64le`):

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                   PHASE 4: NATIVE QNX NEUTRINO RTOS ARCHITECTURE                       │
│                                                                                        │
│  ┌──────────────────────────────────────────────────────────────────────────────────┐  │
│  │ 1. QNX Resource Manager (/dev/aegis/*) via resmgr_attach()                       │  │
│  │    - Device Nodes: /dev/aegis/coordinator  • /dev/aegis/intersectionA           │  │
│  │                    /dev/aegis/intersectionB• /dev/aegis/intersectionC           │  │
│  │                    /dev/aegis/cluster      • /dev/aegis/system                  │  │
│  │    - POSIX API: open(), read(), write(), close(), devctl()                       │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 2. Native QNX Synchronous Message Passing (MsgSend/Receive/Reply)                │  │
│  │    - Blocking rendezvous IPC: Client blocks until Server replies                 │  │
│  │    - ChannelCreate(0), ConnectAttach(), MsgSend(), MsgReceive(), MsgReply()      │  │
│  │    - Message Schemas: HEARTBEAT (0x1001), REGISTRATION (0x1002),                 │  │
│  │                       CORRIDOR_REQUEST (0x1003), SIGNAL_COMMAND (0x1004),        │  │
│  │                       STATUS_UPDATE (0x1005), RECOVERY_MESSAGE (0x1006)          │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 3. QNX Pulse Management (SIGEV_PULSE Asynchronous Events)                        │  │
│  │    - Non-blocking microkernel pulse delivery: MsgSendPulse(), MsgReceivePulse()  │  │
│  │    - Application Pulse Codes: WATCHDOG (1) • HEARTBEAT (2) • TIMEOUT (3) •       │  │
│  │                               DEADLINE (4) • SHUTDOWN (5) • FAULT_RECOVERY (6) • │  │
│  │                               INTERRUPT (7)                                      │  │
│  │    - Thread-safe asynchronous dispatcher loop with callback registration         │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 4. High-Resolution QNX Timers & Microsecond Jitter Statistics                    │  │
│  │    - timer_create(CLOCK_MONOTONIC, &event, &timerId) with SIGEV_PULSE binding     │  │
│  │    - Periodic & one-shot arming via timer_settime() with nanosecond precision     │  │
│  │    - Real-time jitter tracking: min, max, avg (µs) and timer_getoverrun()        │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 5. Real-Time Thread Scheduling Hierarchy (SCHED_FIFO & Prio-Inheritance)         │  │
│  │    - Priority 30: Coordinator Core Dispatch Thread (Highest Safety Priority)     │  │
│  │    - Priority 25: Intersection Controller Threads (Real-Time Actuation)          │  │
│  │    - Priority 21: Dynamic Corridor Release & Timeline Engine                     │  │
│  │    - Priority 12: Health Monitor & Deadline Supervision                          │  │
│  │    - Priority 5:  Telemetry, Event Logging & devctl Handlers                     │  │
│  │    - PrioInheritMutex (PTHREAD_PRIO_INHERIT) eliminating priority inversion      │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 6. GPIO Interrupt-Driven Event Management (BCM2711)                              │  │
│  │    - InterruptAttachEvent() delivering kernel pulses directly into channel       │  │
│  │    - Line Mappings: GPIO 12 (IR Approach) • GPIO 13 (IR Departure) •             │  │
│  │                     GPIO 16 (Emergency Button) • GPIO 24 (Relay Fault)           │  │
│  │    - Masking, unmasking, detachment, and event trigger telemetry                 │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 7. Industrial devctl() Interface (9 Commands, Magic Class 'A' = 0x41)            │  │
│  │    - DCMD_AEGIS_GET_SYSTEM_STATUS   • DCMD_AEGIS_GET_NODE_STATUS                 │  │
│  │    - DCMD_AEGIS_START_CORRIDOR      • DCMD_AEGIS_STOP_CORRIDOR                   │  │
│  │    - DCMD_AEGIS_RESET_NODE          • DCMD_AEGIS_READ_TELEMETRY                  │  │
│  │    - DCMD_AEGIS_READ_LOGS           • DCMD_AEGIS_ENABLE_DIAGNOSTICS              │  │
│  │    - DCMD_AEGIS_RUN_SELF_TEST                                                    │  │
│  └────────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                           │                                            │
│  ┌────────────────────────────────────────▼─────────────────────────────────────────┐  │
│  │ 8. Native QNX Diagnostics & Health Telemetry (QnxDiagnostics)                    │  │
│  │    - Aggregates channel counts, coids, thread priority audit, pulse metrics,     │  │
│  │      timer jitter, interrupt counters, latency, memory, and CPU temperature      │  │
│  │    - Structured JSON report export and live formatted ASCII dashboard tables     │  │
│  └──────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────┘
```
```
