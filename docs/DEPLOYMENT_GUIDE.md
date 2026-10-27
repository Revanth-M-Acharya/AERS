# SMART AEGIS: Phase 5A Raspberry Pi Deployment Guide

## 1. Hardware Architecture Overview

The SMART AEGIS (Ambulance Emergency Response System) distributed system is deployed across **four physical nodes**:

```
                              ┌─────────────────────────────────────────┐
                              │  Raspberry Pi 1: Central Coordinator    │
                              │  OS: QNX Neutrino RTOS (or POSIX Host)  │
                              │  Role: Coordinator Manager / Server     │
                              │  Default IP: 192.168.1.101 : 9000       │
                              └────────────────────┬────────────────────┘
                                                   │
                          Wi-Fi Network / TCP Sockets (Port 9000)
                                                   │
                ┌──────────────────────────────────┼──────────────────────────────────┐
                │                                  │                                  │
  ┌─────────────┴──────────────┐     ┌─────────────┴──────────────┐     ┌─────────────┴──────────────┐
  │      Raspberry Pi 2        │     │      Raspberry Pi 3        │     │      Raspberry Pi 4        │
  │   Intersection Node A      │     │   Intersection Node B      │     │   Intersection Node C      │
  │         (INT_A)            │     │         (INT_B)            │     │         (INT_C)            │
  │     192.168.1.102:9001     │     │     192.168.1.103:9002     │     │     192.168.1.104:9003     │
  └────────────────────────────┘     └────────────────────────────┘     └────────────────────────────┘
```

| Physical Device | Role | Node ID | Default IP | Port | Target OS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Raspberry Pi 1** | Central Coordinator | `COORDINATOR` | `192.168.1.101` | `9000` | QNX Neutrino RTOS |
| **Raspberry Pi 2** | Roadside Intersection A | `INT_A` | `192.168.1.102` | `9001` | Linux (Raspberry Pi OS) |
| **Raspberry Pi 3** | Roadside Intersection B | `INT_B` | `192.168.1.103` | `9002` | Linux (Raspberry Pi OS) |
| **Raspberry Pi 4** | Roadside Intersection C | `INT_C` | `192.168.1.104` | `9003` | Linux (Raspberry Pi OS) |

---

## 2. Directory & Folder Structure

```
AERS/
├── config/
│   └── deployment/
│       ├── rpi1_coordinator.json       # Raspberry Pi 1 Coordinator profile
│       ├── rpi2_intersection_a.json    # Raspberry Pi 2 Node A profile
│       ├── rpi3_intersection_b.json    # Raspberry Pi 3 Node B profile
│       └── rpi4_intersection_c.json    # Raspberry Pi 4 Node C profile
│
├── build/
│   ├── bin/
│   │   ├── aegis_coordinator           # Central Coordinator executable daemon
│   │   ├── aegis_intersection_node     # Edge Intersection Node executable daemon
│   │   ├── test_rpi_deployment         # Deployment integration test suite
│   │   └── test_system_integration     # Multi-node system integration test suite
│   └── lib/
│       └── libaegis.a                  # Core static library archive
│
├── scripts/
│   ├── build.sh                        # Build automation runner (CMake/Make)
│   ├── start_all_nodes.sh              # Local multi-node deployment orchestrator
│   ├── start_rpi1_coordinator.sh       # RPi 1 startup script
│   ├── start_rpi2_int_a.sh             # RPi 2 startup script
│   ├── start_rpi3_int_b.sh             # RPi 3 startup script
│   └── start_rpi4_int_c.sh             # RPi 4 startup script
│
├── src/
│   ├── apps/
│   │   ├── coordinator_main.cpp        # Entrypoint for aegis_coordinator
│   │   └── intersection_main.cpp       # Entrypoint for aegis_intersection_node
│   ├── config/
│   │   ├── NodeConfig.h                # Deployment configuration data model
│   │   └── NodeConfig.cpp              # Robust JSON configuration loader
│   ├── coordinator/                    # Central decision & corridor engine
│   ├── intersection/                   # SignalController, HeartbeatManager, IntersectionNode
│   ├── system/                         # SystemManager system integration layer
│   └── communication/                  # SocketServer, SocketClient, Message protocol
│
└── tests/
    └── integration/
        ├── test_rpi_deployment.cpp     # Deployment integration test suite
        └── test_system_integration.cpp # Full system integration tests
```

