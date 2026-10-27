# SMART AEGIS: Raspberry Pi 4B Pin Mapping & Hardware Interface Specification

```
================================================================================
Document Identifier:  AEGIS-HDS-PIN-001
Title:                Raspberry Pi Pin Mapping & Hardware Interface Specification
Project:              SMART AEGIS (Ambulance Emergency Response System)
Target Baseline:      Pre-Phase 5C Hardware Interface Freeze
Version:              1.0
Author:               Principal Embedded Systems Architect
Target Platform:      Raspberry Pi 4 Model B (Broadcom BCM2711 SoC, 40-Pin Header)
Target Subsystems:    Traffic Signal Node (RSU), Ambulance Node (OBU), Safety Plant
Applicable Standards: ISO 26262 (ASIL-B/D Principles), MISRA C++:2008, NEMA TS 2
Status:               APPROVED FOR HARDWARE BENCH IMPLEMENTATION (PHASE 5C)
================================================================================
```

---

## 1. Purpose

The purpose of this document is to establish the definitive, frozen hardware pin allocation and electrical interfacing specification for the **SMART AEGIS** platform on the **Raspberry Pi 4 Model B** single-board computer (SBC). 

Before writing any physical driver code (Phase 5C: Raspberry Pi GPIO Driver), every hardware peripheral—including signal lamps, proximity sensors, safety switches, watchdog interfaces, and communication buses—must have an unambiguous, electrically validated physical connection to the Raspberry Pi 40-pin GPIO header.

This document serves as the binding contract between the hardware engineering plant (wiring harnesses, opto-isolated relay boards, sensors, and power supplies) and the software driver abstraction layer (`RaspberryPiGpioInterface`).

```
+--------------------------------------------------------------------------------------------------+
|                                  HARDWARE SPECIFICATION CONTRACT                                 |
+--------------------------------------------------------------------------------------------------+
|                                                                                                  |
|   +-----------------------------+                           +--------------------------------+   |
|   |   PHYSICAL PERIPHERALS      |                           |   SOFTWARE ARCHITECTURE        |   |
|   |  - Red / Yellow / Green     |                           |  - TrafficSignalNode           |   |
|   |  - IR Vehicle Sensor        |   ====================    |  - TrafficController           |   |
|   |  - HC-SR04 Ultrasonic       |   THIS PIN SPECIFICATION  |  - SignalStateMachine          |   |
|   |  - Emergency Stop Button    |   (AEGIS-HDS-PIN-001)     |  - PreemptionManager           |   |
|   |  - Manual Override Switch   |   ====================    |  - IGpioInterface (Abstract)  |   |
|   |  - Watchdog Relay Output    |                           |  - RaspberryPiGpioInterface    |   |
|   |  - System Status LED        |                           |    (Concrete Driver)           |   |
|   +-----------------------------+                           +--------------------------------+   |
|                                                                                                  |
+--------------------------------------------------------------------------------------------------+
```

---

## 2. GPIO Design Philosophy

The SMART AEGIS hardware abstraction is governed by three fundamental design rules:

### 2.1 Stability and Immobility of Pin Assignments
Once physical prototypes are wired, wire-wrapped, or fabricated on custom printed circuit boards (PCBs), altering a single GPIO assignment incurs extensive electrical re-qualification, schematic re-verification, and regression testing. Pin assignments documented herein are considered **FROZEN** for Phase 5C.

### 2.2 Strict Prohibition of Hardcoded GPIOs in Business Logic
High-level control logic, state machines, and arbitration algorithms must **NEVER** contain references to BCM GPIO numbers, physical pin indices, or hardware register addresses.
* Modules such as `TrafficController`, `SignalStateMachine`, `PreemptionManager`, and `RecoveryManager` operate purely in terms of semantic states (`SetRed()`, `SetYellow()`, `SetGreen()`, `FlashYellow()`).
* Hardcoding GPIO numbers into business logic is a critical safety violation under ISO 26262, as it breaks portable unit testing and creates hidden coupling to specific silicon.

