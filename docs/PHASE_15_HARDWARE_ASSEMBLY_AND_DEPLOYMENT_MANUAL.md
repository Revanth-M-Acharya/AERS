# SMART AEGIS (AERS) — Phase 15 Engineering Manual
## Physical Hardware Assembly, Direct GPIO Traffic Light LED Module Mapping, Raspberry Pi Deployment & Commissioning Handbook

```
===================================================================================================
  PROJECT:         SMART AEGIS (Automated Emergency Routing System)
  DOCUMENT ID:     AEGIS-MAN-HW-2026-PHASE15
  VERSION:         2.0.0
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
7. [Traffic Light LED Module Wiring](#7-traffic-light-led-module-wiring)
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
24. [Traffic Light Module Verification](#24-traffic-light-module-verification)
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

**Phase 15** delivers the physical realization of this architecture for final hackathon deployment. It transitions the validated software and symbolic designs into an operational physical demonstration rig. Each roadside intersection node directly drives an integrated 3-light Traffic Light LED Module through GPIO with microsecond transition speed. Every electrical connection, BCM GPIO pin assignment, sensor interface, power rail, and startup procedure is documented herein with absolute engineering rigor.

```
+---------------------------------------------------------------------------------------------------+
|                                  CORE ENGINEERING GUARANTEES                                      |
+---------------------------------------------------------------------------------------------------+
| 1. ZERO SOFTWARE REDESIGN: Phases 1-13 software core is 100% frozen, untouched, and guaranteed.   |
| 2. CONFLICT-FREE GPIO: All 19 BCM pins verified for electrical logic, drive, and QNX compatibility.|
| 3. NO LINUX APIS: Native QNX SDP 8.0 POSIX drivers only. Zero wiringPi, pigpio, or sysfs calls.   |
| 4. SOLID-STATE SAFETY: Software mutual exclusion in RTOS driver prevents dual green indications.  |
| 5. SINGLE 5.1V DC RAIL: No 12V supplies, electromechanical relays, or coil transients.            |
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
| **Signals** | Traffic Light LED Modules | Integrated 3-Light Common-Cathode Module (Red, Yellow, Green)| 3 | Nodes 2, 3, 4 | Direct solid-state intersection traffic signal indication |
| **Perception** | Optical IR Proximity Sensors | LM393 Optical Reflection Detector (Adjustable Potentiometer) | 4 | Nodes 2, 3, 4 | Optical vehicle detection (Int 1: 1, Int 2: 2, Int 3: 1) |
| **Operator HMI** | Emergency Push Button | Heavy-Duty Industrial Momentary Push Button (SPST-NO, 22mm) | 1 | Coordinator | Physical trigger at HOME dispatch station for mission launch |
| **Operator HMI** | Police Override Switch | Toggle Switch (SPST Maintained Contact, Panel Mount) | 1 | Coordinator | Simulates verified police advisory for zero-pause dynamic rerouting |
| **V2X Telemetry** | In-Vehicle OLED Display | 0.96" I2C Monochrome Display (SSD1306, 128x64 pixels, 3.3V) | 1 | Coordinator | Driver advisory display rendering real-time corridor & routing status |
| **Acoustics** | High-Decibel Piezo Buzzer | 5V Active Electromagnetic Buzzer (2.7 kHz resonant frequency) | 1 | Coordinator | Acoustic corridor state alerts (5 distinct cadence patterns) |
| **Transistor Buffer**| BJT Driver Circuit | 2N2222A NPN BJT + 1.0k Ohm base resistor + 1N4148 diode | 1 | Coordinator | Amplifies 3.3V GPIO to 5V rail for buzzer drive |
| **Network** | Ethernet Switch / AP | TP-Link 5-Port Gigabit Switch / 5GHz 802.11ac Wi-Fi Gateway | 1 | Master Cluster | Sub-millisecond TCP/IP communication fabric (192.168.1.0/24) |
| **Cabling** | Breadboard / Dupont Jumpers | 24 AWG Female-to-Female & Male-to-Female Dupont Wire (20cm) | 60 | Master Cluster | Point-to-point interconnects between GPIO headers and peripherals |

