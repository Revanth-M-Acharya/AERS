# SMART AEGIS (AERS) — Phase 12 Specification & Field Dossier
## Hardware Demonstration, Live Operation & Police-Assisted Dynamic Replanning

---

### Executive Summary

SMART AEGIS (**Autonomous Emergency Response System**) Phase 12 marks the transition of the platform from a certified RTOS-native software core into a physical, field-integrated multi-node hardware demonstration. 

Building strictly upon the software architecture completed across Phases 1–11, Phase 12 introduces **zero architectural modifications**, **zero algorithmic rewrites**, and **no reduction in coordinator authority**. All scheduling, routing, safety, perception, and digital twin pipelines remain 100% intact. 

Phase 12 physically integrates four (4) Raspberry Pi 4 Model B single-board computers running native **QNX Neutrino RTOS 8.0** (`aarch64le`), interconnected via a dedicated low-latency Wi-Fi / Ethernet TCP/IP mesh. Physical actuators (4-channel optocoupled relay modules driving dual-road 12V traffic lamps), optical IR vehicle approach sensors, an acoustic preemption buzzer, tactile emergency and police override switches, and an I2C OLED V2X display simulator work synchronously to execute live emergency corridors, simultaneous dual-ambulance priority arbitration, and police-assisted autonomous dynamic rerouting with **0.00 seconds of mission interruption**.

---

### 1. Hardware Architecture & System Topology

The physical demonstration layout represents a metropolitan emergency transit corridor connecting an ambulance station (**HOME**) to a regional trauma center (**HOSPITAL**), intersecting three major junctions controlled by independent QNX traffic signal nodes. A bypass thoroughfare circumvents Intersection 2, providing an alternate arterial route when police report unexpected obstructions.

```
+---------------------------------------------------------------------------------------------------------+
|                                     PHYSICAL DEMONSTRATION ROAD TOPOLOGY                                |
+---------------------------------------------------------------------------------------------------------+

 [ HOME ] ---> [ INTERSECTION 1 ] ------------> [ INTERSECTION 2 ] ------------> [ INTERSECTION 3 ] ---> [ HOSPITAL ]
 (Switch)         (RPi Node 1)                     (RPi Node 2)                     (RPi Node 3)
                     |                                  ^                                ^
                     |                                  | (Approach B)                   |
                     |                           [ DEPOT EAST ]                          |
                     |                           (Ambulance B)                           |
                     |                                                                   |
                     +----------------------> [ BYPASS ROUTE ] --------------------------+
                                              (Dynamic Detour)
```

#### Node Allocation & Network Addressing

| Node Identifier | Physical Hardware | QNX SDP 8.0 Target | IP Address | Listen Port | Hardware Peripheral Interfaces |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **RPI_COORD** | Raspberry Pi 4 Model B (4GB) | `aarch64le` | `192.168.1.100` | `9000` | Buzzer, Emergency Switch, Police Switch, I2C OLED |
| **INT_1** | Raspberry Pi 4 Model B (4GB) | `aarch64le` | `192.168.1.101` | `9001` | 4-Ch Relay (Dual Road Signals), Approach IR Sensor |
| **INT_2** | Raspberry Pi 4 Model B (4GB) | `aarch64le` | `192.168.1.102` | `9002` | 4-Ch Relay (Dual Road Signals), Dual Approach IR Sensors |
| **INT_3** | Raspberry Pi 4 Model B (4GB) | `aarch64le` | `192.168.1.103` | `9003` | 4-Ch Relay (Dual Road Signals), Approach IR Sensor |

> [!IMPORTANT]
> **No Microcontrollers:** There are **zero ESP32 boards** in this deployment. All optical sensor edge debouncing, signal interlocking, relay actuation, acoustic rhythm synthesis, and display updates are executed directly on the Raspberry Pi 4 BCM2711 SoC under native QNX Neutrino RTOS microkernel threads.

---

### 2. Bill of Materials (BOM)

