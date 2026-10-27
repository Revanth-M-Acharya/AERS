# SMART AEGIS: Hardware Abstraction Document (HAD)

```
================================================================================
Document Identifier:  AEGIS-HAD-001
Title:                Hardware Abstraction Document (HAD)
Project:              SMART AEGIS (Automated Emergency Grid & Intelligent Shield)
Target Baseline:      Pre-Phase 5C Architecture Transition (Post-Phase 5B)
Version:              1.0
Author:               Principal Embedded Systems Architect
Standards:            ISO 26262 (ASIL-B/D Principles), MISRA C++:2008, IEEE 802.11p/ITS-G5
Operating Targets:    Raspberry Pi 4B (Embedded Linux / QNX Neutrino RTOS 7.1/8.0)
Status:               APPROVED FOR IMPLEMENTATION BASELINE
================================================================================
```

---

## 1. Purpose

### 1.1 Objective of the Hardware Abstraction Layer (HAL)
The SMART AEGIS (Automated Emergency Grid & Intelligent Shield) platform is a safety-critical, distributed cyber-physical emergency management system. It coordinates emergency vehicle tracking, physiological patient monitoring, dynamic traffic signal preemption, and municipal hospital notifications in real time.

The primary objective of this **Hardware Abstraction Document (HAD)** is to establish a rigorous, production-grade architectural specification that decouples high-level control algorithms, state machines, and business logic from low-level physical silicon, microcontrollers, single-board computers (SBCs), peripheral communication buses, and electrical actuators. 

This document defines the interface boundary required prior to implementing physical drivers (starting with Phase 5C: Raspberry Pi GPIO Driver). It guarantees that the software baseline established in Phases 1 through 5B operates identically whether executing against:
1. Pure software mock and loopback harnesses (x86_64 host simulation).
2. Physical Raspberry Pi 4B development rigs running Linux with `/dev/gpiod`.
3. Industrial target controllers executing under the QNX Neutrino Real-Time Operating System (RTOS) using custom POSIX Resource Managers (`resmgr`).

### 1.2 Design Philosophy
The SMART AEGIS hardware abstraction is governed by five foundational engineering principles:

```
+---------------------------------------------------------------------------------------+
|                              FIVE FOUNDATIONAL PRINCIPLES                             |
+---------------------------------------------------------------------------------------+
| 1. Strict Dependency Inversion (SOLID Principle)                                      |
|    High-level modules (TrafficController, GpsTracker) depend on abstract interfaces. |
|    Concrete drivers (RaspberryPiGpioInterface, SerialGpsSource) depend on interfaces. |
+---------------------------------------------------------------------------------------+
| 2. Zero Business Logic Contamination                                                  |
|    Algorithms contain zero direct references to BCM registers, memory offsets,        |
|    file descriptors, baud rates, or peripheral-specific polling loops.                |
+---------------------------------------------------------------------------------------+
| 3. Hardware-Independent Determinism                                                   |
|    Timing guarantees, clearance intervals, and state transition invariants are        |
|    enforced within core state machines, not delegated to peripheral drivers.          |
+---------------------------------------------------------------------------------------+
| 4. Cold-Iron Fail-Safe Safety (Default-Safe States)                                   |
|    Any unhandled hardware exception, communication blackout, or power brownout must   |
|    instantly and deterministically revert the physical plant to a validated safe      |
|    state (e.g., hardware-interlocked Flashing Yellow / All-Red).                      |
+---------------------------------------------------------------------------------------+
| 5. Configuration-Driven Polymorphism                                                  |
|    Drivers are instantiated via factory patterns driven by external configuration     |
|    profiles (config.json), eliminating compile-time #ifdef fragmentation.             |
+---------------------------------------------------------------------------------------+
```

### 1.3 Separation Between Business Logic and Hardware Drivers
To enforce functional safety and prevent architectural drift, the system architecture establishes a strict physical boundary between the **Application Domain** and the **Hardware Execution Domain**:

```
+-----------------------------------------------------------------------------------+
|                        APPLICATION DOMAIN (Business Logic)                        |
|                                                                                   |
|   +--------------------------+  +----------------------+  +--------------------+  |
|   |    CentralCoordinator    |  |  TrafficSignalNode   |  |   AmbulanceNode    |  |
|   |    (Phase 3 Core)        |  |  (Phase 5A/5B FSM)   |  |   (Phase 4 OBU)    |  |
|   +-------------+------------+  +-----------+----------+  +---------+----------+  |
|                 |                           |                       |             |
+-----------------|---------------------------|-----------------------|-------------+
|                 v                           v                       v             |
|   +--------------------------+  +----------------------+  +--------------------+  |
|   |      IMqttTransport      |  |    IGpioInterface    |  |     IGpsSource     |  |
|   |   (Transport Interface)  |  |   (Actuator Interf.) |  |  (Sensor Interf.)  |  |
|   +-------------+------------+  +-----------+----------+  +---------+----------+  |
|                 |                           |                       |             |
|=================|===========================|=======================|=============|
|                 |  HARDWARE ABSTRACTION LAYER (HAL Interface Boundary)|             |
|=================|===========================|=======================|=============|
|                 |                           |                       |             |
|                 v                           v                       v             |
|   +--------------------------+  +----------------------+  +--------------------+  |
|   |  Simulation / Mosquitto  |  |   Simulation / RPi   |  | Simulation / Serial|  |
|   |        Transport         |  |      GPIO Driver     |  |     GPS Source     |  |
|   +-------------+------------+  +-----------+----------+  +---------+----------+  |
|                 |                           |                       |             |
+-----------------|---------------------------|-----------------------|-------------+
|                 v                           v                       v             |
|       [ Physical Network ]          [ Relay & LEDs ]         [ GNSS Receiver ]    |
|       (TCP/IP / Ethernet)           (BCM GPIO / Opto)        (UART /dev/ttyAMA0)  |
|                                                                                   |
|                     PHYSICAL HARDWARE DOMAIN (Physical Plant)                     |
+-----------------------------------------------------------------------------------+
```

### 1.4 Scope
This specification applies to all hardware tiers within SMART AEGIS:
* Central Coordinator Host / Server Infrastructure.
* Roadside Unit (RSU) Traffic Signal Controllers.
* On-Board Unit (OBU) Ambulance Telematics Controllers.
* Opto-isolated relay modules, traffic signal lamp arrays, and interlock hardware.
* Vehicular positioning (GNSS), physiological telemetry interfaces, and ad-hoc wireless (V2X).
* Peripheral sensing systems (Infrared, Ultrasonic, IMU, and OLED operator displays).

---

## 2. System Hardware Overview

### 2.1 Overall Hardware Architecture
SMART AEGIS is deployed as a geographically distributed cyber-physical network comprising three dedicated embedded node classes communicating over a redundant TCP/IP backhaul (MQTT over TLS) and localized ad-hoc wireless datagrams (UDP).

```
+------------------------------------------------------------------------------------------------------------------+
|                                        SMART AEGIS DISTRIBUTED SYSTEM TOPOLOGY                                   |
+------------------------------------------------------------------------------------------------------------------+
|                                                                                                                  |
|                                            +---------------------------+                                         |
|                                            |    CENTRAL COORDINATOR    |                                         |
|                                            |    Raspberry Pi 4B (8GB)  |                                         |
|                                            |   - Emergency Intake Core |                                         |
|                                            |   - Dynamic Green Corridor|                                         |
|                                            |   - A* Route Planner      |                                         |
|                                            +-------------+-------------+                                         |
|                                                          |                                                       |
|                                   +----------------------+----------------------+                                |
|                                   |                                             |                                |
|                                   v                                             v                                |
|                     +---------------------------+                 +---------------------------+                  |
|                     |     MQTT BROKER HOST      |                 |    HOSPITAL DASHBOARD     |                  |
|                     |    Eclipse Mosquitto      |                 |   Workstation / Tablet    |                  |
|                     |   (Port 8883 / TLS 1.3)   |                 |   (Clinical Intake Bay)   |                  |
|                     +-------------+-------------+                 +---------------------------+                  |
|                                   |                                                                              |
|               +-------------------+-------------------+                                                          |
|               |                                       |                                                          |
|               v                                       v                                                          |
| +---------------------------+           +---------------------------+                                            |
| |    AMBULANCE OBU NODE     |           |     TRAFFIC RSU NODE      |                                            |
| |   Raspberry Pi 4B (4GB)   |           |   Raspberry Pi 4B (2GB)   |                                            |
| |  - u-blox NEO-M8N GNSS    |           |  - Opto-Isolated 4-Ch SSR |                                            |
| |  - 6-DOF IMU (Dead-Reck.) |   (V2I)   |  - Red/Yellow/Green LEDs  |                                            |
| |  - Biomedical Monitor I/F | ~ ~ ~ ~ ~ |  - Fail-Safe Flasher Circ |                                            |
| |  - I2C OLED Telematics UI | (UDP:8998)|  - IR / Ultrasonic Sensor |                                            |
| |  - Ad-Hoc V2V (UDP:8999)  |           |  - Hardware Watchdog WDT  |                                            |
| +---------------------------+           +---------------------------+                                            |
|                                                                                                                  |
+------------------------------------------------------------------------------------------------------------------+
```

