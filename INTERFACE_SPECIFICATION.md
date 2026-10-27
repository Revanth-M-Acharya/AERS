# SMART AEGIS: Interface Control Document (ICD)
**Document Identifier:** ICD-AEGIS-2026-V1.0  
**Classification:** Mission-Critical Cyber-Physical System (CPS)  
**Applicable Standards:** IEEE 830-1998, POSIX.1-2017, ISO 26262, IEEE 1609.2 / SAE J2735  
**Normative References:**  
* [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md)  
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md)  
* [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md)  

---

## 1. Purpose

This **Interface Control Document (ICD)** establishes the definitive, binding communication contracts between every software module within the SMART AEGIS system. 

In a distributed, safety-critical real-time cyber-physical system, inter-module communication boundaries must be rigorously governed. **These interfaces are mandatory.** Modules must never bypass these defined contracts, employ hidden side-channels, access another module's raw memory without authorization, or alter payload schemas unilaterally. Every interaction—whether over QNX microkernel IPC, POSIX sockets, direct ad-hoc UDP wireless, or wide-area MQTT over TLS—must conform to the specifications set forth in this document.

---

## 2. Module Interface Overview

The following table provides the comprehensive interface matrix across all 19 system modules defined in `SYSTEM_ARCHITECTURE.md`:

| Module Name | Core Responsibilities | Primary Inputs | Primary Outputs | Communication Method | Core Dependencies |
|---|---|---|---|---|---|
| **1. Emergency Call Module** | Call intake, triage code calculation, incident creation | 911 caller data, ACN telemetry, operator UI | `EmergencyRequest` | HTTPS REST / MQTT (QoS 2) | Auth, Config, Network |
| **2. Ambulance Management** | Fleet state machine, unit reservation, asset tracking | Status transition events, dispatch orders | `AmbulanceStateUpdate`, Unit Locks | QNX IPC / MQTT (QoS 2) | Config, Network, Auth |
| **3. Route Planning** | A* pathing, ETA calculation, waypoint generation | Incident coords, GPS coords, street graph | `NavigationRoute`, Reroute Advisories | In-process C++ API / QNX IPC | Config, Math libs |
| **4. GPS Tracking** | NMEA-0183 parsing, EKF dead-reckoning, kinematics | Raw UART GNSS serial, 6-DOF IMU | `GPSUpdate` (10 Hz kinematics) | POSIX Serial / QNX resmgr / MQTT (QoS 0) | Config, Hardware UART |
| **5. Dynamic Green Corridor** | Preemption envelope computation, corridor triggering | `GPSUpdate`, `NavigationRoute`, Signal states | `TrafficPreemptionRequest`, Clearances | MQTT (QoS 2) / QNX IPC | Config, Math libs |
| **6. Traffic Signal Controller** | Deterministic phase FSM, relay driving, hardware safety | `TrafficPreemptionRequest`, V2I Beacons | `TrafficStatus`, Phase states, Alarms | QNX IPC / GPIO resmgr / MQTT (QoS 1) | Hardware relays, WDT |
| **7. Hospital Dashboard** | Trauma bay readiness, vitals rendering, doctor orders | `VitalsUpdate`, `GPSUpdate` (ETA), Incidents | `HospitalCommand`, Bay Acks | WebSocket / MQTT (QoS 1) | Auth, Network |
| **8. Traffic Dashboard** | Citywide signal monitoring, manual override console | `TrafficStatus`, Corridor envelopes | Manual Preemption Overrides | WebSocket / MQTT (QoS 1/2) | Auth, Network |
| **9. Route Dashboard** | Fleet GIS tracking, tactical navigation display | `GPSUpdate`, `NavigationRoute`, Incident pins | Route Recalculation Requests | WebSocket / MQTT (QoS 1) | Auth, Network |
| **10. Patient Vitals Module** | Biomedical telemetry ingestion, anomaly detection | Serial/BLE vitals feeds, Paramedic inputs | `VitalsUpdate`, `HospitalAlert` | POSIX Serial / MQTT (QoS 1/2) | Auth, Encryption |
| **11. Priority Scheduler** | Multi-criteria triage ranking, resource arbitration | `EmergencyRequest`, `VitalsUpdate`, Fleet | `PriorityDecision`, Corridor ranks | In-process C++ API / QNX IPC | Config |
| **12. Multi Ambulance Coord** | Trajectory deconfliction, spatial convoy spacing | `GPSUpdate`, `NavigationRoute`, Priority ranks | Speed Adjustments, Stagger Orders | In-process C++ API / QNX IPC | Config, Math libs |
| **13. V2V Communication** | Direct ad-hoc wireless beacons, local proximity | `GPSUpdate`, Emergency status, Peer beacons | Local Collision Alerts, V2I Beacons | Direct Ad-hoc UDP Broadcast | Hardware 802.11p/Wi-Fi |
| **14. Police Notification** | Inter-agency clearance requests, route feeds | Active corridor specifications | `PoliceAlertNotice`, CAD payloads | Secure HTTPS / REST / Webhook | Network, Auth |
| **15. Logging** | High-throughput structured tamper-evident audit logs | Trace, warning, and fault events from all mods | Non-volatile log files, `slogger2` | QNX `slogger2` / Asynch Ring Buffer | Storage hardware |
| **16. Authentication** | Identity verification, mTLS handshakes, JWT tokens | User credentials, edge device certificates | Cryptographic JWTs, Access Grants/Denies| OpenSSL TLS 1.3 / In-process API | Crypto hardware / TPM |
| **17. Safety Monitor** | Process heartbeats, WDT servicing, fail-safe trip | Periodic keep-alive pulses, hardware metrics | Watchdog Strobes, Degraded Mode cmds | QNX Pulses / POSIX `/dev/watchdog` | Hardware WDT |
| **18. Network Communication** | Unified transport abstraction, reconnects, buffering | Outgoing module payloads, raw NIC frames | Deserialized event packets, Link states | MQTT / TCP / UDP / WebSockets | OS Network Stack |
| **19. Configuration Manager** | Schema validation, parameter distribution | Config files (`config.json`), CLI updates | Validated key-values, Reload events | In-process C++ API / POSIX Filesystem | Non-volatile storage |

