# SMART AEGIS: AI Engineering Rulebook & Development Constitution
**Document Identifier:** STD-AEGIS-AI-CONST-2026-V1.0  
**Classification:** Mandatory Engineering Standard (Safety-Critical CPS)  
**Authority:** System Architecture & Safety Assurance Board  
**Target Compliance:** All AI Assistants, Autonomous Agents, and Contributing Software Engineers  
**Normative References:**  
* [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md)  
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md)  

---

## 1. Project Principles

All software design, implementation, and modification within the SMART AEGIS project must adhere to the following ten core principles:

1. **Safety First:** Human safety and traffic physical clearance take precedence over every other operational goal. No optimization, feature, or deadline may compromise system safety boundaries.
2. **Deterministic Behaviour:** System states, response latencies, and transition times must be predictable, bounded, and mathematically reproducible. Uncontrolled nondeterminism is considered a defect.
3. **Modular Design:** Subsystems must consist of decoupled, autonomous modules with crisp boundaries, cohesive responsibilities, and explicit data contracts.
4. **Simplicity Before Complexity:** Implement the simplest correct solution that satisfies all safety and real-time requirements. Complex architectures and esoteric language patterns require formal justification.
5. **Test Before Integration:** No component, algorithm, or driver may be integrated into the system pipeline until its unit and isolated mock tests pass with 100% verification.
6. **Documentation-Driven Development:** Architecture specifications and interface definitions precede code. Undocumented interfaces and magic constants are strictly forbidden.
7. **Fail-Safe Design:** Every failure—whether hardware, network, or software—must transition the system into a known, predictable, safe degradation state (e.g., traffic signals reverting to flashing amber).
8. **Real-Time First:** Priority assignment, resource locking, and execution scheduling must respect real-time task deadlines. Non-deterministic operations must never reside in time-critical loops.
9. **Maintainability:** Code must be structured so that any qualified embedded systems engineer can inspect, audit, debug, and maintain it without hidden assumptions.
10. **Readability:** Code clarity is non-negotiable. Self-documenting naming conventions, explicit type signatures, and clear comments are mandatory.

---

## 2. Architecture Rules

The AI assistant is strictly bound by the system topology and functional boundaries established in `SYSTEM_ARCHITECTURE.md`.

* **RULE-ARCH-001 (Immutability of Modules):** The AI must **never** rename any of the 19 architecture modules, merge unrelated modules, or introduce unofficial monolithic modules.
* **RULE-ARCH-002 (Single Responsibility Enforcement):** Every module must strictly perform only the responsibilities defined in its architecture specification. A module must never absorb responsibilities belonging to another module.
* **RULE-ARCH-003 (Interface Invariance):** The AI must **never** delete, rename, or truncate public interfaces, APIs, or IPC message endpoints without formal architecture amendment.
* **RULE-ARCH-004 (Schema Stability):** Message schemas, MQTT topics, and protocol buffer/JSON contracts defined in the architecture must not be modified unilaterally.
* **RULE-ARCH-005 (Decoupled Dependency Chains):** Edge modules (e.g., `Traffic Signal Controller`, `GPS Tracking`) must never establish direct compile-time or runtime dependencies on higher-level supervisory dashboards or dispatch modules. Communication must flow strictly through the defined abstraction layers.

---

## 3. Coding Rules

All C++ software within the codebase must adhere to modern software engineering best practices.