### 2.2 Node Profiles
1. **Central Coordinator Node (Server / SBC):**
   * High-capacity compute host executing dispatch optimization, topological road network graph analysis, dynamic preemption trajectory calculation, and multi-ambulance deconfliction.
   * Interfaces with external Emergency Service IP networks (PSAP) and telemetry aggregators.
2. **Traffic Signal Node (Roadside Unit - RSU):**
   * Deterministic edge computer mounted in municipal traffic cabinets.
   * Direct electrical coupling to solid-state relay (SSR) modules driving primary signal lamps.
   * Interfaces with vehicle presence sensors and a hardware watchdog monitor.
3. **Ambulance Node (On-Board Unit - OBU):**
   * Mobile vehicle computer powered by vehicular DC power.
   * Acquires positioning data via high-precision GNSS/IMU, samples patient diagnostic vitals, and provides in-cab driver status via an OLED display.
   * Dispatches ad-hoc V2V safety beacons and V2I direct preemption triggers.
4. **Hospital Dashboard Terminal:**
   * Workstation terminal located in emergency hospital trauma bays, providing clinical pre-arrival telemetry and dynamic ETA countdowns.

---

## 3. Hardware Components

The following standardized hardware components constitute the physical target bill of materials (BOM) for SMART AEGIS.

### 3.1 Single-Board Computer (SBC)
| Specification Field | Technical Description |
|---|---|
| **Component Name** | Raspberry Pi 4 Model B |
| **System Architecture** | Broadcom BCM2711, Quad-core Cortex-A72 (ARM v8) 64-bit SoC @ 1.5GHz |
| **Operating Voltages** | 5.1V DC via USB-C or GPIO Header (40-Pin LVTTL 3.3V Logic) |
| **Memory Profiles** | Coordinator: 8GB LPDDR4; Ambulance OBU: 4GB LPDDR4; Traffic RSU: 2GB LPDDR4 |
| **Peripherals Utilized** | Gigabit Ethernet, 2.4/5.0 GHz 802.11ac Wi-Fi, 2x UART, 2x I2C, 1x SPI, Hardware WDT |
| **Purpose** | Universal host execution engine across all distributed physical nodes |
| **Driver Class** | `RaspberryPiSystemCore` / Platform OS Kernel |
| **Software Interface** | Standard POSIX.1 / QNX Neutrino Microkernel API |

### 3.2 Relay Actuator Module
| Specification Field | Technical Description |
|---|---|
| **Component Name** | 4-Channel Opto-Isolated Solid-State / Electromechanical Relay Module |
| **Electrical Interface** | Active-LOW logic trigger (0V = Relay Energized; 3.3V/5V = Relay Released) |
| **Switching Voltage** | Contacts rated for 250V AC @ 10A / 30V DC @ 10A (Driving 12V DC LED Lamps) |
| **Isolation** | Optical isolation via PC817 optocouplers (5000V RMS dielectric breakdown) |
| **Communication Bus** | Direct BCM GPIO Output Lines |
| **Purpose** | Galvanic isolation and power switching for physical Red, Yellow, and Green lamps |
| **Driver Class** | `RaspberryPiGpioInterface` |
| **Software Interface** | `aegis::edge::traffic::IGpioInterface` |

### 3.3 Traffic Signal Lamp Array
| Specification Field | Technical Description |
|---|---|
| **Component Name** | Industrial High-Intensity 12V DC Signal Head (Red, Yellow, Green 100mm LEDs) |
| **Operating Voltage** | 12V DC regulated (Typical current draw: 150mA per active lamp face) |
| **Interface** | Switched ground or switched positive via opto-isolated relay contacts |
| **Safety Interlock** | Normally-Closed (NC) auxiliary relay wiring forcing hardware fail-safe Amber on de-energize |
| **Purpose** | Visual signaling to motorists, pedestrians, and automated emergency vehicles |
| **Driver Class** | `RaspberryPiGpioInterface` |
| **Software Interface** | `aegis::edge::traffic::IGpioInterface` |

### 3.4 GNSS / GPS Receiver Module
| Specification Field | Technical Description |
|---|---|
| **Component Name** | u-blox NEO-M8N High-Precision Concurrent GNSS Receiver Module |
| **Communication Bus** | Asynchronous Serial UART (`/dev/ttyAMA0` or `/dev/ttyUSB0`) |
| **Protocol / Baud** | NMEA-0183 v4.0 / UBX Binary Protocol @ 9600 to 115200 baud |
| **Operating Voltage** | 3.3V – 5.0V DC (Integrated LDO regulator; active patch antenna bias 3.3V) |
| **Update Rate** | 10 Hz concurrent GNSS tracking (GPS + GLONASS + Galileo) |
| **Accuracy** | 2.5m CEP (Autonomous), 0.05 m/s velocity accuracy |
| **Purpose** | High-frequency kinematic positioning and vehicle trajectory tracking |
| **Driver Class** | `SerialGpsSource` |
| **Software Interface** | `aegis::edge::ambulance::GpsTracker` |

### 3.5 Operator Telematics Display
| Specification Field | Technical Description |
|---|---|
| **Component Name** | 0.96-inch OLED Graphic Display (SSD1306 Controller) |
| **Resolution** | 128 x 64 pixels, monochrome (White or Blue phosphor) |
| **Communication Bus** | I2C (Bus ID `/dev/i2c-1`, 7-bit slave address `0x3C`) |
| **Operating Voltage** | 3.3V DC |
| **Update Frequency** | 10 Hz refresh budget |
| **Purpose** | Real-time in-cab status: preemption status, speed, corridor clearance, and alerts |
| **Driver Class** | `OledDisplayDriver` |
| **Software Interface** | `aegis::edge::ambulance::IOledDisplay` |

### 3.6 Vehicle Presence Detection (IR Sensor)
| Specification Field | Technical Description |
|---|---|
| **Component Name** | TCRT5000 / Active Infrared Optical Obstacle Detection Sensor |
| **Detection Range** | 2mm – 25mm (Calibrated for miniature scale) or 0.5m – 5m (Long-range optical) |
| **Output Type** | Digital Output (DO: Active LOW upon optical reflection) & Analog Output (AO) |
| **Operating Voltage** | 3.3V – 5.0V DC |
| **Purpose** | Instant confirmation of vehicle physical arrival at intersection stop line |
| **Driver Class** | `IrSensorDriver` |
| **Software Interface** | `aegis::edge::traffic::IVehicleSensor` |

### 3.7 Queue Length Detection (Ultrasonic Sensor)
| Specification Field | Technical Description |
|---|---|
| **Component Name** | HC-SR04 / JSN-SR04T Waterproof Ultrasonic Transducer |
| **Operating Principle** | 40 kHz ultrasonic acoustic time-of-flight pulse |
| **Measurement Range** | 20mm to 4500mm (Resolution: 3mm; Measuring angle: 15 degrees) |
| **Electrical Interface** | Trigger Pin: 10µs TTL pulse; Echo Pin: PWM pulse width proportional to distance |
| **Level Shifting** | Echo pin requires 5V to 3.3V voltage divider (1kΩ / 2kΩ) to protect Pi GPIO |
| **Purpose** | Real-time traffic queue length calculation to adjust preemption lead times |
| **Driver Class** | `UltrasonicDriver` |
| **Software Interface** | `aegis::edge::traffic::IQueueDetector` |

### 3.8 Inertial Measurement Unit (IMU)
| Specification Field | Technical Description |
|---|---|
| **Component Name** | MPU-6050 / LSM6DSO 6-DOF Accelerometer & Gyroscope |
| **Communication Bus** | I2C (`/dev/i2c-1`, 7-bit slave address `0x68`) |
| **Measurement Ranges**| Accel: ±2g, ±4g, ±8g, ±16g; Gyro: ±250, ±500, ±1000, ±2000 deg/s |
| **Operating Voltage** | 3.3V DC |
| **Purpose** | Dead-reckoning kinematic propagation during satellite signal loss (tunnels/canyons)|
| **Driver Class** | `ImuSensorDriver` |
| **Software Interface** | `aegis::edge::ambulance::GpsTracker` (EKF Engine) |

