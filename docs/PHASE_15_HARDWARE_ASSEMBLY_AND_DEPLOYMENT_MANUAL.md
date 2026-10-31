# SMART AEGIS (AERS) — Phase 15 Engineering Manual
## Physical Hardware Assembly, Direct GPIO Prototype LED Mapping, Raspberry Pi Deployment & Commissioning Handbook

```
===================================================================================================
  PROJECT:         SMART AEGIS (Automated Emergency Routing System)
  DOCUMENT ID:     AEGIS-MAN-HW-2026-PHASE15
  VERSION:         2.5.0
  DATE:            2026-10-31
  TARGET RTOS:     QNX Neutrino RTOS 8.0 Starter Kit (aarch64le)
  TARGET HARDWARE: 4x Raspberry Pi 4 Model B (4 GB RAM, Quad Cortex-A72 @ 1.5 GHz)
  CLASSIFICATION:  Physical Hardware Assembly, Wiring, Deployment & Commissioning Handbook
  AUDIENCE:        Hackathon Judges, Incubation Centre Reviewers, Field Engineers & Operators
===================================================================================================
```

---

### Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Hardware Overview](#2-hardware-overview)
3. [Raspberry Pi Cluster Architecture](#3-raspberry-pi-cluster-architecture)
4. [Complete GPIO Mapping Tables](#4-complete-gpio-mapping-tables)
5. [Full Wiring Diagrams](#5-full-wiring-diagrams)
6. [Power Distribution](#6-power-distribution)
7. [Prototype Direct GPIO LED Wiring & Circuit Interfacing](#7-prototype-direct-gpio-led-wiring--circuit-interfacing)
8. [IR Sensor Wiring](#8-ir-sensor-wiring)
9. [OLED Wiring](#9-oled-wiring)
10. [Emergency Switch Wiring](#10-emergency-switch-wiring)
11. [Police Override Wiring](#11-police-override-wiring)
12. [Buzzer Driver Circuit](#12-buzzer-driver-circuit)
13. [Cable Schedule](#13-cable-schedule)
14. [Connector Tables](#14-connector-tables)
15. [Network Architecture](#15-network-architecture)
16. [Bench Assembly Procedure](#16-bench-assembly-procedure)
17. [Raspberry Pi Deployment Procedure](#17-raspberry-pi-deployment-procedure)
18. [SCP / SSH Deployment Workflow](#18-scp--ssh-deployment-workflow)
19. [Git Pull / Software Update Procedure](#19-git-pull--software-update-procedure)
20. [Startup Sequence](#20-startup-sequence)
21. [Commissioning Procedure](#21-commissioning-procedure)
22. [Hardware Bring-up](#22-hardware-bring-up)
23. [GPIO Verification](#23-gpio-verification)
24. [Prototype Direct GPIO LED Verification](#24-prototype-direct-gpio-led-verification)
25. [IR Sensor Verification](#25-ir-sensor-verification)
26. [OLED Verification](#26-oled-verification)
27. [Buzzer Verification](#27-buzzer-verification)
28. [Switch Verification](#28-switch-verification)
29. [TCP Communication Verification](#29-tcp-communication-verification)
30. [Cluster Formation Verification](#30-cluster-formation-verification)
31. [Hardware Acceptance Test](#31-hardware-acceptance-test)
32. [Live Demonstration Procedure](#32-live-demonstration-procedure)
33. [Troubleshooting Guide](#33-troubleshooting-guide)
34. [Preventive Maintenance](#34-preventive-maintenance)
35. [Safety Precautions](#35-safety-precautions)
36. [Future Expansion Possibilities](#36-future-expansion-possibilities)
37. [Final Deployment Checklist](#37-final-deployment-checklist)
38. [Appendix & Document Metadata](#38-appendix--document-metadata)

---

### 1. Executive Summary

Urban emergency transit systems face critical bottlenecks due to static traffic light cycles, dense vehicular queues, and delayed hospital communications. Ambulances frequently lose life-saving minutes navigating signalized intersections or encountering unannounced roadside hazards.

**SMART AEGIS** (**Automated Emergency Routing & Smart Traffic Corridor Preemption System**) solves this challenge through an RTOS-native multi-node edge computing platform. Built upon the hard real-time **QNX Neutrino RTOS 8.0** microkernel running on a cluster of four **Raspberry Pi 4 Model B** single-board computers, SMART AEGIS establishes an intelligent, preemptive green wave corridor that dynamically reserves intersections, arbitrates simultaneous emergency priorities, reroutes around obstacles with zero mission pause, and provides vehicle-to-everything (V2X) visual warnings.

**Phase 15** delivers the physical realization of this architecture for final hackathon deployment:
- **Zero Software Redesign:** All Coordinator intelligence, Dijkstra route planning, green corridor scheduling, priority arbitration, hospital selection, Digital Twin telemetry, and Command Center monitoring logic remain 100% intact.
- **Direct GPIO Prototype LED Architecture:** Relays and 3-light traffic modules are completely eliminated. In their place, direct Raspberry Pi GPIO LED control with individual $330\,\Omega$ current-limiting resistors is deployed across all intersections.
- **Node-Specific Hardware Allocations:**
  - **Coordinator (Raspberry Pi 1):** Zero LEDs connected. Dedicated entirely to supervisory orchestration, graph routing, and multi-node TCP/IP communication.
  - **Intersection 1 (Raspberry Pi 2):** Exactly TWO LEDs: Red LED on BCM GPIO 5 (Pin 29) and Blue LED on BCM GPIO 6 (Pin 31).
  - **Intersection 2 (Raspberry Pi 3):** Exactly TWO LEDs: Red LED on BCM GPIO 5 (Pin 29) and Blue LED on BCM GPIO 6 (Pin 31).
  - **Intersection 3 (Raspberry Pi 4):** Due to prototype hardware availability constraints, exactly ONE LED: Blue LED on BCM GPIO 6 (Pin 31). Zero Red LED connected.
- **Demonstration LED State Transitions:**
  - **Normal Traffic Operation:** Intersection 1 & 2: Blue OFF, Red OFF. Intersection 3: Blue OFF.
  - **Emergency Path Granted:** Intersection 1 & 2: Blue ON, Red OFF. Intersection 3: Blue ON.
  - **Emergency Path Finished / Corridor Closed:** Intersection 1, 2, 3: Blue OFF (and Red OFF).

```
+---------------------------------------------------------------------------------------------------+
|                                  CORE ENGINEERING GUARANTEES                                      |
+---------------------------------------------------------------------------------------------------+
| 1. ZERO SOFTWARE REDESIGN: Phases 1-13 software core is 100% frozen, untouched, and guaranteed.   |
| 2. CONFLICT-FREE GPIO: All BCM pins verified for electrical logic, drive, and QNX compatibility.  |
| 3. NO LINUX APIS: Native QNX SDP 8.0 POSIX drivers only. Zero wiringPi, pigpio, or sysfs calls.   |
| 4. SOLID-STATE DIRECT DRIVE: Microsecond LED response with 330 Ohm current limiters.               |
| 5. SINGLE 5.1V DC RAIL: No 12V supplies, electromechanical relays, or inductive transients.       |
| 6. AUDITABLE & REPEATABLE: Step-by-step procedures allow full assembly from an empty workbench.   |
+---------------------------------------------------------------------------------------------------+
```

---

### 2. Hardware Overview

The SMART AEGIS physical demonstration system comprises industrial-grade compute, solid-state optical indicators, optical detection, and communication hardware:

| Subsystem | Component Description | Model / Specification | Qty | Target Node | Functional Purpose |
| :--- | :--- | :--- | :---: | :--- | :--- |
| **Compute** | Single-Board Computer | Raspberry Pi 4 Model B (4 GB LPDDR4, Quad Cortex-A72 @ 1.5 GHz) | 4 | Master Cluster | Distributed QNX Neutrino RTOS 8.0 nodes (1 Coordinator, 3 Intersections) |
| **Storage** | MicroSD Card | SanDisk Extreme Pro 32GB Class 10 A2 U3 | 4 | Nodes 1, 2, 3, 4 | Microkernel IFS boot image, configuration, and binaries |
| **Power Logic** | Regulated Logic PSU | Mean Well LRS-75-5 (5.10V DC @ 14.0A Regulated) | 1 | Master Power | Powers 4x Raspberry Pi boards and all connected peripherals |
| **Power Connect** | High-Current USB Cables | 20 AWG USB Type-A to USB Type-C (1.0m, 3A rated) | 4 | Nodes 1, 2, 3, 4 | Clean DC power delivery from 5V busbar to Raspberry Pi power input ports |
| **Indicators (Red)**| 5mm High-Efficiency Red LED | 5mm Diffused Red LED ($V_f \approx 1.8\text{V}, 20\text{mA}$) + 330R | 2 | Nodes 2, 3 | Stop / Hold indication (Node 2: Pin 29, Node 3: Pin 29) |
| **Indicators (Blue)**| 5mm High-Efficiency Blue LED | 5mm Clear Blue LED ($V_f \approx 2.8\text{V}, 20\text{mA}$) + 330R | 3 | Nodes 2, 3, 4 | Emergency green wave corridor indication (Pin 31 across Nodes 2, 3, 4) |
| **Perception** | Optical IR Proximity Sensors | LM393 Optical Reflection Detector (Adjustable Potentiometer) | 3 | Nodes 2, 3, 4 | Optical vehicle detection (Int 1: Pin 32, Int 2: Pin 32, Int 3: Pin 32) |
| **Operator HMI** | Emergency Push Button | Heavy-Duty Industrial Momentary Push Button (SPST-NO, 22mm) | 1 | Coordinator | Physical trigger at HOME dispatch station for mission launch |
| **Operator HMI** | Police Override Switch | Toggle Switch (SPST Maintained Contact, Panel Mount) | 1 | Coordinator | Simulates verified police advisory for zero-pause dynamic rerouting |
| **V2X Telemetry** | In-Vehicle OLED Display | 0.96" I2C Monochrome Display (SSD1306, 128x64 pixels, 3.3V) | 1 | Coordinator | Driver advisory display rendering real-time corridor & routing status |
| **Acoustics** | High-Decibel Piezo Buzzer | 5V Active Electromagnetic Buzzer (2.7 kHz resonant frequency) | 1 | Coordinator | Acoustic corridor state alerts (5 distinct cadence patterns) |
| **Transistor Buffer**| BJT Driver Circuit | 2N2222A NPN BJT + 1.0k Ohm base resistor + 1N4148 diode | 1 | Coordinator | Amplifies 3.3V GPIO to 5V rail for buzzer drive |
| **Network** | Ethernet Switch / AP | TP-Link 5-Port Gigabit Switch / 5GHz 802.11ac Wi-Fi Gateway | 1 | Master Cluster | Sub-millisecond TCP/IP communication fabric (192.168.1.0/24) |
| **Cabling** | Breadboard / Dupont Jumpers | 24 AWG Female-to-Female & Male-to-Female Dupont Wire (20cm) | 40 | Master Cluster | Point-to-point interconnects between GPIO headers and peripherals |

---

### 3. Raspberry Pi Cluster Architecture

The physical layout mirrors a real-world municipal emergency route extending from an emergency dispatch depot to a regional trauma center:

```
+---------------------------------------------------------------------------------------------------+
|                                  PHYSICAL CORRIDOR ROAD TOPOLOGY                                  |
+---------------------------------------------------------------------------------------------------+

   [ HOME STATION ]  (RPi 1: Master Coordinator - 192.168.1.100) — ZERO LEDS CONNECTED
         |
         |  Arterial Segment 1 (Speed: 50 km/h, Nominal Transit: 12.0s)
         v
   [ INTERSECTION 1 ] (RPi 2: Traffic Node 1 - 192.168.1.101) — TWO LEDS
     - Red Stop LED (BCM 5, Pin 29)
     - Blue Corridor LED (BCM 6, Pin 31)
     - Approach Optical IR Proximity Sensor (BCM 12, Pin 32)
         |
         |  Arterial Segment 2 (Speed: 50 km/h, Nominal Transit: 14.5s)
         v
   [ INTERSECTION 2 ] (RPi 3: Traffic Node 2 - 192.168.1.102) [PRIORITY ARBITRATION HUB] — TWO LEDS
     - Red Stop LED (BCM 5, Pin 29)
     - Blue Corridor LED (BCM 6, Pin 31)
     - Approach Optical IR Proximity Sensor (BCM 12, Pin 32)
         |
         |  Arterial Segment 3 (Speed: 50 km/h, Nominal Transit: 11.0s)
         v
   [ INTERSECTION 3 ] (RPi 4: Traffic Node 3 - 192.168.1.103) — ONE LED (HARDWARE SHORTAGE)
     - Blue Corridor LED (BCM 6, Pin 31)
     - (Zero Red LED Connected)
     - Hospital Gate Optical IR Sensor (BCM 12, Pin 32)
         |
         v
   [ TRAUMA HOSPITAL ] (Terminal Destination & Emergency Department Handover)
+---------------------------------------------------------------------------------------------------+
```

#### Node Allocation & Responsibilities Table

| Node Identifier | Physical Hardware | Static IP Address | Port | Role & Executable | Primary Responsibilities |
| :--- | :--- | :--- | :---: | :--- | :--- |
| **`RPI_COORD`** | Raspberry Pi 4 (4GB) | `192.168.1.100` | `9000` | Master Coordinator<br>`aegis_coordinator` | Mission intake, Dijkstra graph solver, green corridor reservation, priority arbitration, V2X OLED rendering, acoustic buzzer sequencing. **Zero LEDs connected**. |
| **`INT_1`** | Raspberry Pi 4 (4GB) | `192.168.1.101` | `9001` | Traffic Node 1<br>`aegis_traffic_signal_node` | Intersection 1 signal actuation, direct GPIO LED control (Red Pin 29, Blue Pin 31), optical vehicle sensing, keepalive heartbeat streaming. |
| **`INT_2`** | Raspberry Pi 4 (4GB) | `192.168.1.102` | `9002` | Traffic Node 2<br>`aegis_traffic_signal_node` | Intersection 2 Priority Hub, multi-approach conflict resolution, direct GPIO LED control (Red Pin 29, Blue Pin 31), dynamic detour handling. |
| **`INT_3`** | Raspberry Pi 4 (4GB) | `192.168.1.103` | `9003` | Traffic Node 3<br>`aegis_traffic_signal_node` | Intersection 3 Hospital Gate, terminal corridor clearance, direct GPIO LED control (Blue Pin 31 only; zero Red LED), hospital arrival optical verification. |

---

### 4. Complete GPIO Mapping Tables

#### 4.1 Master Cluster GPIO Allocation Table

All 13 active signals mapped across the four Raspberry Pi 4 Model B headers are verified to be conflict-free:

```
+---------------------------------------------------------------------------------------------------+
|                                     MASTER GPIO MAPPING TABLE                                     |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| Symbolic Identifier      | Node        | BCM GPIO | Header   | Direction | Voltage Level | Active Pol |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| HOME_EMERGENCY_SWITCH    | Coordinator | BCM 23   | Pin 16   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| POLICE_OVERRIDE_SWITCH   | Coordinator | BCM 24   | Pin 18   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| HOME_BUZZER              | Coordinator | BCM 6    | Pin 31   | OUTPUT    | 3.3V -> BJT   | ACTIVE_HIGH|
| V2X_OLED_SDA             | Coordinator | BCM 2    | Pin 3    | BIDIR     | 3.3V Open-Dr  | I2C DATA   |
| V2X_OLED_SCL             | Coordinator | BCM 3    | Pin 5    | OUTPUT    | 3.3V Open-Dr  | I2C CLOCK  |
| (NO LEDS CONNECTED)      | Coordinator | —        | —        | —         | —             | —          |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT1_LED_RED             | Node 1      | BCM 5    | Pin 29   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT1_LED_BLUE            | Node 1      | BCM 6    | Pin 31   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT1_APPROACH_IR         | Node 1      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| (GND RETURN)             | Node 1      | Ground   | Pin 30   | RETURN    | 0V Reference  | —          |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT2_LED_RED             | Node 2      | BCM 5    | Pin 29   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT2_LED_BLUE            | Node 2      | BCM 6    | Pin 31   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT2_APPROACH_IR         | Node 2      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| (GND RETURN)             | Node 2      | Ground   | Pin 30   | RETURN    | 0V Reference  | —          |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT3_LED_BLUE            | Node 3      | BCM 6    | Pin 31   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| (ZERO RED LED CONNECTED) | Node 3      | —        | —        | —         | —             | —          |
| INT3_APPROACH_IR         | Node 3      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| (GND RETURN)             | Node 3      | Ground   | Pin 30   | RETURN    | 0V Reference  | —          |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
```

---

### 5. Full Wiring Diagrams

#### 5.1 Coordinator Station Schematic (RPi 1)

```
+-------------------------------------------------------------+
|              COORDINATOR STATION ELECTRICAL SCHEMATIC        |
|              (ZERO LEDS CONNECTED — PURE TCP ENGINE)        |
+-------------------------------------------------------------+
   Raspberry Pi 4 Model B (Node 1 - Master Coordinator)
   
   [Pin 1 ] +3.3V DC ---------------------> OLED VCC
   [Pin 3 ] BCM 2 (I2C-1 SDA) ============> OLED SDA
   [Pin 5 ] BCM 3 (I2C-1 SCL) ============> OLED SCL
   [Pin 9 ] Ground (GND) -----------------> OLED GND & Switch Common GND
   
   [Pin 16] BCM 23 (EMERGENCY_SW) <-------- Push Button (NO) <---+
                                                                 |
   [Pin 18] BCM 24 (POLICE_SW)    <-------- Toggle Switch    <---+
                                                                 |
   [Pin 14] Ground (GND) ----------------------------------------+
   
   [Pin 2 ] +5.0V DC Rail ----------------> Piezo Buzzer (+)
   [Pin 31] BCM 6 (BUZZER_GATE) ----------> [ 1k Resistor ] -> Base 2N2222A
                                            Collector --------> Piezo Buzzer (-)
                                            Emitter ----------> Ground (Pin 39)
+-------------------------------------------------------------+
```

#### 5.2 Traffic Intersection 1 & 2 Schematic (RPi 2, RPi 3)

```
+-------------------------------------------------------------+
|       TRAFFIC INTERSECTIONS 1 & 2 ELECTRICAL SCHEMATIC      |
|       (EXACTLY TWO LEDS: RED PIN 29 + BLUE PIN 31)         |
+-------------------------------------------------------------+
   Raspberry Pi 4 Model B (Nodes 2 & 3 - Intersections 1 & 2)
   
   [Pin 1 ] +3.3V DC ---------------------> LM393 IR Sensor VCC
   [Pin 9 ] Ground (GND) -----------------> LM393 IR Sensor GND
   [Pin 32] BCM 12 (APPROACH_IR) <-------- LM393 IR Sensor OUT
   
   DIRECT GPIO PROTOTYPE LEDS:
   [Pin 29] BCM 5 (LED_RED)   -----------> [ 330R Resistor ] ---> Anode Red LED   ---+
                                                                                     |
   [Pin 31] BCM 6 (LED_BLUE)  -----------> [ 330R Resistor ] ---> Anode Blue LED  ---+
                                                                                     |
   [Pin 30] Ground (GND)      <--------------------------------- Common Cathode -----+
+-------------------------------------------------------------+
```

#### 5.3 Traffic Intersection 3 Schematic (RPi 4)

```
+-------------------------------------------------------------+
|           TRAFFIC INTERSECTION 3 ELECTRICAL SCHEMATIC       |
|      (ONE LED ONLY: BLUE PIN 31 — ZERO RED LED CONNECTED)   |
+-------------------------------------------------------------+
   Raspberry Pi 4 Model B (Node 4 - Intersection 3 Hospital Gate)
   
   [Pin 1 ] +3.3V DC ---------------------> LM393 IR Sensor VCC
   [Pin 9 ] Ground (GND) -----------------> LM393 IR Sensor GND
   [Pin 32] BCM 12 (APPROACH_IR) <-------- LM393 IR Sensor OUT
   
   DIRECT GPIO PROTOTYPE LED:
   [Pin 31] BCM 6 (LED_BLUE)  -----------> [ 330R Resistor ] ---> Anode Blue LED  ---+
   (Red LED: NOT CONNECTED — Hardware Shortage Specification)                       |
                                                                                     |
   [Pin 30] Ground (GND)      <--------------------------------- Cathode Return -----+
+-------------------------------------------------------------+
```

---

### 6. Power Distribution

#### 6.1 Power Tree Architecture

A single industrial-grade Mean Well LRS-75-5 regulated power supply energizes all four single-board computers and all connected solid-state peripherals:

```
===================================================================================================
                               SMART AEGIS POWER TREE ARCHITECTURE
===================================================================================================

  110V/230V AC Mains
          |
   [ Inline 3A Fuse & Master Switch ]
          |
          v
   [ Mean Well LRS-75-5 Regulated PSU ]
     Output: 5.10V DC +/- 0.05V @ 14.0A Max (71.4W)
          |
          +--------------------------------------+--------------------------------------+
          |                                      |                                      |
          v                                      v                                      v
   [ 5V Distribution Busbar ]          [ Master Star Ground ]                 [ Dual 5V Fans ]
          |                                      |                                      |
   +------+------+------+------+          +------+------+------+------+                 v
   |      |      |      |      |          |      |      |      |      |            Chassis Cooling
   v      v      v      v      v          v      v      v      v      v
  Pi 1   Pi 2   Pi 3   Pi 4   Aux 5V     Pi 1   Pi 2   Pi 3   Pi 4   Aux GND
  USB-C  USB-C  USB-C  USB-C  Buzzer     GND    GND    GND    GND    Buzzer
   |      |      |      |
  OLED   LEDs   LEDs   LED
  Disp   Int1   Int2   Int3
===================================================================================================
```

#### 6.2 Current Consumption & Power Budget

| Subsystem Branch | Voltage | Nominal Current | Peak Current | Total Power | Sizing Factor |
| :--- | :---: | :---: | :---: | :---: | :---: |
| 4x Raspberry Pi 4 Model B (CPU loaded)| 5.1V DC | 4 x 0.85 A = 3.40 A | 4 x 2.50 A = 10.00 A | 51.0 W | Peak boot/burst load |
| 5x Prototype LEDs (with 330R) | 3.3V DC | 5 x 0.003 A = 0.015 A | 5 x 0.005 A = 0.025 A | 0.08 W | All LEDs lit concurrent |
| 3x LM393 Optical IR Sensors | 3.3V DC | 3 x 0.015 A = 0.045 A | 3 x 0.020 A = 0.060 A | 0.20 W | Active beam detect |
| 0.96" I2C SSD1306 OLED Display | 3.3V DC | 0.02 A | 0.035 A | 0.12 W | Full pixel display |
| Acoustic Buzzer Driver Circuit | 5.0V DC | 0.00 A (idle) | 0.040 A | 0.20 W | 2.7 kHz resonant tone |
| Dual 40mm 5V Chassis Fans | 5.0V DC | 0.16 A | 0.240 A | 1.20 W | Continuous cooling |
| **TOTAL LOAD BUDGET** | — | **3.64 A** | **10.40 A** | **52.80 W** | **PSU Capacity: 14.0 A (71.4W)** |

> [!NOTE]
> The Mean Well LRS-75-5 delivers 14.0A continuous output, providing a **34.6% safety margin** ($3.60\text{A}$ headroom) above absolute maximum peak cluster consumption.

---

### 7. Prototype Direct GPIO LED Wiring & Circuit Interfacing

#### 7.1 Direct Solid-State GPIO LED Control Overview

In place of bulky electromechanical relays or 3-light traffic lamp modules, the SMART AEGIS prototype utilizes direct GPIO LED control with individual $330\,\Omega$ current-limiting resistors:

- **Red Stop LED (`LED_RED`):** BCM 5 (Physical Header Pin 29). Present on Node 2 (Int 1) and Node 3 (Int 2).
- **Blue Corridor LED (`LED_BLUE`):** BCM 6 (Physical Header Pin 31). Present on Node 2 (Int 1), Node 3 (Int 2), and Node 4 (Int 3).
- **Common Ground (`GND`):** Physical Header Pin 30, directly adjacent to Pins 29 and 31.

#### 7.2 LED Electrical Circuit & Current Limiting

Each discrete LED connects directly between a Raspberry Pi GPIO output pin and Ground, with an inline $330\,\Omega \pm 5\%$ current-limiting resistor:

```
   Raspberry Pi 4 Header Pin                          Direct Prototype LED Circuit
   
   Pin 29 (BCM 5) [OUTPUT HIGH: 3.3V] -----> [ 330 Ohm ] ----->| (RED LED)   ----+
                                                                                  |
   Pin 31 (BCM 6) [OUTPUT HIGH: 3.3V] -----> [ 330 Ohm ] ----->| (BLUE LED)  ----+
                                                                                  |
   Pin 30 (Ground: 0V)                <-------------------------------------------+
```

- **Red LED Current:** $I_{\text{red}} = (3.3\text{V} - 1.8\text{V}) / 330\,\Omega \approx 4.5\text{ mA}$.
- **Blue LED Current:** $I_{\text{blue}} = (3.3\text{V} - 2.8\text{V}) / 330\,\Omega \approx 1.5\text{ mA}$.
- **Total Combined Current:** $< 6.0\text{ mA}$ per node, far below the Broadcom BCM2711 maximum pin drive limit ($16\text{ mA}$).

#### 7.3 Demonstration LED Operational States

The prototype LEDs visually demonstrate green wave preemption with crystal clarity:

| Operational State | Intersection 1 (RPi 2) | Intersection 2 (RPi 3) | Intersection 3 (RPi 4) | Audible Buzzer (RPi 1) |
| :--- | :---: | :---: | :---: | :---: |
| **Normal Traffic Operation** | Blue OFF, Red OFF | Blue OFF, Red OFF | Blue OFF | Silent |
| **Emergency Path Granted (Green Wave)** | **Blue ON**, Red OFF | **Blue ON**, Red OFF | **Blue ON** | Preemption Cadence |
| **Emergency Path Finished (Corridor Closed)** | Blue OFF, Red OFF | Blue OFF, Red OFF | Blue OFF | Single Confirmation |

---

### 8. IR Sensor Wiring

```
+---------------------------------------------------------------------------------------------------+
|                              LM393 OPTICAL IR SENSOR WIRING DIAGRAM                               |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 4 Header Pin               LM393 Proximity Sensor Module
  
  [Pin 1 ] +3.3V Power Rail     --------> [ VCC ] Supply Input (3.3V DC)
  [Pin 9 ] Ground (GND)         --------> [ GND ] Ground Return
  [Pin 32] BCM 12 (APPROACH_IR) <-------- [ OUT ] Digital Open-Collector Output (Active-Low)
+---------------------------------------------------------------------------------------------------+
```

- **Power Supply:** Powered strictly from the Raspberry Pi $3.3\text{V}$ rail (Pin 1) to guarantee $3.3\text{V}$ LVTTL logic levels.
- **Threshold Calibration:** Potentiometer adjusted so test obstacle trips beam at $5\text{ cm} - 10\text{ cm}$.
- **Debouncing:** QNX driver executes a non-blocking $30\text{ ms}$ debouncing filter to eliminate false triggers from optical noise.

---

### 9. OLED Wiring

```
+---------------------------------------------------------------------------------------------------+
|                                 SSD1306 I2C OLED DISPLAY WIRING                                   |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 1 Header Pin               0.96" SSD1306 OLED Module
  
  [Pin 1] +3.3V Power Rail      --------> [ VCC ] Logic & Panel Power
  [Pin 9] Ground (GND)          --------> [ GND ] Ground Return
  [Pin 3] BCM 2 (I2C-1 SDA)     <=======> [ SDA ] Serial Data Line (3.3V Open-Drain)
  [Pin 5] BCM 3 (I2C-1 SCL)     --------> [ SCL ] Serial Clock Line (400 kHz Fast Mode)
+---------------------------------------------------------------------------------------------------+
```

- **I2C Address:** Hardware default `0x3C`.
- **Clock Frequency:** $400\text{ kHz}$ Fast Mode supported natively by QNX I2C resource manager.

---

### 10. Emergency Switch Wiring

```
+---------------------------------------------------------------------------------------------------+
|                             HOME EMERGENCY PUSH BUTTON WIRING                                     |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 1 Header Pin               Heavy-Duty Push Button (SPST-NO)
  
  [Pin 16] BCM 23 (EMERGENCY_SW) -------> [ Terminal 1 ]
                                                         [ Normally Open Contact ]
  [Pin 14] Ground (GND)          <------- [ Terminal 2 ]
+---------------------------------------------------------------------------------------------------+
```

- **Electrical Logic:** Active-Low with internal pull-up resistor ($50\text{ k}\Omega$).
- **Software Debounce:** $50\text{ ms}$ validation window guarantees clean single-event dispatch intake.

---

### 11. Police Override Wiring

```
+---------------------------------------------------------------------------------------------------+
|                             POLICE DYNAMIC REPLANNING OVERRIDE WIRING                             |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 1 Header Pin               Police Override Toggle Switch (SPST)
  
  [Pin 18] BCM 24 (POLICE_SW) ----------> [ Terminal 1 ]
                                                         [ Maintained Contact ]
  [Pin 20] Ground (GND)       <----------> [ Terminal 2 ]
+---------------------------------------------------------------------------------------------------+
```

- **Operational Role:** Simulates a verified police traffic barricade report on arterial segment 1-2.
- **Zero-Pause Replanning:** Ingested by the Coordinator graph engine to trigger dynamic A* detour recalculation with zero mission pause.

---

### 12. Buzzer Driver Circuit

```
+---------------------------------------------------------------------------------------------------+
|                                ACOUSTIC BUZZER DRIVER CIRCUIT                                     |
+---------------------------------------------------------------------------------------------------+
  [Pin 2] +5V Power Rail ------+
                               |
                             +---+
                             | + | Active 5V Buzzer (2.7 kHz resonant)
                             | - |
                             +---+
                               |
                               +------> Collector (2N2222A NPN Transistor)
                                         Base <---- [ 1k Resistor ] <---- [Pin 31] BCM 6 (BUZZER)
                                       Emitter
                                         |
  [Pin 39] Ground (GND) <----------------+
+---------------------------------------------------------------------------------------------------+
```

---

### 13. Complete Cable Schedule

```
+---------------------------------------------------------------------------------------------------+
|                                      COMPLETE CABLE SCHEDULE                                      |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| Wire ID | Origin           | Destination      | Signal Name           | Gauge | Color  | Term Type|
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-101   | RPi 1 Pin 16     | Emergency SW T1  | HOME_EMERGENCY_SWITCH | 22AWG | Blue   | Spade    |
| W-102   | Emergency SW T2  | RPi 1 Pin 14     | GND_LOGIC             | 22AWG | Black  | Spade    |
| W-103   | RPi 1 Pin 18     | Police SW T1     | POLICE_OVERRIDE_SWITCH| 22AWG | Yellow | Ferrule  |
| W-104   | Police SW T2     | RPi 1 Pin 20     | GND_LOGIC             | 22AWG | Black  | Ferrule  |
| W-105   | RPi 1 Pin 31     | BJT Base 1k Res  | HOME_BUZZER           | 24AWG | Orange | Solder   |
| W-106   | RPi 1 Pin 3      | OLED Pin SDA     | V2X_OLED_SDA          | 26AWG | Green  | Dupont F |
| W-107   | RPi 1 Pin 5      | OLED Pin SCL     | V2X_OLED_SCL          | 26AWG | White  | Dupont F |
| W-108   | RPi 1 Pin 1      | OLED Pin VCC     | +3.3V_OLED            | 26AWG | Red    | Dupont F |
| W-109   | RPi 1 Pin 9      | OLED Pin GND     | GND_OLED              | 26AWG | Black  | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-201   | RPi 2 Pin 29     | Red LED Anode    | INT1_LED_RED (330R)   | 24AWG | Red    | Dupont F |
| W-202   | RPi 2 Pin 31     | Blue LED Anode   | INT1_LED_BLUE (330R)  | 24AWG | Blue   | Dupont F |
| W-203   | RPi 2 Pin 30     | LED Cathodes     | GND_INT1              | 24AWG | Black  | Dupont F |
| W-204   | RPi 2 Pin 32     | IR Sensor 1 OUT  | INT1_APPROACH_IR      | 24AWG | Purple | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-301   | RPi 3 Pin 29     | Red LED Anode    | INT2_LED_RED (330R)   | 24AWG | Red    | Dupont F |
| W-302   | RPi 3 Pin 31     | Blue LED Anode   | INT2_LED_BLUE (330R)  | 24AWG | Blue   | Dupont F |
| W-303   | RPi 3 Pin 30     | LED Cathodes     | GND_INT2              | 24AWG | Black  | Dupont F |
| W-304   | RPi 3 Pin 32     | IR Sensor 2 OUT  | INT2_APPROACH_IR      | 24AWG | Purple | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-401   | RPi 4 Pin 31     | Blue LED Anode   | INT3_LED_BLUE (330R)  | 24AWG | Blue   | Dupont F |
| W-402   | RPi 4 Pin 30     | Blue LED Cathode | GND_INT3              | 24AWG | Black  | Dupont F |
| W-403   | RPi 4 Pin 32     | IR Sensor 3 OUT  | INT3_APPROACH_IR      | 24AWG | Purple | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| PWR-01  | 5V PSU Output +  | RPi 1-4 USB-C    | +5.10V_MAIN           | 20AWG | Red    | USB-C    |
| PWR-02  | 5V PSU Output -  | Star Ground Bus  | GND_MAIN              | 20AWG | Black  | Ferrule  |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
```

---

### 14. Connector Tables

```
+---------------------------------------------------------------------------------------------------+
|                        CONNECTOR & TERMINAL BLOCK ALLOCATION TABLES                               |
+---------------------------------------------------------------------------------------------------+
  5.1V DC LOGIC DISTRIBUTION BUSBAR (DIN RAIL MOUNTED)
  Terminal #    Signal Name           Wire Gauge    Destination Peripheral
  -------------------------------------------------------------------------------------------------
  TB-5V-01      Mean Well +5.1V Out   16 AWG        Power Supply Positive Output
  TB-5V-02      RPi 1 USB-C Power     20 AWG        Coordinator Pi 5V Input
  TB-5V-03      RPi 2 USB-C Power     20 AWG        Intersection 1 Pi 5V Input
  TB-5V-04      RPi 3 USB-C Power     20 AWG        Intersection 2 Pi 5V Input
  TB-5V-05      RPi 4 USB-C Power     20 AWG        Intersection 3 Pi 5V Input
  TB-5V-06      Buzzer Supply (+)     22 AWG        Acoustic Buzzer Positive Terminal
  TB-5V-07      Cooling Fan 1 & 2     22 AWG        Chassis Ventilation Fans
  
  MASTER STAR GROUND BUSBAR (COMMON GROUND REFERENCE)
  Terminal #    Signal Name           Wire Gauge    Destination Peripheral
  -------------------------------------------------------------------------------------------------
  TB-GND-01     Mean Well DC Return   16 AWG        Power Supply Negative Output
  TB-GND-02     RPi 1 USB-C Ground    20 AWG        Coordinator Pi Ground Return
  TB-GND-03     RPi 2 USB-C Ground    20 AWG        Intersection 1 Pi Ground Return
  TB-GND-04     RPi 3 USB-C Ground    20 AWG        Intersection 2 Pi Ground Return
  TB-GND-05     RPi 4 USB-C Ground    20 AWG        Intersection 3 Pi Ground Return
  TB-GND-06     Buzzer Emitter Return 22 AWG        2N2222 Transistor Emitter
  TB-GND-07     Mains Chassis Earth   14 AWG        Chassis Safety Ground Bond (PE)
+---------------------------------------------------------------------------------------------------+
```

---

### 15. Network Architecture

The four nodes communicate over a private Class C subnet (`192.168.1.0/24`) with sub-millisecond round-trip latency:

```
===================================================================================================
                               SMART AEGIS TCP/IP NETWORK TOPOLOGY
===================================================================================================

    +------------------------------------------------------------------------+
    |        Industrial 5-Port Gigabit Switch / Wi-Fi AP (192.168.1.1)       |
    +------------------------------------------------------------------------+
           |                         |                   |                   |
    192.168.1.100              192.168.1.101       192.168.1.102       192.168.1.103
           |                         |                   |                   |
           v                         v                   v                   v
    +--------------+          +--------------+    +--------------+    +--------------+
    | MASTER COORD | <======> | TRAFFIC INT1 |    | TRAFFIC INT2 |    | TRAFFIC INT3 |
    | Port 9000    |   TCP    | Port 9001    |    | Port 9002    |    | Port 9003    |
    | Server       |  Socket  | Client       |    | Client       |    | Client       |
    +--------------+          +--------------+    +--------------+    +--------------+
```

---

### 16. Bench Assembly Procedure

```
+---------------------------------------------------------------------------------------------------+
|                                 BENCHTOP PHYSICAL ASSEMBLY PROCEDURE                              |
+---------------------------------------------------------------------------------------------------+
  STEP 1: Workstation ESD & Structural Prep
    - Clean workbench surface; attach ESD wrist grounding strap to building earth ground.
    - Mount 35mm DIN rail horizontally across demonstration mounting backplane.
    
  STEP 2: Power Subsystem Installation
    - Snap Mean Well LRS-75-5 PSU onto left side of DIN rail.
    - Mount 5.1V DC Distribution Busbar and Master Star Ground Busbar adjacent to PSU.
    - Connect 16 AWG red conductor from PSU V+ to 5V busbar; connect 16 AWG black to Star Ground.
    
  STEP 3: Raspberry Pi Mounting & Power Cabling
    - Secure all 4 Raspberry Pi boards onto nylon standoffs; label RPI_COORD, INT_1, INT_2, INT_3.
    - Route 20 AWG USB-C power leads from 5V distribution busbar to each Pi USB-C power input port.
    - Verify power cable polarity with DMM before connecting.
    
  STEP 4: Prototype Direct GPIO LED Installation
    - Intersection 1 (RPi 2): Connect Red LED to Pin 29, Blue LED to Pin 31, Cathodes to Pin 30 (GND).
    - Intersection 2 (RPi 3): Connect Red LED to Pin 29, Blue LED to Pin 31, Cathodes to Pin 30 (GND).
    - Intersection 3 (RPi 4): Connect Blue LED to Pin 31, Cathode to Pin 30 (GND). (No Red LED).
    - Coordinator (RPi 1): Zero LEDs connected.
        
  STEP 5: Optical IR Proximity Sensor Installation
    - Mount LM393 IR sensors facing oncoming traffic approaches (Int 1: 1, Int 2: 1, Int 3: 1).
    - Wire VCC to Pin 1 (+3.3V), GND to Pin 9, and OUT to Pin 32 (BCM 12).
    
  STEP 6: Coordinator Peripheral Wiring
    - Connect Emergency Push Button to Pin 16 (BCM 23) and Pin 14 (GND).
    - Connect Police Override Switch to Pin 18 (BCM 24) and Pin 20 (GND).
    - Connect SSD1306 OLED to Pin 1 (+3.3V), Pin 9 (GND), Pin 3 (SDA), Pin 5 (SCL).
    - Assemble 2N2222A buzzer driver on mini-breadboard and connect base to Pin 31 (BCM 6).
    
  STEP 7: Pre-Power Cold Inspection
    - Inspect all 13 signal leads against Master GPIO Mapping Table.
    - Verify zero short circuits between 5V DC bus and ground with DMM (R > 100 kOhm).
+---------------------------------------------------------------------------------------------------+
```

---

### 17. Raspberry Pi Deployment Procedure

1. **Flash MicroSD Cards:** Flash official `QNX Neutrino RTOS 8.0 Starter Kit (aarch64le)` image onto 4 SanDisk 32GB MicroSD cards using `dd` or Raspberry Pi Imager.
2. **Configure Boot Parameters:** Mount boot partition and edit `/boot/config.txt`:
   ```ini
   enable_uart=1
   arm_64bit=1
   dtparam=i2c_arm=on
   dtparam=i2c_arm_baudrate=400000
   ```
3. **Static IP Configuration:** On each card, configure `/etc/net.cfg`:
   - Coordinator: `192.168.1.100 / 255.255.255.0`
   - Node 1: `192.168.1.101 / 255.255.255.0`
   - Node 2: `192.168.1.102 / 255.255.255.0`
   - Node 3: `192.168.1.103 / 255.255.255.0`

---

### 18. SCP / SSH Deployment Workflow

Execute automated deployment from the host workstation:

```bash
#!/bin/bash
# deploy_cluster.sh - SMART AEGIS Automated Deployment
NODES=("192.168.1.100" "192.168.1.101" "192.168.1.102" "192.168.1.103")
BINARIES=("aegis_coordinator" "aegis_traffic_signal_node" "test_cluster_deployment_hardware")

echo "=== Deploying SMART AEGIS Binaries to Cluster ==="
for IP in "${NODES[@]}"; do
    echo "Deploying to node: $IP"
    ssh root@$IP "mkdir -p /opt/aegis/bin /opt/aegis/config /opt/aegis/logs"
    scp build/bin/* root@$IP:/opt/aegis/bin/
    scp config/cluster/production/rpi*.conf root@$IP:/opt/aegis/config/
    ssh root@$IP "chmod +x /opt/aegis/bin/*"
done
echo "=== Deployment Complete ==="
```

---

### 19. Git Pull / Software Update Procedure

To update software without re-flashing:
1. `git pull origin release/v1.0-hackathon-core`
2. `make -j4`
3. Execute `./scripts/deployment/deploy_cluster.sh`
4. Node supervisor detects binary update, performs rolling restart with zero corridor downtime.

---

### 20. Startup Sequence

1. **Phase 1 (Power-On):** Energize Mean Well 5V PSU. All 4 Raspberry Pis boot QNX 8.0 concurrently ($\le 18\text{s}$).
2. **Phase 2 (Network Convergence):** Network gateway verifies static ARP entries and TCP connectivity.
3. **Phase 3 (Coordinator Launch):** `aegis_coordinator` starts on RPi 1, binds to port `9000`, initializes I2C OLED display, and sounds 2-pulse ready chime on buzzer. Zero LEDs illuminated on Coordinator.
4. **Phase 4 (Traffic Nodes Launch):** `aegis_traffic_signal_node` launches on RPi 2, 3, 4. Each node establishes persistent TCP connection to Coordinator.
5. **Phase 5 (Default Normal Operation):** Prototype LEDs remain OFF on all intersections (Blue OFF, Red OFF), waiting for green corridor preemption requests.

---

### 21. Commissioning Procedure

Execute automated commissioning via:
```bash
./build/bin/test_cluster_deployment_hardware
```
- **Vector 1:** Network Connectivity (TCP/IP Mesh) verified ($< 5.0\text{ ms}$).
- **Vector 2:** Node Configuration & Pin Allocation verified with zero collisions.
- **Vector 3:** GPIO Prototype LED & Hardware Layer verified:
  - Coordinator: Zero LEDs expected.
  - Intersection 1: GPIO 5 (Red) & GPIO 6 (Blue) loopback verified.
  - Intersection 2: GPIO 5 (Red) & GPIO 6 (Blue) loopback verified.
  - Intersection 3: GPIO 6 (Blue) loopback verified; zero Red LED (shortage specification).
- **Vector 4:** Direct GPIO Prototype LED Driver & State Transitions verified.
- **Vector 5:** Optical IR Sensor Debouncing & Health verified ($30\text{ ms}$ filter).
- **Vector 6:** EventBus Inter-Module Communication verified.
- **Vector 7:** Cluster Time Synchronization verified ($\pm 5.0\text{ ms}$ drift).

---

### 22. Hardware Bring-up

1. **Cold Voltage Verification:** Disconnect Pis. Turn on 5V PSU. Measure voltage on 5V busbar: adjust potentiometer until DMM reads exactly **$5.10\text{V} \pm 0.02\text{V}$**.
2. **Powered Node Bring-Up:** Connect USB-C cables one by one. Confirm power LED illuminates steady red on each board (no undervoltage flash).
3. **Serial Console Audit:** Connect USB-to-UART cable (BCM 14/15) to verify QNX microkernel initialization messages.

---

### 23. GPIO Verification

Run the GPIO diagnostic loopback test:
```bash
./build/bin/test_cluster_deployment_hardware
```
Verifies electrical high/low transition, rise time ($< 15\text{ ns}$), and pull-up bias on all assigned pins.

---

### 24. Prototype Direct GPIO LED Verification

Execute the dedicated Prototype LED diagnostic routine:

```
+---------------------------------------------------------------------------------------------------+
|                        PROTOTYPE LED FUNCTIONAL STEP SEQUENCE                                     |
+---------------------------------------------------------------------------------------------------+
  Step 1: Normal State Verification (Resting)
    - Verify Blue LED is OFF across all three intersections.
    - Verify Red LED is OFF across Intersections 1 & 2.
    
  Step 2: Emergency Corridor Preemption (Green Wave Granted)
    - Command GrantEmergencyPath():
        Intersection 1 (RPi 2): Blue LED turns ON, Red LED turns OFF.
        Intersection 2 (RPi 3): Blue LED turns ON, Red LED turns OFF.
        Intersection 3 (RPi 4): Blue LED turns ON.
    - Measure current draw: Blue LED approx 1.5 mA at 3.3V through 330R.
    
  Step 3: Corridor Clearance & Closure
    - Command CloseEmergencyPath():
        Intersection 1 (RPi 2): Blue LED turns OFF.
        Intersection 2 (RPi 3): Blue LED turns OFF.
        Intersection 3 (RPi 4): Blue LED turns OFF.
    - Verify safe return to normal resting state.
+---------------------------------------------------------------------------------------------------+
```

---

### 25. IR Sensor Verification

1. **Resting State:** Verify sensor output is HIGH ($3.3\text{V}$) when beam is clear.
2. **Beam Obstruction:** Place card $5\text{ cm}$ in front of optics. Verify output drops to LOW ($0\text{V}$).
3. **Debounce Filter:** Verify that rapid vibration of target does not cause multiple trigger events.

---

### 26. OLED Verification

1. Scan I2C bus: `i2cdetect -y 1` must display device at `0x3C`.
2. Display test patterns:
   - Frame 1: `SMART AEGIS - SYSTEM READY`
   - Frame 2: `MISSION INTAKE: AMB_01 [CODE RED]`
   - Frame 3: `CORRIDOR PREEMPTED: INT 1-2-3`
   - Frame 4: `HOSPITAL ARRIVAL CONFIRMED`
3. Verify zero pixel snow, artifacting, or visual flickering.

---

### 27. Buzzer Verification

Sound test pattern across all 5 operational cadences:
- **Tone 1:** Continuous $2.7\text{ kHz}$ tone for $1.0\text{s}$ (Corridor Preemption Active).
- **Tone 2:** Rapid pulsed beep $100\text{ms}$ ON / $100\text{ms}$ OFF (Priority Conflict Warning).
- **Tone 3:** Double-pulse alert (Police Detour Dynamic Replan).
- **Tone 4:** Slow intermittent chirp $50\text{ms}$ ON / $950\text{ms}$ OFF (System Heartbeat).
- **Tone 5:** Ascending alert cadence (Hospital Handover Complete).

---

### 28. Switch Verification

1. **Emergency Button:** Press HOME button; verify QNX interrupt receives low pulse and dispatches mission.
2. **Police Override:** Flip toggle switch; verify Coordinator logs `ROAD_OBSTRUCTION` advisory and recalculates detour.

---

### 29. TCP Communication Verification

1. Ping sweep all four nodes: packet round-trip time must be $< 1.0\text{ ms}$.
2. Flood test: stream 1,000 telemetry packets per second; zero packet drops permitted.
3. Socket disconnect resilience: unplug Ethernet cable on Node 2; reconnect within 5 seconds; verify automatic socket reconnection.

---

### 30. Cluster Formation Verification

Confirm all 4 nodes form a unified cluster:
```bash
./build/bin/aegis_coordinator --show-cluster
```
Output must display all 4 nodes in `ONLINE` state with synchronized monotonic timestamps.

---

### 31. Hardware Acceptance Test

The Hardware Acceptance Test (HAT) certifies the physical rig for hackathon presentation:

```
+---------------------------------------------------------------------------------------------------+
|                             HARDWARE ACCEPTANCE TEST (HAT) MATRIX                                 |
+---------+--------------------------+------------------------------------+-------------------------+
| Test ID | Verification Target      | Procedure                          | Acceptance Criteria     |
+---------+--------------------------+------------------------------------+-------------------------+
| HAT-01  | 5.1V DC Power Rail       | Measure DC busbar with DMM         | 5.10V +/- 0.05V ripple  |
| HAT-02  | QNX Microkernel Boot     | Cold boot all 4 Pis                | Login prompt <= 18s     |
| HAT-03  | Network Latency          | Ping sweep 100 packets             | Latency < 1.0 ms avg    |
| HAT-04  | BSD Socket Handshake     | Start Coordinator & Nodes          | 4/4 connected, CRC16 OK |
| HAT-05  | Heartbeat Streaming      | Monitor 60 seconds idle            | 0 missed heartbeats     |
| HAT-06  | Prototype LED Drive      | Cycle Blue ON / Red OFF            | Crisp optical contrast  |
| HAT-07  | Coordinator Pure TCP     | Verify Coordinator GPIO            | Zero LEDs illuminated   |
| HAT-08  | Optical IR Detection     | Obstruct IR sensor beam            | Latency < 30 ms         |
| HAT-09  | Hardware Shortage Node 3 | Inspect Node 3 (Int 3)             | Blue LED only; zero Red |
| HAT-10  | OLED Rendering           | Display preemption sequence        | Crisp 128x64 text       |
| HAT-11  | Buzzer Acoustic Rhythms  | Trigger all 5 preemption patterns  | Audible at 2.7 kHz      |
| HAT-12  | Emergency Mission Intake | Press HOME Emergency Push Button   | Instant corridor plan   |
| HAT-13  | Police Dynamic Replan    | Toggle Police Override Switch      | Detour with 0 pause     |
| HAT-14  | Hospital Handover        | Trip Int 3 sensor                  | Mission complete chime  |
+---------+--------------------------+------------------------------------+-------------------------+
```

---

### 32. Live Demonstration Procedure

```
+---------------------------------------------------------------------------------------------------+
|                                  HACKATHON DEMO EXECUTION SCRIPT                                  |
+---------------------------------------------------------------------------------------------------+
  STAGE 1: Baseline Normal City Traffic
    - Intersections 1, 2, 3 in normal state: Blue LEDs OFF, Red LEDs OFF.
    - Coordinator operates purely via TCP; zero LEDs illuminated on Coordinator.
    - OLED displays "SMART AEGIS - SYSTEM NORMAL - MONITORING".
    
  STAGE 2: Emergency Call Intake & Green Corridor Activation
    - Presenter presses the HOME EMERGENCY PUSH BUTTON at the Coordinator Station.
    - Buzzer sounds continuous preemption alert.
    - OLED updates instantly: "MISSION INTAKE: AMB_01 (CODE RED) -> PREEMPTING".
    - Intersections 1, 2, 3 immediately switch their Blue LEDs ON (Emergency Path Granted).
    - Red LEDs remain OFF.
    
  STAGE 3: Optical Proximity Tracking Across Intersections
    - Presenter moves ambulance model across Intersection 1; IR sensor trips (Pin 32).
    - Intersection 1 transitions Blue LED OFF after ambulance clears.
    
  STAGE 4: Priority Arbitration at Intersection 2
    - Conflicting ambulance AMB_02 (Code Yellow) approaches Intersection 2.
    - Priority Scheduler arbitrates: AMB_01 (Code Red) retains Blue LED ON corridor preemption.
    - AMB_02 is held until corridor clears.
    
  STAGE 5: Police-Assisted Dynamic Route Replanning
    - Presenter activates the POLICE OVERRIDE SWITCH at the Coordinator.
    - Coordinator receives verified police advisory; calculates dynamic detour around Int 2.
    - Corridor updates with ZERO mission pause; OLED displays detour routing.
    
  STAGE 6: Hospital Handover & Corridor Release
    - Ambulance trips Intersection 3 IR sensor at Hospital gate (Pin 32).
    - Blue LED on Intersection 3 turns OFF.
    - Buzzer sounds handover chime; OLED renders "HOSPITAL ARRIVAL - CORRIDOR RELEASED".
    - All intersections restore normal resting states (All Blue LEDs OFF).
+---------------------------------------------------------------------------------------------------+
```

---

### 33. Troubleshooting Guide

```
+---------------------------------------------------------------------------------------------------+
|                                     TROUBLESHOOTING MATRIX                                        |
+----------------------+-----------------------------+----------------------------------------------+
| Failure Symptom      | Root Cause                  | Corrective Action                            |
+----------------------+-----------------------------+----------------------------------------------+
| LED unlit            | Reversed LED polarity       | Connect longer lead (anode) to Pin 29/31     |
| LED unlit on Node 3  | Expected hardware shortage  | Node 3 has Blue LED only; zero Red LED       |
| Dim LED output       | Missing logic high voltage  | Check GPIO pin configuration (must be OUTPUT)|
| IR beam stuck active | Optical sensor blinded/dirty| Clean lens; turn potentiometer CCW to trim   |
| OLED blank           | Wrong I2C address or wiring | Probe bus with i2cdetect -y 1; verify 0x3C   |
| Buzzer clicking      | Missing transistor base bias| Verify 1k resistor on base and Pin 2 (+5V)   |
| Socket disconnect    | Wi-Fi interference / drift  | Switch AP to 5GHz band; verify static IP     |
| RPi reboots          | 5V DC brownout under load   | Adjust 5V PSU pot to 5.10V at load terminals |
+----------------------+-----------------------------+----------------------------------------------+
```

---

### 34. Preventive Maintenance

- **Every 50 Operating Hours:** Clean optical IR sensor emitter and detector lenses with optical cloth.
- **Every 200 Operating Hours:** Inspect Dupont wire crimps for mechanical fatigue or intermittent contact.
- **Every 500 Operating Hours:** Retorque all power supply screw terminals to $0.5\text{ N}\cdot\text{m}$. Clean cooling fans.

---

### 35. Safety Precautions

1. **Low Voltage Operation:** All system wiring operates under Safety Extra-Low Voltage (SELV $\le 5.10\text{V}$ DC). Zero shock hazard.
2. **Short-Circuit Protection:** Overcurrent protection built into Mean Well PSU automatically shuts down rail in the event of a dead short.
3. **Thermal Limits:** Operating SoC junction temperature must remain $< 55^\circ\text{C}$ supported by cooling fans.

---

### 36. Future Expansion Possibilities

- **Dual-Approach Signal Heads:** Connecting a second LED pair to each Pi provides complete 4-LED dual-approach control.
- **I2C Port Expanders:** Integrating MCP23017 expanders allows 16 isolated signal channels per intersection.
- **C-V2X Radios:** Direct integration of 5.9 GHz DSRC / C-V2X transceivers into QNX network stack.
- **Grid Scaling:** Software architecture natively scales to 32+ intersections without coordinator redesign.

---

### 37. Final Deployment Checklist

```
 [ ] 1. Mean Well 5V power supply adjusted and locked at 5.10V DC.
 [ ] 2. Master Star Ground continuity verified to earth ground (R < 0.5 Ohm).
 [ ] 3. All 4 Raspberry Pis running QNX Neutrino RTOS 8.0 with static IP configuration.
 [ ] 4. All production configuration files deployed to /opt/aegis/config/node.conf.
 [ ] 5. Wi-Fi / Ethernet switch active with static ARP reservations.
 [ ] 6. Direct GPIO Prototype LEDs verified:
        - Coordinator: Zero LEDs connected.
        - Intersection 1: Red LED (Pin 29) & Blue LED (Pin 31) verified.
        - Intersection 2: Red LED (Pin 29) & Blue LED (Pin 31) verified.
        - Intersection 3: Blue LED (Pin 31) verified; zero Red LED (shortage specification).
 [ ] 7. Optical IR sensors calibrated for 5 cm - 10 cm vehicle detection.
 [ ] 8. OLED renders V2X messages with zero visual artifacting.
 [ ] 9. Acoustic buzzer verified at 2.7 kHz resonant preemption tone.
 [ ] 10. Hardware Acceptance Test (HAT-01 to HAT-14) passed 100%.
```

---

### 38. Appendix & Document Metadata

#### 38.1 40-Pin Header Reference Card

```
              Raspberry Pi 4 Model B (40-Pin GPIO Header)
                       +3.3V DC (Pin 1)  (2)  +5V DC Power
             BCM 2 (I2C-1 SDA) (Pin 3)  (4)  +5V DC Power
             BCM 3 (I2C-1 SCL) (Pin 5)  (6)  Ground
                         BCM 4 (Pin 7)  (8)  BCM 14 (UART TX)
                        Ground (Pin 9)  (10) BCM 15 (UART RX)
                        BCM 17 (Pin 11) (12) BCM 18
                        BCM 27 (Pin 13) (14) Ground
                        BCM 22 (Pin 15) (16) BCM 23: EMERGENCY_SW
                        +3.3V (Pin 17) (18) BCM 24: POLICE_SW
                        BCM 10 (Pin 19) (20) Ground
                         BCM 9 (Pin 21) (22) BCM 25
                        BCM 11 (Pin 23) (24) BCM 8
                        Ground (Pin 25) (26) BCM 7
         (Reserved EEPROM ID) (Pin 27) (28) (Reserved EEPROM ID)
          INT1/2_LED_RED: BCM 5 (Pin 29) (30) Ground: LED Cathode Common
  INT1/2/3_LED_BLUE/BUZZ: BCM 6 (Pin 31) (32) BCM 12: INTx_APPROACH_IR
                        BCM 13 (Pin 33) (34) Ground
                        BCM 19 (Pin 35) (36) BCM 16
                        BCM 26 (Pin 37) (38) BCM 20
                        Ground (Pin 39) (40) BCM 21
```

#### 38.2 Document Revision History

| Revision | Date | Description | Author / Approver |
| :---: | :---: | :--- | :--- |
| **v1.0.0** | 2026-10-31 | Initial symbolic hardware reservation specification | SMART AEGIS Core Team |
| **v1.2.0** | 2026-10-31 | Approved conflict-free BCM GPIO mappings and schematics | Hardware Systems Lead |
| **v1.5.0** | 2026-10-31 | Comprehensive 38-section engineering deployment handbook | Lead Systems Architect |
| **v2.0.0** | 2026-10-31 | Direct GPIO Traffic Light LED Module (relays & 12V eliminated) | Lead Hardware Architect |
| **v2.5.0** | 2026-10-31 | Final Prototype Architecture: Direct GPIO LEDs (Int 1 & 2: Red/Blue; Int 3: Blue only; Coord: 0 LEDs) | Lead Hardware Architect |

#### 38.3 Master Metadata

```
Project Name:        SMART AEGIS (Automated Emergency Routing System)
Document Identifier: AEGIS-MAN-HW-2026-PHASE15
Version Number:      2.5.0
Release Date:        October 31, 2026
Document Owner:      Hardware Integration & Deployment Directorate
Hardware Revision:   HW-REV-3.5 (4-Pi Cluster + Direct GPIO LEDs: Int 1/2 Red/Blue, Int 3 Blue only, Coord 0 LEDs)
Software Revision:   v1.0-hackathon-core (Phases 1-15 Verified)
Operating System:    QNX Neutrino RTOS 8.0 Starter Kit (aarch64le)
Compute Platform:    Raspberry Pi 4 Model B (4 GB RAM, Quad Cortex-A72 @ 1.5 GHz)
Verification Status:  APPROVED FOR LIVE HARDWARE COMMISSIONING & DEMONSTRATION
```
