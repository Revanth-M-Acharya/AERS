# QNX Network Communication Layer (SMART AEGIS)
## Target: Raspberry Pi 4 Model B / QNX SDP 8.0 (QNX Neutrino RTOS) & POSIX

---

### 1. System Architecture

```
                 ┌────────────────────────────────┐
                 │       Coordinator Node         │
                 │          (TCP Server)          │
                 │       IP: 192.168.1.101        │
                 │           Port: 9000           │
                 └───────────────┬────────────────┘
                                 │
         ┌───────────────────────┼───────────────────────┐
         │                       │                       │
         ▼                       ▼                       ▼
┌──────────────────┐    ┌──────────────────┐    ┌──────────────────┐
│  Traffic Node A  │    │  Traffic Node B  │    │  Traffic Node C  │
│   (TCP Client)   │    │   (TCP Client)   │    │   (TCP Client)   │
│ IP: 192.168.1.102│    │ IP: 192.168.1.103│    │ IP: 192.168.1.104│
│    Port: 9001    │    │    Port: 9002    │    │    Port: 9003    │
└──────────────────┘    └──────────────────┘    └──────────────────┘
```

The Central Coordinator acts as the centralized **TCP Server**, while edge Traffic Nodes (A, B, C) on Raspberry Pi 4 act as **TCP Clients**.

---

### 2. Directory Structure

```
network/
├── CMakeLists.txt        # CMake build configuration
├── Makefile              # Standalone Makefile for quick compilation
├── README.md             # Architecture, protocol, and usage documentation
├── example_client.cpp    # Standalone Traffic Node demonstration client
├── example_server.cpp    # Standalone Coordinator demonstration server
├── message.cpp           # Wire framing, CRC16, and serialization implementation
├── message.hpp           # Message protocol definition and headers
├── tcp_client.cpp        # Non-blocking client with 1-sec heartbeats & auto-reconnect
├── tcp_client.hpp        # Client header and callbacks
├── tcp_server.cpp        # Multi-client server with 5-sec heartbeat timeout supervision
└── tcp_server.hpp        # Server header and session management
```

---

### 3. Wire Framing & Message Protocol

Every packet is framed with fixed magic bytes and length fields to prevent packet truncation over stream sockets:

```
+-------------------+---------------+-----------------------------------------+
| Field             | Size          | Description                             |
+-------------------+---------------+-----------------------------------------+
| Magic Bytes       | 2 Bytes       | Fixed sync word: 0xA5, 0x5A             |
| Message Type      | 1 Byte        | Enum (1=REGISTER, 2=HEARTBEAT, etc.)    |
| Timestamp         | 8 Bytes       | Big-endian Unix epoch milliseconds      |
| Node ID Length    | 2 Bytes       | Big-endian length of Node ID string (N) |
| Payload Length    | 4 Bytes       | Big-endian length of Payload (M)        |
| Checksum          | 2 Bytes       | CRC16-CCITT (poly 0x1021) over data     |
| Node ID           | N Bytes       | ASCII identifier (e.g. TRAFFIC_NODE_A)  |
| Payload           | M Bytes       | Application payload (JSON / binary)     |
+-------------------+---------------+-----------------------------------------+
```

Fixed Header Size: **19 Bytes**. Total Packet Length: **19 + N + M Bytes**.

#### Supported Message Types:
1. `REGISTER` (1): Initial node handshake and registration with Coordinator.
2. `HEARTBEAT` (2): 1-second keepalive ping.
3. `STATUS` (3): Node health, CPU temperature, queue occupancy.
4. `TRAFFIC_STATE` (4): Signal phase (RED/GREEN/YELLOW), remaining window time.
5. `COMMAND` (5): Preemption corridor initiation and phase override command.
6. `ACK` (6): Message receipt and execution confirmation.
7. `ERROR` (7): Fault notification or checksum alert.

---

### 4. Heartbeat Supervision & Timeout Isolation

- **Client Heartbeat Dispatch:** Each edge Traffic Node background thread transmits a `HEARTBEAT` packet every **1 second** (1,000 ms).
- **Coordinator Timeout Supervision:** The Coordinator background watchdog supervises all connected nodes. If a node fails to report a heartbeat within **5 seconds** (5,000 ms), the Coordinator automatically logs the timeout, closes the socket, and purges the node from the active cluster.

---

### 5. Multi-Client Concurrency & QNX Native Threading

- Implemented using QNX Neutrino RTOS native POSIX thread mechanisms (`pthread_create`, `pthread_join`, `pthread_mutex_t`, and C++17 `std::thread`).
- Dedicated reader thread per connected client ensures simultaneous, non-blocking communication across all traffic nodes.
- Disjoint-scope mutex locking protects connection registries and statistics without deadlocks.

---

### 6. Compilation Instructions

#### Method 1: Standalone Build inside `network/`
```bash
cd network
make
```
This produces:
- `libaegis_qnx_network.a` (Static reusable networking library)
- `example_server` (Coordinator binary)
- `example_client` (Traffic Node binary)

#### Method 2: CMake Standalone Build
```bash
cd network
mkdir -p build && cd build
cmake ..
cmake --build .
```

#### Method 3: Full Project Integration (Root Makefile)
```bash
# In repository root:
make -j8
```
Binaries will be placed in `build/bin/`:
- `build/bin/test_qnx_network_layer`
- `build/bin/example_server`
- `build/bin/example_client`

---

### 7. Execution & Testing Instructions

#### Terminal 1: Launch Coordinator Server
```bash
./network/example_server 9000
```

#### Terminal 2: Launch Traffic Node A
```bash
./network/example_client TRAFFIC_NODE_A 127.0.0.1 9000
```

#### Terminal 3: Launch Traffic Node B
```bash
./network/example_client TRAFFIC_NODE_B 127.0.0.1 9000
```

#### Terminal 4: Launch Traffic Node C
```bash
./network/example_client TRAFFIC_NODE_C 127.0.0.1 9000
```

#### Running the Automated Unit Test Suite
```bash
./build/bin/test_qnx_network_layer
# or from root:
make test
```
All 12 test cases will execute, validating packet serialization, checksum verification, multi-client scaling, 1-second heartbeats, 5-second timeouts, and graceful reconnection.