### 3.9 Regulated Power Supply Subsystem
| Specification Field | Technical Description |
|---|---|
| **Component Name** | Industrial Dual-Rail Regulated Power Converter (12V / 5V DC) |
| **Input Envelope** | 100–240V AC (Mains) or 11V–28V DC (Vehicle Automotive Battery Rail) |
| **Output Rails** | Rail A: 5.1V DC @ 4.0A (SBC, Logic); Rail B: 12.0V DC @ 3.0A (Relays, Lamps) |
| **Protections** | OVP (Over-Voltage), OCP (Over-Current), Reverse Polarity, Thermal Shutdown |
| **Grounding** | Galvanically isolated digital ground and high-current power ground |
| **Purpose** | Clean, ripple-free power delivery eliminating brownouts during relay actuation |
| **Driver Class** | `PowerMonitorDriver` |
| **Software Interface** | `aegis::safety::IPowerMonitor` |

### 3.10 Networking & MQTT Infrastructure
| Specification Field | Technical Description |
|---|---|
| **Component Name** | IEEE 802.3 Gigabit Ethernet & IEEE 802.11ac Dual-Band Wireless PHY |
| **Protocol Suite** | TCP/IPv4, UDP Datagrams, MQTT v3.1.1 / v5.0 over TLS 1.3 |
| **MQTT Broker** | Dedicated Host running Eclipse Mosquitto Daemon (Port 8883 / 1883) |
| **Ad-Hoc Wireless** | Dedicated UDP sockets (Port 8999 for V2V BSM; Port 8998 for V2I Preemption) |
| **Purpose** | Low-latency mission command dispatch, telemetry streaming, and peer safety mesh |
| **Driver Class** | `SimulationTransport` (Dev/Test) / `MosquittoTransport` (Target Deployment) |
| **Software Interface** | `aegis::network::IMqttTransport` |

---

## 4. Raspberry Pi Roles & Deployed Software Architecture

```
+---------------------------------------------------------------------------------------------------+
|                                 NODE SPECIFIC SOFTWARE STACKS                                     |
+---------------------------------+---------------------------------+-------------------------------+
|       COORDINATOR NODE          |        TRAFFIC RSU NODE         |      AMBULANCE OBU NODE       |
+---------------------------------+---------------------------------+-------------------------------+
| Hardware: Pi 4B (8GB RAM)       | Hardware: Pi 4B (2GB RAM)       | Hardware: Pi 4B (4GB RAM)     |
| Role: Central Dispatch & Routing| Role: Field Roadside Controller | Role: Vehicle Telematics Edge |
+---------------------------------+---------------------------------+-------------------------------+
| DAEMON EXECUTABLE:              | DAEMON EXECUTABLE:              | DAEMON EXECUTABLE:            |
| /opt/aegis/bin/aegis_coordinator| /opt/aegis/bin/aegis_traffic_...| /opt/aegis/bin/aegis_ambulance|
+---------------------------------+---------------------------------+-------------------------------+
| EXECUTED MODULES (Phases 1-3):  | EXECUTED MODULES (Phases 5A-5B):| EXECUTED MODULES (Phase 4):   |
| - CentralCoordinator            | - TrafficSignalNode             | - AmbulanceNode               |
| - EmergencyCallHandler          | - TrafficController             | - GpsTracker (NMEA + EKF)     |
| - IncidentManager               | - SignalStateMachine (FSM)      | - VitalsMonitor (Clinical)    |
| - AmbulanceRegistry             | - PreemptionManager             | - V2vModule (BSM + V2I)       |
| - FleetManager                  | - RecoveryManager               | - AmbulanceMissionSimulator   |
| - DispatchEngine                | - IGpioInterface (SSR Actuation)| - Offline Ring Buffer Cache   |
| - RoutePlanner (A* Graph)       | - FailSafe Watchdog Monitor     | - Network Manager Facade      |
| - DynamicGreenCorridor          | - Network Manager Facade        | - MqttClient & IMqttTransport |
| - MqttClient & IMqttTransport   | - MqttClient & IMqttTransport   |                               |
+---------------------------------+---------------------------------+-------------------------------+
| PHYSICAL INTERFACES:            | PHYSICAL INTERFACES:            | PHYSICAL INTERFACES:          |
| - 1000BASE-T RJ45 Ethernet      | - 1000BASE-T Ethernet / Wi-Fi   | - 802.11ac Wi-Fi / LTE Modem  |
| - Status Indicator LEDs         | - BCM GPIO (Relays, Signals)    | - UART (/dev/ttyAMA0: GPS)    |
| - Local Console (HDMI/Serial)   | - BCM GPIO (IR / Ultrasonic)    | - I2C (/dev/i2c-1: OLED/IMU)  |
| - Hardware Watchdog Line        | - Hardware Watchdog (/dev/watch)| - UDP Sockets (8998/8999)     |
+---------------------------------+---------------------------------+-------------------------------+
```

---

## 5. GPIO Mapping Strategy

To prevent premature coupling to experimental pin assignments while ensuring rigorous hardware readiness, the SMART AEGIS GPIO architecture adopts a standardized **Pin Allocation Template**. 

All physical pin numbers remain formal **TBD** entries until Phase 5C bench validation.

### 5.1 Electrical Constraints & Safety Guidelines
1. **Logic Level:** Broadcom BCM2711 GPIO lines operate strictly at **3.3V LVTTL**. Direct connection to 5V logic will result in irreversible silicon breakdown.
2. **Current Sinking/Sourcing Limit:** Maximum current per individual GPIO pin is **16mA**. Total cumulative current across all simultaneous active pins must not exceed **50mA**.
3. **Opto-Coupled Isolation Required:** All relay lines must drive optocoupler inputs rather than raw relay coils to isolate high-current flyback surges.
4. **Pull-Up / Pull-Down Resistors:** Floating inputs are strictly prohibited in safety-critical modes. Inputs must configure internal or external pull-up/pull-down terminations.

### 5.2 Traffic Signal Node (RSU) GPIO Pin Allocation Template
| Device / Function | GPIO (BCM) | Direction | Electrical Voltage | Description | Target Driver Class | Allocation Status |
|---|---|---|---|---|---|---|
| Approach 1 Red Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Primary Red Signal Face | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Approach 1 Yellow Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Primary Yellow Signal Face | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Approach 1 Green Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Primary Green Signal Face | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Approach 2 Red Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Cross-Street Red Signal | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Approach 2 Yellow Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Cross-Street Yellow | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Approach 2 Green Lamp | `TBD` | `OUTPUT` | 3.3V (Opto-Isolated) | Controls Cross-Street Green | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Fail-Safe Flasher Relay | `TBD` | `OUTPUT` | 3.3V (Active-LOW) | De-energizes to force hardware Flashing Amber | `RaspberryPiGpioInterface` | `PLANNED - PRE-5C` |
| Hardware Conflict Monitor | `TBD` | `INPUT` | 3.3V (Active-HIGH) | Hardware interlock line detecting conflicting greens | `SafetySupervisor` | `PLANNED - PRE-5C` |
| Manual Override Pushbutton | `TBD` | `INPUT` | 3.3V (Pull-Up) | Field technician physical preemption cancel | `ManualOverrideDriver` | `PLANNED - PRE-5C` |
| Vehicle IR Stop-Bar Sensor | `TBD` | `INPUT` | 3.3V (Level-Shifted) | Optical presence trigger at stop line | `IrSensorDriver` | `PLANNED - PRE-5C` |
| Ultrasonic Trigger Pin | `TBD` | `OUTPUT` | 3.3V TTL | 10µs ranging trigger pulse | `UltrasonicDriver` | `PLANNED - PRE-5C` |
| Ultrasonic Echo Pin | `TBD` | `INPUT` | 3.3V (Divider 5V->3.3V)| Measures distance time-of-flight | `UltrasonicDriver` | `PLANNED - PRE-5C` |
| Cabinet Door Tamper Switch | `TBD` | `INPUT` | 3.3V (Pull-Up) | Cabinet physical intrusion alarm | `SecurityDriver` | `PLANNED - PRE-5C` |
| Hardware Watchdog Strobe | `TBD` | `OUTPUT` | 3.3V Pulse | Periodic hardware heartbeat pulse (200ms) | `WatchdogDriver` | `PLANNED - PRE-5C` |

