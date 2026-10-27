# AERS ENGINEERING RULEBOOK & CONSTITUTIONAL STANDARD

**Document Identifier:** STD-AERS-CONST-2026-V1.0  
**Project Name:** AERS (Autonomous Emergency Response System)  
**Classification:** Mandatory Engineering Standard (Safety-Critical Cyber-Physical System)  
**Applicability:** All Engineers, System Architects, Quality Assurance Auditors, and AI Assistants  
**Primary Authority Reference:** [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md)  
**Hardware Topology:** Distributed Cluster of Four (4) Raspberry Pi Nodes  
**Target Environments:** QNX Neutrino RTOS 7.1+, Embedded Linux (Raspberry Pi OS 64-bit / Debian), POSIX.1-2017  
**Language Standard:** ISO/IEC 14882:2017 (C++17)  
**Status:** In Force — Mandatory Compliance  

---

## TABLE OF CONTENTS

1. [Project Vision](#1-project-vision)
2. [Document Authority Order](#2-document-authority-order)
3. [Engineering Philosophy](#3-engineering-philosophy)
4. [Working Feature Protection](#4-working-feature-protection)
5. [Repository Rules](#5-repository-rules)
6. [Architecture Rules](#6-architecture-rules)
7. [Hardware Constraints](#7-hardware-constraints)
8. [Hardware Evolution Policy](#8-hardware-evolution-policy)
9. [Software Constraints](#9-software-constraints)
10. [Module Design Rules](#10-module-design-rules)
11. [Communication Rules](#11-communication-rules)
12. [Build System Rules](#12-build-system-rules)
13. [Testing Requirements](#13-testing-requirements)
14. [Phase Completion Criteria](#14-phase-completion-criteria)
15. [Documentation Requirements](#15-documentation-requirements)
16. [Coding Standards](#16-coding-standards)
17. [Git Workflow](#17-git-workflow)
18. [Branching Strategy](#18-branching-strategy)
19. [Release Process](#19-release-process)
20. [Versioning Policy](#20-versioning-policy)
21. [Raspberry Pi Deployment Rules](#21-raspberry-pi-deployment-rules)
22. [Logging Standards](#22-logging-standards)
23. [Configuration Management](#23-configuration-management)
24. [Error Handling Standards](#24-error-handling-standards)
25. [Thread Safety Guidelines](#25-thread-safety-guidelines)
26. [Performance Guidelines](#26-performance-guidelines)
27. [Security Guidelines](#27-security-guidelines)
28. [Definition of Done](#28-definition-of-done)
29. [AI Assistant Guidelines](#29-ai-assistant-guidelines)
30. [Future Development Policy](#30-future-development-policy)

---

# 1 Project Vision

### 1.1 Mission Statement
The **Autonomous Emergency Response System (AERS)** is an advanced distributed cyber-physical system designed to eliminate urban traffic delays for emergency vehicles carrying acute patients. By integrating dynamic green corridors, clinical urgency evaluation, deterministic multi-vehicle intersection conflict resolution, and real-time safety interlocks, AERS minimizes pre-hospital transit times while preserving physical grid safety.

### 1.2 Core Operational Pillars
1. **Dynamic Green Wave Progression:** Traffic signals ahead of an approaching emergency vehicle are preempted autonomously, providing a continuous green corridor without human intervention.
2. **Clinical Urgency Arbitration:** Emergency transit priority is dynamically governed by physiological patient vitals (shock index, hypoxemia, vitals crisis thresholds, incident delay) across 12 medical categories.
3. **Deterministic Conflict Resolution:** When multiple ambulances converge on an intersection within a configurable conflict window (default: 10s), contention is resolved mathematically, ensuring non-conflicting passage and zero deadlocks.
4. **Hard Physical Safety Interlocks:** Yellow clearance intervals ($\ge 3.0$s), all-red buffers ($\ge 1.0$s), and mutual exclusion of conflicting green indications are enforced at the hardware edge.
5. **Distributed Edge Autonomy:** Intersection signal logic executes directly on edge nodes, guaranteeing fail-safe local operation if central network connectivity drops.

---

# 2 Document Authority Order

To eliminate ambiguity across architectural, algorithmic, and operational decisions, the official precedence of project artifacts is defined as follows:

1. **[`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md)** (Primary Source of Truth & Highest Technical Authority)
2. **[`AERS_ENGINEERING_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_ENGINEERING_RULEBOOK.md)** (Mandatory Engineering Constitution & Operational Governance)
3. **Source Code** (Authoritative Executable Ground Truth on branch `develop`)
4. **Test Suite** (Empirical Verification Baseline & Acceptance Contracts)
5. **Remaining Project Documentation** (Architecture documents, meeting notes, guides)

### Authority Invariants:
* **Master Specification Primacy:** The Master Software Specification (`AERS_MASTER_SOFTWARE_SPECIFICATION.md`) is the single highest technical authority for system design, interface contracts, and module responsibilities.
* **Engineering Governance:** The Engineering Rulebook (`AERS_ENGINEERING_RULEBOOK.md`) governs software engineering practices, coding standards, quality metrics, and development workflows.
* **Specification Conflicts:** If any secondary documentation conflicts with the Master Software Specification, the Master Software Specification takes precedence.
* **Source Code Ground Truth:** If documentation conflicts with the active source code on `develop`, the source code is authoritative, and the documentation must be updated accordingly.
* **Pre-Merge Architecture Updates:** If an architectural change is intentionally introduced, the Master Software Specification must be updated and approved before merging the code.

---

# 3 Engineering Philosophy

AERS controls physical municipal infrastructure where software failures directly jeopardize human lives. The following principles govern every line of code, configuration, and architectural design:

* **RULE-PHIL-001 (Safety Supremacy):** Physical clearance, interlocks, and human safety strictly supersede throughput, transit velocity, or system performance. No deadline or optimization may compromise physical safety boundaries.
* **RULE-PHIL-002 (Deterministic Execution):** State transitions, timing clearance intervals, and arbitration decisions must be mathematically reproducible and bounded in time. Nondeterminism is treated as a critical defect.
* **RULE-PHIL-003 (Fail-Safe by Design):** Every system failure—whether communication timeout, node crash, or watchdog expiration—must transition edge signals into a predefined, safe degradation state (isolated flashing amber or fixed-time local cycling).
* **RULE-PHIL-004 (Simplicity and Transparency):** Prefer explicit, auditable, standard C++17 implementations over complex metaprogramming or esoteric language tricks.
* **RULE-PHIL-005 (Continuous Triad Synchronization):** Implementation code, automated tests, and engineering documentation form an inseparable triad. No feature is complete until all three are synchronized and validated.

---

# 4 Working Feature Protection

Stability, reliability, and regression prevention are paramount in safety-critical cyber-physical systems.

Working functionality shall not be removed, rewritten, or replaced unless:
* **Documented Rationale:** There is a documented and validated engineering reason justifying the modification.
* **Superior Implementation:** A demonstrably superior implementation exists that satisfies all safety, timing, and real-time constraints.
* **Interface & Feature Preservation:** Existing operational capabilities, API contracts, and user-facing features are fully preserved.
* **Continuous Test Verification:** All existing automated test suites continue to pass without regression.
* **Documentation Synchronization:** Relevant specifications, rulebooks, and manuals are updated concurrently.

### Incremental Evolution Directive:
* Unnecessary rewrites, speculative refactorings, and gratuitous code churn are strictly discouraged.
* Engineering efforts must prioritize incremental, modular improvements that protect established, working system invariants.

---

# 5 Repository Rules

The repository layout is organized strictly by architectural responsibility:

```
AERS/
├── CMakeLists.txt                 # Top-level CMake orchestration
├── Makefile                       # Universal Native Makefile
├── AERS_MASTER_SOFTWARE_SPECIFICATION.md # Primary engineering specification
├── AERS_ENGINEERING_RULEBOOK.md   # This constitutional rulebook
├── AI_RULEBOOK.md                 # Legacy AI standard
├── CHANGELOG.md                   # Chronological development log
├── DEPLOYMENT_GUIDE.md            # Hardware setup manual
├── DEVELOPMENT_PLAN.md            # 12-phase engineering roadmap
├── config/                        # System configurations and deployment profiles
│   ├── default.json               # Baseline configuration
│   └── deployment/                # Per-node Raspberry Pi JSON configurations
├── include/                       # Public API headers
│   └── aegis/                     # Subsystem interface headers
├── src/                           # Subsystem implementation source files
│   ├── ambulance/                 # Kinematics, tracking, and fleet registry
│   ├── apps/                      # Entrypoint main() functions for daemons
│   ├── common/                    # Math, time, geo, JSON, and thread-safe queues
│   ├── communication/             # POSIX TCP SocketServer, SocketClient, Message
│   ├── config/                    # NodeConfigLoader, configuration parsing
│   ├── control/                   # Manual operator overrides and emergency console
│   ├── coordinator/               # Central coordinator engine and green corridor
│   ├── intersection/              # Edge IntersectionNode and SignalController
│   ├── logger/                    # Logger, EventLogger, SimulationRecorder
│   ├── medical/                   # PatientProfile, PriorityEngine, MedicalDatabase
│   ├── network/                   # NetworkManager, Serializer, MQTT, Transports
│   └── scheduler/                 # Multi-ambulance conflict detection and arbitration
├── scripts/                       # Build, run, and deployment automation scripts
├── tests/                         # Unit, integration, hardware, and simulation tests
└── hardware/                      # Pinouts, schematics, and wiring specifications
```

* **RULE-REPO-001 (Clean Tree Integrity):** Ephemeral build artifacts (`build/`, `*.o`, `*.a`, binaries, `.dSYM`) must reside in the build directory and never be tracked in git.
* **RULE-REPO-002 (Header-Source Segregation):** Public interface declarations must reside in `include/` or designated module header files (`.h`/`.hpp`). Implementation logic must reside exclusively in source files (`.cpp`).
* **RULE-REPO-003 (No Stray Artifacts):** Temporary test logs, scratch files, or test outputs must never be committed to the repository.

---

# 6 Architecture Rules

AERS implements a decoupled, five-tier cyber-physical architecture:

```
┌────────────────────────────────────────────────────────┐
│ 1. Telemetry & Audit Layer (EventLogger, Dashboards)   │
├────────────────────────────────────────────────────────┤
│ 2. Scheduling & Intelligence (Intelligent Scheduler)   │
├────────────────────────────────────────────────────────┤
│ 3. Central Coordination (CoordinatorManager, Corridor) │
├────────────────────────────────────────────────────────┤
│ 4. Transport & Communication (POSIX Sockets, MQTT)     │
├────────────────────────────────────────────────────────┤
│ 5. Physical Edge Execution (IntersectionNode, Signal)  │
└────────────────────────────────────────────────────────┘
```

* **RULE-ARCH-001 (Immutability of Architecture):** The project name **AERS** and the core architecture modules must not be renamed, deleted, or merged into monolithic blobs.
* **RULE-ARCH-002 (Single Responsibility):** Each module must perform only its designated responsibilities. The edge signal controller must never perform routing; the central coordinator must never directly drive GPIO pins.
* **RULE-ARCH-003 (Dependency Inversion):** Edge nodes must never depend on supervisory dashboards or dispatch engines. Edge nodes must be capable of standalone execution if detached from the network.
* **RULE-ARCH-004 (Strict Mutual Exclusion):** Signal controllers and scheduler conflict managers must guarantee that conflicting intersection approaches **never** receive simultaneous green signals.

---

# 7 Hardware Constraints

* **RULE-HW-001 (Four Raspberry Pi Cluster Architecture):** The system hardware architecture is strictly composed of **FOUR (4) Raspberry Pi computers**:
  * **Raspberry Pi 1:** Central Coordinator (`192.168.1.101:9000`)
  * **Raspberry Pi 2:** Intersection Node A / `INT_A` (`192.168.1.102:9001`)
  * **Raspberry Pi 3:** Intersection Node B / `INT_B` (`192.168.1.103:9002`)
  * **Raspberry Pi 4:** Intersection Node C / `INT_C` (`192.168.1.104:9003`)
* **RULE-HW-002 (Prohibition of Microcontrollers):** There shall be **NO ESP32 devices**, **NO Arduino boards**, and **NO auxiliary microcontrollers** responsible for traffic signal logic or coordination. All traffic signal algorithms, state machines, and preemption interlocks execute natively on Raspberry Pi boards. Any historical mention of ESP32 or Arduino in older markdown documents is strictly legacy documentation.
* **RULE-HW-003 (Direct Edge GPIO Execution):** Raspberry Pi edge nodes interface directly with physical or simulated GPIO pins to control traffic signal lamps.
* **RULE-HW-004 (Electrical Isolation):** Physical deployment must utilize optocoupled relay modules to protect Raspberry Pi GPIO lines from high-voltage traffic heads.

---

# 8 Hardware Evolution Policy

The current approved physical hardware architecture consists strictly of:
* **Raspberry Pi 1** — Central Coordinator (`192.168.1.101:9000`)
* **Raspberry Pi 2** — Intersection A / `INT_A` (`192.168.1.102:9001`)
* **Raspberry Pi 3** — Intersection B / `INT_B` (`192.168.1.103:9002`)
* **Raspberry Pi 4** — Intersection C / `INT_C` (`192.168.1.104:9003`)

### Architectural Exclusions:
* **ESP32 is NOT part of the approved architecture.**
* **Arduino is NOT part of the approved architecture.**
* **Microcontroller-based traffic control is NOT part of the approved architecture.**

### Protocol for Future Hardware Evolution:
Any future hardware modification, peripheral integration, or compute platform change requires:
1. **Formal Architecture Review:** Written proposal detailing technical justification, power, thermal, latency, and real-time analysis.
2. **Documentation Updates:** Concurrent revisions to [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and electrical schematics.
3. **Project Owner Approval:** Explicit written approval by the Project Owner.
4. **Backward Compatibility Analysis:** Verification that existing POSIX networking, signal state machines, and configuration schemas remain fully operational.
5. **Empirical Benchmarking & Testing:** Physical hardware-in-the-loop validation prior to formal adoption.

---

# 9 Software Constraints

* **RULE-SW-001 (Language Standard):** All code must compile cleanly under **ISO/IEC 14882:2017 (C++17)**.
* **RULE-SW-002 (Zero Third-Party Core Dependencies):** The core system must compile without requiring external third-party libraries for networking, JSON parsing, or math. Standard POSIX APIs and standard C++17 libraries are authoritative.
* **RULE-SW-003 (Compiler Warning Zero Tolerance):** Compilers must enforce:
  ```text
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
  ```
  The entire codebase must compile with **zero warnings**.
* **RULE-SW-004 (Cross-Platform POSIX Compliance):** All networking and system logic must run identically on Raspberry Pi OS 64-bit (Debian Linux), standard Linux x86_64, macOS (Darwin), and QNX Neutrino RTOS 7.1.

---

# 10 Module Design Rules

* **RULE-MOD-001 (Interface-Driven Abstraction):** Hardware and communication dependencies must be abstracted behind pure interfaces (e.g., `IGpioInterface`, `IMqttTransport`).
* **RULE-MOD-002 (Encapsulation and Data Invariance):** Class members must be private or protected. State mutations must occur through explicit, validated methods.
* **RULE-MOD-003 (Dependency Injection):** Clocks, transports, and hardware interfaces must be injectable to enable hermetic unit testing without physical hardware.
* **RULE-MOD-004 (Reusability & DRY):** Never duplicate classes or algorithms across modules. Common utilities belong in `src/common/`.

---

# 11 Communication Rules

* **RULE-COMM-001 (Dual Transport Stratification):**
  * Use **POSIX Stream Sockets (TCP)** for deterministic, low-latency coordinator-intersection cluster communication.
  * Use **MQTT (over TLS)** for wide-area telematics, vehicle tracking, and dashboard backends.
* **RULE-COMM-002 (Framing Protocol):** Socket communication must use newline (`\n`) delimited JSON messages adhering to the [`Message`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/communication/Message.h) envelope specification.
* **RULE-COMM-003 (Handshake & Keepalive Invariant):**
  1. Edge connects over TCP to Coordinator port 9000.
  2. Edge transmits `REGISTRATION` packet.
  3. Coordinator registers node and replies with `REGISTRATION_ACK`.
  4. Edge commences 1 Hz `HEARTBEAT` emissions.
* **RULE-COMM-004 (Timeout Bounding):** Connection timeout is bounded at 3000ms. If three (3) consecutive heartbeats are missed ($> 3000$ms), the coordinator marks the node offline, and the edge node enters fail-safe mode.
* **RULE-COMM-005 (No Hardcoded IP Addresses):** Network coordinates must be loaded from JSON configuration profiles via [`NodeConfig`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/config/NodeConfig.h).

---

# 12 Build System Rules

* **RULE-BUILD-001 (Dual Build Equivalence):** Both the top-level [`CMakeLists.txt`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CMakeLists.txt) and root [`Makefile`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/Makefile) must remain fully maintained and synchronized.
* **RULE-BUILD-002 (Path-Neutral Execution):** All test binaries and daemons must support execution from either the repository root or the `build/` directory without failing to locate configuration files.
* **RULE-BUILD-003 (Clean Targets):** `make clean` or removing `build/` must restore the repository to an uncompiled state without removing source or configuration files.

---

# 13 Testing Requirements

* **RULE-TEST-001 (100% Pass Policy):** All **18 test suites** must compile and pass cleanly before any code is merged or released.
* **RULE-TEST-002 (Mandatory Test Accompaniment):** No feature, bugfix, or refactoring is accepted without corresponding automated tests.
* **RULE-TEST-003 (Hermetic Independence):** Unit tests must be completely self-contained and execute without requiring external network brokers or physical GPIO.
* **RULE-TEST-004 (Deterministic Verification):** Tests must assert explicit state transitions and avoid nondeterministic timing assertions.

---

# 14 Phase Completion Criteria

A development phase is considered complete only when ALL of the following criteria are satisfied:

* [x] **Feature Implementation Completed:** All planned functional components, data models, and logic are fully implemented without placeholders, mocks, or TODOs.
* [x] **Code Builds Successfully:** The codebase compiles cleanly under both CMake and the native Makefile with zero compiler warnings under `-Wall -Wextra -Wpedantic`.
* [x] **Unit Tests Pass:** All unit test suites pass with 100% verification.
* [x] **Integration Tests Pass:** Multi-node and multi-module integration tests pass (where applicable).
* [x] **Documentation Updated:** [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and related technical specifications reflect all additions.
* [x] **Changelog Updated:** [`CHANGELOG.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CHANGELOG.md) records all user-facing additions, modifications, fixes, and architectural notes.
* [x] **Raspberry Pi Deployment Verified:** Deployment configurations, startup scripts, and network mappings are verified on the Raspberry Pi cluster (if applicable).
* [x] **Repository Passes CI/Build Validation:** Clean build and test execution pass across all target environments (Linux and macOS).
* [x] **No Critical Defects Remain:** Zero unresolved critical bugs, safety violations, memory leaks, or concurrency deadlocks.

---

# 15 Documentation Requirements

* **RULE-DOC-001 (Continuous Triad Synchronization):** Whenever code or tests change, documentation must be updated in lockstep.
* **RULE-DOC-002 (Primary Specification Authority):** [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) is the primary single source of truth and must reflect the exact current state of the codebase.
* **RULE-DOC-003 (Doxygen Standards):** All public classes, structs, methods, parameters, and exceptions must be documented using Doxygen comments (`/** ... */`).
* **RULE-DOC-004 (Changelog Traceability):** Versioned releases and architectural updates must be documented in [`CHANGELOG.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CHANGELOG.md) under standard categories (Added, Changed, Fixed, Removed).

---

# 16 Coding Standards

* **RULE-CODE-001 (Naming Conventions):**
  * Namespaces: `lowercase` (`aegis::coordinator`, `aegis::scheduler`)
  * Classes / Structs: `UpperCamelCase` (`IntelligentPriorityScheduler`, `PatientProfile`)
  * Methods / Functions: `UpperCamelCase` (`CalculateScore()`, `RegisterNode()`)
  * Variables / Members: `lowerCamelCase` (`heartbeatIntervalMs`, `nodeId`)
  * Constants / Enums: `UPPER_SNAKE_CASE` (`SignalState::ALL_RED`, `EmergencyType::STROKE`)
* **RULE-CODE-002 (RAII & Memory Safety):** Use `std::unique_ptr` and `std::shared_ptr`. Raw owning pointers, direct calls to `malloc()`, `free()`, raw `new`, or raw `delete` are strictly forbidden.
* **RULE-CODE-003 (Const Correctness):** Qualify every non-mutated variable, parameter, and method as `const` or `constexpr`.
* **RULE-CODE-004 (Prohibition of Global Mutable State):** Global mutable variables are banned. Shared instances must be passed via dependency injection or managed via thread-safe singletons.

---

# 17 Git Workflow

* **RULE-GIT-001 (Working Branch):** Active development occurs on the `develop` branch.
* **RULE-GIT-002 (Atomic Commits):** Commits must represent a single cohesive change. Mixing unrelated refactorings or features is prohibited.
* **RULE-GIT-003 (Conventional Commits):** Commit messages must follow the Conventional Commits specification:
  * `feat(<scope>): <description>`
  * `fix(<scope>): <description>`
  * `docs(<scope>): <description>`
  * `test(<scope>): <description>`
  * `refactor(<scope>): <description>`

---

# 18 Branching Strategy

```
  main      ───●────────────────────────────● (v0.8.0 Release Tag)
                ▲                            ▲
                 \                          /
  release/*       \──────●─────────────────/ (Stabilization & Docs)
                         ▲
                          \
  develop   ──●──────●─────●─────●────────── (Active Integration)
              ▲      ▲           ▲
             /        \         /
  feature/* ─●         ●───────●   (Isolated Features)
```

* **RULE-BRANCH-001 (Protected Main):** Direct commits to `main` are strictly prohibited. Code enters `main` only through validated release merges.
* **RULE-BRANCH-002 (Feature Branch Lifecycle):** Features branch from `develop` as `feature/<name>` and merge back into `develop` once tests pass 100%.

---

# 19 Release Process

To create a release (such as `v0.8.0`):
1. **Verification:** Execute `make clean && make -j4 all && make test` to ensure zero compilation warnings and 100% test passes.
2. **Documentation Audit:** Update [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and [`CHANGELOG.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CHANGELOG.md).
3. **Merge to Main:** Merge verified `develop` into `main`.
4. **Annotated Tag:** Create an annotated git tag:
   ```bash
   git tag -a vX.Y.Z -m "Release description"
   git push origin vX.Y.Z
   ```

---

# 20 Versioning Policy

AERS adheres strictly to **Semantic Versioning 2.0.0 (`MAJOR.MINOR.PATCH`)**:
* **MAJOR:** Incompatible architectural overhauls, protocol schema revisions, or hardware topology changes.
* **MINOR:** Backward-compatible new capabilities (e.g., Phase 8 Intelligent Priority Scheduling and Multi-Pi Deployment in `v0.8.0`).
* **PATCH:** Backward-compatible bugfixes, test repairs, or performance tuning.

---

# 21 Raspberry Pi Deployment Rules

* **RULE-DEPLOY-001 (Target Identification & Network Mappings):**

| Board | Role | Node ID | Static IP | Port |
|---|---|---|---|---|
| **Raspberry Pi 1** | Central Coordinator | `COORDINATOR` | `192.168.1.101` | `9000` |
| **Raspberry Pi 2** | Intersection A | `INT_A` | `192.168.1.102` | `9001` |
| **Raspberry Pi 3** | Intersection B | `INT_B` | `192.168.1.103` | `9002` |
| **Raspberry Pi 4** | Intersection C | `INT_C` | `192.168.1.104` | `9003` |

* **RULE-DEPLOY-002 (Startup Sequencing):** The Central Coordinator (RPi 1) must be launched first. Edge Intersection nodes (RPi 2–4) launch second and establish automatic TCP sessions.
* **RULE-DEPLOY-003 (Deployment Scripts):** Cluster deployment must be orchestrated using [`scripts/start_all_nodes.sh`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/scripts/start_all_nodes.sh) or individual node scripts ([`scripts/start_rpi1_coordinator.sh`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/scripts/start_rpi1_coordinator.sh), etc.).

---

# 22 Logging Standards

* **RULE-LOG-001 (Stratified Logging):**
  * System operational logs are managed by [`Logger`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/include/aegis/logger/logger.hpp) across 6 levels: `TRACE`, `LOG_DEBUG`, `INFO`, `WARN`, `ERR`, `CRITICAL`.
  * Forensic audit trail is recorded by [`EventLogger`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/logger/EventLogger.h) across 13 significant tactical events.
* **RULE-LOG-002 (Timestamp Mandate):** Every log entry must include high-resolution microsecond ISO 8601 timestamps.
* **RULE-LOG-003 (JSON Exportability):** Tactical events must support JSON serialization for ingestion by external operations centers.

---

# 23 Configuration Management

* **RULE-CFG-001 (Zero Hardcoding):** Network addresses, port numbers, timeout intervals, and hardware parameters must be loaded via JSON configuration files.
* **RULE-CFG-002 (Schema Validation):** All loaded configurations must be validated via `NodeConfig::IsValid()` before socket binding or transmission occurs.
* **RULE-CFG-003 (Profile Separation):** Maintain distinct configuration profiles for development ([`config/development.json`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/config/development.json)), production ([`config/production.json`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/config/production.json)), and hardware deployment ([`config/deployment/`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/config/deployment/)).

---

# 24 Error Handling Standards

* **RULE-ERR-001 (Explicit Status & Results):** Functions must return explicit boolean status flags, typed error codes, or `Result<T, ErrorCode>` structures.
* **RULE-ERR-002 (Zero Silent Swallowing):** Never catch an error or exception and discard it silently. All anomalies must be logged at `WARN`, `ERR`, or `CRITICAL`.
* **RULE-ERR-003 (Graceful Edge Degradation):** Malformed incoming network packets must be rejected safely with an audit log; they must never crash or terminate edge signal daemons.

---

# 25 Thread Safety Guidelines

* **RULE-THRD-001 (Guarded Shared State):** Any shared data accessed concurrently across multiple threads must be protected by `std::mutex` or modeled using lock-free atomics (`std::atomic`).
* **RULE-THRD-002 (No Busy-Wait Polling):** Threads must block on condition variables (`std::condition_variable`) or blocking queues (`ThreadSafeQueue`). CPU-spinning loops are strictly banned.
* **RULE-THRD-003 (Deadlock Prevention):** When acquiring multiple locks simultaneously, always use `std::scoped_lock` to guarantee deadlock-free lock acquisition.

---

# 26 Performance Guidelines

* **RULE-PERF-001 (Zero Dynamic Allocations in Real-Time Loops):** Time-critical edge loops (signal state machine cycles, GPS kinematics filtering) must use pre-allocated buffers or stack allocations. Calls to `new` or `malloc()` are banned in time-critical loops.
* **RULE-PERF-002 (Preemption Latency Bound):** Emergency preemption command processing and physical state transition initiation must execute within $\le 50$ms.
* **RULE-PERF-003 (Embedded Memory Footprint):** Ensure daemons operate efficiently within constrained embedded environments ($\ge 2$GB RAM per Raspberry Pi).

---

# 27 Security Guidelines

* **RULE-SEC-001 (Zero-Trust Input Sanitization):** All network packets must undergo length, schema, and type validation before being parsed.
* **RULE-SEC-002 (Replay Attack Defense):** Commands must contain monotonic sequence counters and millisecond timestamps; packets older than 5000ms must be rejected.
* **RULE-SEC-003 (Cryptographic Operator Overrides):** Manual control center overrides require authenticated operator tokens and are permanently recorded in the forensic audit log.
* **RULE-SEC-004 (Network Isolation):** The four-Pi cluster must be deployed on an isolated, secure VLAN or dedicated physical LAN.

---

# 28 Definition of Done

An individual feature, pull request, or engineering task is considered complete ("Done") only when ALL of the following conditions are verified:

* [x] **Implementation Complete:** Fully functional, standards-compliant C++17 implementation adhering to single-responsibility module boundaries.
* [x] **Compiles Successfully:** Compiles with zero compiler warnings under strict flags (`-Wall -Wextra -Wpedantic`) across all supported toolchains.
* [x] **All Tests Pass:** 100% pass rate across all 18 test suites, including new unit and regression tests.
* [x] **Documentation Updated:** [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and relevant design documents updated concurrently.
* [x] **Changelog Updated:** Traceable entry logged in [`CHANGELOG.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/CHANGELOG.md) under Conventional Commits categories.
* [x] **Code Reviewed:** Code reviewed against this Engineering Rulebook for architectural integrity, thread safety, and memory management.
* [x] **Ready for Release:** Clean working tree, tagged if applicable, and verified deployable to Raspberry Pi hardware.
* [x] **No Unresolved Critical Issues:** Zero open safety invariant violations, unhandled error paths, memory leaks, or undefined behaviors.

---

# 29 AI Assistant Guidelines

Every AI assistant contributing to AERS must comply with this binding contract:

1. **Mandatory Specification & Rulebook Review:** Before generating or altering code, the AI must consult [`AERS_MASTER_SOFTWARE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AERS_MASTER_SOFTWARE_SPECIFICATION.md) and this rulebook.
2. **Topology Invariance:** Never propose, introduce, or write code for ESP32 or Arduino devices. The system is strictly a 4-Raspberry Pi cluster.
3. **Triad Enforcement:** When generating a feature, the AI must provide the implementation, corresponding automated tests, and documentation updates.
4. **Regression Prevention:** The AI must ensure that all 18 test suites compile and pass 100% after every change.
5. **Architectural Stewardship:** Treat the codebase as a safety-critical system built for long-term production maintenance.

---

# 30 Future Development Policy

* **RULE-FUT-001 (Gated Phasing):** Features must progress according to the 12-phase development roadmap. Unscheduled features must not be introduced arbitrarily.
* **RULE-FUT-002 (Phase Roadmap Placeholders):**
  * **Phase 9:** Physical Raspberry Pi GPIO Relay Interfacing & NMEA-0183 Serial GPS Receiver Drivers.
  * **Phase 10:** Cellular C-V2X (PC5 Sidelink) Direct Ad-Hoc Interoperability & Cloud Replication.
  * **Phase 11:** AI Predictive Traffic Congestion Forecasting & Multi-City Grid Scaling.
* **RULE-FUT-003 (No Speculative Implementations):** Unimplemented phases remain strictly placeholders until formally scheduled and scoped.