| Item # | Component Description | Specification / Model | Qty | Target Node | Purpose |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **1** | Single-Board Computer | Raspberry Pi 4 Model B (4 GB RAM, Quad Cortex-A72 @ 1.5 GHz) | 4 | All Nodes | Core computation, QNX RTOS microkernel |
| **2** | Storage Card | SanDisk Extreme Pro 32GB MicroSD (A2, V30, U3) | 4 | All Nodes | QNX SDP 8.0 Starter Kit IFS / Root FS |
| **3** | Relay Actuator Board | 4-Channel 5V Optocoupled Relay Module (10A 250VAC / 30VDC) | 3 | INT_1, 2, 3 | Physical isolation & traffic lamp switching |
| **4** | Traffic Signal Modules | 12V / 5V Common-Cathode Red/Yellow/Green LED Traffic Light Units | 6 | INT_1, 2, 3 | 2x per intersection (Road A & Road B) |
| **5** | Optical Proximity Sensor | Active Optical Infrared Obstacle Sensor (LM393, 3.3V/5V) | 4 | INT_1, 2, 3 | Approach vehicle detection (Int 2 has 2 sensors) |
| **6** | Acoustic Transducer | 5V Active Piezoelectric Buzzer Module (Continuous drive) | 1 | RPI_COORD | 5 distinct acoustic preemption rhythm patterns |
| **7** | Emergency Push Switch | SPST Momentary Heavy-Duty Push Button (Normally Open, Red) | 1 | RPI_COORD | Physical mission intake trigger near HOME |
| **8** | Police Override Switch | SPST Latching / Heavy Momentary Switch (Yellow / Blue) | 1 | RPI_COORD | Verified roadside incident / barricade reporting |
| **9** | V2X Advisory Display | 0.96-inch Monochrome I2C OLED (SSD1306, 128x64 pixels) | 1 | RPI_COORD | In-vehicle & roadside simulated V2X alert panel |
| **10** | Power Distribution Hub | 5V 10A DC Multi-Channel Switching Power Supply (MeanWell) | 1 | Benchtop | Shared regulated 5V logic & relay coil power |
| **11** | Network Infrastructure | 5-Port Gigabit Ethernet Switch & 2.4/5GHz Wi-Fi Access Point | 1 | Benchtop | Low-latency TCP/IP mesh interconnect |

---

### 3. Symbolic GPIO Pinout & Electrical Interfacing

SMART AEGIS strictly adheres to the **Symbolic GPIO Hardware Abstraction Layer** (`SymbolicGpioHAL`), eliminating all hardcoded GPIO pin numbers from application logic. Pin allocations are ingested from `config/cluster/demo_hardware.conf`.

#### 3.1 Coordinator Node (RPi 1) Pin Mapping

| Symbolic Signal Name | BCM GPIO | Physical Header Pin | Direction | Active Polarity | Electrical Interface & Wiring Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `COORDINATOR_BUZZER` | **GPIO 6** | Pin 31 | `OUTPUT` | `ACTIVE_HIGH` | Transistor switch to 5V active buzzer module |
| `EMERGENCY_SWITCH` | **GPIO 23** | Pin 16 | `INPUT` | `ACTIVE_LOW` | Internal pull-up to 3.3V; switch connects to GND |
| `POLICE_OVERRIDE_SWITCH` | **GPIO 24** | Pin 18 | `INPUT` | `ACTIVE_LOW` | Internal pull-up to 3.3V; switch connects to GND |
| `COORDINATOR_STATUS_LED` | **GPIO 18** | Pin 12 | `OUTPUT` | `ACTIVE_HIGH` | Green status LED via 330Ω current limiting resistor |
| `OLED_I2C_SDA` | **GPIO 2** | Pin 3 | `BIDIRECTIONAL` | — | I2C-1 Data (Hardware pull-up on OLED module) |
| `OLED_I2C_SCL` | **GPIO 3** | Pin 5 | `OUTPUT` | — | I2C-1 Clock (Hardware pull-up on OLED module) |

#### 3.2 Intersection 1 (RPi 2) Pin Mapping

| Symbolic Signal Name | BCM GPIO | Physical Pin | Direction | Active Polarity | Controlled Element |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `INT1_ROAD_A_RED` | **GPIO 17** | Pin 11 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 1 (Road A Red Lamp) |
| `INT1_ROAD_A_YELLOW` | **GPIO 27** | Pin 13 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 2 (Road A Yellow Lamp) |
| `INT1_ROAD_A_GREEN` | **GPIO 22** | Pin 15 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 3 (Road A Green Lamp - Corridor) |
| `INT1_ROAD_B_RED` | **GPIO 23** | Pin 16 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Road B Red Lamp) |
| `INT1_ROAD_B_YELLOW` | **GPIO 24** | Pin 18 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Road B Yellow Lamp) |
| `INT1_ROAD_B_GREEN` | **GPIO 25** | Pin 22 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 4 (Road B Green Lamp) |
| `INT1_APPROACH_IR` | **GPIO 12** | Pin 32 | `INPUT` | `ACTIVE_LOW` | Optical IR Approach Sensor (Debounced 30ms) |

#### 3.3 Intersection 2 (RPi 3 - Priority Hub) Pin Mapping