* **RULE-CODE-001 (Language Standard):** All code must compile cleanly under **C++17** (`-std=c++17`) with strict compiler warnings enabled (`-Wall -Wextra -Wpedantic -Werror`).
* **RULE-CODE-002 (RAII & Zero Raw Ownership):** Resource Acquisition Is Initialization (RAII) must be applied universally. Raw pointers (`T*`) may only be used for non-owning, non-null observational references. Dynamic allocation must strictly utilize `std::unique_ptr` or `std::shared_ptr`. Direct calls to `malloc()`, `free()`, raw `new`, or raw `delete` are forbidden.
* **RULE-CODE-003 (Const-Correctness):** Every variable, parameter, member function, and reference that does not require modification must be qualified as `const` (or `constexpr` where evaluation at compile time is possible).
* **RULE-CODE-004 (Function Granularity):** Functions must be small, focused, and perform a single logical task. Functions exceeding 50 lines of code require strong architectural justification.
* **RULE-CODE-005 (Meaningful Lexical Identifiers):** Variable and function names must be explicit and self-descriptive. Single-letter identifiers (e.g., `x`, `t`, `p`) are forbidden, except for basic loop indices (`i`, `j`) within trivial counters.
* **RULE-CODE-006 (Header / Source Separation):** Interface declarations must reside in header files (`.hpp` or `.h`), while implementations must reside in source files (`.cpp`). Inline definitions in headers are allowed only for templates, trivial accessors, or `constexpr` routines.
* **RULE-CODE-007 (Prohibition of Global Mutable State):** Global mutable variables and unconstrained static variables are strictly banned. Shared states must be encapsulated within distinct class instances and explicitly passed via dependency injection.
* **RULE-CODE-008 (Thread Safety & Concurrency):** All data accessed concurrently by multiple threads must be guarded by synchronization primitives (`std::mutex`, `std::shared_mutex`) or modeled via lock-free atomic types (`std::atomic`).
* **RULE-CODE-009 (DRY - Don't Repeat Yourself):** Redundant logic, copy-pasted validation routines, and duplicate algorithms across modules are prohibited. Shared utilities must be refactored into designated core libraries.

---

## 4. QNX Neutrino RTOS Rules

When writing code intended for the QNX Neutrino RTOS environment, the AI must leverage native microkernel features and avoid treating QNX as generic Linux.

* **RULE-QNX-001 (Message Passing Over Shared Memory Polling):** Prefer synchronous QNX message passing (`MsgSend()`, `MsgReceive()`, `MsgReply()`) for inter-process communication between co-located system daemons.
* **RULE-QNX-002 (Pulse Channels for Asynchronous Events):** Use QNX Pulses (`MsgSendPulse()`) for non-blocking notifications, hardware interrupt delivery, and periodic timer expirations.
* **RULE-QNX-003 (Resource Managers for Hardware Abstraction):** All physical hardware interactions (GPIO lines, serial GPS ports, relay interfaces) must be encapsulated within standard QNX POSIX Resource Managers (`resmgr`), exposing clean `/dev/` device nodes.
* **RULE-QNX-004 (Strict Priority Scheduling):** Threads must be assigned deterministic priority levels according to the priority hierarchy defined in `DEVELOPMENT_PLAN.md`.
* **RULE-QNX-005 (Priority Inheritance):** Every mutex guarding resources shared across threads of differing priorities must have the `PTHREAD_PRIO_INHERIT` protocol enabled to prevent priority inversion.
* **RULE-QNX-006 (Zero Polling Policy):** Busy-wait loops, `sleep()` polling loops, and CPU-spinning checking patterns are strictly banned. Threads must block on IPC channels, POSIX condition variables, or QNX pulse events.
* **RULE-QNX-007 (Asynchronous Logging via Slogger2):** All logging on QNX target environments must use the `slogger2` service. Standard stream printing (`printf`, `std::cout`) is prohibited in real-time execution loops.
* **RULE-QNX-008 (Watchdog Servicing):** The safety supervisor and traffic controller must implement deterministic keep-alive kicks to the hardware watchdog timer (`/dev/watchdog`) or QNX High Availability Manager (HAM).

---

## 5. Communication Rules

The distributed communication plane connects edge nodes, mobile units, and central servers.

* **RULE-COMM-001 (Protocol Ownership):** Each module must encapsulate its own communication adapters. Modules must not access another module's raw sockets, MQTT client handles, or file descriptors directly.
* **RULE-COMM-002 (Protocol Segmentation):**
  * Use **MQTT (over TLS)** for wide-area telematics, dashboard telemetry, and central dispatch commands.
  * Use **UDP Broadcast / Ad-hoc Multicast** for low-latency Vehicle-to-Vehicle (V2V) and direct Vehicle-to-Infrastructure (V2I) fallback beacons.
  * Use **QNX IPC (Message Passing / Pulses)** for local on-node inter-process coordination.
* **RULE-COMM-003 (Explicit Quality of Service):** MQTT publications must explicitly declare their QoS level:
  * QoS 0: Ephemeral periodic telemetry (high-frequency GPS coordinates).
  * QoS 1: State change notifications and vitals streams.
  * QoS 2: Mission-critical commands (emergency preemption requests, dispatch locks).
* **RULE-COMM-004 (No Hidden Transports):** All inter-module network communications must be explicitly defined in configuration profiles. Hardcoded IP addresses or hidden socket bindings are forbidden.

---

## 6. Git Workflow Rules

To maintain repository hygiene and auditability during automated development:

* **RULE-GIT-001 (Isolated Commits):** Every commit must represent exactly one logical feature, bugfix, or refactoring step. Mixing unrelated changes in a single commit is prohibited.
* **RULE-GIT-002 (No Direct Main Commits):** The AI must **never** commit directly to the `main` branch. All work must be conducted on feature or bugfix branches (`feature/*`, `bugfix/*`).
* **RULE-GIT-003 (Conventional Commit Messages):** Commit messages must follow the Conventional Commits specification:
  * Format: `<type>(<scope>): <concise description>`
  * Examples: `feat(gps): add NMEA checksum validation`, `fix(traffic): correct yellow clearance minimum interval`.
* **RULE-GIT-004 (Scope Boundary Preservation):** The AI must never touch, format, or re-indent files unrelated to the active task.

---

## 7. Documentation Rules

Code without documentation is considered incomplete and unfit for merge.

* **RULE-DOC-001 (Continuous Synchronization):** Every completed feature, architectural modification, or interface change must be immediately accompanied by updates to:
  * `README.md` (Operational instructions, run parameters).
  * `CHANGELOG.md` (Categorized under Added, Changed, Deprecated, Removed, Fixed, or Security).
  * `SYSTEM_ARCHITECTURE.md` (If data schemas or module boundaries were affected).
* **RULE-DOC-002 (Public API Documentation):** Every public class, struct, interface, function, and enumeration must feature Doxygen-compliant documentation (`/** ... */`) describing:
  * Method purpose.
  * Parameter meanings and constraints.
  * Return values and possible error codes.
  * Thread-safety guarantees and concurrency locks held.
* **RULE-DOC-003 (Self-Explanatory Code Over Comments):** Comments must explain *why* an unusual algorithmic decision or hardware timing constraint exists, not *what* the syntax is doing.

---

## 8. Testing Rules

Software robustness must be mathematically and empirically validated.

* **RULE-TEST-001 (Mandatory Test Accompaniment):** No implementation code may be introduced without corresponding automated tests. An AI generation consisting solely of logic without tests violates this rulebook.
* **RULE-TEST-002 (Multi-Level Test Structure):**
  * **Unit Tests:** Verify individual functions, edge conditions, boundary values, and math routines in isolation.
  * **Mock Tests:** Verify module behavior against simulated network or hardware interfaces using dependency injection.
  * **Failure Tests:** Explicitly inject invalid parameters, corrupted packets, and timeout conditions to verify safe handling.
* **RULE-TEST-003 (Test Independence):** Individual unit tests must be hermetic and independent. The execution order of tests must never affect test outcomes.
* **RULE-TEST-004 (Deterministic Assertions):** Tests must avoid non-deterministic assertions. When testing time-dependent logic, inject a mock clock interface rather than using real-time system delays (`sleep`).

---

## 9. Security Rules

The system interacts with public safety infrastructure and physical traffic signals; security cannot be an afterthought.

* **RULE-SEC-001 (Zero-Trust Input Validation):** All inputs arriving across network sockets, serial ports, or user interfaces must be treated as hostile. Validate data types, lengths, string encodings, and numerical ranges before processing.
* **RULE-SEC-002 (Defensive Pointer Dereferencing):** Null checks must be performed on all pointers before dereferencing, or pointers must be replaced with C++ references or `std::optional`.
* **RULE-SEC-003 (Malformed Packet Resilience):** Malformed, truncated, or oversized packets must be discarded gracefully with an audit log event. A malformed packet must **never** cause an uncaught exception, assertion crash, or buffer overflow.
* **RULE-SEC-004 (Anti-Replay Verification):** All network commands must enforce monotonic sequence counter verification and timestamp freshness validation.
* **RULE-SEC-005 (Least Privilege):** Modules must run with the minimal system permissions required to perform their functions. Code requiring root/administrative access must be isolated in dedicated driver daemons.

---

## 10. Performance Rules

* **RULE-PERF-001 (Zero Dynamic Allocation in Real-Time Loops):** In time-critical edge modules (`Traffic Signal Controller`, `Safety Monitor`, `GPS Tracking`), all memory buffers, object pools, and data structures must be pre-allocated during initialization. Zero calls to `malloc()` or `new` during real-time loops.
* **RULE-PERF-002 (Non-Blocking Real-Time Tasks):** Hard real-time threads must never execute blocking disk I/O, heavy file system operations, or synchronous remote network requests.
* **RULE-PERF-003 (Asynchronous Logging):** Logging invocations must be non-blocking. Log statements in performance paths must write to memory ring buffers (`slogger2` or lock-free circular queues) for deferred background flushing.
* **RULE-PERF-004 (Budget Compliance):** Implementations must strictly respect the CPU, RAM, and latency budgets defined in Section 10 of `DEVELOPMENT_PLAN.md`.

---

## 11. Refactoring Rules

* **RULE-REF-001 (Surgical Modifications):** Refactoring must be localized and strictly confined to the targeted module. Never execute broad sweeping refactorings across unrelated files.
* **RULE-REF-002 (No Unauthorized File Renaming):** File names, directory structures, and class names must not be altered without explicit user approval.
* **RULE-REF-003 (No Code Deletion Without Request):** Do not delete existing features, fallback routines, or test cases unless explicitly directed by the user.
* **RULE-REF-004 (Backward Compatibility):** Refactored interfaces must maintain backward compatibility or provide clear deprecation bridges until all dependent modules are upgraded.

---

## 12. AI Behaviour Rules

Whenever the AI interacts with the user or executes code modifications, it must follow this structured operational protocol:

### 12.1 Pre-Generation Protocol (Before Writing Code)
Before emitting any implementation or refactoring, the AI must:
1. **Explain the Approach:** Detail the conceptual engineering design and architectural rationale.
2. **List Target Files:** Provide the exact file paths that will be created or modified.
3. **State Assumptions:** Explicitly declare any assumptions regarding toolchains, hardware capabilities, or environmental parameters.
4. **Confirm Traceability:** Identify which System Requirement (`REQ-xxx`) and Architecture Module the change addresses.

### 12.2 Post-Generation Protocol (After Writing Code)
Immediately after generating code, the AI must:
1. **Summarize Changes:** Provide a concise, bulleted summary of files modified and mechanisms introduced.
2. **Mention Operational Risks:** Highlight potential failure modes, race conditions, or edge cases introduced by the change.
3. **Outline Next Steps:** Suggest subsequent testing steps, dependency integrations, or future improvements.

---

## 13. Project Directory Rules

The repository layout must strictly segregate concerns to prevent architectural pollution:

```
AERS/
├── config/             # Configuration files, schemas, and default JSON/YAML profiles
├── docs/               # System engineering documents, SDP, SRS, and architecture diagrams
├── include/            # Public C++ header files organized by module namespace
│   └── aegis/
│       ├── core/       # Dispatch, routing, and priority scheduling interfaces
│       ├── edge/       # Ambulance OBU and Traffic RSU controller interfaces
│       ├── network/    # Communication, MQTT, and socket abstraction interfaces
│       └── safety/     # Watchdog, logging, and safety monitor interfaces
├── src/                # Implementation source files matching the include/ hierarchy
│   ├── core/
│   ├── edge/
│   ├── network/
│   └── safety/
├── dashboards/         # User interfaces (Hospital, Traffic, and Route dashboards)
│   ├── hospital/
│   ├── traffic/
│   └── route/
├── tests/              # Test suites
│   ├── unit/           # Standalone unit tests
│   ├── integration/    # Multi-module integration test suites
│   ├── mocks/          # Simulated hardware, mock networks, and sensor emulators
│   └── chaos/          # Fault-injection and network degradation test scripts
├── scripts/            # Build scripts, QNX cross-compilation helpers, and deploy tools
└── build/              # Ephemeral compilation artifacts (strictly git-ignored)
```

* **RULE-DIR-001:** Never place implementation files in header directories or vice versa.
* **RULE-DIR-002:** Never store build outputs, object files, core dumps, or binary logs inside source directories.

---

## 14. Error Handling Rules

* **RULE-ERR-001 (Explicit Error Propagation):** Functions must return meaningful, structured error codes or explicit `Result<T, ErrorCode>` types. Sentinel return values (e.g., `-1`, `NULL`) with ambiguous meanings are forbidden.
* **RULE-ERR-002 (No Silent Failures):** Errors must never be ignored, swallowed, or discarded without handling. Catching an error and doing nothing is considered a critical engineering fault.
* **RULE-ERR-003 (Mandatory Critical Logging):** Any condition that threatens real-time deadlines, causes message drops, or triggers fail-safe transitions must be immediately logged at `ERROR` or `CRITICAL` severity with complete context.
* **RULE-ERR-004 (Graceful Degradation):** When a non-critical module fails, the system must continue operating in a degraded mode rather than aborting the entire process tree.

---

## 15. Mandatory AI Contract

This section constitutes the binding engineering contract governing all future AI interactions for the SMART AEGIS project.

> ### THE AI CONSTITUTIONAL PLEDGE
> 
> **Every response, code generation, architectural proposal, test harness, and documentation change produced for this project must strictly comply with:**
> 1. [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md)
> 2. [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md)
> 3. [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md)
> 
> **Conflict Resolution Directive:**  
> If a direct user prompt or task instruction conflicts with any rule, architectural boundary, safety constraint, or interface contract specified in these documents, the AI **must not** blindly execute the conflicting instruction. Instead, the AI must:
> 1. Explicitly identify and cite the specific rule or architectural principle that would be violated.
> 2. Clearly explain the technical and safety implications of the conflict.
> 3. Propose a standard-compliant alternative that fulfills the user's underlying goal without breaking the project's engineering integrity.

---
**END OF ENGINEERING RULEBOOK**