---

## 3. System Communication Diagram

```
┌────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│                                       CENTRAL DISPATCH & SUPERVISORY                                   │
│                                                                                                        │
│   [Emergency Call Module] ──► [Priority Scheduler] ◄──► [Ambulance Management] ◄──► [Route Planner]    │
│            │                          │                         │                           │          │
│            │ (HTTP REST)              │ (In-Process / IPC)      │ (In-Process / IPC)        │ (In-Proc)│
│            ▼                          ▼                         ▼                           ▼          │
│   ══════════════════════════════════════════════════════════════════════════════════════════════════   │
│                                    MQTT TELEMETRY BROKER (TLS 1.3)                                     │
│   ══════════════════════════════════════════════════════════════════════════════════════════════════   │
│         ▲                    ▲                  ▲                     ▲                   ▲            │
│         │                    │                  │                     │                   │            │
│  (WebSocket)          (WebSocket)        (MQTT QoS 1/2)        (MQTT QoS 1/2)      (MQTT QoS 0/1/2)    │
│         │                    │                  │                     │                   │            │
│         ▼                    ▼                  ▼                     ▼                   │            │
│  [Traffic Dashboard]  [Hospital Dashboard]  [Route Dashboard]  [Police Notification]      │            │
│                                                                                           │            │
└───────────────────────────────────────────────────────────────────────────────────────────┼────────────┘
                                                                                            │
                                                                                            │ (WAN LTE)
                                ┌───────────────────────────────────────────────────────────┴────────────┐
                                │                                                                        │
                                ▼                                                                        ▼
┌───────────────────────────────────────────────────────┐      ┌─────────────────────────────────────────────────────────┐
│        AMBULANCE OBU NODE (Raspberry Pi/QNX)          │      │         TRAFFIC RSU NODE (Raspberry Pi/QNX)             │
│                                                       │      │                                                         │
│  [Patient Vitals]           [GPS Tracking]            │      │  [Traffic Signal Controller]                            │
│         │                          │                  │      │         │                     │                         │
│         │ (Serial/POSIX)           │ (UART/resmgr)    │      │         │ (GPIO resmgr)       │ (Pulse/MsgSend)         │
│         ▼                          ▼                  │      │         ▼                     ▼                         │
│  ┌─────────────────────────────────────────────────┐  │      │  [Physical Relays]     [Safety Monitor]                 │
│  │     Local Communication & Store-Forward Queue   │  │      │  (R/Y/G Traffic Lamps) (WDT / Flashing Amber)           │
│  └────────────────────────┬────────────────────────┘  │      │                                                         │
│                           │                           │      │                                                         │
│                           ▼ (Local Socket)            │      │                                                         │
│                [V2V Communication Module]             │      │                                                         │
│                           │                           │      │                                                         │
└───────────────────────────┼───────────────────────────┘      └─────────────────────────────▲───────────────────────────┘
                            │                                                                │
                            │                DIRECT WIRELESS AD-HOC V2I BEACON               │
                            └────────────────────────── (UDP Port 8998) ─────────────────────┘
```

---

## 4. MQTT Topics Specification

All central telemetry and commands traversing the wide-area network use MQTT over TLS 1.3 (Port 8883) or MQTT over WebSockets (Port 9001).