| Symbolic Signal Name | BCM GPIO | Physical Pin | Direction | Active Polarity | Controlled Element |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `INT2_ROAD_A_RED` | **GPIO 5** | Pin 29 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 1 (Top Approach Road A Red Lamp) |
| `INT2_ROAD_A_YELLOW` | **GPIO 6** | Pin 31 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 2 (Top Approach Road A Yellow Lamp) |
| `INT2_ROAD_A_GREEN` | **GPIO 13** | Pin 33 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 3 (Top Approach Road A Green Lamp) |
| `INT2_ROAD_B_RED` | **GPIO 19** | Pin 35 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Right Approach Road B Red Lamp) |
| `INT2_ROAD_B_YELLOW` | **GPIO 26** | Pin 37 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Right Approach Road B Yellow Lamp) |
| `INT2_ROAD_B_GREEN` | **GPIO 16** | Pin 36 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 4 (Right Approach Road B Green Lamp) |
| `INT2_APPROACH_IR_TOP` | **GPIO 20** | Pin 38 | `INPUT` | `ACTIVE_LOW` | Optical IR Sensor Approach A (Top - Amb A) |
| `INT2_APPROACH_IR_RIGHT` | **GPIO 21** | Pin 40 | `INPUT` | `ACTIVE_LOW` | Optical IR Sensor Approach B (Right - Amb B) |

#### 3.4 Intersection 3 (RPi 4) Pin Mapping

| Symbolic Signal Name | BCM GPIO | Physical Pin | Direction | Active Polarity | Controlled Element |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `INT3_ROAD_A_RED` | **GPIO 7** | Pin 26 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 1 (Road A Red Lamp) |
| `INT3_ROAD_A_YELLOW` | **GPIO 8** | Pin 24 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 2 (Road A Yellow Lamp) |
| `INT3_ROAD_A_GREEN` | **GPIO 11** | Pin 23 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 3 (Road A Green Lamp - Corridor) |
| `INT3_ROAD_B_RED` | **GPIO 9** | Pin 21 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Road B Red Lamp) |
| `INT3_ROAD_B_YELLOW` | **GPIO 10** | Pin 19 | `OUTPUT` | `ACTIVE_HIGH` | Direct Drive (Road B Yellow Lamp) |
| `INT3_ROAD_B_GREEN` | **GPIO 25** | Pin 22 | `OUTPUT` | `ACTIVE_LOW` | Relay Ch 4 (Road B Green Lamp) |
| `INT3_APPROACH_IR` | **GPIO 18** | Pin 12 | `INPUT` | `ACTIVE_LOW` | Optical IR Approach Sensor (Debounced 30ms) |

---

### 4. Complete Electrical Wiring Schematics

#### 4.1 Coordinator Wiring Schematic (RPi 1)

```
                       RASPBERRY PI 4 (COORDINATOR)
                     +-------------------------------+
                     |                               |
                     |  [Pin 01] 3.3V DC Power       |
                     |  [Pin 02] 5.0V DC Power ----+ |
                     |  [Pin 03] GPIO 2 (SDA) ---+ | |
                     |  [Pin 05] GPIO 3 (SCL) -+ | | |
                     |  [Pin 06] GND ----------|-|-|-+---------------+
                     |  [Pin 31] GPIO 6 -------|-|-|-|--+            |
                     |  [Pin 16] GPIO 23 ------|-|-|-|--|--+         |
                     |  [Pin 18] GPIO 24 ------|-|-|-|--|--|--+      |
                     |                               |  |  |  |      |
                     +-------------------------------+  |  |  |      |
                                                        |  |  |      |
        +-----------------------------------------------+  |  |      |
        |                                                  |  |      |
        |   +-------------------+                          |  |      |
        +-->| VCC               |                          |  |      |
            |      SSD1306      |                          |  |      |
     +----->| SDA   0.96" OLED  |                          |  |      |
     | +--->| SCL   128x64 I2C  |                          |  |      |
     | | +->| GND               |                          |  |      |
     | | |  +-------------------+                          |  |      |
     | | |                                                 |  |      |
     | | |  +-------------------+                          |  |      |
     | | |  | VCC (+5V) <-------+                          |  |      |
     | | +->| GND               |                          |  |      |
     | |    | SIG <-------------|--------------------------+  |      |
     | |    |  ACTIVE BUZZER    |                             |      |
     | |    +-------------------+                             |      |
     | |                                                      |      |
     | |    +-------------------+                             |      |
     | |    | PIN 1 <-----------|-----------------------------+      |
     | +--->| PIN 2 (GND)       | (Internal 3.3V Pull-Up)            |
     |      |  EMERGENCY SWITCH | (Momentary Push Button)            |
     |      +-------------------+                                    |
     |                                                               |
     |      +-------------------+                                    |
     |      | PIN 1 <-----------|------------------------------------+
     +----->| PIN 2 (GND)       | (Internal 3.3V Pull-Up)
            |   POLICE SWITCH   | (Road Closed / Barricade)
            +-------------------+
```