---

### 3. Raspberry Pi Cluster Architecture

The physical layout mirrors a real-world municipal emergency route extending from an emergency dispatch depot to a regional trauma center:

```
+---------------------------------------------------------------------------------------------------+
|                                  PHYSICAL CORRIDOR ROAD TOPOLOGY                                  |
+---------------------------------------------------------------------------------------------------+

   [ HOME STATION ]  (RPi 1: Master Coordinator - 192.168.1.100)
         |
         |  Arterial Segment 1 (Speed: 50 km/h, Nominal Transit: 12.0s)
         v
   [ INTERSECTION 1 ] (RPi 2: Traffic Node 1 - 192.168.1.101)
     - Traffic Light LED Module 1 (Red: BCM 17, Yellow: BCM 27, Green: BCM 22)
     - Approach Optical IR Proximity Sensor (BCM 12)
         |
         |  Arterial Segment 2 (Speed: 50 km/h, Nominal Transit: 14.5s)
         v
   [ INTERSECTION 2 ] (RPi 3: Traffic Node 2 - 192.168.1.102) [PRIORITY ARBITRATION HUB]
     - Traffic Light LED Module 2 (Red: BCM 17, Yellow: BCM 27, Green: BCM 22)
     - Top Approach Optical IR Sensor (Code Red Inbound - BCM 12)
     - Right Approach Optical IR Sensor (Code Yellow Cross-Traffic - BCM 13)
         |
         |  Arterial Segment 3 (Speed: 50 km/h, Nominal Transit: 11.0s)
         v
   [ INTERSECTION 3 ] (RPi 4: Traffic Node 3 - 192.168.1.103)
     - Traffic Light LED Module 3 (Red: BCM 17, Yellow: BCM 27, Green: BCM 22)
     - Hospital Gate Optical IR Sensor (BCM 12)
         |
         v
   [ TRAUMA HOSPITAL ] (Terminal Destination & Emergency Department Handover)
+---------------------------------------------------------------------------------------------------+
```

#### Node Allocation & Responsibilities Table

| Node Identifier | Physical Hardware | Static IP Address | Port | Role & Executable | Primary Responsibilities |
| :--- | :--- | :--- | :---: | :--- | :--- |
| **`RPI_COORD`** | Raspberry Pi 4 (4GB) | `192.168.1.100` | `9000` | Master Coordinator<br>`aegis_coordinator` | Mission intake, Dijkstra graph solver, green corridor reservation, priority arbitration, V2X OLED rendering, acoustic buzzer sequencing. |
| **`INT_1`** | Raspberry Pi 4 (4GB) | `192.168.1.101` | `9001` | Traffic Node 1<br>`aegis_traffic_signal_node` | Intersection 1 signal actuation, direct GPIO LED control, vehicle approach optical sensing, keepalive heartbeat streaming. |
| **`INT_2`** | Raspberry Pi 4 (4GB) | `192.168.1.102` | `9002` | Traffic Node 2<br>`aegis_traffic_signal_node` | Intersection 2 Priority Hub, multi-approach conflict resolution, dual IR sensing (Top vs Right approach), dynamic detour handling. |
| **`INT_3`** | Raspberry Pi 4 (4GB) | `192.168.1.103` | `9003` | Traffic Node 3<br>`aegis_traffic_signal_node` | Intersection 3 Hospital Gate, terminal corridor clearance, hospital arrival optical verification, mission completion signal. |

---

### 4. Complete GPIO Mapping Tables

#### 4.1 Master Cluster GPIO Allocation Table

All 19 signals mapped across the four Raspberry Pi 4 Model B headers are verified to be conflict-free:

```
+---------------------------------------------------------------------------------------------------+
|                                     MASTER GPIO MAPPING TABLE                                     |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| Symbolic Identifier      | Node        | BCM GPIO | Header   | Direction | Voltage Level | Active Pol |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| HOME_EMERGENCY_SWITCH    | Coordinator | BCM 23   | Pin 16   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| POLICE_OVERRIDE_SWITCH   | Coordinator | BCM 24   | Pin 18   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| HOME_BUZZER              | Coordinator | BCM 6    | Pin 31   | OUTPUT    | 3.3V -> BJT   | ACTIVE_HIGH|
| COORDINATOR_STATUS_LED   | Coordinator | BCM 18   | Pin 12   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| V2X_OLED_SDA             | Coordinator | BCM 2    | Pin 3    | BIDIR     | 3.3V Open-Dr  | I2C DATA   |
| V2X_OLED_SCL             | Coordinator | BCM 3    | Pin 5    | OUTPUT    | 3.3V Open-Dr  | I2C CLOCK  |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT1_GPIO_RED            | Node 1      | BCM 17   | Pin 11   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT1_GPIO_YELLOW         | Node 1      | BCM 27   | Pin 13   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT1_GPIO_GREEN          | Node 1      | BCM 22   | Pin 15   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT1_IR_SENSOR           | Node 1      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT2_GPIO_RED            | Node 2      | BCM 17   | Pin 11   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT2_GPIO_YELLOW         | Node 2      | BCM 27   | Pin 13   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT2_GPIO_GREEN          | Node 2      | BCM 22   | Pin 15   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT2_IR_SENSOR_TOP       | Node 2      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
| INT2_IR_SENSOR_RIGHT     | Node 2      | BCM 13   | Pin 33   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
| INT3_GPIO_RED            | Node 3      | BCM 17   | Pin 11   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT3_GPIO_YELLOW         | Node 3      | BCM 27   | Pin 13   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT3_GPIO_GREEN          | Node 3      | BCM 22   | Pin 15   | OUTPUT    | 3.3V LVTTL    | ACTIVE_HIGH|
| INT3_IR_SENSOR           | Node 3      | BCM 12   | Pin 32   | INPUT     | 3.3V Pull-Up  | ACTIVE_LOW |
+--------------------------+-------------+----------+----------+-----------+---------------+------------+
```

---

### 5. Full Wiring Diagrams

#### 5.1 Coordinator Station Schematic (RPi 1)

```
+-------------------------------------------------------------+
|              COORDINATOR STATION ELECTRICAL SCHEMATIC        |
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
   
   [Pin 12] BCM 18 (STATUS_LED) ----------> [ 330R Resistor ] ---> Green LED (+)
   [Pin 6 ] Ground (GND) ----------------------------------------> Green LED (-)
   
   [Pin 2 ] +5.0V DC Rail ----------------> Piezo Buzzer (+)
   [Pin 31] BCM 6 (BUZZER_GATE) ----------> [ 1k Resistor ] -> Base 2N2222A
                                            Collector --------> Piezo Buzzer (-)
                                            Emitter ----------> Ground (Pin 39)
+-------------------------------------------------------------+
```

#### 5.2 Traffic Intersection Nodes Schematic (RPi 2, 3, 4)

```
+-------------------------------------------------------------+
|           TRAFFIC INTERSECTION NODES ELECTRICAL SCHEMATIC    |
+-------------------------------------------------------------+
   Raspberry Pi 4 Model B (Nodes 2, 3, 4 - Intersection RSU)
   
   [Pin 1 ] +3.3V DC ---------------------> LM393 IR Sensor VCC
   [Pin 9 ] Ground (GND) -----------------> LM393 IR Sensor GND
   [Pin 32] BCM 12 (IR_INPUT_A) <---------- LM393 IR Sensor OUT (Approach A)
   [Pin 33] BCM 13 (IR_INPUT_B) <---------- LM393 IR Sensor OUT (Approach B - Node 2 Only)
   
   DIRECT GPIO TRAFFIC LIGHT LED MODULE:
   [Pin 11] BCM 17 (GPIO_RED) ------------> Traffic Light Module Pin R (Red LED)
   [Pin 13] BCM 27 (GPIO_YELLOW) ---------> Traffic Light Module Pin Y (Yellow LED)
   [Pin 15] BCM 22 (GPIO_GREEN) ----------> Traffic Light Module Pin G (Green LED)
   [Pin 14] Ground (GND) -----------------> Traffic Light Module Pin GND (Cathode)
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
  OLED   LED    LED    LED
  Disp   Mod1   Mod2   Mod3
===================================================================================================
```