### 2.3 Strict Single Ownership
Only the concrete driver class (`aegis::edge::traffic::RaspberryPiGpioInterface`) is permitted to access the GPIO subsystem (via `/dev/gpiod` or memory-mapped peripherals). The concrete driver reads pin mappings from configuration profiles (`config.json`) or verified platform header constants.

```
+-----------------------------------------------------------------------------+
|                        ARCHITECTURAL DECOUPLING HIERARCHY                   |
+-----------------------------------------------------------------------------+
|                                                                             |
|  [ TrafficController / FSM ]   <-- Operates purely on abstract semantics    |
|               │                    (NO knowledge of pins or voltages)       |
|               ▼                                                             |
|       [ IGpioInterface ]       <-- Pure C++ Abstract Interface Contract     |
|               │                                                             |
|               ▼                                                             |
|  [ RaspberryPiGpioInterface ]  <-- ONLY class that knows BCM pin numbers    |
|               │                    (Configured via config.json / Constants) |
|               ▼                                                             |
|      [ Linux /dev/gpiod ]      <-- Kernel Character Device Driver           |
|               │                                                             |
|               ▼                                                             |
|  [ BCM2711 40-Pin Header ]     <-- Physical Silicon & Electrical Domain     |
|                                                                             |
+-----------------------------------------------------------------------------+
```

---

## 3. Raspberry Pi Pinout Strategy

### 3.1 BCM Numbering vs. Physical Header Numbering
The Raspberry Pi 4B exposes a 40-pin dual-row expansion header (P1). Connections can be designated by:
1. **Physical Pin Numbering (1 to 40):** Refers to the physical mechanical location of the header pin. Essential for wiring, continuity tracing, and bench probing.
2. **Broadcom (BCM) GPIO Numbering (GPIO 0 to GPIO 27):** Refers to the internal silicon register line on the Broadcom BCM2711 SoC.

```
> [!IMPORTANT]
> **BCM numbering is the MANDATORY software standard.**
> Modern Linux kernel drivers (`libgpiod`, `/dev/gpiochip*`), device tree overlays, and real-time RTOS resource managers address pins exclusively by their Broadcom (BCM) GPIO index. All configuration files and internal software constants must utilize BCM numbering.
```

### 3.2 Raspberry Pi 4B 40-Pin Header Physical Layout (J8)

```
                              +--[ RPi 4B 40-PIN HEADER ]--+
              +3.3V DC Power  |  1 [ ] [ ]  2  |  +5.0V DC Power
        BCM 2 (I2C1 SDA) [R]  |  3 [ ] [ ]  4  |  +5.0V DC Power
        BCM 3 (I2C1 SCL) [R]  |  5 [ ] [ ]  6  |  Ground (GND)
                       BCM 4  |  7 [ ] [ ]  8  |  BCM 14 (UART TX) [R]
                Ground (GND)  |  9 [ ] [ ] 10  |  BCM 15 (UART RX) [R]
         BCM 17 (RED LAMP 1)  | 11 [ ] [ ] 12  |  BCM 18 (PWM / Siren) [R]
      BCM 27 (YELLOW LAMP 1)  | 13 [ ] [ ] 14  |  Ground (GND)
       BCM 22 (GREEN LAMP 1)  | 15 [ ] [ ] 16  |  BCM 23 (IR SENSOR)
              +3.3V DC Power  | 17 [ ] [ ] 18  |  BCM 24 (ULTRASONIC TRIG)
      BCM 10 (SPI0 MOSI) [R]  | 19 [ ] [ ] 20  |  Ground (GND)
      BCM  9 (SPI0 MISO) [R]  | 21 [ ] [ ] 22  |  BCM 25 (ULTRASONIC ECHO)
      BCM 11 (SPI0 SCLK) [R]  | 23 [ ] [ ] 24  |  BCM  8 (SPI0 CE0) [R]
                Ground (GND)  | 25 [ ] [ ] 26  |  BCM  7 (SPI0 CE1) [R]
          BCM 0 (ID_SD) [RES] | 27 [ ] [ ] 28  |  BCM 1 (ID_SC) [RES]
       BCM 5 (EMERGENCY STOP) | 29 [ ] [ ] 30  |  Ground (GND)
       BCM 6 (MANUAL OVERRID) | 31 [ ] [ ] 32  |  BCM 12 (PWM0 Reserve) [R]
        BCM 13 (PWM1 Reserve) | 33 [ ] [ ] 34  |  Ground (GND)
               BCM 19 (Expan) | 35 [ ] [ ] 36  |  BCM 16 (STATUS LED)
     BCM 26 (WATCHDOG RELAY)  | 37 [ ] [ ] 38  |  BCM 20 (Expan Reserve) [R]
                Ground (GND)  | 39 [ ] [ ] 40  |  BCM 21 (Expan Reserve) [R]
                              +----------------------------+
       Legend: [R] = Reserved for Future Subsystems; [RES] = Reserved for HAT EEPROM
```