---

## 3. Binary Locations

Following compilation, all binaries are generated in the `build/bin/` directory:

| Binary Name | Relative Path | Purpose |
| :--- | :--- | :--- |
| **`aegis_coordinator`** | `build/bin/aegis_coordinator` | Central Coordinator daemon (Raspberry Pi 1) |
| **`aegis_intersection_node`** | `build/bin/aegis_intersection_node` | Roadside Intersection Node daemon (Raspberry Pis 2, 3, 4) |
| **`test_rpi_deployment`** | `build/bin/test_rpi_deployment` | Phase 5A deployment test suite |
| **`test_system_integration`** | `build/bin/test_system_integration` | System integration verification test suite |

---

## 4. Configuration Editing

Each Raspberry Pi reads its configuration dynamically at launch from a dedicated JSON file using [`NodeConfigLoader`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/src/config/NodeConfig.h). No values are hardcoded in the C++ binaries.

### Raspberry Pi 1 Configuration: `config/deployment/rpi1_coordinator.json`
```json
{
  "nodeId": "COORDINATOR",
  "role": "Coordinator",
  "ipAddress": "192.168.1.101",
  "port": 9000,
  "coordinatorIp": "192.168.1.101",
  "coordinatorPort": 9000,
  "heartbeatIntervalMs": 1000,
  "connectTimeoutMs": 3000,
  "ackTimeoutMs": 3000,
  "autoReconnect": true
}
```

### Raspberry Pi 2 Configuration: `config/deployment/rpi2_intersection_a.json`
```json
{
  "nodeId": "INT_A",
  "role": "Intersection",
  "ipAddress": "192.168.1.102",
  "port": 9001,
  "coordinatorIp": "192.168.1.101",
  "coordinatorPort": 9000,
  "heartbeatIntervalMs": 1000,
  "connectTimeoutMs": 3000,
  "ackTimeoutMs": 3000,
  "autoReconnect": true
}
```

### Raspberry Pi 3 Configuration: `config/deployment/rpi3_intersection_b.json`
```json
{
  "nodeId": "INT_B",
  "role": "Intersection",
  "ipAddress": "192.168.1.103",
  "port": 9002,
  "coordinatorIp": "192.168.1.101",
  "coordinatorPort": 9000,
  "heartbeatIntervalMs": 1000,
  "connectTimeoutMs": 3000,
  "ackTimeoutMs": 3000,
  "autoReconnect": true
}
```

### Raspberry Pi 4 Configuration: `config/deployment/rpi4_intersection_c.json`
```json
{
  "nodeId": "INT_C",
  "role": "Intersection",
  "ipAddress": "192.168.1.104",
  "port": 9003,
  "coordinatorIp": "192.168.1.101",
  "coordinatorPort": 9000,
  "heartbeatIntervalMs": 1000,
  "connectTimeoutMs": 3000,
  "ackTimeoutMs": 3000,
  "autoReconnect": true
}
```

> **Editing Instructions**:
> To change the Wi-Fi subnet or port numbers, simply update `coordinatorIp` on all edge nodes to point to the actual IP assigned to Raspberry Pi 1 by the local Wi-Fi router.

---

## 5. Startup Sequence

The system enforces an **automated, ordered startup sequence**:

```
Step 1: Start Central Coordinator (Raspberry Pi 1)
   └── Binds TCP socket server on port 9000
   └── Initializes EventQueue and IntersectionRegistry
   └── Listens for incoming connections

Step 2: Start Intersection Node A (Raspberry Pi 2)
   └── Connects over Wi-Fi TCP to Coordinator
   └── Sends REGISTER packet
   └── Receives registration ACK
   └── HeartbeatManager starts periodic beacons (1 Hz)

Step 3: Start Intersection Node B (Raspberry Pi 3)
   └── Connects over Wi-Fi TCP to Coordinator
   └── Sends REGISTER packet
   └── Receives registration ACK
   └── HeartbeatManager starts periodic beacons (1 Hz)

Step 4: Start Intersection Node C (Raspberry Pi 4)
   └── Connects over Wi-Fi TCP to Coordinator
   └── Sends REGISTER packet
   └── Receives registration ACK
   └── HeartbeatManager starts periodic beacons (1 Hz)

Step 5: Coordinator Convergence
   └── All 3 nodes confirmed online
   └── Green Corridor preemption ready
```

