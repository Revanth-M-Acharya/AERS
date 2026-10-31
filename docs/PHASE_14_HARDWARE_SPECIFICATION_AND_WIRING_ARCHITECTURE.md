# SMART AEGIS (AERS) — Phase 14 Specification & Technical Dossier
## Prototype Direct GPIO LED Wiring Architecture, Complete GPIO Reservation & Hardware Connection Specification

**Document Identifier:** `AEGIS-DOC-HW-2026-PHASE14`  
**Classification:** Hardware Engineering Architecture, Electrical Connection & Wiring Manual  
**Target Hardware:** 4x Raspberry Pi 4 Model B (4 GB RAM, Quad Cortex-A72 @ 1.5 GHz)  
**Target Operating System:** QNX Neutrino RTOS 8.0 Starter Kit (`aarch64le`)  
**Hardware Layer:** Symbolic GPIO HAL (Phase 11), Direct GPIO Prototype LED Driver, Optical IR Driver  
**Status:** **APPROVED FOR FINAL HACKATHON HARDWARE IMPLEMENTATION**

---

### Executive Summary

SMART AEGIS (**Automated Emergency Routing & Smart Traffic Corridor Preemption System**) Phase 14 provides the complete, authoritative electrical engineering and hardware connection documentation required to physically assemble, wire, and commission the SMART AEGIS multi-node cluster.

Building upon the software core completed and frozen across Phases 1–13:
- **Zero Software Redesign:** All Coordinator intelligence, Dijkstra route planning, green corridor scheduling, priority arbitration, hospital selection, Digital Twin telemetry, and Command Center monitoring logic remain 100% intact.
- **Zero Modification to Application Logic:** No algorithms, network protocols, or FSM transitions are altered.
- **Native QNX RTOS & Symbolic HAL Compatibility:** All hardware interactions strictly use the native C++20 `SymbolicGpioHAL`, `PrototypeLedDriver`, and `NodeConfigManager` developed in Phase 11. No Linux-specific APIs (`/sys/class/gpio`, `wiringPi`, `pigpio`, `bcm2835`) are permitted.
- **Complete Elimination of Relays & Traffic Modules:** High-voltage 12V auxiliary power supplies, electromechanical relays, flyback diodes, and 3-light traffic lamp modules are completely removed. Direct Raspberry Pi GPIO LED control with 330 Ohm current-limiting resistors is used across all intersections.
- **Node-Specific Prototype LED Hardware Allocation:**
  - **Coordinator (Raspberry Pi 1):** Zero LEDs connected. Dedicated exclusively to Coordinator intelligence, Dijkstra routing, priority scheduling, and multi-node TCP/IP communication.
  - **Intersection 1 (Raspberry Pi 2):** Exactly TWO LEDs: Red LED on BCM GPIO 5 (Physical Header Pin 29) and Blue LED on BCM GPIO 6 (Physical Header Pin 31).
  - **Intersection 2 (Raspberry Pi 3):** Exactly TWO LEDs: Red LED on BCM GPIO 5 (Physical Header Pin 29) and Blue LED on BCM GPIO 6 (Physical Header Pin 31).
  - **Intersection 3 (Raspberry Pi 4):** Due to prototype hardware shortage, exactly ONE LED: Blue LED on BCM GPIO 6 (Physical Header Pin 31). Zero Red LED connected.
- **Live Demonstration LED States:**
  - **Normal Operation:** Intersection 1 & 2: Blue OFF, Red OFF. Intersection 3: Blue OFF.
  - **Emergency Path Granted:** Intersection 1 & 2: Blue ON, Red OFF. Intersection 3: Blue ON.
  - **Emergency Path Finished / Corridor Closed:** Intersection 1, 2, 3: Blue OFF (and Red OFF).
- **Single 5.1V DC Power Architecture:** The entire cluster operates safely from a single regulated 5.1V DC power supply.

---

### Table of Contents