---

## 4. GPIO Assignment Table

The following table establishes the official hardware mapping for the SMART AEGIS Traffic Signal Node (RSU) and integrated bench prototype.

### 4.1 Primary Peripheral Allocations

| Peripheral / Signal Name | Direction | BCM Pin | Physical Pin | Logic Level | Default Boot State | Circuit Description & Interface Purpose |
|---|---|---|---|---|---|---|
| **Approach 1 Red Lamp** | `OUTPUT` | **BCM 17** | Pin 11 | 3.3V LVTTL | `LOW` (0V) | Drives Channel 1 of opto-isolated relay board. Energizes 12V Red signal head face. |
| **Approach 1 Yellow Lamp** | `OUTPUT` | **BCM 27** | Pin 13 | 3.3V LVTTL | `LOW` (0V) | Drives Channel 2 of opto-isolated relay board. Energizes 12V Yellow signal head face. |
| **Approach 1 Green Lamp** | `OUTPUT` | **BCM 22** | Pin 15 | 3.3V LVTTL | `LOW` (0V) | Drives Channel 3 of opto-isolated relay board. Energizes 12V Green signal head face. |
| **IR Vehicle Sensor** | `INPUT` | **BCM 23** | Pin 16 | 3.3V LVTTL | Pull-Up (`HIGH`) | Optical stop-bar vehicle presence detector. Active LOW when vehicle blocks IR beam. |
| **Ultrasonic Trigger** | `OUTPUT` | **BCM 24** | Pin 18 | 3.3V LVTTL | `LOW` (0V) | HC-SR04 distance transmitter. Emits 10µs pulse to initiate ultrasonic sound burst. |
| **Ultrasonic Echo** | `INPUT` | **BCM 25** | Pin 22 | 3.3V LVTTL | Pull-Down (`LOW`)| HC-SR04 echo receiver. Pulse-width proportional to distance. Interfaced via voltage divider. |
| **Emergency Stop Button** | `INPUT` | **BCM 5** | Pin 29 | 3.3V LVTTL | Pull-Up (`HIGH`) | Field emergency stop switch. Active LOW when depressed. Hardware normally-closed (NC). |
| **Manual Override Switch** | `INPUT` | **BCM 6** | Pin 31 | 3.3V LVTTL | Pull-Up (`HIGH`) | Technician manual preemption toggle. Active LOW to hold intersection green. |
| **Watchdog Relay Output** | `OUTPUT` | **BCM 26** | Pin 37 | 3.3V LVTTL | `LOW` (0V) | Controls external fail-safe flasher relay. Must be pulsed at 2 Hz; drops to Amber on failure. |
| **System Status LED** | `OUTPUT` | **BCM 16** | Pin 36 | 3.3V LVTTL | `LOW` (0V) | Green LED on cabinet front panel. Blinks at 1 Hz during normal software loop execution. |

### 4.2 Reserved GPIOs for Future Hardware Expansion (Minimum 8 Required)

The SMART AEGIS architecture explicitly reserves **14 GPIO lines** to accommodate future Phase expansion (Sensors, Displays, Positioning, Radios, Audio Sirens) without pin contention.