#### 4.2 Traffic Node Wiring Schematic (RPi 2, 3, 4 with Optocoupled Relay & Dual Signals)

```
                       RASPBERRY PI 4 (TRAFFIC NODE)
                     +-------------------------------+
                     |                               |
                     |  [Pin 02] 5.0V DC Power ----+ |
                     |  [Pin 06] GND --------------|-+---------------+
                     |  [Pin 11] GPIO 17 (IN1) --+ | |               |
                     |  [Pin 13] GPIO 27 (IN2) -+|-| |               |
                     |  [Pin 15] GPIO 22 (IN3) +|-|-|-+              |
                     |  [Pin 22] GPIO 25 (IN4) |-|-|-|-+             |
                     |  [Pin 32] GPIO 12 (IR) -|-|-|-|-|-+           |
                     |                               | | | | | |     |
                     +-------------------------------+ | | | | |     |
                                                       | | | | |     |
     +-------------------------------------------------+ | | | |     |
     |   +---------------------------------------------+ | | | |     |
     |   |   +-------------------------------------------+ | | |     |
     |   |   |   +-----------------------------------------+ | |     |
     |   |   |   |   +---------------------------------------+ |     |
     |   |   |   |   |                                         |     |
     |   |   |   |   |    4-CHANNEL OPTOCOUPLED RELAY          |     |
     |   |   |   |   |  +-----------------------------+        |     |
     +-->|---|---|---|->| VCC                         |        |     |
         |   |   |   |  | GND <-----------------------|--------+     |
         +-->|---|---|->| IN1 (Road A Red)   [COM-NO] |----> [ROAD A RED LAMP]
             +-->|---|->| IN2 (Road A Yellow)[COM-NO] |----> [ROAD A YELLOW LAMP]
                 +-->|->| IN3 (Road A Green) [COM-NO] |----> [ROAD A GREEN LAMP (Corridor)]
                     +->| IN4 (Road B Green) [COM-NO] |----> [ROAD B GREEN LAMP]
                        +-----------------------------+
                                                                     |
                        OPTICAL INFRARED PROXIMITY SENSOR            |
                        +-----------------------------+              |
                        | VCC (3.3V or 5V) <----------+              |
                        | GND <--------------------------------------+
                        | OUT (Digital) -------------> [To RPi GPIO 12 / 20 / 21]
                        +-----------------------------+
```

---

### 5. Mission Execution & Control Sequences

#### 5.1 Feature 1: Emergency Mission Trigger Sequence

When an emergency call or medical emergency arises, the tactile **Emergency Switch** located near **HOME** is pressed.

```mermaid
sequenceDiagram
    autonumber
    actor Dispatcher as Emergency Dispatcher
    participant Sw as Emergency Switch Driver
    participant HAL as Symbolic Gpio HAL
    participant Coord as Coordinator Core
    participant Planner as Dijkstra Route Planner
    participant Corridor as Green Corridor Planner
    participant Nodes as RPi Intersections 1, 2, 3
    participant Buzzer as Acoustic Buzzer
    participant OLED as OLED V2X Display
    participant DT as Operations Digital Twin

    Dispatcher->>Sw: Presses Switch (GND Contact)
    Sw->>HAL: Read Pin (GPIO 23 asserted)
    Sw->>Coord: OnEmergencyTrigger(timestamp)
    Coord->>Coord: Create Mission EMG-01 (Triage: Code Red)
    Coord->>Planner: ComputeShortestPath(HOME, HOSPITAL)
    Planner-->>Coord: Route: HOME -> INT_1 -> INT_2 -> INT_3 -> HOSPITAL (1450m)
    Coord->>Corridor: GenerateCorridorPlan(AMB_01, Route, ETA)
    Corridor-->>Coord: Staged Green Wave Plan Generated
    
    par Hardware Preemption Dispatch
        Coord->>Nodes: TCP DISPATCH: SetEmergencyPreemption(ROAD_A_GREEN)
        Nodes-->>Coord: TCP ACK: Confirmed Preemption Active
    and Acoustic Warning
        Coord->>Buzzer: PlayPattern(CONTINUOUS_TONE)
    and V2X Visual Broadcast
        Coord->>OLED: ShowAmbulanceApproaching() -> ShowGreenCorridorActive()
    and Digital Twin Sync
        Coord->>DT: Publish(EmergencyCreated, ExecutionStarted)
    end
```

#### 5.2 Feature 2: Mission Completion Sequence

When the ambulance traverses the final approach and reaches the **HOSPITAL**, the emergency corridor is safely released:

1. Final IR sensor at Intersection 3 registers vehicle clearance.
2. Coordinator receives corridor completion event.
3. Coordinator issues `RESTORE_NORMAL_CYCLE` commands to RPi Intersections 1, 2, and 3.
4. Relay drivers execute soft transitions (Yellow clearance where appropriate, returning to normal alternating cycle).
5. **Buzzer Pattern:** Switches immediately from Continuous Tone to **Two Long Beeps** (`450ms ON, 200ms OFF, 450ms ON`).
6. **OLED Display:** Renders `MISSION COMPLETE / SIGNALS NORMAL`.
7. **Digital Twin:** Archives mission statistics (Travel time reduction: 48%, Zero safety violations, 100% preemption compliance).

---

### 6. Feature 3: Intersection 2 Dual-Ambulance Priority Scheduling

Intersection 2 is configured as a high-density crossing where two emergency vehicles converge simultaneously:
- **Ambulance A:** Approaching from **Top** (Arterial corridor from HOME, `Priority 1 / CRITICAL - Cardiac Arrest / Stroke`).
- **Ambulance B:** Approaching from **Right** (Cross street from Depot East, `Priority 2 / HIGH - Severe Trauma / Fracture`).

```mermaid
flowchart TD
    A["Simultaneous Arrival Detected at Intersection 2"] --> B["IR Sensor Top Triggered (Ambulance A)"]
    A --> C["IR Sensor Right Triggered (Ambulance B)"]
    
    B --> D["Coordinator PriorityScheduler"]
    C --> D
    
    D --> E{"Arbitrate Medical Priority Levels"}
    E -->|"Priority 1 > Priority 2"| F["Grant Reservation to Ambulance A (Top)"]
    E -->|"Hold Ambulance B"| G["Calculate Queue Delay for Ambulance B"]
    
    F --> H["Set Road A Green (Corridor Passage)"]
    G --> I["Set Road B Red (Safe Hold)"]
    
    H --> J["Acoustic Buzzer: 3 Short Beeps"]
    I --> J
    
    J --> K["OLED: PASS AMB-01 / HOLD AMB-02 / WAIT 15.0s"]
    K --> L["Ambulance A Clears Intersection 2"]
    L --> M["Automatically Grant Road B Green to Ambulance B"]
```

#### Console Dashboard Output Format

```
======================================================
       INTERSECTION 2 PRIORITY SCHEDULING PANEL       
======================================================
Mission A: MISSION_A | AMB_01 [Approach: TOP]
Priority 1 (CRITICAL - Cardiac Arrest / Stroke)

Mission B: MISSION_B | AMB_02 [Approach: RIGHT]
Priority 2 (HIGH - Limb Fracture / Trauma)
------------------------------------------------------
Selected Mission:       MISSION_A (AMB_01)
Arbitration Reason:     Higher medical triage priority (Code Red Priority 1 > Code Yellow Priority 2)
Ambulance B Status:     HELD AT SIGNAL (RED)
Estimated Waiting Time: 15.0 sec
======================================================
```

---

### 7. Feature 4: Police-Assisted Dynamic Route Replanning

#### Architectural Invariant: Sole Decision Authority

> [!CAUTION]
> **POLICE OVERRIDE DOES NOT PAUSE THE MISSION OR TAKE MANUAL TRAFFIC LIGHT CONTROL.**
> Under the SMART AEGIS architecture, the Police Override Switch represents traffic police on-scene reporting a verified road blockage (e.g. fallen tree, water main break, major multi-vehicle accident, or temporary police barricade) at **Intersection 2**. 
> The Coordinator retains **100% sovereign authority**. The police simply supply verified ground truth. The Coordinator instantly and autonomously recalculates the safest detour via the bypass route with **zero pause, zero manual intervention, and zero mission delay**.