---

## 6. Deployment Steps

### Step 1: Compile the Project

On your development host or target build machine:
```bash
make all
# Or alternatively using the CMake build script:
./scripts/build.sh
```

Run test suites to verify system integrity:
```bash
make test
# Or:
./scripts/build.sh test
```

### Step 2: Transfer Binaries and Configs to Raspberry Pis

From your build workstation, copy the required files to each Raspberry Pi:

```bash
# 1. Raspberry Pi 1 (Coordinator)
ssh pi@192.168.1.101 "mkdir -p ~/aegis/bin ~/aegis/config/deployment ~/aegis/scripts"
scp build/bin/aegis_coordinator pi@192.168.1.101:~/aegis/bin/
scp config/deployment/rpi1_coordinator.json pi@192.168.1.101:~/aegis/config/deployment/
scp scripts/start_rpi1_coordinator.sh pi@192.168.1.101:~/aegis/scripts/

# 2. Raspberry Pi 2 (Intersection Node A)
ssh pi@192.168.1.102 "mkdir -p ~/aegis/bin ~/aegis/config/deployment ~/aegis/scripts"
scp build/bin/aegis_intersection_node pi@192.168.1.102:~/aegis/bin/
scp config/deployment/rpi2_intersection_a.json pi@192.168.1.102:~/aegis/config/deployment/
scp scripts/start_rpi2_int_a.sh pi@192.168.1.102:~/aegis/scripts/

# 3. Raspberry Pi 3 (Intersection Node B)
ssh pi@192.168.1.103 "mkdir -p ~/aegis/bin ~/aegis/config/deployment ~/aegis/scripts"
scp build/bin/aegis_intersection_node pi@192.168.1.103:~/aegis/bin/
scp config/deployment/rpi3_intersection_b.json pi@192.168.1.103:~/aegis/config/deployment/
scp scripts/start_rpi3_int_b.sh pi@192.168.1.103:~/aegis/scripts/

# 4. Raspberry Pi 4 (Intersection Node C)
ssh pi@192.168.1.104 "mkdir -p ~/aegis/bin ~/aegis/config/deployment ~/aegis/scripts"
scp build/bin/aegis_intersection_node pi@192.168.1.104:~/aegis/bin/
scp config/deployment/rpi4_intersection_c.json pi@192.168.1.104:~/aegis/config/deployment/
scp scripts/start_rpi4_int_c.sh pi@192.168.1.104:~/aegis/scripts/
```

### Step 3: Launch Nodes

**Terminal 1 (Raspberry Pi 1 - Coordinator):**
```bash
./scripts/start_rpi1_coordinator.sh
# Expected output:
# [READY] Central Coordinator listening for Intersection Nodes...
```

**Terminal 2 (Raspberry Pi 2 - INT_A):**
```bash
./scripts/start_rpi2_int_a.sh
# Expected output:
# [READY] Registered with Coordinator! Heartbeat beaconing active.
```

**Terminal 3 (Raspberry Pi 3 - INT_B):**
```bash
./scripts/start_rpi3_int_b.sh
# Expected output:
# [READY] Registered with Coordinator! Heartbeat beaconing active.
```

**Terminal 4 (Raspberry Pi 4 - INT_C):**
```bash
./scripts/start_rpi4_int_c.sh
# Expected output:
# [READY] Registered with Coordinator! Heartbeat beaconing active.
```

---

## 7. Local Multi-Node Simulation / Orchestration

For local workstation testing (simulating all 4 Raspberry Pis on loopback):
```bash
./scripts/start_all_nodes.sh
```
This script automatically executes the ordered startup sequence, registers Nodes A, B, and C with the Coordinator, and terminates all 4 processes cleanly upon receiving `Ctrl+C`.