| Reserved Subsystem | BCM Pin | Physical Pin | Reserved Protocol | Planned Future Hardware Expansion |
|---|---|---|---|---|
| **I2C1 SDA** | **BCM 2** | Pin 3 | I2C Data (Fixed 1.8kΩ PU) | SSD1306 0.96" OLED Telematics Display (`0x3C`) & MPU-6050 IMU (`0x68`) |
| **I2C1 SCL** | **BCM 3** | Pin 5 | I2C Clock (Fixed 1.8kΩ PU)| Master clock for OLED, IMU, and INA219 power monitors |
| **UART0 TXD** | **BCM 14** | Pin 8 | Serial Transmit (3.3V) | High-speed telemetry link or RS-485 municipal transit gateway |
| **UART0 RXD** | **BCM 15** | Pin 10 | Serial Receive (3.3V) | u-blox NEO-M8N GNSS Module (10 Hz NMEA-0183 serial stream) |
| **SPI0 MOSI** | **BCM 10** | Pin 19 | SPI Master Out Slave In | MCP2515 CAN Bus Controller (Automotive OBD/J1939 telematics) |
| **SPI0 MISO** | **BCM 9** | Pin 21 | SPI Master In Slave Out | SX1262 LoRa Long-Range Wireless Transceiver (Backup WAN mesh) |
| **SPI0 SCLK** | **BCM 11** | Pin 23 | SPI Serial Clock | Synchronous bus clock for CAN / LoRa coprocessors (up to 10 MHz) |
| **SPI0 CE0** | **BCM 8** | Pin 24 | SPI Chip Select 0 | Primary chip select for CAN Bus controller |
| **SPI0 CE1** | **BCM 7** | Pin 26 | SPI Chip Select 1 | Secondary chip select for LoRa module |
| **Hardware Siren PWM** | **BCM 18** | Pin 12 | Hardware PWM0 | High-decibel audible emergency acoustic siren / piezo buzzer |
| **PWM0 Channel B** | **BCM 12** | Pin 32 | Hardware PWM0 | Display backlight dimming or motor actuator |
| **PWM1 Channel A** | **BCM 13** | Pin 33 | Hardware PWM1 | Optical strobe flasher modulation |
| **Expansion General 1**| **BCM 19** | Pin 35 | GPIO Input/Output | Traffic Cabinet Door Intrusion / Tamper Detection Switch |
| **Expansion General 2**| **BCM 20** | Pin 38 | GPIO Input/Output | Pedestrian Pushbutton Crossing Demand Sensor |
| **Expansion General 3**| **BCM 21** | Pin 40 | GPIO Input/Output | Optical Conflict Monitor (Hardware Dual-Green Sensor Feedback) |

---

## 5. Electrical Characteristics & Interfacing Specifications

```
> [!CAUTION]
> **ABSOLUTE ELECTRICAL LIMITS (BCM2711 SoC):**
> * Input Voltage Range: **-0.5V to +3.6V** (Direct connection to 5.0V will destroy the SoC).
> * Maximum Current per Pin: **16 mA** (Continuous sourcing or sinking).
> * Total Aggregate Current for all GPIOs: **50 mA maximum**.
```

### 5.1 Per-Peripheral Electrical Interfacing Matrix

```
+-------------------------------------------------------------------------------------------------------------------+
|                                        ELECTRICAL INTERFACING MATRIX                                              |
+---------------------+---------+-----------+----------------+-----------------+-------------------+----------------+
| Peripheral Name     | Voltage | Max Load  | Termination    | Debounce Method | Noise Protection  | Cabling Specs  |
+---------------------+---------+-----------+----------------+-----------------+-------------------+----------------+
| Approach 1 Red      | 3.3V Out| 8.0 mA    | None (Direct)  | N/A (Output)    | Opto-isolated     | Ribbon < 30cm  |
| Approach 1 Yellow   | 3.3V Out| 8.0 mA    | None (Direct)  | N/A (Output)    | Opto-isolated     | Ribbon < 30cm  |
| Approach 1 Green    | 3.3V Out| 8.0 mA    | None (Direct)  | N/A (Output)    | Opto-isolated     | Ribbon < 30cm  |
| IR Stop-Bar Sensor  | 3.3V In | < 1.0 mA  | Pull-Up (Int)  | 20ms Software   | Ferrite bead      | 3-wire STP < 2m|
| Ultrasonic Trigger  | 3.3V Out| 5.0 mA    | None (Direct)  | N/A (Output)    | RC Filter         | 4-wire STP < 2m|
| Ultrasonic Echo     | 3.3V In | < 1.0 mA  | Voltage Divider| 10µs Timeout    | Shunt Zener (3.3V)| 4-wire STP < 2m|
| Emergency Stop      | 3.3V In | < 1.0 mA  | 10k Ext Pull-Up| 50ms Hardware+SW| RC Low-Pass Filter| 2-wire STP < 5m|
| Manual Override     | 3.3V In | < 1.0 mA  | 10k Ext Pull-Up| 30ms Software   | Schmitt Trigger   | 2-wire STP < 5m|
| Watchdog Relay Out  | 3.3V Out| 8.0 mA    | None (Direct)  | N/A (2 Hz pulse)| Opto-isolated     | Ribbon < 30cm  |
| System Status LED   | 3.3V Out| 4.5 mA    | 330Ω Resistor  | N/A (Output)    | Current Limiting  | Chassis Panel  |
+---------------------+---------+-----------+----------------+-----------------+-------------------+----------------+
```