### 5.3 Ambulance Node (OBU) GPIO Pin Allocation Template
| Device / Function | GPIO (BCM) | Direction | Electrical Voltage | Description | Target Driver Class | Allocation Status |
|---|---|---|---|---|---|---|
| Vehicle Ignition Sense | `TBD` | `INPUT` | 3.3V (Opto 12V->3.3V)| Detects vehicle engine running state | `PowerMonitorDriver` | `PLANNED - PRE-5C` |
| Emergency Siren Active Sense | `TBD` | `INPUT` | 3.3V (Opto 12V->3.3V)| Detects vehicle acoustic siren activation | `VehicleSensorDriver` | `PLANNED - PRE-5C` |
| Emergency Lightbar Sense | `TBD` | `INPUT` | 3.3V (Opto 12V->3.3V)| Detects visual emergency lightbar status | `VehicleSensorDriver` | `PLANNED - PRE-5C` |
| Driver Mission Ack Button | `TBD` | `INPUT` | 3.3V (Pull-Up) | In-cab physical mission accept toggle | `OperatorUiDriver` | `PLANNED - PRE-5C` |
| Critical Vitals Alarm Buzzer | `TBD` | `OUTPUT` | 3.3V (Transistor Gate)| High-decibel audio alert on acute drop | `AlarmBuzzerDriver` | `PLANNED - PRE-5C` |
| System Health Status LED | `TBD` | `OUTPUT` | 3.3V (Current-Limited) | Green pulse indicating software loop OK | `StatusLedDriver` | `PLANNED - PRE-5C` |

---

## 6. Communication Interfaces

```
+-----------------------------------------------------------------------------------------------------------+
|                                    COMMUNICATION BUS TAXONOMY                                             |
+----------------+--------------------+--------------------------------+------------------------------------+
| Hardware Bus   | Target Peripheral  | Physical Device Node           | Software Driver / Protocol         |
+----------------+--------------------+--------------------------------+------------------------------------+
| UART           | GNSS Receiver      | /dev/ttyAMA0 (Ambulance OBU)   | SerialGpsSource (NMEA-0183 @ 9600) |
| I2C (Bus 1)    | OLED Display       | /dev/i2c-1 (Addr: 0x3C)        | OledDisplayDriver (SSD1306)        |
| I2C (Bus 1)    | 6-DOF IMU          | /dev/i2c-1 (Addr: 0x68)        | ImuSensorDriver (MPU6050 / EKF)    |
| SPI            | High-Speed ADC/WDT | /dev/spidev0.0                 | HardwareWatchdogDriver             |
| GPIO           | Relays & Sensors   | /dev/gpiod (Linux) or QNX resm | RaspberryPiGpioInterface           |
| Ethernet       | Backhaul Network   | eth0 (1000BASE-T RJ45)         | TCP/IP Stack -> MosquittoTransport |
| Wi-Fi (WLAN)   | Field Mesh / V2X   | wlan0 (802.11ac Ad-Hoc / UDP)  | SocketClient/Server (Port 8998/99) |
| MQTT (Logical) | Telemetry Broker   | Network Sockets (Port 8883)    | MqttClient via IMqttTransport      |
+----------------+--------------------+--------------------------------+------------------------------------+
```

### 6.1 UART (Universal Asynchronous Receiver-Transmitter)
* **Purpose:** Continuous ingestion of raw NMEA-0183 sentences (`$GPRMC`, `$GPGGA`, `$GPVTG`) from physical GNSS hardware.
* **Electrical Parameters:** 3.3V LVTTL Serial; 8 Data bits, No Parity, 1 Stop bit (8-N-1); Baud: 9600 bps baseline (upgradeable to 115200 bps).
* **Target Device Node:** `/dev/ttyAMA0` (PL011 hardware UART on Raspberry Pi) or `/dev/ttyUSB0`.
* **Future Driver Class:** `SerialGpsSource` (implements `IGpsSource`, feeding `GpsTracker`).

### 6.2 I2C (Inter-Integrated Circuit)
* **Purpose:** Multi-drop two-wire serial communication with dashboard displays and inertial measurement units.
* **Electrical Parameters:** 3.3V open-drain lines with 1.8kΩ pull-up resistors on SDA and SCL. Standard Mode (100 kHz) and Fast Mode (400 kHz).
* **Target Device Node:** `/dev/i2c-1`.
* **Future Driver Classes:** `OledDisplayDriver` (`0x3C`), `ImuSensorDriver` (`0x68`).

### 6.3 SPI (Serial Peripheral Interface)
* **Purpose:** High-throughput synchronous master-slave bus reserved for external hardware watchdog coprocessors or multi-channel ADC chips.
* **Electrical Parameters:** 3.3V 4-wire (MOSI, MISO, SCLK, CS0); clock speeds up to 10 MHz.
* **Target Device Node:** `/dev/spidev0.0`.
* **Future Driver Class:** `SpiWatchdogDriver`.

### 6.4 GPIO (General-Purpose Input/Output)
* **Purpose:** Direct logic-level control of opto-isolated relay triggers and pulse sensing for proximity sensors.
* **Target Device Interface:** Linux `libgpiod` character device interface (`/dev/gpiochip*`) avoiding deprecated sysfs. On QNX, custom resource manager (`/dev/gpio`).
* **Future Driver Class:** `RaspberryPiGpioInterface` (implements `IGpioInterface`).

### 6.5 Ethernet & Wi-Fi Networking
* **Purpose:** Mission-critical broker telemetry and direct ad-hoc vehicle-to-infrastructure datagrams.
* **Protocols:** IEEE 802.3 Gigabit Ethernet, IEEE 802.11ac dual-band wireless, and IEEE 802.11p/ITS-G5 ad-hoc UDP channels.
* **Target Software Classes:** `SocketClient`, `SocketServer`, `NetworkManager`.

### 6.6 MQTT Logical Bus
* **Purpose:** Standardized message distribution across all nodes over unified topics (`aegis::network::topics`).
* **Protocols:** MQTT v3.1.1 / v5.0 over TLS 1.3 with X.509 client certificate mutual authentication.
* **Target Software Class:** `MqttClient` delegating to `IMqttTransport`.

---

## 7. Hardware Abstraction Layers

To maintain complete independence from underlying hardware platforms, SMART AEGIS structures every physical subsystem behind abstract C++ interface classes.

### 7.1 Actuation Abstraction Layer (`IGpioInterface`)
The traffic controller logic and signal state machine interact strictly with `IGpioInterface`. The concrete driver is resolved at runtime via factory instantiation.

```
+-----------------------------------------------------------------------------+
|                              ACTUATION ARCHITECTURE                         |
+-----------------------------------------------------------------------------+
|                                                                             |
|                       +-------------------------------+                     |
|                       |    aegis::edge::traffic::     |                     |
|                       |       TrafficController       |                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                                       v                                     |
|                       +-------------------------------+                     |
|                       |         <<interface>>         |                     |
|                       |        IGpioInterface         |                     |
|                       |  + Initialize()               |                     |
|                       |  + SetRed() / SetYellow()     |                     |
|                       |  + SetGreen()                 |                     |
|                       |  + FlashYellow() / FlashRed() |                     |
|                       |  + AllOff() / Shutdown()      |                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                     +-----------------+-----------------+                   |
|                     |                                   |                   |
|                     v                                   v                   |
|   +-----------------------------------+   +-------------------------------+ |
|   |      SimulationGpioInterface      |   |   RaspberryPiGpioInterface    | |
|   |  - Logs state transitions to mem  |   |   - Operates /dev/gpiod       | |
|   |  - Zero hardware dependencies     |   |   - Drives physical opto-SSRs | |
|   |  - Active in Host / Unit Tests    |   |   - Active in Phase 5C Target | |
|   +-----------------------------------+   +-------------------------------+ |
|                                                                             |
+-----------------------------------------------------------------------------+
```

### 7.2 Transport Abstraction Layer (`IMqttTransport`)
The communication infrastructure decouples network pub/sub semantics from specific broker client libraries.

```
+-----------------------------------------------------------------------------+
|                              TRANSPORT ARCHITECTURE                         |
+-----------------------------------------------------------------------------+
|                                                                             |
|                       +-------------------------------+                     |
|                       |    aegis::network::MqttClient |                     |
|                       |    (Unified Application Facade|                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                                       v                                     |
|                       +-------------------------------+                     |
|                       |         <<interface>>         |                     |
|                       |        IMqttTransport         |                     |
|                       |  + Connect() / Disconnect()   |                     |
|                       |  + Publish() / Subscribe()    |                     |
|                       |  + SetOfflineBufferSize()     |                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                     +-----------------+-----------------+                   |
|                     |                                   |                   |
|                     v                                   v                   |
|   +-----------------------------------+   +-------------------------------+ |
|   |        SimulationTransport        |   |      MosquittoTransport       | |
|   |  - Wraps in-memory MessageRouter  |   |   - Wraps libmosquitto        | |
|   |  - Loopback exact & wildcard      |   |   - Manages physical TCP/TLS  | |
|   |  - Offline Ring Buffer (100 msgs) |   |   - Autonomous reconnect engine|
|   |  - Active in Simulation & Tests   |   |   - Active on Hardware SBCs   | |
|   +-----------------------------------+   +-------------------------------+ |
|                                                                             |
+-----------------------------------------------------------------------------+
```