| Topic Hierarchy | Publisher | Subscriber(s) | QoS | Payload Schema | Description |
|---|---|---|---|---|---|
| `aegis/intersection/+/commands` | Dynamic Green Corridor, Dashboard | Traffic Signal Node | 1/2 | `IntersectionCommand` | Standardized intersection control (`PREEMPT`, `CLEAR`, `RESTORE`, `HEARTBEAT`, `PING`, `STATUS_REQUEST`). |
| `aegis/intersection/+/status` | Traffic Signal Node | Coordinator, Dashboard, Municipal GIS | 1 | `IntersectionStatus` | 1 Hz intersection phase & health telemetry with fail-safe status. |
| `aegis/fleet/+/location` | Ambulance OBU | Route Planner, Dashboards, Corridor Engine | 0 | `GPSUpdate` | 10 Hz real-time kinematic position, speed, and heading. |
| `aegis/fleet/+/vitals` | Ambulance OBU | Hospital Dashboard, Priority Scheduler | 1 | `VitalsUpdate` | 1 Hz streaming physiological patient vitals (ECG, BP, SpO2). |
| `aegis/fleet/+/alarms` | Ambulance OBU | Hospital Dashboard, Priority Scheduler | 2 | `HospitalAlert` | Critical patient deterioration alarm (acute arrhythmia, pressure drop). |
| `aegis/fleet/+/state` | Ambulance OBU / Dispatch | Fleet Manager, Dashboards | 2 | `AmbulanceStateUpdate` | Vehicle operational status transition (`DISPATCHED`, `ARRIVED`, etc.). |
| `aegis/fleet/+/heartbeat` | Ambulance OBU | Fleet Manager, Dashboards | 0 | `Heartbeat` | 1 Hz vehicle node heartbeat and computing diagnostics. |
| `aegis/hospital/+/commands` | Hospital Dashboard | Ambulance OBU | 2 | `HospitalCommand` | Pre-arrival clinical instructions from ER trauma physician to crew. |
| `aegis/emergency/request` | Emergency Call Intake | Central Coordinator, Dispatch | 2 | `EmergencyRequest` | Validated emergency distress call requiring ambulance dispatch. |
| `aegis/incident/+/dispatched` | Central Coordinator | Fleet Manager, Dashboards | 2 | `EmergencyRequest` | Dispatch confirmation and unit assignment event. |
| `aegis/system/heartbeat/+` | All Edge Nodes | Safety Monitor | 0 | `Heartbeat` | Periodic node health pulse containing CPU, RAM, and state metrics. |
| `aegis/system/logs` | All Modules | Central Logging Sink | 1 | `FaultEvent` / LogRecord | Aggregated error reports and audit trail events. |

---

## 5. QNX IPC Interfaces