#### 6.2 Current Consumption & Power Budget

| Subsystem Branch | Voltage | Nominal Current | Peak Current | Total Power | Sizing Factor |
| :--- | :---: | :---: | :---: | :---: | :---: |
| 4x Raspberry Pi 4 Model B (CPU loaded)| 5.1V DC | 4 x 0.85 A = 3.40 A | 4 x 2.50 A = 10.00 A | 51.0 W | Peak boot/burst load |
| 3x Traffic Light LED Modules | 3.3V DC | 3 x 0.01 A = 0.03 A | 3 x 0.03 A = 0.09 A | 0.45 W | All LEDs lit concurrent |
| 4x LM393 Optical IR Sensors | 3.3V DC | 4 x 0.015 A = 0.06 A | 4 x 0.02 A = 0.08 A | 0.26 W | Active beam detect |
| 0.96" I2C SSD1306 OLED Display | 3.3V DC | 0.02 A | 0.035 A | 0.12 W | Full pixel display |
| Acoustic Buzzer Driver Circuit | 5.0V DC | 0.00 A (idle) | 0.04 A | 0.20 W | 2.7 kHz resonant tone |
| Dual 40mm 5V Chassis Fans | 5.0V DC | 0.16 A | 0.24 A | 1.20 W | Continuous cooling |
| **TOTAL LOAD BUDGET** | — | **3.67 A** | **10.49 A** | **53.23 W** | **PSU Capacity: 14.0 A (71.4W)** |

> [!NOTE]
> The Mean Well LRS-75-5 delivers 14.0A continuous output, providing a **33.5% safety margin** ($3.51\text{A}$ headroom) above absolute maximum peak cluster consumption.

---

### 7. Traffic Light LED Module Wiring

#### 7.1 Traffic Light LED Module Installation & Pin Allocation

Each intersection uses an integrated 3-Light Traffic Signal LED Module featuring discrete Red, Yellow, and Green LEDs with a common cathode return:

```
+---------------------------------------------------------------------------------------------------+
|                        TRAFFIC LIGHT LED MODULE PIN ALLOCATION                                    |
+---------------------------------------------------------------------------------------------------+
  Module Pin        Signal Name           Target RPi Pin     Logic Level     Operational Function
  -------------------------------------------------------------------------------------------------
  Pin R (Red)       GPIO_RED              Pin 11 (BCM 17)    3.3V LVTTL      Road A Stop Indication
  Pin Y (Yellow)    GPIO_YELLOW           Pin 13 (BCM 27)    3.3V LVTTL      Corridor Clearance Warn
  Pin G (Green)     GPIO_GREEN            Pin 15 (BCM 22)    3.3V LVTTL      Green Corridor Preempt
  Pin GND           COMMON_GROUND         Pin 9 / Pin 14     0V Reference    Common Cathode Return
+---------------------------------------------------------------------------------------------------+
```

#### 7.2 Direct GPIO Wiring & Current-Limiting Resistor Specifications

Standard 3-light traffic LED modules incorporate on-board surface-mount current-limiting resistors:
- **Red LED:** $220\,\Omega \pm 5\%$ limits current to $I = (3.3\text{V} - 1.8\text{V}) / 220\,\Omega \approx 6.8\text{ mA}$.
- **Yellow LED:** $330\,\Omega \pm 5\%$ limits current to $I = (3.3\text{V} - 2.0\text{V}) / 330\,\Omega \approx 3.9\text{ mA}$.
- **Green LED:** $220\,\Omega \pm 5\%$ limits current to $I = (3.3\text{V} - 2.1\text{V}) / 220\,\Omega \approx 5.5\text{ mA}$.