### 5.2 Ultrasonic Echo Level Shifter Circuit (5V to 3.3V)
The HC-SR04 sensor outputs a **5.0V TTL pulse** on its Echo pin. Directly connecting this to a Raspberry Pi GPIO will cause catastrophic failure of the GPIO bank.
* **Mandatory Voltage Divider Topology:**
  ```
  HC-SR04 Echo (5V) ────[ R1: 1.0 kΩ ]────┬──── BCM 25 (Input: 3.33V Max)
                                          │
                                   [ R2: 2.0 kΩ ]
                                          │
                                     Ground (GND)
  ```
* $V_{out} = 5.0V \times \frac{2000Ω}{1000Ω + 2000Ω} = 3.33V$ (Within BCM2711 safe input tolerance).

### 5.3 Relay Optocoupler Driving Circuit (Active-LOW Protection)
Standard relay boards use PC817 optocouplers. Driving them from 3.3V logic:
* When the GPIO is set to `LOW` (0V), current flows through the optocoupler internal IR LED, switching the output transistor and energizing the relay.
* To prevent accidental relay closure when the GPIO floats during power-up, an external **10 kΩ pull-up resistor** to +3.3V is mandatory on all output trigger lines.

---

## 6. Boot State Requirements & Startup Sequencing

```
+-----------------------------------------------------------------------------------+
|                        BCM2711 SILICON BOOT INVARIANTS                            |
+-----------------------------------------------------------------------------------+
| * GPIO 0 to 8:    Default to INPUT with INTERNAL PULL-UP enabled on power-up.     |
| * GPIO 9 to 27:   Default to INPUT with INTERNAL PULL-DOWN enabled on power-up.   |
+-----------------------------------------------------------------------------------+
```

### 6.1 Critical Safety Boot Rules
1. **Zero Spurious Actuation:** Under no circumstances may the Green signal lamp activate during system bootloader execution, kernel decompression, or systemd daemon startup.
2. **Fail-Safe Relay De-energized:** The Watchdog Relay Output (**BCM 26**) must remain strictly `LOW` throughout boot, ensuring the physical hardware flasher relay remains in its normally-closed **Flashing Amber** state.
3. **Ultrasonic Transmitter Inactive:** Ultrasonic Trigger (**BCM 24**) must initialize to `LOW` to prevent acoustic transducer saturation.

### 6.2 Deterministic Initialization Sequence in Software