On QNX Neutrino RTOS nodes (Ambulance OBU and Traffic RSU), inter-process communication is governed by microkernel primitives:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        QNX IPC PRIMITIVES ARCHITECTURE                 │
│                                                                        │
│   Synchronous (Zero-Copy):    MsgSend() ──► MsgReceive() ──► MsgReply()│
│                                                                        │
│   Asynchronous (Deterministic): MsgSendPulse() ──► Pulse Event Buffer │
│                                                                        │
│   Hardware Abstraction:       POSIX I/O (devctl, read, write) ──► resmgr│
└────────────────────────────────────────────────────────────────────────┘
```

### 5.1 Synchronous Message Passing (`MsgSendv` / `MsgReceivev` / `MsgReplyv`)
* **Usage:** Used for zero-copy, deterministic transactions where the client process requires immediate acknowledgment or return data before proceeding.
* **Channels & Connections:**
  * **Traffic Safety Channel:** The `Traffic Signal Controller` creates a channel (`ChannelCreate()`). The `Safety Monitor` connects (`ConnectAttach()`) to exchange phase interlock validations synchronously.
  * **Priority Arbitration Channel:** Local schedulers query pre-allocated priority decision matrices with guaranteed bounded turnaround (< 1ms).

### 5.2 Pulse Channels (`MsgSendPulse`)
* **Usage:** Non-blocking, fixed-size 32-bit notifications carrying an 8-bit code and 32-bit payload value. Pulses never block the sender and do not require a reply.
* **Allocations:**
  * `PULSE_CODE_WDT_KICK (0x01)`: Dispatched every 200ms by the `Safety Monitor` to reset the hardware watchdog daemon.
  * `PULSE_CODE_TIMER_EXPIRE (0x02)`: Dispatched by QNX POSIX timers (`timer_create()`) to signal yellow-clearance and all-red expiration in the `Traffic Signal Controller`.
  * `PULSE_CODE_SENSOR_ARRIVED (0x03)`: Dispatched by hardware serial interrupt handlers when a complete NMEA sentence arrives in the UART ring buffer.

### 5.3 POSIX Resource Managers (`resmgr`)
Hardware drivers are exposed through standard POSIX `/dev/` nodes:
* `/dev/traffic_controller`: Responds to `write()` for phase changes and `devctl()` for safety diagnostics and relay interlocking checks.
* `/dev/gps_nmea`: Responds to `read()` returning pre-filtered, checksummed NMEA frames.

### 5.4 POSIX Shared Memory (`shm_open`, `mmap`)
* **Usage:** Reserved exclusively for high-frequency telemetry caching (e.g., local 10 Hz GPS kinematics and 100 Hz raw IMU buffers) accessible by `GPS Tracking`, `V2V Communication`, and `Safety Monitor` without IPC serialization overhead. Guarded by inter-process `pthread_mutex_t` configured with `PTHREAD_PRIO_INHERIT`.

---

## 6. TCP/UDP Interfaces

| Connection Purpose | Sender Module | Receiver Module | Transport & Port | Expected Latency | Retry Policy & Timeout |
|---|---|---|---|---|---|
| **V2V Ad-hoc Beacon** | Ambulance `V2V Communication` | Peer Ambulances | UDP Broadcast: **8999** | < 10 ms | No retry (10 Hz periodic stream, ephemeral) |
| **V2I Preemption Beacon**| Ambulance `V2V Communication` | Traffic Controller RSU | UDP Unicast/Broadcast: **8998**| < 15 ms | Retransmit every 100ms until UDP Ack or timeout (500ms) |
| **Internal RPC Interface**| Central Coordinator Core | Module Daemons | TCP / gRPC: **50051** | < 5 ms | Exponential backoff (100ms, 200ms, 400ms), max 3 retries |
| **Emergency Call Ingest** | Web Dispatch Console / ACN | Emergency Call Module | HTTPS / TLS: **443** | < 50 ms | Client retry on HTTP 5xx with 500ms backoff |
| **RSU Field Maintenance** | Diagnostic Laptop | Traffic Controller RSU | SSH / TCP: **22** / **50052** | < 20 ms | Interactive session, keepalive every 10s |

---

## 7. Message Definitions

All application payloads transmitted over MQTT, TCP, and UDP are structured according to strict JSON / Protocol Buffer schemas.

---

### 7.1 `EmergencyRequest`
* **Purpose:** Carries verified emergency call data from intake to the dispatch and priority engine.
* **Fields & Types:**
  * `incidentId` (`string`, UUIDv4): Unique incident identifier.
  * `timestamp` (`uint64`, Unix milliseconds): Time of call intake.
  * `callerContact` (`string`): Masked phone number of caller.
  * `latitude` (`float64`, Degrees): Incident latitude (-90.0 to +90.0).
  * `longitude` (`float64`, Degrees): Incident longitude (-180.0 to +180.0).
  * `triageCategory` (`string`): Enum: `CODE_RED`, `CODE_AMBER`, `CODE_GREEN`.
  * `incidentType` (`string`): Enum: `CARDIAC`, `TRAUMA`, `RESPIRATORY`, `STROKE`, `GENERAL`.
  * `requiredCapabilities` (`array[string]`): e.g., `["ALS", "VENTILATOR"]`.
* **Validation Rules:** Latitude/longitude must fall within city bounding box. Triage category must not be null.
* **Example Payload:**
```json
{
  "incidentId": "d7b4202a-9d90-4a81-bb8e-1736780c1091",
  "timestamp": 1792790400000,
  "callerContact": "+1-555-019-2831",
  "latitude": 12.971598,
  "longitude": 77.594562,
  "triageCategory": "CODE_RED",
  "incidentType": "CARDIAC",
  "requiredCapabilities": ["ALS", "DEFIBRILLATOR"]
}
```

---

### 7.2 `GPSUpdate`
* **Purpose:** Periodic 10 Hz kinematic telemetry stream from vehicle On-Board Unit.
* **Fields & Types:**
  * `unitId` (`string`): Ambulance asset ID (e.g., `AMB-01`).
  * `timestamp` (`uint64`, Unix milliseconds): GNSS fix timestamp.
  * `latitude` (`float64`, Degrees): Current latitude.
  * `longitude` (`float64`, Degrees): Current longitude.
  * `speed` (`float32`, km/h): Instantaneous ground speed (0.0 to 180.0).
  * `bearing` (`float32`, Degrees): True heading (0.0 to 359.9).
  * `hdop` (`float32`): Horizontal Dilution of Precision (< 2.0 considered high quality).
  * `fixQuality` (`uint8`): 0=Invalid, 1=GPS, 2=DGPS/RTK, 6=Dead-Reckoning.
* **Validation Rules:** Speed must not exceed 180 km/h; bearing must be in range `[0, 360)`.
* **Example Payload:**
```json
{
  "unitId": "AMB-01",
  "timestamp": 1792790401100,
  "latitude": 12.973412,
  "longitude": 77.597120,
  "speed": 64.5,
  "bearing": 88.2,
  "hdop": 0.9,
  "fixQuality": 1
}
```

---

### 7.3 `VitalsUpdate`
* **Purpose:** 1 Hz streaming physiological data from ambulance patient monitor to receiving hospital.
* **Fields & Types:**
  * `unitId` (`string`): Ambulance asset ID.
  * `patientId` (`string`, UUIDv4): Pseudo-anonymous patient tracking ID.
  * `timestamp` (`uint64`, Unix milliseconds): Sampling timestamp.
  * `heartRate` (`uint16`, bpm): Heart rate (0 to 300).
  * `spO2` (`uint8`, Percentage): Blood oxygen saturation (0 to 100%).
  * `systolicBP` (`uint16`, mmHg): Systolic blood pressure (0 to 300).
  * `diastolicBP` (`uint16`, mmHg): Diastolic blood pressure (0 to 200).
  * `respirationRate` (`uint8`, breaths/min): (0 to 60).
  * `gcsScore` (`uint8`): Glasgow Coma Scale (3 to 15).
* **Validation Rules:** `spO2` <= 100; `diastolicBP` < `systolicBP`.
* **Example Payload:**
```json
{
  "unitId": "AMB-01",
  "patientId": "p-90182-c",
  "timestamp": 1792790402000,
  "heartRate": 118,
  "spO2": 93,
  "systolicBP": 95,
  "diastolicBP": 58,
  "respirationRate": 24,
  "gcsScore": 13
}
```

---

### 7.4 `TrafficPreemptionRequest`
* **Purpose:** Command to preempt traffic signal into a safe, cleared green corridor phase.
* **Fields & Types:**
  * `requestId` (`string`, UUIDv4): Unique preemption request ID.
  * `intersectionId` (`string`): Target intersection controller ID (e.g., `INT-104`).
  * `unitId` (`string`): Requesting ambulance ID.
  * `corridorPriority` (`uint8`): Priority level (1=Highest, 10=Lowest).
  * `requestedPhase` (`string`): Target green phase movement (e.g., `NORTH_BOUND_GREEN`).
  * `estimatedArrivalSec` (`uint16`, Seconds): Expected arrival time at intersection.
  * `holdDurationSec` (`uint16`, Seconds): Max time to hold preemption before auto-release.
  * `signature` (`string`, Base64): HMAC-SHA256 signature for verification.
* **Validation Rules:** `holdDurationSec` must not exceed 60 seconds; signature must verify against shared key.
* **Example Payload:**
```json
{
  "requestId": "a8f1345c-01b2-4d56-8e12-334455667788",
  "intersectionId": "INT-104",
  "unitId": "AMB-01",
  "corridorPriority": 1,
  "requestedPhase": "NORTH_BOUND_GREEN",
  "estimatedArrivalSec": 8,
  "holdDurationSec": 20,
  "signature": "kG4Z...b8M="
}
```

---

### 7.5 `TrafficStatus`
* **Purpose:** Real-time signal phase and operational state broadcast from intersection controller.
* **Fields & Types:**
  * `intersectionId` (`string`): Intersection ID.
  * `timestamp` (`uint64`, Unix milliseconds): State timestamp.
  * `currentMode` (`string`): Enum: `NORMAL_CYCLE`, `PREEMPTION_PREPARE`, `GREEN_CORRIDOR`, `RESTORE`, `FAILSAFE_FLASHING_AMBER`.
  * `activePhases` (`object`): Directional states (`northSouth`, `eastWest`), values `RED`, `YELLOW`, `GREEN`.
  * `activePreemptionUnit` (`string` or `null`): ID of ambulance currently holding preemption.
  * `wdtHealthy` (`bool`): Hardware watchdog health indicator.
* **Validation Rules:** Under no circumstance may both `northSouth` and `eastWest` report `GREEN`.
* **Example Payload:**
```json
{
  "intersectionId": "INT-104",
  "timestamp": 1792790403000,
  "currentMode": "GREEN_CORRIDOR",
  "activePhases": {
    "northSouth": "GREEN",
    "eastWest": "RED"
  },
  "activePreemptionUnit": "AMB-01",
  "wdtHealthy": true
}
```

---

### 7.6 `HospitalAlert`
* **Purpose:** Immediate clinical alarm emitted upon acute patient deterioration during transit.
* **Fields & Types:**
  * `alertId` (`string`, UUIDv4): Alarm event ID.
  * `unitId` (`string`): Ambulance ID.
  * `patientId` (`string`): Patient ID.
  * `severity` (`string`): Enum: `CRITICAL`, `WARNING`, `ADVISORY`.
  * `alarmType` (`string`): Enum: `HYPOTENSIVE_CRISIS`, `VENTRICULAR_ARRHYTHMIA`, `HYPOXIA_DROP`, `ARREST`.
  * `triggeredValue` (`string`): e.g., `"BP 70/40 mmHg (Mean: 50)"`.
  * `timestamp` (`uint64`, Unix milliseconds): Alarm trigger time.
* **Validation Rules:** Mandatory audible/visual flag on hospital UI; ACK required within 15 seconds.
* **Example Payload:**
```json
{
  "alertId": "c109283f-4e11-4192-8091-8899aabbccdd",
  "unitId": "AMB-01",
  "patientId": "p-90182-c",
  "severity": "CRITICAL",
  "alarmType": "HYPOTENSIVE_CRISIS",
  "triggeredValue": "BP 70/40 mmHg",
  "timestamp": 1792790404500
}
```

---

### 7.7 `HospitalCommand`
* **Purpose:** Clinical orders dispatched from emergency trauma doctor directly to the ambulance cabin.
* **Fields & Types:**
  * `commandId` (`string`, UUIDv4): Command ID.
  * `unitId` (`string`): Target ambulance ID.
  * `physicianName` (`string`): Name/ID of issuing physician.
  * `instruction` (`string`): Plain-text clinical action (e.g., `"Administer 1mg Epinephrine IV"`).
  * `destinationBay` (`string`): Assigned trauma bay (e.g., `"Trauma Bay 2 - Red"`).
  * `timestamp` (`uint64`, Unix milliseconds): Issuance time.
* **Validation Rules:** Must be signed by active authenticated `Role: Physician` session.
* **Example Payload:**
```json
{
  "commandId": "e2299100-3344-5566-7788-99aabbccddeeff",
  "unitId": "AMB-01",
  "physicianName": "Dr. Sarah Lin (ID: MD-409)",
  "instruction": "Administer 1mg Epinephrine IV and prep for Cath Lab",
  "destinationBay": "Trauma Bay 2",
  "timestamp": 1792790405000
}
```

---

### 7.8 `PriorityDecision`
* **Purpose:** Algorithmic resolution of resource contention between multiple responding units.
* **Fields & Types:**
  * `decisionId` (`string`, UUIDv4): Decision ID.
  * `contestedResourceId` (`string`): Intersection ID or Hospital Dock ID.
  * `grantedUnitId` (`string`): Ambulance granted right-of-way.
  * `yieldingUnitId` (`string`): Ambulance ordered to yield/stagger.
  * `grantedPriorityScore` (`float32`): Normalized priority score of winning unit.
  * `yieldingPriorityScore` (`float32`): Normalized priority score of yielding unit.
  * `yieldAdvisorySpeedKmH` (`float32`): Target speed recommendation for yielding unit.
* **Validation Rules:** Winning unit priority score must strictly exceed yielding unit score.
* **Example Payload:**
```json
{
  "decisionId": "f9901122-8877-6655-4433-221100aabbcc",
  "contestedResourceId": "INT-104",
  "grantedUnitId": "AMB-01",
  "yieldingUnitId": "AMB-02",
  "grantedPriorityScore": 94.2,
  "yieldingPriorityScore": 68.5,
  "yieldAdvisorySpeedKmH": 25.0
}
```

---

### 7.9 `Heartbeat`
* **Purpose:** Periodic 5 Hz health keep-alive transmitted from all edge and server nodes.
* **Fields & Types:**
  * `nodeId` (`string`): Node ID (e.g., `AMB-01`, `RSU-104`, `DISPATCH-SRV`).
  * `timestamp` (`uint64`, Unix milliseconds): Heartbeat timestamp.
  * `cpuUsagePct` (`float32`): Current CPU utilization (0.0 to 100.0).
  * `ramUsageMb` (`float32`): Current Resident Set Size (RSS) memory in MB.
  * `processStatus` (`string`): Enum: `HEALTHY`, `DEGRADED`, `CRITICAL`.
  * `sequenceNumber` (`uint64`): Monotonically increasing sequence number.
* **Validation Rules:** Sequence numbers must increase monotonically; timestamp skew < 500ms.
* **Example Payload:**
```json
{
  "nodeId": "RSU-104",
  "timestamp": 1792790406000,
  "cpuUsagePct": 3.8,
  "ramUsageMb": 14.2,
  "processStatus": "HEALTHY",
  "sequenceNumber": 14209
}
```

---

### 7.10 `FaultEvent`
* **Purpose:** Structured error and exception report routed to logging and safety supervision.
* **Fields & Types:**
  * `faultId` (`string`, UUIDv4): Fault event ID.
  * `originModule` (`string`): Module reporting fault (e.g., `GPS_TRACKING`).
  * `errorCode` (`string`): Standard error code (e.g., `ERR_001_GPS_TIMEOUT`).
  * `severity` (`string`): Enum: `INFO`, `WARNING`, `ERROR`, `FATAL`.
  * `description` (`string`): Detailed diagnostic context.
  * `timestamp` (`uint64`, Unix milliseconds): Event time.
* **Validation Rules:** Fatal severity faults automatically trigger an audit alert to system supervisors.
* **Example Payload:**
```json
{
  "faultId": "a1b2c3d4-e5f6-7890-1234-56789abcdef0",
  "originModule": "GPS_TRACKING",
  "errorCode": "ERR_001",
  "severity": "ERROR",
  "description": "NMEA UART serial stream timeout > 1000ms. Activating IMU dead-reckoning.",
  "timestamp": 1792790407100
}
```

---

## 8. State Transitions & Finite State Machines (FSM)

### 8.1 Ambulance Operational State Machine

```
               ┌───────────────┐
               │   AVAILABLE   │◄─────────────────────────────┐
               └───────┬───────┘                              │
                       │ Incident Dispatched                  │
                       ▼                                      │
               ┌───────────────┐                              │
               │  DISPATCHED   │                              │
               └───────┬───────┘                              │
                       │ Crew Acknowledged & Rolling          │
                       ▼                                      │
               ┌───────────────────┐                          │
               │ EN_ROUTE_PATIENT  │                          │
               └───────┬───────────┘                          │
                       │ Arrival At Scene                     │
                       ▼                                      │
               ┌───────────────────┐                          │
               │  PATIENT_PICKED   │                          │
               └───────┬───────────┘                          │
                       │ Departure to Hospital                │
                       ▼                                      │
               ┌───────────────────┐                          │
               │ EN_ROUTE_HOSPITAL │                          │
               └───────┬───────────┘                          │
                       │ Arrival at Trauma Bay                │
                       ▼                                      │
               ┌───────────────┐                              │
               │    ARRIVED    │                              │
               └───────┬───────┘                              │
                       │ Patient Handover Complete            │
                       └──────────────────────────────────────┘
