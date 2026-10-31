# SMART AEGIS — Cluster Deployment Guide

================================================================================
SMART AEGIS: Automated Emergency Routing & Smart Traffic Corridor Preemption System
Production Hardware Deployment Handbook — Target: 4-Node Raspberry Pi Cluster
================================================================================

## 1. Hardware Architecture & Cluster Specification

The SMART AEGIS physical demonstration environment operates on a dedicated **4-node Raspberry Pi 4 cluster** running QNX Neutrino RTOS 8.0 (or POSIX Linux/macOS simulation):

```
                              ┌─────────────────────────────────────────┐
                              │  Raspberry Pi 1: Central Coordinator    │
                              │  OS: QNX SDP 8.0 (Pure Software)        │
                              │  Role: TCP Client Engine & Scheduler    │
                              │  Static IP: 10.204.179.4                │
                              │  Hardware: ZERO GPIO ACCESS (No LEDs)   │
                              └────────────────────┬────────────────────┘
                                                   │
                           TCP Sockets / ISO 8601 CRC16-CCITT Control Mesh
                                                   │
                 ┌─────────────────────────────────┼─────────────────────────────────┐
                 │                                 │                                 │
   ┌─────────────┴──────────────┐    ┌─────────────┴──────────────┐    ┌─────────────┴──────────────┐
   │      Raspberry Pi 2        │    │      Raspberry Pi 3        │    │      Raspberry Pi 4        │
   │      Intersection 1        │    │      Intersection 2        │    │      Intersection 3        │
   │  IP: 10.204.179.245:5001   │    │  IP: 10.204.179.130:5002   │    │  IP: 10.204.179.144:5003   │
   │  Role: TCP Server Daemon   │    │  Role: TCP Server Daemon   │    │  Role: TCP Server Daemon   │
   │  LEDs:                     │    │  LEDs:                     │    │  LEDs:                     │
   │  • Red:  BCM 5  (Pin 29)   │    │  • Red:  BCM 5  (Pin 29)   │    │  • Red:  NONE (HW Shortage)│
   │  • Blue: BCM 6  (Pin 31)   │    │  • Blue: BCM 6  (Pin 31)   │    │  • Blue: BCM 6  (Pin 31)   │
   │  Resistors: 330Ω Series    │    │  Resistors: 330Ω Series    │    │  Resistors: 330Ω Series    │
   └────────────────────────────┘    └────────────────────────────┘    └────────────────────────────┘
```

| Physical Device | Role | Node ID | Static IP | Port | Hardware Setup |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Raspberry Pi 1** | Central Coordinator | `COORDINATOR` | `10.204.179.4` | N/A (Client) | Pure software; No LEDs connected |
| **Raspberry Pi 2** | Roadside Intersection 1 | `INT_1` | `10.204.179.245` | `5001` | Red (BCM 5 / Pin 29), Blue (BCM 6 / Pin 31) |
| **Raspberry Pi 3** | Roadside Intersection 2 | `INT_2` | `10.204.179.130` | `5002` | Red (BCM 5 / Pin 29), Blue (BCM 6 / Pin 31) |
| **Raspberry Pi 4** | Roadside Intersection 3 | `INT_3` | `10.204.179.144` | `5003` | Blue (BCM 6 / Pin 31 only; Red shortage handled) |

---

## 2. Directory & Binary Structure

```
AERS/
├── CMakeLists.txt                         # Modern CMake (>= 3.20) build configuration
├── config/
│   └── intersections.json                 # Master cluster network & pin configuration
├── build/
│   ├── bin/
│   │   ├── coordinator                    # Central Coordinator daemon (Pi 1)
│   │   ├── intersection                   # Roadside Intersection daemon (Pi 2, 3, 4)
│   │   ├── simulation                     # In-process simulation dashboard runner
│   │   ├── validator                      # 9-vector pre-flight system validator
│   │   ├── gui                            # Qt6 graphical desktop application
│   │   └── test_smart_aegis_system        # Comprehensive unit & integration test suite
│   └── lib/
│       └── libsmart_aegis_core.a          # Core static engine archive
└── scripts/
    ├── start_coordinator.sh               # Launch Coordinator on Pi 1
    ├── start_intersection1.sh             # Launch Node 1 on Pi 2 (port 5001)
    ├── start_intersection2.sh             # Launch Node 2 on Pi 3 (port 5002)
    ├── start_intersection3.sh             # Launch Node 3 on Pi 4 (port 5003)
    ├── stop_all.sh                        # Stop all daemons
    ├── restart_all.sh                     # Cluster restart orchestrator
    ├── health_check.sh                    # Diagnostic network & port ping utility
    └── deploy.sh                          # Automated SSH distribution script
```