```
   [ POWER APPLIED ]
          │
          ▼
   [ Silicon Bootloader & Kernel Initialization ]
   - All pins default to High-Z Inputs (GPIO 9-27 Pulled Down).
   - Relays de-energized; Hardware Flasher active (Flashing Amber).
          │
          ▼
   [ Step 1: Initialize Safety Subsystem ]
   - Configure BCM 26 (Watchdog Relay) as OUTPUT, drive LOW.
   - Configure BCM 16 (Status LED) as OUTPUT, drive LOW.
          │
          ▼
   [ Step 2: Establish Passive Lamp States ]
   - Configure BCM 17, 27, 22 as OUTPUTS, drive LOW (relays unasserted).
          │
          ▼
   [ Step 3: Configure Safety Inputs ]
   - Configure BCM 5 (E-Stop) and BCM 6 (Override) as INPUTS with PULL-UP.
   - Read physical state: If E-Stop depressed, ABORT startup into safe halt.
          │
          ▼
   [ Step 4: Configure Sensor Lines ]
   - Configure BCM 24 (Trig) as OUTPUT (LOW).
   - Configure BCM 25 (Echo) as INPUT (PULL-DOWN).
   - Configure BCM 23 (IR) as INPUT (PULL-UP).
          │
          ▼
   [ Step 5: Assert Initial Red Phase ]
   - Assert BCM 17 (Red Lamp 1) HIGH.
   - Hold Red state for mandatory startup stabilization (≥ 2000 ms).
          │
          ▼
   [ Step 6: Enable Watchdog & Main FSM Loop ]
   - Begin pulsing BCM 26 at 2 Hz to hold off hardware fail-safe relay.
   - Signal state machine transitions to active cycling.
```

---

## 7. Fail-Safe Behaviour Matrix

```
+---------------------------------------------------------------------------------------------------------------+
|                                      HARDWARE FAIL-SAFE REACTION MATRIX                                       |
+------------------------+-------------------------------+------------------------------+-----------------------+
| Failure Trigger        | Hardware Electrical Reaction  | Optical Signal State         | Recovery Mechanism    |
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Complete Power Loss**| Relays completely de-energize;| All lamps dark; or auxiliary | Power restoration;    |
|                        | Battery backup flasher trips. | 12V analog flasher Amber.    | Cold boot sequence.   |
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Software Crash /     | BCM 26 ceases pulsing;        | Hardware de-energizes green; | Systemd restarts      |
| Core Dump / Panic**    | Watchdog timer expires (2s).  | Reverts to FLASHING AMBER.   | daemon; or SoC resets.|
+------------------------+-------------------------------+------------------------------+-----------------------+
| **MQTT Disconnect      | Node detects 5000ms heartbeat | Autonomous local FSM triggers| MQTT reconnects; state|
| (Network Partition)**  | timeout via software timer.   | FLASHING YELLOW clearance.   | resynchronized.       |
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Emergency Stop       | NC circuit opens physically;  | Hardware kills all green     | Manual physical reset |
| Button Pressed**       | BCM 5 triggers interrupt.     | signals; drops to ALL-RED.   | and operator sign-off.|
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Manual Override      | BCM 6 switches to active LOW; | Holds preemption green on    | Manual switch toggled |
| Switch Asserted**      | Preemption FSM locked.        | primary approach.            | back to normal AUTO.  |
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Relay Contact Weld / | Auxiliary contact feedback on | Hardware interlock line cuts | Lockout to FLASHING   |
| Stuck Green Fault**    | BCM 21 detects dual-power.    | approach 2 power mechanically| AMBER; technician req.|
+------------------------+-------------------------------+------------------------------+-----------------------+
| **Ultrasonic / IR      | Software timeout flags        | Uses worst-case static       | Automatic on sensor   |
| Sensor Disconnect**    | sensor reading as invalid.    | clearance times (5.0s yellow)| signal restoration.   |
+------------------------+-------------------------------+------------------------------+-----------------------+
```

---

## 8. GPIO Ownership & Architectural Encapsulation

### 8.1 Single-Class Ownership Architecture
To ensure strict adherence to MISRA C++:2008 and ISO 26262 functional safety standards:
1. **The Class Boundary:** The class `aegis::edge::traffic::RaspberryPiGpioInterface` is the **EXCLUSIVELY AUTHORIZED** software component permitted to interact with physical BCM GPIOs.
2. **Information Hiding:** High-level controllers never see BCM integers. They communicate solely through the virtual methods of `IGpioInterface`:

```cpp
// WHAT HIGH-LEVEL MODULES (TrafficController) SEE:
class IGpioInterface {
public:
    virtual ~IGpioInterface() = default;
    virtual bool Initialize() = 0;
    virtual void SetRed() = 0;
    virtual void SetYellow() = 0;
    virtual void SetGreen() = 0;
    virtual void FlashYellow() = 0;
    virtual void FlashRed() = 0;
    virtual void AllOff() = 0;
    virtual void Shutdown() = 0;
};
```