1. [Symbolic GPIO Reservation Table](#1-symbolic-gpio-reservation-table)
2. [Reserved GPIO Allocation Matrix](#2-reserved-gpio-allocation-matrix)
3. [Approved BCM Mapping Table](#3-approved-bcm-mapping-table)
4. [Complete Hardware Block Diagram](#4-complete-hardware-block-diagram)
5. [Per-Node Hardware Block Diagrams](#5-per-node-hardware-block-diagrams)
6. [Complete Wiring Architecture](#6-complete-wiring-architecture)
7. [Prototype Direct GPIO LED Wiring Architecture](#7-prototype-direct-gpio-led-wiring-architecture)
8. [IR Sensor Wiring Architecture](#8-ir-sensor-wiring-architecture)
9. [OLED Display Wiring Architecture](#9-oled-display-wiring-architecture)
10. [Emergency Switch Wiring Architecture](#10-emergency-switch-wiring-architecture)
11. [Police Override Switch Wiring Architecture](#11-police-override-switch-wiring-architecture)
12. [Acoustic Buzzer Wiring Architecture](#12-acoustic-buzzer-wiring-architecture)
13. [Power Distribution Architecture & Electrical Load Budget](#13-power-distribution-architecture--electrical-load-budget)
14. [Network Topology Diagram](#14-network-topology-diagram)
15. [Connector Specification Table](#15-connector-specification-table)
16. [Cable Schedule](#16-cable-schedule)
17. [Configuration File Templates](#17-configuration-file-templates)
18. [Hardware Commissioning Checklist (Pre-Power Cold Inspection)](#18-hardware-commissioning-checklist)
19. [Hardware Validation Checklist (Powered Functional Verification)](#19-hardware-validation-checklist)
20. [Safety Verification & Fault Mitigation Procedures](#20-safety-verification--fault-mitigation-procedures)

---

### 1. Symbolic GPIO Reservation Table

In strict accordance with Phase 14 engineering constraints, every electrical signal across the four physical nodes is designated by a **Symbolic Identifier**.

| Symbolic Identifier | Target Node | Direction | Electrical Standard | Polarity | Purpose & Functional Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`HOME_EMERGENCY_SWITCH`** | Coordinator (RPi 1) | INPUT | 3.3V LVTTL (Pull-Up) | ACTIVE_LOW | Momentary push button at HOME station; initiates emergency corridor dispatch. |
| **`POLICE_OVERRIDE_SWITCH`** | Coordinator (RPi 1) | INPUT | 3.3V LVTTL (Pull-Up) | ACTIVE_LOW | Police verified incident switch; triggers zero-pause dynamic route replanning. |
| **`HOME_BUZZER`** | Coordinator (RPi 1) | OUTPUT | 3.3V LVTTL -> 5V BJT | ACTIVE_HIGH | Acoustic preemption buzzer generating 5 mission confirmation sound patterns. |
| **`V2X_OLED_SDA`** | Coordinator (RPi 1) | BIDIR | 3.3V Open-Drain | I2C DATA | I2C Serial Data bus for 0.96" SSD1306 V2X advisory in-vehicle display. |
| **`V2X_OLED_SCL`** | Coordinator (RPi 1) | OUTPUT | 3.3V Open-Drain | I2C CLOCK | I2C Serial Clock bus (400 kHz Fast Mode) for V2X advisory display. |
| **`INT1_LED_RED`** | Intersection 1 (RPi 2) | OUTPUT | 3.3V LVTTL | ACTIVE_HIGH | Direct GPIO drive: Intersection 1 Red Stop LED (330 Ohm current limiter). |
| **`INT1_LED_BLUE`** | Intersection 1 (RPi 2) | OUTPUT | 3.3V LVTTL | ACTIVE_HIGH | Direct GPIO drive: Intersection 1 Blue Emergency Corridor LED (330 Ohm limiter). |
| **`INT1_APPROACH_IR`** | Intersection 1 (RPi 2) | INPUT | 3.3V Open-Collector | ACTIVE_LOW | Active optical IR proximity sensor detecting approaching vehicles on Road A. |
| **`INT2_LED_RED`** | Intersection 2 (RPi 3) | OUTPUT | 3.3V LVTTL | ACTIVE_HIGH | Direct GPIO drive: Intersection 2 Red Stop LED (330 Ohm current limiter). |
| **`INT2_LED_BLUE`** | Intersection 2 (RPi 3) | OUTPUT | 3.3V LVTTL | ACTIVE_HIGH | Direct GPIO drive: Intersection 2 Blue Emergency Corridor LED (330 Ohm limiter). |
| **`INT2_APPROACH_IR`** | Intersection 2 (RPi 3) | INPUT | 3.3V Open-Collector | ACTIVE_LOW | Optical IR sensor on Approach A; detects primary emergency ambulance. |
| **`INT3_LED_BLUE`** | Intersection 3 (RPi 4) | OUTPUT | 3.3V LVTTL | ACTIVE_HIGH | Direct GPIO drive: Intersection 3 Blue Emergency Corridor LED (330 Ohm limiter). |
| **`INT3_APPROACH_IR`** | Intersection 3 (RPi 4) | INPUT | 3.3V Open-Collector | ACTIVE_LOW | Active optical IR proximity sensor on Road A hospital terminal access link. |

> [!NOTE]
> Due to prototype hardware availability constraints, **Intersection 3 (RPi 4) operates with only ONE LED (Blue LED)**. No Red LED is connected on Node 3.
> **Coordinator (RPi 1) operates with ZERO LEDs connected**, dedicating 100% of compute and GPIO bus bandwidth to Coordinator intelligence and TCP communications.

---

### 2. Reserved GPIO Allocation Matrix

The matrix below segregates all reserved symbolic signals across the four discrete physical Raspberry Pi compute boards:

```
+---------------------------------------------------------------------------------------------------+
|                            SMART AEGIS 4-NODE PHYSICAL CLUSTER MATRIX                             |
+---------------------------------------------------------------------------------------------------+
| [ RPi 1: Master Coordinator ] (192.168.1.100) — ZERO LEDS CONNECTED                              |
|   - HOME_EMERGENCY_SWITCH   (Input, Active-Low, Internal Pull-Up, Pin 16)                         |
|   - POLICE_OVERRIDE_SWITCH  (Input, Active-Low, Internal Pull-Up, Pin 18)                         |
|   - HOME_BUZZER             (Output, Active-High, 2N2222 Transistor Buffer, Pin 31)               |
|   - V2X_OLED_SDA            (I2C-1 Data, 3.3V Open-Drain, Pin 3)                                  |
|   - V2X_OLED_SCL            (I2C-1 Clock, 3.3V Open-Drain, 400 kHz, Pin 5)                        |
+---------------------------------------------------------------------------------------------------+
| [ RPi 2: Traffic Node 1 - Intersection 1 ] (192.168.1.101) — EXACTLY TWO LEDS                     |
|   - INT1_LED_RED            (Output, Active-High -> Red LED, BCM 5, Pin 29)                       |
|   - INT1_LED_BLUE           (Output, Active-High -> Blue LED, BCM 6, Pin 31)                      |
|   - INT1_APPROACH_IR        (Input, Active-Low, LM393 Optical Proximity, BCM 12, Pin 32)          |
+---------------------------------------------------------------------------------------------------+
| [ RPi 3: Traffic Node 2 - Intersection 2 Priority Hub ] (192.168.1.102) — EXACTLY TWO LEDS        |
|   - INT2_LED_RED            (Output, Active-High -> Red LED, BCM 5, Pin 29)                       |
|   - INT2_LED_BLUE           (Output, Active-High -> Blue LED, BCM 6, Pin 31)                      |
|   - INT2_APPROACH_IR        (Input, Active-Low, LM393 Optical Proximity, BCM 12, Pin 32)          |
+---------------------------------------------------------------------------------------------------+
| [ RPi 4: Traffic Node 3 - Intersection 3 Hospital Gate ] (192.168.1.103) — EXACTLY ONE LED (BLUE) |
|   - INT3_LED_BLUE           (Output, Active-High -> Blue LED, BCM 6, Pin 31)                      |
|   - (Zero Red LED Connected — Hardware Shortage Specification)                                    |
|   - INT3_APPROACH_IR        (Input, Active-Low, LM393 Optical Proximity, BCM 12, Pin 32)          |
+---------------------------------------------------------------------------------------------------+
```

#### Node Inventory Summary
- **Coordinator Node (RPi 1):** 5 signals (2 inputs, 1 output, 2 I2C pins). **Zero LEDs**.
- **Traffic Node 1 (RPi 2):** 3 signals (2 LED outputs: Red Pin 29, Blue Pin 31; 1 IR input: Pin 32).
- **Traffic Node 2 (RPi 3):** 3 signals (2 LED outputs: Red Pin 29, Blue Pin 31; 1 IR input: Pin 32).
- **Traffic Node 3 (RPi 4):** 2 signals (1 LED output: Blue Pin 31; 1 IR input: Pin 32).
- **Total Physical Cluster Signals:** **13 active I/O signals** mapped cleanly with zero pin contention.

---

### 3. Approved BCM Mapping Table

| Symbolic Identifier | Hardware Node | Signal Type | Target BCM Pin | Physical Header Pin | Electrical Level | Final Status |
| :--- | :--- | :--- | :---: | :---: | :---: | :--- |
| `HOME_EMERGENCY_SWITCH` | Coordinator (RPi 1) | Digital Input | BCM 23 | Pin 16 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `POLICE_OVERRIDE_SWITCH` | Coordinator (RPi 1) | Digital Input | BCM 24 | Pin 18 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `HOME_BUZZER` | Coordinator (RPi 1) | Digital Output | BCM 6 | Pin 31 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `V2X_OLED_SDA` | Coordinator (RPi 1) | I2C Data | BCM 2 | Pin 3 | 3.3V Open-Drain | **ACTIVE APPROVED** |
| `V2X_OLED_SCL` | Coordinator (RPi 1) | I2C Clock | BCM 3 | Pin 5 | 3.3V Open-Drain | **ACTIVE APPROVED** |
| `INT1_LED_RED` | Intersection 1 (RPi 2) | LED Signal Driver | BCM 5 | Pin 29 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT1_LED_BLUE` | Intersection 1 (RPi 2) | LED Signal Driver | BCM 6 | Pin 31 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT1_APPROACH_IR` | Intersection 1 (RPi 2) | Proximity Input | BCM 12 | Pin 32 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT2_LED_RED` | Intersection 2 (RPi 3) | LED Signal Driver | BCM 5 | Pin 29 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT2_LED_BLUE` | Intersection 2 (RPi 3) | LED Signal Driver | BCM 6 | Pin 31 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT2_APPROACH_IR` | Intersection 2 (RPi 3) | Proximity Input | BCM 12 | Pin 32 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT3_LED_BLUE` | Intersection 3 (RPi 4) | LED Signal Driver | BCM 6 | Pin 31 | 3.3V LVTTL | **ACTIVE APPROVED** |
| `INT3_APPROACH_IR` | Intersection 3 (RPi 4) | Proximity Input | BCM 12 | Pin 32 | 3.3V LVTTL | **ACTIVE APPROVED** |

---

### 4. Complete Hardware Block Diagram

The complete SMART AEGIS hardware ecosystem connects four dedicated single-board computers via a local TCP/IP networking fabric, powered by a single regulated 5.1V DC supply with direct solid-state GPIO control of traffic signal LEDs and optical detectors.

```
===================================================================================================
                               SMART AEGIS SYSTEM HARDWARE ARCHITECTURE
===================================================================================================

       +--------------------------------------------------------------------------+
       |             REGULATED 5.1V DC POWER SUPPLY (Mean Well LRS-75-5)          |
       +--------------------------------------------------------------------------+
             |                     |                     |                     |
     +5.1V / GND (USB-C)   +5.1V / GND (USB-C)   +5.1V / GND (USB-C)   +5.1V / GND (USB-C)
             |                     |                     |                     |
             v                     v                     v                     v
     +---------------+     +---------------+     +---------------+     +---------------+
     | RPi 1: COORD  |     | RPi 2: INT 1  |     | RPi 3: INT 2  |     | RPi 4: INT 3  |
     | 192.168.1.100 |     | 192.168.1.101 |     | 192.168.1.102 |     | 192.168.1.103 |
     +---------------+     +---------------+     +---------------+     +---------------+
       |   |   |                 |       |             |       |             |       |
       |   |   |                 |       v             |       v             |       v
       |   |   +-> OLED (I2C)    |   +-------+         |   +-------+         |   +-------+
       |   +-----> Buzzer        |   |LM393  |         |   |LM393  |         |   |LM393  |
       +---------> Switches      |   |IR Sens|         |   |IR Sens|         |   |IR Sens|
       (ZERO LEDs Connected)     |   +-------+         |   +-------+         |   +-------+
                                 v                     v                     v
                           +-----------+         +-----------+         +-----------+
                           | 2 LEDs    |         | 2 LEDs    |         | 1 LED     |
                           | RED: Pin29|         | RED: Pin29|         | BLUE:Pin31|
                           | BLUE:Pin31|         | BLUE:Pin31|         |(No Red LED|
                           +-----------+         +-----------+         +-----------+

       <===================== 100/1000 Mbps TCP/IP Ethernet / Wi-Fi Mesh =====================>
```

---

### 5. Per-Node Hardware Block Diagrams

#### 5.1 Coordinator Node (Raspberry Pi 1)

The Coordinator node handles emergency mission intake, V2X advisories, acoustic dispatch alerts, and overall RTOS supervisory orchestration. It has **zero LEDs connected**.

```
+---------------------------------------------------------------------------------------------------+
|                        COORDINATOR NODE HARDWARE ARCHITECTURE (RPi 1)                             |
|                        (ZERO LEDS CONNECTED — PURE TCP & COORDINATOR LOGIC)                      |
+---------------------------------------------------------------------------------------------------+
  [ Emergency Push Button ] -----> Pin 16 (BCM 23: HOME_EMERGENCY_SWITCH) ----+
  [ Police Detour Switch ]  -----> Pin 18 (BCM 24: POLICE_OVERRIDE_SWITCH) ---+
                                                                              |--> BCM2711 SoC
  [ 5V Active Buzzer ] <---------- Pin 31 (BCM 6: HOME_BUZZER) -> [2N2222] <--+    (QNX 8.0)
                                                                              |
  [ SSD1306 0.96" OLED ] <=======> Pin 3 (BCM 2: SDA) & Pin 5 (BCM 3: SCL) -+
                                   Powered via Pin 1 (+3.3V) & Pin 9 (GND)
+---------------------------------------------------------------------------------------------------+
```

#### 5.2 Intersection 1 Node (Raspberry Pi 2)

Controls the northern gateway intersection. Equipped with exactly **two LEDs** (Red on Pin 29, Blue on Pin 31) and one optical vehicle approach sensor.

```
+---------------------------------------------------------------------------------------------------+
|                       INTERSECTION 1 NODE HARDWARE ARCHITECTURE (RPi 2)                           |
+---------------------------------------------------------------------------------------------------+
  [ Approach IR Sensor ] --------> Pin 32 (BCM 12: INT1_APPROACH_IR) ----------+
                                   Powered via Pin 1 (+3.3V) & Pin 9 (GND)    |
                                                                              |--> BCM2711 SoC
  [ Direct GPIO LEDs ]                                                        |    (QNX 8.0)
    - Red Stop LED        <------- Pin 29 (BCM 5: INT1_LED_RED) [330R] -------+
    - Blue Corridor LED   <------- Pin 31 (BCM 6: INT1_LED_BLUE) [330R] ------+
    - Common Ground Return -------> Pin 30 (GND) -----------------------------+
+---------------------------------------------------------------------------------------------------+
```

#### 5.3 Intersection 2 Node (Raspberry Pi 3 — Priority Hub)

Hosts the priority arbitration system for emergency response. Equipped with exactly **two LEDs** (Red on Pin 29, Blue on Pin 31) and optical vehicle approach detection.

```
+---------------------------------------------------------------------------------------------------+
|                       INTERSECTION 2 NODE HARDWARE ARCHITECTURE (RPi 3)                           |
+---------------------------------------------------------------------------------------------------+
  [ Approach IR Sensor ] --------> Pin 32 (BCM 12: INT2_APPROACH_IR) ----------+
                                   Powered via Pin 1 (+3.3V) & Pin 9 (GND)    |
                                                                              |--> BCM2711 SoC
  [ Direct GPIO LEDs ]                                                        |    (QNX 8.0)
    - Red Stop LED        <------- Pin 29 (BCM 5: INT2_LED_RED) [330R] -------+
    - Blue Corridor LED   <------- Pin 31 (BCM 6: INT2_LED_BLUE) [330R] ------+
    - Common Ground Return -------> Pin 30 (GND) -----------------------------+
+---------------------------------------------------------------------------------------------------+
```

#### 5.4 Intersection 3 Node (Raspberry Pi 4)

Controls the terminal approach into the regional HOSPITAL trauma entrance. Due to prototype hardware availability constraints, it has **only ONE LED (Blue on Pin 31)** and zero Red LED.

```
+---------------------------------------------------------------------------------------------------+
|                       INTERSECTION 3 NODE HARDWARE ARCHITECTURE (RPi 4)                           |
|                       (HARDWARE SHORTAGE: BLUE LED ONLY — ZERO RED LED)                          |
+---------------------------------------------------------------------------------------------------+
  [ Hospital IR Sensor ] --------> Pin 32 (BCM 12: INT3_APPROACH_IR) ----------+
                                   Powered via Pin 1 (+3.3V) & Pin 9 (GND)    |
                                                                              |--> BCM2711 SoC
  [ Direct GPIO LED ]                                                         |    (QNX 8.0)
    - Blue Corridor LED   <------- Pin 31 (BCM 6: INT3_LED_BLUE) [330R] ------+
    - (Red LED: NONE)                                                         |
    - Common Ground Return -------> Pin 30 (GND) -----------------------------+
+---------------------------------------------------------------------------------------------------+
```

---

### 6. Complete Wiring Architecture

The physical connection topology maps discrete electrical conductors between Raspberry Pi GPIO headers, discrete LEDs, optical detectors, switches, and displays:

```
===================================================================================================
                               SMART AEGIS PHYSICAL WIRING TOPOLOGY
===================================================================================================

[ COORDINATOR NODE: RPi 1 ]
  Signal: HOME_EMERGENCY_SWITCH  ------> Pin 16 (BCM 23) -> Push Button -> Pin 14 (GND)
  Signal: POLICE_OVERRIDE_SWITCH ------> Pin 18 (BCM 24) -> Toggle SW   -> Pin 14 (GND)
  Signal: HOME_BUZZER            ------> Pin 31 (BCM 6)  -> 1k Resistor -> Base 2N2222 -> Buzzer
  Signal: V2X_OLED_SDA           ------> Pin 3  (BCM 2)  -> Pin SDA of 0.96" I2C OLED (SSD1306)
  Signal: V2X_OLED_SCL           ------> Pin 5  (BCM 3)  -> Pin SCL of 0.96" I2C OLED (SSD1306)
  Supply: +3.3V DC (Pin 1)       ------> Pin VCC of OLED Display
  Supply: GND (Pin 9)            ------> Pin GND of OLED Display & Common Logic Return
  (NO LEDS CONNECTED ON COORDINATOR)

[ INTERSECTION 1: RPi 2 ]
  Signal: INT1_LED_RED           ------> Pin 29 (BCM 5) -> 330 Ohm -> Anode Red LED -> Cathode -> Pin 30 (GND)
  Signal: INT1_LED_BLUE          ------> Pin 31 (BCM 6) -> 330 Ohm -> Anode Blue LED -> Cathode -> Pin 30 (GND)
  Signal: INT1_APPROACH_IR       ------> Pin 32 (BCM 12) <- OUT of LM393 Optical IR Sensor
  Supply: +3.3V DC (Pin 1)       ------> VCC of LM393 Optical IR Sensor
  Return: GND (Pin 9)            ------> GND of LM393 Optical IR Sensor

[ INTERSECTION 2: RPi 3 ]
  Signal: INT2_LED_RED           ------> Pin 29 (BCM 5) -> 330 Ohm -> Anode Red LED -> Cathode -> Pin 30 (GND)
  Signal: INT2_LED_BLUE          ------> Pin 31 (BCM 6) -> 330 Ohm -> Anode Blue LED -> Cathode -> Pin 30 (GND)
  Signal: INT2_APPROACH_IR       ------> Pin 32 (BCM 12) <- OUT of LM393 Optical IR Sensor
  Supply: +3.3V DC (Pin 1)       ------> VCC of LM393 Optical IR Sensor
  Return: GND (Pin 9)            ------> GND of LM393 Optical IR Sensor

[ INTERSECTION 3: RPi 4 ]
  Signal: INT3_LED_BLUE          ------> Pin 31 (BCM 6) -> 330 Ohm -> Anode Blue LED -> Cathode -> Pin 30 (GND)
  (ZERO RED LED CONNECTED — HARDWARE SHORTAGE SPECIFICATION)
  Signal: INT3_APPROACH_IR       ------> Pin 32 (BCM 12) <- OUT of LM393 Optical IR Sensor
  Supply: +3.3V DC (Pin 1)       ------> VCC of LM393 Optical IR Sensor
  Return: GND (Pin 9)            ------> GND of LM393 Optical IR Sensor
===================================================================================================
```

---

### 7. Prototype Direct GPIO LED Wiring Architecture

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

### 8. IR Sensor Wiring Architecture

The LM393 Optical IR sensor detects inbound emergency vehicles via infrared beam reflection:

```
+---------------------------------------------------------------------------------------------------+
|                              LM393 OPTICAL IR SENSOR WIRING DIAGRAM                               |
+---------------------------------------------------------------------------------------------------+
  Raspberry Pi 4 Header Pin               LM393 Proximity Sensor Module
  
  [Pin 1 ] +3.3V Power Rail     --------> [ VCC ] Supply Input
  [Pin 9 ] Ground (GND)         --------> [ GND ] Ground Return
  [Pin 32] BCM 12 (IR_INPUT)    <-------- [ OUT ] Digital Open-Collector Output (Active-Low)
+---------------------------------------------------------------------------------------------------+
```

#### Electrical Specifications & Integration Rules
- **Operating Voltage:** Powered strictly from the Raspberry Pi $3.3\text{V}$ rail (Pin 1) to ensure the `OUT` signal remains at $3.3\text{V}$ LVTTL logic levels without requiring voltage dividers.
- **Detection Threshold:** Calibrated via the on-board multi-turn cermet potentiometer for an optical trip distance of $5\text{ cm} - 10\text{ cm}$.
- **Debounce Invariant:** The software driver applies a non-blocking $30\text{ ms}$ debouncing filter to eliminate false triggers from optical reflections.

---

### 9. OLED Display Wiring Architecture

The 0.96" SSD1306 $128 \times 64$ monochrome OLED communicates over the dedicated hardware I2C-1 bus:

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

---

### 10. Emergency Switch Wiring Architecture

The HOME emergency push button triggers immediate green corridor dispatch across all three intersections:

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

- **Logic:** Active-Low with internal $50\text{ k}\Omega$ pull-up on BCM 23.
- **Debouncing:** $50\text{ ms}$ software validation window eliminates mechanical switch contact chatter.

---

### 11. Police Override Switch Wiring Architecture

A physical toggle switch simulates a verified police traffic advisory reporting an obstacle on arterial segment 1-2:

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

- **Zero-Pause Guarantee:** When closed, the Coordinator ingests the event as a verified road condition advisory, recalculating an optimal detour route with zero mission pause.

---

### 12. Acoustic Buzzer Wiring Architecture

A 5V active piezoelectric buzzer provides acoustic confirmation of preemption milestones:

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

### 13. Power Distribution Architecture & Electrical Load Budget

#### 13.1 Electrical Load Budget & Current Consumption

| Component | Operating Voltage | Nominal Current (Each) | Peak Inrush (Each) | Quantity | Total Peak Current | Power Consumption |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Raspberry Pi 4 Model B** | 5.1V DC | 850 mA | 2,500 mA | 4 | 10.00 A | 51.0 W |
| **Discrete LEDs (with 330R)** | 3.3V DC | 3 mA (per lit LED) | 5 mA (all on) | 5 LEDs | 0.025 A | 0.08 W |
| **Active Optical IR Sensors** | 3.3V DC | 15 mA | 20 mA | 3 sensors | 0.060 A | 0.20 W |
| **0.96" SSD1306 OLED Display** | 3.3V DC | 20 mA | 35 mA | 1 display | 0.035 A | 0.12 W |
| **Piezo Preemption Buzzer** | 5.0V DC | 25 mA | 40 mA | 1 buzzer | 0.040 A | 0.20 W |
| **TOTALS** | — | — | — | — | **5.1V DC: 10.16 A** | **51.60 W** |

#### 13.2 Power Supply Sizing & Recommendation
- **Single 5.1V DC Supply:** Rated at **$5.10\text{V} \text{ DC} @ \mathbf{14.0\text{ A}}$** (Mean Well LRS-75-5, 71.4W).
- **Safety Margin:** Provides a $37\%$ continuous power margin above peak system draw ($10.16\text{A}$), maintaining voltage stability across all microkernels during burst CPU and networking operations.

---

### 14. Network Topology Diagram

All four nodes communicate over a dedicated low-latency TCP/IP network fabric:

```
===================================================================================================
                               SMART AEGIS TCP/IP NETWORK TOPOLOGY
===================================================================================================

    +------------------------------------------------------------------------+
    |           Industrial Wi-Fi / Ethernet Switch (Subnet 192.168.1.0/24)   |
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

### 15. Connector Specification Table

| Symbolic Signal | Source Device | Voltage Level | Connector Type (Source) | Connector Type (Target) | Cable Specification | Target Peripheral |
| :--- | :--- | :---: | :--- | :--- | :--- | :--- |
| `HOME_EMERGENCY_SWITCH` | RPi 1 Breakout | 3.3V | 2.54mm Dupont 2-Pin | Quick-Disconnect 6.3mm Spade | 22 AWG Twisted Pair | Heavy-Duty Push Button |
| `POLICE_OVERRIDE_SWITCH` | RPi 1 Breakout | 3.3V | 2.54mm Dupont 2-Pin | Screw Terminal Phoenix Contact | 22 AWG Shielded Pair | Field Police Switch |
| `HOME_BUZZER` | RPi 1 Breakout | 3.3V | 2.54mm Dupont 1-Pin | Solder Pad / JST-XH 2-Pin | 24 AWG Stranded Wire | BJT Base Drive Circuit |
| `V2X_OLED_SDA` | RPi 1 Breakout | 3.3V | 2.54mm Dupont 1-Pin | JST-XH 4-Pin / Female Dupont | 26 AWG Ribbon Conductor | 0.96" SSD1306 Display |
| `V2X_OLED_SCL` | RPi 1 Breakout | 3.3V | 2.54mm Dupont 1-Pin | JST-XH 4-Pin / Female Dupont | 26 AWG Ribbon Conductor | 0.96" SSD1306 Display |
| `INT1_LED_RED` | RPi 2 Header Pin 29 | 3.3V | 2.54mm Female Dupont | 330 Ohm to LED Anode | 24 AWG Stranded Wire | Red 5mm LED |
| `INT1_LED_BLUE` | RPi 2 Header Pin 31 | 3.3V | 2.54mm Female Dupont | 330 Ohm to LED Anode | 24 AWG Stranded Wire | Blue 5mm LED |
| `INT2_LED_RED` | RPi 3 Header Pin 29 | 3.3V | 2.54mm Female Dupont | 330 Ohm to LED Anode | 24 AWG Stranded Wire | Red 5mm LED |
| `INT2_LED_BLUE` | RPi 3 Header Pin 31 | 3.3V | 2.54mm Female Dupont | 330 Ohm to LED Anode | 24 AWG Stranded Wire | Blue 5mm LED |
| `INT3_LED_BLUE` | RPi 4 Header Pin 31 | 3.3V | 2.54mm Female Dupont | 330 Ohm to LED Anode | 24 AWG Stranded Wire | Blue 5mm LED |
| `INTx_LED_GND` | RPi Header Pin 30 | 0V | 2.54mm Female Dupont | LED Common Cathode | 24 AWG Stranded Wire | Common Ground Return |
| `INTx_APPROACH_IR` | Optical Module | 3.3V | 2.54mm Dupont 3-Pin | 2.54mm Header Pin 32 | 24 AWG 3-Conductor Ribbon| LM393 Optical Detector |

---

### 16. Cable Schedule

| Wire ID | Origin | Destination | Symbolic Signal | Purpose | Gauge | Insulation Color | Termination Type |
| :---: | :--- | :--- | :--- | :--- | :---: | :---: | :--- |
| **W-101** | RPi 1 Pin 16 | SW_EMERG Pin 1 | `HOME_EMERGENCY_SWITCH` | Emergency Mission Trigger | 22 AWG | **Blue** | 2.54mm Pin to Spade |
| **W-102** | SW_EMERG Pin 2 | RPi 1 Pin 14 | `GND_LOGIC` | Switch Ground Return | 22 AWG | **Black** | Spade to 2.54mm Pin |
| **W-103** | RPi 1 Pin 18 | SW_POLICE Pin 1 | `POLICE_OVERRIDE_SWITCH` | Police Advisory Detour | 22 AWG | **Yellow** | 2.54mm Pin to Ferrule |
| **W-104** | SW_POLICE Pin 2 | RPi 1 Pin 20 | `GND_LOGIC` | Police Switch Return | 22 AWG | **Black** | Ferrule to 2.54mm Pin |
| **W-105** | RPi 1 Pin 31 | BJT Base Resistor | `HOME_BUZZER` | Buzzer Tone Gate | 24 AWG | **Orange** | 2.54mm Pin to Solder |
| **W-106** | RPi 1 Pin 3 | OLED Pin SDA | `V2X_OLED_SDA` | I2C Data Signal | 26 AWG | **Green** | Female-to-Female Dupont |
| **W-107** | RPi 1 Pin 5 | OLED Pin SCL | `V2X_OLED_SCL` | I2C Clock Signal | 26 AWG | **White** | Female-to-Female Dupont |
| **W-201** | RPi 2 Pin 29 | Red LED Anode | `INT1_LED_RED` | Int 1 Red Stop LED (330R)| 24 AWG | **Red** | Female Dupont to Solder |
| **W-202** | RPi 2 Pin 31 | Blue LED Anode | `INT1_LED_BLUE` | Int 1 Blue Corridor (330R)| 24 AWG | **Blue** | Female Dupont to Solder |
| **W-203** | RPi 2 Pin 30 | LED Cathodes | `GND_INT1` | Int 1 LED Ground Return | 24 AWG | **Black** | Female Dupont to Solder |
| **W-204** | IR Sensor 1 OUT| RPi 2 Pin 32 | `INT1_APPROACH_IR` | Int 1 Approach Vehicle | 24 AWG | **Purple** | Female-to-Female Dupont |
| **W-301** | RPi 3 Pin 29 | Red LED Anode | `INT2_LED_RED` | Int 2 Red Stop LED (330R)| 24 AWG | **Red** | Female Dupont to Solder |
| **W-302** | RPi 3 Pin 31 | Blue LED Anode | `INT2_LED_BLUE` | Int 2 Blue Corridor (330R)| 24 AWG | **Blue** | Female Dupont to Solder |
| **W-303** | RPi 3 Pin 30 | LED Cathodes | `GND_INT2` | Int 2 LED Ground Return | 24 AWG | **Black** | Female Dupont to Solder |
| **W-304** | IR Sensor 2 OUT| RPi 3 Pin 32 | `INT2_APPROACH_IR` | Int 2 Approach Vehicle | 24 AWG | **Purple** | Female-to-Female Dupont |
| **W-401** | RPi 4 Pin 31 | Blue LED Anode | `INT3_LED_BLUE` | Int 3 Blue Corridor (330R)| 24 AWG | **Blue** | Female Dupont to Solder |
| **W-402** | RPi 4 Pin 30 | Blue LED Cathode | `GND_INT3` | Int 3 LED Ground Return | 24 AWG | **Black** | Female Dupont to Solder |
| **W-403** | IR Sensor 3 OUT| RPi 4 Pin 32 | `INT3_APPROACH_IR` | Int 3 Hospital Vehicle | 24 AWG | **Purple** | Female-to-Female Dupont |
| **PWR-01** | 5V PSU (+) | RPi 1-4 USB-C | `+5.1V_MAIN` | RPi Microprocessor Power | 18 AWG | **Red** | Screw Terminal to USB-C |
| **PWR-02** | 5V PSU (-) | Star Ground Bus | `GND_MAIN` | Main DC Return | 18 AWG | **Black** | Ferrule to Ring Lug |

---

### 17. Configuration File Templates

Four ready-to-deploy configuration templates have been authored and placed into [`config/cluster/templates/`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/config/cluster/templates/). They strictly contain **Symbolic Signal Names** and zero hardcoded numbers, maintaining complete compatibility with [`NodeConfigManager`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/hardware_integration/config/NodeConfigManager.cpp).

1. [`rpi1_coordinator_symbolic.conf`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/config/cluster/templates/rpi1_coordinator_symbolic.conf)
2. [`rpi2_intersection1_symbolic.conf`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/config/cluster/templates/rpi2_intersection1_symbolic.conf)
3. [`rpi3_intersection2_symbolic.conf`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/config/cluster/templates/rpi3_intersection2_symbolic.conf)
4. [`rpi4_intersection3_symbolic.conf`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX hackathon/AERS/config/cluster/templates/rpi4_intersection3_symbolic.conf)

---

### 18. Hardware Commissioning Checklist

> [!CAUTION]
> **PRE-POWER COLD INSPECTION:** Perform these verification steps with power supply completely disconnected.

- [ ] **Step 1: Visual Inspection & Workmanship**
  - Verify zero solder bridges or loose strands across Raspberry Pi GPIO pins.
  - Verify discrete LEDs have inline $330\,\Omega$ current-limiting resistors soldered to the anode.
  - Verify that Node 1 (Coordinator) has **zero LEDs connected**.
  - Verify that Node 4 (Intersection 3) has **only one LED connected (Blue on Pin 31)** and zero Red LED.
- [ ] **Step 2: Ground Continuity & Isolation**
  - Measure continuity between all Pin 30 GND points and the master DC star ground. Resistance must be $< 0.1\,\Omega$.
- [ ] **Step 3: Pin Polarities & Wiring**
  - Verify Pin 29 connects to Red LED anode (Nodes 2 & 3).
  - Verify Pin 31 connects to Blue LED anode (Nodes 2, 3, & 4).
  - Verify Pin 32 connects to LM393 IR sensor OUT (Nodes 2, 3, & 4).

---

### 19. Hardware Validation Checklist

Execute with 5.1V DC power energized:

- [ ] **Vector 1: Network Mesh Connectivity** — Verify ping round-trip time between Coordinator (`192.168.1.100`) and all three Traffic Nodes (`.101`, `.102`, `.103`) is $< 5.0\text{ ms}$.
- [ ] **Vector 2: Node Configuration Validation** — Run `test_cluster_deployment_hardware` and verify `NodeConfigManager::Validate()` returns clean with zero pin collisions.
- [ ] **Vector 3: Direct GPIO Prototype LED Validation** — Verify `DeploymentValidator`:
  - Coordinator: Zero LEDs expected.
  - Intersection 1: GPIO 5 (Red LED) & GPIO 6 (Blue LED) verified.
  - Intersection 2: GPIO 5 (Red LED) & GPIO 6 (Blue LED) verified.
  - Intersection 3: GPIO 6 (Blue LED) verified; zero Red LED (shortage specification).
- [ ] **Vector 4: Direct Prototype LED Driver State Transitions** — Verify `GrantEmergencyPath()` illuminates Blue LED while turning Red LED off; verify `CloseEmergencyPath()` extinguishes Blue LED.
- [ ] **Vector 5: Optical IR Sensor Debouncing** — Verify that waving a hand over IR sensors trips active detection with $30\text{ ms}$ debounce.
- [ ] **Vector 6: EventBus Communication** — Verify strongly-typed EventBus publish/subscribe confirmed.
- [ ] **Vector 7: Time Synchronization** — Verify NTP / monotonic clock drift across the four boards is within $\pm 5.0\text{ ms}$.

---

### 20. Safety Verification & Fault Mitigation Procedures

1. **Microsecond Solid-State Switching:** The absence of mechanical relays eliminates contact bounce, coil inductance spikes, and mechanical wear.
2. **Software Interlocking:** The `PrototypeLedDriver` enforces mutual exclusion in software so conflicting indications can never occur.
3. **Hardware Shortage Mitigation:** Node 3's missing Red LED is handled gracefully in software. Calls to control the Red LED on Node 3 execute safe no-ops without asserting errors or tripping watchdogs.
4. **Failsafe Default:** Upon system boot or shutdown, all LEDs remain completely extinguished (OFF), ensuring predictable behavior.