Total current per traffic node never exceeds $16.2\text{ mA}$, even if all three indicators are illuminated simultaneously during self-test. This is well within the Broadcom BCM2711 per-rail GPIO current capability ($50\text{ mA}$ maximum total).

```
   Raspberry Pi 4 GPIO Pin                          Traffic Light Module
   
   Pin 11 (BCM 17) [OUTPUT HIGH: 3.3V] -----> [R] ---[ 220 Ohm ]--->| (Red LED)   ---+
   Pin 13 (BCM 27) [OUTPUT HIGH: 3.3V] -----> [Y] ---[ 330 Ohm ]--->| (Yellow LED) -+--> [GND] -> Pin 9 (GND)
   Pin 15 (BCM 22) [OUTPUT HIGH: 3.3V] -----> [G] ---[ 220 Ohm ]--->| (Green LED)  -+
```

#### 7.3 Solid-State Safety Switching & Mutual Exclusion

1. **Microsecond Switching:** LED state changes occur in $< 1\,\mu\text{s}$, eliminating mechanical relay contact chatter, arc erosion, and contact welding risks.
2. **Deterministic Driver Interlocks:** The QNX RTOS signal driver enforces mutual exclusion in software. If both Red and Green states are commanded simultaneously, the driver traps the conflict and forces an immediate safe **ALL_RED** transition.
3. **Boot & Fail-Safe Invariant:** Upon QNX bootloader initialization or node crash, all GPIO lines default to inputs with weak pull-downs, ensuring all signal lights are extinguished or held at safe default states.

---

### 8. IR Sensor Wiring