```

* **AVAILABLE:** Parked at station or idling; ready for call allocation.
* **DISPATCHED:** Incident assigned; crew alerted; navigation computing.
* **EN_ROUTE_PATIENT:** Actively navigating toward incident scene; standard priority.
* **PATIENT_PICKED:** On scene; paramedic performing initial evaluation and triage stabilization.
* **EN_ROUTE_HOSPITAL:** Navigating to receiving emergency department; vitals streaming; Green Corridor active.
* **ARRIVED:** Arrived at hospital dock; physical transfer to trauma team.

---

### 8.2 Traffic Signal Controller State Machine

```
              ┌───────────────────────────┐
              │          NORMAL           │◄─────────────────────────────┐
              │  (Standard Cycle Phases)  │                              │
              └─────────────┬─────────────┘                              │
                            │ Preemption Request Verified                │
                            ▼                                            │
              ┌───────────────────────────┐                              │
              │          PREPARE          │                              │
              │ (Enforce Yellow Clearance)│                              │
              └─────────────┬─────────────┘                              │
                            │ Yellow & All-Red Complete (≥ 4.0s)         │
                            ▼                                            │
              ┌───────────────────────────┐                              │
              │      GREEN_CORRIDOR       │                              │
              │  (Target Direction Green) │                              │
              └─────────────┬─────────────┘                              │
                            │ Ambulance Clears Geofence / Timeout        │
                            ▼                                            │
              ┌───────────────────────────┐                              │
              │          RESTORE          │                              │
              │(Yellow Clearance to Green)│                              │
              └─────────────┬─────────────┘                              │
                            │ Safe Phasing Recovery                      │
                            └────────────────────────────────────────────┘

     ════════════════════ FAIL-SAFE DEGRADATION ════════════════════
     [Any State] ──► (WDT Trip / Process Fault / Communication Drop > 5s)
                           │
                           ▼
              ┌───────────────────────────┐
              │  FAILSAFE_FLASHING_AMBER  │
              │(Hardware Relays Flashing) │
              └───────────────────────────┘
