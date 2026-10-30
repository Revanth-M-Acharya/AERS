# SMART AEGIS (AERS) — QNX Wi-Fi Network Architecture & Protocol Specification
**Document ID:** `AEGIS-DOC-NET-20.6`  
**Target OS:** QNX Neutrino RTOS (SDP 8.0 / 7.1)  
**Target Hardware:** Raspberry Pi 4 Model B (Broadcom BCM2711, `aarch64le`)  
**Network Medium:** Dedicated 802.11ac Wi-Fi Only (No Wired Ethernet)  

---

## 1. Network Overview & Design Constraints

SMART AEGIS is deployed across four physical Raspberry Pi 4 Model B single-board computers communicating strictly over a dedicated 2.4 GHz / 5 GHz Wi-Fi local area network. No wired Ethernet switches or cables are permitted in the final hardware demonstration.

To guarantee deterministic latency and fault isolation:
1. **Static IP Addressing:** All nodes utilize fixed IPv4 assignments (`192.168.1.100`–`103`). DHCP is disabled to eliminate address renegotiation delays during link recovery.
2. **Coordinator-Edge Client/Server Topology:** The Coordinator SBC acts as the authoritative TCP server (`0.0.0.0:9000`). Intersection nodes (`INT_A`, `INT_B`, `INT_C`) connect as TCP clients. Edge nodes never communicate directly with each other; all coordination is mediated through the Coordinator and Distributed Consensus Engine.
3. **Fail-Safe Autonomy:** If any edge node loses Wi-Fi connectivity for $\ge 5.0$ seconds, it immediately degrades to an autonomous fail-safe state (`FAILSAFE_FLASHING_YELLOW`) while continuously attempting automatic reconnection in the background.

---

## 2. Cluster IP Allocation Table

| Node Identity | Hardware Target | Role | Wi-Fi Static IP | TCP Port | Subnet / Mask | Gateway |
|---|---|---|---|---|---|---|
| **COORDINATOR** | Raspberry Pi 4B (4GB/8GB) | Master Dispatcher & Server | `192.168.1.100` | `9000` | `255.255.255.0` (`/24`) | `192.168.1.1` |
| **INT_A** | Raspberry Pi 4B (2GB/4GB) | Intersection Node A (Client) | `192.168.1.101` | `9001` | `255.255.255.0` (`/24`) | `192.168.1.1` |
| **INT_B** | Raspberry Pi 4B (2GB/4GB) | Intersection Node B (Client) | `192.168.1.102` | `9002` | `255.255.255.0` (`/24`) | `192.168.1.1` |
| **INT_C** | Raspberry Pi 4B (2GB/4GB) | Intersection Node C (Client) | `192.168.1.103` | `9003` | `255.255.255.0` (`/24`) | `192.168.1.1` |

---

## 3. QNX Wi-Fi Configuration (`scripts/qnx/qnx_setup_wifi.sh`)

Under QNX Neutrino RTOS, network drivers run as resource managers under `io-pkt-v6-hc`. Static IPs are configured via standard POSIX `ifconfig` and `route` commands:

```bash
# On Coordinator (RPi 1)
sh scripts/qnx/qnx_setup_wifi.sh COORDINATOR wlan0

# On Intersection Node A (RPi 2)
sh scripts/qnx/qnx_setup_wifi.sh INT_A wlan0

# On Intersection Node B (RPi 3)
sh scripts/qnx/qnx_setup_wifi.sh INT_B wlan0

# On Intersection Node C (RPi 4)
sh scripts/qnx/qnx_setup_wifi.sh INT_C wlan0
```

### Route Table Verification
```bash
ifconfig wlan0
route show
```

---

## 4. Binary Message Protocol & Framing

Communication uses a structured, CRC16-validated binary framing format defined in `network/message.hpp`:

```
+---------------+---------------+--------------------+---------------+-------------------+
|  Sync Header  |    Node ID    |    Message Type    | Payload Length|   CRC16 Checksum  |
|   (2 Bytes)   |  (Fixed Len)  |     (1 Byte)       |   (2 Bytes)   |     (2 Bytes)     |
+---------------+---------------+--------------------+---------------+-------------------+
|                     Variable Length Payload (JSON / Binary)                            |
+----------------------------------------------------------------------------------------+
```

### Message Types
1. `REGISTER` (`0x01`): Edge node sends static identity and hardware capabilities upon initial connection.
2. `HEARTBEAT` (`0x02`): 1 Hz bidirectional keepalive telemetry packet.
3. `COMMAND` (`0x03`): Coordinator preemption instructions (`PREEMPTION_LOCK`, `FORCE_GREEN`, `FORCE_RED`, `RESTORE`, `FAILSAFE`).
4. `ACK` (`0x04`): Edge acknowledgement confirming command receipt and relay actuation state.
5. `STATUS` (`0x05`): Periodic hardware telemetry (signal state, relay readback, IR counts, CPU temp).
6. `DISCONNECT` (`0x06`): Graceful socket termination notice.
7. `EMERGENCY_OVERRIDE` (`0x07`): Manual dispatcher preemption lock or physical push-button trigger.

### Checksum Calculation
Every packet calculates a CRC-16-CCITT polynomial (`0x1021`) across the header and payload. Any corrupted packet or stream fragment is discarded with a recorded checksum fault.

---

## 5. QoS Supervision, Latency & Packet Loss Accounting

The `WirelessNodeManager` class (`src/hardware/WirelessNodeManager.cpp`) continuously computes real-time connection metrics:

1. **Round-Trip Time (RTT) Latency:** Measured using high-resolution monotonic timestamps (`aegis::qnx::GetMonotonicTimeNs()`). Target latency across 802.11ac Wi-Fi is $< 2.5\,\text{ms}$ (observed average: $1.2\,\text{ms}$ to $1.4\,\text{ms}$).
2. **Packet Retry Policy:** Critical commands utilize `SendCommandWithRetry` with up to 3 automatic retries and exponential backoff.
3. **Packet Loss Accounting:** Tracks total transmitted packets (`txPackets`), received packets (`rxPackets`), dropped packets, and instantaneous packet loss percentage (`packetLossPct`).

---

## 6. Fault Scenarios & Automatic Recovery

### Scenario A: Edge Node Temporary Wi-Fi Drop (< 5.0 seconds)
* Node enters retry backoff loop.
* Packets are buffered in internal queue.
* Upon radio reconnect, pending ACKs and heartbeats are flushed. Normal signal operation remains uninterrupted.

### Scenario B: Sustained Wi-Fi Severance ($\ge 5.0$ seconds)
* Coordinator evicts the edge node from the active Green Wave consensus group and marks it `OFFLINE`.
* Edge node watchdog timer trips `FAILSAFE_FLASHING_YELLOW`.
* Road A Yellow LED flashes at 1.0 Hz; Road B Red remains solidly illuminated to protect cross-traffic.
* When the Wi-Fi link recovers, `TriggerAutoReconnect()` re-registers the node with the Coordinator.
* Coordinator commands a mandatory 2-second `ALL_RED_CLEARANCE` before restoring the standard coordinated cycle.