### 7.3 Kinematic Positioning Abstraction Layer (`IGpsSource`)
The GPS tracking engine accepts kinematic streams from either a serial hardware receiver or a software route interpolation generator.

```
+-----------------------------------------------------------------------------+
|                              POSITIONING ARCHITECTURE                       |
+-----------------------------------------------------------------------------+
|                                                                             |
|                       +-------------------------------+                     |
|                       |    aegis::edge::ambulance::   |                     |
|                       |          GpsTracker           |                     |
|                       |  - NMEA Parser & Checksum     |                     |
|                       |  - EKF Dead-Reckoning Engine  |                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                                       v                                     |
|                       +-------------------------------+                     |
|                       |         <<interface>>         |                     |
|                       |          IGpsSource           |                     |
|                       |  + Open() / Close()           |                     |
|                       |  + ReadSentence()             |                     |
|                       |  + IsDataAvailable()          |                     |
|                       +---------------+---------------+                     |
|                                       |                                     |
|                     +-----------------+-----------------+                   |
|                     |                                   |                   |
|                     v                                   v                   |
|   +-----------------------------------+   +-------------------------------+ |
|   |       SimulationGpsSource         |   |        SerialGpsSource        | |
|   |  - Synthetic GeoJSON waypoints    |   |   - Opens /dev/ttyAMA0        | |
|   |  - Configurable kinematic speed   |   |   - POSIX termios 9600 8-N-1  | |
|   |  - Artificial satellite dropout   |   |   - Hardware serial buffering | |
|   +-----------------------------------+   +-------------------------------+ |
|                                                                             |
+-----------------------------------------------------------------------------+
```

---

## 8. Planned Driver Classes (Phase 5C & Beyond)

*Notice: This section outlines driver responsibilities, interfaces, and integration notes. In accordance with architectural directives, no executable C++ implementation code is generated herein.*

### 8.1 `RaspberryPiGpioInterface`
* **Subsystem / Tier:** Roadside Unit (RSU) / Traffic Signal Controller (`aegis::edge::traffic`).
* **Primary Responsibility:** Interfaces directly with physical BCM GPIO pins to actuate opto-isolated relay modules driving 12V traffic lamps (Red, Yellow, Green, and Flashing Yellow).
* **Interface Implemented:** `aegis::edge::traffic::IGpioInterface`.
* **Hardware & OS Dependencies:** Broadcom BCM2711 GPIO controller; Linux `libgpiod` character device API (`/dev/gpiochip*`) or QNX Resource Manager (`/dev/gpio`).
* **Implementation Notes:**
  1. Implements strict mutex protection across all pin operations.
  2. Ensures atomic transition between lamp states to prevent intermediate float states.
  3. Validates that `SetGreen()` cannot be asserted if the opposing approach green relay is energized (software assertion supplementing the physical interlock).
  4. On process termination, `Shutdown()` forces the fail-safe relay to drop to normally-closed flashing amber.

### 8.2 `SerialGpsSource`
* **Subsystem / Tier:** Ambulance On-Board Unit (OBU) / Positioning (`aegis::edge::ambulance`).
* **Primary Responsibility:** Manages the POSIX serial UART interface, configuring baud rates, non-blocking asynchronous reads, and delivering raw NMEA bitstreams to `GpsTracker`.
* **Interface Implemented:** `aegis::edge::ambulance::IGpsSource`.
* **Hardware & OS Dependencies:** u-blox NEO-M8N GNSS module; POSIX `termios` API via `/dev/ttyAMA0` or `/dev/ttyUSB0`.
* **Implementation Notes:**
  1. Configures raw mode (`cfmakeraw`), disabling software flow control (XON/XOFF) and local echo.
  2. Enforces ring-buffered line-based sentence extraction terminating on `\r\n`.
  3. Drops malformed sentences exceeding the 82-character maximum NMEA frame limit.

### 8.3 `IrSensorDriver`
* **Subsystem / Tier:** Roadside Unit (RSU) / Vehicle Presence (`aegis::edge::traffic`).
* **Primary Responsibility:** Samples the digital output of the optical TCRT5000 IR sensor at the intersection stop-bar to detect physical vehicle arrivals with sub-millisecond precision.
* **Interface Implemented:** `aegis::edge::traffic::IVehicleSensor`.
* **Hardware & OS Dependencies:** BCM GPIO input line configured with active-LOW interrupt edge detection via `gpiod_line_event_wait()`.
* **Implementation Notes:**
  1. Applies a 20ms software debounce filter to eliminate spurious optical transients.
  2. Emits an event callback directly into the preemption clearance evaluator.

### 8.4 `UltrasonicDriver`
* **Subsystem / Tier:** Roadside Unit (RSU) / Queue Estimation (`aegis::edge::traffic`).
* **Primary Responsibility:** Generates 10µs ultrasonic trigger pulses on the transmitter pin and measures the precise high-pulse duration on the echo pin using microsecond monotonic clocks.
* **Interface Implemented:** `aegis::edge::traffic::IQueueDetector`.
* **Hardware & OS Dependencies:** HC-SR04 / JSN-SR04T transducer; high-resolution POSIX clock (`clock_gettime(CLOCK_MONOTONIC)`).
* **Implementation Notes:**
  1. Calculates distance in centimeters: `Distance_cm = (PulseWidth_µs * 0.0343) / 2`.
  2. Implements a 30ms measurement timeout to prevent thread blocking on missed echo pulses.
  3. Clamps valid range between 2cm and 400cm; reports error status on out-of-bounds readings.

### 8.5 `OledDisplayDriver`
* **Subsystem / Tier:** Ambulance On-Board Unit (OBU) / Telematics UI (`aegis::edge::ambulance`).
* **Primary Responsibility:** Manages the SSD1306 128x64 graphic OLED display over I2C, rendering vehicle speed, upcoming intersection preemption status, and medical telemetry warnings.
* **Interface Implemented:** `aegis::edge::ambulance::IOledDisplay`.
* **Hardware & OS Dependencies:** Linux I2C user-space driver (`/dev/i2c-1`) utilizing `ioctl(I2C_SLAVE)`.
* **Implementation Notes:**
  1. Maintains a 1024-byte in-memory frame buffer (128x64 bits).
  2. Implements dirty-region transmission to optimize I2C bus bandwidth.
  3. Execution is throttled to 10 Hz to prevent starvation of critical kinematic tasks.

### 8.6 `WatchdogDriver`
* **Subsystem / Tier:** Safety Supervisor (`aegis::safety`).
* **Primary Responsibility:** Interfaces with the hardware watchdog timer peripheral, issuing periodic refresh strobes ("petting") as long as all critical software threads meet their execution deadlines.
* **Interface Implemented:** `aegis::safety::IWatchdogSupervisor`.
* **Hardware & OS Dependencies:** Linux kernel watchdog device (`/dev/watchdog`) or QNX High Availability Manager (HAM).
* **Implementation Notes:**
  1. Opens `/dev/watchdog` with a 2000ms hardware timeout budget.
  2. Pings the watchdog every 500ms only if the traffic controller FSM and safety monitor report healthy status.
  3. If a software deadlock occurs, the watchdog is deliberately starved, triggering a hardware reboot within 2000ms.

### 8.7 `PowerMonitorDriver`
* **Subsystem / Tier:** Safety Supervisor (`aegis::safety`).
* **Primary Responsibility:** Monitors SBC input voltage rail stability, vehicle ignition lines, and power supply health to initiate an orderly emergency state save prior to blackout.
* **Interface Implemented:** `aegis::safety::IPowerMonitor`.
* **Hardware & OS Dependencies:** I2C Voltage/Current Monitor (INA219) or opto-isolated ignition GPIO.
* **Implementation Notes:**
  1. Samples supply rail voltage at 5 Hz.
  2. If voltage drops below 4.75V DC for > 50ms, asserts an imminent brownout alarm, flushes in-memory store-and-forward telemetry to non-volatile flash, and prepares controllers for cold-iron shutdown.

---

## 9. Timing Requirements & Operational Budgets

The following timing budgets represent rigid functional safety boundaries. Execution jitter exceeding these thresholds constitutes a safety violation.

