# SMART AEGIS: Distributed Ambulance Emergency Response System

SMART AEGIS is a safety-critical, QNX-based distributed cyber-physical system designed to coordinate ambulances, traffic signals, hospitals, and municipal emergency dispatch in real time.

---

## 1. System Architecture Highlights
* **Deterministic Real-Time Edge:** QNX Neutrino RTOS / Raspberry Pi edge nodes for On-Board Units (OBU) and Roadside Traffic Signal Controllers (RSU).
* **Dynamic Green Corridor:** Adaptive traffic signal preemption with safety interlocks, minimum-yellow, and all-red clearance intervals.
* **Telemedicine & Live Vitals:** Real-time physiological telemetry streaming (ECG, SpO2, NIBP) to receiving hospital emergency wards.
* **Decentralized V2X Fallback:** Low-latency direct ad-hoc wireless beacons (UDP) ensuring corridor preemption functions even during complete cellular/WAN outages.
* **Formal Engineering Standards:** Designed in compliance with ISO 26262 principles, POSIX.1-2017, and IEEE 802.11p/C-V2X architecture.

---

## 2. Normative Documentation Reference
The engineering implementation is strictly governed by the following documents:
* [`docs/architecture/SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md) - System architecture and 19 module specifications.
* [`docs/design/DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md) - 15-phase engineering roadmap, SRS, RTM, and QNX RTOS design.
* [`docs/design/AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md) - Mandatory coding standards and AI constitution.
* [`docs/design/INTERFACE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/INTERFACE_SPECIFICATION.md) - Interface Control Document (ICD), MQTT topics, schemas, and error codes.

---

## 3. Directory Layout
```
SMART_AEGIS/
├── config/       # Default, development, and production configuration profiles
├── docs/         # Architecture, design specifications, and meeting notes
├── scripts/      # Build, execution, deployment, and QNX flashing utilities
├── tools/        # Sensor, GPS, traffic, and vehicle simulation suites
├── src/          # Implementation source tree (modules & libraries)
├── include/      # Public C++ header tree (namespace aegis)
├── tests/        # Unit, integration, hardware, and simulation test harnesses
├── assets/       # Media, demonstration videos, and presentation slide deck
└── hardware/     # Raspberry Pi schematics, wiring pinouts, and sensor guides
```

---

## 4. Quick Start: Build & Execution

### Prerequisites
* CMake >= 3.20
* C++17 compliant compiler (GCC 11+, Clang 13+, or QNX SDP 7.1/8.0 `qcc`)
* Python 3.9+ (for simulation tools)
* Mosquitto MQTT Broker

### Build
```bash
./scripts/build.sh
```

### Run Multi-Node Simulation
```bash
./scripts/run_simulation.sh
```