```mermaid
sequenceDiagram
    autonumber
    actor Police as Traffic Police Officer
    participant Sw as Police Override Switch
    participant Coord as Coordinator Intelligence
    participant Graph as Road Graph Model
    participant Dijkstra as Dijkstra Route Planner
    participant Corridor as Green Corridor Planner
    participant OldNode as RPi Intersection 2 (Blocked)
    participant BypassNodes as RPi Nodes 1, Bypass, 3
    participant Buzzer as Acoustic Buzzer
    participant OLED as OLED V2X Display
    participant DT as Operations Digital Twin

    Police->>Sw: Presses Police Switch (Road Blocked at Int 2)
    Sw->>Coord: POLICE_OVERRIDE (ROAD_CLOSED: INT_2)
    Note over Coord: ZERO MISSION PAUSE<br/>Coordinator retains 100% authority
    Coord->>Graph: SetIntersectionEnabled(INT_2, false)<br/>SetSegmentBlocked(SEG_INT1_INT2, true)
    Coord->>Dijkstra: RecomputeRoute(CurrentPos: INT_1, Dest: HOSPITAL, Avoid: INT_2)
    Dijkstra-->>Coord: New Route: INT_1 -> INT_BYPASS -> INT_3 -> HOSPITAL
    Coord->>Corridor: GenerateCorridorPlan(New Route, New ETA: 42s)
    
    par Revert Blocked Intersection
        Coord->>OldNode: RestoreNormalCycling()
    and Preempt New Bypass Corridor
        Coord->>BypassNodes: SetEmergencyPreemption(ROAD_A_GREEN)
    and Sound Police Event Alert
        Coord->>Buzzer: PlayPattern(RAPID_BEEPS)
    and Update V2X Display
        Coord->>OLED: ShowPoliceReplanning("NEW ETA: 42s | BYPASS")
    and Animate Digital Twin
        Coord->>DT: Publish(RouteChanged, CorridorReplanningAdvised)
        DT->>DT: Remove Old Corridor -> Render New Bypass Corridor
    end
```

#### Console Dashboard Output Format

```
==============================
POLICE EVENT
Road Closed
Intersection 2
Replanning Route...
New ETA
42 sec
New Corridor Generated
==============================
```

---

### 8. Feature 5: OLED V2X Warning Sequences

The 0.96-inch I2C OLED display directly simulates in-vehicle telematics and roadside V2X signs. Messages transition deterministically according to corridor status:

````carousel
```
+----------------------------+
| [ V2X OLED DISPLAY ]       |
+----------------------------+
| AMBULANCE                  |
| APPROACHING                |
|                            |
| MOVE LEFT                  |
+----------------------------+
```
<!-- slide -->
```
+----------------------------+
| [ V2X OLED DISPLAY ]       |
+----------------------------+
| GREEN                      |
| CORRIDOR                   |
| ACTIVE                     |
| DO NOT BLOCK               |
+----------------------------+
```
<!-- slide -->
```
+----------------------------+
| [ V2X OLED DISPLAY ]       |
+----------------------------+
| POLICE BARRICADE           |
| ROAD CLOSED: INT-2         |
| ROUTE REPLANNED            |
| NEW ETA: 42s               |
+----------------------------+
```
<!-- slide -->
```
+----------------------------+
| [ V2X OLED DISPLAY ]       |
+----------------------------+
| PRIORITY SCHEDULER         |
| PASS: AMB-01               |
| HOLD: AMB-02               |
| EST WAIT: 15.0s            |
+----------------------------+
```
<!-- slide -->
```
+----------------------------+
| [ V2X OLED DISPLAY ]       |
+----------------------------+
| MISSION                    |
| COMPLETE                   |
|                            |
| SIGNALS NORMAL             |
+----------------------------+
```
````

---

### 9. Feature 6: Acoustic Buzzer Preemption Rhythm Patterns

The piezoelectric buzzer operates in an asynchronous background worker thread, ensuring that tone synthesis never blocks real-time network dispatch or safety checks:

```
1. CONTINUOUS TONE (Corridor Active)
   Tone: ▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇▇ (Steady ON)

2. THREE SHORT BEEPS (Intersection 2 Priority Conflict)
   Tone: ▇▇▇       ▇▇▇       ▇▇▇                     (Cycle: 120ms ON, 100ms OFF x3)

3. RAPID BEEPS (Police Road-Closed / Replanning Event)
   Tone: █ █ █ █ █ █ █ █ █ █                         (Cycle: 60ms ON, 60ms OFF x8)

4. TWO LONG BEEPS (Mission Completed at Hospital)
   Tone: ▇▇▇▇▇▇▇▇▇         ▇▇▇▇▇▇▇▇▇                 (Cycle: 450ms ON, 200ms OFF x2)

5. ALTERNATING WARNING (Critical Roadside Fault Fallback)
   Tone: ▇▇▇▇▇▇   ▇▇▇   ▇▇▇▇▇▇   ▇▇▇                 (Cycle: 250ms ON, 100ms OFF, 150ms ON, 100ms OFF)
```

---

### 10. Feature 8: Complete 5-Mission DEMO MODE Workflow

The master demonstration orchestrator (`HardwareDemoOrchestrator`) automates the entire presentation into a structured **5 to 7 minute live demonstration** suitable for hackathon evaluations:

| Mission # | Demonstration Phase | Hardware Action & Stimulus | Observed Actuation & System Response | Target Duration |
| :---: | :--- | :--- | :--- | :---: |
| **1** | **Normal Green Corridor** | Emergency switch pressed near HOME | Route planned (`HOME -> INT_1 -> INT_2 -> INT_3 -> HOSP`). Continuous tone sounds. Relays switch to Green. Sequential IR triggers. 2 long beeps upon hospital arrival. | ~60 sec |
| **2** | **Priority Scheduling** | Simultaneous IR trigger at Intersection 2 (Top & Right) | Priority Scheduler arbitrates. Code Red (Ambulance A) receives immediate green. Code Yellow (Ambulance B) held red for 15s. 3 short beeps sound. OLED displays arbitration. | ~60 sec |
| **3** | **Police Dynamic Replanning** | Police switch pressed while corridor active | Police report barricade at Int 2. Coordinator replans via Bypass with **0s pause**. Int 2 reverts to normal; Int 1, Bypass, Int 3 become Green. Rapid beeps sound. New ETA 42s. | ~75 sec |
| **4** | **Fault Injection & Fail-Safe** | IR sensor stuck-active fault injected at Int 3 | Supervisor flags fault in <500ms. Int 3 shifts to All-Red safe interlock. Alternating warning sounds. Safe degraded corridor maintained. | ~45 sec |
| **5** | **Mission Replay & Telemetry** | Operations Center review command | Digital Twin replays mission telemetry. Displays: 0 accidents, 48% travel time saved, <1.0ms preemption latency. Unified report generated. | ~60 sec |

---

### 11. Judge Demonstration Script & Speaking Points

#### Phase 1: Introduction & Mission Trigger (00:00 - 01:15)
* **Action:** Present the 4-node physical setup. Press the red **Emergency Switch** near HOME.
* **Speaking Points:**
  > *"Judges, what you see before you is SMART AEGIS running natively on a four-node Raspberry Pi 4 cluster under QNX Neutrino RTOS 8.0. There are no cloud dependencies and no microcontrollers—every decision is executed directly in real-time on QNX.
  > When I press this emergency button at our simulated ambulance station, our Coordinator instantly ingests the emergency, assigns a Code Red cardiac triage level, and executes Dijkstra's shortest path routing. 
  > Notice the acoustic preemption buzzer sounding a continuous tone, our roadside OLED displaying real-time V2X warnings, and the physical relay bank pre-clearing traffic lights to green ahead of the vehicle."*

#### Phase 2: Multi-Ambulance Priority Arbitration (01:15 - 02:30)
* **Action:** Trigger both IR sensors simultaneously at Intersection 2 (Top and Right).
* **Speaking Points:**
  > *"Now observe our Priority Scheduling demonstration at Intersection 2. Here, two emergency vehicles are converging simultaneously: Ambulance A from the top arterial with a critical cardiac arrest patient, and Ambulance B from the right with a limb fracture.
  > Instead of causing gridlock or creating a fatal intersection collision, our Coordinator's Priority Scheduler evaluates their triage severity. Ambulance A is granted priority passage, while Ambulance B is safely held at a red signal with an estimated wait time of exactly 15 seconds. 
  > Listen to the three distinct short beeps signaling priority arbitration, and notice our dashboard showing both patient states and arbitration rationale."*

#### Phase 3: Police-Assisted Dynamic Route Replanning (02:30 - 04:00)
* **Action:** Trigger the yellow **Police Override Switch**.
* **Speaking Points:**
  > *"This next scenario showcases one of SMART AEGIS's most critical innovations: Police-Assisted Dynamic Replanning. 
  > In legacy systems, a manual override button halts the green corridor or hands control to an operator. In SMART AEGIS, the Coordinator retains 100% sovereign authority at all times.
  > When a police officer presses this switch to report a road closure or temporary barricade at Intersection 2, the mission does NOT pause. The Coordinator consumes this as verified road intelligence, instantly recalculates a bypass route around the obstruction, and re-routes the green corridor via our bypass arterial. 
  > Notice that Intersection 2 immediately reverts to its normal cycle to clear local congestion, while the new bypass route lights turn green. The entire replanning took less than 2 milliseconds, and our new ETA of 42 seconds is broadcast via rapid acoustic beeps and our OLED display."*

#### Phase 4: Fault Tolerance & Fail-Safe Fallback (04:00 - 05:00)
* **Action:** Trigger sensor stuck fault on Intersection 3.
* **Speaking Points:**
  > *"In smart cities, roadside hardware fails. Here we inject an optical sensor stuck-active fault at Intersection 3. 
  > In less than 500 milliseconds, our QNX health supervisor detects the anomaly, isolates the faulty input, trips the hardware interlock into an All-Red fail-safe condition, and triggers an alternating acoustic warning. The system never deadlocks or exhibits conflicting green indications."*