```

* **NORMAL:** Signal cycles between North-South and East-West according to civil timings.
* **PREPARE:** Preemption received; current green phase terminates; mandatory minimum yellow clearance (≥ 3.0s) and all-red clearance (≥ 1.0s) enforced.
* **GREEN_CORRIDOR:** Preemption direction illuminated GREEN; conflicting directions locked RED.
* **RESTORE:** Ambulance clears exit geofence; green corridor terminates with safe yellow clearance before restoring normal civil cycle.
* **FAILSAFE_FLASHING_AMBER:** Emergency fallback state triggered by hardware watchdog timeout, software crash, or safety interlock violation.

---

## 9. Error Codes & Fault Handling

Every module utilizes standard error codes structured as 16-bit integers paired with standardized mnemonic strings:

| Error Code | Mnemonic String | Origin Module | Severity | Description & Remedial Action |
|---|---|---|---|---|
| **`ERR_001`** | `GPS_TIMEOUT` | GPS Tracking | WARNING | NMEA serial frames ceased > 1000ms. Automatically switches to IMU dead-reckoning. |
| **`ERR_002`** | `MQTT_DISCONNECTED` | Network Comm | ERROR | Primary broker link severed. Activates local store-and-forward queue and V2I fallback. |
| **`ERR_003`** | `INVALID_MESSAGE` | All Modules | WARNING | Inbound payload failed schema validation or had corrupt checksum. Packet discarded. |
| **`ERR_004`** | `TRAFFIC_OFFLINE` | Dynamic Corridor | ERROR | Target traffic controller unresponsive. Alerts driver and initiates local V2I broadcast. |
| **`ERR_005`** | `VITALS_FAILURE` | Patient Vitals | WARNING | Sensor lead detachment detected. Emits audio advisory to paramedic in cabin. |
| **`ERR_006`** | `PREEMPT_REJECTED` | Traffic Controller | ERROR | Preemption request rejected due to conflicting higher-priority preemption lock. |
| **`ERR_007`** | `AUTH_TOKEN_EXPIRED`| Authentication | ERROR | JWT session token expired. Forces client to refresh credentials via mutual TLS. |
| **`ERR_008`** | `REPLAY_ATTACK_DET` | Authentication | CRITICAL | Monotonic counter regression or timestamp skew > 5s detected. Source IP blocked. |
| **`ERR_009`** | `WDT_STARVATION` | Safety Monitor | FATAL | Real-time thread missed heartbeat deadline. Hardware watchdog asserts hardware reset. |
| **`ERR_010`** | `ROUTE_BLOCKED` | Route Planning | WARNING | Road closure detected on active trajectory. Immediate dynamic reroute recalculation. |
| **`ERR_011`** | `DUAL_GREEN_ABORT` | Traffic Controller | FATAL | Hardware/software interlock detected conflicting green request. Drops to flashing amber. |
| **`ERR_012`** | `STORAGE_FULL` | Logging | WARNING | Local disk capacity > 90%. Triggers FIFO log compression and oldest non-critical deletion. |

---

## 10. Timing Constants, Timeouts & Deadlines

To guarantee deterministic real-time behavior, the following constants are hard boundaries across all implementations:

| Timing Parameter | Nominal Value | Maximum Tolerable Value | Failure Action Upon Exceedance |
|---|---|---|---|
| **Heartbeat Interval** | 200 ms (5 Hz) | 500 ms | Mark node `DEGRADED`; if > 1000ms, mark `OFFLINE`. |
| **Hardware WDT Timeout** | 500 ms | 600 ms | Hardware asserts cold system reboot via SoC reset line. |
| **GPS Kinematic Period** | 100 ms (10 Hz) | 300 ms | Switch to IMU dead-reckoning model; raise `ERR_001`. |
| **Vitals Telemetry Period** | 1,000 ms (1 Hz) | 2,500 ms | Display stale telemetry warning on Hospital Dashboard. |
| **MQTT Reconnect Backoff** | 500 ms base | 5,000 ms ceiling | Store-and-forward queue accumulates telemetry in RAM. |
| **Preemption Yellow Clearance**| 3,000 ms | Fixed minimum | Hard safety rule; yellow clearance cannot be truncated. |
| **Preemption All-Red Clearance**| 1,000 ms | Fixed minimum | Hard safety rule; all-red clearance cannot be truncated. |
| **Max Preemption Hold** | 20,000 ms | 45,000 ms | Auto-release corridor to prevent civil gridlock if exit missed. |
| **Hospital ACK Timeout** | 5,000 ms | 15,000 ms | Escalate alarm audible volume; re-publish over backup channel. |
| **Priority Decision Deadline** | 15 ms | 50 ms | Default to distance-based first-come priority allocation. |

---

## 11. Interface Versioning & Evolution

To maintain long-term stability and prevent breaking changes across heterogeneous nodes:

### 11.1 Semantic Versioning (`MAJOR.MINOR.PATCH`)
* **MAJOR (e.g., v2.0):** Incompatible schema modifications (field deletions, altered semantics, changed encryption standards). Requires synchronized fleetwide deployment.
* **MINOR (e.g., v1.1):** Backward-compatible additions (optional telemetry fields, new metadata tags). Existing consumers ignore unknown fields.
* **PATCH (e.g., v1.0.1):** Backward-compatible bugfixes and performance optimizations.

### 11.2 Backward Compatibility Guarantees
* Schema parsers must support unknown field tolerance (ignore unrecognized JSON keys).
* Deprecated fields must be maintained for at least one full minor version cycle before removal.

---

## 12. Reserved Interfaces for Future Expansion

The following communication endpoints and topic structures are formally reserved for subsequent system phases:

```
┌────────────────────────────────────────────────────────────────────────┐
│                        RESERVED FUTURE INTERFACES                      │
│                                                                        │
│   • Police Perimeter Escort:      aegis/police/escort/requests         │
│   • Fire Dept Co-Dispatch:        aegis/fire/mutual_aid                │
│   • AI Traffic Prediction:        aegis/analytics/flow_prediction      │
│   • Regional Cloud Data Lake:     aegis/cloud/telemetry/archive        │
│   • 5G NR C-V2X Sidelink (PC5):   Direct PC5 MAC-layer frame hooks     │
└────────────────────────────────────────────────────────────────────────┘
```

* `aegis/police/escort/requests` (Reserved): Direct payload push to automated law enforcement patrol vehicle terminals.
* `aegis/fire/mutual_aid` (Reserved): Joint dispatch synchronization for structural fire and heavy rescue operations.
* `aegis/analytics/flow_prediction` (Reserved): High-throughput machine learning stream predicting citywide traffic queue recovery post-corridor.
* `aegis/cloud/telemetry/archive` (Reserved): Cold-storage data warehouse synchronization for municipal transit policy optimization.

---

## 13. Document Quality & Compliance Verification

This Interface Control Document has been reviewed and cross-referenced against `SYSTEM_ARCHITECTURE.md`, `DEVELOPMENT_PLAN.md`, and `AI_RULEBOOK.md`.

* **Interface Invariance:** No defined interface bypasses architectural boundaries.
* **Safety Mandate:** Hardware relay interlocks and fail-safe transitions take precedence over all software commands.
* **Traceability:** Every message corresponds directly to defined module responsibilities and system requirements (`REQ-001` through `REQ-025`).

---
**END OF INTERFACE CONTROL DOCUMENT**