```
+-------------------------------------------------------------------------------------------------------+
|                                    OPERATIONAL TIMING SPECIFICATION                                   |
+------------------------------+------------------+------------------+------------------+---------------+
| Task / Operation             | Target Period    | Max Latency      | Jitter Tolerance | Failure Action|
+------------------------------+------------------+------------------+------------------+---------------+
| Traffic FSM Main Tick        | 100 ms (10 Hz)   | 10 ms            | ± 5 ms           | Flashing Amber|
| Coordinator Liveness Heartb. | 1000 ms (1 Hz)   | 50 ms            | ± 50 ms          | Heartbeat Warn|
| GNSS Kinematic Sampling      | 100 ms (10 Hz)   | 15 ms            | ± 10 ms          | EKF Fallback  |
| Patient Vitals Sampling      | 1000 ms (1 Hz)   | 50 ms            | ± 50 ms          | Telemetry Log |
| Acute Deterioration Alarm    | Event-Driven     | < 100 ms         | N/A (Immediate)  | Audio Alarm   |
| MQTT Telemetry Dispatch      | 100 ms (10 Hz)   | 20 ms            | ± 15 ms          | Offline Buffer|
| MQTT Heartbeat Watchdog Loss | 5000 ms (Timeout)| 5050 ms          | + 50 ms          | Flashing Yellow|
| IR Sensor Polling            | 50 ms (20 Hz)    | 5 ms             | ± 2 ms           | Fallback Time |
| Ultrasonic Queue Ping        | 100 ms (10 Hz)   | 35 ms            | ± 5 ms           | Default Queue |
| Preemption Yellow Clearance  | 3000 ms (Fixed)  | Deterministic    | - 0 ms / + 100 ms| Hard Safety   |
| Preemption All-Red Clearance | 1000 ms (Fixed)  | Deterministic    | - 0 ms / + 100 ms| Hard Safety   |
| Post-Clearance Recovery Hold | 4000 ms (Fixed)  | Deterministic    | - 0 ms / + 100 ms| Normal Cycle  |
| Hardware WDT Refresh Pulse   | 500 ms (2 Hz)    | 10 ms            | ± 20 ms          | Hard SoC Reset|
+------------------------------+------------------+------------------+------------------+---------------+
```

---

## 10. Safety and Fail-Safe Behaviour

```
                                  +-----------------------------+
                                  |     NORMAL CYCLE STATE      |
                                  |   (FSM: RED-GREEN-YELLOW)   |
                                  +--------------+--------------+
                                                 |
                   +-----------------------------+-----------------------------+
                   |                             |                             |
      Coordinator Heartbeat Lost           MQTT Broker Loss             Hardware Fault /
          (Timeout > 5000ms)             (Network Partition)           Watchdog Starvation
                   |                             |                             |
                   v                             v                             v
     +---------------------------+ +---------------------------+ +---------------------------+
     |   AUTONOMOUS LOCAL FSM    | |   STORE-AND-FORWARD MODE  | |  COLD-IRON RELAY FALLBACK |
     |  - Continues local cycle  | |  - Cycle continues locally| |  - Watchdog de-energizes  |
     |  - Drops to FLASHING_YEL  | |  - Buffers 100 msgs in RAM| |  - Hardware forces all-red|
     |    upon cross conflict    | |  - Zero network blocking  | |    or Flashing Amber      |
     +-------------+-------------+ +-------------+-------------+ +---------------------------+
                   |                             |
                   +--------------+--------------+
                                  |
                      Communication Restored
                                  |
                                  v
                   +-----------------------------+
                   |      SAFE RECOVERY CYCLE    |
                   |  - Forces full RED hold     |
                   |  - Re-syncs coordinator     |
                   |  - Resumes Normal Operation |
                   +-----------------------------+
```

### 10.1 Coordinator Disconnection & Network Partition
* **Trigger:** Absence of MQTT heartbeat frames on `aegis/intersection/{id}/commands` for greater than 5000ms.
* **Deterministic Action:**
  1. Traffic Signal Node logs `CRITICAL` communication timeout event.
  2. If an emergency preemption corridor is currently active, it is immediately cancelled via `RecoveryManager`.
  3. Node transitions directly to **Degraded Autonomous Mode** (`FLASHING_YELLOW`).
  4. Local actuation continues indefinitely without relying on remote network input.

### 10.2 Power Interruption & Brownout
* **Trigger:** Supply voltage on 5V rail drops below 4.75V DC.
* **Deterministic Action:**
  1. Primary electromechanical / solid-state relays de-energize instantly.
  2. Hardware normally-closed (NC) wiring paths mechanically connect the yellow lamp circuit to an autonomous analog flasher relay or extinguish all green indications.
  3. No combination of software crashes can leave a green light energized when control power fails.

### 10.3 Sensor & Peripheral Failure
* **GNSS Fix Loss:** If satellite fix quality drops to `0` (Invalid) or HDOP exceeds `4.0`, the `GpsTracker` automatically activates its Extended Kalman Filter (EKF) dead-reckoning engine, using IMU integration to maintain valid location tracking for up to 30 seconds.
* **IR / Ultrasonic Failure:** If ranging sensor timeouts persist for > 3 consecutive cycles, the `TrafficController` substitutes dynamic queue measurements with worst-case default clearance times (5.0s yellow, 2.0s all-red).

### 10.4 Conflicting Green Prevention (Hardware Interlock)
* **Design Rule:** Simultaneous green indications across conflicting approaches are categorically prohibited.
* **Dual Protection:**
  1. *Software FSM Interlock:* `SignalStateMachine` rejects invalid transitions (`GREEN -> RED` without Yellow; simultaneous green).
  2. *Hardware Interlock:* Physical SPDT relay contacts are wired in series-complementary topology: the electrical power to Approach 2 Green physically passes through the normally-closed contact of Approach 1 Green relay. It is electrically impossible for both greens to illuminate simultaneously.

---

## 11. Network Architecture

### 11.1 Network Topology & Addressing
The SMART AEGIS physical network is arranged as an isolated private Class C subnet (`192.168.10.0/24`) secured against external unauthorized broadcast traffic.

```
+---------------------------------------------------------------------------------------------------+
|                                  PHYSICAL NETWORK IP TOPOLOGY                                     |
+---------------------------------------------------------------------------------------------------+
|  Host Device                  | IP Address       | Physical Interface   | Open Service Ports      |
+-------------------------------+------------------+----------------------+-------------------------+
|  Central MQTT Broker Host     | 192.168.10.100   | 1000BASE-T Ethernet  | 8883 (MQTTS), 443 (REST)|
|  Central Coordinator Host     | 192.168.10.101   | 1000BASE-T Ethernet  | 50050 (Internal RPC)    |
|  Ambulance OBU Node 1         | 192.168.10.111   | 802.11ac / LTE WLAN  | 8999 (V2V UDP Broadcast)|
|  Ambulance OBU Node 2         | 192.168.10.112   | 802.11ac / LTE WLAN  | 8999 (V2V UDP Broadcast)|
|  Traffic Signal RSU Node 1    | 192.168.10.201   | 1000BASE-T Ethernet  | 8998 (V2I UDP Preempt)  |
|  Traffic Signal RSU Node 2    | 192.168.10.202   | 1000BASE-T Ethernet  | 8998 (V2I UDP Preempt)  |
|  Hospital Intake Dashboard    | 192.168.10.220   | 1000BASE-T Ethernet  | 9001 (WebSockets)       |
+-------------------------------+------------------+----------------------+-------------------------+
```

### 11.2 Unified MQTT Topic Namespace (`aegis::network::topics`)
All nodes strictly publish and subscribe according to the unified topic hierarchy established in the Pre-5C refinement:

```
+---------------------------------------------------------------------------------------------------+
|                                 UNIFIED MQTT TOPIC HIERARCHY                                      |
+--------------------------------------+-------------+---------------+------------------------------+
| Topic Pattern                        | Direction   | QoS Level     | Payload Type / Schema        |
+--------------------------------------+-------------+---------------+------------------------------+
| aegis/intersection/{id}/commands     | Coord -> RSU| QoS 1 (Ack)   | IntersectionCommandMsg       |
| aegis/intersection/{id}/status       | RSU -> Coord| QoS 1 (Ack)   | IntersectionStatusMsg        |
| aegis/ambulance/{id}/location        | OBU -> Coord| QoS 0 (Fast)  | AmbulanceLocationMsg         |
| aegis/ambulance/{id}/vitals          | OBU -> Hosp | QoS 0 (Fast)  | PatientVitalsTelemetryMsg    |
| aegis/ambulance/{id}/alarms          | OBU -> All  | QoS 2 (Exact) | PatientVitalsAlarmMsg        |
| aegis/ambulance/{id}/heartbeat       | OBU -> Coord| QoS 0 (Fast)  | Node Heartbeat Frame         |
| aegis/ambulance/{id}/state           | OBU -> Coord| QoS 1 (Ack)   | AmbulanceStatusMsg           |
| aegis/hospital/{id}/commands         | Coord -> Hosp| QoS 1 (Ack)  | Hospital Intake Advisory     |
| aegis/emergency/request              | PSAP -> Coord| QoS 1 (Ack)  | Emergency Incident Intake    |
| aegis/incident/{id}/dispatched       | Coord -> All| QoS 1 (Ack)   | Incident Dispatch Broadcast  |
+--------------------------------------+-------------+---------------+------------------------------+
```

