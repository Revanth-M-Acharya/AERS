# Changelog
All notable changes to the SMART AEGIS project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
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