---

## 3. Configuration Management

All cluster parameters are maintained centrally in `config/intersections.json`:

```json
{
  "intersections": [
    {
      "id": 1,
      "name": "MG Road - Brigade Junction",
      "ip": "10.204.179.245",
      "port": 5001,
      "gpio": {
        "red_led_bcm": 5,
        "blue_led_bcm": 6
      }
    },
    {
      "id": 2,
      "name": "Residency Road Cross",
      "ip": "10.204.179.130",
      "port": 5002,
      "gpio": {
        "red_led_bcm": 5,
        "blue_led_bcm": 6
      }
    },
    {
      "id": 3,
      "name": "Richmond Circle",
      "ip": "10.204.179.144",
      "port": 5003,
      "gpio": {
        "red_led_bcm": -1,
        "blue_led_bcm": 6
      }
    }
  ],
  "system": {
    "reconnect_interval_ms": 2000,
    "ping_interval_ms": 5000,
    "command_timeout_ms": 1000,
    "max_command_retries": 3
  }
}
```

---

## 4. Startup Sequence

Because the Coordinator operates as a **TCP Client** and the Intersections operate as **TCP Servers**, the startup sequence is:

```
Step 1: Launch Intersection Nodes (Pis 2, 3, 4)
   ├── Bind TCP server sockets on 5001, 5002, and 5003
   ├── Initialize BCM2711 direct GPIO memory registers (0xFE200000)
   ├── Turn off all LEDs into safe idle state
   └── Enter listen loop waiting for Coordinator connection

Step 2: Launch Central Coordinator (Pi 1)
   ├── Read config/intersections.json
   ├── Connect outgoing TCP sockets to 10.204.179.245:5001, 10.204.179.130:5002, 10.204.179.144:5003
   ├── Receive INTERSECTION_READY greeting frames
   ├── Establish 5-second PING heartbeat interval
   └── Ready to dispatch Green Corridor preemption commands
```

---

## 5. Deployment Procedures

### Automated Cluster Deployment (`scripts/deploy.sh`)

Ensure SSH public key authentication is configured for target user `qnxuser` or `pi`:

```bash
# Set deployment target user if not default
export RPI_USER=qnxuser

# Deploy binaries and configs to all 4 nodes in parallel
./scripts/deploy.sh
```

### Manual Target Deployment

**1. Raspberry Pi 1 (Coordinator - `10.204.179.4`):**
```bash
scp build/bin/coordinator config/intersections.json scripts/start_coordinator.sh scripts/stop_all.sh qnxuser@10.204.179.4:~/aegis/
```

**2. Raspberry Pi 2 (Intersection 1 - `10.204.179.245`):**
```bash
scp build/bin/intersection config/intersections.json scripts/start_intersection1.sh scripts/stop_all.sh qnxuser@10.204.179.245:~/aegis/
```

**3. Raspberry Pi 3 (Intersection 2 - `10.204.179.130`):**
```bash
scp build/bin/intersection config/intersections.json scripts/start_intersection2.sh scripts/stop_all.sh qnxuser@10.204.179.130:~/aegis/
```

**4. Raspberry Pi 4 (Intersection 3 - `10.204.179.144`):**
```bash
scp build/bin/intersection config/intersections.json scripts/start_intersection3.sh scripts/stop_all.sh qnxuser@10.204.179.144:~/aegis/
```

---

## 6. Execution Commands

### On Edge Nodes (Intersections)

**Pi 2 (Node 1):**
```bash
./scripts/start_intersection1.sh
# Or direct command:
./build/bin/intersection --id 1 --port 5001 --hardware
```

**Pi 3 (Node 2):**
```bash
./scripts/start_intersection2.sh
# Or direct command:
./build/bin/intersection --id 2 --port 5002 --hardware
```

**Pi 4 (Node 3):**
```bash
./scripts/start_intersection3.sh
# Or direct command:
./build/bin/intersection --id 3 --port 5003 --hardware
```

### On Master Node (Coordinator)

**Pi 1:**
```bash
./scripts/start_coordinator.sh
# Or direct command:
./build/bin/coordinator --hardware
```

---

## 7. Diagnostics & Cluster Health

To verify network reachability, socket accessibility, and port status:

```bash
./scripts/health_check.sh
```

To run deep system validation:
```bash
./build/bin/validator
```