---

## 12. Power Requirements & Electrical Isolation

```
+-----------------------------------------------------------------------------------+
|                       POWER DISTRIBUTION SCHEMATIC (RSU / OBU)                    |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [ 12V DC Mains / Vehicle Battery ]                                               |
|                 |                                                                 |
|                 +--------------------+                                            |
|                 | 10A Master Fuse    |                                            |
|                 +---------+----------+                                            |
|                           |                                                       |
|             +-------------+-------------+                                         |
|             |                           |                                         |
|             v                           v                                         |
|  +---------------------+     +---------------------+                              |
|  | Isolated Buck Conv. |     | Relay Power Rail    |                              |
|  | 12V -> 5.1V @ 4A    |     | 12V DC Regulated    |                              |
|  +----------+----------+     +----------+----------+                              |
|             |                           |                                         |
|             v                           v                                         |
|  +---------------------+     +---------------------+                              |
|  | Raspberry Pi 4B SBC |     | 4-Channel Opto SSR  |                              |
|  | 5.1V USB-C Logic    |     | High-Current Relay  |                              |
|  +----------+----------+     +----------+----------+                              |
|             |                           |                                         |
|      (3.3V Logic Pins)                  | (12V Switched Power)                    |
|             |                           |                                         |
|             +--------[ PC817 ]----------+                                         |
|                   Optocoupler                                                     |
|                 (5000V Isolation)                                                 |
|                                         |                                         |
|                                         v                                         |
|                              [ Traffic Signal LEDs ]                              |
|                                (Red, Yellow, Green)                               |
|                                                                                   |
+-----------------------------------------------------------------------------------+
```

### 12.1 Electrical Isolation Specifications
1. **Galvanic Isolation:** The digital logic ground of the Raspberry Pi 4B must remain strictly separated from the 12V actuator supply ground. Communication between the Pi and the relays must traverse optical barriers (PC817 optocouplers).
2. **Inductive Surge Suppression:** All electromechanical relay coils and long wiring runs to signal heads must be fitted with fast-recovery flyback diodes (1N4007 or TVS diodes) to clamp inductive back-EMF spikes.
3. **Fuse Protection:**
   * Main 12V DC bus: 5.0A fast-acting ceramic fuse.
   * 5V DC SBC supply: 3.5A resettable Polyfuse (PTC).
   * LED lamp channels: 1.0A individual inline blade fuses.
4. **Single-Point Earth Grounding:** All metallic equipment chassis must bond to a single common earth grounding stud (Star Ground topology) to eliminate ground loops and EMI interference with the GNSS receiver.

---

## 13. Deployment Architecture

### 13.1 Filesystem Hierarchy on Target Hardware
```
/opt/aegis/
├── bin/
│   ├── aegis_coordinator          # Coordinator application daemon
│   ├── aegis_traffic_controller   # Traffic Signal RSU application daemon
│   └── aegis_ambulance            # Ambulance OBU application daemon
├── lib/                           # Shared runtime dependencies (libmosquitto, libc++)
└── scripts/
    ├── start_traffic.sh           # Safe startup script with GPIO pre-flight checks
    └── emergency_halt.sh          # Manual recovery and safe relay kill script

/etc/aegis/
├── config.json                    # Active node configuration profile
├── network_topology.json          # Node IP mappings and broker endpoints
└── tls/
    ├── ca.crt                     # Private Root Certificate Authority
    ├── node.crt                   # Node device public certificate
    └── node.key                   # Node device private key (chmod 0400)

/var/log/aegis/                    # High-endurance circular log partition
├── aegis_system.log               # Unified operational trace log
└── safety_audit.log               # Tamper-evident safety transition audit trail
```

### 13.2 Systemd Service Units
Every SMART AEGIS node executes under dedicated Linux `systemd` supervisors ensuring automatic startup, watchdog integration, and crash recovery.

#### Example Service Definition: `aegis-traffic.service`
```ini
[Unit]
Description=SMART AEGIS Roadside Traffic Signal Controller Daemon
After=network-online.target systemd-time-wait-sync.target
Wants=network-online.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/aegis
ExecStart=/opt/aegis/bin/aegis_traffic_controller --config /etc/aegis/config.json
Restart=always
RestartSec=2s
KillMode=process
WatchdogSec=2s
NotifyAccess=main
StandardOutput=journal
StandardError=journal
LimitMEMLOCK=infinity
CPUSchedulingPolicy=rr
CPUSchedulingPriority=80

[Install]
WantedBy=multi-user.target
```

### 13.3 Read-Only Root Filesystem Strategy
To eliminate flash storage corruption resulting from abrupt vehicle battery disconnects or roadside power cuts:
* The Raspberry Pi target OS (Raspbian / Yocto Linux) is mounted with a **Read-Only Root Filesystem (`ro`)**.
* Dynamic runtime state (`/var/run`, `/tmp`) utilizes volatile RAM disks (`tmpfs`).
* Persistent logging is directed to an endurance-rated, journaled partition with atomic write barriers.

---

## 14. Hardware Testing Plan

Prior to authorizing Phase 5C implementation code, the physical testbed must execute the following hardware verification protocols:

```
+---------------------------------------------------------------------------------------------------+
|                                  HARDWARE VERIFICATION SUITE                                      |
+-----------+-------------------------+-----------------------------------+-------------------------+
| Test ID   | Focus Area              | Verification Procedure            | Acceptance Standard     |
+-----------+-------------------------+-----------------------------------+-------------------------+
| HT-GPIO-01| Static Logic Levels     | Measure voltage on all output BCM | High: 3.3V ± 0.1V       |
|           |                         | pins under no-load and 10mA load  | Low:  0.0V ± 0.05V      |
| HT-RLY-01 | Relay Switching Latency | Measure delay between GPIO assert | Actuation delay < 15ms  |
|           |                         | and 12V lamp contact closure (DSO)| Bounce duration < 3ms   |
| HT-LMP-01 | Dual-Green Interlock    | Force simultaneous green inputs to| Zero occurrence: circuit|
|           |                         | opto-isolators electrically       | locks out conflicting gr|
| HT-GPS-01 | NMEA Serial Throughput  | Ingest 10 Hz stream over UART     | 10 pkts/s sustained;    |
|           |                         | for 60 minutes continuously       | zero checksum failures  |
| HT-SNR-01 | Ultrasonic Ranging Acc. | Measure stepped targets at 500mm, | Measurement error < ±5mm|
|           |                         | 1000mm, 2000mm, and 3000mm        | Timeout recovery < 35ms |
| HT-NET-01 | MQTT Network Reconnect  | Disconnect physical Ethernet wire | Queue retains 100 msgs; |
|           |                         | for 30 seconds during preemption  | reconnect time < 1.5s   |
| HT-PWR-01 | Cold Power Interruption | Abruptly disconnect 12V supply    | Relays de-energize; zero|
|           |                         | 100 consecutive times during run  | filesystem corruption   |
| HT-WDT-01 | Hardware WDT Starvation | Deliberately freeze control loop  | SoC hardware resets     |
|           |                         | via test command                  | within 2000ms ± 100ms   |
+-----------+-------------------------+-----------------------------------+-------------------------+
```

---

## 15. Future Expansion & Technology Reservations

The SMART AEGIS hardware architecture includes reserved interfaces and expansion headroom for advanced intelligent transportation technologies:

1. **Cellular V2X (C-V2X / 5G NR Sidelink PC5):** Dedicated expansion headers accommodate Quectel or Telit C-V2X modules, providing direct vehicle-to-infrastructure communication over the 5.9 GHz ITS band without cellular tower dependence.
2. **Sub-GHz Long-Range Telemetry (LoRa / SX1262):** Reserved SPI/UART slot for long-range municipal telemetry, enabling preemption triggers in rural or mountain terrain where cellular coverage is unavailable.
3. **Edge AI Vision Accelerator (Google Coral / Hailo-8):** USB 3.0 / PCIe slot reserved for dedicated neural processing units (NPUs) running real-time YOLO vehicle queue detection from existing roadside CCTV cameras.
4. **Autonomous Emergency Vehicle Interlocks:** High-speed CAN bus interface (MCP2515 via SPI) to integrate directly with vehicle steering, braking, and emergency warning lightbars.
5. **Direct Hospital Electronic Health Record (EHR) Bridge:** Secure REST/FHIR gateway to automatically inject pre-arrival vital trends into Epic / Cerner hospital databases prior to patient bay arrival.