```
+---------------------------------------------------------------------------------------------------+
|                              LM393 OPTICAL IR SENSOR WIRING DIAGRAM                               |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 4 Header Pin               LM393 Proximity Sensor Module
  
  [Pin 1 ] +3.3V Power Rail     --------> [ VCC ] Supply Input (3.3V DC)
  [Pin 9 ] Ground (GND)         --------> [ GND ] Ground Return
  [Pin 32] BCM 12 (IR_INPUT_A)  <-------- [ OUT ] Digital Open-Collector Output (Active-Low)
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

- **Transistor:** 2N2222A NPN BJT in common-emitter switching configuration.
- **Base Drive:** $1.0\text{ k}\Omega$ base resistor limits drive current to $2.6\text{ mA}$ from BCM 6, guaranteeing saturation ($V_{CE(sat)} \le 0.2\text{V}$).

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
| W-201   | RPi 2 Pin 11     | Traffic Mod 1 R  | INT1_GPIO_RED         | 24AWG | Red    | Dupont F |
| W-202   | RPi 2 Pin 13     | Traffic Mod 1 Y  | INT1_GPIO_YELLOW      | 24AWG | Yellow | Dupont F |
| W-203   | RPi 2 Pin 15     | Traffic Mod 1 G  | INT1_GPIO_GREEN       | 24AWG | Green  | Dupont F |
| W-204   | RPi 2 Pin 9      | Traffic Mod 1 GND| INT1_LED_GND          | 24AWG | Black  | Dupont F |
| W-205   | RPi 2 Pin 32     | IR Sensor 1 OUT  | INT1_IR_SENSOR        | 24AWG | Purple | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-301   | RPi 3 Pin 11     | Traffic Mod 2 R  | INT2_GPIO_RED         | 24AWG | Red    | Dupont F |
| W-302   | RPi 3 Pin 13     | Traffic Mod 2 Y  | INT2_GPIO_YELLOW      | 24AWG | Yellow | Dupont F |
| W-303   | RPi 3 Pin 15     | Traffic Mod 2 G  | INT2_GPIO_GREEN       | 24AWG | Green  | Dupont F |
| W-304   | RPi 3 Pin 9      | Traffic Mod 2 GND| INT2_LED_GND          | 24AWG | Black  | Dupont F |
| W-305   | RPi 3 Pin 32     | IR Sensor 2A OUT | INT2_IR_SENSOR_TOP    | 24AWG | Purple | Dupont F |
| W-306   | RPi 3 Pin 33     | IR Sensor 2B OUT | INT2_IR_SENSOR_RIGHT  | 24AWG | Grey   | Dupont F |
+---------+------------------+------------------+-----------------------+-------+--------+----------+
| W-401   | RPi 4 Pin 11     | Traffic Mod 3 R  | INT3_GPIO_RED         | 24AWG | Red    | Dupont F |
| W-402   | RPi 4 Pin 13     | Traffic Mod 3 Y  | INT3_GPIO_YELLOW      | 24AWG | Yellow | Dupont F |
| W-403   | RPi 4 Pin 15     | Traffic Mod 3 G  | INT3_GPIO_GREEN       | 24AWG | Green  | Dupont F |
| W-404   | RPi 4 Pin 9      | Traffic Mod 3 GND| INT3_LED_GND          | 24AWG | Black  | Dupont F |
| W-405   | RPi 4 Pin 32     | IR Sensor 3 OUT  | INT3_IR_SENSOR        | 24AWG | Purple | Dupont F |
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

- **Protocol:** Non-blocking BSD sockets with framing headers, 16-bit CRC checksum validation, and sliding sequence numbers.
- **Heartbeat:** $1.0\text{-second}$ interval; node failure declared after 3 missed beats ($3000\text{ ms}$).

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
    
  STEP 4: Traffic Light LED Module Installation
    - Position Traffic Light LED Module 1 at Intersection 1; Module 2 at Int 2; Module 3 at Int 3.
    - Connect 4-conductor Dupont ribbon cable from Pi header to each module:
        Red Pin (R)    <---> Header Pin 11 (BCM 17)
        Yellow Pin (Y) <---> Header Pin 13 (BCM 27)
        Green Pin (G)  <---> Header Pin 15 (BCM 22)
        Ground Pin(GND)<---> Header Pin 9 or Pin 14 (GND)
        
  STEP 5: Optical IR Proximity Sensor Installation
    - Mount LM393 IR sensors facing oncoming traffic approaches (Int 1: 1, Int 2: 2, Int 3: 1).
    - Wire VCC to Pin 1 (+3.3V), GND to Pin 9, and OUT to Pin 32 (BCM 12) [Pin 33 for Int 2 Right].
    
  STEP 6: Coordinator Peripheral Wiring
    - Connect Emergency Push Button to Pin 16 (BCM 23) and Pin 14 (GND).
    - Connect Police Override Switch to Pin 18 (BCM 24) and Pin 20 (GND).
    - Connect SSD1306 OLED to Pin 1 (+3.3V), Pin 9 (GND), Pin 3 (SDA), Pin 5 (SCL).
    - Assemble 2N2222A buzzer driver on mini-breadboard and connect base to Pin 31 (BCM 6).
    
  STEP 7: Pre-Power Cold Inspection
    - Inspect all 19 signal leads against Master GPIO Mapping Table.
    - Verify zero short circuits between 5V DC bus and ground with DMM ($R > 100\text{ k}\Omega$).
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
BINARIES=("aegis_coordinator" "aegis_traffic_signal_node" "aegis_commissioning")

echo "=== Deploying SMART AEGIS Binaries to Cluster ==="
for IP in "${NODES[@]}"; do
    echo "Deploying to node: $IP"
    ssh root@$IP "mkdir -p /opt/aegis/bin /opt/aegis/config /opt/aegis/logs"
    scp build/bin/* root@$IP:/opt/aegis/bin/
    scp config/cluster/rpi*.conf root@$IP:/opt/aegis/config/
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
3. **Phase 3 (Coordinator Launch):** `aegis_coordinator` starts on RPi 1, binds to port `9000`, initializes I2C OLED display, and sounds 2-pulse ready chime on buzzer.
4. **Phase 4 (Traffic Nodes Launch):** `aegis_traffic_signal_node` launches on RPi 2, 3, 4. Each node establishes persistent TCP connection to Coordinator.
5. **Phase 5 (Default All-Red):** Traffic light modules illuminate **RED** on all approaches, waiting for corridor preemption requests.

---

### 21. Commissioning Procedure

Execute automated commissioning via:
```bash
./build/bin/aegis_commissioning --node RPI_ALL
```
- **Vector 1:** QNX microkernel tick resolution and monotonic clock sync verified ($< 1.0\text{ ms}$).
- **Vector 2:** Traffic Light LED Module mutual exclusion verified: Red, Yellow, Green step sequence OK.
- **Vector 3:** Optical IR sensor beam trip detection verified on all channels.
- **Vector 4:** I2C OLED display frame rendering verified.
- **Vector 5:** Acoustic buzzer resonant tone verified.
- **Vector 6:** Emergency push button and police switch interrupt loops verified.
- **Vector 7:** TCP socket streaming latency verified ($< 2.5\text{ ms}$).
- **Vector 8:** Cluster quorum and heartbeat synchronization established.

---

### 22. Hardware Bring-up

1. **Cold Voltage Verification:** Disconnect Pis. Turn on 5V PSU. Measure voltage on 5V busbar: adjust potentiometer until DMM reads exactly **$5.10\text{V} \pm 0.02\text{V}$**.
2. **Powered Node Bring-Up:** Connect USB-C cables one by one. Confirm power LED illuminates steady red on each board (no undervoltage flash).
3. **Serial Console Audit:** Connect USB-to-UART cable (BCM 14/15) to verify QNX microkernel initialization messages.

---

### 23. GPIO Verification

Run the GPIO diagnostic loopback test:
```bash
./build/bin/aegis_commissioning --test gpio_loopback
```
Verifies electrical high/low transition, rise time ($< 15\text{ ns}$), and pull-up bias on all 19 assigned pins.

---

### 24. Traffic Light Module Verification

Execute the dedicated Traffic Light Module diagnostic routine:
```bash
./build/bin/aegis_commissioning --test traffic_signal_led
```

```
+---------------------------------------------------------------------------------------------------+
|                        TRAFFIC LIGHT LED FUNCTIONAL STEP SEQUENCE                                 |
+---------------------------------------------------------------------------------------------------+
  Step 1: All-Red Verification
    - Assert INTx_GPIO_RED = HIGH. Verify Red LED illuminates crisp and bright.
    - Measure current draw: 6.8 mA. Verify Yellow and Green LEDs are completely dark.
    
  Step 2: Yellow Clearance Verification
    - De-assert Red; assert INTx_GPIO_YELLOW = HIGH for 3.0 seconds.
    - Verify Yellow LED illuminates; measure current draw: 3.9 mA.
    
  Step 3: Green Preemption Verification
    - De-assert Yellow; assert INTx_GPIO_GREEN = HIGH for 5.0 seconds.
    - Verify Green LED illuminates; measure current draw: 5.5 mA.
    
  Step 4: Mutual Exclusion Conflict Test
    - Command simultaneous Green and Red: driver must reject and force immediate ALL_RED.
    
  Step 5: Safe Shutdown Verification
    - Terminate driver process: verify all GPIO lines return to low or safe default state.
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
| HAT-06  | Traffic Light Illumination| Cycle Red -> Yellow -> Green      | 100% optical visibility |
| HAT-07  | Mutual Exclusion Lock    | Attempt dual green command         | Trapped; All-Red safe   |
| HAT-08  | Optical IR Detection     | Obstruct IR sensor beam            | Latency < 30 ms         |
| HAT-09  | Dual IR Priority Hub     | Simultaneous trip on Int 2         | Higher priority wins    |
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
    - Intersections 1, 2, 3 cycle through normal traffic light phases (Red -> Green -> Yellow).
    - OLED displays "SMART AEGIS - SYSTEM NORMAL - MONITORING".
    
  STAGE 2: Emergency Call Intake & Green Corridor Activation
    - Presenter presses the HOME EMERGENCY PUSH BUTTON at the Coordinator Station.
    - Buzzer sounds continuous preemption alert.
    - OLED updates instantly: "MISSION INTAKE: AMB_01 (CODE RED) -> PREEMPTING".
    - Intersections 1, 2, 3 immediately switch their Traffic Light Modules to GREEN preemption.
    
  STAGE 3: Optical Proximity Tracking Across Intersections
    - Presenter moves ambulance model across Intersection 1; IR sensor trips.
    - Intersection 1 transitions back to normal after ambulance passes.
    
  STAGE 4: Priority Arbitration at Intersection 2
    - Conflicting ambulance AMB_02 (Code Yellow) trips Right IR sensor at Intersection 2.
    - Priority Scheduler grants Green to primary Code Red, holding Code Yellow at Red.
    
  STAGE 5: Police-Assisted Dynamic Route Replanning
    - Presenter activates the POLICE OVERRIDE SWITCH at Intersection 2.
    - Coordinator receives verified police advisory; calculates dynamic detour around Int 2.
    - Corridor updates with ZERO mission pause; OLED displays detour routing.
    
  STAGE 6: Hospital Handover & Corridor Release
    - Ambulance trips Intersection 3 IR sensor at Hospital gate.
    - Buzzer sounds handover chime; OLED renders "HOSPITAL ARRIVAL - CORRIDOR RELEASED".
    - All intersections restore normal traffic cycles.
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
| Traffic LED unlit    | Loose Dupont ground lead    | Verify Pin GND connection to Pi Pin 9/14     |
| Wrong LED illuminates| Inverted R/Y/G wire pairing | Swap Pin 11/13/15 connections against table  |
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

