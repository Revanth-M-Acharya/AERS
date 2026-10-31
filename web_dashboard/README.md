# SMART AEGIS — Digital Twin Operations Center

================================================================================
Cyber-Physical Digital Twin Web Dashboard for QNX SDP 8.0 International Hackathon
Target: React 18, TypeScript, Vite, TailwindCSS, Framer Motion, Recharts
================================================================================

The **SMART AEGIS Digital Twin Operations Center** is a modern, cyber-operations web application providing real-time visualization, mission control, and digital-twin synchronization for the SMART AEGIS Emergency Response platform.

It operates seamlessly in **two modes without code modification**:
1. **Simulation Mode:** Standalone in-browser simulation engine generating dynamic Green Corridors, traffic preemption, road hazards, and ambulance transit.
2. **Live QNX Cluster Mode:** Connects to the master Coordinator via WebSocket to mirror live telemetry from real Raspberry Pi 4 nodes running QNX Neutrino RTOS 8.0.

---

## 1. Quick Start

### Installation
Ensure Node.js (>= 18) is installed.

```bash
cd web_dashboard
npm install
```

### Launch Development Server
```bash
npm run dev
```

Open `http://localhost:3000` in your web browser.

### Production Build
```bash
npm run build
```

---

## 2. Digital Twin Architecture & Features

### A. Navigation Structure
- **Dashboard:** High-level executive overview (KPI cards, mini city map, telemetry strip, live audit logs).
- **City Map:** Fullscreen interactive SVG map with dynamic green corridor glow, 3 intersections with signal heads and GPIO pods, animated ambulance, pan, zoom, and sensor triggers.
- **Command Center:** Emergency call ingestion, multi-criteria triage scoring, Dijkstra route execution timeline, and historical mission log.
- **Emergency Calls:** Intake deck for simulated 911 / 108 emergency calls with priority tagging (`P1_CRITICAL`, `P2_URGENT`, `P3_STANDARD`).
- **Ambulances:** Fleet tracking deck with live vehicle speed, route progress, and continuous patient telemetry (Heart rate, SpO2, Blood Pressure).
- **Traffic Signals:** Roadside intersection node status, physical BCM GPIO pin assignments (Pin 29 BCM5, Pin 31 BCM6), and Node 3 hardware shortage virtualization.
- **Hospitals:** Trauma Level-1 readiness, trauma bays, surgical suites, and real-time ER triage wait times.
- **Digital Twin:** Bi-directional synchronization engine verifying in-memory virtual state vs physical QNX cluster state.
- **Analytics:** Recharts data visualizations comparing standard transit times vs SMART AEGIS preemption (-61% delay reduction), packet rates, and CPU load.
- **Mission Replay:** Blackbox mission retrospective player with timeline scrubber and speed multipliers (1x, 2x, 5x).
- **Event Logs:** Searchable, filterable audit log with one-click JSON and CSV export.
- **Settings:** WebSocket live bridge endpoint configuration and physical cluster IP registry.

### B. Scenario & Fault Injection Matrix
The control panel allows testing 15 dynamic scenarios:
- **Emergency Dispatch:** Triggers P1 cardiac intake mission with full 5-stage preemption sequence.
- **Police Override:** Instantly freezes all intersections into emergency hold.
- **Road Blocked:** Simulates an obstruction on MG Road, triggering dynamic Dijkstra replanning.
- **Heavy Traffic:** Dynamically increases cross-traffic density and expands preemption green wave window.
- **Accident Scene:** Places a secondary collision on the route.
- **Ambulance Breakdown:** Halts primary vehicle and routes telemetry to backup unit.
- **Node Failure:** Simulates Intersection 2 dropping off network and entering autonomous amber fail-safe.
- **Environmental Controls:** Day, Night, Rain (reduced traction), and Fog (reduced visibility).

---

## 3. Technology Stack

- **Framework:** React 18, TypeScript, Vite
- **Styling:** TailwindCSS, Custom Glassmorphism, Cyber Ops Dark Theme
- **Icons:** Lucide React
- **Charts:** Recharts
- **Networking:** WebSocket Client with auto-reconnect and frame deserializer
