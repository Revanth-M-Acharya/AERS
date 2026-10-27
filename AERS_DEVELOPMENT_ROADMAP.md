# AERS DEVELOPMENT ROADMAP: PATHWAY TO PRODUCTION V1.0

**Document Identifier:** RDM-AERS-EXEC-2026-V1.0  
**Project Name:** AERS (Autonomous Emergency Response System)  
**Current Baseline:** Version 0.8.0 (`develop` branch, Git Tag `v0.8.0`)  
**Target Release:** Version 1.0.0 Production Release  
**Primary References:**  
* [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md)  
* [`AERS_ENGINEERING_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_ENGINEERING_RULEBOOK.md)  
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md)  
**Target Hardware Architecture:** Distributed Cluster of Four (4) Raspberry Pi Nodes (No ESP32, No Arduino)  
**Verification Baseline:** 18 Automated Test Suites (100% Passing)  

---

## TABLE OF CONTENTS

1. [Executive Summary & Repository Audit](#1-executive-summary--repository-audit)
2. [Current State Assessment](#2-current-state-assessment)
3. [Phase-by-Phase Roadmap & Remaining Engineering Work](#3-phase-by-phase-roadmap--remaining-engineering-work)
   * [Phase 1: Foundation, Utilities & Configuration](#phase-1-foundation-utilities--configuration)
   * [Phase 2: Communication Infrastructure & Network Backbone](#phase-2-communication-infrastructure--network-backbone)
   * [Phase 3: Coordinator Decision Engine & Dynamic Green Corridor](#phase-3-coordinator-decision-engine--dynamic-green-corridor)
   * [Phase 4: Ambulance Kinematics, Telemetry & V2V Module](#phase-4-ambulance-kinematics-telemetry--v2v-module)
   * [Phase 5A: Intersection Node Foundation & Raspberry Pi Deployment](#phase-5a-intersection-node-foundation--raspberry-pi-deployment)
   * [Phase 5B: MQTT Broker Integration & Edge Signal Resiliency](#phase-5b-mqtt-broker-integration--edge-signal-resiliency)
   * [Phase 6: Hospital Emergency Ingestion & Dashboard Backend](#phase-6-hospital-emergency-ingestion--dashboard-backend)
   * [Phase 7: Traffic Management Center & Dashboard Backend](#phase-7-traffic-management-center--dashboard-backend)
   * [Phase 8: Multi-Ambulance Conflict Resolution & Intelligent Priority Scheduling](#phase-8-multi-ambulance-conflict-resolution--intelligent-priority-scheduling)
   * [Phase 9: Physical Raspberry Pi GPIO Relay Interfacing & NMEA GPS Integration](#phase-9-physical-raspberry-pi-gpio-relay-interfacing--nmea-gps-integration)
   * [Phase 10: Cellular C-V2X (PC5 Sidelink) Direct Ad-Hoc Interoperability](#phase-10-cellular-c-v2x-pc5-sidelink-direct-ad-hoc-interoperability)
   * [Phase 11: Safety Monitoring, Hardware Watchdogs & Failsafe Supervision](#phase-11-safety-monitoring-hardware-watchdogs--failsafe-supervision)
   * [Phase 12: Production Hardening, System Integration & Release 1.0 Packaging](#phase-12-production-hardening-system-integration--release-10-packaging)
4. [Cross-Cutting Gap Analysis](#4-cross-cutting-gap-analysis)
5. [Final Milestone Roadmap to Version 1.0](#5-final-milestone-roadmap-to-version-10)

---

# 1 Executive Summary & Repository Audit

### 1.1 Purpose of this Document
This roadmap provides an exhaustive, factual, repository-grounded blueprint of all **remaining engineering work** required to transition AERS from its current verified state (**Version 0.8.0**) to a fully deployable, production-grade cyber-physical system (**Version 1.0.0**). It explicitly documents what has been implemented, what remains partially completed or stubbed with placeholders, and what must be engineered to achieve commercial and municipal certification.

### 1.2 Invariant System Topography
The system architecture remains fixed across all future phases:
* **Raspberry Pi 1:** Central Coordinator (`192.168.1.101:9000` / QNX Neutrino RTOS 7.1 or Embedded Linux)
* **Raspberry Pi 2:** Intersection Node A / `INT_A` (`192.168.1.102:9001`)
* **Raspberry Pi 3:** Intersection Node B / `INT_B` (`192.168.1.103:9002`)
* **Raspberry Pi 4:** Intersection Node C / `INT_C` (`192.168.1.104:9003`)
* **Strict Hardware Invariant:** **NO ESP32 devices, NO Arduino boards, and NO microcontroller-based traffic controllers** are permitted.

---

# 2 Current State Assessment

An exhaustive audit of the codebase on branch `develop` (Commit `9036771`, Tag `v0.8.0`) reveals:

| Subsystem / Layer | Current Implementation State | Production Readiness | Primary Source Files |
|---|---|---|---|
| **Common Utilities** | Complete, fully tested | Production-Ready | `src/common/*.cpp`, `include/aegis/common/*.hpp` |
| **Configuration** | Dual-loader operational (`NodeConfig`) | Production-Ready | `src/config/NodeConfig.cpp`, `config/deployment/*.json` |
| **Communication (TCP)** | Complete POSIX stream sockets | Production-Ready | `src/communication/*.cpp` |
| **Networking (MQTT)** | Transports, serializers, router verified | Production-Ready | `src/network/*.cpp` |
| **Coordinator Core** | Event queue, corridors, registry active | Production-Ready | `src/coordinator/*.cpp` |
| **Intersection Nodes** | Edge FSM, heartbeats, 4-node deployment | Production-Ready | `src/intersection/*.cpp` |
| **Medical Triage Engine**| 14-field patient profile, 12 conditions | Production-Ready | `src/medical/*.cpp` |
| **Intelligent Scheduler**| 5-factor priority, conflict engine, override | Production-Ready | `src/scheduler/*.cpp` |
| **Logging & Telemetry** | 13-event recorder, JSON export | Production-Ready | `src/logger/EventLogger.cpp`, `SimulationRecorder.cpp` |
| **Ambulance Kinematics** | Simulated GPS & NMEA parser | Partial (UART Serial Pending)| `src/ambulance/gps_tracker.cpp` |
| **Traffic Controller Edge** | FSM & timing complete; GPIO simulated | Partial (Physical Relay Pending)| `src/traffic_controller/gpio_interface.cpp` |
| **Hospital Dashboard** | Class headers defined; WebSocket stubbed | Partial (Contains TODO stubs) | `src/hospital_dashboard/hospital_dashboard_backend.cpp` |
| **Traffic Dashboard** | Class headers defined; WebSocket stubbed | Partial (Contains TODO stubs) | `src/traffic_dashboard/traffic_dashboard_backend.cpp` |
| **Safety Watchdog** | Class headers defined; WDT calls stubbed | Partial (Contains TODO stubs) | `src/safety/watchdog.cpp`, `safety_monitor.cpp` |
| **Physical GPIO Relays**| Abstract interface exists; live driver missing | Planned (Phase 9) | `include/aegis/edge/gpio_interface.hpp` |
| **Cellular C-V2X Mesh** | V2V data structures ready; PC5 RF missing | Planned (Phase 10) | `src/ambulance/v2v_module.cpp` |

---

# 3 Phase-by-Phase Roadmap & Remaining Engineering Work

---

### Phase 1: Foundation, Utilities & Configuration
* **Objective:** Deliver high-performance foundational utilities, math, RFC 4122 UUIDs, high-resolution timestamps, circular buffers, thread-safe queues, and schema-validated configuration loading.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * Geodesic Haversine math (`geo_utils.cpp`), UTC microsecond timestamps (`time_utils.cpp`), UUID v4 generator (`uuid_utils.cpp`).
  * Zero-dependency string tokenization and JSON extraction (`json_helper.cpp`), parameter sanitization (`validator.cpp`).
  * Concurrency primitives: `ThreadSafeQueue.hpp`, `CircularBuffer.hpp`.
  * Multi-target deployment configuration loader (`NodeConfig.cpp`, `NodeConfig.h`) with dual-path resolution supporting root and `build/` execution.
* **Remaining Engineering Work:**
  * Clean up legacy stub comments in [`src/config/config_manager.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/config/config_manager.cpp) to eliminate redundant placeholder references.
* **Dependencies:** None.
* **Required Tests:** [`test_common_foundation`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_common_foundation.cpp), [`test_build_smoke`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_build_smoke.cpp) (Both Passing).
* **Required Documentation:** Synchronized in [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) Sections 6.1 and 6.2.
* **Estimated Implementation Complexity:** Low (Maintenance only).
* **Definition of Completion:** Fully satisfied.

---

### Phase 2: Communication Infrastructure & Network Backbone
* **Objective:** Provide robust, decoupled transport layers supporting POSIX stream sockets, MQTT pub/sub, ICD packet serialization, and token authentication.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * Direct POSIX stream socket server and client (`SocketServer.cpp`, `SocketClient.cpp`) with newline-delimited framing and reconnection loops.
  * Unified `NetworkManager` facade, `Serializer` encoding/decoding 12 ICD JSON schemas.
  * `MessageRouter` with MQTT topic wildcard matching (`+`, `#`), `AuthenticationManager` verifying RBAC and HMAC tokens.
  * `MosquittoTransport` for live broker connectivity and `SimulationTransport` for zero-port unit testing.
* **Remaining Engineering Work:**
  * Add optional TLS wrapper (`openssl` or `mbedtls`) over raw TCP sockets for WAN deployments across untrusted public routers.
* **Dependencies:** Phase 1.
* **Required Tests:** [`test_communication_infrastructure`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_communication_infrastructure.cpp), [`test_communication_module`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_communication_module.cpp) (Both Passing).
* **Required Documentation:** Synchronized in Master Specification Section 9.
* **Estimated Implementation Complexity:** Low.
* **Definition of Completion:** Fully satisfied.

---

### Phase 3: Coordinator Decision Engine & Dynamic Green Corridor
* **Objective:** Implement central grid coordination, dynamic green wave preemption, intersection state registries, and event-driven command dispatching.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * `CoordinatorManager` central daemon core driving network listeners and command distribution.
  * `GreenCorridor` tracking multi-intersection route progression (`INT_A` $\to$ `INT_B` $\to$ `INT_C`), advance steps, preemption extension, and cleanup.
  * `IntersectionRegistry` managing live connection state, heartbeat freshness, and IP/port bindings for all connected nodes.
  * `EventQueue` thread-safe blocking event buffer and `TrafficCommandManager` packet factory.
* **Remaining Engineering Work:**
  * Implement persistent registry state dump/restore to local disk (JSON or SQLite) to preserve active corridor states across coordinator process restarts.
* **Dependencies:** Phase 1, Phase 2.
* **Required Tests:** [`test_coordinator_core`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_coordinator_core.cpp), [`test_coordinator_foundation`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_coordinator_foundation.cpp), [`test_green_corridor`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_green_corridor.cpp) (All Passing).
* **Required Documentation:** Synchronized in Master Specification Section 10.
* **Estimated Implementation Complexity:** Low to Medium.
* **Definition of Completion:** Fully satisfied.

---

### Phase 4: Ambulance Kinematics, Telemetry & V2V Module
* **Objective:** Construct vehicle On-Board Unit (OBU) processing GPS kinematics, dead-reckoning extrapolation, vital signs telemetry, and ad-hoc V2V broadcasts.
* **Current Implementation Status:** **Substantially Complete (Software Layer Complete; Physical Serial UART Hardware Driver Pending)**.
* **Components Completed:**
  * `AmbulanceProfile` and `AmbulanceRegistry` managing vehicle state, patient assignment, and speed/ETA telemetry.
  * `VitalsMonitor` collecting physiological data and emitting abnormal clinical alarms.
  * `V2vModule` broadcasting Basic Safety Messages (BSM) and direct preemption beacons.
  * `GpsTracker` complete with NMEA-0183 sentence parser (`$GPGGA`, `$GPRMC`), checksum validation, coordinate conversion, and route waypoint interpolation.
* **Remaining Engineering Work:**
  * Wire physical Linux serial TTY port handling (`termios` interface on `/dev/ttyAMA0` or `/dev/serial0`) inside `GpsTracker::Initialize` and `GpsTracker::RunLoop` to ingest live NMEA sentences from physical GPS hardware.
* **Dependencies:** Phase 1, Phase 2.
* **Required Tests:** [`test_ambulance_node`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_ambulance_node.cpp), [`test_ambulance_registry`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_ambulance_registry.cpp) (Both Passing).
* **Required Documentation:** Synchronized in Master Specification Section 6.10.
* **Estimated Implementation Complexity:** Medium.
* **Definition of Completion:** Physical UART reading enabled; validated with physical GPS hardware receiver emitting valid NMEA streams.

---

### Phase 5A: Intersection Node Foundation & Raspberry Pi Deployment
* **Objective:** Deliver edge intersection daemons executing natively on Raspberry Pis 2, 3, and 4, complete with registration handshake, keepalives, coordinator runtime monitoring, and deployment automation.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * `IntersectionNode` edge daemon handling coordinator connection, preemption command reception, and state dispatch.
  * `SignalController` enforcing state machine intervals (`RED`, `YELLOW`, `GREEN`, `ALL_RED`, `FLASHING_YELLOW`).
  * `HeartbeatManager` emitting periodic 1 Hz keepalives and detecting link failures.
  * `CoordinatorManager` Phase 5A runtime monitoring: live console logging (`[CONNECT]`, `[REGISTER]`, `[HEARTBEAT]`, `[DISCONNECT]`, `[RECONNECT]`), thread-safe in-memory connected node table, and `PrintNetworkStatus` helper function.
  * Deployment configurations (`rpi1_coordinator.json`, `rpi2_intersection_a.json`, `rpi3_intersection_b.json`, `rpi4_intersection_c.json`).
  * Multi-process cluster launch automation (`scripts/start_all_nodes.sh`, `scripts/start_rpi*.sh`).
* **Remaining Engineering Work:**
  * Generate `systemd` unit service files (`aegis-coordinator.service`, `aegis-intersection.service`) for automatic daemon startup upon Raspberry Pi boot.
* **Dependencies:** Phase 1, Phase 2, Phase 3.
* **Required Tests:** [`test_intersection_node`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_intersection_node.cpp), [`test_rpi_deployment`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/integration/test_rpi_deployment.cpp), [`test_coordinator_monitoring`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_coordinator_monitoring.cpp) (All Passing).
* **Required Documentation:** Synchronized in Master Specification Sections 7, 11, and 15.
* **Estimated Implementation Complexity:** Low.
* **Definition of Completion:** Fully satisfied.

---

### Phase 5B: MQTT Broker Integration & Edge Signal Resiliency
* **Objective:** Provide edge resilience against network drops, reconnect synchronization, duplicate publication suppression, and fail-safe signal reversion.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * `TrafficSignalNode` integrating MQTT edge communication with local signal controllers.
  * `PreemptionManager` and `RecoveryManager` ensuring smooth clearance and civilian timing restoration post-preemption.
  * Fail-safe degradation: Automatic transition to `FLASHING_YELLOW` or local fixed cycling upon link loss ($> 3000$ms).
* **Remaining Engineering Work:**
  * Physical relay switching latency benchmarks on real Raspberry Pi hardware (covered under Phase 9).
* **Dependencies:** Phase 2, Phase 5A.
* **Required Tests:** [`test_traffic_signal_node`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_traffic_signal_node.cpp) (15/15 test cases Passing).
* **Required Documentation:** Synchronized in Master Specification Sections 6.11 and 11.
* **Estimated Implementation Complexity:** Low.
* **Definition of Completion:** Fully satisfied.

---

### Phase 6: Hospital Emergency Ingestion & Dashboard Backend
* **Objective:** Provide a real-time hospital triage gateway streaming ambulance ETA countdowns, live patient physiological vitals, and bidirectional clinical preparation advice.
* **Current Implementation Status:** **Partial / Placeholder Backend**.
* **Components Completed:**
  * C++ interface header [`include/aegis/dashboard/hospital_dashboard_backend.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/dashboard/hospital_dashboard_backend.hpp).
  * Data structures for patient vitals packets, ETA countdowns, and clinical alerts.
* **Remaining Engineering Work:**
  * Replace placeholder TODO comments in [`src/hospital_dashboard/hospital_dashboard_backend.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/hospital_dashboard/hospital_dashboard_backend.cpp) with a functional WebSocket server or SSE (Server-Sent Events) HTTP endpoint.
  * Implement framed JSON vitals streaming to browser clients.
  * Implement authenticated bidirectional clinical advice reception (hospital back to ambulance).
* **Dependencies:** Phase 2, Phase 4.
* **Required Tests:** Create new test suite `tests/unit/test_hospital_dashboard.cpp` verifying WebSocket connection, vitals frame distribution, and advice command parsing.
* **Required Documentation:** Update Master Specification Section 6.17 and create user manual in `docs/dashboards/`.
* **Estimated Implementation Complexity:** Medium.
* **Definition of Completion:** Real-time web browser client receives 1 Hz vitals telemetry and updates dynamic ETA countdown without dropped packets.

---

### Phase 7: Traffic Management Center & Dashboard Backend
* **Objective:** Provide supervisory municipal traffic operators with real-time intersection phase telemetry, green corridor visualization, and authenticated manual override capabilities.
* **Current Implementation Status:** **Partial / Placeholder Backend**.
* **Components Completed:**
  * C++ interface header [`include/aegis/dashboard/traffic_dashboard_backend.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/dashboard/traffic_dashboard_backend.hpp).
  * Data structures for phase state reports, queue density, and preemption overrides.
  * Core manual override engine implemented in [`src/control/ManualOverride.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/control/ManualOverride.cpp) and [`src/scheduler/ManualOverrideManager.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/scheduler/ManualOverrideManager.cpp).
* **Remaining Engineering Work:**
  * Replace placeholder TODO comments in [`src/traffic_dashboard/traffic_dashboard_backend.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/traffic_dashboard/traffic_dashboard_backend.cpp) with an active WebSocket server streaming intersection phase changes to GIS mapping clients.
  * Connect operator console inputs directly to `ManualOverrideManager`.
* **Dependencies:** Phase 2, Phase 3, Phase 5A.
* **Required Tests:** Create new test suite `tests/unit/test_traffic_dashboard.cpp` verifying phase telemetry broadcasts and operator command dispatching.
* **Required Documentation:** Update Master Specification Section 6.18.
* **Estimated Implementation Complexity:** Medium.
* **Definition of Completion:** Web client displays real-time signal phases for `INT_A`, `INT_B`, `INT_C` and successfully triggers an authenticated green preemption.

---

### Phase 8: Multi-Ambulance Conflict Resolution & Intelligent Priority Scheduling
* **Objective:** Implement dynamic arrival contention detection, clinical triage scoring, deterministic multi-factor priority arbitration, automated conflict commands, and dispatcher overrides.
* **Current Implementation Status:** **100% Complete & Production-Ready**.
* **Components Completed:**
  * `PatientProfile` (14 attributes) and `PriorityEngine` calculating 0–100 urgency scores across 12 medical conditions, shock index, and delay penalties.
  * `MedicalDatabase` pre-populated with 16 clinical reference cases.
  * `ConflictDetectionEngine` detecting arrival contention within 10.0s time windows.
  * `IntelligentPriorityScheduler` multi-factor scoring (Medical 40%, Category 25%, ETA 20%, Distance 10%, Override 5%) with deterministic tie-breaking.
  * `ConflictResolutionManager` executing `PROCEED`, `HOLD`, `DELAY`, `RESUME`, `ALTERNATE_ROUTE`.
  * `ManualOverrideManager` operator console and `AmbulanceAwarenessService` peer proximity advisories.
  * `MultiAmbulanceSimulation` demonstrating 8 concurrent clinical emergencies.
* **Remaining Engineering Work:** None for core logic.
* **Dependencies:** Phase 3, Phase 4, Phase 5A.
* **Required Tests:** [`test_medical_priority`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_medical_priority.cpp), [`test_conflict_detection`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_conflict_detection.cpp), [`test_manual_override`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_manual_override.cpp), [`test_phase8_intelligent_scheduler`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/unit/test_phase8_intelligent_scheduler.cpp) (All Passing).
* **Required Documentation:** Synchronized in Master Specification Sections 12 and 13.
* **Estimated Implementation Complexity:** Low (Maintenance only).
* **Definition of Completion:** Fully satisfied.

---

### Phase 9: Physical Raspberry Pi GPIO Relay Interfacing & NMEA GPS Integration
* **Objective:** Replace simulation GPIO drivers and simulated GPS kinematics with direct physical hardware interfacing on physical Raspberry Pi boards.
* **Current Implementation Status:** **Not Started (Abstract Interfaces & Pin Mappings Prepared)**.
* **Components Completed:**
  * Abstract interface `IGpioInterface` in [`include/aegis/edge/gpio_interface.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/edge/gpio_interface.hpp).
  * Physical pin mapping specifications in [`RASPBERRY_PI_PIN_MAPPING.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/RASPBERRY_PI_PIN_MAPPING.md) and [`HARDWARE_ABSTRACTION_DOCUMENT.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/HARDWARE_ABSTRACTION_DOCUMENT.md).
* **Remaining Engineering Work:**
  1. Implement `RPiGpioInterface` class implementing `IGpioInterface` using Linux `libgpiod` or memory-mapped `/dev/gpiomem`:
     * Map Broadcom GPIO 17 (Red), GPIO 27 (Yellow), GPIO 22 (Green) for primary approach.
     * Map GPIO 23, 24, 25 for secondary approach.
     * Configure active-low relay switching logic with optocoupler protection.
  2. Implement hardware UART serial reader in `GpsTracker`:
     * Open `/dev/serial0` at 9600 baud with 8N1 serial configuration.
     * Ingest continuous NMEA sentences into the existing parser pipeline.
  3. Compile and verify on physical Raspberry Pi OS 64-bit hardware.
* **Dependencies:** Phase 4, Phase 5A, Phase 5B.
* **Required Tests:**
  * Compile and run [`tests/hardware/test_gpio_timing.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/tests/hardware/test_gpio_timing.cpp) on physical Raspberry Pis.
  * Verify physical relay switching latencies ($\le 10$ms) with an oscilloscope or multimeter.
* **Required Documentation:** Update Master Specification Section 15 and create wiring verification logs in `hardware/`.
* **Estimated Implementation Complexity:** High.
* **Definition of Completion:** Physical LED signal heads on Raspberry Pis 2, 3, 4 switch in real time upon corridor preemption triggered by physical GPS coordinate updates.

---

### Phase 10: Cellular C-V2X (PC5 Sidelink) Direct Ad-Hoc Interoperability
* **Objective:** Enable direct, ad-hoc wireless V2V and V2I communication between emergency vehicles and roadside intersections, providing decentralized preemption fallback during cellular WAN outages.
* **Current Implementation Status:** **Planned / Architecture Designed**.
* **Components Completed:**
  * `BasicSafetyMessage` (BSM) and `DirectPreemptionBeacon` data structures in [`include/aegis/edge/v2v_module.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/edge/v2v_module.hpp).
  * Direct preemption parsing logic in `TrafficSignalNode`.
* **Remaining Engineering Work:**
  * Implement UDP broadcast / multicast socket transport over ad-hoc Wi-Fi (IEEE 802.11p / 802.11n ad-hoc mode) binding to `239.255.0.1:9090`.
  * Implement direct local preemption beacon verification: If central coordinator heartbeats drop, edge intersections accept authenticated direct V2I preemption beacons from nearby ambulances ($\le 300$m).
* **Dependencies:** Phase 4, Phase 5A, Phase 5B.
* **Required Tests:** Create test suite `tests/unit/test_v2x_direct_ad_hoc.cpp` asserting direct preemption activation during coordinator disconnect.
* **Required Documentation:** Master Specification Section 20.2.
* **Estimated Implementation Complexity:** High.
* **Definition of Completion:** Ambulances trigger green preemption on Intersection Node A over direct ad-hoc wireless beaconing while the Central Coordinator is completely powered off.

---

### Phase 11: Safety Monitoring, Hardware Watchdogs & Failsafe Supervision
* **Objective:** Provide hard real-time safety supervision, servicing physical hardware watchdogs (`/dev/watchdog`), detecting thread stalls, and guaranteeing fail-safe state reversion within 500ms.
* **Current Implementation Status:** **Partial / Placeholder Code in Repository**.
* **Components Completed:**
  * Class declarations for `Watchdog` and `SafetyMonitor` in [`include/aegis/safety/watchdog.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/safety/watchdog.hpp) and [`safety_monitor.hpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/safety/safety_monitor.hpp).
* **Remaining Engineering Work:**
  1. Replace placeholder TODO comments in [`src/safety/watchdog.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/safety/watchdog.cpp):
     * Implement physical `/dev/watchdog` file descriptor opening and `ioctl(fd, WDIOC_KEEPALIVE)` strobe.
     * Implement fallback software watchdog thread for non-Linux/host environments.
  2. Replace placeholder TODO comments in [`src/safety/safety_monitor.cpp`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/safety/safety_monitor.cpp):
     * Implement thread heartbeat registration map (`RegisterHeartbeat(threadName)`).
     * Verify all registered threads have checked in within their respective deadlines ($\le 500$ms) inside `SafetyMonitor::Pulse()`.
     * If any thread misses two consecutive deadlines, immediately trigger fail-safe transition (`FLASHING_YELLOW`) and withhold WDT kick to force controlled reboot.
* **Dependencies:** Phase 1, Phase 5A.
* **Required Tests:** Create test suite `tests/unit/test_safety_watchdog.cpp` injecting artificial thread stalls and verifying fail-safe degradation within $\le 500$ms.
* **Required Documentation:** Update Master Specification Section 6.13 and add safety compliance matrix.
* **Estimated Implementation Complexity:** Medium.
* **Definition of Completion:** Watchdog supervisor detects simulated worker deadlock and triggers failsafe relay release within 500ms.

---

### Phase 12: Production Hardening, System Integration & Release 1.0 Packaging
* **Objective:** Deliver fully packaged, systemd-orchestrated, production-ready software release (Version 1.0.0) deployable across physical Raspberry Pi hardware.
* **Current Implementation Status:** **Pending Milestones 1–4**.
* **Components Completed:**
  * Clean C++17 compilation with zero warnings.
  * Native build and run automation scripts in `scripts/`.
* **Remaining Engineering Work:**
  1. Author systemd service unit configurations:
     * `aegis-coordinator.service` (for RPi 1)
     * `aegis-intersection-a.service` (for RPi 2)
     * `aegis-intersection-b.service` (for RPi 3)
     * `aegis-intersection-c.service` (for RPi 4)
  2. Implement automated Debian packaging (`.deb`) for Raspberry Pi OS 64-bit.
  3. Execute 24-hour continuous burn-in stress test across the physical 4-Pi cluster without memory leaks, crashes, or unhandled socket drops.
  4. Final documentation synchronization and release tagging (`v1.0.0`).
* **Dependencies:** All preceding phases (Phases 1 through 11).
* **Required Tests:** End-to-end multi-hour cluster regression test runner.
* **Required Documentation:** Operational manual, hardware deployment checklist, and final Release Notes.
* **Estimated Implementation Complexity:** Medium.
* **Definition of Completion:** Four physical Raspberry Pis boot unattended, establish coordinated green wave execution upon ambulance dispatch, and pass a 24-hour endurance test with zero faults.

---

# 4 Cross-Cutting Gap Analysis

### 4.1 Implementation Status Matrix

```
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                               PHASE MATURITY CLASSIFICATION                            │
├───────────────────────────────┬───────────────────────────────────┬────────────────────┤
│ FULLY COMPLETE & VERIFIED     │ PARTIAL / PLACEHOLDER STUBS       │ PLANNED / ROADMAP  │
├───────────────────────────────┼───────────────────────────────────┼────────────────────┤
│ Phase 1: Foundation Utilities │ Phase 4: GPS Physical UART        │ Phase 9: GPIO      │
│ Phase 2: Comm & Sockets       │ Phase 6: Hospital Dashboard WS    │          Relays &  │
│ Phase 3: Coordinator Core     │ Phase 7: Traffic Dashboard WS     │          Serial GPS│
│ Phase 5A: Intersection Nodes  │ Phase 11: Safety Watchdogs / WDT  │ Phase 10: C-V2X    │
│ Phase 5B: Resilient Traffic   │                                   │           Ad-Hoc   │
│ Phase 8: Intelligent Scheduler│                                   │ Phase 12: Prod V1.0│
└───────────────────────────────┴───────────────────────────────────┴────────────────────┘
```

### 4.2 Modules Requiring Engineering Refinement

| Module | Source File | Current Code Deficiency | Required Engineering Action |
|---|---|---|---|
| **Safety Watchdog** | `src/safety/watchdog.cpp` | Lines 11 & 16 contain `TODO` comments; no `/dev/watchdog` calls | Implement physical Linux `ioctl` WDT driver with fallback software timer |
| **Safety Monitor** | `src/safety/safety_monitor.cpp` | Lines 14 & 18 contain `TODO` comments; `IsSystemHealthy()` hardcoded to `true` | Implement thread registration map, deadline verification, and fail-safe triggers |
| **Hospital Backend** | `src/hospital_dashboard/hospital_dashboard_backend.cpp` | Lines 11 & 16 contain `TODO` comments; no socket listener | Implement lightweight WebSocket/HTTP server pushing framed JSON vitals |
| **Traffic Backend** | `src/traffic_dashboard/traffic_dashboard_backend.cpp` | Lines 11 & 16 contain `TODO` comments; no stream listener | Implement WebSocket server streaming intersection phase updates to GIS clients |
| **GPS Tracker** | `src/ambulance/gps_tracker.cpp` | Line 42 notes physical TTY omitted; runs in simulation mode | Implement physical `/dev/serial0` UART reader using POSIX `termios` |
| **GPIO Interface** | `src/traffic_controller/gpio_interface.cpp` | Only `SimulationGpioInterface` implemented | Implement `RPiGpioInterface` driving physical Broadcom GPIO pins via `libgpiod` |

---

# 5 Final Milestone Roadmap to Version 1.0

The following five sequential milestones outline the exact critical path from Version 0.8.0 to Version 1.0.0.

```
 [Current: v0.8.0]
        │
        ▼
 ┌──────────────┐
 │ Milestone 1  │──► Physical Hardware GPIO Relay & Serial GPS Drivers (Phase 9 & 4)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ Milestone 2  │──► Safety Monitor & Hardware Watchdog Implementation (Phase 11)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ Milestone 3  │──► Live WebSockets for Hospital & Traffic Dashboards (Phases 6 & 7)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ Milestone 4  │──► Direct Ad-Hoc V2I Wireless Fallback (Phase 10)
 └──────┬───────┘
        │
        ▼
 ┌──────────────┐
 │ Milestone 5  │──► Systemd Service Units, Packaging & 24h Burn-in (Release 1.0.0)
 └──────────────┘
```

---

### Milestone 1: Physical Hardware GPIO Relay & Serial GPS Drivers
* **Purpose:** Transition AERS from software simulation to physical hardware actuation on physical Raspberry Pi boards.
* **Dependencies:** Current baseline (`v0.8.0`).
* **Deliverables:**
  1. `src/traffic_controller/rpi_gpio_interface.cpp` driving Broadcom GPIO 17, 27, 22, 23, 24, 25.
  2. Physical UART serial ingestion in `src/ambulance/gps_tracker.cpp` reading `/dev/serial0`.
  3. Hardware timing verification test passing on physical Raspberry Pi OS 64-bit.
* **Estimated Effort:** 3–4 Days.
* **Exit Criteria:** Physical relay board LEDs cycle correctly in response to coordinator preemption commands; serial GPS receiver updates vehicle kinematics at 10 Hz.

---

### Milestone 2: Safety Supervision & Hardware Watchdog Implementation
* **Purpose:** Eliminate safety stubs and provide hard failsafe supervision with hardware watchdog reset capabilities.
* **Dependencies:** Milestone 1.
* **Deliverables:**
  1. Fully functional `src/safety/watchdog.cpp` supporting Linux `/dev/watchdog` and QNX HAM.
  2. Thread deadline monitor in `src/safety/safety_monitor.cpp` monitoring all edge loops.
  3. Automated unit test suite `tests/unit/test_safety_watchdog.cpp`.
* **Estimated Effort:** 2 Days.
* **Exit Criteria:** Simulated thread deadlock causes safety monitor to trip failsafe flashing amber within $\le 500$ms and trigger a controlled hardware reboot.

---

### Milestone 3: Live WebSockets for Hospital & Traffic Dashboards
* **Purpose:** Replace dashboard placeholder comments with real-time browser telemetry streaming.
* **Dependencies:** Milestone 1.
* **Deliverables:**
  1. Lightweight C++ WebSocket server integrated into `src/hospital_dashboard/hospital_dashboard_backend.cpp`.
  2. Lightweight C++ WebSocket server integrated into `src/traffic_dashboard/traffic_dashboard_backend.cpp`.
  3. New unit test suites `tests/unit/test_hospital_dashboard.cpp` and `tests/unit/test_traffic_dashboard.cpp`.
  4. Integration of `ManualOverrideManager` with live web console override commands.
* **Estimated Effort:** 3 Days.
* **Exit Criteria:** Web client displays live 1 Hz patient vitals, dynamic ETA countdown, and intersection phase states; operator can trigger manual preemption from browser.

---

### Milestone 4: Direct Ad-Hoc V2I Wireless Fallback (Phase 10)
* **Purpose:** Ensure emergency vehicles can preempt traffic lights even when the Central Coordinator or municipal Wi-Fi router is offline.
* **Dependencies:** Milestone 1, Milestone 2.
* **Deliverables:**
  1. Ad-hoc UDP multicast beacon sender in `src/ambulance/v2v_module.cpp`.
  2. Direct V2I beacon listener in `src/intersection/IntersectionNode.cpp`.
  3. Automated fallback test in `tests/integration/test_v2i_fallback.cpp`.
* **Estimated Effort:** 3 Days.
* **Exit Criteria:** Turning off Raspberry Pi 1 (Coordinator) causes Intersection A (RPi 2) to accept direct wireless preemption from approaching ambulance node within 200ms.

---

### Milestone 5: System Integration, Packaging & Production Release 1.0
* **Purpose:** Complete final hardening, systemd automation, and 24-hour multi-node endurance validation for official Release 1.0.0.
* **Dependencies:** Milestones 1, 2, 3, 4.
* **Deliverables:**
  1. Production `systemd` service files for all four Raspberry Pi nodes.
  2. Automated Debian `.deb` package generation scripts.
  3. 24-hour endurance test report with zero memory leaks, thread stalls, or socket crashes.
  4. Updated [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and [`CHANGELOG.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CHANGELOG.md).
  5. Annotated Git Tag `v1.0.0` pushed to repository.
* **Estimated Effort:** 3 Days.
* **Exit Criteria:** Unattended four-Raspberry Pi cluster boots automatically, establishes coordinated green corridor preemption upon vehicle dispatch, resolves multi-vehicle conflicts safely, and runs for 24 hours without a single fault.