- **Dual-Approach Signal Heads:** Connecting a second 3-light traffic module to each Pi provides complete 6-LED dual-approach control.
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
 [ ] 6. Traffic Light LED Modules verified on all three intersections (Red, Yellow, Green).
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
         INTx_GPIO_RED: BCM 17 (Pin 11) (12) BCM 18: STATUS_LED
      INTx_GPIO_YELLOW: BCM 27 (Pin 13) (14) Ground
       INTx_GPIO_GREEN: BCM 22 (Pin 15) (16) BCM 23: EMERGENCY_SW
                        +3.3V (Pin 17) (18) BCM 24: POLICE_SW
                        BCM 10 (Pin 19) (20) Ground
                         BCM 9 (Pin 21) (22) BCM 25
                        BCM 11 (Pin 23) (24) BCM 8
                        Ground (Pin 25) (26) BCM 7
         (Reserved EEPROM ID) (Pin 27) (28) (Reserved EEPROM ID)
                         BCM 5 (Pin 29) (30) Ground
            HOME_BUZZER: BCM 6 (Pin 31) (32) BCM 12: INTx_IR_SENSOR (Approach A)
   INT2_IR_SENSOR_RIGHT: BCM 13 (Pin 33) (34) Ground
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
| **v2.0.0** | 2026-10-31 | Final Hackathon Hardware: Direct GPIO Traffic Light LED Module (relays & 12V eliminated) | Lead Hardware Architect |

#### 38.3 Master Metadata

```
Project Name:       SMART AEGIS (Automated Emergency Routing System)
Document Identifier: AEGIS-MAN-HW-2026-PHASE15
Version Number:     2.0.0
Release Date:       October 31, 2026
Document Owner:     Hardware Integration & Deployment Directorate
Hardware Revision:  HW-REV-3.0 (4-Pi Cluster + Traffic Light LED Modules + LM393 IR)
Software Revision:  v1.0-hackathon-core (Phases 1-15 Verified)
Operating System:   QNX Neutrino RTOS 8.0 Starter Kit (aarch64le)
Compute Platform:   Raspberry Pi 4 Model B (4 GB RAM, Quad Cortex-A72 @ 1.5 GHz)
Verification Status: APPROVED FOR LIVE HARDWARE COMMISSIONING & DEMONSTRATION
```