---

## 16. Traceability Matrix

This matrix maps physical hardware components through software interfaces, driver implementations, application modules, and formal verification tests.

```
+---------------------------------------------------------------------------------------------------------------------------------+
|                                                 ENGINEERING TRACEABILITY MATRIX                                                 |
+---------------------+-----------------------+--------------------------+-----------------------+--------------------------------+
| Hardware Component  | Software Interface    | Concrete Driver Class    | Consuming Module      | Verification Test Identifier   |
+---------------------+-----------------------+--------------------------+-----------------------+--------------------------------+
| 4-Ch Relay Board    | IGpioInterface        | RaspberryPiGpioInterface | TrafficController     | HT-GPIO-01, HT-RLY-01, UT-SIG-01|
| Traffic Lamp Heads  | IGpioInterface        | RaspberryPiGpioInterface | SignalStateMachine    | HT-LMP-01, HT-RLY-01, IT-SIG-01|
| u-blox GNSS Module  | IGpsSource            | SerialGpsSource          | GpsTracker            | HT-GPS-01, UT-GPS-01, IT-GPS-01|
| 6-DOF IMU Sensor    | IImuSource            | ImuSensorDriver          | GpsTracker (EKF)      | UT-GPS-02, IT-GPS-02           |
| SSD1306 OLED Display| IOledDisplay          | OledDisplayDriver        | AmbulanceNode         | HT-UI-01, IT-AMB-01            |
| TCRT5000 IR Sensor  | IVehicleSensor        | IrSensorDriver           | PreemptionManager     | HT-SNR-01, IT-SIG-02           |
| HC-SR04 Ultrasonic  | IQueueDetector        | UltrasonicDriver         | TrafficSignalNode     | HT-SNR-01, UT-SIG-02           |
| BCM2711 Watchdog    | IWatchdogSupervisor   | WatchdogDriver           | SafetyMonitor         | HT-WDT-01, UT-SAF-01           |
| 1000BASE-T Ethernet | IMqttTransport        | MosquittoTransport       | NetworkManager        | HT-NET-01, UT-NET-01, IT-NET-01|
| 12V Buck Regulator  | IPowerMonitor         | PowerMonitorDriver       | SafetyMonitor         | HT-PWR-01, UT-SAF-02           |
+---------------------+-----------------------+--------------------------+-----------------------+--------------------------------+
```

---

## 17. Engineering Risk Analysis & Mitigation Matrix

```
+----------------------------------------------------------------------------------------------------------------------------------+
|                                                  ENGINEERING RISK ASSESSMENT                                                     |
+---------+-----------------------------+---------+------------+----------+--------------------------------------------------------+
| Risk ID | Hazard Description          | Severity| Likelihood | Risk Lvl | Engineering Mitigation Strategy                        |
+---------+-----------------------------+---------+------------+----------+--------------------------------------------------------+
| RSK-E01 | Inductive back-EMF spikes   | CRITICAL| HIGH       | HIGH     | Optical isolation via PC817 optocouplers; fast-recovery|
|         | from relays damage SBC GPIO |         |            |          | flyback diodes across all inductive coil terminals.    |
| RSK-E02 | Ground loops induce noise   | HIGH    | MEDIUM     | HIGH     | Star-ground topology separating digital logic ground   |
|         | on analog/GNSS signals      |         |            |          | from high-current actuator power ground.               |
| RSK-E03 | SD card filesystem corrupt. | CRITICAL| HIGH       | HIGH     | Mount target OS with Read-Only Root Filesystem (ro);   |
|         | from abrupt power cut-off   |         |            |          | direct logs to volatile tmpfs and journaled eMMC.      |
| RSK-E04 | Thermal throttling on Pi 4B | HIGH    | HIGH       | HIGH     | Fit CNC aluminum armor heatsink with active PWM fan;   |
|         | in sealed roadside cabinet  |         |            |          | configure QNX/Linux thermal governor alerts at 70°C.   |
| RSK-E05 | Relay contact welding traps | CRITICAL| LOW        | HIGH     | Physical hardware series-complementary interlock wiring|
|         | intersection in Green state |         |            |          | prevents opposing greens regardless of relay welds.    |
| RSK-E06 | GNSS multipath reflections  | HIGH    | HIGH       | HIGH     | 6-DOF IMU Extended Kalman Filter dead-reckoning engine |
|         | in dense urban canyons      |         |            |          | bridges positioning dropouts up to 30 seconds.         |
| RSK-E07 | GPIO race conditions across | HIGH    | MEDIUM     | MEDIUM   | All pin read/write calls serialized behind std::mutex  |
|         | concurrent controller tasks |         |            |          | inside concrete RaspberryPiGpioInterface class.        |
+---------+-----------------------------+---------+------------+----------+--------------------------------------------------------+
```

---

## 18. Appendices

### Appendix A: Glossary of Terms
* **Actuation Plant:** The collection of physical electromechanical relays, solid-state switches, and lamp heads that govern vehicle right-of-way.
* **Dead-Reckoning:** The mathematical process of calculating current position based upon previously determined position and advancing that position based upon estimated speed, elapsed time, and gyro course.
* **Fail-Safe Mode:** A system operating condition designed to automatically trigger upon critical malfunction to prevent catastrophic collisions (in SMART AEGIS: Flashing Yellow or All-Red).
* **Galvanic Isolation:** The principle of isolating functional sections of electrical systems to prevent direct current flow while allowing data and energy exchange via optical, capacitive, or magnetic means.
* **On-Board Unit (OBU):** The computing and communication system permanently installed within the emergency response vehicle.
* **Roadside Unit (RSU):** The field controller deployed within the municipal traffic cabinet at each signalized road intersection.

### Appendix B: Abbreviations & Acronyms
* **ASIL:** Automotive Safety Integrity Level (ISO 26262)
* **BCM:** Broadcom Corporation (referring to Raspberry Pi SoC GPIO numbering)
* **BSM:** Basic Safety Message (SAE J2735)
* **CEP:** Circular Error Probable (GNSS accuracy metric)
* **EKF:** Extended Kalman Filter
* **FSM:** Finite State Machine
* **GNSS:** Global Navigation Satellite System (GPS, GLONASS, Galileo)
* **HAD:** Hardware Abstraction Document
* **HAL:** Hardware Abstraction Layer
* **HDOP:** Horizontal Dilution of Precision
* **ICD:** Interface Control Document
* **IMU:** Inertial Measurement Unit
* **ITS:** Intelligent Transportation Systems
* **LVTTL:** Low-Voltage Transistor-Transistor Logic (3.3V)
* **OBU:** On-Board Unit (Vehicular)
* **OLED:** Organic Light-Emitting Diode
* **QoS:** Quality of Service (MQTT)
* **RSU:** Roadside Unit (Intersection)
* **RTOS:** Real-Time Operating System
* **SSR:** Solid-State Relay
* **WDT:** Watchdog Timer

### Appendix C: Hardware Assumptions & Operational Envelopes
* **Operating Ambient Temperature:** -20°C to +70°C (Industrial temperature rating required for cabinet hardware).
* **Relative Humidity:** 5% to 95% non-condensing.
* **Primary Power Supply Stability:** 12V DC ± 10% under full 5A transient switching loads.
* **Vehicle System Voltage:** 12V automotive battery bus with cold-crank voltage dipping to no lower than 9.0V DC.

### Appendix D: Baseline References & Standards
1. [`SYSTEM_ARCHITECTURE.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/SYSTEM_ARCHITECTURE.md): Architectural topology, 19 single-responsibility modules, and interface definitions.
2. [`DEVELOPMENT_PLAN.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/DEVELOPMENT_PLAN.md): 15-phase engineering roadmap and safety requirements.
3. [`INTERFACE_SPECIFICATION.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/INTERFACE_SPECIFICATION.md): Formal Interface Control Document (ICD) and schemas.
4. [`AI_RULEBOOK.md`](file:///Users/revanthmacharya/Education/NHCE/Hackathons/QNX%20hackathon/AERS/AI_RULEBOOK.md): Mandatory AI engineering constitution, C++17 rules, and safety policies.
5. **ISO 26262:** Road Vehicles — Functional Safety.
6. **IEEE 802.11p / SAE J2735:** Dedicated Short Range Communications (DSRC) Message Set Dictionary.
7. **NEMA TS 2:** Traffic Controller Assemblies with NTCIP Requirements.