#### Phase 5: Mission Replay & Evaluation Summary (05:00 - 06:00)
* **Action:** Present the Digital Twin operations dashboard and replay engine.
* **Speaking Points:**
  > *"Finally, our read-only Digital Twin captures full telemetry across every millisecond of the mission. We have demonstrated a 48% reduction in emergency transit time, zero accidents, deterministic sub-millisecond preemption latency, and full QNX RTOS reliability. Thank you, and we welcome your questions."*

---

### 12. Photographic & Field Demonstration Placeholders

```
+-----------------------------------------------------------------------------------+
|                           [ PHOTO PLACEHOLDER 1 ]                                 |
|                                                                                   |
|                   COMPLETE 4-NODE RASPBERRY PI 4 CLUSTER SETUP                    |
|       Showing Coordinator Pi, Intersections 1, 2, 3, Relay Banks, and Power Hub   |
+-----------------------------------------------------------------------------------+

+-----------------------------------------------------------------------------------+
|                           [ PHOTO PLACEHOLDER 2 ]                                 |
|                                                                                   |
|                 INTERSECTION 2 ACTUATOR BANK & DUAL IR SENSORS                    |
|         Showing Dual-Approach Signal Lamps & Optocoupled Relay Interlocks         |
+-----------------------------------------------------------------------------------+

+-----------------------------------------------------------------------------------+
|                           [ PHOTO PLACEHOLDER 3 ]                                 |
|                                                                                   |
|                 COORDINATOR CONTROL CONSOLE & V2X OLED DISPLAY                    |
|        Showing Tactile Emergency Switch, Police Switch, and OLED Warnings         |
+-----------------------------------------------------------------------------------+
```

---

### 13. Pre-Deployment & Demonstration Checklists

#### 13.1 Pre-Deployment Hardware Checklist
- [ ] Ensure all 4 Raspberry Pi boards have QNX Neutrino RTOS 8.0 flashed on SanDisk Extreme Pro MicroSD cards.
- [ ] Verify 5V 10A power supply outputs stable 5.15V DC under full relay coil load.
- [ ] Connect common ground (`GND`) across all Raspberry Pis, relay modules, and sensor grounds.
- [ ] Verify optocoupled relay jumpers are set to `LOW` trigger logic for active-low operation.
- [ ] Validate optical IR sensor potentiometer sensitivity (active LED lights up at 5–15 cm approach distance).
- [ ] Verify static IP assignments: `192.168.1.100` (Coord), `.101` (Int 1), `.102` (Int 2), `.103` (Int 3).
- [ ] Test bidirectional `ping` across all 4 nodes (< 1.5 ms round-trip latency).

#### 13.2 Live Demonstration Checklist
- [ ] Execute `make test` to verify all 40+ test suites pass 100%.
- [ ] Check OLED initialization display (`[ V2X ADVISORY ] SMART AEGIS RTOS READY`).
- [ ] Verify Buzzer single test chirp on coordinator power-up.
- [ ] Ensure tactile switches are free of mechanical sticking.
- [ ] Confirm Operations Center terminal dashboard is visible to judges.
- [ ] Set demo orchestrator stage delay to 1500ms for comfortable audience visibility.

#### 13.3 Troubleshooting Checklist
| Symptom | Probable Cause | Immediate Remediation |
| :--- | :--- | :--- |
| **Relay coil clicks but lamp does not illuminate** | 12V auxiliary power disconnected or blown fuse | Check 12V DC auxiliary lamp power rail and relay terminal screw tightness. |
| **Conflicting green indications observed** | Software bug or damaged relay contact | Hardware interlock will automatically trip All-Red within 5ms. Check `relayDriver->ResetInterlock()`. |
| **IR sensor constantly triggers (Stuck-Active)** | Ambient optical sunlight or optical crosstalk | Adjust LM393 trimmer counter-clockwise; shield photodiode from direct bench lighting. |
| **OLED display remains blank** | I2C address mismatch or loose SDA/SCL wire | Verify I2C wiring (SDA to Pin 3, SCL to Pin 5). Ensure pull-up resistors are energized. |
| **Police switch does not trigger replanning** | Contact debounce window too long | Check `PoliceOverrideSwitchConfig::debounceMs` (default 50ms). Inspect switch terminal continuity. |
| **Node heartbeat timeout flagged** | Wi-Fi packet drop or high interference | Switch to wired Ethernet switch for hackathon presentation environments. |

---

### 14. Engineering Verification & Conclusion

Phase 12 elevates SMART AEGIS from an advanced algorithmic and software framework into a proven, reliable, and visually compelling physical demonstration. By enforcing strict adherence to real-time deterministic software engineering, zero-pause police-assisted dynamic replanning, and full hardware-software abstraction, SMART AEGIS stands ready for hackathon demonstration, incubation evaluation, and municipal smart city deployment.