3. **Mutex Serialization:** All GPIO line operations inside `RaspberryPiGpioInterface` must be guarded by an internal `std::mutex` to prevent concurrent write corruption from asynchronous safety threads.

---

## 9. Future Hardware Expansion Allocation Plan

To ensure scalability into subsequent engineering phases without requiring board redesigns, the following physical interfaces are reserved:

```
+---------------------------------------------------------------------------------------------------------+
|                                    FUTURE EXPANSION BUS ALLOCATION                                      |
+------------------+-----------------------+------------------------+-------------------------------------+
| Target Feature   | Physical Interface    | Allocated Pins         | Technical Purpose                   |
+------------------+-----------------------+------------------------+-------------------------------------+
| OLED Display     | I2C Bus 1 (/dev/i2c-1)| BCM 2 (SDA), BCM 3(SCL)| Real-time in-cab telematics status  |
| 6-DOF IMU        | I2C Bus 1 (/dev/i2c-1)| BCM 2 (SDA), BCM 3(SCL)| Dead-reckoning kinematic tracking   |
| GNSS Receiver    | UART0 (/dev/ttyAMA0)  | BCM 14(TX), BCM 15(RX) | u-blox 10 Hz high-precision GPS     |
| CAN Bus Module   | SPI0 (/dev/spidev0.0) | BCM 8, 9, 10, 11       | Automotive J1939 vehicle telemetry  |
| LoRa Transceiver | SPI0 (/dev/spidev0.0) | BCM 7, 9, 10, 11       | Long-range municipal backup radio   |
| RS-485 Gateway   | UART0 (Multiplexed)   | BCM 14, 15 + BCM 4 DIR | Legacy NEMA TS2 intersection bridge |
| Acoustic Siren   | Hardware PWM0         | BCM 18                 | 105 dB vehicle preemption siren     |
| Edge AI Camera   | USB 3.0 / MIPI CSI    | Dedicated Camera Port  | Real-time video queue length NPU    |
+------------------+-----------------------+------------------------+-------------------------------------+
```

---

## 10. Hardware Validation & Bench Checklist

Before applying power to any Raspberry Pi 4B connected to prototype relays or sensors, the lead electrical engineer must complete and sign this validation checklist:

```
+---------------------------------------------------------------------------------------------------+
|                                  HARDWARE PRE-FLIGHT BENCH CHECKLIST                              |
+---------------------------------------------------------------------------------------------------+
| Check Item | Inspection & Verification Criteria                          | Status  | Verified By  |
+------------+-------------------------------------------------------------+---------+--------------+
| **CHK-01** | LED Polarity: Cathodes tied to common ground; anodes to     | [ ] PASS| ____________ |
|            | 330Ω current-limiting resistors.                            |         |              |
| **CHK-02** | Voltage Isolation: Optical isolation confirmed between      | [ ] PASS| ____________ |
|            | 3.3V Pi logic and 12V relay actuator rail (DMM > 10 MΩ).    |         |              |
| **CHK-03** | Ultrasonic Level Shifter: Voltage divider outputs exactly   | [ ] PASS| ____________ |
|            | 3.3V ± 0.1V when 5.0V is applied to HC-SR04 Echo pin.       |         |              |
| **CHK-04** | Current Limits: Sourcing current measured on all active     | [ ] PASS| ____________ |
|            | outputs simultaneously is < 40 mA total (Limit: 50 mA).     |         |              |
| **CHK-05** | Relay Non-Overlap: Oscilloscope confirms zero simultaneous  | [ ] PASS| ____________ |
|            | Red and Green contact closures under simulated stepping.    |         |              |
| **CHK-06** | E-Stop Integrity: Depressing Emergency Stop instantly opens | [ ] PASS| ____________ |
|            | relay power rail independent of software response.          |         |              |
| **CHK-07** | Pull-Up Verification: Inputs (BCM 5, 6, 23) read 3.3V ± 0.1V| [ ] PASS| ____________ |
|            | when idle, and 0.0V when switch is physically depressed.    |         |              |
| **CHK-08** | Cold Boot Glitch Test: Logic analyzer verifies BCM 22       | [ ] PASS| ____________ |
|            | (Green) produces zero voltage spikes (>0.5V) during reboot. |         |              |
| **CHK-09** | Power Brownout Recovery: SBC survives 12V supply step-drop  | [ ] PASS| ____________ |
|            | to 9.0V without crashing or corrupting filesystem.          |         |              |
| **CHK-10** | Thermal Headroom: SoC temperature stays < 65°C under full   | [ ] PASS| ____________ |
|            | continuous 10 Hz telemetry and relay switching load.        |         |              |
+------------+-------------------------------------------------------------+---------+--------------+
```

