# Changelog
All notable changes to the SMART AEGIS project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- Complete Phase 5A: Traffic Signal Node Foundation (`aegis_traffic_controller`) in modern C++17:
  1. `IGpioInterface` & `SimulationGpioInterface`: Pure virtual hardware abstraction layer decoupling traffic controller logic from physical GPIO/relay hardware. `SimulationGpioInterface` logs all simulated lamp actuations (`SET_RED`, `SET_YELLOW`, `SET_GREEN`, `FLASH_YELLOW`, `FLASH_RED`, `ALL_OFF`, `SHUTDOWN`) without hardware dependencies.
  2. `SignalStateMachine`: Deterministic Finite State Machine enforcing valid normal cycles (`RED -> GREEN -> YELLOW -> RED`) and emergency preemption sequences (`Current -> ALL_RED -> GREEN -> Hold Green -> Recovery -> YELLOW -> RED`), while strictly rejecting hazardous transitions (e.g., `GREEN -> RED`, `YELLOW -> GREEN`, `RED -> YELLOW`).
  3. `PreemptionManager`: Thread-safe preemption status manager (`StartEmergency()`, `CancelEmergency()`, `IsEmergencyActive()`) logging activation and cancellation events without timing or hardware coupling.
  4. `RecoveryManager`: Thread-safe recovery cycle tracker (`StartRecovery()`, `RecoveryFinished()`, `Reset()`, `IsRecoveryActive()`) logging transition progress.
  5. `TrafficController`: Master roadside intersection controller coordinating the state machine, preemption manager, recovery manager, and GPIO abstraction with configurable phase timings (`TrafficTimingConfig`).
  6. `TrafficSignalNode`: Top-level edge daemon managing controller lifecycle (`Start()`, `Stop()`, `Run()`, `Tick()`).
- Comprehensive automated unit test suite (`tests/unit/test_traffic_signal_node.cpp`) covering state transitions, emergency activation, preemption cancellation, post-clearance recovery, invalid transition rejection, GPIO simulation logging, controller lifecycle, and node execution with 100% pass rate.

### Changed
- Cleaned up include hierarchy and build configuration before Phase 4:
  - Removed 18 obsolete Phase 1 placeholder header files from `src/` to eliminate shadowing of public API in `include/aegis/`.
  - Preserved internal socket loopback header `src/network/socket_loopback.hpp`.
  - Migrated `tests/unit/test_common_geo.cpp` to include `"aegis/common/geo_utils.hpp"` instead of relative path to `src/`.
  - Removed `${CMAKE_CURRENT_SOURCE_DIR}/src` from global include directories in root `CMakeLists.txt` and removed `-Isrc` from `Makefile`.
  - Added private include path for `aegis_network` target to encapsulate internal headers.
  - Fixed argument handling in `scripts/build.sh` so both `./scripts/build.sh test` and `./scripts/build.sh <type> test` trigger automated test execution.

### Added
- Complete Phase 4: Ambulance Node (OBU) Implementation (`aegis_ambulance`) in modern C++17:
  1. `GpsTracker`: 10 Hz kinematic positioning engine featuring NMEA-0183 serial sentence parser (`$GPRMC`, `$GPGGA`) with checksum validation, coordinate conversion, waypoint simulation step interpolation, and Extended Kalman Filter (EKF) / Dead-Reckoning fallback upon GNSS fix loss.
  2. `VitalsMonitor`: Real-time physiological patient telemetry engine generating multi-parameter vitals (Heart Rate, SpO2, Blood Pressure, Respiration Rate, Core Temperature, and GCS). Implements acute deterioration evaluation and instant alarm callbacks for critical conditions (acute hypoxia, cardiac arrest, hypotensive crisis, and severe pyrexia).
  3. `V2vModule`: Autonomous ad-hoc wireless transceiver generating SAE J2735-compatible Basic Safety Messages (BSMs) at 10 Hz over UDP port 8999, along with direct vehicle-to-infrastructure (V2I) emergency preemption fallback beacons dispatched to roadside units on UDP port 8998.
  4. `AmbulanceNode`: Main in-vehicle OBU edge daemon orchestrating GPS tracking, physiological telemetry, V2V safety broadcasting, and vehicle operational state transitions (`AVAILABLE`, `DISPATCHED`, `EN_ROUTE_PATIENT`, `ON_SCENE`, `EN_ROUTE_HOSPITAL`, `ARRIVED`). Features local store-and-forward telemetry ring buffering ensuring zero packet loss across WAN dropouts.
  5. `AmbulanceMissionSimulator`: Full incident mission simulator executing end-to-end mission lifecycle (`Dispatch` -> `Travel to patient` -> `Pickup on-scene` -> `Travel to hospital` -> `Arrive at trauma bay` -> `Mission complete`) with continuous coordinate updates.
  6. Python simulation tools updated (`tools/simulator/gps_simulator`, `tools/simulator/vitals_simulator`, `tools/simulator/ambulance_simulator`).
