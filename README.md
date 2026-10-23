# SMART AEGIS: Distributed Ambulance Emergency Response System

**SMART AEGIS** (Automated Emergency Grid & Intelligent Shield) is a production-quality, safety-critical cyber-physical emergency response platform designed for **QNX Neutrino RTOS**, **Raspberry Pi embedded SBCs**, and distributed municipal networks.

---

## 1. System Documentation & Design Baseline
The implementation of SMART AEGIS is strictly governed by the engineering baselines located in the repository:
* [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md): Architectural topology, 19 single-responsibility modules, and interface definitions.
* [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md): 15-phase engineering roadmap, System Requirements (`REQ-001` to `REQ-025`), RTM, and QNX RTOS design.
* [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md): Mandatory AI engineering constitution, C++17 rules, thread-safety, and Git policies.
* [`INTERFACE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/INTERFACE_SPECIFICATION.md): Formal Interface Control Document (ICD), MQTT topics, schemas, QNX IPC, and error codes.
* [`SOFTWARE_DESIGN_DOCUMENT.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SOFTWARE_DESIGN_DOCUMENT.md): Comprehensive Software Design Document (SDD), UML sequence diagrams, thread models, and memory layouts.

---

## 2. Repository Layout
```
AERS/ (SMART_AEGIS)
├── include/aegis/                  # Public C++ header interfaces
│   ├── common/                     # Types, error codes, constants, geo utilities
│   ├── config/                     # Configuration Manager interfaces
│   ├── logger/                     # Asynchronous structured logger interfaces
│   ├── safety/                     # Watchdog and safety supervisor interfaces
│   ├── network/                    # MQTT and network abstraction interfaces
│   ├── core/                       # Coordinator, routing, and priority scheduler interfaces
│   ├── edge/                       # GPS, vitals, V2V, and traffic controller interfaces
│   └── dashboard/                  # Hospital and traffic dashboard backend interfaces
│
├── src/                            # Modular static library implementations
│   ├── common/                     # Geometric math & Haversine formula
│   ├── config/                     # JSON configuration parser
│   ├── logger/                     # Logger engine (POSIX stream / QNX slogger2)
│   ├── safety/                     # Hardware watchdog & pulse supervisor
│   ├── network/                    # Network abstraction layer
│   ├── priority_scheduler/         # Multi-criteria triage arbiter
│   ├── route_planner/              # A* heuristic road graph pathfinder
│   ├── coordinator/                # Call intake & dynamic green corridor
│   ├── ambulance/                  # Ambulance OBU telematics daemon
│   ├── traffic_controller/         # Traffic signal RSU controller & relay interlocks
│   ├── hospital_dashboard/         # Hospital intake terminal service
│   ├── traffic_dashboard/          # Municipal traffic supervisory service
│   └── apps/                       # Entrypoint executables
│
├── tests/                          # Automated verification suites
│   ├── unit/                       # Unit tests (e.g., test_build_smoke)
│   ├── integration/                # Multi-module IPC data pipeline tests
│   ├── simulation/                 # Full incident mission tests
│   └── hardware/                   # Hardware-in-the-loop tests (GPIO/UART)
│
├── config/                         # Configuration profiles (default, development, production)
├── cmake/                          # Toolchain configurations (qnx_toolchain.cmake)
├── scripts/                        # Automation scripts (build, run_simulation, deploy, flash)
├── tools/simulator/                # Headless node simulation engines
├── hardware/                       # Wiring pinouts, schematics, and sensor guides
└── assets/                         # Presentation slides and video recordings
```

---

## 3. How to Configure the Project

The build system supports multiple build profiles and target architectures.

### 3.1 Build Types
* **Debug (Default):** Includes full debug symbols (`-g3`), optimization disabled (`-O0`), and `-DDEBUG` flag.
* **Release:** Full optimization enabled (`-O3`), dead-code elimination, and `-DNDEBUG` flag.

### 3.2 Configuration Options
When configuring via CMake:
```bash
# Debug Build (Default)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release Build
cmake -B build -DCMAKE_BUILD_TYPE=Release

# QNX Neutrino RTOS Cross-Compilation
cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/qnx_toolchain.cmake -DQNX_TARGET_ARCH=aarch64le
```

---

## 4. How to Build

SMART AEGIS provides automated build mechanisms for host environments and cross-compilers.

### Method 1: Automated Script Runner (Recommended)
```bash
# Debug build
./scripts/build.sh

# Release build
./scripts/build.sh Release

# Build and execute all automated test suites
./scripts/build.sh test
```