---

## 11. Engineering Risk Analysis & Mitigation Matrix

```
+-------------------------------------------------------------------------------------------------------------------------------+
|                                                HARDWARE ENGINEERING RISK MATRIX                                               |
+---------+-----------------------------+---------+------------+----------+-----------------------------------------------------+
| Risk ID | Hazard Description          | Severity| Likelihood | Risk Lvl | Technical Mitigation Strategy                       |
+---------+-----------------------------+---------+------------+----------+-----------------------------------------------------+
| **R-01**| 5V accidentally injected    | FATAL   | MEDIUM     | HIGH     | Mandatory dual-resistor voltage divider on Echo pin;|
|         | into 3.3V GPIO pin          |         |            |          | 3.3V clamping Zener diodes on all external inputs.  |
| **R-02**| Floating input generates    | HIGH    | HIGH       | HIGH     | Hardware external 10kΩ pull-up resistors fitted to  |
|         | spurious emergency stops    |         |            |          | all safety lines; internal SoC pull-ups enabled.    |
| **R-03**| Relay contact welds closed, | CRITICAL| LOW        | HIGH     | Physical series-complementary contact wiring forbids|
|         | trapping signal in Green    |         |            |          | cross-street green illumination mechanically.       |
| **R-04**| Relay coil inrush current   | HIGH    | HIGH       | HIGH     | Dual-rail power architecture: separate isolated buck|
|         | causes SBC 5V brownout      |         |            |          | converter for Pi logic and 12V rail for relays.     |
| **R-05**| Inductive back-EMF spikes   | CRITICAL| HIGH       | HIGH     | 1N4007 fast-recovery flyback diodes installed across|
|         | damage switching transistors|         |            |          | all inductive relay coil terminals.                 |
| **R-06**| Noise on IR sensor cable    | MEDIUM  | HIGH       | MEDIUM   | Shielded twisted pair (STP) cabling with grounded   |
|         | triggers false preemption   |         |            |          | drain wire; 20ms software debounce validation.      |
| **R-07**| Software pin mapping drift  | CRITICAL| LOW        | HIGH     | Pin mapping frozen in this specification; automated |
|         | across code versions        |         |            |          | unit tests verify mapping constants against spec.   |
+---------+-----------------------------+---------+------------+----------+-----------------------------------------------------+
```

---

## 12. Final Approval & Implementation Gate Criteria

In accordance with SMART AEGIS project governance, the transition to **Phase 5C: Raspberry Pi GPIO Driver Implementation** is strictly blocked until all five gate criteria are formally signed off:

```
+-----------------------------------------------------------------------------------+
|                               PHASE 5C ENTRY GATE CRITERIA                        |
+-----------------------------------------------------------------------------------+
| [X] Criterion 1: RASPBERRY_PI_PIN_MAPPING.md formally authored and ratified.       |
| [ ] Criterion 2: Electrical schematic review completed and signed by HW Lead.    |
| [ ] Criterion 3: Bench prototype wired and inspected against Section 4 & 5.       |
| [ ] Criterion 4: Pre-flight checklist (Section 10) verified with logic analyzer.  |
| [ ] Criterion 5: Formal sign-off granted by Principal Embedded Systems Architect.  |
+-----------------------------------------------------------------------------------+
```

```
================================================================================
APPROVAL RECORD:
Principal Embedded Systems Architect: ___________________________ Date: _________
Lead Hardware Systems Engineer:       ___________________________ Date: _________
Functional Safety Officer:            ___________________________ Date: _________
================================================================================
```