- Comprehensive automated unit test suite (`tests/unit/test_ambulance_node.cpp`) covering all 6 test suites with 100% pass rate.
- Complete Phase 3: Coordinator Core (`aegis_coordinator` and `aegis_route_planner`) in modern C++17:
  1. `CentralCoordinator` (`Coordinator`): Master dispatch orchestrator uniting call intake, fleet tracking, route planning, dynamic green corridor preemption, and MQTT telemetry broadcast.
  2. `EmergencyCallHandler`: 911 distress call qualification, metropolitan geofencing validation, symptom-based triage categorization (`CODE_RED`, `CODE_AMBER`, `CODE_GREEN`), and required capability inference.
  3. `IncidentManager`: Thread-safe incident lifecycle tracker supporting state transitions (`REPORTED` -> `ASSIGNED` -> `EN_ROUTE_SCENE` -> `ON_SCENE` -> `EN_ROUTE_HOSPITAL` -> `RESOLVED`), queries, and ambulance assignment.
  4. `AmbulanceRegistry`: Thread-safe fleet registry maintaining vehicle profiles (ALS vs BLS), location, kinematics, and atomic reservation check-and-set locking.
  5. `FleetManager`: Fleet supervisor API wrapping the ambulance registry and populating initial simulated metropolitan fleet deployments.
  6. `DispatchEngine`: Automated dispatch selection algorithm matching required clinical capabilities, vehicle readiness, and proximity distances with ALS preference for critical incidents.
  7. `RoutePlanner`: Graph-based municipal navigation engine executing A* shortest-time pathfinding with Dijkstra fallback over a 20-node simulated metropolitan street network, computing waypoint coordinates, ordered intersection sequences, and travel ETAs.
  8. `GreenCorridorEngine` (`DynamicGreenCorridor`): Spatial corridor preemption scheduler evaluating vehicle progress along route vectors, activating signal preemption windows, generating signed `TrafficPreemptionRequestMsg` payloads, and releasing intersections upon passage.
- Comprehensive automated unit test suite (`tests/unit/test_coordinator_core.cpp`) covering all 7 test suites with 100% pass rate.
- Complete Phase 2: Communication Infrastructure (`aegis_network`) with 10 production-quality C++17 components:
  1. `NetworkManager`: Unified network subsystem facade coordinating MQTT, Sockets, Auth, and Heartbeat.
  2. `MqttClient`: Thread-safe client supporting QoS 0/1/2, Last Will and Testament (LWT), automatic reconnection, loopback simulation, and offline ring buffering.
  3. `MessageRouter`: Thread-safe topic subscription router supporting exact match, single-level wildcard (`+`), and multi-level wildcard (`#`).
  4. `AuthenticationManager`: Token-based bearer authentication, Role-Based Access Control (RBAC), HMAC-SHA256 equivalent payload signatures, and future-ready mTLS interface.
  5. `RetryPolicy`: Configurable exponential backoff retry policy with jitter bounds and attempt threshold limits.
  6. `HeartbeatManager`: Node heartbeat generation, incremental sequencing, peer timestamp tracking, and dead peer staleness detection.
  7. `Serializer`: Strict JSON serializer and deserializer for all 10 ICD schemas (`EmergencyRequest`, `GPSUpdate`, `VitalsUpdate`, `TrafficPreemptionRequest`, `TrafficStatus`, `HospitalAlert`, `HospitalCommand`, `PriorityDecision`, `Heartbeat`, `FaultEvent`).
  8. `SocketClient`: Lightweight POSIX UDP socket client for direct V2V / V2I ad-hoc beacons with sandboxed datagram fallback.
  9. `SocketServer`: Non-blocking POSIX UDP receiver for local beacon listening with non-blocking poll capability.
  10. `ConnectionManager`: State machine tracking link connectivity states (`DISCONNECTED`, `CONNECTING`, `CONNECTED`, `RECONNECTING`, `FAILED`) and keep-alive ping round-trips.
- Comprehensive automated unit test suite (`tests/unit/test_communication_infrastructure.cpp`) covering all 9 test suites with 100% pass rate.
- Complete Common Foundation Library (`aegis_common`) with 15 reusable components:
  1. Geo utilities (Haversine distance, bearing, ETA, coordinate validation).
  2. Time utilities (monotonic microseconds, UTC milliseconds, ISO 8601 formatting, Stopwatch).
  3. UUID utilities (RFC 4122 UUIDv4, event IDs, incident IDs, vehicle asset formatters).
  4. Generic thread-safe blocking/non-blocking FIFO queue (`ThreadSafeQueue<T>`).
  5. Fixed-capacity, zero-heap circular ring buffer (`CircularBuffer<T, N>`).
  6. Non-throwing monadic result container (`Result<T, E>`).
  7. Structured error descriptor (`AegisError`).
  8. Extensible event base class (`Event`).
  9. Generic thread-safe state machine base (`StateMachine<StateEnum>`).
  10. Non-blocking software timer utility (`SoftwareTimer`).
  11. Lightweight JSON string builder and tokenizer (`JsonHelper`).
  12. System-wide temporal, physical, and network constants.
  13. Strongly typed enumerations for triage, ambulance, and traffic states.
  14. Serialization abstraction interface (`ISerializable`).
  15. Data validators (range checks, string bounds, and CRC-32 checksums).
- Comprehensive automated unit test suite (`tests/unit/test_common_foundation.cpp`) with 100% component coverage.
- Complete Phase 1 build infrastructure: top-level and modular `CMakeLists.txt`, `Makefile`, and `cmake/qnx_toolchain.cmake`.
- Public C++17 header hierarchy in `include/aegis/` for all 12 system modules with namespaces and type safety.
- 12 placeholder static libraries in `src/` conforming to the Software Design Document dependency DAG.
- Target application daemons (`aegis_coordinator`, `aegis_ambulance`, `aegis_traffic_controller`) in `src/apps/`.
- Automated smoke verification test harness (`tests/unit/test_build_smoke.cpp`) compiling and passing with 100% success.
- Complete system architecture specification (`SYSTEM_ARCHITECTURE.md`).
- Multi-tier engineering development roadmap and SRS matrix (`DEVELOPMENT_PLAN.md`).
- Binding AI engineering constitution and coding standard (`AI_RULEBOOK.md`).
- Formal Interface Control Document (ICD) with schemas and error codes (`INTERFACE_SPECIFICATION.md`).
- Complete Software Design Document (`SOFTWARE_DESIGN_DOCUMENT.md`).