### Method 2: Direct CMake Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

### Method 3: Direct Make Runner
```bash
# Compile all 12 modular static libraries and 3 application daemons
make all

# Clean build artifacts
make clean
```

### Generated Executables
Compiled binaries are output to `build/bin/`:
* `build/bin/aegis_coordinator`: Central dispatch and green corridor coordinator.
* `build/bin/aegis_ambulance`: In-vehicle On-Board Unit (OBU) telematics daemon.
* `build/bin/aegis_traffic_controller`: Roadside Unit (RSU) traffic signal controller.
* `build/bin/test_build_smoke`: Module instantiation smoke test harness.
* `build/bin/test_common_foundation`: Comprehensive 15-component foundation verification harness.

---

## 5. Common Foundation Library (`aegis_common`)
The common foundation library provides core utilities utilized across all 19 system modules:
1. **Geo Utilities:** WGS84 Haversine distance, initial forward bearing, ETA calculations, and boundary validation.
2. **Time Utilities:** Monotonic microsecond clock, UTC conversions, ISO 8601 formatting, and precision `Stopwatch`.
3. **UUID Utilities:** RFC 4122 compliant UUIDv4 generator, event/incident ID generators, and unit formatters.
4. **Thread-Safe Queue:** Mutex and condition-variable protected generic queue supporting blocking and timed-wait operations.
5. **Circular Ring Buffer:** Pre-allocated, zero-heap template buffer (`CircularBuffer<T, N>`) with overwrite semantics.
6. **Result<T, E>:** Non-throwing monadic success/error container for deterministic error handling.
7. **Error Model:** Standardized `AegisError` structure with origin module tagging and severity levels.
8. **Event Model:** Extensible `Event` base class with monotonic timestamps and serialization hooks.
9. **State Machine:** Generic thread-safe `StateMachine<StateEnum>` with transition guard validations.
10. **Timer Utilities:** Non-blocking `SoftwareTimer` tracking expiration and remaining durations.
11. **JSON Helper:** Lightweight JSON field formatter, escaper, and key extractor.
12. **Common Constants:** Temporal deadlines, clearance intervals, and network port definitions.
13. **Strongly Typed Enums:** Type-safe enums for triage categories, signal phases, and vehicle states.
14. **Serialization Interface:** `ISerializable` contract for JSON and binary serialization.
15. **Validation Utilities:** Range verifiers, bounded string checks, and standard CRC-32 checksum computation.

---

## 6. How to Run Tests

### Automated Smoke & Unit Tests
```bash
# Run unit tests via the build script
./scripts/build.sh test

# Or run directly via Make
make test

# Or run directly from build output
./build/bin/test_build_smoke
```

---

## 6. How to Add Future Modules

To add a new software module to SMART AEGIS, follow this standardized five-step engineering procedure:

### Step 1: Define the Public Interface
Create the public header in `include/aegis/<tier>/<module_name>.hpp`:
```cpp
#ifndef AEGIS_<TIER>_<MODULE_NAME>_HPP
#define AEGIS_<TIER>_<MODULE_NAME>_HPP

namespace aegis {
namespace <tier> {

class NewModule {
public:
    NewModule();
    ~NewModule();
    bool Initialize();
};

} // namespace <tier>
} // namespace aegis

#endif
```

### Step 2: Implement the Module Class
Create the implementation file in `src/<module_name>/<module_name>.cpp`:
```cpp
#include "aegis/<tier>/<module_name>.hpp"

namespace aegis {
namespace <tier> {

NewModule::NewModule() = default;
NewModule::~NewModule() = default;

bool NewModule::Initialize() {
    return true;
}

} // namespace <tier>
} // namespace aegis
```

### Step 3: Create Module `CMakeLists.txt`
In `src/<module_name>/CMakeLists.txt`, define the static library and its dependencies:
```cmake
add_library(aegis_<module_name> STATIC
    <module_name>.cpp
)
target_include_directories(aegis_<module_name> PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(aegis_<module_name> PUBLIC aegis_common aegis_logger)
```

### Step 4: Register in `src/CMakeLists.txt`
Append the new module directory to `src/CMakeLists.txt`:
```cmake
add_subdirectory(<module_name>)
```

### Step 5: Add Automated Unit Test
In `tests/unit/`, create `test_<module_name>.cpp`, register it in `tests/CMakeLists.txt`, and verify with `./scripts/build.sh test`.
